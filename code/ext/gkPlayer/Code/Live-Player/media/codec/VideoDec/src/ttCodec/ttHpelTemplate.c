
#include "ttIntreadwrite.h"

#include "ttPixels.h"

#include "ttBitDepthTemplate.c"

#define DEF_HPEL(OPNAME, OP)                                            \
static inline void FUNC(OPNAME ## _pixels8_l2)(uint8_t *dst,            \
                                               const uint8_t *src1,     \
                                               const uint8_t *src2,     \
                                               int dst_stride,          \
                                               int src_stride1,         \
                                               int src_stride2,         \
                                               int h)                   \
{                                                                       \
    int i;                                                              \
    for (i = 0; i < h; i++) {                                           \
        pixel4 a, b;                                                    \
        a = TTV_RN4P(&src1[i * src_stride1]);                            \
        b = TTV_RN4P(&src2[i * src_stride2]);                            \
        OP(*((pixel4 *) &dst[i * dst_stride]), rnd_avg_pixel4(a, b));   \
        a = TTV_RN4P(&src1[i * src_stride1 + 4 * sizeof(pixel)]);        \
        b = TTV_RN4P(&src2[i * src_stride2 + 4 * sizeof(pixel)]);        \
        OP(*((pixel4 *) &dst[i * dst_stride + 4 * sizeof(pixel)]),      \
           rnd_avg_pixel4(a, b));                                       \
    }                                                                   \
}                                                                       \
                                                                        \
static inline void FUNC(OPNAME ## _pixels4_l2)(uint8_t *dst,            \
                                               const uint8_t *src1,     \
                                               const uint8_t *src2,     \
                                               int dst_stride,          \
                                               int src_stride1,         \
                                               int src_stride2,         \
                                               int h)                   \
{                                                                       \
    int i;                                                              \
    for (i = 0; i < h; i++) {                                           \
        pixel4 a, b;                                                    \
        a = TTV_RN4P(&src1[i * src_stride1]);                            \
        b = TTV_RN4P(&src2[i * src_stride2]);                            \
        OP(*((pixel4 *) &dst[i * dst_stride]), rnd_avg_pixel4(a, b));   \
    }                                                                   \
}                                                                       \
                                                                        \
static inline void FUNC(OPNAME ## _pixels2_l2)(uint8_t *dst,            \
                                               const uint8_t *src1,     \
                                               const uint8_t *src2,     \
                                               int dst_stride,          \
                                               int src_stride1,         \
                                               int src_stride2,         \
                                               int h)                   \
{                                                                       \
    int i;                                                              \
    for (i = 0; i < h; i++) {                                           \
        pixel4 a, b;                                                    \
        a = TTV_RN2P(&src1[i * src_stride1]);                            \
        b = TTV_RN2P(&src2[i * src_stride2]);                            \
        OP(*((pixel2 *) &dst[i * dst_stride]), rnd_avg_pixel4(a, b));   \
    }                                                                   \
}                                                                       \
                                                                        \
static inline void FUNC(OPNAME ## _pixels16_l2)(uint8_t *dst,           \
                                                const uint8_t *src1,    \
                                                const uint8_t *src2,    \
                                                int dst_stride,         \
                                                int src_stride1,        \
                                                int src_stride2,        \
                                                int h)                  \
{                                                                       \
    FUNC(OPNAME ## _pixels8_l2)(dst, src1, src2, dst_stride,            \
                                src_stride1, src_stride2, h);           \
    FUNC(OPNAME ## _pixels8_l2)(dst  + 8 * sizeof(pixel),               \
                                src1 + 8 * sizeof(pixel),               \
                                src2 + 8 * sizeof(pixel),               \
                                dst_stride, src_stride1,                \
                                src_stride2, h);                        \
}                                                                       \

#define op_avg(a, b) a = rnd_avg_pixel4(a, b)
#define op_put(a, b) a = b
DEF_HPEL(avg, op_avg)
DEF_HPEL(put, op_put)
#undef op_avg
#undef op_put
