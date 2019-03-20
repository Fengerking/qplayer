#define UNCHECKED_BITSTREAM_READER 1

#include "ttAvassert.h"
#include "ttDisplay.h"
#include "ttImgutils.h"
#include "ttOpt.h"
#include "ttInternal.h"
#include "ttInternal.h"
#include "ttCabac.h"
#include "ttCabacFunctions.h"
#include "ttErrorResilience.h"
#include "ttAvcodec.h"
#include "ttH264.h"
#include "ttH264data.h"
#include "ttH264chroma.h"
#include "ttH264Mvpred.h"
#include "ttGolomb.h"
#include "ttMathops.h"
#include "ttMpegutils.h"
#include "ttThread.h"

#include <assert.h>

const uint16_t tt_h264_mb_sizes[4] = { 256, 384, 512, 768 };

static void h264_er_decode_mb(void *opaque, int ref, int mv_dir, int mv_type,
                              int (*mv)[2][4][2],
                              int mb_x, int mb_y, int mb_intra, int mb_skipped)
{
    H264Context *h = opaque;

    h->mb_x  = mb_x;
    h->mb_y  = mb_y;
    h->mb_xy = mb_x + mb_y * h->mb_stride;
    memset(h->non_zero_count_cache, 0, sizeof(h->non_zero_count_cache));
    ttv_assert1(ref >= 0);
    /* FIXME: It is possible albeit uncommon that slice references
     * differ between slices. We take the easy approach and ignore
     * it for now. If this turns out to have any relevance in
     * practice then correct remapping should be added. */
    if (ref >= h->ref_count[0])
        ref = 0;
    if (!h->ref_list[0][ref].f.data[0]) {
        ttv_log(h->avctx, TTV_LOG_DEBUG, "Reference not available for error concealing\n");
        ref = 0;
    }
    if ((h->ref_list[0][ref].reference&3) != 3) {
        ttv_log(h->avctx, TTV_LOG_DEBUG, "Reference invalid\n");
        return;
    }
    fill_rectangle(&h->cur_pic.ref_index[0][4 * h->mb_xy],
                   2, 2, 2, ref, 1);
    fill_rectangle(&h->ref_cache[0][scan8[0]], 4, 4, 8, ref, 1);
    fill_rectangle(h->mv_cache[0][scan8[0]], 4, 4, 8,
                   pack16to32((*mv)[0][0][0], (*mv)[0][0][1]), 4);
    h->mb_mbaff =
    h->mb_field_decoding_flag = 0;
    tt_h264_hl_decode_mb(h);
}

void tt_h264_draw_horiz_band(H264Context *h, int y, int height)
{
    TTCodecContext *avctx = h->avctx;
    TTFrame *cur  = &h->cur_pic.f;
    TTFrame *last = h->ref_list[0][0].f.data[0] ? &h->ref_list[0][0].f : NULL;
    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(avctx->pix_fmt);
    int vshift = desc->log2_chroma_h;
    const int field_pic = h->picture_structure != PICT_FRAME;
    if (field_pic) {
        height <<= 1;
        y      <<= 1;
    }

    height = FFMIN(height, avctx->height - y);

    if (field_pic && h->first_field && !(avctx->slice_flags & SLICE_FLAG_ALLOW_FIELD))
        return;

    if (avctx->draw_horiz_band) {
        TTFrame *src;
        int offset[TTV_NUM_DATA_POINTERS];
        int i;

        if (cur->pict_type == TTV_PICTURE_TYPE_B || h->low_delay ||
            (avctx->slice_flags & SLICE_FLAG_CODED_ORDER))
            src = cur;
        else if (last)
            src = last;
        else
            return;

        offset[0] = y * src->linesize[0];
        offset[1] =
        offset[2] = (y >> vshift) * src->linesize[1];
        for (i = 3; i < TTV_NUM_DATA_POINTERS; i++)
            offset[i] = 0;

        emms_c();

        avctx->draw_horiz_band(avctx, src, offset,
                               y, h->picture_structure, height);
    }
}

/**
 * Check if the top & left blocks are available if needed and
 * change the dc mode so it only uses the available blocks.
 */
int tt_h264_check_intra4x4_pred_mode(H264Context *h)
{
    static const int8_t top[12] = {
        -1, 0, LEFT_DC_PRED, -1, -1, -1, -1, -1, 0
    };
    static const int8_t left[12] = {
        0, -1, TOP_DC_PRED, 0, -1, -1, -1, 0, -1, DC_128_PRED
    };
    int i;

    if (!(h->top_samples_available & 0x8000)) {
        for (i = 0; i < 4; i++) {
            int status = top[h->intra4x4_pred_mode_cache[scan8[0] + i]];
            if (status < 0) {
                ttv_log(h->avctx, TTV_LOG_ERROR,
                       "top block unavailable for requested intra4x4 mode %d at %d %d\n",
                       status, h->mb_x, h->mb_y);
                return TTERROR_INVALIDDATA;
            } else if (status) {
                h->intra4x4_pred_mode_cache[scan8[0] + i] = status;
            }
        }
    }

    if ((h->left_samples_available & 0x8888) != 0x8888) {
        static const int mask[4] = { 0x8000, 0x2000, 0x80, 0x20 };
        for (i = 0; i < 4; i++)
            if (!(h->left_samples_available & mask[i])) {
                int status = left[h->intra4x4_pred_mode_cache[scan8[0] + 8 * i]];
                if (status < 0) {
                    ttv_log(h->avctx, TTV_LOG_ERROR,
                           "left block unavailable for requested intra4x4 mode %d at %d %d\n",
                           status, h->mb_x, h->mb_y);
                    return TTERROR_INVALIDDATA;
                } else if (status) {
                    h->intra4x4_pred_mode_cache[scan8[0] + 8 * i] = status;
                }
            }
    }

    return 0;
} // FIXME cleanup like tt_h264_check_intra_pred_mode

/**
 * Check if the top & left blocks are available if needed and
 * change the dc mode so it only uses the available blocks.
 */
int tt_h264_check_intra_pred_mode(H264Context *h, int mode, int is_chroma)
{
    static const int8_t top[4]  = { LEFT_DC_PRED8x8, 1, -1, -1 };
    static const int8_t left[5] = { TOP_DC_PRED8x8, -1,  2, -1, DC_128_PRED8x8 };

    if (mode > 3U) {
        ttv_log(h->avctx, TTV_LOG_ERROR,
               "out of range intra chroma pred mode at %d %d\n",
               h->mb_x, h->mb_y);
        return TTERROR_INVALIDDATA;
    }

    if (!(h->top_samples_available & 0x8000)) {
        mode = top[mode];
        if (mode < 0) {
            ttv_log(h->avctx, TTV_LOG_ERROR,
                   "top block unavailable for requested intra mode at %d %d\n",
                   h->mb_x, h->mb_y);
            return TTERROR_INVALIDDATA;
        }
    }

    if ((h->left_samples_available & 0x8080) != 0x8080) {
        mode = left[mode];
        if (mode < 0) {
            ttv_log(h->avctx, TTV_LOG_ERROR,
                   "left block unavailable for requested intra mode at %d %d\n",
                   h->mb_x, h->mb_y);
            return TTERROR_INVALIDDATA;
        }
        if (is_chroma && (h->left_samples_available & 0x8080)) {
            // mad cow disease mode, aka MBAFF + constrained_intra_pred
            mode = ALZHEIMER_DC_L0T_PRED8x8 +
                   (!(h->left_samples_available & 0x8000)) +
                   2 * (mode == DC_128_PRED8x8);
        }
    }

    return mode;
}

const uint8_t *tt_h264_decode_nal(H264Context *h, const uint8_t *src,
                                  int *dst_length, int *consumed, int length)
{
    int i, si, di;
    uint8_t *dst;
    int bufidx;

    // src[0]&0x80; // forbidden bit
    h->nal_ref_idc   = src[0] >> 5;
    h->nal_unit_type = src[0] & 0x1F;

    src++;
    length--;

#define STARTCODE_TEST                                                  \
    if (i + 2 < length && src[i + 1] == 0 && src[i + 2] <= 3) {         \
        if (src[i + 2] != 3 && src[i + 2] != 0) {                       \
            /* startcode, so we must be past the end */                 \
            length = i;                                                 \
        }                                                               \
        break;                                                          \
    }

#if HAVE_FAST_UNALIGNED
#define FIND_FIRST_ZERO                                                 \
    if (i > 0 && !src[i])                                               \
        i--;                                                            \
    while (src[i])                                                      \
        i++

#if HAVE_FAST_64BIT
    for (i = 0; i + 1 < length; i += 9) {
        if (!((~TTV_RN64A(src + i) &
               (TTV_RN64A(src + i) - 0x0100010001000101ULL)) &
              0x8000800080008080ULL))
            continue;
        FIND_FIRST_ZERO;
        STARTCODE_TEST;
        i -= 7;
    }
#else
    for (i = 0; i + 1 < length; i += 5) {
        if (!((~TTV_RN32A(src + i) &
               (TTV_RN32A(src + i) - 0x01000101U)) &
              0x80008080U))
            continue;
        FIND_FIRST_ZERO;
        STARTCODE_TEST;
        i -= 3;
    }
#endif
#else
    for (i = 0; i + 1 < length; i += 2) {
        if (src[i])
            continue;
        if (i > 0 && src[i - 1] == 0)
            i--;
        STARTCODE_TEST;
    }
#endif

    // use second escape buffer for inter data
    bufidx = h->nal_unit_type == NAL_DPC ? 1 : 0;

    ttv_fast_padded_malloc(&h->rbsp_buffer[bufidx], &h->rbsp_buffer_size[bufidx], length+MAX_MBPAIR_SIZE);
    dst = h->rbsp_buffer[bufidx];

    if (!dst)
        return NULL;

    if(i>=length-1){ //no escaped 0
        *dst_length= length;
        *consumed= length+1; //+1 for the header
        if(h->avctx->flags2 & CODEC_FLAG2_FAST){
            return src;
        }else{
            memcpy(dst, src, length);
            return dst;
        }
    }

    memcpy(dst, src, i);
    si = di = i;
    while (si + 2 < length) {
        // remove escapes (very rare 1:2^22)
        if (src[si + 2] > 3) {
            dst[di++] = src[si++];
            dst[di++] = src[si++];
        } else if (src[si] == 0 && src[si + 1] == 0 && src[si + 2] != 0) {
            if (src[si + 2] == 3) { // escape
                dst[di++]  = 0;
                dst[di++]  = 0;
                si        += 3;
                continue;
            } else // next start code
                goto nsc;
        }

        dst[di++] = src[si++];
    }
    while (si < length)
        dst[di++] = src[si++];

nsc:
    memset(dst + di, 0, TT_INPUT_BUFFER_PADDING_SIZE);

    *dst_length = di;
    *consumed   = si + 1; // +1 for the header
    /* FIXME store exact number of bits in the getbitcontext
     * (it is needed for decoding) */
    return dst;
}

