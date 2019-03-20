#include "ttAvassert.h"
#include "ttBuffer.h"
#include "ttCommon.h"
#include "ttDict.h"
#include "ttFrame.h"
#include "ttImgutils.h"
#include "ttMem.h"
#include "ttInternal.h"

MAKE_ACCESSORS(TTFrame, frame, int64_t, best_effort_timestamp)
MAKE_ACCESSORS(TTFrame, frame, int64_t, pkt_duration)
MAKE_ACCESSORS(TTFrame, frame, int64_t, pkt_pos)
MAKE_ACCESSORS(TTFrame, frame, int64_t, channel_layout)
MAKE_ACCESSORS(TTFrame, frame, int,     channels)
MAKE_ACCESSORS(TTFrame, frame, int,     sample_rate)
MAKE_ACCESSORS(TTFrame, frame, TTDictionary *, metadata)
MAKE_ACCESSORS(TTFrame, frame, int,     decode_error_flags)
MAKE_ACCESSORS(TTFrame, frame, int,     pkt_size)
MAKE_ACCESSORS(TTFrame, frame, enum TTColorSpace, colorspace)
MAKE_ACCESSORS(TTFrame, frame, enum TTColorRange, color_range)

TTDictionary **ttpriv_frame_get_metadatap(TTFrame *frame) {return &frame->metadata;};

int ttv_frame_set_qp_table(TTFrame *f, TTBufferRef *buf, int stride, int qp_type)
{
    ttv_buffer_unref(&f->qp_table_buf);

    f->qp_table_buf = buf;

    f->qscale_table = buf->data;
    f->qstride      = stride;
    f->qscale_type  = qp_type;

    return 0;
}

static void get_frame_defaults(TTFrame *frame)
{
    if (frame->extended_data != frame->data)
        ttv_freep(&frame->extended_data);

    memset(frame, 0, sizeof(*frame));

    frame->pts                   =
    frame->pkt_dts               =
    frame->pkt_pts               = TTV_NOPTS_VALUE;
    ttv_frame_set_best_effort_timestamp(frame, TTV_NOPTS_VALUE);
    ttv_frame_set_pkt_duration         (frame, 0);
    ttv_frame_set_pkt_pos              (frame, -1);
    ttv_frame_set_pkt_size             (frame, -1);
    frame->key_frame           = 1;
    frame->sample_aspect_ratio = ttv_make_q(0, 1);
    frame->format              = -1; /* unknown */
    frame->extended_data       = frame->data;
    frame->color_primaries     = AVCOL_PRI_UNSPECIFIED;
    frame->color_trc           = AVCOL_TRC_UNSPECIFIED;
    frame->colorspace          = AVCOL_SPC_UNSPECIFIED;
    frame->color_range         = AVCOL_RANGE_UNSPECIFIED;
    frame->chroma_location     = AVCHROMA_LOC_UNSPECIFIED;
}

static void free_side_data(TTFrameSideData **ptr_sd)
{
    TTFrameSideData *sd = *ptr_sd;

    ttv_freep(&sd->data);
    ttv_dict_free(&sd->metadata);
    ttv_freep(ptr_sd);
}

TTFrame *ttv_frame_alloc(void)
{
    TTFrame *frame = ttv_mallocz(sizeof(*frame));

    if (!frame)
        return NULL;

    frame->extended_data = NULL;
    get_frame_defaults(frame);

    return frame;
}

void ttv_frame_free(TTFrame **frame)
{
    if (!frame || !*frame)
        return;

    ttv_frame_unref(*frame);
    ttv_freep(frame);
}

static int get_video_buffer(TTFrame *frame, int align)
{
    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(frame->format);
    int ret, i;

    if (!desc)
        return AVERROR(EINVAL);

    if ((ret = ttv_image_check_size(frame->width, frame->height, 0, NULL)) < 0)
        return ret;

    if (!frame->linesize[0]) {
        for(i=1; i<=align; i+=i) {
            ret = ttv_image_fill_linesizes(frame->linesize, frame->format,
                                          FFALIGN(frame->width, i));
            if (ret < 0)
                return ret;
            if (!(frame->linesize[0] & (align-1)))
                break;
        }

        for (i = 0; i < 4 && frame->linesize[i]; i++)
            frame->linesize[i] = FFALIGN(frame->linesize[i], align);
    }

    for (i = 0; i < 4 && frame->linesize[i]; i++) {
        int h = FFALIGN(frame->height, 32);
        if (i == 1 || i == 2)
            h = TT_CEIL_RSHIFT(h, desc->log2_chroma_h);

        frame->buf[i] = ttv_buffer_alloc(frame->linesize[i] * h + 16 + 16/*STRIDE_ALIGN*/ - 1);
        if (!frame->buf[i])
            goto fail;

        frame->data[i] = frame->buf[i]->data;
    }
    if (desc->flags & TTV_PIX_FMT_FLAG_PAL || desc->flags & TTV_PIX_FMT_FLAG_PSEUDOPAL) {
        ttv_buffer_unref(&frame->buf[1]);
        frame->buf[1] = ttv_buffer_alloc(1024);
        if (!frame->buf[1])
            goto fail;
        frame->data[1] = frame->buf[1]->data;
    }

    frame->extended_data = frame->data;

    return 0;
fail:
    ttv_frame_unref(frame);
    return AVERROR(ENOMEM);
}


