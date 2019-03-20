#ifndef __TTPOD_TT_H264QPEL_H
#define __TTPOD_TT_H264QPEL_H

#include <stdint.h>
#include "config.h"
#include <stddef.h>
typedef void (*qpel_mc_func)(uint8_t *dst /* align width (8 or 16) */,
							 const uint8_t *src /* align 1 */,
							 ptrdiff_t stride);


typedef struct H264QpelContext {
    qpel_mc_func put_h264_qpel_pixels_tab[4][16];
    qpel_mc_func avg_h264_qpel_pixels_tab[4][16];
} H264QpelContext;

void tt_h264qpel_init(H264QpelContext *c, int bit_depth);

void tt_h264qpel_init_aarch64(H264QpelContext *c, int bit_depth);
void tt_h264qpel_init_arm(H264QpelContext *c, int bit_depth);
void tt_h264qpel_init_ppc(H264QpelContext *c, int bit_depth);
void tt_h264qpel_init_x86(H264QpelContext *c, int bit_depth);

#define BYTE_VEC32(c) ((c) * 0x01010101UL)

static inline uint32_t rnd_avg32(uint32_t a, uint32_t b)
{
	return (a | b) - (((a ^ b) & ~BYTE_VEC32(0x01)) >> 1);
}

#endif /* __TTPOD_TT_H264QPEL_H */
