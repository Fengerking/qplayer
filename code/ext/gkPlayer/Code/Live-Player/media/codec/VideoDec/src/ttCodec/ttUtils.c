#include "config.h"
#include "ttAtomic.h"
#include "ttAttributes.h"
#include "ttAvassert.h"
#include "ttFrame.h"
#include "ttInternal.h"
#include "ttMathematics.h"
#include "ttPixdesc.h"
#include "ttImgutils.h"
#include "ttDict.h"
#include "ttAvcodec.h"
#include "ttOpt.h"
#include "ttThread.h"
#include "ttIntreadwrite.h"
#include "ttGetBits.h"


#if CONFIG_ICONV
# include <iconv.h>
#endif

#if HAVE_PTHREADS
#include <pthread.h>
#elif HAVE_W32THREADS
#include "compat/w32pthreads.h"
#elif HAVE_OS2THREADS
#include "compat/os2threads.h"
#endif

#if HAVE_PTHREADS || HAVE_W32THREADS || HAVE_OS2THREADS
static int default_lockmgr_cb(void **arg, enum AVLockOp op)
{
    void * volatile * mutex = arg;
    int err;

    switch (op) {
    case TTV_LOCK_CREATE:
        return 0;
    case TTV_LOCK_OBTAIN:
        if (!*mutex) {
            pthread_mutex_t *tmp = ttv_malloc(sizeof(pthread_mutex_t));
            if (!tmp)
                return AVERROR(ENOMEM);
            if ((err = pthread_mutex_init(tmp, NULL))) {
                ttv_free(tmp);
                return AVERROR(err);
            }
            if (ttpriv_atomic_ptr_cas(mutex, NULL, tmp)) {
                pthread_mutex_destroy(tmp);
                ttv_free(tmp);
            }
        }

        if ((err = pthread_mutex_lock(*mutex)))
            return AVERROR(err);

        return 0;
    case TTV_LOCK_RELEASE:
        if ((err = pthread_mutex_unlock(*mutex)))
            return AVERROR(err);

        return 0;
    case TTV_LOCK_DESTROY:
        if (*mutex)
            pthread_mutex_destroy(*mutex);
        ttv_free(*mutex);
        ttpriv_atomic_ptr_cas(mutex, *mutex, NULL);
        return 0;
    }
    return 1;
}
static int (*lockmgr_cb)(void **mutex, enum AVLockOp op) = default_lockmgr_cb;
#else
static int (*lockmgr_cb)(void **mutex, enum AVLockOp op) = NULL;
#endif

extern TTCodec tt_h264_decoder;

void ttcodec_register(void)
{
	static int initialized;

	if (initialized)
		return;
	initialized = 1;

	ttcodec_register_c(&tt_h264_decoder);
}

volatile int tt_ttcodec_locked;
static int volatile entangled_thread_counter = 0;
static void *codec_mutex;
//static void *avformat_mutex;

static inline int tt_fast_malloc(void *ptr, unsigned int *size, size_t min_size, int zero_realloc)
{
    void **p = ptr;
    if (min_size <= *size && *p)
        return 0;
    min_size = FFMAX(17 * min_size / 16 + 32, min_size);
    ttv_free(*p);
    *p = zero_realloc ? ttv_mallocz(min_size) : ttv_malloc(min_size);
    if (!*p)
        min_size = 0;
    *size = min_size;
    return 1;
}

void ttv_fast_padded_malloc(void *ptr, unsigned int *size, size_t min_size)
{
    uint8_t **p = ptr;
    if (min_size > SIZE_MAX - TT_INPUT_BUFFER_PADDING_SIZE) {
        ttv_freep(p);
        *size = 0;
        return;
    }
    if (!tt_fast_malloc(p, size, min_size + TT_INPUT_BUFFER_PADDING_SIZE, 1))
        memset(*p + min_size, 0, TT_INPUT_BUFFER_PADDING_SIZE);
}



/* encoder management */
static TTCodec *first_avcodec = NULL;
static TTCodec **last_avcodec = &first_avcodec;

TTCodec *ttv_codec_next(const TTCodec *c)
{
    if (c)
        return c->next;
    else
        return first_avcodec;
}


int ttv_codec_is_decoder(const TTCodec *codec)
{
    return codec && codec->decode;
}

ttv_cold void ttcodec_register_c(TTCodec *codec)
{
    TTCodec **p;
    p = last_avcodec;
    codec->next = NULL;

    while(*p || ttpriv_atomic_ptr_cas((void * volatile *)p, NULL, codec))
        p = &(*p)->next;
    last_avcodec = &codec->next;

    if (codec->init_static_data)
        codec->init_static_data(codec);
}

int tt_set_dimensions(TTCodecContext *s, int width, int height)
{
    int ret = ttv_image_check_size(width, height, 0, s);

    if (ret < 0)
        width = height = 0;

    s->coded_width  = width;
    s->coded_height = height;
    s->width        = TT_CEIL_RSHIFT(width,  s->lowres);
    s->height       = TT_CEIL_RSHIFT(height, s->lowres);

    return ret;
}

int tt_set_sar(TTCodecContext *avctx, TTRational sar)
{
    int ret = ttv_image_check_sar(avctx->width, avctx->height, sar);

    if (ret < 0) {
        ttv_log(avctx, TTV_LOG_WARNING, "ignoring invalid SAR: %u/%u\n",
               sar.num, sar.den);
        avctx->sample_aspect_ratio = ttv_make_q(0, 1);
        return ret;
    } else {
        avctx->sample_aspect_ratio = sar;
    }
    return 0;
}



