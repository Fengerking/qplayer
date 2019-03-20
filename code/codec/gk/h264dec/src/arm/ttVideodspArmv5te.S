
#include "config.h"
#include "asm.S"

function tt_prefetch_arm, export=1
        subs            r2,  r2,  #1
        pld             [r0]
        add             r0,  r0,  r1
        bne             X(tt_prefetch_arm)
        bx              lr
endfunc
