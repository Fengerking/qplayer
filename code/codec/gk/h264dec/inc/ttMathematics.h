#ifndef __TTPOD_TT_MATHEMATICS_H_
#define __TTPOD_TT_MATHEMATICS_H_

#include <stdint.h>
#include <math.h>
#include "ttAttributes.h"
#include "ttRational.h"

#ifndef M_E
#define M_E            2.7182818284590452354   /* e */
#endif
#ifndef M_LN2
#define M_LN2          0.69314718055994530942  /* log_e 2 */
#endif
#ifndef M_LN10
#define M_LN10         2.30258509299404568402  /* log_e 10 */
#endif
#ifndef M_LOG2_10
#define M_LOG2_10      3.32192809488736234787  /* log_2 10 */
#endif
#ifndef M_PHI
#define M_PHI          1.61803398874989484820   /* phi / golden ratio */
#endif
#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif
#ifndef M_PI_2
#define M_PI_2         1.57079632679489661923  /* pi/2 */
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2      0.70710678118654752440  /* 1/sqrt(2) */
#endif
#ifndef M_SQRT2
#define M_SQRT2        1.41421356237309504880  /* sqrt(2) */
#endif
#ifndef NAN
#define NAN            ttv_int2float(0x7fc00000)
#endif
#ifndef INFINITY
#define INFINITY       ttv_int2float(0x7f800000)
#endif

enum AVRounding {
    TTV_ROUND_ZERO     = 0, ///< Round toward zero.
    TTV_ROUND_INF      = 1, ///< Round away from zero.
    TTV_ROUND_DOWN     = 2, ///< Round toward -infinity.
    TTV_ROUND_UP       = 3, ///< Round toward +infinity.
    TTV_ROUND_NEAR_INF = 5, ///< Round to nearest and halfway cases away from zero.
    TTV_ROUND_PASS_MINMAX = 8192, ///< Flag to pass INT64_MIN/MAX through instead of rescaling, this avoids special cases for TTV_NOPTS_VALUE
};

int64_t ttv_const ttv_gcd(int64_t a, int64_t b);

int64_t ttv_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding) ttv_const;

#endif /* __TTPOD_TT_MATHEMATICS_H_ */