/**
 * Identify the exact end of the bitstream
 * @return the length of the trailing, or 0 if damaged
 */
static int decode_rbsp_trailing(H264Context *h, const uint8_t *src)
{
    int v = *src;
    int r;

    tprintf(h->avctx, "rbsp trailing %X\n", v);

    for (r = 1; r < 9; r++) {
        if (v & 1)
            return r;
        v >>= 1;
    }
    return 0;
}

void tt_h264_free_tables(H264Context *h, int free_rbsp)
{
    int i;
    H264Context *hx;

    ttv_freep(&h->intra4x4_pred_mode);
    ttv_freep(&h->chroma_pred_mode_table);
    ttv_freep(&h->cbp_table);
    ttv_freep(&h->mvd_table[0]);
    ttv_freep(&h->mvd_table[1]);
    ttv_freep(&h->direct_table);
    ttv_freep(&h->non_zero_count);
    ttv_freep(&h->slice_table_base);
    h->slice_table = NULL;
    ttv_freep(&h->list_counts);

    ttv_freep(&h->mb2b_xy);
    ttv_freep(&h->mb2br_xy);

    ttv_buffer_pool_uninit(&h->qscale_table_pool);
    ttv_buffer_pool_uninit(&h->mb_type_pool);
    ttv_buffer_pool_uninit(&h->motion_val_pool);
    ttv_buffer_pool_uninit(&h->ref_index_pool);

    if (free_rbsp && h->DPB) {
        for (i = 0; i < H264_MAX_PICTURE_COUNT; i++)
            tt_h264_unref_picture(h, &h->DPB[i]);
        ttv_freep(&h->DPB);
    } else if (h->DPB) {
        for (i = 0; i < H264_MAX_PICTURE_COUNT; i++)
            h->DPB[i].needs_realloc = 1;
    }

    h->cur_pic_ptr = NULL;

    for (i = 0; i < H264_MAX_THREADS; i++) {
        hx = h->thread_context[i];
        if (!hx)
            continue;
        ttv_freep(&hx->top_borders[1]);
        ttv_freep(&hx->top_borders[0]);
        ttv_freep(&hx->bipred_scratchpad);
        ttv_freep(&hx->edge_emu_buffer);
        ttv_freep(&hx->dc_val_base);
        ttv_freep(&hx->er.mb_index2xy);
        ttv_freep(&hx->er.error_status_table);
        ttv_freep(&hx->er.er_temp_buffer);
        ttv_freep(&hx->er.mbintra_table);
        ttv_freep(&hx->er.mbskip_table);

        if (free_rbsp) {
            ttv_freep(&hx->rbsp_buffer[1]);
            ttv_freep(&hx->rbsp_buffer[0]);
            hx->rbsp_buffer_size[0] = 0;
            hx->rbsp_buffer_size[1] = 0;
        }
        if (i)
            ttv_freep(&h->thread_context[i]);
    }
}

int tt_h264_alloc_tables(H264Context *h)
{
    const int big_mb_num = h->mb_stride * (h->mb_height + 1);
    const int row_mb_num = 2*h->mb_stride*FFMAX(h->avctx->thread_count, 1);
    int x, y, i;

    TT_ALLOCZ_ARRAY_OR_GOTO(h->avctx, h->intra4x4_pred_mode,
                      row_mb_num, 8 * sizeof(uint8_t), fail)
    TT_ALLOCZ_OR_GOTO(h->avctx, h->non_zero_count,
                      big_mb_num * 48 * sizeof(uint8_t), fail)
    TT_ALLOCZ_OR_GOTO(h->avctx, h->slice_table_base,
                      (big_mb_num + h->mb_stride) * sizeof(*h->slice_table_base), fail)
    TT_ALLOCZ_OR_GOTO(h->avctx, h->cbp_table,
                      big_mb_num * sizeof(uint16_t), fail)
    TT_ALLOCZ_OR_GOTO(h->avctx, h->chroma_pred_mode_table,
                      big_mb_num * sizeof(uint8_t), fail)
    TT_ALLOCZ_ARRAY_OR_GOTO(h->avctx, h->mvd_table[0],
                      row_mb_num, 16 * sizeof(uint8_t), fail);
    TT_ALLOCZ_ARRAY_OR_GOTO(h->avctx, h->mvd_table[1],
                      row_mb_num, 16 * sizeof(uint8_t), fail);
    TT_ALLOCZ_OR_GOTO(h->avctx, h->direct_table,
                      4 * big_mb_num * sizeof(uint8_t), fail);
    TT_ALLOCZ_OR_GOTO(h->avctx, h->list_counts,
                      big_mb_num * sizeof(uint8_t), fail)

    memset(h->slice_table_base, -1,
           (big_mb_num + h->mb_stride) * sizeof(*h->slice_table_base));
    h->slice_table = h->slice_table_base + h->mb_stride * 2 + 1;

    TT_ALLOCZ_OR_GOTO(h->avctx, h->mb2b_xy,
                      big_mb_num * sizeof(uint32_t), fail);
    TT_ALLOCZ_OR_GOTO(h->avctx, h->mb2br_xy,
                      big_mb_num * sizeof(uint32_t), fail);
    for (y = 0; y < h->mb_height; y++)
        for (x = 0; x < h->mb_width; x++) {
            const int mb_xy = x + y * h->mb_stride;
            const int b_xy  = 4 * x + 4 * y * h->b_stride;

            h->mb2b_xy[mb_xy]  = b_xy;
            h->mb2br_xy[mb_xy] = 8 * (FMO ? mb_xy : (mb_xy % (2 * h->mb_stride)));
        }

    if (!h->dequant4_coeff[0])
        h264_init_dequant_tables(h);

    if (!h->DPB) {
        h->DPB = ttv_mallocz_array(H264_MAX_PICTURE_COUNT, sizeof(*h->DPB));
        if (!h->DPB)
            goto fail;
        for (i = 0; i < H264_MAX_PICTURE_COUNT; i++)
            ttv_frame_unref(&h->DPB[i].f);
        ttv_frame_unref(&h->cur_pic.f);
    }

    return 0;

fail:
    tt_h264_free_tables(h, 1);
    return AVERROR(ENOMEM);
}

/**
 * Init context
 * Allocate buffers which are not shared amongst multiple threads.
 */
