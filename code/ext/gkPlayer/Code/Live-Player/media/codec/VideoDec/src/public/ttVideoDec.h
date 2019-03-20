#ifndef __ffmpegWrap_H__
#define __ffmpegWrap_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "GKOsalConfig.h"
#include "TTVideo.h"
#include "AVCDecoderTypesInner.h"

DLLEXPORT_C TTInt32 ttGetH264DecAPI (TTVideoCodecAPI* pDecHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __ffmpegWrap_H__
