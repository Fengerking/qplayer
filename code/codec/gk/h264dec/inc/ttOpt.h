#ifndef __TTPOD_TT_OPT_H_
#define __TTPOD_TT_OPT_H_

/**
 * @file
 * AVOptions
 */

#include "ttRational.h"
#include "ttAvutil.h"
#include "ttDict.h"
#include "ttLog.h"
#include "ttPixfmt.h"


enum AVOptionType{
    TTV_OPT_TYPE_FLAGS,
    TTV_OPT_TYPE_INT,
    TTV_OPT_TYPE_INT64,
    TTV_OPT_TYPE_DOUBLE,
    TTV_OPT_TYPE_FLOAT,
    TTV_OPT_TYPE_STRING,
    TTV_OPT_TYPE_RATIONAL,
    TTV_OPT_TYPE_BINARY,  ///< offset must point to a pointer immediately followed by an int for the length
    TTV_OPT_TYPE_DICT,
    TTV_OPT_TYPE_CONST = 128,
    TTV_OPT_TYPE_IMAGE_SIZE = MKBETAG('S','I','Z','E'), ///< offset must point to two consecutive integers
    TTV_OPT_TYPE_PIXEL_FMT  = MKBETAG('P','F','M','T'),
    TTV_OPT_TYPE_SAMPLE_FMT = MKBETAG('S','F','M','T'),
    TTV_OPT_TYPE_VIDEO_RATE = MKBETAG('V','R','A','T'), ///< offset must point to TTRational
    TTV_OPT_TYPE_DURATION   = MKBETAG('D','U','R',' '),
    TTV_OPT_TYPE_COLOR      = MKBETAG('C','O','L','R'),
    TTV_OPT_TYPE_CHANNEL_LAYOUT = MKBETAG('C','H','L','A'),
#if TT_API_OLD_AVOPTIONS
    TT_OPT_TYPE_FLAGS = 0,
    TT_OPT_TYPE_INT,
    TT_OPT_TYPE_INT64,
    TT_OPT_TYPE_DOUBLE,
    TT_OPT_TYPE_FLOAT,
    TT_OPT_TYPE_STRING,
    TT_OPT_TYPE_RATIONAL,
    TT_OPT_TYPE_BINARY,  ///< offset must point to a pointer immediately followed by an int for the length
    TT_OPT_TYPE_CONST=128,
#endif
};

/**
 * AVOption
 */
typedef struct AVOption {
    const char *name;

    const char *help;

    int offset;
    enum AVOptionType type;

    union {
        int64_t i64;
        double dbl;
        const char *str;
        /* TODO those are unused now */
        TTRational q;
    } default_val;
    double min;                 ///< minimum valid value for the option
    double max;                 ///< maximum valid value for the option

    int flags;
#define TTV_OPT_FLAG_ENCODING_PARAM  1   ///< a generic parameter which can be set by the user for muxing or encoding
#define TTV_OPT_FLAG_DECODING_PARAM  2   ///< a generic parameter which can be set by the user for demuxing or decoding
#if TT_API_OPT_TYPE_METADATA
#define TTV_OPT_FLAG_METADATA        4   ///< some data extracted or inserted into the file like title, comment, ...
#endif
#define TTV_OPT_FLAG_AUDIO_PARAM     8
#define TTV_OPT_FLAG_VIDEO_PARAM     16
#define TTV_OPT_FLAG_SUBTITLE_PARAM  32
#define TTV_OPT_FLAG_EXPORT          64
#define TTV_OPT_FLAG_READONLY        128
#define TTV_OPT_FLAG_FILTERING_PARAM (1<<16) ///< a generic parameter which can be set by the user for filtering
    const char *unit;
} AVOption;

/**
 * A single allowed range of values, or a single allowed value.
 */
typedef struct AVOptionRange {
    const char *str;
    double value_min, value_max;
    double component_min, component_max;
    int is_range;
} AVOptionRange;

/**
 * List of AVOptionRange structs.
 */
typedef struct AVOptionRanges {
    AVOptionRange **range;
    int nb_ranges;
    int nb_components;
} AVOptionRanges;


void ttv_opt_set_defaults(void *s);

#if TT_API_OLD_AVOPTIONS
attribute_deprecated
void ttv_opt_set_defaults2(void *s, int mask, int flags);
#endif

void ttv_opt_free(void *obj);
int ttv_opt_set_dict(void *obj, struct TTDictionary **options);
int ttv_opt_set_dict2(void *obj, struct TTDictionary **options, int search_flags);

enum {
    TTV_OPT_FLAG_IMPLICIT_KEY = 1,
};

int ttv_opt_eval_flags (void *obj, const AVOption *o, const char *val, int        *flags_out);
int ttv_opt_eval_int   (void *obj, const AVOption *o, const char *val, int        *int_out);
int ttv_opt_eval_int64 (void *obj, const AVOption *o, const char *val, int64_t    *int64_out);
int ttv_opt_eval_float (void *obj, const AVOption *o, const char *val, float      *float_out);
int ttv_opt_eval_double(void *obj, const AVOption *o, const char *val, double     *double_out);
int ttv_opt_eval_q     (void *obj, const AVOption *o, const char *val, TTRational *q_out);

#define TTV_OPT_SEARCH_CHILDREN   0x0001 
#define TTV_OPT_SEARCH_FAKE_OBJ   0x0002
#define TTV_OPT_MULTI_COMPONENT_RANGE 0x1000


const AVOption *ttv_opt_next(void *obj, const AVOption *prev);

#endif /* __TTPOD_TT_OPT_H_ */