int ttv_frame_get_buffer(TTFrame *frame, int align)
{
    if (frame->format < 0)
        return AVERROR(EINVAL);

    if (frame->width > 0 && frame->height > 0)
        return get_video_buffer(frame, align);

    return AVERROR(EINVAL);
}

int ttv_frame_ref(TTFrame *dst, const TTFrame *src)
{
    int i, ret = 0;

    dst->format         = src->format;
    dst->width          = src->width;
    dst->height         = src->height;
    dst->channels       = src->channels;
    dst->channel_layout = src->channel_layout;
    dst->nb_samples     = src->nb_samples;

    ret = ttv_frame_copy_props(dst, src);
    if (ret < 0)
        return ret;

    /* duplicate the frame data if it's not refcounted */
    if (!src->buf[0]) {
        ret = ttv_frame_get_buffer(dst, 32);
        if (ret < 0)
            return ret;

        ret = ttv_frame_copy(dst, src);
        if (ret < 0)
            ttv_frame_unref(dst);

        return ret;
    }

    /* ref the buffers */
    for (i = 0; i < TT_ARRAY_ELEMS(src->buf); i++) {
        if (!src->buf[i])
            continue;
        dst->buf[i] = ttv_buffer_ref(src->buf[i]);
        if (!dst->buf[i]) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
    }

    if (src->extended_buf) {
        dst->extended_buf = ttv_mallocz_array(sizeof(*dst->extended_buf),
                                       src->nb_extended_buf);
        if (!dst->extended_buf) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        dst->nb_extended_buf = src->nb_extended_buf;

        for (i = 0; i < src->nb_extended_buf; i++) {
            dst->extended_buf[i] = ttv_buffer_ref(src->extended_buf[i]);
            if (!dst->extended_buf[i]) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
        }
    }

    /* duplicate extended data */
    if (src->extended_data != src->data) {
        int ch = src->channels;

        if (!ch) {
            ret = AVERROR(EINVAL);
            goto fail;
        }
        //CHECK_CHANNELS_CONSISTENCY(src);

        dst->extended_data = ttv_malloc_array(sizeof(*dst->extended_data), ch);
        if (!dst->extended_data) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        memcpy(dst->extended_data, src->extended_data, sizeof(*src->extended_data) * ch);
    } else
        dst->extended_data = dst->data;

    memcpy(dst->data,     src->data,     sizeof(src->data));
    memcpy(dst->linesize, src->linesize, sizeof(src->linesize));

    return 0;

fail:
    ttv_frame_unref(dst);
    return ret;
}

void ttv_frame_unref(TTFrame *frame)
{
    int i;

    for (i = 0; i < frame->nb_side_data; i++) {
        free_side_data(&frame->side_data[i]);
    }
    ttv_freep(&frame->side_data);

    for (i = 0; i < TT_ARRAY_ELEMS(frame->buf); i++)
        ttv_buffer_unref(&frame->buf[i]);
    for (i = 0; i < frame->nb_extended_buf; i++)
        ttv_buffer_unref(&frame->extended_buf[i]);
    ttv_freep(&frame->extended_buf);
    ttv_dict_free(&frame->metadata);
    ttv_buffer_unref(&frame->qp_table_buf);

    get_frame_defaults(frame);
}

void ttv_frame_move_ref(TTFrame *dst, TTFrame *src)
{
    *dst = *src;
    if (src->extended_data == src->data)
        dst->extended_data = dst->data;
    memset(src, 0, sizeof(*src));
    get_frame_defaults(src);
}

int ttv_frame_is_writable(TTFrame *frame)
{
    int i, ret = 1;

    /* assume non-refcounted frames are not writable */
    if (!frame->buf[0])
        return 0;

    for (i = 0; i < TT_ARRAY_ELEMS(frame->buf); i++)
        if (frame->buf[i])
            ret &= !!ttv_buffer_is_writable(frame->buf[i]);
    for (i = 0; i < frame->nb_extended_buf; i++)
        ret &= !!ttv_buffer_is_writable(frame->extended_buf[i]);

    return ret;
}

int ttv_frame_make_writable(TTFrame *frame)
{
    TTFrame tmp;
    int ret;

    if (!frame->buf[0])
        return AVERROR(EINVAL);

    if (ttv_frame_is_writable(frame))
        return 0;

    memset(&tmp, 0, sizeof(tmp));
    tmp.format         = frame->format;
    tmp.width          = frame->width;
    tmp.height         = frame->height;
    tmp.channels       = frame->channels;
    tmp.channel_layout = frame->channel_layout;
    tmp.nb_samples     = frame->nb_samples;
    ret = ttv_frame_get_buffer(&tmp, 32);
    if (ret < 0)
        return ret;

    ret = ttv_frame_copy(&tmp, frame);
    if (ret < 0) {
        ttv_frame_unref(&tmp);
        return ret;
    }

    ret = ttv_frame_copy_props(&tmp, frame);
    if (ret < 0) {
        ttv_frame_unref(&tmp);
        return ret;
    }

    ttv_frame_unref(frame);

    *frame = tmp;
    if (tmp.data == tmp.extended_data)
        frame->extended_data = frame->data;

    return 0;
}

