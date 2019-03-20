
#include <stdint.h>

#include "ttCodec/ttAttributes.h"
#include "ttCodec/ttBlockdsp.h"
#include "ttBlockdspArm.h"

void tt_clear_block_neon(int16_t *block);
void tt_clear_blocks_neon(int16_t *blocks);

ttv_cold void tt_blockdsp_init_neon(BlockDSPContext *c, unsigned high_bit_depth)
{
    if (!high_bit_depth) {
        c->clear_block  = tt_clear_block_neon;
        c->clear_blocks = tt_clear_blocks_neon;
    }
}
