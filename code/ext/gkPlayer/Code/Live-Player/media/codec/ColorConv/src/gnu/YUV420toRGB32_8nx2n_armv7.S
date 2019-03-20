	@AREA    |.text|, CODE, READONLY
	.section .text
	
	.global colorConNxN_neon
  
colorConNxN_neon: @PROC
@(psrc_y, psrc_u, psrc_v, in_stride, out_buf, out_stride, width, height, uin_stride, vin_stride)
	stmdb	sp!, {r4-r11, lr}	  @ save regs used
	
	@sub	sp, sp, #64				  @ sp - 32

	ldr	r6, [sp, #36]			@out_buf
	ldr	r7, [sp, #40]			@out_stride
	ldr	r8, [sp, #44]			@width
	ldr	r5, [sp, #48]			@height
	ldr	r4, [sp, #52]			@in_strideu
	ldr	r9, [sp, #56]			@in_stridev  no use
	
	adr   lr, coefficients
	vld1.s16        {d6[],  d7[]}, [lr]!
	vld1.s16        {d8[],  d9[]}, [lr]!
	vld1.s16        {d10[], d11[]}, [lr]!
	
 vmov.u8        	d0, #74 
  vmov.u8			d1, #102   
  vmov.u8        	d2, #25 
  vmov.u8         	d3, #52 
  vmov.u8			d4, #129
ARGB32_Height_loop_new: 
  add             ip, r0, r3 @ip y_b_ptr
  add             fp, r6, r7 @fp out_b_ptr
  mov             r10, r8     @s1 = with
	
  ands            r9, r8, #15 @count
  moveq           r9, #16
ARGB32_Width_loop_new:
	@ Load u and v
  vld1.u8         d17, [r2]      @v
  add             r2, r9, asr #1 @ v_ptr + 8
  vld1.u8         d16, [r1]      @u
  add             r1, r9, asr #1 @ u_ptr + 8

	@Calculate contribution from chrominance signals.
	@R = CLAMP((Y-16)*1.164 + 1.596*V)
	@G = CLAMP((Y-16)*1.164 - 0.391*U - 0.813*V) 
	@B = CLAMP((Y-16)*1.164 + 2.018*U )
  vmull.u8        q6, d17, d1   @1.596*V
  vmull.u8        q7, d16, d2   @0.391*U
  vmlal.u8        q7, d17, d3   @0.813*V
  vmull.u8        q8, d16, d4   @2.018*U
  
  @Add bias
  vadd.s16        q6, q6, q3
  vsub.s16        q7, q4, q7
  vadd.s16        q8, q8, q5
  
  pld             [r2] @pre load V
	pld             [r1] @pre load U
	
	@-----------Y TOP ROW even
	vld2.u8         {d24, d26}, [r0], r9  @d24: y even | d26: y odd
	vmull.u8        q15, d24, d0          @1.164*y  
	   
  vqadd.s16       q9,  q15, q6     @r
  vqadd.s16       q10, q15, q7     @g
  vqadd.s16       q11, q15, q8     @b
  @alpha 
  vmov.u8         d29, #0xff         @a
  
  vqrshrun.s16    d24, q9,  #6
  vqrshrun.s16    d28, q10, #6
  vqrshrun.s16    d25, q11, #6	
  pld             [r0]	
  vzip.u8         d25, d29   @
  vzip.u8         d24, d28   @

@Y TOP ROW odd
  vmull.u8        q15, d26, d0
  vqadd.s16       q9 , q15, q6
  vqadd.s16       q10, q15, q7
  vqadd.s16       q11, q15, q8
  vmov.u8         d31, #0xff
  
  vqrshrun.s16    d26, q9,  #6
  vqrshrun.s16    d27, q11, #6
  vqrshrun.s16    d30, q10, #6
  
  vzip.u8         d27, d31
  vzip.u8         d26, d30
	
	@ Top row: Store 16 pixels of ARGB32, interleaving even and odd
  vst4.u16        {d24, d25, d26, d27}, [r6]  @rgb_t_ptr
  add             r6, r9, lsl #1
  vst4.u16        {d28, d29, d30, d31}, [r6]
  add             r6, r9, lsl #1  
  
  @----------Y BOTTOM ROW
  vld2.u8        {d24, d26}, [ip], r9

  @ Bottom row, even
  vmull.u8        q15, d24, d0        
  vqadd.s16       q9,  q15, q6
  vqadd.s16       q10, q15, q7
  vqadd.s16       q11, q15, q8
  vmov.u8         d29, #0xff
  vqrshrun.s16    d24, q9,  #6
  vqrshrun.s16    d25, q11, #6
  vqrshrun.s16    d28, q10, #6
  pld             [ip]
  vzip.u8         d25, d29
	vzip.u8         d24, d28
	
	@ Bottom row, odd
  vmull.u8        q15, d26, d0        
  vqadd.s16       q9 , q15, q6
  vqadd.s16       q10, q15, q7
  vqadd.s16       q11, q15, q8
  vmov.u8         d31, #0xff
  
  vqrshrun.s16    d26, q9 , #6
  vqrshrun.s16    d27, q11, #6
  vqrshrun.s16    d30, q10, #6  
  
  vzip.u8         d27, d31
  vzip.u8         d26, d30
  
  subs            r10, r10, r9
  vst4.u16        {d24, d25, d26, d27}, [fp]   @rgb_b_ptr
  add             fp, r9, lsl #1                      
	vst4.u16       {d28, d29, d30, d31}, [fp] 
	add             fp, r9, lsl #1  
	
	mov             r9, #16
	bgt             ARGB32_Width_loop_new
	
	sub             r6, r8, lsl #2  		@rgb_t_ptr
  sub             r0, r8             @
  sub             r1, r8, asr #1
  sub             r2, r8, asr #1
  add             r6, r7, lsl #1
  add             r0, r3, lsl #1
  add             r1, r4
  add             r2, r4
  
  @Have we reached the bottom row yet?
  subs            r5, r5, #2
  bgt             ARGB32_Height_loop_new
	
  ldmia	sp!, {r4-r11, pc}		@ restore and return 
  
  	.ALIGN 8
coefficients:
    .short  -14240  @/* bias_r = 74 * (-16)                + (102 * -128) */
    .short    8672  @/* bias_g = 74 * (-16) -  25 * (-128) - ( 52 * -128) */
    .short  -17696  @/* bias_b = 74 * (-16) + 129 * (-128)                */
    .short       0
	
.END