void ttcodec_align_dimensions2(TTCodecContext *s, int *width, int *height,
                               int linesize_align[TTV_NUM_DATA_POINTERS])
{
    int i;
    int w_align = 1;
    int h_align = 1;
    AVPixFmtDescriptor const *desc = ttv_pix_fmt_desc_get(s->pix_fmt);

    if (desc) {
        w_align = 1 << desc->log2_chroma_w;
        h_align = 1 << desc->log2_chroma_h;
    }

    switch (s->pix_fmt) {
    case TTV_PIX_FMT_YUV420P:
    case TTV_PIX_FMT_YUYV422:
//    case TTV_PIX_FMT_YVYU422:
    case TTV_PIX_FMT_UYVY422:
    case TTV_PIX_FMT_YUV422P:
    case TTV_PIX_FMT_YUV440P:
    case TTV_PIX_FMT_YUV444P:
    case TTV_PIX_FMT_GBRAP:
    case TTV_PIX_FMT_GBRP:
    case TTV_PIX_FMT_GRAY8:
    case TTV_PIX_FMT_GRAY16BE:
    case TTV_PIX_FMT_GRAY16LE:
    case TTV_PIX_FMT_YUVJ420P:
    case TTV_PIX_FMT_YUVJ422P:
    case TTV_PIX_FMT_YUVJ440P:
    case TTV_PIX_FMT_YUVJ444P:
    case TTV_PIX_FMT_YUVA420P:
    case TTV_PIX_FMT_YUVA422P:
    case TTV_PIX_FMT_YUVA444P:
    case TTV_PIX_FMT_YUV420P9LE:
    case TTV_PIX_FMT_YUV420P9BE:
    case TTV_PIX_FMT_YUV420P10LE:
    case TTV_PIX_FMT_YUV420P10BE:
    case TTV_PIX_FMT_YUV420P12LE:
    case TTV_PIX_FMT_YUV420P12BE:
    case TTV_PIX_FMT_YUV420P14LE:
    case TTV_PIX_FMT_YUV420P14BE:
    case TTV_PIX_FMT_YUV420P16LE:
    case TTV_PIX_FMT_YUV420P16BE:
    case TTV_PIX_FMT_YUVA420P9LE:
    case TTV_PIX_FMT_YUVA420P9BE:
    case TTV_PIX_FMT_YUVA420P10LE:
    case TTV_PIX_FMT_YUVA420P10BE:
    case TTV_PIX_FMT_YUVA420P16LE:
    case TTV_PIX_FMT_YUVA420P16BE:
    case TTV_PIX_FMT_YUV422P9LE:
    case TTV_PIX_FMT_YUV422P9BE:
    case TTV_PIX_FMT_YUV422P10LE:
    case TTV_PIX_FMT_YUV422P10BE:
    case TTV_PIX_FMT_YUV422P12LE:
    case TTV_PIX_FMT_YUV422P12BE:
    case TTV_PIX_FMT_YUV422P14LE:
    case TTV_PIX_FMT_YUV422P14BE:
    case TTV_PIX_FMT_YUV422P16LE:
    case TTV_PIX_FMT_YUV422P16BE:
    case TTV_PIX_FMT_YUVA422P9LE:
    case TTV_PIX_FMT_YUVA422P9BE:
    case TTV_PIX_FMT_YUVA422P10LE:
    case TTV_PIX_FMT_YUVA422P10BE:
    case TTV_PIX_FMT_YUVA422P16LE:
    case TTV_PIX_FMT_YUVA422P16BE:
    case TTV_PIX_FMT_YUV444P9LE:
    case TTV_PIX_FMT_YUV444P9BE:
    case TTV_PIX_FMT_YUV444P10LE:
    case TTV_PIX_FMT_YUV444P10BE:
    case TTV_PIX_FMT_YUV444P12LE:
    case TTV_PIX_FMT_YUV444P12BE:
    case TTV_PIX_FMT_YUV444P14LE:
    case TTV_PIX_FMT_YUV444P14BE:
    case TTV_PIX_FMT_YUV444P16LE:
    case TTV_PIX_FMT_YUV444P16BE:
    case TTV_PIX_FMT_YUVA444P9LE:
    case TTV_PIX_FMT_YUVA444P9BE:
    case TTV_PIX_FMT_YUVA444P10LE:
    case TTV_PIX_FMT_YUVA444P10BE:
    case TTV_PIX_FMT_YUVA444P16LE:
    case TTV_PIX_FMT_YUVA444P16BE:
    case TTV_PIX_FMT_GBRP9LE:
    case TTV_PIX_FMT_GBRP9BE:
    case TTV_PIX_FMT_GBRP10LE:
    case TTV_PIX_FMT_GBRP10BE:
    case TTV_PIX_FMT_GBRP12LE:
    case TTV_PIX_FMT_GBRP12BE:
    case TTV_PIX_FMT_GBRP14LE:
    case TTV_PIX_FMT_GBRP14BE:
    case TTV_PIX_FMT_GBRP16LE:
    case TTV_PIX_FMT_GBRP16BE:
        w_align = 16; //FIXME assume 16 pixel per macroblock
        h_align = 16 * 2; // interlaced needs 2 macroblocks height
        break;
    case TTV_PIX_FMT_YUV411P:
    case TTV_PIX_FMT_YUVJ411P:
    case TTV_PIX_FMT_UYYVYY411:
        w_align = 32;
        h_align = 8;
        break;
    case TTV_PIX_FMT_YUV410P:
        if (s->codec_id == TTV_CODEC_ID_SVQ1) {
            w_align = 64;
            h_align = 64;
        }
        break;
    case TTV_PIX_FMT_RGB555:
        if (s->codec_id == TTV_CODEC_ID_RPZA) {
            w_align = 4;
            h_align = 4;
        }
        break;
    case TTV_PIX_FMT_PAL8:
    case TTV_PIX_FMT_BGR8:
    case TTV_PIX_FMT_RGB8:
        if (s->codec_id == TTV_CODEC_ID_SMC ||
            s->codec_id == TTV_CODEC_ID_CINEPAK) {
            w_align = 4;
            h_align = 4;
        }
        if (s->codec_id == TTV_CODEC_ID_JV) {
            w_align = 8;
            h_align = 8;
        }
        break;
    case TTV_PIX_FMT_BGR24:
        if ((s->codec_id == TTV_CODEC_ID_MSZH) ||
            (s->codec_id == TTV_CODEC_ID_ZLIB)) {
            w_align = 4;
            h_align = 4;
        }
        break;
    case TTV_PIX_FMT_RGB24:
        if (s->codec_id == TTV_CODEC_ID_CINEPAK) {
            w_align = 4;
            h_align = 4;
        }
        break;
    default:
        break;
    }

    if (s->codec_id == TTV_CODEC_ID_ITT_ILBM || s->codec_id == TTV_CODEC_ID_ITT_BYTERUN1) {
        w_align = FFMAX(w_align, 8);
    }

    *width  = FFALIGN(*width, w_align);
    *height = FFALIGN(*height, h_align);
    if (s->codec_id == TTV_CODEC_ID_H264 || s->lowres)
        // some of the optimized chroma MC reads one line too much
        // which is also done in mpeg decoders with lowres > 0
        *height += 2;

    for (i = 0; i < 4; i++)
        linesize_align[i] = STRIDE_ALIGN;
}