int tt_h264_context_init(H264Context *h)
{
    ERContext *er = &h->er;
    int mb_array_size = h->mb_height * h->mb_stride;
    int y_size  = (2 * h->mb_width + 1) * (2 * h->mb_height + 1);
    int c_size  = h->mb_stride * (h->mb_height + 1);
    int yc_size = y_size + 2   * c_size;
    int x, y, i;

    TT_ALLOCZ_ARRAY_OR_GOTO(h->avctx, h->top_borders[0],
                      h->mb_width, 16 * 3 * sizeof(uint8_t) * 2, fail)
    TT_ALLOCZ_ARRAY_OR_GOTO(h->avctx, h->top_borders[1],
                      h->mb_width, 16 * 3 * sizeof(uint8_t) * 2, fail)

    h->ref_cache[0][scan8[5]  + 1] =
    h->ref_cache[0][scan8[7]  + 1] =
    h->ref_cache[0][scan8[13] + 1] =
    h->ref_cache[1][scan8[5]  + 1] =
    h->ref_cache[1][scan8[7]  + 1] =
    h->ref_cache[1][scan8[13] + 1] = PART_NOT_AVAILABLE;

    if (CONFIG_ERROR_RESILIENCE) {
        /* init ER */
        er->avctx          = h->avctx;
 //       er->mecc           = &h->mecc;
        er->decode_mb      = h264_er_decode_mb;
        er->opaque         = h;
        er->quarter_sample = 1;

        er->mb_num      = h->mb_num;
        er->mb_width    = h->mb_width;
        er->mb_height   = h->mb_height;
        er->mb_stride   = h->mb_stride;
        er->b8_stride   = h->mb_width * 2 + 1;

        // error resilience code looks cleaner with this
        TT_ALLOCZ_OR_GOTO(h->avctx, er->mb_index2xy,
                          (h->mb_num + 1) * sizeof(int), fail);

        for (y = 0; y < h->mb_height; y++)
            for (x = 0; x < h->mb_width; x++)
                er->mb_index2xy[x + y * h->mb_width] = x + y * h->mb_stride;

        er->mb_index2xy[h->mb_height * h->mb_width] = (h->mb_height - 1) *
                                                      h->mb_stride + h->mb_width;

        TT_ALLOCZ_OR_GOTO(h->avctx, er->error_status_table,
                          mb_array_size * sizeof(uint8_t), fail);

        TT_ALLOC_OR_GOTO(h->avctx, er->mbintra_table, mb_array_size, fail);
        memset(er->mbintra_table, 1, mb_array_size);

        TT_ALLOCZ_OR_GOTO(h->avctx, er->mbskip_table, mb_array_size + 2, fail);

        TT_ALLOC_OR_GOTO(h->avctx, er->er_temp_buffer,
                         h->mb_height * h->mb_stride, fail);

        TT_ALLOCZ_OR_GOTO(h->avctx, h->dc_val_base,
                          yc_size * sizeof(int16_t), fail);
        er->dc_val[0] = h->dc_val_base + h->mb_width * 2 + 2;
        er->dc_val[1] = h->dc_val_base + y_size + h->mb_stride + 1;
        er->dc_val[2] = er->dc_val[1] + c_size;
        for (i = 0; i < yc_size; i++)
            h->dc_val_base[i] = 1024;
    }

    return 0;

fail:
    return AVERROR(ENOMEM); // tt_h264_free_tables will clean up for us
}

static int decode_nal_units(H264Context *h, const uint8_t *buf, int buf_size,
                            int parse_extradata);

int tt_h264_decode_extradata(H264Context *h, const uint8_t *buf, int size)
{
    TTCodecContext *avctx = h->avctx;
    int ret;

    if (!buf || size <= 0)
        return -1;

    if (buf[0] == 1) {
        int i, cnt, nalsize;
        const unsigned char *p = buf;

        h->is_avc = 1;

        if (size < 7) {
            ttv_log(avctx, TTV_LOG_ERROR,
                   "avcC %d too short\n", size);
            return TTERROR_INVALIDDATA;
        }
        /* sps and pps in the avcC always have length coded with 2 bytes,
         * so put a fake nal_length_size = 2 while parsing them */
        h->nal_length_size = 2;
        // Decode sps from avcC
        cnt = *(p + 5) & 0x1f; // Number of sps
        p  += 6;
        for (i = 0; i < cnt; i++) {
            nalsize = TTV_RB16(p) + 2;
            if(nalsize > size - (p-buf))
                return TTERROR_INVALIDDATA;
            ret = decode_nal_units(h, p, nalsize, 1);
            if (ret < 0) {
                ttv_log(avctx, TTV_LOG_ERROR,
                       "Decoding sps %d from avcC failed\n", i);
                return ret;
            }
            p += nalsize;
        }
        // Decode pps from avcC
        cnt = *(p++); // Number of pps
        for (i = 0; i < cnt; i++) {
            nalsize = TTV_RB16(p) + 2;
            if(nalsize > size - (p-buf))
                return TTERROR_INVALIDDATA;
            ret = decode_nal_units(h, p, nalsize, 1);
            if (ret < 0) {
                ttv_log(avctx, TTV_LOG_ERROR,
                       "Decoding pps %d from avcC failed\n", i);
                return ret;
            }
            p += nalsize;
        }
        // Store right nal length size that will be used to parse all other nals
        h->nal_length_size = (buf[4] & 0x03) + 1;
    } else {
        h->is_avc = 0;
        ret = decode_nal_units(h, buf, size, 1);
        if (ret < 0)
            return ret;
    }
    return size;
}

ttv_cold int tt_h264_decode_init(TTCodecContext *avctx)
{
    H264Context *h = avctx->priv_data;
    int i;
    int ret;

    h->avctx = avctx;

    h->bit_depth_luma    = 8;
    h->chroma_format_idc = 1;

    h->avctx->bits_per_raw_sample = 8;
    h->cur_chroma_format_idc = 1;

    tt_h264dsp_init(&h->h264dsp, 8, 1);
    ttv_assert0(h->sps.bit_depth_chroma == 0);
    tt_h264chroma_init(&h->h264chroma, h->sps.bit_depth_chroma);
    tt_h264qpel_init(&h->h264qpel, 8);
    tt_h264_pred_init(&h->hpc, h->avctx->codec_id, 8, 1);

    h->dequant_coett_pps = -1;
    h->current_sps_id = -1;

    /* needed so that IDCT permutation is known early */
//     if (CONFIG_ERROR_RESILIENCE)
//         tt_me_cmp_init(&h->mecc, h->avctx);
    tt_videodsp_init(&h->vdsp, 8);

    memset(h->pps.scaling_matrix4, 16, 6 * 16 * sizeof(uint8_t));
    memset(h->pps.scaling_matrix8, 16, 2 * 64 * sizeof(uint8_t));

    h->picture_structure   = PICT_FRAME;
    h->slice_context_count = 1;
    h->workaround_bugs     = avctx->workaround_bugs;
    h->flags               = avctx->flags;

    /* set defaults */
    // s->decode_mb = tt_h263_decode_mb;
    if (!avctx->has_b_frames)
        h->low_delay = 1;

    avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;

    tt_h264_decode_init_vlc();

    tt_init_cabac_states();

    h->pixel_shift        = 0;
    h->sps.bit_depth_luma = avctx->bits_per_raw_sample = 8;

    h->thread_context[0] = h;
    h->outputed_poc      = h->next_outputed_poc = INT_MIN;
    for (i = 0; i < MAX_DELAYED_PIC_COUNT; i++)
        h->last_pocs[i] = INT_MIN;
    h->prev_poc_msb = 1 << 16;
    h->prev_frame_num = -1;
    h->x264_build   = -1;
    h->sei_fpa.frame_packing_arrangement_cancel_flag = -1;
    tt_h264_reset_sei(h);
    if (avctx->codec_id == TTV_CODEC_ID_H264) {
        if (avctx->ticks_per_frame == 1) {
            if(h->avctx->time_base.den < INT_MAX/2) {
                h->avctx->time_base.den *= 2;
            } else
                h->avctx->time_base.num /= 2;
        }
        avctx->ticks_per_frame = 2;
    }

    if (avctx->extradata_size > 0 && avctx->extradata) {
        ret = tt_h264_decode_extradata(h, avctx->extradata, avctx->extradata_size);
        if (ret < 0) {
            tt_h264_free_context(h);
            return ret;
        }
    }

    if (h->sps.bitstream_restriction_flag &&
        h->avctx->has_b_frames < h->sps.num_reorder_frames) {
        h->avctx->has_b_frames = h->sps.num_reorder_frames;
        h->low_delay           = 0;
    }

    avctx->internal->allocate_progress = 1;

    tt_h264_flush_change(h);

    return 0;
}

static int decode_init_thread_copy(TTCodecContext *avctx)
{
    H264Context *h = avctx->priv_data;

    if (!avctx->internal->is_copy)
        return 0;
    memset(h->sps_buffers, 0, sizeof(h->sps_buffers));
    memset(h->pps_buffers, 0, sizeof(h->pps_buffers));

    h->rbsp_buffer[0] = NULL;
    h->rbsp_buffer[1] = NULL;
    h->rbsp_buffer_size[0] = 0;
    h->rbsp_buffer_size[1] = 0;
    h->context_initialized = 0;

    return 0;
}

/**
 * Run setup operations that must be run after slice header decoding.
 * This includes finding the next displayed frame.
 *
 * @param h h264 master context
 * @param setup_finished enough NALs have been read that we can call
 * tt_thread_finish_setup()
 */
