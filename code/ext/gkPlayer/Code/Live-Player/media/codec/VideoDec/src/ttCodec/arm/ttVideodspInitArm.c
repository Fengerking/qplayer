
#include "ttCodec/ttAttributes.h"
#include "ttCodec/arm/cpu.h"
#include "ttCodec/ttVideodsp.h"
#include "ttVideodspArm.h"

ttv_cold void tt_videodsp_init_arm(VideoDSPContext *ctx, int bpc)
{
    int cpu_flags = ttv_get_cpu_flags();
    if (have_armv5te(cpu_flags)) tt_videodsp_init_armv5te(ctx, bpc);
}
