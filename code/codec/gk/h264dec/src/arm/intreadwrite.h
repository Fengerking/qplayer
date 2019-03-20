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

#ifndef __TTPOD_TT_ARM_INTREADWRITE_H
#define __TTPOD_TT_ARM_INTREADWRITE_H

#include <stdint.h>
#include "config.h"
#include "ttAttributes.h"

#if HAVE_FAST_UNALIGNED && HAVE_INLINE_ASM && !TTV_GCC_VERSION_AT_LEAST(4,7)

#define TTV_RN16 TTV_RN16
static ttv_always_inline unsigned TTV_RN16(const void *p)
{
    const uint8_t *q = p;
    unsigned v;
#if !TTV_GCC_VERSION_AT_LEAST(4,6)
    __asm__ ("ldrh %0, %1" : "=r"(v) : "m"(*(const uint16_t *)q));
#elif defined __thumb__
    __asm__ ("ldrh %0, %1" : "=r"(v) : "m"(q[0]), "m"(q[1]));
#else
    __asm__ ("ldrh %0, %1" : "=r"(v) : "Uq"(q[0]), "m"(q[1]));
#endif
    return v;
}

#define TTV_WN16 TTV_WN16
static ttv_always_inline void TTV_WN16(void *p, uint16_t v)
{
    __asm__ ("strh %1, %0" : "=m"(*(uint16_t *)p) : "r"(v));
}

#define TTV_RN32 TTV_RN32
static ttv_always_inline uint32_t TTV_RN32(const void *p)
{
    const struct __attribute__((packed)) { uint32_t v; } *q = p;
    uint32_t v;
    __asm__ ("ldr  %0, %1" : "=r"(v) : "m"(*q));
    return v;
}

#define TTV_WN32 TTV_WN32
static ttv_always_inline void TTV_WN32(void *p, uint32_t v)
{
    __asm__ ("str  %1, %0" : "=m"(*(uint32_t *)p) : "r"(v));
}

#if HAVE_ASM_MOD_Q

#define TTV_RN64 TTV_RN64
static ttv_always_inline uint64_t TTV_RN64(const void *p)
{
    const struct __attribute__((packed)) { uint32_t v; } *q = p;
    uint64_t v;
    __asm__ ("ldr   %Q0, %1  \n\t"
             "ldr   %R0, %2  \n\t"
             : "=&r"(v)
             : "m"(q[0]), "m"(q[1]));
    return v;
}

#define TTV_WN64 TTV_WN64
static ttv_always_inline void TTV_WN64(void *p, uint64_t v)
{
    __asm__ ("str  %Q2, %0  \n\t"
             "str  %R2, %1  \n\t"
             : "=m"(*(uint32_t*)p), "=m"(*((uint32_t*)p+1))
             : "r"(v));
}

#endif /* HAVE_ASM_MOD_Q */

#endif /* HAVE_INLINE_ASM */

#endif /* __TTPOD_TT_ARM_INTREADWRITE_H */
