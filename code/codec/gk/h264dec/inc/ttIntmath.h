#ifndef __TTPOD_TT_INTMATH_H_
#define __TTPOD_TT_INTMATH_H_

#include <stdint.h>

#include "config.h"
#include "ttAttributes.h"

#ifdef __APPLE__

#if ARCH_ARM
#   include "arm_ios/ttIntmath.h"
#endif

#else

#if ARCH_ARM
#   include "arm/ttIntmath.h"
#endif

#endif

/**
 * @addtogroup lavu_internal
 * @{
 */

#if HAVE_FAST_CLZ
#if TTV_GCC_VERSION_AT_LEAST(3,4)
#ifndef tt_log2_as
#   define tt_log2_as(x) (31 - __builtin_clz((x)|1))
#   ifndef tt_log2_16bit
#      define tt_log2_16bit ttv_log2
#   endif
#endif /* tt_log2_as */
#elif defined( __INTEL_COMPILER )
#ifndef tt_log2_as
#   define tt_log2_as(x) (_bit_scan_reverse(x|1))
#   ifndef tt_log2_16bit
#      define tt_log2_16bit ttv_log2
#   endif
#endif /* tt_log2_as */
#endif
#endif /* TTV_GCC_VERSION_AT_LEAST(3,4) */

extern const uint8_t tt_log2_tab[256];

#ifndef tt_log2_as
#define tt_log2_as tt_log2_c
#if !defined( _MSC_VER )
static ttv_always_inline ttv_const int tt_log2_c(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += tt_log2_tab[v];

    return n;
}
#else
static ttv_always_inline ttv_const int tt_log2_c(unsigned int v)
{
    unsigned int n;
    _BitScanReverse(&n, v|1);
    return n;
}
#define tt_log2_16bit ttv_log2
#endif
#endif

#ifndef tt_log2_16bit
#define tt_log2_16bit tt_log2_16bit_c
static ttv_always_inline ttv_const int tt_log2_16bit_c(unsigned int v)
{
    int n = 0;
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += tt_log2_tab[v];

    return n;
}
#endif

#define ttv_log2       tt_log2_as
#define ttv_log2_16bit tt_log2_16bit


#endif /* __TTPOD_TT_INTMATH_H_ */
