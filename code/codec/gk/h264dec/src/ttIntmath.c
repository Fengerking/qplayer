#include "ttIntmath.h"

/* undef these to get the function prototypes from ttCommon.h */
#undef ttv_log2
#undef ttv_log2_16bit
#include "ttCommon.h"

int ttv_log2(unsigned v)
{
    return tt_log2_as(v);
}