static int update_frame_pool(TTCodecContext *avctx, TTFrame *frame)
{
    FramePool *pool = avctx->internal->pool;
    int i, ret;

    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_VIDEO: {
        TTPicture picture;
        int size[4] = { 0 };
        int w = frame->width;
        int h = frame->height;
        int tmpsize, unaligned;

        if (pool->format == frame->format &&
            pool->width == frame->width && pool->height == frame->height)
            return 0;

        ttcodec_align_dimensions2(avctx, &w, &h, pool->stride_align);

        do {
            // NOTE: do not align linesizes individually, this breaks e.g. assumptions
            // that linesize[0] == 2*linesize[1] in the MPEG-encoder for 4:2:2
            ttv_image_fill_linesizes(picture.linesize, avctx->pix_fmt, w);
            // increase alignment of w for next try (rhs gives the lowest bit set in w)
            w += w & ~(w - 1);

            unaligned = 0;
            for (i = 0; i < 4; i++)
                unaligned |= picture.linesize[i] % pool->stride_align[i];
        } while (unaligned);

        tmpsize = ttv_image_fill_pointers(picture.data, avctx->pix_fmt, h,
                                         NULL, picture.linesize);
        if (tmpsize < 0)
            return -1;

        for (i = 0; i < 3 && picture.data[i + 1]; i++)
            size[i] = picture.data[i + 1] - picture.data[i];
        size[i] = tmpsize - (picture.data[i] - picture.data[0]);

        for (i = 0; i < 4; i++) {
            ttv_buffer_pool_uninit(&pool->pools[i]);
            pool->linesize[i] = picture.linesize[i];
            if (size[i]) {
                pool->pools[i] = ttv_buffer_pool_init(size[i] + 16 + STRIDE_ALIGN - 1,
                                                     CONFIG_MEMORY_POISONING ?
                                                        NULL :
                                                        ttv_buffer_allocz);
                if (!pool->pools[i]) {
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }
            }
        }
        pool->format = frame->format;
        pool->width  = frame->width;
        pool->height = frame->height;

        break;
        }
    default: ttv_assert0(0);
    }
    return 0;
fail:
    for (i = 0; i < 4; i++)
        ttv_buffer_pool_uninit(&pool->pools[i]);
    pool->format = -1;
    pool->planes =  pool->samples = 0;
    pool->width  = pool->height = 0;
    return ret;
}


static int video_get_buffer(TTCodecContext *s, TTFrame *pic)
{
    FramePool *pool = s->internal->pool;
    int i;

    if (pic->data[0]) {
        ttv_log(s, TTV_LOG_ERROR, "pic->data[0]!=NULL in ttcodec_default_get_buffer\n");
        return -1;
    }

    memset(pic->data, 0, sizeof(pic->data));
    pic->extended_data = pic->data;

    for (i = 0; i < 4 && pool->pools[i]; i++) {
        pic->linesize[i] = pool->linesize[i];

        pic->buf[i] = ttv_buffer_pool_get(pool->pools[i]);
        if (!pic->buf[i])
            goto fail;

        pic->data[i] = pic->buf[i]->data;
    }
    for (; i < TTV_NUM_DATA_POINTERS; i++) {
        pic->data[i] = NULL;
        pic->linesize[i] = 0;
    }
    if (pic->data[1] && !pic->data[2])
        ttpriv_set_systematic_pal2((uint32_t *)pic->data[1], s->pix_fmt);

    if (s->debug & TT_DEBUG_BUFFERS)
        ttv_log(s, TTV_LOG_DEBUG, "default_get_buffer called on pic %p\n", pic);

    return 0;
fail:
    ttv_frame_unref(pic);
    return AVERROR(ENOMEM);
}

void ttpriv_color_frame(TTFrame *frame, const int c[4])
{
    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(frame->format);
    int p, y, x;

    ttv_assert0(desc->flags & TTV_PIX_FMT_FLAG_PLANAR);

    for (p = 0; p<desc->nb_components; p++) {
        uint8_t *dst = frame->data[p];
        int is_chroma = p == 1 || p == 2;
        int bytes  = is_chroma ? TT_CEIL_RSHIFT(frame->width,  desc->log2_chroma_w) : frame->width;
        int height = is_chroma ? TT_CEIL_RSHIFT(frame->height, desc->log2_chroma_h) : frame->height;
        for (y = 0; y < height; y++) {
            if (desc->comp[0].depth_minus1 >= 8) {
                for (x = 0; x<bytes; x++)
                    ((uint16_t*)dst)[x] = c[p];
            }else
                memset(dst, c[p], bytes);
            dst += frame->linesize[p];
        }
    }
}

int ttcodec_default_get_buffer2(TTCodecContext *avctx, TTFrame *frame, int flags)
{
    int ret;

    if ((ret = update_frame_pool(avctx, frame)) < 0)
        return ret;

#if TT_API_GET_BUFFER
TT_DISABLE_DEPRECATION_WARNINGS
    frame->type = TT_BUFFER_TYPE_INTERNAL;
TT_ENABLE_DEPRECATION_WARNINGS
#endif

    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        return video_get_buffer(avctx, frame);
    default:
        return -1;
    }
}

int tt_init_buffer_info(TTCodecContext *avctx, TTFrame *frame)
{
    TTPacket *pkt = avctx->internal->pkt;
    int i;
    static const struct {
        enum TTPacketSideDataType packet;
        enum TTFrameSideDataType frame;
    } sd[] = {
        { TTV_PKT_DATA_REPLAYGAIN ,   TTV_FRAME_DATA_REPLAYGAIN },
        { TTV_PKT_DATA_DISPLAYMATRIX, TTV_FRAME_DATA_DISPLAYMATRIX },
        { TTV_PKT_DATA_STEREO3D,      TTV_FRAME_DATA_STEREO3D },
    };

    if (pkt) {
        frame->pkt_pts = pkt->pts;
        ttv_frame_set_pkt_pos     (frame, pkt->pos);
        ttv_frame_set_pkt_duration(frame, pkt->duration);
        ttv_frame_set_pkt_size    (frame, pkt->size);

        for (i = 0; i < TT_ARRAY_ELEMS(sd); i++) {
            int size;
            uint8_t *packet_sd = ttv_packet_get_side_data(pkt, sd[i].packet, &size);
            if (packet_sd) {
                TTFrameSideData *frame_sd = ttv_frame_new_side_data(frame,
                                                                   sd[i].frame,
                                                                   size);
                if (!frame_sd)
                    return AVERROR(ENOMEM);

                memcpy(frame_sd->data, packet_sd, size);
            }
        }
    } else {
        frame->pkt_pts = TTV_NOPTS_VALUE;
        ttv_frame_set_pkt_pos     (frame, -1);
        ttv_frame_set_pkt_duration(frame, 0);
        ttv_frame_set_pkt_size    (frame, -1);
    }
    frame->reordered_opaque = avctx->reordered_opaque;

    if (frame->color_primaries == AVCOL_PRI_UNSPECIFIED)
        frame->color_primaries = avctx->color_primaries;
    if (frame->color_trc == AVCOL_TRC_UNSPECIFIED)
        frame->color_trc = avctx->color_trc;
    if (ttv_frame_get_colorspace(frame) == AVCOL_SPC_UNSPECIFIED)
        ttv_frame_set_colorspace(frame, avctx->colorspace);
    if (ttv_frame_get_color_range(frame) == AVCOL_RANGE_UNSPECIFIED)
        ttv_frame_set_color_range(frame, avctx->color_range);
    if (frame->chroma_location == AVCHROMA_LOC_UNSPECIFIED)
        frame->chroma_location = avctx->chroma_sample_location;

    switch (avctx->codec->type) {
    case AVMEDIA_TYPE_VIDEO:
        frame->format              = avctx->pix_fmt;
        if (!frame->sample_aspect_ratio.num)
            frame->sample_aspect_ratio = avctx->sample_aspect_ratio;

        if (frame->width && frame->height &&
            ttv_image_check_sar(frame->width, frame->height,
                               frame->sample_aspect_ratio) < 0) {
            ttv_log(avctx, TTV_LOG_WARNING, "ignoring invalid SAR: %u/%u\n",
                   frame->sample_aspect_ratio.num,
                   frame->sample_aspect_ratio.den);
            frame->sample_aspect_ratio = ttv_make_q( 0, 1);
        }

        break;
    }
    return 0;
}

