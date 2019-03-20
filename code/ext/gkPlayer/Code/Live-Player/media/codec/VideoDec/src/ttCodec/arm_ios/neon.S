#include "ttH264DecArm32Macro.h"
#if H264_ARM32_OPEN
.macro  transpose_8x8   r0, r1, r2, r3, r4, r5, r6, r7
        vtrn.32         \r0, \r4
        vtrn.32         \r1, \r5
        vtrn.32         \r2, \r6
        vtrn.32         \r3, \r7
        vtrn.16         \r0, \r2
        vtrn.16         \r1, \r3
        vtrn.16         \r4, \r6
        vtrn.16         \r5, \r7
        vtrn.8          \r0, \r1
        vtrn.8          \r2, \r3
        vtrn.8          \r4, \r5
        vtrn.8          \r6, \r7
.endm

.macro  transpose_4x4   r0, r1, r2, r3
        vtrn.16         \r0, \r2
        vtrn.16         \r1, \r3
        vtrn.8          \r0, \r1
        vtrn.8          \r2, \r3
.endm

.macro  swap4           r0, r1, r2, r3, r4, r5, r6, r7
        vswp            \r0, \r4
        vswp            \r1, \r5
        vswp            \r2, \r6
        vswp            \r3, \r7
.endm

.macro  transpose16_4x4 r0, r1, r2, r3, r4, r5, r6, r7
        vtrn.32         \r0, \r2
        vtrn.32         \r1, \r3
        vtrn.32         \r4, \r6
        vtrn.32         \r5, \r7
        vtrn.16         \r0, \r1
        vtrn.16         \r2, \r3
        vtrn.16         \r4, \r5
        vtrn.16         \r6, \r7
.endm
#endif