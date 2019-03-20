#include "ttH264DecArm32Macro.h"
#if H264_ARM32_OPEN

#include "ttAttributes.h"
#include "cpu.h"
#include "ttVideodsp.h"
#include "ttVideodspArm.h"

void tt_prefetch_arm(uint8_t *mem, ptrdiff_t stride, int h);

ttv_cold void tt_videodsp_init_armv5te(VideoDSPContext *ctx, int bpc)
{
#if HAVE_ARMV5TE_EXTERNAL
    ctx->prefetch = tt_prefetch_arm;
#endif
}
#endif