#if TT_API_GET_BUFFER


typedef struct CompatReleaseBufPriv {
    TTCodecContext avctx;
    TTFrame frame;
    uint8_t avframe_padding[1024]; // hack to allow linking to a avutil with larger TTFrame
} CompatReleaseBufPriv;

static void compat_free_buffer(void *opaque, uint8_t *data)
{
    CompatReleaseBufPriv *priv = opaque;
    if (priv->avctx.release_buffer)
        priv->avctx.release_buffer(&priv->avctx, &priv->frame);
    ttv_freep(&priv);
}

static void compat_release_buffer(void *opaque, uint8_t *data)
{
    TTBufferRef *buf = opaque;
    ttv_buffer_unref(&buf);
}
TT_ENABLE_DEPRECATION_WARNINGS
#endif

int tt_decode_frame_props(TTCodecContext *avctx, TTFrame *frame)
{
    return tt_init_buffer_info(avctx, frame);
}

static int get_buffer_internal(TTCodecContext *avctx, TTFrame *frame, int flags)
{
  //  const AVHWAccel *hwaccel = avctx->hwaccel;
    int override_dimensions = 1;
    int ret;

    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        if ((ret = ttv_image_check_size(avctx->width, avctx->height, 0, avctx)) < 0 || avctx->pix_fmt<0) {
            ttv_log(avctx, TTV_LOG_ERROR, "video_get_buffer: image parameters invalid\n");
            return AVERROR(EINVAL);
        }
    }
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (frame->width <= 0 || frame->height <= 0) {
            frame->width  = FFMAX(avctx->width,  TT_CEIL_RSHIFT(avctx->coded_width,  avctx->lowres));
            frame->height = FFMAX(avctx->height, TT_CEIL_RSHIFT(avctx->coded_height, avctx->lowres));
            override_dimensions = 0;
        }
    }
    ret = tt_decode_frame_props(avctx, frame);
    if (ret < 0)
        return ret;
    if ((ret = tt_init_buffer_info(avctx, frame)) < 0)
        return ret;

//     if (hwaccel && hwaccel->alloc_frame) {
//         ret = hwaccel->alloc_frame(avctx, frame);
//         goto end;
//     }

#if TT_API_GET_BUFFER
TT_DISABLE_DEPRECATION_WARNINGS
    /*
     * Wrap an old get_buffer()-allocated buffer in a bunch of AVBuffers.
     * We wrap each plane in its own AVBuffer. Each of those has a reference to
     * a dummy AVBuffer as its private data, unreffing it on free.
     * When all the planes are freed, the dummy buffer's free callback calls
     * release_buffer().
     */
    if (avctx->get_buffer) {
        CompatReleaseBufPriv *priv = NULL;
        TTBufferRef *dummy_buf = NULL;
        int planes, i, ret;

        if (flags & TTV_GET_BUFFER_FLAG_REF)
            frame->reference    = 1;

        ret = avctx->get_buffer(avctx, frame);
        if (ret < 0)
            return ret;

        /* return if the buffers are already set up
         * this would happen e.g. when a custom get_buffer() calls
         * ttcodec_default_get_buffer
         */
        if (frame->buf[0])
            goto end0;

        priv = ttv_mallocz(sizeof(*priv));
        if (!priv) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        priv->avctx = *avctx;
        priv->frame = *frame;

        dummy_buf = ttv_buffer_create(NULL, 0, compat_free_buffer, priv, 0);
        if (!dummy_buf) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }

#define WRAP_PLANE(ref_out, data, data_size)                            \
do {                                                                    \
    TTBufferRef *dummy_ref = ttv_buffer_ref(dummy_buf);                  \
    if (!dummy_ref) {                                                   \
        ret = AVERROR(ENOMEM);                                          \
        goto fail;                                                      \
    }                                                                   \
    ref_out = ttv_buffer_create(data, data_size, compat_release_buffer,  \
                               dummy_ref, 0);                           \
    if (!ref_out) {                                                     \
        ttv_frame_unref(frame);                                          \
        ret = AVERROR(ENOMEM);                                          \
        goto fail;                                                      \
    }                                                                   \
} while (0)

        if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(frame->format);

            planes = ttv_pix_fmt_count_planes(frame->format);
            /* workaround for AVHWAccel plane count of 0, buf[0] is used as
               check for allocated buffers: make libavcodec happy */
            if (desc && desc->flags & TTV_PIX_FMT_FLAG_HWACCEL)
                planes = 1;
            if (!desc || planes <= 0) {
                ret = AVERROR(EINVAL);
                goto fail;
            }

            for (i = 0; i < planes; i++) {
                int v_shift    = (i == 1 || i == 2) ? desc->log2_chroma_h : 0;
                int plane_size = (frame->height >> v_shift) * frame->linesize[i];

                WRAP_PLANE(frame->buf[i], frame->data[i], plane_size);
            }
        } else {
			assert(0);
        }

        ttv_buffer_unref(&dummy_buf);

end0:
        frame->width  = avctx->width;
        frame->height = avctx->height;

        return 0;

fail:
        avctx->release_buffer(avctx, frame);
        ttv_freep(&priv);
        ttv_buffer_unref(&dummy_buf);
        return ret;
    }
TT_ENABLE_DEPRECATION_WARNINGS
#endif

    ret = avctx->get_buffer2(avctx, frame, flags);

end:
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO && !override_dimensions) {
        frame->width  = avctx->width;
        frame->height = avctx->height;
    }

    return ret;
}

int tt_get_buffer(TTCodecContext *avctx, TTFrame *frame, int flags)
{
    int ret = get_buffer_internal(avctx, frame, flags);
    if (ret < 0)
        ttv_log(avctx, TTV_LOG_ERROR, "get_buffer() failed\n");
    return ret;
}


int ttcodec_default_execute(TTCodecContext *c, int (*func)(TTCodecContext *c2, void *arg2), void *arg, int *ret, int count, int size)
{
    int i;

    for (i = 0; i < count; i++) {
        int r = func(c, (char *)arg + i * size);
        if (ret)
            ret[i] = r;
    }
    return 0;
}

