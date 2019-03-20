#include "ttAvcodec.h"
#include "ttInternal.h"
#include "ttAvassert.h"
#include "ttInternal.h"
#include "ttMem.h"
#include "ttOpt.h"
#include <float.h>              /* FLT_MIN, FLT_MAX */
#include <string.h>

#include "ttOptionsTable.h"

static const char* context_to_name(void* ptr) {
    TTCodecContext *avc= ptr;

    if(avc && avc->codec && avc->codec->name)
        return avc->codec->name;
    else
        return "NULL";
}

static void *codec_child_next(void *obj, void *prev)
{
    TTCodecContext *s = obj;
    if (!prev && s->codec && s->codec->priv_class && s->priv_data)
        return s->priv_data;
    return NULL;
}

static const AVClass *codec_child_class_next(const AVClass *prev)
{
    TTCodec *c = NULL;

    /* find the codec that corresponds to prev */
    while (prev && (c = ttv_codec_next(c)))
        if (c->priv_class == prev)
            break;

    /* find next codec with priv options */
    while (c = ttv_codec_next(c))
        if (c->priv_class)
            return c->priv_class;
    return NULL;
}

static AVClassCategory get_category(void *ptr)
{
    TTCodecContext* avctx = ptr;
    if(avctx->codec && avctx->codec->decode) return TTV_CLASS_CATEGORY_DECODER;
    else                                     return TTV_CLASS_CATEGORY_ENCODER;
}

// typedef struct AVClass {
//     const char* class_name;
//     const char* (*item_name)(void* ctx);
//     const struct AVOption *option;
//     int version;
//     int log_level_offset_offset;
//     int parent_log_context_offset;
//     void* (*child_next)(void *obj, void *prev);
//     const struct AVClass* (*child_class_next)(const struct AVClass *prev);
//     AVClassCategory category;
//     AVClassCategory (*get_category)(void* ctx);
//     int (*query_ranges)(struct AVOptionRanges **, void *obj, const char *key, int flags);
// } AVClass;

static const AVClass ttv_codec_context_class = {
    "TTCodecContext",
    context_to_name,
    ttcodec_options,
    LIB__TTPOD_TT_VERSION_INT,
    offsetof(TTCodecContext, log_level_offset),
	0,
    codec_child_next,
    codec_child_class_next,
    TTV_CLASS_CATEGORY_ENCODER,
    get_category,
};

int ttcodec_get_context_defaults3(TTCodecContext *s, const TTCodec *codec)
{
    int flags=0;
    memset(s, 0, sizeof(TTCodecContext));

    s->ttv_class = &ttv_codec_context_class;

    s->codec_type = codec ? codec->type : AVMEDIA_TYPE_UNKNOWN;
    if (codec) {
        s->codec = codec;
        s->codec_id = codec->id;
    }

    if(s->codec_type == AVMEDIA_TYPE_AUDIO)
        flags= TTV_OPT_FLAG_AUDIO_PARAM;
    else if(s->codec_type == AVMEDIA_TYPE_VIDEO)
        flags= TTV_OPT_FLAG_VIDEO_PARAM;
    else if(s->codec_type == AVMEDIA_TYPE_SUBTITLE)
        flags= TTV_OPT_FLAG_SUBTITLE_PARAM;
    ttv_opt_set_defaults2(s, flags, flags);

    s->time_base           = ttv_make_q(0,1);
    s->framerate           = ttv_make_q(0, 1);
    s->pkt_timebase        = ttv_make_q(0, 1);
    s->get_buffer2         = ttcodec_default_get_buffer2;
    s->get_format          = ttcodec_default_get_format;
    s->execute             = ttcodec_default_execute;
    s->execute2            = ttcodec_default_execute2;
    s->sample_aspect_ratio = ttv_make_q(0,1);
    s->pix_fmt             = TTV_PIX_FMT_NONE;
//    s->sample_fmt          = TTV_SAMPLE_FMT_NONE;

    s->reordered_opaque    = TTV_NOPTS_VALUE;
    if(codec && codec->priv_data_size){
        if(!s->priv_data){
            s->priv_data= ttv_mallocz(codec->priv_data_size);
            if (!s->priv_data) {
                return AVERROR(ENOMEM);
            }
        }
        if(codec->priv_class){
            *(const AVClass**)s->priv_data = codec->priv_class;
            ttv_opt_set_defaults(s->priv_data);
        }
    }
    return 0;
}

TTCodecContext *ttcodec_alloc_context3(const TTCodec *codec)
{
    TTCodecContext *avctx= ttv_malloc(sizeof(TTCodecContext));

    if (!avctx)
        return NULL;

    if(ttcodec_get_context_defaults3(avctx, codec) < 0){
        ttv_free(avctx);
        return NULL;
    }

    return avctx;
}

