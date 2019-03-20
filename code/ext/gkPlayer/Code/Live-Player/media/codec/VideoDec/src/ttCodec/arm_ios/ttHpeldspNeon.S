#include "ttH264DecArm32Macro.h"

#include "asm.S"
#if H264_ARM32_OPEN

.macro  pixels16        rnd=1, avg=0, rnd=1
  .if \avg
        mov             r12, r0
  .endif
1:      vld1.8          {q0},     [r1], r2
        vld1.8          {q1},     [r1], r2
        vld1.8          {q2},     [r1], r2
        pld             [r1, r2, lsl #2]
        vld1.8          {q3},     [r1], r2
        pld             [r1]
        pld             [r1, r2]
        pld             [r1, r2, lsl #1]
  .if \avg
        vld1.8          {q8},     [r12,:128], r2
        vrhadd.u8       q0,  q0,  q8
        vld1.8          {q9},     [r12,:128], r2
        vrhadd.u8       q1,  q1,  q9
        vld1.8          {q10},    [r12,:128], r2
        vrhadd.u8       q2,  q2,  q10
        vld1.8          {q11},    [r12,:128], r2
        vrhadd.u8       q3,  q3,  q11
  .endif
        subs            r3,  r3,  #4
        vst1.64         {q0},     [r0,:128], r2
        vst1.64         {q1},     [r0,:128], r2
        vst1.64         {q2},     [r0,:128], r2
        vst1.64         {q3},     [r0,:128], r2
        bne             1b
        bx              lr
.endm

.macro  pixels16_x2     rnd=1, avg=0, rnd=1
1:      vld1.8          {d0-d2},  [r1], r2
        vld1.8          {d4-d6},  [r1], r2
        pld             [r1]
        pld             [r1, r2]
        subs            r3,  r3,  #2
        vext.8          q1,  q0,  q1,  #1
        avg             q0,  q0,  q1,  \rnd
        vext.8          q3,  q2,  q3,  #1
        avg             q2,  q2,  q3,  \rnd
  .if \avg
        vld1.8          {q1},     [r0,:128], r2
        vld1.8          {q3},     [r0,:128]
        vrhadd.u8       q0,  q0,  q1
        vrhadd.u8       q2,  q2,  q3
        sub             r0,  r0,  r2
  .endif
        vst1.8          {q0},     [r0,:128], r2
        vst1.8          {q2},     [r0,:128], r2
        bne             1b
        bx              lr
.endm

.macro  pixels16_y2     rnd=1, avg=0, rnd=1
        sub             r3,  r3,  #2
        vld1.8          {q0},     [r1], r2
        vld1.8          {q1},     [r1], r2
1:      subs            r3,  r3,  #2
        avg             q2,  q0,  q1,   \rnd
        vld1.8          {q0},     [r1], r2
        avg             q3,  q0,  q1,   \rnd
        vld1.8          {q1},     [r1], r2
        pld             [r1]
        pld             [r1, r2]
  .if \avg
        vld1.8          {q8},     [r0,:128], r2
        vld1.8          {q9},     [r0,:128]
        vrhadd.u8       q2,  q2,  q8
        vrhadd.u8       q3,  q3,  q9
        sub             r0,  r0,  r2
  .endif
        vst1.8          {q2},     [r0,:128], r2
        vst1.8          {q3},     [r0,:128], r2
        bne             1b

        avg             q2,  q0,  q1,   \rnd
        vld1.8          {q0},     [r1], r2
        avg             q3,  q0,  q1,   \rnd
  .if \avg
        vld1.8          {q8},     [r0,:128], r2
        vld1.8          {q9},     [r0,:128]
        vrhadd.u8       q2,  q2,  q8
        vrhadd.u8       q3,  q3,  q9
        sub             r0,  r0,  r2
  .endif
        vst1.8          {q2},     [r0,:128], r2
        vst1.8          {q3},     [r0,:128], r2

        bx              lr
.endm

.macro  pixels16_xy2    rnd=1, avg=0, rnd=1
        sub             r3,  r3,  #2
        vld1.8          {d0-d2},  [r1], r2
        vld1.8          {d4-d6},  [r1], r2
.if \rnd == 0
        vmov.i16        q13, #1
.endif
        pld             [r1]
        pld             [r1, r2]
        vext.8          q1,  q0,  q1,  #1
        vext.8          q3,  q2,  q3,  #1
        vaddl.u8        q8,  d0,  d2
        vaddl.u8        q10, d1,  d3
        vaddl.u8        q9,  d4,  d6
        vaddl.u8        q11, d5,  d7
1:      subs            r3,  r3,  #2
        vld1.8          {d0-d2},  [r1], r2
        vadd.u16        q12, q8,  q9
        pld             [r1]
.if \rnd == 0
        vadd.u16        q12, q12, q13
.endif
        vext.8          q15, q0,  q1,  #1
        vadd.u16        q1 , q10, q11
        shrn            d28, q12, #2, \rnd
.if \rnd == 0
        vadd.u16        q1,  q1,  q13
.endif
        shrn            d29, q1,  #2, \rnd
  .if \avg
        vld1.8          {q8},     [r0,:128]
        vrhadd.u8       q14, q14, q8
  .endif
        vaddl.u8        q8,  d0,  d30
        vld1.8          {d2-d4},  [r1], r2
        vaddl.u8        q10, d1,  d31
        vst1.8          {q14},    [r0,:128], r2
        vadd.u16        q12, q8,  q9
        pld             [r1, r2]
.if \rnd == 0
        vadd.u16        q12, q12, q13
.endif
        vext.8          q2,  q1,  q2,  #1
        vadd.u16        q0,  q10, q11
        shrn            d30, q12, #2, \rnd
.if \rnd == 0
        vadd.u16        q0,  q0,  q13
.endif
        shrn            d31, q0,  #2, \rnd
  .if \avg
        vld1.8          {q9},     [r0,:128]
        vrhadd.u8       q15, q15, q9
  .endif
        vaddl.u8        q9,  d2,  d4
        vaddl.u8        q11, d3,  d5
        vst1.8          {q15},    [r0,:128], r2
        bgt             1b

        vld1.8          {d0-d2},  [r1], r2
        vadd.u16        q12, q8,  q9
.if \rnd == 0
        vadd.u16        q12, q12, q13
.endif
        vext.8          q15, q0,  q1,  #1
        vadd.u16        q1 , q10, q11
        shrn            d28, q12, #2, \rnd
.if \rnd == 0
        vadd.u16        q1,  q1,  q13
.endif
        shrn            d29, q1,  #2, \rnd
  .if \avg
        vld1.8          {q8},     [r0,:128]
        vrhadd.u8       q14, q14, q8
  .endif
        vaddl.u8        q8,  d0,  d30
        vaddl.u8        q10, d1,  d31
        vst1.8          {q14},    [r0,:128], r2
        vadd.u16        q12, q8,  q9
.if \rnd == 0
        vadd.u16        q12, q12, q13
.endif
        vadd.u16        q0,  q10, q11
        shrn            d30, q12, #2, \rnd
.if \rnd == 0
        vadd.u16        q0,  q0,  q13
.endif
        shrn            d31, q0,  #2, \rnd
  .if \avg
        vld1.8          {q9},     [r0,:128]
        vrhadd.u8       q15, q15, q9
  .endif
        vst1.8          {q15},    [r0,:128], r2

        bx              lr
.endm

.macro  pixels8         rnd=1, avg=0, rnd=1
1:      vld1.8          {d0},     [r1], r2
        vld1.8          {d1},     [r1], r2
        vld1.8          {d2},     [r1], r2
        pld             [r1, r2, lsl #2]
        vld1.8          {d3},     [r1], r2
        pld             [r1]
        pld             [r1, r2]
        pld             [r1, r2, lsl #1]
  .if \avg
        vld1.8          {d4},     [r0,:64], r2
        vrhadd.u8       d0,  d0,  d4
        vld1.8          {d5},     [r0,:64], r2
        vrhadd.u8       d1,  d1,  d5
        vld1.8          {d6},     [r0,:64], r2
        vrhadd.u8       d2,  d2,  d6
        vld1.8          {d7},     [r0,:64], r2
        vrhadd.u8       d3,  d3,  d7
        sub             r0,  r0,  r2,  lsl #2
  .endif
        subs            r3,  r3,  #4
        vst1.8          {d0},     [r0,:64], r2
        vst1.8          {d1},     [r0,:64], r2
        vst1.8          {d2},     [r0,:64], r2
        vst1.8          {d3},     [r0,:64], r2
        bne             1b
        bx              lr
.endm

.macro  pixels8_x2      rnd=1, avg=0, rnd=1
1:      vld1.8          {q0},     [r1], r2
        vext.8          d1,  d0,  d1,  #1
        vld1.8          {q1},     [r1], r2
        vext.8          d3,  d2,  d3,  #1
        pld             [r1]
        pld             [r1, r2]
        subs            r3,  r3,  #2
        vswp            d1,  d2
        avg             q0,  q0,  q1,  \rnd
  .if \avg
        vld1.8          {d4},     [r0,:64], r2
        vld1.8          {d5},     [r0,:64]
        vrhadd.u8       q0,  q0,  q2
        sub             r0,  r0,  r2
  .endif
        vst1.8          {d0},     [r0,:64], r2
        vst1.8          {d1},     [r0,:64], r2
        bne             1b
        bx              lr
.endm

.macro  pixels8_y2      rnd=1, avg=0, rnd=1
        sub             r3,  r3,  #2
        vld1.8          {d0},     [r1], r2
        vld1.8          {d1},     [r1], r2
1:      subs            r3,  r3,  #2
        avg             d4,  d0,  d1,   \rnd
        vld1.8          {d0},     [r1], r2
        avg             d5,  d0,  d1,   \rnd
        vld1.8          {d1},     [r1], r2
        pld             [r1]
        pld             [r1, r2]
  .if \avg
        vld1.8          {d2},     [r0,:64], r2
        vld1.8          {d3},     [r0,:64]
        vrhadd.u8       q2,  q2,  q1
        sub             r0,  r0,  r2
  .endif
        vst1.8          {d4},     [r0,:64], r2
        vst1.8          {d5},     [r0,:64], r2
        bne             1b

        avg             d4,  d0,  d1,   \rnd
        vld1.8          {d0},     [r1], r2
        avg             d5,  d0,  d1,   \rnd
  .if \avg
        vld1.8          {d2},     [r0,:64], r2
        vld1.8          {d3},     [r0,:64]
        vrhadd.u8       q2,  q2,  q1
        sub             r0,  r0,  r2
  .endif
        vst1.8          {d4},     [r0,:64], r2
        vst1.8          {d5},     [r0,:64], r2

        bx              lr
.endm

.macro  pixels8_xy2     rnd=1, avg=0, rnd=1
        sub             r3,  r3,  #2
        vld1.8          {q0},     [r1], r2
        vld1.8          {q1},     [r1], r2
.if \rnd == 0
        vmov.i16        q11, #1
.endif
        pld             [r1]
        pld             [r1, r2]
        vext.8          d4,  d0,  d1,  #1
        vext.8          d6,  d2,  d3,  #1
        vaddl.u8        q8,  d0,  d4
        vaddl.u8        q9,  d2,  d6
1:      subs            r3,  r3,  #2
        vld1.8          {q0},     [r1], r2
        pld             [r1]
        vadd.u16        q10, q8,  q9
        vext.8          d4,  d0,  d1,  #1
.if \rnd == 0
        vadd.u16        q10, q10, q11
.endif
        vaddl.u8        q8,  d0,  d4
        shrn            d5,  q10, #2, \rnd
        vld1.8          {q1},     [r1], r2
        vadd.u16        q10, q8,  q9
        pld             [r1, r2]
  .if \avg
        vld1.8          {d7},     [r0,:64]
        vrhadd.u8       d5,  d5,  d7
  .endif
.if \rnd == 0
        vadd.u16        q10, q10, q11
.endif
        vst1.8          {d5},     [r0,:64], r2
        shrn            d7,  q10, #2, \rnd
  .if \avg
        vld1.8          {d5},     [r0,:64]
        vrhadd.u8       d7,  d7,  d5
  .endif
        vext.8          d6,  d2,  d3,  #1
        vaddl.u8        q9,  d2,  d6
        vst1.8          {d7},     [r0,:64], r2
        bgt             1b

        vld1.8          {q0},     [r1], r2
        vadd.u16        q10, q8,  q9
        vext.8          d4,  d0,  d1,  #1
.if \rnd == 0
        vadd.u16        q10, q10, q11
.endif
        vaddl.u8        q8,  d0,  d4
        shrn            d5,  q10, #2, \rnd
        vadd.u16        q10, q8,  q9
  .if \avg
        vld1.8          {d7},     [r0,:64]
        vrhadd.u8       d5,  d5,  d7
  .endif
.if \rnd == 0
        vadd.u16        q10, q10, q11
.endif
        vst1.8          {d5},     [r0,:64], r2
        shrn            d7,  q10, #2, \rnd
  .if \avg
        vld1.8          {d5},     [r0,:64]
        vrhadd.u8       d7,  d7,  d5
  .endif
        vst1.8          {d7},     [r0,:64], r2

        bx              lr
.endm

.macro avg  rd, rn, rm, rnd
.if \rnd
    vrhadd.u8       \rd, \rn, \rm
.else
    vhadd.u8        \rd, \rn, \rm
.endif
.endm
.macro shrn rd, rn, rm, rnd
.if \rnd
    vrshrn.u16      \rd, \rn, \rm
.else
    vshrn.u16       \rd, \rn, \rm
.endif
.endm

.macro  pixfunc         pfx, name, suf, rnd=1, avg=0
function tt_\pfx\name\suf\()_neon, 1
        \name           \rnd, \avg, \rnd
endfunc
@        .purgem         avg
@        .purgem         shrn
@        .purgem         NRND
.endm

.macro  pixfunc2        pfx, name, avg=0
        pixfunc         \pfx, \name,   ""   , 1, \avg
        pixfunc         \pfx, \name, _no_rnd, 0, \avg
.endm

function tt_put_h264_qpel16_mc00_neon, 1
        mov             r3,  #16
endfunc

        pixfunc         put_, pixels16,     0
        pixfunc2        put_, pixels16_x2,  0
        pixfunc2        put_, pixels16_y2,  0
        pixfunc2        put_, pixels16_xy2, 0

function tt_avg_h264_qpel16_mc00_neon, 1
        mov             r3,  #16
endfunc

        pixfunc         avg_, pixels16, "", 1,  1
        pixfunc2        avg_, pixels16_x2,  1
        pixfunc2        avg_, pixels16_y2,  1
        pixfunc2        avg_, pixels16_xy2, 1

function tt_put_h264_qpel8_mc00_neon, 1
        mov             r3,  #8
endfunc

        pixfunc         put_, pixels8, "", 1, 0
        pixfunc2        put_, pixels8_x2,  0
        pixfunc2        put_, pixels8_y2,  0
        pixfunc2        put_, pixels8_xy2, 0

function tt_avg_h264_qpel8_mc00_neon, 1
        mov             r3,  #8
endfunc

        pixfunc         avg_, pixels8, "", 1, 1
        pixfunc         avg_, pixels8_x2,  1
        pixfunc         avg_, pixels8_y2,  1
        pixfunc         avg_, pixels8_xy2, 1
#endif
