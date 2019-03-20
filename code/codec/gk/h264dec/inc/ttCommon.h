#ifndef __TTPOD_TT_COMMON_H_
#define __TTPOD_TT_COMMON_H_

#if defined(__cplusplus) && !defined(__STDC_CONSTANT_MACROS) && !defined(UINT64_C)
#error missing -D__STDC_CONSTANT_MACROS / #define __STDC_CONSTANT_MACROS
#endif

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ttAttributes.h"
#include "ttVersion.h"

#define TTV_HAVE_BIGENDIAN 0

#if TTV_HAVE_BIGENDIAN
#   define TTV_NE(be, le) (be)
#else
#   define TTV_NE(be, le) (le)
#endif

//rounded division & shift
#define RSHIFT(a,b) ((a) > 0 ? ((a) + ((1<<(b))>>1))>>(b) : ((a) + ((1<<(b))>>1)-1)>>(b))
/* assume b>0 */
#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
/* assume a>0 and b>0 */
#define TT_CEIL_RSHIFT(a,b) (!ttv_builtin_constant_p(b) ? -((-(a)) >> (b)) \
                                                       : ((a) + (1<<(b)) - 1) >> (b))
#define FFUDIV(a,b) (((a)>0 ?(a):(a)-(b)+1) / (b))
#define FFUMOD(a,b) ((a)-(b)*FFUDIV(a,b))
#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))
#define FFSIGN(a) ((a) > 0 ? 1 : -1)

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
#define TT_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#define FFALIGN(x, a) (((x)+(a)-1)&~((a)-1))

/* misc math functions */

/**
 * Reverse the order of the bits of an 8-bits unsigned integer.
 */
#if TT_API_TTV_REVERSE
extern attribute_deprecated const uint8_t ttv_reverse[256];
#endif

#ifdef HAVE_TTV_CONFIG_H
#   include "config.h"
#   include "ttIntmath.h"
#endif

/* Pull in unguarded fallback defines at the end of this file. */
#include "ttCommon.h"

#ifndef ttv_log2
ttv_const int ttv_log2(unsigned v);
#endif

#ifndef ttv_log2_16bit
ttv_const int ttv_log2_16bit(unsigned v);
#endif

/**
 * Clip a signed integer value into the amin-amax range.
 * @param a value to clip
 * @param amin minimum value of the clip range
 * @param amax maximum value of the clip range
 * @return clipped value
 */
static ttv_always_inline ttv_const int ttv_clip_c(int a, int amin, int amax)
{
#if defined(HAVE_TTV_CONFIG_H) && defined(ASSERT_LEVEL) && ASSERT_LEVEL >= 2
    if (amin > amax) abort();
#endif
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}


/**
 * Clip a signed integer value into the 0-255 range.
 * @param a value to clip
 * @return clipped value
 */
static ttv_always_inline ttv_const uint8_t ttv_clip_uint8_c(int a)
{
    if (a&(~0xFF)) return (-a)>>31;
    else           return a;
}



/**
 * Count number of bits set to one in x
 * @param x value to count bits of
 * @return the number of bits set to one in x
 */
static ttv_always_inline ttv_const int ttv_popcount_c(uint32_t x)
{
    x -= (x >> 1) & 0x55555555;
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    x += x >> 8;
    return (x + (x >> 16)) & 0x3F;
}

/**
 * Count number of bits set to one in x
 * @param x value to count bits of
 * @return the number of bits set to one in x
 */
static ttv_always_inline ttv_const int ttv_popcount64_c(uint64_t x)
{
    return ttv_popcount((uint32_t)x) + ttv_popcount((uint32_t)(x >> 32));
}

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define MKBETAG(a,b,c,d) ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))



#include "ttMem.h"

#ifdef HAVE_TTV_CONFIG_H
#    include "ttInternal.h"
#endif /* HAVE_TTV_CONFIG_H */

#endif /* __TTPOD_TT_COMMON_H_ */

/*
 * The following definitions are outside the multiple inclusion guard
 * to ensure they are immediately available in ttIntmath.h.
 */

#ifndef ttv_clip
#   define ttv_clip          ttv_clip_c
#endif

#ifndef ttv_clip_uint8
#   define ttv_clip_uint8    ttv_clip_uint8_c
#endif



#ifndef ttv_popcount
#   define ttv_popcount      ttv_popcount_c
#endif
#ifndef ttv_popcount64
#   define ttv_popcount64    ttv_popcount64_c
#endif