static void decode_postinit(H264Context *h, int setup_finished)
{
    H264Picture *out = h->cur_pic_ptr;
    H264Picture *cur = h->cur_pic_ptr;
    int i, pics, out_of_order, out_idx;

    h->cur_pic_ptr->f.pict_type = h->pict_type;

    if (h->next_output_pic)
        return;

    if (cur->field_poc[0] == INT_MAX || cur->field_poc[1] == INT_MAX) {
        /* FIXME: if we have two PAFF fields in one packet, we can't start
         * the next thread here. If we have one field per packet, we can.
         * The check in decode_nal_units() is not good enough to find this
         * yet, so we assume the worst for now. */
        // if (setup_finished)
        //    tt_thread_finish_setup(h->avctx);
        return;
    }

    cur->f.interlaced_frame = 0;
    cur->f.repeat_pict      = 0;

    /* Signal interlacing information externally. */
    /* Prioritize picture timing SEI information over used
     * decoding process if it exists. */

    if (h->sps.pic_struct_present_flag) {
        switch (h->sei_pic_struct) {
        case SEI_PIC_STRUCT_FRAME:
            break;
        case SEI_PIC_STRUCT_TOP_FIELD:
        case SEI_PIC_STRUCT_BOTTOM_FIELD:
            cur->f.interlaced_frame = 1;
            break;
        case SEI_PIC_STRUCT_TOP_BOTTOM:
        case SEI_PIC_STRUCT_BOTTOM_TOP:
            if (FIELD_OR_MBATT_PICTURE(h))
                cur->f.interlaced_frame = 1;
            else
                // try to flag soft telecine progressive
                cur->f.interlaced_frame = h->prev_interlaced_frame;
            break;
        case SEI_PIC_STRUCT_TOP_BOTTOM_TOP:
        case SEI_PIC_STRUCT_BOTTOM_TOP_BOTTOM:
            /* Signal the possibility of telecined film externally
             * (pic_struct 5,6). From these hints, let the applications
             * decide if they apply deinterlacing. */
            cur->f.repeat_pict = 1;
            break;
        case SEI_PIC_STRUCT_FRAME_DOUBLING:
            cur->f.repeat_pict = 2;
            break;
        case SEI_PIC_STRUCT_FRAME_TRIPLING:
            cur->f.repeat_pict = 4;
            break;
        }

        if ((h->sei_ct_type & 3) &&
            h->sei_pic_struct <= SEI_PIC_STRUCT_BOTTOM_TOP)
            cur->f.interlaced_frame = (h->sei_ct_type & (1 << 1)) != 0;
    } else {
        /* Derive interlacing flag from used decoding process. */
        cur->f.interlaced_frame = FIELD_OR_MBATT_PICTURE(h);
    }
    h->prev_interlaced_frame = cur->f.interlaced_frame;

    if (cur->field_poc[0] != cur->field_poc[1]) {
        /* Derive top_field_first from field pocs. */
        cur->f.top_field_first = cur->field_poc[0] < cur->field_poc[1];
    } else {
        if (cur->f.interlaced_frame || h->sps.pic_struct_present_flag) {
            /* Use picture timing SEI information. Even if it is a
             * information of a past frame, better than nothing. */
            if (h->sei_pic_struct == SEI_PIC_STRUCT_TOP_BOTTOM ||
                h->sei_pic_struct == SEI_PIC_STRUCT_TOP_BOTTOM_TOP)
                cur->f.top_field_first = 1;
            else
                cur->f.top_field_first = 0;
        } else {
            /* Most likely progressive */
            cur->f.top_field_first = 0;
        }
    }

    if (h->sei_display_orientation_present &&
        (h->sei_anticlockwise_rotation || h->sei_hflip || h->sei_vflip)) {
        double angle = h->sei_anticlockwise_rotation * 360 / (double) (1 << 16);
        TTFrameSideData *rotation = ttv_frame_new_side_data(&cur->f,
                                                           TTV_FRAME_DATA_DISPLAYMATRIX,
                                                           sizeof(int32_t) * 9);
        if (rotation) {
            ttv_display_rotation_set((int32_t *)rotation->data, angle);
            ttv_display_matrix_flip((int32_t *)rotation->data,
                                   h->sei_hflip, h->sei_vflip);
        }
    }

    cur->mmco_reset = h->mmco_reset;
    h->mmco_reset = 0;

    // FIXME do something with unavailable reference frames

    /* Sort B-frames into display order */

    if (h->sps.bitstream_restriction_flag &&
        h->avctx->has_b_frames < h->sps.num_reorder_frames) {
        h->avctx->has_b_frames = h->sps.num_reorder_frames;
        h->low_delay           = 0;
    }

    if (h->avctx->strict_std_compliance >= TT_COMPLIANCE_STRICT &&
        !h->sps.bitstream_restriction_flag) {
        h->avctx->has_b_frames = MAX_DELAYED_PIC_COUNT - 1;
        h->low_delay           = 0;
    }

    for (i = 0; 1; i++) {
        if(i == MAX_DELAYED_PIC_COUNT || cur->poc < h->last_pocs[i]){
            if(i)
                h->last_pocs[i-1] = cur->poc;
            break;
        } else if(i) {
            h->last_pocs[i-1]= h->last_pocs[i];
        }
    }
    out_of_order = MAX_DELAYED_PIC_COUNT - i;
    if(   cur->f.pict_type == TTV_PICTURE_TYPE_B
       || (h->last_pocs[MAX_DELAYED_PIC_COUNT-2] > INT_MIN && h->last_pocs[MAX_DELAYED_PIC_COUNT-1] - h->last_pocs[MAX_DELAYED_PIC_COUNT-2] > 2))
        out_of_order = FFMAX(out_of_order, 1);
    if (out_of_order == MAX_DELAYED_PIC_COUNT) {
        ttv_log(h->avctx, TTV_LOG_VERBOSE, "Invalid POC %d<%d\n", cur->poc, h->last_pocs[0]);
        for (i = 1; i < MAX_DELAYED_PIC_COUNT; i++)
            h->last_pocs[i] = INT_MIN;
        h->last_pocs[0] = cur->poc;
        cur->mmco_reset = 1;
    } else if(h->avctx->has_b_frames < out_of_order && !h->sps.bitstream_restriction_flag){
        ttv_log(h->avctx, TTV_LOG_VERBOSE, "Increasing reorder buffer to %d\n", out_of_order);
        h->avctx->has_b_frames = out_of_order;
        h->low_delay = 0;
    }

    pics = 0;
    while (h->delayed_pic[pics])
        pics++;

    ttv_assert0(pics <= MAX_DELAYED_PIC_COUNT);

    h->delayed_pic[pics++] = cur;
    if (cur->reference == 0)
        cur->reference = DELAYED_PIC_REF;

    out     = h->delayed_pic[0];
    out_idx = 0;
    for (i = 1; h->delayed_pic[i] &&
                !h->delayed_pic[i]->f.key_frame &&
                !h->delayed_pic[i]->mmco_reset;
         i++)
        if (h->delayed_pic[i]->poc < out->poc) {
            out     = h->delayed_pic[i];
            out_idx = i;
        }
    if (h->avctx->has_b_frames == 0 &&
        (h->delayed_pic[0]->f.key_frame || h->delayed_pic[0]->mmco_reset))
        h->next_outputed_poc = INT_MIN;
    out_of_order = out->poc < h->next_outputed_poc;

    if (out_of_order || pics > h->avctx->has_b_frames) {
        out->reference &= ~DELAYED_PIC_REF;
        // for frame threading, the owner must be the second field's thread or
        // else the first thread can release the picture and reuse it unsafely
        for (i = out_idx; h->delayed_pic[i]; i++)
            h->delayed_pic[i] = h->delayed_pic[i + 1];
    }
    if (!out_of_order && pics > h->avctx->has_b_frames) {
        h->next_output_pic = out;
        if (out_idx == 0 && h->delayed_pic[0] && (h->delayed_pic[0]->f.key_frame || h->delayed_pic[0]->mmco_reset)) {
            h->next_outputed_poc = INT_MIN;
        } else
            h->next_outputed_poc = out->poc;
    } else {
        ttv_log(h->avctx, TTV_LOG_DEBUG, "no picture %s\n", out_of_order ? "ooo" : "");
    }

    if (h->next_output_pic) {
        if (h->next_output_pic->recovered) {
            // We have reached an recovery point and all frames after it in
            // display order are "recovered".
            h->frame_recovered |= FRAME_RECOVERED_SEI;
        }
        h->next_output_pic->recovered |= !!(h->frame_recovered & FRAME_RECOVERED_SEI);
    }

    if (setup_finished /*&& !h->avctx->hwaccel*/)
        tt_thread_finish_setup(h->avctx);
}

int tt_pred_weight_table(H264Context *h)
{
    int list, i;
    int luma_def, chroma_def;

    h->use_weight             = 0;
    h->use_weight_chroma      = 0;
    h->luma_log2_weight_denom = get_ue_golomb(&h->gb);
    if (h->sps.chroma_format_idc)
        h->chroma_log2_weight_denom = get_ue_golomb(&h->gb);
    luma_def   = 1 << h->luma_log2_weight_denom;
    chroma_def = 1 << h->chroma_log2_weight_denom;

    for (list = 0; list < 2; list++) {
        h->luma_weight_flag[list]   = 0;
        h->chroma_weight_flag[list] = 0;
        for (i = 0; i < h->ref_count[list]; i++) {
            int luma_weight_flag, chroma_weight_flag;

            luma_weight_flag = get_bits1(&h->gb);
            if (luma_weight_flag) {
                h->luma_weight[i][list][0] = get_se_golomb(&h->gb);
                h->luma_weight[i][list][1] = get_se_golomb(&h->gb);
                if (h->luma_weight[i][list][0] != luma_def ||
                    h->luma_weight[i][list][1] != 0) {
                    h->use_weight             = 1;
                    h->luma_weight_flag[list] = 1;
                }
            } else {
                h->luma_weight[i][list][0] = luma_def;
                h->luma_weight[i][list][1] = 0;
            }

            if (h->sps.chroma_format_idc) {
                chroma_weight_flag = get_bits1(&h->gb);
                if (chroma_weight_flag) {
                    int j;
                    for (j = 0; j < 2; j++) {
                        h->chroma_weight[i][list][j][0] = get_se_golomb(&h->gb);
                        h->chroma_weight[i][list][j][1] = get_se_golomb(&h->gb);
                        if (h->chroma_weight[i][list][j][0] != chroma_def ||
                            h->chroma_weight[i][list][j][1] != 0) {
                            h->use_weight_chroma        = 1;
                            h->chroma_weight_flag[list] = 1;
                        }
                    }
                } else {
                    int j;
                    for (j = 0; j < 2; j++) {
                        h->chroma_weight[i][list][j][0] = chroma_def;
                        h->chroma_weight[i][list][j][1] = 0;
                    }
                }
            }
        }
        if (h->slice_type_nos != TTV_PICTURE_TYPE_B)
            break;
    }
    h->use_weight = h->use_weight || h->use_weight_chroma;
    return 0;
}

