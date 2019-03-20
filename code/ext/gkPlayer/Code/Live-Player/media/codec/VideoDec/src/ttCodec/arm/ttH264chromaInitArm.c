
#include <stdint.h>

#include "ttCodec/ttAttributes.h"
#include "ttCodec/ttCpu.h"
#include "ttCodec/arm/cpu.h"
#include "ttCodec/ttH264chroma.h"

void tt_put_h264_chroma_mc8_neon(uint8_t *, uint8_t *, int, int, int, int);
void tt_put_h264_chroma_mc4_neon(uint8_t *, uint8_t *, int, int, int, int);
void tt_put_h264_chroma_mc2_neon(uint8_t *, uint8_t *, int, int, int, int);

void tt_avg_h264_chroma_mc8_neon(uint8_t *, uint8_t *, int, int, int, int);
void tt_avg_h264_chroma_mc4_neon(uint8_t *, uint8_t *, int, int, int, int);
void tt_avg_h264_chroma_mc2_neon(uint8_t *, uint8_t *, int, int, int, int);

ttv_cold void tt_h264chroma_init_arm(H264ChromaContext *c, int bit_depth)
{
    const int high_bit_depth = bit_depth > 8;
    int cpu_flags = ttv_get_cpu_flags();

    if (have_neon(cpu_flags) && !high_bit_depth) {
        c->put_h264_chroma_pixels_tab[0] = tt_put_h264_chroma_mc8_neon;
        c->put_h264_chroma_pixels_tab[1] = tt_put_h264_chroma_mc4_neon;
        c->put_h264_chroma_pixels_tab[2] = tt_put_h264_chroma_mc2_neon;

        c->avg_h264_chroma_pixels_tab[0] = tt_avg_h264_chroma_mc8_neon;
        c->avg_h264_chroma_pixels_tab[1] = tt_avg_h264_chroma_mc4_neon;
        c->avg_h264_chroma_pixels_tab[2] = tt_avg_h264_chroma_mc2_neon;
    }
}
