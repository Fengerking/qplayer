#include "ttAvutil.h"
#include "ttCommon.h"
#include "ttOpt.h"
#include "ttDict.h"
#include "ttLog.h"
#include "ttPixdesc.h"
#include "ttMathematics.h"
#include "ttLibm.h"
#include <float.h>
#include "ttH264Log.h"

const AVOption *ttv_opt_next(void *obj, const AVOption *last)
{
    AVClass *class = *(AVClass**)obj;
    if (!last && class && class->option && class->option[0].name)
        return class->option;
    if (last && last[1].name)
        return ++last;
    return NULL;
}

static int write_number(void *obj, const AVOption *o, void *dst, double num, int den, int64_t intnum)
{
    if (o->type != TTV_OPT_TYPE_FLAGS &&
        (o->max * den < num * intnum || o->min * den > num * intnum)) {
        ttv_log(obj, TTV_LOG_ERROR, "Value %f for parameter '%s' out of range [%g - %g]\n",
               num*intnum/den, o->name, o->min, o->max);
        return AVERROR(ERANGE);
    }
    if (o->type == TTV_OPT_TYPE_FLAGS) {
        double d = num*intnum/den;
        if (d < -1.5 || d > 0xFFFFFFFF+0.5 || (llrint(d*256) & 255)) {
            ttv_log(obj, TTV_LOG_ERROR,
                   "Value %f for parameter '%s' is not a valid set of 32bit integer flags\n",
                   num*intnum/den, o->name);
            return AVERROR(ERANGE);
        }
    }

    switch (o->type) {
    case TTV_OPT_TYPE_FLAGS:
    case TTV_OPT_TYPE_PIXEL_FMT:
    case TTV_OPT_TYPE_SAMPLE_FMT:
    case TTV_OPT_TYPE_INT:   *(int       *)dst= llrint(num/den)*intnum; break;
    case TTV_OPT_TYPE_DURATION:
    case TTV_OPT_TYPE_CHANNEL_LAYOUT:
    case TTV_OPT_TYPE_INT64: *(int64_t   *)dst= llrint(num/den)*intnum; break;
    case TTV_OPT_TYPE_FLOAT: *(float     *)dst= num*intnum/den;         break;
    case TTV_OPT_TYPE_DOUBLE:*(double    *)dst= num*intnum/den;         break;
    case TTV_OPT_TYPE_RATIONAL:
        if ((int)num == num) *(TTRational*)dst= ttv_make_q(num*intnum, den);
        else                 *(TTRational*)dst= ttv_d2q(num*intnum/den, 1<<24);
        break;
    default:
        return AVERROR(EINVAL);
    }
    return 0;
}

static int hexchar2int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}



static int set_string(void *obj, const AVOption *o, const char *val, uint8_t **dst)
{
    ttv_freep(dst);
    *dst = ttv_strdup(val);
    return 0;
}


void ttv_opt_set_defaults(void *s)
{
#if TT_API_OLD_AVOPTIONS
    ttv_opt_set_defaults2(s, 0, 0);
}

void ttv_opt_set_defaults2(void *s, int mask, int flags)
{
#endif
    const AVOption *opt = NULL;
    while ((opt = ttv_opt_next(s, opt))) {
        void *dst = ((uint8_t*)s) + opt->offset;
#if TT_API_OLD_AVOPTIONS
        if ((opt->flags & mask) != flags)
            continue;
#endif

        if (opt->flags & TTV_OPT_FLAG_READONLY)
            continue;

        switch (opt->type) {
            case TTV_OPT_TYPE_CONST:
                /* Nothing to be done here */
            break;
            case TTV_OPT_TYPE_FLAGS:
            case TTV_OPT_TYPE_INT:
            case TTV_OPT_TYPE_INT64:
            case TTV_OPT_TYPE_DURATION:
            case TTV_OPT_TYPE_CHANNEL_LAYOUT:
                write_number(s, opt, dst, 1, 1, opt->default_val.i64);
            break;
            case TTV_OPT_TYPE_DOUBLE:
            case TTV_OPT_TYPE_FLOAT: {
                double val;
                val = opt->default_val.dbl;
                write_number(s, opt, dst, val, 1, 1);
            }
            break;
            case TTV_OPT_TYPE_RATIONAL: {
                TTRational val;
                val = ttv_d2q(opt->default_val.dbl, INT_MAX);
                write_number(s, opt, dst, 1, val.den, val.num);
            }
            break;
//             case TTV_OPT_TYPE_COLOR:
//                 set_string_color(s, opt, opt->default_val.str, dst);
//                 break;
            case TTV_OPT_TYPE_STRING:
                set_string(s, opt, opt->default_val.str, dst);
                break;
//             case TTV_OPT_TYPE_IMAGE_SIZE:
//                 set_string_image_size(s, opt, opt->default_val.str, dst);
//                 break;
//             case TTV_OPT_TYPE_VIDEO_RATE:
//                 set_string_video_rate(s, opt, opt->default_val.str, dst);
//                 break;
            case TTV_OPT_TYPE_PIXEL_FMT:
                write_number(s, opt, dst, 1, 1, opt->default_val.i64);
                break;
            case TTV_OPT_TYPE_SAMPLE_FMT:
                write_number(s, opt, dst, 1, 1, opt->default_val.i64);
                break;
//             case TTV_OPT_TYPE_BINARY:
//                 set_string_binary(s, opt, opt->default_val.str, dst);
                break;
            case TTV_OPT_TYPE_DICT:
                /* Cannot set defaults for these types */
            break;
            default:
                ttv_log(s, TTV_LOG_DEBUG, "AVOption type %d of option %s not implemented yet\n", opt->type, opt->name);
        }
    }
}

void ttv_opt_free(void *obj)
{
    const AVOption *o = NULL;
    while ((o = ttv_opt_next(obj, o))) {
        switch (o->type) {
        case TTV_OPT_TYPE_STRING:
        case TTV_OPT_TYPE_BINARY:
            ttv_freep((uint8_t *)obj + o->offset);
            break;

        case TTV_OPT_TYPE_DICT:
            ttv_dict_free((TTDictionary **)(((uint8_t *)obj) + o->offset));
            break;

        default:
            break;
        }
    }
}



int ttv_opt_set_dict(void *obj, TTDictionary **options)
{
    return 0;
}