int ttcodec_default_execute2(TTCodecContext *c, int (*func)(TTCodecContext *c2, void *arg2, int jobnr, int threadnr), void *arg, int *ret, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        int r = func(c, arg, i, 0);
        if (ret)
            ret[i] = r;
    }
    return 0;
}



static int is_hwaccel_pix_fmt(enum TTPixelFormat pix_fmt)
{
    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(pix_fmt);
    return desc->flags & TTV_PIX_FMT_FLAG_HWACCEL;
}

enum TTPixelFormat ttcodec_default_get_format(struct TTCodecContext *s, const enum TTPixelFormat *fmt)
{
    while (*fmt != TTV_PIX_FMT_NONE && is_hwaccel_pix_fmt(*fmt))
        ++fmt;
    return fmt[0];
}

int tt_get_format(TTCodecContext *avctx, const enum TTPixelFormat *fmt)
{
    const AVPixFmtDescriptor *desc;
    enum TTPixelFormat *choices;
    enum TTPixelFormat ret;
    unsigned n = 0;

    while (fmt[n] != TTV_PIX_FMT_NONE)
        ++n;

    choices = ttv_malloc_array(n + 1, sizeof(*choices));
    if (!choices)
        return TTV_PIX_FMT_NONE;

    memcpy(choices, fmt, (n + 1) * sizeof(*choices));

    for (;;) {
//         if (avctx->hwaccel && avctx->hwaccel->uninit)
//             avctx->hwaccel->uninit(avctx);
//        ttv_freep(&avctx->internal->hwaccel_priv_data);
//        avctx->hwaccel = NULL;

        ret = avctx->get_format(avctx, choices);

        desc = ttv_pix_fmt_desc_get(ret);
        if (!desc) {
            ret = TTV_PIX_FMT_NONE;
            break;
        }

		break;
    }

    ttv_freep(&choices);
    return ret;
}


//MAKE_ACCESSORS(TTCodecContext, codec, TTRational, pkt_timebase)
//MAKE_ACCESSORS(TTCodecContext, codec, const TTCodecDescriptor *, codec_descriptor)
MAKE_ACCESSORS(TTCodecContext, codec, int, lowres)
MAKE_ACCESSORS(TTCodecContext, codec, int, seek_preroll)
MAKE_ACCESSORS(TTCodecContext, codec, uint16_t*, chroma_intra_matrix)


static int get_bit_rate(TTCodecContext *ctx)
{
    int bit_rate;
    //int bits_per_sample;

    switch (ctx->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
    case AVMEDIA_TYPE_DATA:
    case AVMEDIA_TYPE_ATTACHMENT:
        bit_rate = ctx->bit_rate;
        break;
    default:
        bit_rate = 0;
        break;
    }
    return bit_rate;
}


