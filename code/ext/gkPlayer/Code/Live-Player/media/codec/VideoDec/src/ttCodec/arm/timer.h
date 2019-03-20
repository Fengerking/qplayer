
#ifndef __TTPOD_TT_ARM_TIMER_H
#define __TTPOD_TT_ARM_TIMER_H

#include <stdint.h>
#include "config.h"

#if HAVE_INLINE_ASM && defined(__ARM_ARCH_7A__)

#define TTV_READ_TIME read_time

static inline uint64_t read_time(void)
{
    unsigned cc;
    __asm__ volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r"(cc));
    return cc;
}

#endif /* HAVE_INLINE_ASM && __ARM_ARCH_7A__ */

#endif /* __TTPOD_TT_ARM_TIMER_H */