/**
 * instantaneous decoder refresh.
 */
static void idr(H264Context *h)
{
    int i;
    tt_h264_remove_all_refs(h);
    h->prev_frame_num        =
    h->prev_frame_num_offset = 0;
    h->prev_poc_msb          = 1<<16;
    h->prev_poc_lsb          = 0;
    for (i = 0; i < MAX_DELAYED_PIC_COUNT; i++)
        h->last_pocs[i] = INT_MIN;
}

/* forget old pics after a seek */
void tt_h264_flush_change(H264Context *h)
{
    int i, j;

    h->outputed_poc          = h->next_outputed_poc = INT_MIN;
    h->prev_interlaced_frame = 1;
    idr(h);

    h->prev_frame_num = -1;
    if (h->cur_pic_ptr) {
        h->cur_pic_ptr->reference = 0;
        for (j=i=0; h->delayed_pic[i]; i++)
            if (h->delayed_pic[i] != h->cur_pic_ptr)
                h->delayed_pic[j++] = h->delayed_pic[i];
        h->delayed_pic[j] = NULL;
    }
    h->first_field = 0;
    memset(h->ref_list[0], 0, sizeof(h->ref_list[0]));
    memset(h->ref_list[1], 0, sizeof(h->ref_list[1]));
    memset(h->default_ref_list[0], 0, sizeof(h->default_ref_list[0]));
    memset(h->default_ref_list[1], 0, sizeof(h->default_ref_list[1]));
    tt_h264_reset_sei(h);
    h->recovery_frame = -1;
    h->frame_recovered = 0;
    h->list_count = 0;
    h->current_slice = 0;
    h->mmco_reset = 1;
}

/* forget old pics after a seek */
static void flush_dpb(TTCodecContext *avctx)
{
    H264Context *h = avctx->priv_data;
    int i;

    for (i = 0; i <= MAX_DELAYED_PIC_COUNT; i++) {
        if (h->delayed_pic[i])
            h->delayed_pic[i]->reference = 0;
        h->delayed_pic[i] = NULL;
    }

    tt_h264_flush_change(h);

    if (h->DPB)
        for (i = 0; i < H264_MAX_PICTURE_COUNT; i++)
            tt_h264_unref_picture(h, &h->DPB[i]);
    h->cur_pic_ptr = NULL;
    tt_h264_unref_picture(h, &h->cur_pic);

    h->mb_x = h->mb_y = 0;

//     h->parse_context.state             = -1;
//     h->parse_context.frame_start_found = 0;
//     h->parse_context.overread          = 0;
//     h->parse_context.overread_index    = 0;
//     h->parse_context.index             = 0;
//     h->parse_context.last_index        = 0;

    tt_h264_free_tables(h, 1);
    h->context_initialized = 0;
}

int tt_init_poc(H264Context *h, int pic_field_poc[2], int *pic_poc)
{
    const int max_frame_num = 1 << h->sps.log2_max_frame_num;
    int field_poc[2];

    h->frame_num_offset = h->prev_frame_num_offset;
    if (h->frame_num < h->prev_frame_num)
        h->frame_num_offset += max_frame_num;

    if (h->sps.poc_type == 0) {
        const int max_poc_lsb = 1 << h->sps.log2_max_poc_lsb;

        if (h->poc_lsb < h->prev_poc_lsb &&
            h->prev_poc_lsb - h->poc_lsb >= max_poc_lsb / 2)
            h->poc_msb = h->prev_poc_msb + max_poc_lsb;
        else if (h->poc_lsb > h->prev_poc_lsb &&
                 h->prev_poc_lsb - h->poc_lsb < -max_poc_lsb / 2)
            h->poc_msb = h->prev_poc_msb - max_poc_lsb;
        else
            h->poc_msb = h->prev_poc_msb;
        field_poc[0] =
        field_poc[1] = h->poc_msb + h->poc_lsb;
        if (h->picture_structure == PICT_FRAME)
            field_poc[1] += h->delta_poc_bottom;
    } else if (h->sps.poc_type == 1) {
        int abs_frame_num, expected_delta_per_poc_cycle, expectedpoc;
        int i;

        if (h->sps.poc_cycle_length != 0)
            abs_frame_num = h->frame_num_offset + h->frame_num;
        else
            abs_frame_num = 0;

        if (h->nal_ref_idc == 0 && abs_frame_num > 0)
            abs_frame_num--;

        expected_delta_per_poc_cycle = 0;
        for (i = 0; i < h->sps.poc_cycle_length; i++)
            // FIXME integrate during sps parse
            expected_delta_per_poc_cycle += h->sps.offset_for_ref_frame[i];

        if (abs_frame_num > 0) {
            int poc_cycle_cnt          = (abs_frame_num - 1) / h->sps.poc_cycle_length;
            int frame_num_in_poc_cycle = (abs_frame_num - 1) % h->sps.poc_cycle_length;

            expectedpoc = poc_cycle_cnt * expected_delta_per_poc_cycle;
            for (i = 0; i <= frame_num_in_poc_cycle; i++)
                expectedpoc = expectedpoc + h->sps.offset_for_ref_frame[i];
        } else
            expectedpoc = 0;

        if (h->nal_ref_idc == 0)
            expectedpoc = expectedpoc + h->sps.offset_for_non_ref_pic;

        field_poc[0] = expectedpoc + h->delta_poc[0];
        field_poc[1] = field_poc[0] + h->sps.offset_for_top_to_bottom_field;

        if (h->picture_structure == PICT_FRAME)
            field_poc[1] += h->delta_poc[1];
    } else {
        int poc = 2 * (h->frame_num_offset + h->frame_num);

        if (!h->nal_ref_idc)
            poc--;

        field_poc[0] = poc;
        field_poc[1] = poc;
    }

    if (h->picture_structure != PICT_BOTTOM_FIELD)
        pic_field_poc[0] = field_poc[0];
    if (h->picture_structure != PICT_TOP_FIELD)
        pic_field_poc[1] = field_poc[1];
    *pic_poc = FFMIN(pic_field_poc[0], pic_field_poc[1]);

    return 0;
}

/**
 * Compute profile from profile_idc and constraint_set?_flags.
 *
 * @param sps SPS
 *
 * @return profile as defined by TT_PROFILE_H264_*
 */
int tt_h264_get_profile(SPS *sps)
{
    int profile = sps->profile_idc;

    switch (sps->profile_idc) {
    case TT_PROFILE_H264_BASELINE:
        // constraint_set1_flag set to 1
        profile |= (sps->constraint_set_flags & 1 << 1) ? TT_PROFILE_H264_CONSTRAINED : 0;
        break;
    case TT_PROFILE_H264_HIGH_10:
    case TT_PROFILE_H264_HIGH_422:
    case TT_PROFILE_H264_HIGH_444_PREDICTIVE:
        // constraint_set3_flag set to 1
        profile |= (sps->constraint_set_flags & 1 << 3) ? TT_PROFILE_H264_INTRA : 0;
        break;
    }

    return profile;
}

int tt_h264_set_parameter_from_sps(H264Context *h)
{
    if (h->flags & CODEC_FLAG_LOW_DELAY ||
        (h->sps.bitstream_restriction_flag &&
         !h->sps.num_reorder_frames)) {
        if (h->avctx->has_b_frames > 1 || h->delayed_pic[0])
            ttv_log(h->avctx, TTV_LOG_WARNING, "Delayed frames seen. "
                   "Reenabling low delay requires a codec flush.\n");
        else
            h->low_delay = 1;
    }

    if (h->avctx->has_b_frames < 2)
        h->avctx->has_b_frames = !h->low_delay;

    if (h->avctx->bits_per_raw_sample != h->sps.bit_depth_luma ||
        h->cur_chroma_format_idc      != h->sps.chroma_format_idc) {
        if (h->avctx->codec &&
            h->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU &&
            (h->sps.bit_depth_luma != 8 || h->sps.chroma_format_idc > 1)) {
            ttv_log(h->avctx, TTV_LOG_ERROR,
                   "VDPAU decoding does not support video colorspace.\n");
            return TTERROR_INVALIDDATA;
        }
        if (h->sps.bit_depth_luma >= 8 && h->sps.bit_depth_luma <= 14 &&
            h->sps.bit_depth_luma != 11 && h->sps.bit_depth_luma != 13) {
            h->avctx->bits_per_raw_sample = h->sps.bit_depth_luma;
            h->cur_chroma_format_idc      = h->sps.chroma_format_idc;
            h->pixel_shift                = h->sps.bit_depth_luma > 8;

            tt_h264dsp_init(&h->h264dsp, h->sps.bit_depth_luma,
                            h->sps.chroma_format_idc);
            tt_h264chroma_init(&h->h264chroma, h->sps.bit_depth_chroma);
            tt_h264qpel_init(&h->h264qpel, h->sps.bit_depth_luma);
            tt_h264_pred_init(&h->hpc, h->avctx->codec_id, h->sps.bit_depth_luma,
                              h->sps.chroma_format_idc);

//             if (CONFIG_ERROR_RESILIENCE)
//                 tt_me_cmp_init(&h->mecc, h->avctx);
            tt_videodsp_init(&h->vdsp, h->sps.bit_depth_luma);
        } else {
            ttv_log(h->avctx, TTV_LOG_ERROR, "Unsupported bit depth %d\n",
                   h->sps.bit_depth_luma);
            return TTERROR_INVALIDDATA;
        }
    }
    return 0;
}