int attribute_align_arg ttcodec_open2(TTCodecContext *avctx, const TTCodec *codec, TTDictionary **options)
{
    int ret = 0;
    TTDictionary *tmp = NULL;

    if (ttcodec_is_open(avctx))
        return 0;

    if ((!codec && !avctx->codec)) {
        ttv_log(avctx, TTV_LOG_ERROR, "No codec provided to ttcodec_open2()\n");
        return AVERROR(EINVAL);
    }
    if ((codec && avctx->codec && codec != avctx->codec)) {
        ttv_log(avctx, TTV_LOG_ERROR, "This TTCodecContext was allocated for %s, "
                                    "but %s passed to ttcodec_open2()\n", avctx->codec->name, codec->name);
        return AVERROR(EINVAL);
    }
    if (!codec)
        codec = avctx->codec;

    if (avctx->extradata_size < 0 || avctx->extradata_size >= TT_MAX_EXTRADATA_SIZE)
        return AVERROR(EINVAL);

    if (options)
        ttv_dict_copy(&tmp, *options, 0);

    ret = tt_lock_avcodec(avctx);
    if (ret < 0)
        return ret;

    avctx->internal = ttv_mallocz(sizeof(TTCodecInternal));
    if (!avctx->internal) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avctx->internal->pool = ttv_mallocz(sizeof(*avctx->internal->pool));
    if (!avctx->internal->pool) {
        ret = AVERROR(ENOMEM);
        goto free_and_end;
    }

    avctx->internal->to_free = ttv_frame_alloc();
    if (!avctx->internal->to_free) {
        ret = AVERROR(ENOMEM);
        goto free_and_end;
    }

    if (codec->priv_data_size > 0) {
        if (!avctx->priv_data) {
            avctx->priv_data = ttv_mallocz(codec->priv_data_size);
            if (!avctx->priv_data) {
                ret = AVERROR(ENOMEM);
                goto end;
            }
            if (codec->priv_class) {
                *(const AVClass **)avctx->priv_data = codec->priv_class;
                ttv_opt_set_defaults(avctx->priv_data);
            }
        }
        if (codec->priv_class && (ret = ttv_opt_set_dict(avctx->priv_data, &tmp)) < 0)
            goto free_and_end;
    } else {
        avctx->priv_data = NULL;
    }
    if ((ret = ttv_opt_set_dict(avctx, &tmp)) < 0)
        goto free_and_end;

//     if (avctx->codec_whitelist && ttv_match_list(codec->name, avctx->codec_whitelist, ',') <= 0) {
//         ttv_log(avctx, TTV_LOG_ERROR, "Codec (%s) not on whitelist\n", codec->name);
//         ret = AVERROR(EINVAL);
//         goto free_and_end;
//     }

    // only call tt_set_dimensions() for non H.264/VP6F codecs so as not to overwrite previously setup dimensions
    if (!(avctx->coded_width && avctx->coded_height && avctx->width && avctx->height &&
          (avctx->codec_id == TTV_CODEC_ID_H264 || avctx->codec_id == TTV_CODEC_ID_VP6F))) {
    if (avctx->coded_width && avctx->coded_height)
        ret = tt_set_dimensions(avctx, avctx->coded_width, avctx->coded_height);
    else if (avctx->width && avctx->height)
        ret = tt_set_dimensions(avctx, avctx->width, avctx->height);
    if (ret < 0)
        goto free_and_end;
    }

    if ((avctx->coded_width || avctx->coded_height || avctx->width || avctx->height)
        && (  ttv_image_check_size(avctx->coded_width, avctx->coded_height, 0, avctx) < 0
           || ttv_image_check_size(avctx->width,       avctx->height,       0, avctx) < 0)) {
        ttv_log(avctx, TTV_LOG_WARNING, "Ignoring invalid width/height values\n");
        tt_set_dimensions(avctx, 0, 0);
    }

    if (avctx->width > 0 && avctx->height > 0) {
        if (ttv_image_check_sar(avctx->width, avctx->height,
                               avctx->sample_aspect_ratio) < 0) {
            ttv_log(avctx, TTV_LOG_WARNING, "ignoring invalid SAR: %u/%u\n",
                   avctx->sample_aspect_ratio.num,
                   avctx->sample_aspect_ratio.den);
            avctx->sample_aspect_ratio = ttv_make_q(0, 1 );
        }
    }

    /* if the decoder init function was already called previously,
     * free the already allocated subtitle_header before overwriting it */
    if (ttv_codec_is_decoder(codec))
        ttv_freep(&avctx->subtitle_header);

    avctx->codec = codec;
    if ((avctx->codec_type == AVMEDIA_TYPE_UNKNOWN || avctx->codec_type == codec->type) &&
        avctx->codec_id == TTV_CODEC_ID_NONE) {
        avctx->codec_type = codec->type;
        avctx->codec_id   = codec->id;
    }
    if (avctx->codec_id != codec->id || (avctx->codec_type != codec->type
                                         && avctx->codec_type != AVMEDIA_TYPE_ATTACHMENT)) {
        ttv_log(avctx, TTV_LOG_ERROR, "Codec type or id mismatches\n");
        ret = AVERROR(EINVAL);
        goto free_and_end;
    }
    avctx->frame_number = 0;
 //   avctx->codec_descriptor = ttcodec_descriptor_get(avctx->codec_id);

    if (avctx->codec->capabilities & CODEC_CAP_EXPERIMENTAL &&
        avctx->strict_std_compliance > TT_COMPLIANCE_EXPERIMENTAL) {
        const char *codec_string = "decoder";
        TTCodec *codec2;
        ttv_log(avctx, TTV_LOG_ERROR,
               "The %s '%s' is experimental but experimental codecs are not enabled, "
               "add '-strict %d' if you want to use it.\n",
               codec_string, codec->name, TT_COMPLIANCE_EXPERIMENTAL);
        codec2 = ttcodec_find_decoder(codec->id);
        if (!(codec2->capabilities & CODEC_CAP_EXPERIMENTAL))
            ttv_log(avctx, TTV_LOG_ERROR, "Alternatively use the non experimental %s '%s'.\n",
                codec_string, codec2->name);
        ret = TTERROR_EXPERIMENTAL;
        goto free_and_end;
    }

    if (!HAVE_THREADS)
        ttv_log(avctx, TTV_LOG_WARNING, "Warning: not compiled with thread support, using thread emulation\n");

    if (HAVE_THREADS
        && !(avctx->internal->frame_thread_encoder && (avctx->active_thread_type&TT_THREAD_FRAME))) {
        ret = tt_thread_init(avctx);
        if (ret < 0) {
            goto free_and_end;
        }
    }
    if (!HAVE_THREADS && !(codec->capabilities & CODEC_CAP_AUTO_THREADS))
        avctx->thread_count = 1;

    if (avctx->codec->max_lowres < avctx->lowres || avctx->lowres < 0) {
        ttv_log(avctx, TTV_LOG_ERROR, "The maximum value for lowres supported by the decoder is %d\n",
               avctx->codec->max_lowres);
        ret = AVERROR(EINVAL);
        goto free_and_end;
    }

#if TT_API_VISMV
    if (avctx->debug_mv)
        ttv_log(avctx, TTV_LOG_WARNING, "The 'vismv' option is deprecated, "
               "see the codecview filter instead.\n");
#endif

    avctx->pts_correction_num_faulty_pts =
    avctx->pts_correction_num_faulty_dts = 0;
    avctx->pts_correction_last_pts =
    avctx->pts_correction_last_dts = INT64_MIN;

    if (   avctx->codec->init && (!(avctx->active_thread_type&TT_THREAD_FRAME)
        || avctx->internal->frame_thread_encoder)) {
        ret = avctx->codec->init(avctx);
        if (ret < 0) {
            goto free_and_end;
        }
    }

    ret=0;
    if (ttv_codec_is_decoder(avctx->codec)) {
        if (!avctx->bit_rate)
            avctx->bit_rate = get_bit_rate(avctx);
        /* validate channel layout from the decoder */

#if TT_API_AVCTX_TIMEBASE
        if (avctx->framerate.num > 0 && avctx->framerate.den > 0)
            avctx->time_base = ttv_inv_q(ttv_mul_q(avctx->framerate, ttv_make_q(avctx->ticks_per_frame, 1)));
#endif
    }
end:
    tt_unlock_avcodec();
    if (options) {
        ttv_dict_free(options);
        *options = tmp;
    }

    return ret;
free_and_end:
    ttv_dict_free(&tmp);
    ttv_freep(&avctx->priv_data);
    if (avctx->internal) {
        ttv_frame_free(&avctx->internal->to_free);
        ttv_freep(&avctx->internal->pool);
    }
    ttv_freep(&avctx->internal);
    avctx->codec = NULL;
    goto end;
}


static int64_t guess_correct_pts(TTCodecContext *ctx,
                                 int64_t reordered_pts, int64_t dts)
{
    int64_t pts = TTV_NOPTS_VALUE;

    if (dts != TTV_NOPTS_VALUE) {
        ctx->pts_correction_num_faulty_dts += dts <= ctx->pts_correction_last_dts;
        ctx->pts_correction_last_dts = dts;
    } else if (reordered_pts != TTV_NOPTS_VALUE)
        ctx->pts_correction_last_dts = reordered_pts;

    if (reordered_pts != TTV_NOPTS_VALUE) {
        ctx->pts_correction_num_faulty_pts += reordered_pts <= ctx->pts_correction_last_pts;
        ctx->pts_correction_last_pts = reordered_pts;
    } else if(dts != TTV_NOPTS_VALUE)
        ctx->pts_correction_last_pts = dts;

    if ((ctx->pts_correction_num_faulty_pts<=ctx->pts_correction_num_faulty_dts || dts == TTV_NOPTS_VALUE)
       && reordered_pts != TTV_NOPTS_VALUE)
        pts = reordered_pts;
    else
        pts = dts;

    return pts;
}

#define DEF(type, name, bytes, read, write)                                  \
	static ttv_always_inline type bytestream_get_ ## name(const uint8_t **b)        \
{                                                                              \
	(*b) += bytes;                                                             \
	return read(*b - bytes);                                                   \
}                                                                              \
	static ttv_always_inline void bytestream_put_ ## name(uint8_t **b,              \
	const type value)         \
{                                                                              \
	write(*b, value);                                                          \
	(*b) += bytes;                                                             \
}                                                                              


DEF(uint64_t,     le64, 8, TTV_RL64, TTV_WL64)
DEF(unsigned int, le32, 4, TTV_RL32, TTV_WL32)

