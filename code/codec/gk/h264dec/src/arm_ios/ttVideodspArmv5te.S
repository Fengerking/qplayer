#include "ttH264DecArm32Macro.h"

#include "config.h"
#include "asm.S"
#if H264_ARM32_OPEN

function tt_prefetch_arm, 1
        subs            r2,  r2,  #1
        pld             [r0]
        add             r0,  r0,  r1
        bne             X(tt_prefetch_arm)
        bx              lr
endfunc
#endif