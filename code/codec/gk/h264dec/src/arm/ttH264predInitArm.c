
#include <stdint.h>

#include "ttAttributes.h"
#include "arm/cpu.h"
#include "ttAvcodec.h"
#include "ttH264pred.h"

void tt_pred16x16_vert_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred16x16_hor_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred16x16_plane_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred16x16_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred16x16_128_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred16x16_left_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred16x16_top_dc_neon(uint8_t *src, ptrdiff_t stride);

void tt_pred8x8_vert_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_hor_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_plane_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_128_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_left_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_top_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_l0t_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_0lt_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_l00_dc_neon(uint8_t *src, ptrdiff_t stride);
void tt_pred8x8_0l0_dc_neon(uint8_t *src, ptrdiff_t stride);

static ttv_cold void h264_pred_init_neon(H264PredContext *h, int codec_id,
                                        const int bit_depth,
                                        const int chroma_format_idc)
{
#if HAVE_NEON
    const int high_depth = bit_depth > 8;

    if (high_depth)
        return;
    if(chroma_format_idc == 1){
    h->pred8x8[VERT_PRED8x8     ] = tt_pred8x8_vert_neon;
    h->pred8x8[HOR_PRED8x8      ] = tt_pred8x8_hor_neon;
    if (codec_id != TTV_CODEC_ID_VP7 && codec_id != TTV_CODEC_ID_VP8)
        h->pred8x8[PLANE_PRED8x8] = tt_pred8x8_plane_neon;
    h->pred8x8[DC_128_PRED8x8   ] = tt_pred8x8_128_dc_neon;
    if (codec_id != TTV_CODEC_ID_RV40 && codec_id != TTV_CODEC_ID_VP7 &&
        codec_id != TTV_CODEC_ID_VP8) {
        h->pred8x8[DC_PRED8x8     ] = tt_pred8x8_dc_neon;
        h->pred8x8[LEFT_DC_PRED8x8] = tt_pred8x8_left_dc_neon;
        h->pred8x8[TOP_DC_PRED8x8 ] = tt_pred8x8_top_dc_neon;
        h->pred8x8[ALZHEIMER_DC_L0T_PRED8x8] = tt_pred8x8_l0t_dc_neon;
        h->pred8x8[ALZHEIMER_DC_0LT_PRED8x8] = tt_pred8x8_0lt_dc_neon;
        h->pred8x8[ALZHEIMER_DC_L00_PRED8x8] = tt_pred8x8_l00_dc_neon;
        h->pred8x8[ALZHEIMER_DC_0L0_PRED8x8] = tt_pred8x8_0l0_dc_neon;
    }
    }

    h->pred16x16[DC_PRED8x8     ] = tt_pred16x16_dc_neon;
    h->pred16x16[VERT_PRED8x8   ] = tt_pred16x16_vert_neon;
    h->pred16x16[HOR_PRED8x8    ] = tt_pred16x16_hor_neon;
    h->pred16x16[LEFT_DC_PRED8x8] = tt_pred16x16_left_dc_neon;
    h->pred16x16[TOP_DC_PRED8x8 ] = tt_pred16x16_top_dc_neon;
    h->pred16x16[DC_128_PRED8x8 ] = tt_pred16x16_128_dc_neon;
    if (codec_id != TTV_CODEC_ID_SVQ3 && codec_id != TTV_CODEC_ID_RV40 &&
        codec_id != TTV_CODEC_ID_VP7 && codec_id != TTV_CODEC_ID_VP8)
        h->pred16x16[PLANE_PRED8x8  ] = tt_pred16x16_plane_neon;
#endif // HAVE_NEON
}

ttv_cold void tt_h264_pred_init_arm(H264PredContext *h, int codec_id,
                                   int bit_depth, const int chroma_format_idc)
{
    int cpu_flags = ttv_get_cpu_flags();

    if (have_neon(cpu_flags))
        h264_pred_init_neon(h, codec_id, bit_depth, chroma_format_idc);
}
