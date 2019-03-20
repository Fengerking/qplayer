#ifndef __TTPOD_TT_GOLOMB_H
#define __TTPOD_TT_GOLOMB_H

#include <stdint.h>

#include "ttGetBits.h"

extern const uint8_t tt_golomb_vlc_len[512];
extern const uint8_t tt_ue_golomb_vlc_code[512];
extern const  int8_t tt_se_golomb_vlc_code[512];


static inline int get_ue_golomb(GetBitContext *gb)
{
    unsigned int buf;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);

    if (buf >= (1 << 27)) {
        buf >>= 32 - 9;
        LAST_SKIP_BITS(re, gb, tt_golomb_vlc_len[buf]);
        CLOSE_READER(re, gb);

        return tt_ue_golomb_vlc_code[buf];
    } else {
        int log = 2 * ttv_log2(buf) - 31;
        LAST_SKIP_BITS(re, gb, 32 - log);
        CLOSE_READER(re, gb);
        if (CONFIG_FTRAPV && log < 0) {
            ttv_log(0, TTV_LOG_ERROR, "Invalid UE golomb code\n");
            return TTERROR_INVALIDDATA;
        }
        buf >>= log;
        buf--;

        return buf;
    }
}

/**
 * Read an unsigned Exp-Golomb code in the range 0 to UINT32_MAX-1.
 */
static inline unsigned get_ue_golomb_long(GetBitContext *gb)
{
    unsigned buf, log;

    buf = show_bits_long(gb, 32);
    log = 31 - ttv_log2(buf);
    skip_bits_long(gb, log);

    return get_bits_long(gb, log + 1) - 1;
}

/**
 * read unsigned exp golomb code, constraint to a max of 31.
 * the return value is undefined if the stored value exceeds 31.
 */
static inline int get_ue_golomb_31(GetBitContext *gb)
{
    unsigned int buf;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);

    buf >>= 32 - 9;
    LAST_SKIP_BITS(re, gb, tt_golomb_vlc_len[buf]);
    CLOSE_READER(re, gb);

    return tt_ue_golomb_vlc_code[buf];
}


/**
 * read signed exp golomb code.
 */
static inline int get_se_golomb(GetBitContext *gb)
{
    unsigned int buf;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);

    if (buf >= (1 << 27)) {
        buf >>= 32 - 9;
        LAST_SKIP_BITS(re, gb, tt_golomb_vlc_len[buf]);
        CLOSE_READER(re, gb);

        return tt_se_golomb_vlc_code[buf];
    } else {
        int log = ttv_log2(buf);
        LAST_SKIP_BITS(re, gb, 31 - log);
        UPDATE_CACHE(re, gb);
        buf = GET_CACHE(re, gb);

        buf >>= log;

        LAST_SKIP_BITS(re, gb, 32 - log);
        CLOSE_READER(re, gb);

        if (buf & 1)
            buf = -(buf >> 1);
        else
            buf = (buf >> 1);

        return buf;
    }
}




#endif /* __TTPOD_TT_GOLOMB_H */