int tt_set_ref_count(H264Context *h)
{
    int ref_count[2], list_count;
    int num_ref_idx_active_override_flag;

    // set defaults, might be overridden a few lines later
    ref_count[0] = h->pps.ref_count[0];
    ref_count[1] = h->pps.ref_count[1];

    if (h->slice_type_nos != TTV_PICTURE_TYPE_I) {
        unsigned max[2];
        max[0] = max[1] = h->picture_structure == PICT_FRAME ? 15 : 31;

        if (h->slice_type_nos == TTV_PICTURE_TYPE_B)
            h->direct_spatial_mv_pred = get_bits1(&h->gb);
        num_ref_idx_active_override_flag = get_bits1(&h->gb);

        if (num_ref_idx_active_override_flag) {
            ref_count[0] = get_ue_golomb(&h->gb) + 1;
            if (h->slice_type_nos == TTV_PICTURE_TYPE_B) {
                ref_count[1] = get_ue_golomb(&h->gb) + 1;
            } else
                // full range is spec-ok in this case, even for frames
                ref_count[1] = 1;
        }

        if (ref_count[0]-1 > max[0] || ref_count[1]-1 > max[1]){
            ttv_log(h->avctx, TTV_LOG_ERROR, "reference overflow %u > %u or %u > %u\n", ref_count[0]-1, max[0], ref_count[1]-1, max[1]);
            h->ref_count[0] = h->ref_count[1] = 0;
            h->list_count   = 0;
            return TTERROR_INVALIDDATA;
        }

        if (h->slice_type_nos == TTV_PICTURE_TYPE_B)
            list_count = 2;
        else
            list_count = 1;
    } else {
        list_count   = 0;
        ref_count[0] = ref_count[1] = 0;
    }

    if (list_count != h->list_count ||
        ref_count[0] != h->ref_count[0] ||
        ref_count[1] != h->ref_count[1]) {
        h->ref_count[0] = ref_count[0];
        h->ref_count[1] = ref_count[1];
        h->list_count   = list_count;
        return 1;
    }

    return 0;
}

//static const uint8_t start_code[] = { 0x00, 0x00, 0x01 };

static int get_bit_length(H264Context *h, const uint8_t *buf,
                          const uint8_t *ptr, int dst_length,
                          int i, int next_avc)
{
    if ((h->workaround_bugs & TT_BUG_AUTODETECT) && i + 3 < next_avc &&
        buf[i]     == 0x00 && buf[i + 1] == 0x00 &&
        buf[i + 2] == 0x01 && buf[i + 3] == 0xE0)
        h->workaround_bugs |= TT_BUG_TRUNCATED;

    if (!(h->workaround_bugs & TT_BUG_TRUNCATED))
        while (dst_length > 0 && ptr[dst_length - 1] == 0)
            dst_length--;

    if (!dst_length)
        return 0;

    return 8 * dst_length - decode_rbsp_trailing(h, ptr + dst_length - 1);
}

static int get_last_needed_nal(H264Context *h, const uint8_t *buf, int buf_size)
{
    int next_avc    = h->is_avc ? 0 : buf_size;
    int nal_index   = 0;
    int buf_index   = 0;
    int nals_needed = 0;
    int first_slice = 0;

    while(1) {
        int nalsize = 0;
        int dst_length, bit_length, consumed;
        const uint8_t *ptr;

        if (buf_index >= next_avc) {
            nalsize = get_avc_nalsize(h, buf, buf_size, &buf_index);
            if (nalsize < 0)
                break;
            next_avc = buf_index + nalsize;
        } else {
            buf_index = find_start_code(buf, buf_size, buf_index, next_avc);
            if (buf_index >= buf_size)
                break;
            if (buf_index >= next_avc)
                continue;
        }

        ptr = tt_h264_decode_nal(h, buf + buf_index, &dst_length, &consumed,
                                 next_avc - buf_index);

        if (!ptr || dst_length < 0)
            return TTERROR_INVALIDDATA;

        buf_index += consumed;

        bit_length = get_bit_length(h, buf, ptr, dst_length,
                                    buf_index, next_avc);
        nal_index++;

        /* packets can sometimes contain multiple PPS/SPS,
         * e.g. two PAFF field pictures in one packet, or a demuxer
         * which splits NALs strangely if so, when frame threading we
         * can't start the next thread until we've read all of them */
        switch (h->nal_unit_type) {
        case NAL_SPS:
        case NAL_PPS:
            nals_needed = nal_index;
            break;
        case NAL_DPA:
        case NAL_IDR_SLICE:
        case NAL_SLICE:
            init_get_bits(&h->gb, ptr, bit_length);
            if (!get_ue_golomb(&h->gb) ||
                !first_slice ||
                first_slice != h->nal_unit_type)
                nals_needed = nal_index;
            if (!first_slice)
                first_slice = h->nal_unit_type;
        }
    }

    return nals_needed;
}

