
#include <stdint.h>
#include <stddef.h>
#include "ttH264DecArm32Macro.h"
#if H264_ARM32_OPEN

#include "config.h"
#include "ttAttributes.h"
#include "cpu.h"
#include "ttH264qpel.h"

void tt_put_h264_qpel16_mc00_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc10_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc20_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc30_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc01_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc11_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc21_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc31_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc02_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc12_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc22_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc32_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc03_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc13_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc23_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel16_mc33_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);

void tt_put_h264_qpel8_mc00_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc10_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc20_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc30_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc01_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc11_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc21_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc31_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc02_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc12_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc22_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc32_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc03_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc13_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc23_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_put_h264_qpel8_mc33_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);

void tt_avg_h264_qpel16_mc00_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc10_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc20_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc30_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc01_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc11_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc21_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc31_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc02_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc12_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc22_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc32_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc03_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc13_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc23_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel16_mc33_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);

void tt_avg_h264_qpel8_mc00_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc10_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc20_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc30_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc01_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc11_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc21_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc31_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc02_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc12_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc22_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc32_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc03_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc13_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc23_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);
void tt_avg_h264_qpel8_mc33_neon(uint8_t *dst, const uint8_t *src, ptrdiff_t stride);

ttv_cold void tt_h264qpel_init_arm(H264QpelContext *c, int bit_depth)
{
    const int high_bit_depth = bit_depth > 8;
    int cpu_flags = ttv_get_cpu_flags();

    if (have_neon(cpu_flags) && !high_bit_depth) {
        c->put_h264_qpel_pixels_tab[0][ 0] = tt_put_h264_qpel16_mc00_neon;
        c->put_h264_qpel_pixels_tab[0][ 1] = tt_put_h264_qpel16_mc10_neon;
        c->put_h264_qpel_pixels_tab[0][ 2] = tt_put_h264_qpel16_mc20_neon;
        c->put_h264_qpel_pixels_tab[0][ 3] = tt_put_h264_qpel16_mc30_neon;
        c->put_h264_qpel_pixels_tab[0][ 4] = tt_put_h264_qpel16_mc01_neon;
        c->put_h264_qpel_pixels_tab[0][ 5] = tt_put_h264_qpel16_mc11_neon;
        c->put_h264_qpel_pixels_tab[0][ 6] = tt_put_h264_qpel16_mc21_neon;
        c->put_h264_qpel_pixels_tab[0][ 7] = tt_put_h264_qpel16_mc31_neon;
        c->put_h264_qpel_pixels_tab[0][ 8] = tt_put_h264_qpel16_mc02_neon;
        c->put_h264_qpel_pixels_tab[0][ 9] = tt_put_h264_qpel16_mc12_neon;
        c->put_h264_qpel_pixels_tab[0][10] = tt_put_h264_qpel16_mc22_neon;
        c->put_h264_qpel_pixels_tab[0][11] = tt_put_h264_qpel16_mc32_neon;
        c->put_h264_qpel_pixels_tab[0][12] = tt_put_h264_qpel16_mc03_neon;
        c->put_h264_qpel_pixels_tab[0][13] = tt_put_h264_qpel16_mc13_neon;
        c->put_h264_qpel_pixels_tab[0][14] = tt_put_h264_qpel16_mc23_neon;
        c->put_h264_qpel_pixels_tab[0][15] = tt_put_h264_qpel16_mc33_neon;

        c->put_h264_qpel_pixels_tab[1][ 0] = tt_put_h264_qpel8_mc00_neon;
        c->put_h264_qpel_pixels_tab[1][ 1] = tt_put_h264_qpel8_mc10_neon;
        c->put_h264_qpel_pixels_tab[1][ 2] = tt_put_h264_qpel8_mc20_neon;
        c->put_h264_qpel_pixels_tab[1][ 3] = tt_put_h264_qpel8_mc30_neon;
        c->put_h264_qpel_pixels_tab[1][ 4] = tt_put_h264_qpel8_mc01_neon;
        c->put_h264_qpel_pixels_tab[1][ 5] = tt_put_h264_qpel8_mc11_neon;
        c->put_h264_qpel_pixels_tab[1][ 6] = tt_put_h264_qpel8_mc21_neon;
        c->put_h264_qpel_pixels_tab[1][ 7] = tt_put_h264_qpel8_mc31_neon;
        c->put_h264_qpel_pixels_tab[1][ 8] = tt_put_h264_qpel8_mc02_neon;
        c->put_h264_qpel_pixels_tab[1][ 9] = tt_put_h264_qpel8_mc12_neon;
        c->put_h264_qpel_pixels_tab[1][10] = tt_put_h264_qpel8_mc22_neon;
        c->put_h264_qpel_pixels_tab[1][11] = tt_put_h264_qpel8_mc32_neon;
        c->put_h264_qpel_pixels_tab[1][12] = tt_put_h264_qpel8_mc03_neon;
        c->put_h264_qpel_pixels_tab[1][13] = tt_put_h264_qpel8_mc13_neon;
        c->put_h264_qpel_pixels_tab[1][14] = tt_put_h264_qpel8_mc23_neon;
        c->put_h264_qpel_pixels_tab[1][15] = tt_put_h264_qpel8_mc33_neon;

        c->avg_h264_qpel_pixels_tab[0][ 0] = tt_avg_h264_qpel16_mc00_neon;
        c->avg_h264_qpel_pixels_tab[0][ 1] = tt_avg_h264_qpel16_mc10_neon;
        c->avg_h264_qpel_pixels_tab[0][ 2] = tt_avg_h264_qpel16_mc20_neon;
        c->avg_h264_qpel_pixels_tab[0][ 3] = tt_avg_h264_qpel16_mc30_neon;
        c->avg_h264_qpel_pixels_tab[0][ 4] = tt_avg_h264_qpel16_mc01_neon;
        c->avg_h264_qpel_pixels_tab[0][ 5] = tt_avg_h264_qpel16_mc11_neon;
        c->avg_h264_qpel_pixels_tab[0][ 6] = tt_avg_h264_qpel16_mc21_neon;
        c->avg_h264_qpel_pixels_tab[0][ 7] = tt_avg_h264_qpel16_mc31_neon;
        c->avg_h264_qpel_pixels_tab[0][ 8] = tt_avg_h264_qpel16_mc02_neon;
        c->avg_h264_qpel_pixels_tab[0][ 9] = tt_avg_h264_qpel16_mc12_neon;
        c->avg_h264_qpel_pixels_tab[0][10] = tt_avg_h264_qpel16_mc22_neon;
        c->avg_h264_qpel_pixels_tab[0][11] = tt_avg_h264_qpel16_mc32_neon;
        c->avg_h264_qpel_pixels_tab[0][12] = tt_avg_h264_qpel16_mc03_neon;
        c->avg_h264_qpel_pixels_tab[0][13] = tt_avg_h264_qpel16_mc13_neon;
        c->avg_h264_qpel_pixels_tab[0][14] = tt_avg_h264_qpel16_mc23_neon;
        c->avg_h264_qpel_pixels_tab[0][15] = tt_avg_h264_qpel16_mc33_neon;

        c->avg_h264_qpel_pixels_tab[1][ 0] = tt_avg_h264_qpel8_mc00_neon;
        c->avg_h264_qpel_pixels_tab[1][ 1] = tt_avg_h264_qpel8_mc10_neon;
        c->avg_h264_qpel_pixels_tab[1][ 2] = tt_avg_h264_qpel8_mc20_neon;
        c->avg_h264_qpel_pixels_tab[1][ 3] = tt_avg_h264_qpel8_mc30_neon;
        c->avg_h264_qpel_pixels_tab[1][ 4] = tt_avg_h264_qpel8_mc01_neon;
        c->avg_h264_qpel_pixels_tab[1][ 5] = tt_avg_h264_qpel8_mc11_neon;
        c->avg_h264_qpel_pixels_tab[1][ 6] = tt_avg_h264_qpel8_mc21_neon;
        c->avg_h264_qpel_pixels_tab[1][ 7] = tt_avg_h264_qpel8_mc31_neon;
        c->avg_h264_qpel_pixels_tab[1][ 8] = tt_avg_h264_qpel8_mc02_neon;
        c->avg_h264_qpel_pixels_tab[1][ 9] = tt_avg_h264_qpel8_mc12_neon;
        c->avg_h264_qpel_pixels_tab[1][10] = tt_avg_h264_qpel8_mc22_neon;
        c->avg_h264_qpel_pixels_tab[1][11] = tt_avg_h264_qpel8_mc32_neon;
        c->avg_h264_qpel_pixels_tab[1][12] = tt_avg_h264_qpel8_mc03_neon;
        c->avg_h264_qpel_pixels_tab[1][13] = tt_avg_h264_qpel8_mc13_neon;
        c->avg_h264_qpel_pixels_tab[1][14] = tt_avg_h264_qpel8_mc23_neon;
        c->avg_h264_qpel_pixels_tab[1][15] = tt_avg_h264_qpel8_mc33_neon;
    }
}
#endif
