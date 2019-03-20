#include "ttH264DecArm32Macro.h"
#include "asm.S"

#if H264_ARM32_OPEN

function tt_clear_block_neon, 1
        vmov.i16        q0,  #0
        .rept           8
        vst1.16         {q0}, [r0,:128]!
        .endr
        bx              lr
endfunc

function tt_clear_blocks_neon, 1
        vmov.i16        q0,  #0
        .rept           8*6
        vst1.16         {q0}, [r0,:128]!
        .endr
        bx              lr
endfunc
#endif