static int apply_param_change(TTCodecContext *avctx, TTPacket *avpkt)
{
    int size = 0, ret;
    const uint8_t *data;
    uint32_t flags;

    data = ttv_packet_get_side_data(avpkt, TTV_PKT_DATA_PARAM_CHANGE, &size);
    if (!data)
        return 0;

    if (!(avctx->codec->capabilities & CODEC_CAP_PARAM_CHANGE)) {
        ttv_log(avctx, TTV_LOG_ERROR, "This decoder does not support parameter "
               "changes, but PARAM_CHANGE side data was sent to it.\n");
        return AVERROR(EINVAL);
    }

    if (size < 4)
        goto fail;

    flags = bytestream_get_le32(&data);
    size -= 4;

    if (flags & TTV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT) {
        if (size < 4)
            goto fail;
        avctx->channels = bytestream_get_le32(&data);
        size -= 4;
    }
    if (flags & TTV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT) {
        if (size < 8)
            goto fail;
        avctx->channel_layout = bytestream_get_le64(&data);
        size -= 8;
    }
    if (flags & TTV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE) {
        if (size < 4)
            goto fail;
        avctx->sample_rate = bytestream_get_le32(&data);
        size -= 4;
    }
    if (flags & TTV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS) {
        if (size < 8)
            goto fail;
        avctx->width  = bytestream_get_le32(&data);
        avctx->height = bytestream_get_le32(&data);
        size -= 8;
        ret = tt_set_dimensions(avctx, avctx->width, avctx->height);
        if (ret < 0)
            return ret;
    }

    return 0;
fail:
    ttv_log(avctx, TTV_LOG_ERROR, "PARAM_CHANGE side data too small.\n");
    return TTERROR_INVALIDDATA;
}

static int add_metadata_from_side_data(TTCodecContext *avctx, TTFrame *frame)
{
    int size;
    const uint8_t *side_metadata;

    TTDictionary **frame_md = ttpriv_frame_get_metadatap(frame);

    side_metadata = ttv_packet_get_side_data(avctx->internal->pkt,
                                            TTV_PKT_DATA_STRINGS_METADATA, &size);
    return ttv_packet_unpack_dictionary(side_metadata, size, frame_md);
}

static int unrefcount_frame(TTCodecInternal *avci, TTFrame *frame)
{
    int ret;

    /* move the original frame to our backup */
    ttv_frame_unref(avci->to_free);
    ttv_frame_move_ref(avci->to_free, frame);

    /* now copy everything except the AVBufferRefs back
     * note that we make a COPY of the side data, so calling ttv_frame_free() on
     * the caller's frame will work properly */
    ret = ttv_frame_copy_props(frame, avci->to_free);
    if (ret < 0)
        return ret;

    memcpy(frame->data,     avci->to_free->data,     sizeof(frame->data));
    memcpy(frame->linesize, avci->to_free->linesize, sizeof(frame->linesize));
    if (avci->to_free->extended_data != avci->to_free->data) {
        int planes = ttv_frame_get_channels(avci->to_free);
        int size   = planes * sizeof(*frame->extended_data);

        if (!size) {
            ttv_frame_unref(frame);
            return TTERROR_BUG;
        }

        frame->extended_data = ttv_malloc(size);
        if (!frame->extended_data) {
            ttv_frame_unref(frame);
            return AVERROR(ENOMEM);
        }
        memcpy(frame->extended_data, avci->to_free->extended_data,
               size);
    } else
        frame->extended_data = frame->data;

    frame->format         = avci->to_free->format;
    frame->width          = avci->to_free->width;
    frame->height         = avci->to_free->height;
    frame->channel_layout = avci->to_free->channel_layout;
    frame->nb_samples     = avci->to_free->nb_samples;
    ttv_frame_set_channels(frame, ttv_frame_get_channels(avci->to_free));

    return 0;
}

int attribute_align_arg ttcodec_decode_video2(TTCodecContext *avctx, TTFrame *picture,
                                              int *got_picture_ptr,
                                              const TTPacket *avpkt)
{
    TTCodecInternal *avci = avctx->internal;
    int ret;
    // copy to ensure we do not change avpkt
    TTPacket tmp = *avpkt;

    if (!avctx->codec)
        return AVERROR(EINVAL);
    if (avctx->codec->type != AVMEDIA_TYPE_VIDEO) {
        ttv_log(avctx, TTV_LOG_ERROR, "Invalid media type for video\n");
        return AVERROR(EINVAL);
    }

    *got_picture_ptr = 0;
    if ((avctx->coded_width || avctx->coded_height) && ttv_image_check_size(avctx->coded_width, avctx->coded_height, 0, avctx))
        return AVERROR(EINVAL);

    ttv_frame_unref(picture);

    if ((avctx->codec->capabilities & CODEC_CAP_DELAY) || avpkt->size || (avctx->active_thread_type & TT_THREAD_FRAME)) {
        int did_split = ttv_packet_split_side_data(&tmp);
        ret = apply_param_change(avctx, &tmp);
        if (ret < 0) {
            ttv_log(avctx, TTV_LOG_ERROR, "Error applying parameter changes.\n");
            if (avctx->err_recognition & TTV_EF_EXPLODE)
                goto fail;
        }

        avctx->internal->pkt = &tmp;
        if (HAVE_THREADS && avctx->active_thread_type & TT_THREAD_FRAME)
            ret = tt_thread_decode_frame(avctx, picture, got_picture_ptr,
                                         &tmp);
        else {
            ret = avctx->codec->decode(avctx, picture, got_picture_ptr,
                                       &tmp);
            picture->pkt_dts = avpkt->dts;

            if(!avctx->has_b_frames){
                ttv_frame_set_pkt_pos(picture, avpkt->pos);
            }
            //FIXME these should be under if(!avctx->has_b_frames)
            /* get_buffer is supposed to set frame parameters */
            if (!(avctx->codec->capabilities & CODEC_CAP_DR1)) {
                if (!picture->sample_aspect_ratio.num)    picture->sample_aspect_ratio = avctx->sample_aspect_ratio;
                if (!picture->width)                      picture->width               = avctx->width;
                if (!picture->height)                     picture->height              = avctx->height;
                if (picture->format == TTV_PIX_FMT_NONE)   picture->format              = avctx->pix_fmt;
            }
        }
        add_metadata_from_side_data(avctx, picture);

fail:
        emms_c(); //needed to avoid an emms_c() call before every return;

        avctx->internal->pkt = NULL;
        if (did_split) {
            ttv_packet_free_side_data(&tmp);
            if(ret == tmp.size)
                ret = avpkt->size;
        }

        if (*got_picture_ptr) {
            if (!avctx->refcounted_frames) {
                int err = unrefcount_frame(avci, picture);
                if (err < 0)
                    return err;
            }

            avctx->frame_number++;
            ttv_frame_set_best_effort_timestamp(picture,
                                               guess_correct_pts(avctx,
                                                                 picture->pkt_pts,
                                                                 picture->pkt_dts));
        } else
            ttv_frame_unref(picture);
    } else
        ret = 0;

    /* many decoders assign whole TTFrames, thus overwriting extended_data;
     * make sure it's set correctly */
    ttv_assert0(!picture->extended_data || picture->extended_data == picture->data);

#if TT_API_AVCTX_TIMEBASE
    if (avctx->framerate.num > 0 && avctx->framerate.den > 0)
        avctx->time_base = ttv_inv_q(ttv_mul_q(avctx->framerate, ttv_make_q(avctx->ticks_per_frame, 1)));
#endif

    return ret;
}


