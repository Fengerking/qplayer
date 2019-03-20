#ifndef __TTPOD_TT_H264PRED_H
#define __TTPOD_TT_H264PRED_H

#include <stddef.h>
#include <stdint.h>

/**
 * Prediction types
 */
//@{
#define VERT_PRED              0
#define HOR_PRED               1
#define DC_PRED                2
#define DIAG_DOWN_LEFT_PRED    3
#define DIAG_DOWN_RIGHT_PRED   4
#define VERT_RIGHT_PRED        5
#define HOR_DOWN_PRED          6
#define VERT_LEFT_PRED         7
#define HOR_UP_PRED            8

// DC edge (not for VP8)
#define LEFT_DC_PRED           9
#define TOP_DC_PRED           10
#define DC_128_PRED           11

// RV40 specific
#define DIAG_DOWN_LEFT_PRED_RV40_NODOWN   12
#define HOR_UP_PRED_RV40_NODOWN           13
#define VERT_LEFT_PRED_RV40_NODOWN        14

// VP8 specific
#define TM_VP8_PRED            9    ///< "True Motion", used instead of plane
#define VERT_VP8_PRED         10    ///< for VP8, #VERT_PRED is the average of
                                    ///< (left col+cur col x2+right col) / 4;
                                    ///< this is the "unaveraged" one
#define HOR_VP8_PRED          14    ///< unaveraged version of #HOR_PRED, see
                                    ///< #VERT_VP8_PRED for details
#define DC_127_PRED           12
#define DC_129_PRED           13

#define DC_PRED8x8             0
#define HOR_PRED8x8            1
#define VERT_PRED8x8           2
#define PLANE_PRED8x8          3

// DC edge
#define LEFT_DC_PRED8x8        4
#define TOP_DC_PRED8x8         5
#define DC_128_PRED8x8         6

// H264/SVQ3 (8x8) specific
#define ALZHEIMER_DC_L0T_PRED8x8  7
#define ALZHEIMER_DC_0LT_PRED8x8  8
#define ALZHEIMER_DC_L00_PRED8x8  9
#define ALZHEIMER_DC_0L0_PRED8x8 10

// VP8 specific
#define DC_127_PRED8x8         7
#define DC_129_PRED8x8         8
//@}

/**
 * Context for storing H.264 prediction functions
 */
typedef struct H264PredContext {
    void(*pred4x4[9 + 3 + 3])(uint8_t *src, const uint8_t *topright,
                              ptrdiff_t stride);
    void(*pred8x8l[9 + 3])(uint8_t *src, int topleft, int topright,
                           ptrdiff_t stride);
    void(*pred8x8[4 + 3 + 4])(uint8_t *src, ptrdiff_t stride);
    void(*pred16x16[4 + 3 + 2])(uint8_t *src, ptrdiff_t stride);

    void(*pred4x4_add[2])(uint8_t *pix /*align  4*/,
                          int16_t *block /*align 16*/, ptrdiff_t stride);
    void(*pred8x8l_add[2])(uint8_t *pix /*align  8*/,
                           int16_t *block /*align 16*/, ptrdiff_t stride);
    void(*pred8x8l_filter_add[2])(uint8_t *pix /*align  8*/,
                           int16_t *block /*align 16*/, int topleft, int topright, ptrdiff_t stride);
    void(*pred8x8_add[3])(uint8_t *pix /*align  8*/,
                          const int *block_offset,
                          int16_t *block /*align 16*/, ptrdiff_t stride);
    void(*pred16x16_add[3])(uint8_t *pix /*align 16*/,
                            const int *block_offset,
                            int16_t *block /*align 16*/, ptrdiff_t stride);
} H264PredContext;

void tt_h264_pred_init(H264PredContext *h, int codec_id,
                       const int bit_depth, const int chroma_format_idc);
void tt_h264_pred_init_arm(H264PredContext *h, int codec_id,
                           const int bit_depth, const int chroma_format_idc);
void tt_h264_pred_init_x86(H264PredContext *h, int codec_id,
                           const int bit_depth, const int chroma_format_idc);

#endif /* __TTPOD_TT_H264PRED_H */
