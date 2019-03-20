#include <stdint.h>

#include "ttAttributes.h"
#include "ttAvassert.h"

#include "ttAvcodec.h"
#include "ttH264dsp.h"
#include "ttH264idct.h"
#include "ttCommon.h"

#define BIT_DEPTH 8
#include "ttH264dspTemplate.c"
#undef BIT_DEPTH

// #define BIT_DEPTH 9
// #include "ttH264dspTemplate.c"
// #undef BIT_DEPTH
// 
// #define BIT_DEPTH 10
// #include "ttH264dspTemplate.c"
// #undef BIT_DEPTH
// 
// #define BIT_DEPTH 12
// #include "ttH264dspTemplate.c"
// #undef BIT_DEPTH
// 
// #define BIT_DEPTH 14
// #include "ttH264dspTemplate.c"
// #undef BIT_DEPTH

#define BIT_DEPTH 8
#include "ttH264addpxTemplate.c"
#undef BIT_DEPTH

// #define BIT_DEPTH 16
// #include "ttH264addpxTemplate.c"
// #undef BIT_DEPTH

ttv_cold void tt_h264dsp_init(H264DSPContext *c, const int bit_depth,
                             const int chroma_format_idc)
{
#undef FUNC
#define FUNC(a, depth) a ## _ ## depth ## _c

#define ADDPX_DSP(depth) \
    c->h264_add_pixels4_clear = FUNC(tt_h264_add_pixels4, depth);\
    c->h264_add_pixels8_clear = FUNC(tt_h264_add_pixels8, depth)

    if (bit_depth > 8 && bit_depth <= 16) {
 //       ADDPX_DSP(16);
    } else {
        ADDPX_DSP(8);
    }

#define H264_DSP(depth) \
    c->h264_idct_add= FUNC(tt_h264_idct_add, depth);\
    c->h264_idct8_add= FUNC(tt_h264_idct8_add, depth);\
    c->h264_idct_dc_add= FUNC(tt_h264_idct_dc_add, depth);\
    c->h264_idct8_dc_add= FUNC(tt_h264_idct8_dc_add, depth);\
    c->h264_idct_add16     = FUNC(tt_h264_idct_add16, depth);\
    c->h264_idct8_add4     = FUNC(tt_h264_idct8_add4, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_idct_add8  = FUNC(tt_h264_idct_add8, depth);\
    else\
        c->h264_idct_add8  = FUNC(tt_h264_idct_add8_422, depth);\
    c->h264_idct_add16intra= FUNC(tt_h264_idct_add16intra, depth);\
    c->h264_luma_dc_dequant_idct= FUNC(tt_h264_luma_dc_dequant_idct, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_chroma_dc_dequant_idct= FUNC(tt_h264_chroma_dc_dequant_idct, depth);\
    else\
        c->h264_chroma_dc_dequant_idct= FUNC(tt_h264_chroma422_dc_dequant_idct, depth);\
\
    c->weight_h264_pixels_tab[0]= FUNC(weight_h264_pixels16, depth);\
    c->weight_h264_pixels_tab[1]= FUNC(weight_h264_pixels8, depth);\
    c->weight_h264_pixels_tab[2]= FUNC(weight_h264_pixels4, depth);\
    c->weight_h264_pixels_tab[3]= FUNC(weight_h264_pixels2, depth);\
    c->biweight_h264_pixels_tab[0]= FUNC(biweight_h264_pixels16, depth);\
    c->biweight_h264_pixels_tab[1]= FUNC(biweight_h264_pixels8, depth);\
    c->biweight_h264_pixels_tab[2]= FUNC(biweight_h264_pixels4, depth);\
    c->biweight_h264_pixels_tab[3]= FUNC(biweight_h264_pixels2, depth);\
\
    c->h264_v_loop_filter_luma= FUNC(h264_v_loop_filter_luma, depth);\
    c->h264_h_loop_filter_luma= FUNC(h264_h_loop_filter_luma, depth);\
    c->h264_h_loop_filter_luma_mbaff= FUNC(h264_h_loop_filter_luma_mbaff, depth);\
    c->h264_v_loop_filter_luma_intra= FUNC(h264_v_loop_filter_luma_intra, depth);\
    c->h264_h_loop_filter_luma_intra= FUNC(h264_h_loop_filter_luma_intra, depth);\
    c->h264_h_loop_filter_luma_mbatt_intra= FUNC(h264_h_loop_filter_luma_mbatt_intra, depth);\
    c->h264_v_loop_filter_chroma= FUNC(h264_v_loop_filter_chroma, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_h_loop_filter_chroma= FUNC(h264_h_loop_filter_chroma, depth);\
    else\
        c->h264_h_loop_filter_chroma= FUNC(h264_h_loop_filter_chroma422, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_h_loop_filter_chroma_mbaff= FUNC(h264_h_loop_filter_chroma_mbaff, depth);\
    else\
        c->h264_h_loop_filter_chroma_mbaff= FUNC(h264_h_loop_filter_chroma422_mbaff, depth);\
    c->h264_v_loop_filter_chroma_intra= FUNC(h264_v_loop_filter_chroma_intra, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_h_loop_filter_chroma_intra= FUNC(h264_h_loop_filter_chroma_intra, depth);\
    else\
        c->h264_h_loop_filter_chroma_intra= FUNC(h264_h_loop_filter_chroma422_intra, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_h_loop_filter_chroma_mbatt_intra= FUNC(h264_h_loop_filter_chroma_mbatt_intra, depth);\
    else\
        c->h264_h_loop_filter_chroma_mbatt_intra= FUNC(h264_h_loop_filter_chroma422_mbatt_intra, depth);\
    c->h264_loop_filter_strength= NULL;

    switch (bit_depth) {
    case 9:
//        H264_DSP(9);
        break;
//     case 10:
//         H264_DSP(10);
//         break;
//     case 12:
//         H264_DSP(12);
//         break;
//     case 14:
//         H264_DSP(14);
//         break;
    default:
        ttv_assert0(bit_depth<=8);
        H264_DSP(8);
        break;
    }
//    c->startcode_find_candidate = tt_startcode_find_candidate_c;
#if ARCH_AARCH64
    if (ARCH_AARCH64) tt_h264dsp_init_aarch64(c, bit_depth, chroma_format_idc);
#endif
#if ARCH_ARM
    if (ARCH_ARM) tt_h264dsp_init_arm(c, bit_depth, chroma_format_idc);
#endif
#if ARCH_PPC
    if (ARCH_PPC) tt_h264dsp_init_ppc(c, bit_depth, chroma_format_idc);
#endif
#if ARCH_X86
    if (ARCH_X86) tt_h264dsp_init_x86(c, bit_depth, chroma_format_idc);
#endif
}