ttv_cold int ttcodec_close(TTCodecContext *avctx)
{
    if (!avctx)
        return 0;

    if (ttcodec_is_open(avctx)) {
        FramePool *pool = avctx->internal->pool;
        int i;
#if CONFIG_FRAME_THREAD_ENCODER
        if (CONFIG_FRAME_THREAD_ENCODER &&
            avctx->internal->frame_thread_encoder && avctx->thread_count > 1) {
            tt_frame_thread_encoder_free(avctx);
        }
#endif
        if (HAVE_THREADS && avctx->internal->thread_ctx)
            tt_thread_free(avctx);
        if (avctx->codec && avctx->codec->close)
            avctx->codec->close(avctx);
        avctx->coded_frame = NULL;
        avctx->internal->byte_buffer_size = 0;
        ttv_freep(&avctx->internal->byte_buffer);
        ttv_frame_free(&avctx->internal->to_free);
        for (i = 0; i < TT_ARRAY_ELEMS(pool->pools); i++)
            ttv_buffer_pool_uninit(&pool->pools[i]);
        ttv_freep(&avctx->internal->pool);

//         if (avctx->hwaccel && avctx->hwaccel->uninit)
//             avctx->hwaccel->uninit(avctx);
//        ttv_freep(&avctx->internal->hwaccel_priv_data);

        ttv_freep(&avctx->internal);
    }

    if (avctx->priv_data && avctx->codec && avctx->codec->priv_class)
        ttv_opt_free(avctx->priv_data);
    ttv_opt_free(avctx);
    ttv_freep(&avctx->priv_data);
    avctx->codec = NULL;
    avctx->active_thread_type = 0;

    return 0;
}


TTCodec *ttcodec_find_decoder(enum TTCodecID id)
{
	TTCodec *p, *experimental = NULL;
	p = first_avcodec;
	while (p) {
		if (p->id == id) {
			if (p->capabilities & CODEC_CAP_EXPERIMENTAL && !experimental) {
				experimental = p;
			} else
				return p;
		}
		p = p->next;
	}
	return experimental;
}

void ttcodec_flush_buffers(TTCodecContext *avctx)
{
    if (HAVE_THREADS && avctx->active_thread_type & TT_THREAD_FRAME)
        tt_thread_flush(avctx);
    else if (avctx->codec->flush)
        avctx->codec->flush(avctx);

    avctx->pts_correction_last_pts =
    avctx->pts_correction_last_dts = INT64_MIN;

    if (!avctx->refcounted_frames)
        ttv_frame_unref(avctx->internal->to_free);
}


#if !HAVE_THREADS
int tt_thread_init(TTCodecContext *s)
{
    return -1;
}

#endif


int tt_lock_avcodec(TTCodecContext *log_ctx)
{
    if (lockmgr_cb) {
        if ((*lockmgr_cb)(&codec_mutex, TTV_LOCK_OBTAIN))
            return -1;
    }
    entangled_thread_counter++;
    if (entangled_thread_counter != 1) {
        ttv_log(log_ctx, TTV_LOG_ERROR, "Insufficient thread locking around ttcodec_open/close()\n");
        if (!lockmgr_cb)
            ttv_log(log_ctx, TTV_LOG_ERROR, "No lock manager is set, please see ttv_lockmgr_register()\n");
        tt_ttcodec_locked = 1;
        tt_unlock_avcodec();
        return AVERROR(EINVAL);
    }
    ttv_assert0(!tt_ttcodec_locked);
    tt_ttcodec_locked = 1;
    return 0;
}

int tt_unlock_avcodec(void)
{
    ttv_assert0(tt_ttcodec_locked);
    tt_ttcodec_locked = 0;
    entangled_thread_counter--;
    if (lockmgr_cb) {
        if ((*lockmgr_cb)(&codec_mutex, TTV_LOCK_RELEASE))
            return -1;
    }

    return 0;
}

static inline ttv_const int ttv_toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		c ^= 0x20;
	return c;
}



int tt_thread_ref_frame(ThreadFrame *dst, ThreadFrame *src)
{
    int ret;

    dst->owner = src->owner;

    ret = ttv_frame_ref(dst->f, src->f);
    if (ret < 0)
        return ret;

    if (src->progress &&
        !(dst->progress = ttv_buffer_ref(src->progress))) {
        tt_thread_release_buffer(dst->owner, dst);
        return AVERROR(ENOMEM);
    }

    return 0;
}

#if !HAVE_THREADS

enum TTPixelFormat tt_thread_get_format(TTCodecContext *avctx, const enum TTPixelFormat *fmt)
{
    return tt_get_format(avctx, fmt);
}

int tt_thread_get_buffer(TTCodecContext *avctx, ThreadFrame *f, int flags)
{
    f->owner = avctx;
    return tt_get_buffer(avctx, f->f, flags);
}

void tt_thread_release_buffer(TTCodecContext *avctx, ThreadFrame *f)
{
    if (f->f)
        ttv_frame_unref(f->f);
}

void tt_thread_finish_setup(TTCodecContext *avctx)
{
}

void tt_thread_report_progress(ThreadFrame *f, int progress, int field)
{
}

void tt_thread_await_progress(ThreadFrame *f, int progress, int field)
{
}

int tt_thread_can_start_frame(TTCodecContext *avctx)
{
    return 1;
}

int tt_alloc_entries(TTCodecContext *avctx, int count)
{
    return 0;
}

void tt_reset_entries(TTCodecContext *avctx)
{
}

void tt_thread_await_progress2(TTCodecContext *avctx, int field, int thread, int shift)
{
}

void tt_thread_report_progress2(TTCodecContext *avctx, int field, int thread, int n)
{
}

#endif



int ttcodec_is_open(TTCodecContext *s)
{
    return !!s->internal;
}



const uint8_t *ttpriv_find_start_code(const uint8_t *ttv_restrict p,
                                      const uint8_t *end,
                                      uint32_t *ttv_restrict state)
{
    int i;

    ttv_assert0(p <= end);
    if (p >= end)
        return end;

    for (i = 0; i < 3; i++) {
        uint32_t tmp = *state << 8;
        *state = tmp + *(p++);
        if (tmp == 0x100 || p == end)
            return p;
    }

    while (p < end) {
        if      (p[-1] > 1      ) p += 3;
        else if (p[-2]          ) p += 2;
        else if (p[-3]|(p[-1]-1)) p++;
        else {
            p++;
            break;
        }
    }

    p = FFMIN(p, end) - 4;
    *state = TTV_RB32(p);

    return p + 4;
}
