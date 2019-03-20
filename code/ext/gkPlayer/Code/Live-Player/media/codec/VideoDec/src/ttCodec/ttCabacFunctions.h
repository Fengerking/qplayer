#ifndef __TTPOD_TT_CABAC_FUNCTIONS_H
#define __TTPOD_TT_CABAC_FUNCTIONS_H

#include <stdint.h>

#include "ttCabac.h"
#include "config.h"

#ifndef UNCHECKED_BITSTREAM_READER
#define UNCHECKED_BITSTREAM_READER !CONFIG_SAFE_BITSTREAM_READER
#endif

#ifdef __APPLE__

#if ARCH_ARM
#   include "arm_ios/ttCabac.h"
#endif

#else

#if ARCH_AARCH64
#   include "aarch64/ttCabac.h"
#endif
#if ARCH_ARM
#   include "arm/ttCabac.h"
#endif
#if ARCH_X86
#   include "x86/ttCabac.h"
#endif

#endif

static CABAC_TABLE_CONST uint8_t * const tt_h264_norm_shift = tt_h264_cabac_tables + H264_NORM_SHIFT_OFFSET;
static CABAC_TABLE_CONST uint8_t * const tt_h264_lps_range = tt_h264_cabac_tables + H264_LPS_RANGE_OFFSET;
static CABAC_TABLE_CONST uint8_t * const tt_h264_mlps_state = tt_h264_cabac_tables + H264_MLPS_STATE_OFFSET;
static CABAC_TABLE_CONST uint8_t * const tt_h264_last_coett_flag_offset_8x8 = tt_h264_cabac_tables + H264_LAST_COETT_FLAG_OFFSET_8x8_OFFSET;

static void refill(CABACContext *c){
#if CABAC_BITS == 16
        c->low+= (c->bytestream[0]<<9) + (c->bytestream[1]<<1);
#else
        c->low+= c->bytestream[0]<<1;
#endif
    c->low -= CABAC_MASK;
#if !UNCHECKED_BITSTREAM_READER
    if (c->bytestream < c->bytestream_end)
#endif
        c->bytestream += CABAC_BITS / 8;
}

static inline void renorm_cabac_decoder_once(CABACContext *c){
    int shift= (uint32_t)(c->range - 0x100)>>31;
    c->range<<= shift;
    c->low  <<= shift;
    if(!(c->low & CABAC_MASK))
        refill(c);
}

#ifndef get_cabac_inline
static void refill2(CABACContext *c){
    int i, x;

    x= c->low ^ (c->low-1);
    i= 7 - tt_h264_norm_shift[x>>(CABAC_BITS-1)];

    x= -CABAC_MASK;

#if CABAC_BITS == 16
        x+= (c->bytestream[0]<<9) + (c->bytestream[1]<<1);
#else
        x+= c->bytestream[0]<<1;
#endif

    c->low += x<<i;
#if !UNCHECKED_BITSTREAM_READER
    if (c->bytestream < c->bytestream_end)
#endif
        c->bytestream += CABAC_BITS/8;
}

static ttv_always_inline int get_cabac_inline(CABACContext *c, uint8_t * const state){
    int s = *state;
    int RangeLPS= tt_h264_lps_range[2*(c->range&0xC0) + s];
    int bit, lps_mask;

    c->range -= RangeLPS;
    lps_mask= ((c->range<<(CABAC_BITS+1)) - c->low)>>31;

    c->low -= (c->range<<(CABAC_BITS+1)) & lps_mask;
    c->range += (RangeLPS - c->range) & lps_mask;

    s^=lps_mask;
    *state= (tt_h264_mlps_state+128)[s];
    bit= s&1;

    lps_mask= tt_h264_norm_shift[c->range];
    c->range<<= lps_mask;
    c->low  <<= lps_mask;
    if(!(c->low & CABAC_MASK))
        refill2(c);
    return bit;
}
#endif

static int ttv_noinline ttv_unused get_cabac_noinline(CABACContext *c, uint8_t * const state){
    return get_cabac_inline(c,state);
}

static int ttv_unused get_cabac(CABACContext *c, uint8_t * const state){
    return get_cabac_inline(c,state);
}

#ifndef get_cabac_bypass
static int ttv_unused get_cabac_bypass(CABACContext *c){
    int range;
    c->low += c->low;

    if(!(c->low & CABAC_MASK))
        refill(c);

    range= c->range<<(CABAC_BITS+1);
    if(c->low < range){
        return 0;
    }else{
        c->low -= range;
        return 1;
    }
}
#endif

#ifndef get_cabac_bypass_sign
static ttv_always_inline int get_cabac_bypass_sign(CABACContext *c, int val){
    int range, mask;
    c->low += c->low;

    if(!(c->low & CABAC_MASK))
        refill(c);

    range= c->range<<(CABAC_BITS+1);
    c->low -= range;
    mask= c->low >> 31;
    range &= mask;
    c->low += range;
    return (val^mask)-mask;
}
#endif

/**
 *
 * @return the number of bytes read or 0 if no end
 */
static int ttv_unused get_cabac_terminate(CABACContext *c){
    c->range -= 2;
    if(c->low < c->range<<(CABAC_BITS+1)){
        renorm_cabac_decoder_once(c);
        return 0;
    }else{
        return c->bytestream - c->bytestream_start;
    }
}



#endif /* __TTPOD_TT_CABAC_FUNCTIONS_H */
