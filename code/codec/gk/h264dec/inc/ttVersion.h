#ifndef __TTPOD_VERSION_H_
#define __TTPOD_VERSION_H_

#include "ttMacros.h"

/**
 * @addtogroup version_utils
 *
 * Useful to check and match library version in order to maintain
 * backward compatibility.
 *
 * @{
 */

#define TTV_VERSION_INT(a, b, c) (a<<16 | b<<8 | c)
#define TTV_VERSION_DOT(a, b, c) a ##.## b ##.## c
#define TTV_VERSION(a, b, c) TTV_VERSION_DOT(a, b, c)

/**
 * @}
 */

/**
 * @file
 * @ingroup lavu
 * Libavutil version macros
 */

/**
 * @defgroup lavu_ver Version and Build diagnostics
 *
 * Macros and function useful to check at compiletime and at runtime
 * which version of libavutil is in use.
 *
 * @{
 */

#define LIB__TTPOD_TT_VERSION_MAJOR  54
#define LIB__TTPOD_TT_VERSION_MINOR  14
#define LIB__TTPOD_TT_VERSION_MICRO 100

#define LIB__TTPOD_TT_VERSION_INT   TTV_VERSION_INT(LIB__TTPOD_TT_VERSION_MAJOR, \
                                               LIB__TTPOD_TT_VERSION_MINOR, \
                                               LIB__TTPOD_TT_VERSION_MICRO)
#define LIB__TTPOD_TT_VERSION       TTV_VERSION(LIB__TTPOD_TT_VERSION_MAJOR,     \
                                           LIB__TTPOD_TT_VERSION_MINOR,     \
                                           LIB__TTPOD_TT_VERSION_MICRO)
#define LIB__TTPOD_TT_BUILD         LIB__TTPOD_TT_VERSION_INT

#define LIB__TTPOD_TT_IDENT         "Lavu" TTV_STRINGIFY(LIB__TTPOD_TT_VERSION)

/**
 * @}
 *
 * @defgroup depr_guards Deprecation guards
 * TT_API_* defines may be placed below to indicate public API that will be
 * dropped at a future version bump. The defines themselves are not part of
 * the public API and may change, break or disappear at any time.
 *
 * @{
 */

#ifndef TT_API_OLD_AVOPTIONS
#define TT_API_OLD_AVOPTIONS            (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_PIX_FMT
#define TT_API_PIX_FMT                  (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_CONTEXT_SIZE
#define TT_API_CONTEXT_SIZE             (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_PIX_FMT_DESC
#define TT_API_PIX_FMT_DESC             (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_TTV_REVERSE
#define TT_API_TTV_REVERSE               (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_AUDIOCONVERT
#define TT_API_AUDIOCONVERT             (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_CPU_FLAG_MMX2
#define TT_API_CPU_FLAG_MMX2            (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_LLS_PRIVATE
#define TT_API_LLS_PRIVATE              (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_AVFRAME_LAVC
#define TT_API_AVFRAME_LAVC             (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_VDPAU
#define TT_API_VDPAU                    (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_GET_CHANNEL_LAYOUT_COMPAT
#define TT_API_GET_CHANNEL_LAYOUT_COMPAT (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_XVMC
#define TT_API_XVMC                     (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif
#ifndef TT_API_OPT_TYPE_METADATA
#define TT_API_OPT_TYPE_METADATA        (LIB__TTPOD_TT_VERSION_MAJOR < 55)
#endif


#ifndef TT_CONST_AVUTIL53
#if LIB__TTPOD_TT_VERSION_MAJOR >= 53
#define TT_CONST_AVUTIL53 const
#else
#define TT_CONST_AVUTIL53
#endif
#endif


#define TTPOD_VERSION_MAJOR 56
#define TTPOD_VERSION_MINOR  12
#define TTPOD_VERSION_MICRO 101

#define TTPOD_VERSION_INT  TTV_VERSION_INT(TTPOD_VERSION_MAJOR, \
                                               TTPOD_VERSION_MINOR, \
                                               TTPOD_VERSION_MICRO)
#define TTPOD_VERSION      TTV_VERSION(TTPOD_VERSION_MAJOR,    \
                                           TTPOD_VERSION_MINOR,    \
                                           TTPOD_VERSION_MICRO)
#define TTPOD_BUILD        TTPOD_VERSION_INT

