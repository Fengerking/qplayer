
#include <stdint.h>

#include "ttAttributes.h"
#include "arm/cpu.h"
#include "ttH264dsp.h"

void tt_h264_v_loop_filter_luma_neon(uint8_t *pix, int stride, int alpha,
                                     int beta, int8_t *tc0);
void tt_h264_h_loop_filter_luma_neon(uint8_t *pix, int stride, int alpha,
                                     int beta, int8_t *tc0);
void tt_h264_v_loop_filter_chroma_neon(uint8_t *pix, int stride, int alpha,
                                       int beta, int8_t *tc0);
void tt_h264_h_loop_filter_chroma_neon(uint8_t *pix, int stride, int alpha,
                                       int beta, int8_t *tc0);

void tt_weight_h264_pixels_16_neon(uint8_t *dst, int stride, int height,
                                   int log2_den, int weight, int offset);
void tt_weight_h264_pixels_8_neon(uint8_t *dst, int stride, int height,
                                  int log2_den, int weight, int offset);
void tt_weight_h264_pixels_4_neon(uint8_t *dst, int stride, int height,
                                  int log2_den, int weight, int offset);

void tt_biweight_h264_pixels_16_neon(uint8_t *dst, uint8_t *src, int stride,
                                     int height, int log2_den, int weightd,
                                     int weights, int offset);
void tt_biweight_h264_pixels_8_neon(uint8_t *dst, uint8_t *src, int stride,
                                    int height, int log2_den, int weightd,
                                    int weights, int offset);
void tt_biweight_h264_pixels_4_neon(uint8_t *dst, uint8_t *src, int stride,
                                    int height, int log2_den, int weightd,
                                    int weights, int offset);

void tt_h264_idct_add_neon(uint8_t *dst, int16_t *block, int stride);
void tt_h264_idct_dc_add_neon(uint8_t *dst, int16_t *block, int stride);
void tt_h264_idct_add16_neon(uint8_t *dst, const int *block_offset,
                             int16_t *block, int stride,
                             const uint8_t nnzc[6*8]);
void tt_h264_idct_add16intra_neon(uint8_t *dst, const int *block_offset,
                                  int16_t *block, int stride,
                                  const uint8_t nnzc[6*8]);
void tt_h264_idct_add8_neon(uint8_t **dest, const int *block_offset,
                            int16_t *block, int stride,
                            const uint8_t nnzc[6*8]);

void tt_h264_idct8_add_neon(uint8_t *dst, int16_t *block, int stride);
void tt_h264_idct8_dc_add_neon(uint8_t *dst, int16_t *block, int stride);
void tt_h264_idct8_add4_neon(uint8_t *dst, const int *block_offset,
                             int16_t *block, int stride,
                             const uint8_t nnzc[6*8]);

static ttv_cold void h264dsp_init_neon(H264DSPContext *c, const int bit_depth,
                                      const int chroma_format_idc)
{
#if HAVE_NEON
    if (bit_depth == 8) {
        c->h264_v_loop_filter_luma   = tt_h264_v_loop_filter_luma_neon;
        c->h264_h_loop_filter_luma   = tt_h264_h_loop_filter_luma_neon;
        if(chroma_format_idc == 1){
        c->h264_v_loop_filter_chroma = tt_h264_v_loop_filter_chroma_neon;
        c->h264_h_loop_filter_chroma = tt_h264_h_loop_filter_chroma_neon;
        }

        c->weight_h264_pixels_tab[0] = tt_weight_h264_pixels_16_neon;
        c->weight_h264_pixels_tab[1] = tt_weight_h264_pixels_8_neon;
        c->weight_h264_pixels_tab[2] = tt_weight_h264_pixels_4_neon;

        c->biweight_h264_pixels_tab[0] = tt_biweight_h264_pixels_16_neon;
        c->biweight_h264_pixels_tab[1] = tt_biweight_h264_pixels_8_neon;
        c->biweight_h264_pixels_tab[2] = tt_biweight_h264_pixels_4_neon;

        c->h264_idct_add        = tt_h264_idct_add_neon;
        c->h264_idct_dc_add     = tt_h264_idct_dc_add_neon;
        c->h264_idct_add16      = tt_h264_idct_add16_neon;
        c->h264_idct_add16intra = tt_h264_idct_add16intra_neon;
        if (chroma_format_idc <= 1)
            c->h264_idct_add8   = tt_h264_idct_add8_neon;
        c->h264_idct8_add       = tt_h264_idct8_add_neon;
        c->h264_idct8_dc_add    = tt_h264_idct8_dc_add_neon;
        c->h264_idct8_add4      = tt_h264_idct8_add4_neon;
    }
#endif // HAVE_NEON
}

ttv_cold void tt_h264dsp_init_arm(H264DSPContext *c, const int bit_depth,
                                 const int chroma_format_idc)
{
    int cpu_flags = ttv_get_cpu_flags();

// #if HAVE_ARMV6
//     if (have_setend(cpu_flags))
//         c->startcode_find_candidate = tt_startcode_find_candidate_armv6;
// #endif
    if (have_neon(cpu_flags))
        h264dsp_init_neon(c, bit_depth, chroma_format_idc);
}
