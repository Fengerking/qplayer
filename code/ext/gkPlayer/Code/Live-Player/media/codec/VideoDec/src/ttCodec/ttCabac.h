#ifndef __TTPOD_TT_CABAC_H
#define __TTPOD_TT_CABAC_H

#include <stdint.h>

#if CONFIG_HARDCODED_TABLES
#define CABAC_TABLE_CONST const
#else
#define CABAC_TABLE_CONST
#endif
extern CABAC_TABLE_CONST uint8_t tt_h264_cabac_tables[512 + 4*2*64 + 4*64 + 63];
#define H264_NORM_SHIFT_OFFSET 0
#define H264_LPS_RANGE_OFFSET 512
#define H264_MLPS_STATE_OFFSET 1024
#define H264_LAST_COETT_FLAG_OFFSET_8x8_OFFSET 1280

#define CABAC_BITS 16
#define CABAC_MASK ((1<<CABAC_BITS)-1)

typedef struct CABACContext{
    int low;
    int range;
    int outstanding_count;
    const uint8_t *bytestream_start;
    const uint8_t *bytestream;
    const uint8_t *bytestream_end;
//    PutBitContext pb;
}CABACContext;

void tt_init_cabac_decoder(CABACContext *c, const uint8_t *buf, int buf_size);
void tt_init_cabac_states(void);

#endif /* __TTPOD_TT_CABAC_H */
