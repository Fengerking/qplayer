#include "config.h"
#include "ttAttributes.h"
#include "ttH264chroma.h"

#define BIT_DEPTH 8
#include "ttH264chromaTemplate.c"
#undef BIT_DEPTH

// #define BIT_DEPTH 16
// #include "ttH264chromaTemplate.c"
// #undef BIT_DEPTH

#define SET_CHROMA(depth)                                                   \
    c->put_h264_chroma_pixels_tab[0] = put_h264_chroma_mc8_ ## depth ## _c; \
    c->put_h264_chroma_pixels_tab[1] = put_h264_chroma_mc4_ ## depth ## _c; \
    c->put_h264_chroma_pixels_tab[2] = put_h264_chroma_mc2_ ## depth ## _c; \
    c->put_h264_chroma_pixels_tab[3] = put_h264_chroma_mc1_ ## depth ## _c; \
    c->avg_h264_chroma_pixels_tab[0] = avg_h264_chroma_mc8_ ## depth ## _c; \
    c->avg_h264_chroma_pixels_tab[1] = avg_h264_chroma_mc4_ ## depth ## _c; \
    c->avg_h264_chroma_pixels_tab[2] = avg_h264_chroma_mc2_ ## depth ## _c; \
    c->avg_h264_chroma_pixels_tab[3] = avg_h264_chroma_mc1_ ## depth ## _c; \

ttv_cold void tt_h264chroma_init(H264ChromaContext *c, int bit_depth)
{
    if (bit_depth > 8 && bit_depth <= 16) {
     //   SET_CHROMA(16);
    } else {
        SET_CHROMA(8);
    }
#if ARCH_AARCH64
    if (ARCH_AARCH64)
        tt_h264chroma_init_aarch64(c, bit_depth);
#endif
#if ARCH_ARM
    if (ARCH_ARM)
        tt_h264chroma_init_arm(c, bit_depth);
#endif
#if ARCH_PPC
    if (ARCH_PPC)
        tt_h264chroma_init_ppc(c, bit_depth);
#endif
#if ARCH_X86
    if (ARCH_X86)
        tt_h264chroma_init_x86(c, bit_depth);
#endif
}
