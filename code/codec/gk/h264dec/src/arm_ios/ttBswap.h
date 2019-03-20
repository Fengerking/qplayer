/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __TTPOD_TT_ARM_BSWAP_H
#define __TTPOD_TT_ARM_BSWAP_H

#include <stdint.h>
#include "config.h"
#include "libavutil/attributes.h"

#ifdef __ARMCC_VERSION

#if HAVE_ARMV6
#define ttv_bswap32 ttv_bswap32
static ttv_always_inline ttv_const uint32_t ttv_bswap32(uint32_t x)
{
    return __rev(x);
}
#endif /* HAVE_ARMV6 */

#elif HAVE_INLINE_ASM

#if HAVE_ARMV6_INLINE
#define ttv_bswap16 ttv_bswap16
static ttv_always_inline ttv_const unsigned ttv_bswap16(unsigned x)
{
    __asm__("rev16 %0, %0" : "+r"(x));
    return x;
}
#endif

#if !TTV_GCC_VERSION_AT_LEAST(4,5)
#define ttv_bswap32 ttv_bswap32
static ttv_always_inline ttv_const uint32_t ttv_bswap32(uint32_t x)
{
#if HAVE_ARMV6_INLINE
    __asm__("rev %0, %0" : "+r"(x));
#else
    uint32_t t;
    __asm__ ("eor %1, %0, %0, ror #16 \n\t"
             "bic %1, %1, #0xFF0000   \n\t"
             "mov %0, %0, ror #8      \n\t"
             "eor %0, %0, %1, lsr #8  \n\t"
             : "+r"(x), "=&r"(t));
#endif /* HAVE_ARMV6_INLINE */
    return x;
}
#endif /* !TTV_GCC_VERSION_AT_LEAST(4,5) */

#endif /* __ARMCC_VERSION */

#endif /* __TTPOD_TT_ARM_BSWAP_H */
