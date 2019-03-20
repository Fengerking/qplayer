#ifndef __TTPOD_TT_AVCODEC_H__
#define __TTPOD_TT_AVCODEC_H__

#include "ttCommon.h"
#include "ttError.h"
#include "ttRational.h"
#include "ttVersion.h"
#include "ttMacros.h"
#include "ttMathematics.h"
#include "ttLog.h"


enum AVMediaType {
    AVMEDIA_TYPE_UNKNOWN = -1,  ///< Usually treated as AVMEDIA_TYPE_DATA
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_DATA,          ///< Opaque data information usually continuous
    AVMEDIA_TYPE_SUBTITLE,
    AVMEDIA_TYPE_ATTACHMENT,    ///< Opaque data information usually sparse
    AVMEDIA_TYPE_NB
};


#define TT_QP2LAMBDA 118 ///< factor to convert from H.263 QP to lambda
#define TT_LAMBDA_MAX (256*128-1)



/**
 * @}
 * @defgroup lavu_time Timestamp specific
 *
 * FFmpeg internal timebase and timestamp definitions
 *
 * @{
 */

/**
 * @brief Undefined timestamp value
 *
 * Usually reported by demuxer that work on containers that do not provide
 * either pts or dts.
 */

#define TTV_NOPTS_VALUE          ((int64_t)UINT64_C(0x8000000000000000))


enum TTPictureType {
    TTV_PICTURE_TYPE_NONE = 0, ///< Undefined
    TTV_PICTURE_TYPE_I,     ///< Intra
    TTV_PICTURE_TYPE_P,     ///< Predicted
    TTV_PICTURE_TYPE_B,     ///< Bi-dir predicted
    TTV_PICTURE_TYPE_S,     ///< S(GMC)-VOP MPEG4
    TTV_PICTURE_TYPE_SI,    ///< Switching Intra
    TTV_PICTURE_TYPE_SP,    ///< Switching Predicted
    TTV_PICTURE_TYPE_BI,    ///< BI type
};

#endif /* __TTPOD_TT_AVCODEC_H__ */