static int decode_nal_units(H264Context *h, const uint8_t *buf, int buf_size,
                            int parse_extradata)
{
    TTCodecContext *const avctx = h->avctx;
    H264Context *hx; ///< thread context
    int buf_index;
    unsigned context_count;
    int next_avc;
    int nals_needed = 0; ///< number of NALs that need decoding before the next frame thread starts
    int nal_index;
    int idr_cleared=0;
    int ret = 0;

    h->nal_unit_type= 0;

    if(!h->slice_context_count)
         h->slice_context_count= 1;
    h->max_contexts = h->slice_context_count;
    if (!(avctx->flags2 & CODEC_FLAG2_CHUNKS)) {
        h->current_slice = 0;
        if (!h->first_field)
            h->cur_pic_ptr = NULL;
        tt_h264_reset_sei(h);
    }

    if (h->nal_length_size == 4) {
        if (buf_size > 8 && TTV_RB32(buf) == 1 && TTV_RB32(buf+5) > (unsigned)buf_size) {
            h->is_avc = 0;
        }else if(buf_size > 3 && TTV_RB32(buf) > 1 && TTV_RB32(buf) <= (unsigned)buf_size)
            h->is_avc = 1;
    }

    if (avctx->active_thread_type & TT_THREAD_FRAME)
        nals_needed = get_last_needed_nal(h, buf, buf_size);

    {
        buf_index     = 0;
        context_count = 0;
        next_avc      = h->is_avc ? 0 : buf_size;
        nal_index     = 0;
        for (;;) {
            int consumed;
            int dst_length;
            int bit_length;
            const uint8_t *ptr;
            int nalsize = 0;
            int err;

            if (buf_index >= next_avc) {
                nalsize = get_avc_nalsize(h, buf, buf_size, &buf_index);
                if (nalsize < 0)
                    break;
                next_avc = buf_index + nalsize;
            } else {
                buf_index = find_start_code(buf, buf_size, buf_index, next_avc);
                if (buf_index >= buf_size)
                    break;
                if (buf_index >= next_avc)
                    continue;
            }

            hx = h->thread_context[context_count];

            ptr = tt_h264_decode_nal(hx, buf + buf_index, &dst_length,
                                     &consumed, next_avc - buf_index);
            if (!ptr || dst_length < 0) {
                ret = -1;
                goto end;
            }

            bit_length = get_bit_length(h, buf, ptr, dst_length,
                                        buf_index + consumed, next_avc);

            if (h->avctx->debug & TT_DEBUG_STARTCODE)
                ttv_log(h->avctx, TTV_LOG_DEBUG,
                       "NAL %d/%d at %d/%d length %d\n",
                       hx->nal_unit_type, hx->nal_ref_idc, buf_index, buf_size, dst_length);

            if (h->is_avc && (nalsize != consumed) && nalsize)
                ttv_log(h->avctx, TTV_LOG_DEBUG,
                       "AVC: Consumed only %d bytes instead of %d\n",
                       consumed, nalsize);

            buf_index += consumed;
            nal_index++;

            if (avctx->skip_frame >= AVDISCARD_NONREF &&
                h->nal_ref_idc == 0 &&
                h->nal_unit_type != NAL_SEI)
                continue;

again:
            if (   !(avctx->active_thread_type & TT_THREAD_FRAME)
                || nals_needed >= nal_index)
                h->au_pps_id = -1;
            /* Ignore per frame NAL unit type during extradata
             * parsing. Decoding slices is not possible in codec init
             * with frame-mt */
            if (parse_extradata) {
                switch (hx->nal_unit_type) {
                case NAL_IDR_SLICE:
                case NAL_SLICE:
                case NAL_DPA:
                case NAL_DPB:
                case NAL_DPC:
                    ttv_log(h->avctx, TTV_LOG_WARNING,
                           "Ignoring NAL %d in global header/extradata\n",
                           hx->nal_unit_type);
                    // fall through to next case
                case NAL_AUXILIARY_SLICE:
                    hx->nal_unit_type = NAL_TT_IGNORE;
                }
            }

            err = 0;

            switch (hx->nal_unit_type) {
            case NAL_IDR_SLICE:
                if ((ptr[0] & 0xFC) == 0x98) {
                    ttv_log(h->avctx, TTV_LOG_ERROR, "Invalid inter IDR frame\n");
                    h->next_outputed_poc = INT_MIN;
                    ret = -1;
                    goto end;
                }
                if (h->nal_unit_type != NAL_IDR_SLICE) {
                    ttv_log(h->avctx, TTV_LOG_ERROR,
                           "Invalid mix of idr and non-idr slices\n");
                    ret = -1;
                    goto end;
                }
                if(!idr_cleared)
                    idr(h); // FIXME ensure we don't lose some frames if there is reordering
                idr_cleared = 1;
                h->has_recovery_point = 1;
            case NAL_SLICE:
                init_get_bits(&hx->gb, ptr, bit_length);
                hx->intra_gb_ptr      =
                hx->inter_gb_ptr      = &hx->gb;
                hx->data_partitioning = 0;

                if ((err = tt_h264_decode_slice_header(hx, h)))
                    break;

                if (h->sei_recovery_frame_cnt >= 0) {
                    if (h->frame_num != h->sei_recovery_frame_cnt || hx->slice_type_nos != TTV_PICTURE_TYPE_I)
                        h->valid_recovery_point = 1;

                    if (   h->recovery_frame < 0
                        || ((h->recovery_frame - h->frame_num) & ((1 << h->sps.log2_max_frame_num)-1)) > h->sei_recovery_frame_cnt) {
                        h->recovery_frame = (h->frame_num + h->sei_recovery_frame_cnt) &
                                            ((1 << h->sps.log2_max_frame_num) - 1);

                        if (!h->valid_recovery_point)
                            h->recovery_frame = h->frame_num;
                    }
                }

                h->cur_pic_ptr->f.key_frame |=
                    (hx->nal_unit_type == NAL_IDR_SLICE);

                if (hx->nal_unit_type == NAL_IDR_SLICE ||
                    h->recovery_frame == h->frame_num) {
                    h->recovery_frame         = -1;
                    h->cur_pic_ptr->recovered = 1;
                }
                // If we have an IDR, all frames after it in decoded order are
                // "recovered".
                if (hx->nal_unit_type == NAL_IDR_SLICE)
                    h->frame_recovered |= FRAME_RECOVERED_IDR;
                h->frame_recovered |= 3*!!(avctx->flags2 & CODEC_FLAG2_SHOW_ALL);
                h->frame_recovered |= 3*!!(avctx->flags & CODEC_FLAG_OUTPUT_CORRUPT);
#if 1
                h->cur_pic_ptr->recovered |= h->frame_recovered;
#else
                h->cur_pic_ptr->recovered |= !!(h->frame_recovered & FRAME_RECOVERED_IDR);
#endif

                if (h->current_slice == 1) {
                    if (!(avctx->flags2 & CODEC_FLAG2_CHUNKS))
                        decode_postinit(h, nal_index >= nals_needed);

//                     if (h->avctx->hwaccel &&
//                         (ret = h->avctx->hwaccel->start_frame(h->avctx, NULL, 0)) < 0)
//                         return ret;
//                     if (CONFIG_H264_VDPAU_DECODER &&
//                         h->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU)
//                         tt_vdpau_h264_picture_start(h);
                }

                if (hx->redundant_pic_count == 0) {
//                     if (avctx->hwaccel) {
//                         ret = avctx->hwaccel->decode_slice(avctx,
//                                                            &buf[buf_index - consumed],
//                                                            consumed);
//                         if (ret < 0)
//                             return ret;
//                     } 
// 					else if (CONFIG_H264_VDPAU_DECODER &&
//                                h->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU) {
//                         tt_vdpau_add_data_chunk(h->cur_pic_ptr->f.data[0],
//                                                 start_code,
//                                                 sizeof(start_code));
//                         tt_vdpau_add_data_chunk(h->cur_pic_ptr->f.data[0],
//                                                 &buf[buf_index - consumed],
//                                                 consumed);
//                     } 
//					else
                        context_count++;
                }
                break;
            case NAL_DPA:
                if (h->avctx->flags & CODEC_FLAG2_CHUNKS) {
                    ttv_log(h->avctx, TTV_LOG_ERROR,
                           "Decoding in chunks is not supported for "
                           "partitioned slices.\n");
                    return AVERROR(ENOSYS);
                }

                init_get_bits(&hx->gb, ptr, bit_length);
                hx->intra_gb_ptr =
                hx->inter_gb_ptr = NULL;

                if ((err = tt_h264_decode_slice_header(hx, h))) {
                    /* make sure data_partitioning is cleared if it was set
                     * before, so we don't try decoding a slice without a valid
                     * slice header later */
                    h->data_partitioning = 0;
                    break;
                }

                hx->data_partitioning = 1;
                break;
            case NAL_DPB:
                init_get_bits(&hx->intra_gb, ptr, bit_length);
                hx->intra_gb_ptr = &hx->intra_gb;
                break;
            case NAL_DPC:
                init_get_bits(&hx->inter_gb, ptr, bit_length);
                hx->inter_gb_ptr = &hx->inter_gb;

                ttv_log(h->avctx, TTV_LOG_ERROR, "Partitioned H.264 support is incomplete\n");
                break;

                if (hx->redundant_pic_count == 0 &&
                    hx->intra_gb_ptr &&
                    hx->data_partitioning &&
                    h->cur_pic_ptr && h->context_initialized &&
                    (avctx->skip_frame < AVDISCARD_NONREF || hx->nal_ref_idc) &&
                    (avctx->skip_frame < AVDISCARD_BIDIR  ||
                     hx->slice_type_nos != TTV_PICTURE_TYPE_B) &&
                    (avctx->skip_frame < AVDISCARD_NONINTRA ||
                     hx->slice_type_nos == TTV_PICTURE_TYPE_I) &&
                    avctx->skip_frame < AVDISCARD_ALL)
                    context_count++;
                break;
            case NAL_SEI:
                init_get_bits(&h->gb, ptr, bit_length);
                ret = tt_h264_decode_sei(h);
                if (ret < 0 && (h->avctx->err_recognition & TTV_EF_EXPLODE))
                    goto end;
                break;
            case NAL_SPS:
                init_get_bits(&h->gb, ptr, bit_length);
                if (tt_h264_decode_seq_parameter_set(h) < 0 && (h->is_avc ? nalsize : 1)) {
                    ttv_log(h->avctx, TTV_LOG_DEBUG,
                           "SPS decoding failure, trying again with the complete NAL\n");
                    if (h->is_avc)
                        ttv_assert0(next_avc - buf_index + consumed == nalsize);
                    if ((next_avc - buf_index + consumed - 1) >= INT_MAX/8)
                        break;
                    init_get_bits(&h->gb, &buf[buf_index + 1 - consumed],
                                  8*(next_avc - buf_index + consumed - 1));
                    tt_h264_decode_seq_parameter_set(h);
                }

                break;
            case NAL_PPS:
                init_get_bits(&h->gb, ptr, bit_length);
                ret = tt_h264_decode_picture_parameter_set(h, bit_length);
                if (ret < 0 && (h->avctx->err_recognition & TTV_EF_EXPLODE))
                    goto end;
                break;
            case NAL_AUD:
            case NAL_END_SEQUENCE:
            case NAL_END_STREAM:
            case NAL_FILLER_DATA:
            case NAL_SPS_EXT:
            case NAL_AUXILIARY_SLICE:
                break;
            case NAL_TT_IGNORE:
                break;
            default:
                ttv_log(avctx, TTV_LOG_DEBUG, "Unknown NAL code: %d (%d bits)\n",
                       hx->nal_unit_type, bit_length);
            }

            if (context_count == h->max_contexts) {
                ret = tt_h264_execute_decode_slices(h, context_count);
                if (ret < 0 && (h->avctx->err_recognition & TTV_EF_EXPLODE))
                    goto end;
                context_count = 0;
            }

            if (err < 0 || err == SLICE_SKIPED) {
                if (err < 0)
                    ttv_log(h->avctx, TTV_LOG_ERROR, "decode_slice_header error\n");
                h->ref_count[0] = h->ref_count[1] = h->list_count = 0;
            } else if (err == SLICE_SINGLETHREAD) {
                /* Slice could not be decoded in parallel mode, copy down
                 * NAL unit stuff to context 0 and restart. Note that
                 * rbsp_buffer is not transferred, but since we no longer
                 * run in parallel mode this should not be an issue. */
                h->nal_unit_type = hx->nal_unit_type;
                h->nal_ref_idc   = hx->nal_ref_idc;
                hx               = h;
                goto again;
            }
        }
    }
    if (context_count) {
        ret = tt_h264_execute_decode_slices(h, context_count);
        if (ret < 0 && (h->avctx->err_recognition & TTV_EF_EXPLODE))
            goto end;
    }

    ret = 0;
end:
    /* clean up */
    if (h->cur_pic_ptr && !h->droppable) {
        tt_thread_report_progress(&h->cur_pic_ptr->tf, INT_MAX,
                                  h->picture_structure == PICT_BOTTOM_FIELD);
    }

    return (ret < 0) ? ret : buf_index;
}

/**
 * Return the number of bytes consumed for building the current frame.
 */
