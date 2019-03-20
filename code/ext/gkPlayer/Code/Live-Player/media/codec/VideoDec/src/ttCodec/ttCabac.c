#include <string.h>

#include "ttCommon.h"
#include "ttGetBits.h"
#include "ttCabac.h"
#include "ttCabacFunctions.h"
#include "ttCabacTablegen.h"

void tt_init_cabac_decoder(CABACContext *c, const uint8_t *buf, int buf_size){
    c->bytestream_start=
    c->bytestream= buf;
    c->bytestream_end= buf + buf_size;

#if CABAC_BITS == 16
    c->low =  (*c->bytestream++)<<18;
    c->low+=  (*c->bytestream++)<<10;
#else
    c->low =  (*c->bytestream++)<<10;
#endif
    c->low+= ((*c->bytestream++)<<2) + 2;
    c->range= 0x1FE;
}

void tt_init_cabac_states(void)
{
    static int initialized = 0;

    if (initialized)
        return;

    cabac_tableinit();

    initialized = 1;
}

