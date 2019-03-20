#ifndef __TTPOD_TT_LOG_H_
#define __TTPOD_TT_LOG_H_

#include <stdarg.h>
#include "config.h"
#include "ttAvutil.h"
#include "ttAttributes.h"

typedef enum {
    TTV_CLASS_CATEGORY_NA = 0,
    TTV_CLASS_CATEGORY_INPUT,
    TTV_CLASS_CATEGORY_OUTPUT,
    TTV_CLASS_CATEGORY_MUXER,
    TTV_CLASS_CATEGORY_DEMUXER,
    TTV_CLASS_CATEGORY_ENCODER,
    TTV_CLASS_CATEGORY_DECODER,
    TTV_CLASS_CATEGORY_FILTER,
    TTV_CLASS_CATEGORY_BITSTREAM_FILTER,
    TTV_CLASS_CATEGORY_SWSCALER,
    TTV_CLASS_CATEGORY_SWRESAMPLER,
    TTV_CLASS_CATEGORY_DEVICE_VIDEO_OUTPUT = 40,
    TTV_CLASS_CATEGORY_DEVICE_VIDEO_INPUT,
    TTV_CLASS_CATEGORY_DEVICE_AUDIO_OUTPUT,
    TTV_CLASS_CATEGORY_DEVICE_AUDIO_INPUT,
    TTV_CLASS_CATEGORY_DEVICE_OUTPUT,
    TTV_CLASS_CATEGORY_DEVICE_INPUT,
    TTV_CLASS_CATEGORY_NB, ///< not part of ABI/API
}AVClassCategory;


struct AVOptionRanges;

typedef struct AVClass {
    const char* class_name;
    const char* (*item_name)(void* ctx);
    const struct AVOption *option;
    int version;
    int log_level_offset_offset;
    int parent_log_context_offset;
    void* (*child_next)(void *obj, void *prev);
    const struct AVClass* (*child_class_next)(const struct AVClass *prev);
    AVClassCategory category;
    AVClassCategory (*get_category)(void* ctx);
    int (*query_ranges)(struct AVOptionRanges **, void *obj, const char *key, int flags);
} AVClass;

#define TTV_LOG_QUIET    -8

/**
 * Something went wrong and recovery is not possible.
 * For example, no header was found for a format which depends
 * on headers or an illegal combination of parameters is used.
 */
#define TTV_LOG_FATAL     8

/**
 * Something went wrong and cannot losslessly be recovered.
 * However, not all future data is affected.
 */
#define TTV_LOG_ERROR    16

/**
 * Something somehow does not look correct. This may or may not
 * lead to problems. An example would be the use of '-vstrict -2'.
 */
#define TTV_LOG_WARNING  24

/**
 * Standard information.
 */
#define TTV_LOG_INFO     32

/**
 * Detailed information.
 */
#define TTV_LOG_VERBOSE  40

/**
 * Stuff which is only useful for libav* developers.
 */
#define TTV_LOG_DEBUG    48

#define TTV_LOG_MAX_OFFSET (TTV_LOG_DEBUG - TTV_LOG_QUIET)


static inline void ttv_log(void* avcl, int level, const char *fmt, ...){}

const char* ttv_default_item_name(void* ctx);

#ifdef DEBUG
#    define ttv_dlog(pctx, ...) ttv_log(pctx, TTV_LOG_DEBUG, __VA_ARGS__)
#else
#    define ttv_dlog(pctx, ...) do { if (0) ttv_log(pctx, TTV_LOG_DEBUG, __VA_ARGS__); } while (0)
#endif

#endif /* __TTPOD_TT_LOG_H_ */
