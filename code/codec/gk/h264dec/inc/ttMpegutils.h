#ifndef __TTPOD_TT_MPEGUTILS_H
#define __TTPOD_TT_MPEGUTILS_H

#include <stdint.h>

#include "ttFrame.h"

#include "ttAvcodec.h"
#include "ttVersion.h"


/* picture type */
#define PICT_TOP_FIELD     1
#define PICT_BOTTOM_FIELD  2
#define PICT_FRAME         3

/**
 * Value of Picture.reference when Picture is not a reference picture, but
 * is held for delayed output.
 */
#define DELAYED_PIC_REF 4



#define MB_TYPE_INTRA    MB_TYPE_INTRA4x4 // default mb_type if there is just one type

#define IS_INTRA4x4(a)   ((a) & MB_TYPE_INTRA4x4)
#define IS_INTRA16x16(a) ((a) & MB_TYPE_INTRA16x16)
#define IS_PCM(a)        ((a) & MB_TYPE_INTRA_PCM)
#define IS_INTRA(a)      ((a) & 7)
#define IS_INTER(a)      ((a) & (MB_TYPE_16x16 | MB_TYPE_16x8 | \
                                 MB_TYPE_8x16  | MB_TYPE_8x8))
#define IS_SKIP(a)       ((a) & MB_TYPE_SKIP)
#define IS_INTRA_PCM(a)  ((a) & MB_TYPE_INTRA_PCM)
#define IS_INTERLACED(a) ((a) & MB_TYPE_INTERLACED)
#define IS_DIRECT(a)     ((a) & MB_TYPE_DIRECT2)
#define IS_GMC(a)        ((a) & MB_TYPE_GMC)
#define IS_16X16(a)      ((a) & MB_TYPE_16x16)
#define IS_16X8(a)       ((a) & MB_TYPE_16x8)
#define IS_8X16(a)       ((a) & MB_TYPE_8x16)
#define IS_8X8(a)        ((a) & MB_TYPE_8x8)
#define IS_SUB_8X8(a)    ((a) & MB_TYPE_16x16) // note reused
#define IS_SUB_8X4(a)    ((a) & MB_TYPE_16x8)  // note reused
#define IS_SUB_4X8(a)    ((a) & MB_TYPE_8x16)  // note reused
#define IS_SUB_4X4(a)    ((a) & MB_TYPE_8x8)   // note reused
#define IS_ACPRED(a)     ((a) & MB_TYPE_ACPRED)
#define IS_QUANT(a)      ((a) & MB_TYPE_QUANT)
#define IS_DIR(a, part, list) ((a) & (MB_TYPE_P0L0 << ((part) + 2 * (list))))

// does this mb use listX, note does not work if subMBs
#define USES_LIST(a, list) ((a) & ((MB_TYPE_P0L0 | MB_TYPE_P1L0) << (2 * (list))))




#endif /* __TTPOD_TT_PICTTYPE_H */