#define TTPOD_IDENT        "Lavc" TTV_STRINGIFY(TTPOD_VERSION)

/**
 * TT_API_* defines may be placed below to indicate public API that will be
 * dropped at a future version bump. The defines themselves are not part of
 * the public API and may change, break or disappear at any time.
 */

#ifndef TT_API_REQUEST_CHANNELS
#define TT_API_REQUEST_CHANNELS (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_OLD_DECODE_AUDIO
#define TT_API_OLD_DECODE_AUDIO (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_OLD_ENCODE_AUDIO
#define TT_API_OLD_ENCODE_AUDIO (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_OLD_ENCODE_VIDEO
#define TT_API_OLD_ENCODE_VIDEO (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_CODEC_ID
#define TT_API_CODEC_ID          (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_AUDIO_CONVERT
#define TT_API_AUDIO_CONVERT     (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API___TTPOD_TT_RESAMPLE
#define TT_API___TTPOD_TT_RESAMPLE  TT_API_AUDIO_CONVERT
#endif
#ifndef TT_API_DEINTERLACE
#define TT_API_DEINTERLACE       (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_DESTRUCT_PACKET
#define TT_API_DESTRUCT_PACKET   (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_GET_BUFFER
#define TT_API_GET_BUFFER        (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_MISSING_SAMPLE
#define TT_API_MISSING_SAMPLE    (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_LOWRES
#define TT_API_LOWRES            (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_CAP_VDPAU
#define TT_API_CAP_VDPAU         (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_BUFS_VDPAU
#define TT_API_BUFS_VDPAU        (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_VOXWARE
#define TT_API_VOXWARE           (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_SET_DIMENSIONS
#define TT_API_SET_DIMENSIONS    (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_DEBUG_MV
#define TT_API_DEBUG_MV          (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_AC_VLC
#define TT_API_AC_VLC            (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_OLD_MSMPEG4
#define TT_API_OLD_MSMPEG4       (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_ASPECT_EXTENDED
#define TT_API_ASPECT_EXTENDED   (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_THREAD_OPAQUE
#define TT_API_THREAD_OPAQUE     (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_CODEC_PKT
#define TT_API_CODEC_PKT         (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_ARCH_ALPHA
#define TT_API_ARCH_ALPHA        (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_XVMC
#define TT_API_XVMC              (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_ERROR_RATE
#define TT_API_ERROR_RATE        (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_QSCALE_TYPE
#define TT_API_QSCALE_TYPE       (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_MB_TYPE
#define TT_API_MB_TYPE           (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_MAX_BFRAMES
#define TT_API_MAX_BFRAMES       (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_NEG_LINESIZES
#define TT_API_NEG_LINESIZES     (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_EMU_EDGE
#define TT_API_EMU_EDGE          (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_ARCH_SH4
#define TT_API_ARCH_SH4          (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_ARCH_SPARC
#define TT_API_ARCH_SPARC        (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_UNUSED_MEMBERS
#define TT_API_UNUSED_MEMBERS    (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_IDCT_XVIDMMX
#define TT_API_IDCT_XVIDMMX      (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_INPUT_PRESERVED
#define TT_API_INPUT_PRESERVED   (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_NORMALIZE_AQP
#define TT_API_NORMALIZE_AQP     (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_GMC
#define TT_API_GMC               (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_MV0
#define TT_API_MV0               (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_CODEC_NAME
#define TT_API_CODEC_NAME        (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_AFD
#define TT_API_AFD               (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_VISMV
/* XXX: don't forget to drop the -vismv documentation */
#define TT_API_VISMV             (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_DV_FRAME_PROFILE
#define TT_API_DV_FRAME_PROFILE  (TTPOD_VERSION_MAJOR < 57)
#endif
#ifndef TT_API_AUDIOENC_DELAY
#define TT_API_AUDIOENC_DELAY    (TTPOD_VERSION_MAJOR < 58)
#endif
#ifndef TT_API_AVCTX_TIMEBASE
#define TT_API_AVCTX_TIMEBASE    (TTPOD_VERSION_MAJOR < 59)
#endif
#ifndef TT_API_MPV_OPT
#define TT_API_MPV_OPT           (TTPOD_VERSION_MAJOR < 59)
#endif

#endif /* __TTPOD_VERSION_H_ */

