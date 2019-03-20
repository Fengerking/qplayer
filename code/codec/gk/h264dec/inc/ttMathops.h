#ifndef __TTPOD_TT_MATHOPS_H
#define __TTPOD_TT_MATHOPS_H

#include <stdint.h>

#include "ttCommon.h"
#include "config.h"

#define MAX_NEG_CROP 1024

extern const uint32_t tt_inverse[257];
extern const uint8_t  tt_reverse[256];

extern const uint8_t tt_zigzag_direct[64];

#ifdef __APPLE__

#if   ARCH_ARM
#   include "arm_ios/ttMathops.h"

#endif

#else

#if   ARCH_ARM
#   include "arm/ttMathops.h"
#elif ARCH_X86
#   include "x86/ttMathops.h"
#endif

#endif

/* generic implementation */


/* signed 16x16 -> 32 multiply add accumulate */
#ifndef MAC16
#   define MAC16(rt, ra, rb) rt += (ra) * (rb)
#endif

/* signed 16x16 -> 32 multiply */
#ifndef MUL16
#   define MUL16(ra, rb) ((ra) * (rb))
#endif



/* median of 3 */
#ifndef mid_pred
#define mid_pred mid_pred
static inline ttv_const int mid_pred(int a, int b, int c)
{
#if 0
    int t= (a-b)&((a-b)>>31);
    a-=t;
    b+=t;
    b-= (b-c)&((b-c)>>31);
    b+= (a-b)&((a-b)>>31);

    return b;
#else
    if(a>b){
        if(c>b){
            if(c>a) b=a;
            else    b=c;
        }
    }else{
        if(b>c){
            if(c>a) b=c;
            else    b=a;
        }
    }
    return b;
#endif
}
#endif

#ifndef sign_extend
static inline ttv_const int sign_extend(int val, unsigned bits)
{
    unsigned shift = 8 * sizeof(int) - bits;
    union { unsigned u; int s; } v = { (unsigned) val << shift };
    return v.s >> shift;
}
#endif

#ifndef NEG_SSR32
#   define NEG_SSR32(a,s) ((( int32_t)(a))>>(32-(s)))
#endif

#ifndef NEG_USR32
#   define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))
#endif

#if HAVE_BIGENDIAN
# ifndef PACK_2U8
#   define PACK_2U8(a,b)     (((a) <<  8) | (b))
# endif
# ifndef PACK_4U8
#   define PACK_4U8(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
# endif
# ifndef PACK_2U16
#   define PACK_2U16(a,b)    (((a) << 16) | (b))
# endif
#else
# ifndef PACK_2U8
#   define PACK_2U8(a,b)     (((b) <<  8) | (a))
# endif
# ifndef PACK_4U2
#   define PACK_4U8(a,b,c,d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))
# endif
# ifndef PACK_2U16
#   define PACK_2U16(a,b)    (((b) << 16) | (a))
# endif
#endif

#ifndef PACK_2S8
#   define PACK_2S8(a,b)     PACK_2U8((a)&255, (b)&255)
#endif


#endif /* __TTPOD_TT_MATHOPS_H */
