#include "ttH264DecArm32Macro.h"
#if H264_ARM32_OPEN

#include "ttAttributes.h"
#include "cpu.h"
#include "ttVideodsp.h"
#include "ttVideodspArm.h"

ttv_cold void tt_videodsp_init_arm(VideoDSPContext *ctx, int bpc)
{
    int cpu_flags = ttv_get_cpu_flags();
    if (have_armv5te(cpu_flags)) tt_videodsp_init_armv5te(ctx, bpc);
}
#endif
