#include "ttAttributes.h"
#include "ttAvassert.h"
#include "ttCommon.h"
#include "ttVideodsp.h"

#define BIT_DEPTH 8
#include "ttVideodspTemplate.c"
#undef BIT_DEPTH

// #define BIT_DEPTH 16
// #include "videodsp_template.c"
// #undef BIT_DEPTH

static void just_return(uint8_t *buf, ptrdiff_t stride, int h)
{
}

ttv_cold void tt_videodsp_init(VideoDSPContext *ctx, int bpc)
{
    ctx->prefetch = just_return;
    if (bpc <= 8) {
        ctx->emulated_edge_mc = tt_emulated_edge_mc_8;
    } else {
        //ctx->emulated_edge_mc = tt_emulated_edge_mc_16;
		assert(0);
    }
#if ARCH_AARCH64
    if (ARCH_AARCH64)
        tt_videodsp_init_aarch64(ctx, bpc);
#endif
#if ARCH_ARM
    if (ARCH_ARM)
        tt_videodsp_init_arm(ctx, bpc);
#endif
#if ARCH_PPC
    if (ARCH_PPC)
        tt_videodsp_init_ppc(ctx, bpc);
#endif
#if ARCH_X86
    if (ARCH_X86)
        tt_videodsp_init_x86(ctx, bpc);
#endif
}