static int get_consumed_bytes(int pos, int buf_size)
{
    if (pos == 0)
        pos = 1;        // avoid infinite loops (I doubt that is needed but...)
    if (pos + 10 > buf_size)
        pos = buf_size; // oops ;)

    return pos;
}

static int output_frame(H264Context *h, TTFrame *dst, H264Picture *srcp)
{
    TTFrame *src = &srcp->f;
    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(src->format);
    int i;
    int ret = ttv_frame_ref(dst, src);
    if (ret < 0)
        return ret;

    ttv_dict_set(&dst->metadata, "stereo_mode", tt_h264_sei_stereo_mode(h), 0);

    if (srcp->sei_recovery_frame_cnt == 0)
        dst->key_frame = 1;
    if (!srcp->crop)
        return 0;

    for (i = 0; i < desc->nb_components; i++) {
        int hshift = (i > 0) ? desc->log2_chroma_w : 0;
        int vshift = (i > 0) ? desc->log2_chroma_h : 0;
        int off    = ((srcp->crop_left >> hshift) << h->pixel_shift) +
                      (srcp->crop_top  >> vshift) * dst->linesize[i];
        dst->data[i] += off;
    }
    return 0;
}

static int is_extra(const uint8_t *buf, int buf_size)
{
    int cnt= buf[5]&0x1f;
    const uint8_t *p= buf+6;
    while(cnt--){
        int nalsize= TTV_RB16(p) + 2;
        if(nalsize > buf_size - (p-buf) || p[2]!=0x67)
            return 0;
        p += nalsize;
    }
    cnt = *(p++);
    if(!cnt)
        return 0;
    while(cnt--){
        int nalsize= TTV_RB16(p) + 2;
        if(nalsize > buf_size - (p-buf) || p[2]!=0x68)
            return 0;
        p += nalsize;
    }
    return 1;
}

static int h264_decode_frame(TTCodecContext *avctx, void *data,
                             int *got_frame, TTPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size       = avpkt->size;
    H264Context *h     = avctx->priv_data;
    TTFrame *pict      = data;
    int buf_index      = 0;
    H264Picture *out;
    int i, out_idx;
    int ret;

    h->flags = avctx->flags;
    /* reset data partitioning here, to ensure GetBitContexts from previous
     * packets do not get used. */
    h->data_partitioning = 0;

    /* end of stream, output what is still in the buffers */
    if (buf_size == 0) {
 out:

        h->cur_pic_ptr = NULL;
        h->first_field = 0;

        // FIXME factorize this with the output code below
        out     = h->delayed_pic[0];
        out_idx = 0;
        for (i = 1;
             h->delayed_pic[i] &&
             !h->delayed_pic[i]->f.key_frame &&
             !h->delayed_pic[i]->mmco_reset;
             i++)
            if (h->delayed_pic[i]->poc < out->poc) {
                out     = h->delayed_pic[i];
                out_idx = i;
            }

        for (i = out_idx; h->delayed_pic[i]; i++)
            h->delayed_pic[i] = h->delayed_pic[i + 1];

        if (out) {
            out->reference &= ~DELAYED_PIC_REF;
            ret = output_frame(h, pict, out);
            if (ret < 0)
                return ret;
            *got_frame = 1;
        }

        return buf_index;
    }
    if (h->is_avc && ttv_packet_get_side_data(avpkt, TTV_PKT_DATA_NEW_EXTRADATA, NULL)) {
        int side_size;
        uint8_t *side = ttv_packet_get_side_data(avpkt, TTV_PKT_DATA_NEW_EXTRADATA, &side_size);
        if (is_extra(side, side_size))
            tt_h264_decode_extradata(h, side, side_size);
    }
    if(h->is_avc && buf_size >= 9 && buf[0]==1 && buf[2]==0 && (buf[4]&0xFC)==0xFC && (buf[5]&0x1F) && buf[8]==0x67){
        if (is_extra(buf, buf_size))
            return tt_h264_decode_extradata(h, buf, buf_size);
    }

    buf_index = decode_nal_units(h, buf, buf_size, 0);
    if (buf_index < 0)
        return TTERROR_INVALIDDATA;

    if (!h->cur_pic_ptr && h->nal_unit_type == NAL_END_SEQUENCE) {
        ttv_assert0(buf_index <= buf_size);
        goto out;
    }

    if (!(avctx->flags2 & CODEC_FLAG2_CHUNKS) && !h->cur_pic_ptr) {
        if (avctx->skip_frame >= AVDISCARD_NONREF ||
            buf_size >= 4 && !memcmp("Q264", buf, 4))
            return buf_size;
        ttv_log(avctx, TTV_LOG_ERROR, "no frame!\n");
        return TTERROR_INVALIDDATA;
    }

    if (!(avctx->flags2 & CODEC_FLAG2_CHUNKS) ||
        (h->mb_y >= h->mb_height && h->mb_height)) {
        if (avctx->flags2 & CODEC_FLAG2_CHUNKS)
            decode_postinit(h, 1);

        tt_h264_field_end(h, 0);

        /* Wait for second field. */
        *got_frame = 0;
        if (h->next_output_pic && (
                                   h->next_output_pic->recovered)) {
            if (!h->next_output_pic->recovered)
                h->next_output_pic->f.flags |= TTV_FRAME_FLAG_CORRUPT;

            ret = output_frame(h, pict, h->next_output_pic);
            if (ret < 0)
                return ret;
            *got_frame = 1;
//             if (CONFIG_MPEGVIDEO) {
//                 tt_print_debug_info2(h->avctx, pict, h->er.mbskip_table,
//                                     h->next_output_pic->mb_type,
//                                     h->next_output_pic->qscale_table,
//                                     h->next_output_pic->motion_val,
//                                     &h->low_delay,
//                                     h->mb_width, h->mb_height, h->mb_stride, 1);
//             }
        }
    }

    assert(pict->buf[0] || !*got_frame);

    return get_consumed_bytes(buf_index, buf_size);
}

ttv_cold void tt_h264_free_context(H264Context *h)
{
    int i;

    tt_h264_free_tables(h, 1); // FIXME cleanup init stuff perhaps

    for (i = 0; i < MAX_SPS_COUNT; i++)
        ttv_freep(h->sps_buffers + i);

    for (i = 0; i < MAX_PPS_COUNT; i++)
        ttv_freep(h->pps_buffers + i);
}

static ttv_cold int h264_decode_end(TTCodecContext *avctx)
{
    H264Context *h = avctx->priv_data;

    tt_h264_remove_all_refs(h);
    tt_h264_free_context(h);

    tt_h264_unref_picture(h, &h->cur_pic);

    return 0;
}

static const TTProfile profiles[] = {
    { TT_PROFILE_H264_BASELINE,             "Baseline"              },
    { TT_PROFILE_H264_CONSTRAINED_BASELINE, "Constrained Baseline"  },
    { TT_PROFILE_H264_MAIN,                 "Main"                  },
    { TT_PROFILE_H264_EXTENDED,             "Extended"              },
    { TT_PROFILE_H264_HIGH,                 "High"                  },
    { TT_PROFILE_H264_HIGH_10,              "High 10"               },
    { TT_PROFILE_H264_HIGH_10_INTRA,        "High 10 Intra"         },
    { TT_PROFILE_H264_HIGH_422,             "High 4:2:2"            },
    { TT_PROFILE_H264_HIGH_422_INTRA,       "High 4:2:2 Intra"      },
    { TT_PROFILE_H264_HIGH_444,             "High 4:4:4"            },
    { TT_PROFILE_H264_HIGH_444_PREDICTIVE,  "High 4:4:4 Predictive" },
    { TT_PROFILE_H264_HIGH_444_INTRA,       "High 4:4:4 Intra"      },
    { TT_PROFILE_H264_CAVLC_444,            "CAVLC 4:4:4"           },
    { TT_PROFILE_UNKNOWN },
};

static const AVOption h264_options[] = {
    {"is_avc", "is avc", offsetof(H264Context, is_avc), TT_OPT_TYPE_INT, {/*.i64 =*/ 0}, 0, 1, 0},
    {"nal_length_size", "nal_length_size", offsetof(H264Context, nal_length_size), TT_OPT_TYPE_INT, {/*.i64 =*/ 0}, 0, 4, 0},
    {NULL}
};



const char *ttv_default_item_name(void *ptr)
{
	return (*(AVClass **) ptr)->class_name;
}


static const AVClass h264_class = {
   "H264 Decoder",
   ttv_default_item_name,
   h264_options,
   LIB__TTPOD_TT_VERSION_INT,
};

TTCodec tt_h264_decoder = {
	"h264",
	NULL_IF_CONFIG_SMALL("H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"),
	AVMEDIA_TYPE_VIDEO,
	TTV_CODEC_ID_H264,
	sizeof(H264Context),
	/*CODEC_CAP_DRAW_HORIZ_BAND |*/ CODEC_CAP_DR1 | CODEC_CAP_DELAY | CODEC_CAP_SLICE_THREADS | CODEC_CAP_FRAME_THREADS,
	NULL,
	&h264_class,
	tt_h264_decode_init,
	h264_decode_frame,
	h264_decode_end,
	NULL,
	NULL,
	flush_dpb,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL_IF_CONFIG_SMALL(profiles),
	NULL,
	ONLY_IF_THREADS_ENABLED(decode_init_thread_copy),
	ONLY_IF_THREADS_ENABLED(tt_h264_update_thread_context),
};

