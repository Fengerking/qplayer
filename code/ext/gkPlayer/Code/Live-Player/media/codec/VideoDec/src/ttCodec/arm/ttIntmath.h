
#ifndef __TTPOD_TT_ARM_INTMATH_H
#define __TTPOD_TT_ARM_INTMATH_H

#include <stdint.h>

#include "config.h"
#include "ttCodec/ttAttributes.h"

#if HAVE_INLINE_ASM

#if HAVE_ARMV6_INLINE

#define ttv_clip_uint8 ttv_clip_uint8_arm
static ttv_always_inline ttv_const unsigned ttv_clip_uint8_arm(int a)
{
    unsigned x;
    __asm__ ("usat %0, #8,  %1" : "=r"(x) : "r"(a));
    return x;
}

#define ttv_clip_int8 ttv_clip_int8_arm
static ttv_always_inline ttv_const int ttv_clip_int8_arm(int a)
{
    int x;
    __asm__ ("ssat %0, #8,  %1" : "=r"(x) : "r"(a));
    return x;
}

#define ttv_clip_uint16 ttv_clip_uint16_arm
static ttv_always_inline ttv_const unsigned ttv_clip_uint16_arm(int a)
{
    unsigned x;
    __asm__ ("usat %0, #16, %1" : "=r"(x) : "r"(a));
    return x;
}

#define ttv_clip_int16 ttv_clip_int16_arm
static ttv_always_inline ttv_const int ttv_clip_int16_arm(int a)
{
    int x;
    __asm__ ("ssat %0, #16, %1" : "=r"(x) : "r"(a));
    return x;
}

#define ttv_clip_uintp2 ttv_clip_uintp2_arm
static ttv_always_inline ttv_const unsigned ttv_clip_uintp2_arm(int a, int p)
{
    unsigned x;
    __asm__ ("usat %0, %2, %1" : "=r"(x) : "r"(a), "i"(p));
    return x;
}

#define ttv_sat_add32 ttv_sat_add32_arm
static ttv_always_inline int ttv_sat_add32_arm(int a, int b)
{
    int r;
    __asm__ ("qadd %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

#define ttv_sat_dadd32 ttv_sat_dadd32_arm
static ttv_always_inline int ttv_sat_dadd32_arm(int a, int b)
{
    int r;
    __asm__ ("qdadd %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

#endif /* HAVE_ARMV6_INLINE */

#if HAVE_ASM_MOD_Q

#define ttv_clipl_int32 ttv_clipl_int32_arm
static ttv_always_inline ttv_const int32_t ttv_clipl_int32_arm(int64_t a)
{
    int x, y;
    __asm__ ("adds   %1, %R2, %Q2, lsr #31  \n\t"
             "itet   ne                     \n\t"
             "mvnne  %1, #1<<31             \n\t"
             "moveq  %0, %Q2                \n\t"
             "eorne  %0, %1,  %R2, asr #31  \n\t"
             : "=r"(x), "=&r"(y) : "r"(a) : "cc");
    return x;
}

#endif /* HAVE_ASM_MOD_Q */

#endif /* HAVE_INLINE_ASM */

#endif /* __TTPOD_TT_ARM_INTMATH_H */
