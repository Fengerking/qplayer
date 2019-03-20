#ifndef __TTPOD_TT_H264CHROMA_H
#define __TTPOD_TT_H264CHROMA_H

#include <stdint.h>

typedef void (*h264_chroma_mc_func)(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int srcStride, int h, int x, int y);

typedef struct H264ChromaContext {
    h264_chroma_mc_func put_h264_chroma_pixels_tab[4];
    h264_chroma_mc_func avg_h264_chroma_pixels_tab[4];
} H264ChromaContext;

void tt_h264chroma_init(H264ChromaContext *c, int bit_depth);

void tt_h264chroma_init_aarch64(H264ChromaContext *c, int bit_depth);
void tt_h264chroma_init_arm(H264ChromaContext *c, int bit_depth);
void tt_h264chroma_init_ppc(H264ChromaContext *c, int bit_depth);
void tt_h264chroma_init_x86(H264ChromaContext *c, int bit_depth);

#endif /* __TTPOD_TT_H264CHROMA_H */