int ttv_frame_copy_props(TTFrame *dst, const TTFrame *src)
{
    int i;

    dst->key_frame              = src->key_frame;
    dst->pict_type              = src->pict_type;
    dst->sample_aspect_ratio    = src->sample_aspect_ratio;
    dst->pts                    = src->pts;
    dst->repeat_pict            = src->repeat_pict;
    dst->interlaced_frame       = src->interlaced_frame;
    dst->top_field_first        = src->top_field_first;
    dst->palette_has_changed    = src->palette_has_changed;
    dst->sample_rate            = src->sample_rate;
    dst->opaque                 = src->opaque;
#if TT_API_AVFRAME_LAVC
    dst->type                   = src->type;
#endif
    dst->pkt_pts                = src->pkt_pts;
    dst->pkt_dts                = src->pkt_dts;
    dst->pkt_pos                = src->pkt_pos;
    dst->pkt_size               = src->pkt_size;
    dst->pkt_duration           = src->pkt_duration;
    dst->reordered_opaque       = src->reordered_opaque;
    dst->quality                = src->quality;
    dst->best_effort_timestamp  = src->best_effort_timestamp;
    dst->coded_picture_number   = src->coded_picture_number;
    dst->display_picture_number = src->display_picture_number;
    dst->flags                  = src->flags;
    dst->decode_error_flags     = src->decode_error_flags;
    dst->color_primaries        = src->color_primaries;
    dst->color_trc              = src->color_trc;
    dst->colorspace             = src->colorspace;
    dst->color_range            = src->color_range;
    dst->chroma_location        = src->chroma_location;

    ttv_dict_copy(&dst->metadata, src->metadata, 0);

    memcpy(dst->error, src->error, sizeof(dst->error));

    for (i = 0; i < src->nb_side_data; i++) {
        const TTFrameSideData *sd_src = src->side_data[i];
        TTFrameSideData *sd_dst;
        if (   sd_src->type == TTV_FRAME_DATA_PANSCAN
            && (src->width != dst->width || src->height != dst->height))
            continue;
        sd_dst = ttv_frame_new_side_data(dst, sd_src->type,
                                                         sd_src->size);
        if (!sd_dst) {
            for (i = 0; i < dst->nb_side_data; i++) {
                free_side_data(&dst->side_data[i]);
            }
            ttv_freep(&dst->side_data);
            return AVERROR(ENOMEM);
        }
        memcpy(sd_dst->data, sd_src->data, sd_src->size);
        ttv_dict_copy(&sd_dst->metadata, sd_src->metadata, 0);
    }

    dst->qscale_table = NULL;
    dst->qstride      = 0;
    dst->qscale_type  = 0;
    if (src->qp_table_buf) {
        dst->qp_table_buf = ttv_buffer_ref(src->qp_table_buf);
        if (dst->qp_table_buf) {
            dst->qscale_table = dst->qp_table_buf->data;
            dst->qstride      = src->qstride;
            dst->qscale_type  = src->qscale_type;
        }
    }

    return 0;
}



TTFrameSideData *ttv_frame_new_side_data(TTFrame *frame,
                                        enum TTFrameSideDataType type,
                                        int size)
{
    TTFrameSideData *ret, **tmp;

    if (frame->nb_side_data > INT_MAX / sizeof(*frame->side_data) - 1)
        return NULL;

    tmp = ttv_realloc(frame->side_data,
                     (frame->nb_side_data + 1) * sizeof(*frame->side_data));
    if (!tmp)
        return NULL;
    frame->side_data = tmp;

    ret = ttv_mallocz(sizeof(*ret));
    if (!ret)
        return NULL;

    ret->data = ttv_malloc(size);
    if (!ret->data) {
        ttv_freep(&ret);
        return NULL;
    }

    ret->size = size;
    ret->type = type;

    frame->side_data[frame->nb_side_data++] = ret;

    return ret;
}



static int frame_copy_video(TTFrame *dst, const TTFrame *src)
{
    const uint8_t *src_data[4];
    int i, planes;

    if (dst->width  < src->width ||
        dst->height < src->height)
        return AVERROR(EINVAL);

    planes = ttv_pix_fmt_count_planes(dst->format);
    for (i = 0; i < planes; i++)
        if (!dst->data[i] || !src->data[i])
            return AVERROR(EINVAL);

    memcpy(src_data, src->data, sizeof(src_data));
    ttv_image_copy(dst->data, dst->linesize,
                  src_data, src->linesize,
                  dst->format, src->width, src->height);

    return 0;
}

int ttv_frame_copy(TTFrame *dst, const TTFrame *src)
{
    if (dst->format != src->format || dst->format < 0)
        return AVERROR(EINVAL);

    if (dst->width > 0 && dst->height > 0)
        return frame_copy_video(dst, src);

    return AVERROR(EINVAL);
}


