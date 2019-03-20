
#ifndef SSE_HEADER_H
#define SSE_HEADER_H

//#ifdef __cplusplus
//extern "C"
//{
//#endif /* __cplusplus */

#ifdef _WIN32

#define USE_SSE2

#ifdef USE_SSE2
#include <emmintrin.h>
#elif defined(USE_AVX)
#include <immintrin.h>
#endif /* abc */

#elif defined BEEPS_FOR_ANDROID
#define USE_NEON_FUN
#include <arm_neon.h>
#endif /* INTRINSICS */

//#ifdef __cplusplus
//}
//#endif /* __cplusplus */

#endif /* SSE_HEADER_H */