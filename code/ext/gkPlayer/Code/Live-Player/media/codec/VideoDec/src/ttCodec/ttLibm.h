#ifndef __TTPOD_TT_LIBM_H_
#define __TTPOD_TT_LIBM_H_

#include <math.h>
#include "config.h"
#include "ttAttributes.h"

#if HAVE_MIPSFPU && HAVE_INLINE_ASM
#include "libavutil/mips/libm_mips.h"
#endif /* HAVE_MIPSFPU && HAVE_INLINE_ASM*/

#if !HAVE_ATANF
#undef atanf
#define atanf(x) ((float)atan(x))
#endif

#if !HAVE_ATAN2F
#undef atan2f
#define atan2f(y, x) ((float)atan2(y, x))
#endif

#if !HAVE_POWF
#undef powf
#define powf(x, y) ((float)pow(x, y))
#endif

#if !HAVE_CBRT
ttv_always_inline double cbrt(double x)
{
    return x < 0 ? -pow(-x, 1.0 / 3.0) : pow(x, 1.0 / 3.0);
}
#endif

#if !HAVE_CBRTF
ttv_always_inline float cbrtf(float x)
{
    return x < 0 ? -powf(-x, 1.0 / 3.0) : powf(x, 1.0 / 3.0);
}
#endif

#if !HAVE_COSF
#undef cosf
#define cosf(x) ((float)cos(x))
#endif

#if !HAVE_EXPF
#undef expf
#define expf(x) ((float)exp(x))
#endif

#if !HAVE_EXP2
#undef exp2
#define exp2(x) exp((x) * 0.693147180559945)
#endif /* HAVE_EXP2 */

#if !HAVE_EXP2F
#undef exp2f
#define exp2f(x) ((float)exp2(x))
#endif /* HAVE_EXP2F */

#if !HAVE_ISINF
ttv_always_inline ttv_const int isinf(float x)
{
    uint32_t v = ttv_float2int(x);
    if ((v & 0x7f800000) != 0x7f800000)
        return 0;
    return !(v & 0x007fffff);
}
#endif /* HAVE_ISINF */

#if !HAVE_ISNAN
ttv_always_inline ttv_const int isnan(float x)
{
    uint32_t v = ttv_float2int(x);
    if ((v & 0x7f800000) != 0x7f800000)
        return 0;
    return v & 0x007fffff;
}
#endif /* HAVE_ISNAN */

#if !HAVE_LDEXPF
#undef ldexpf
#define ldexpf(x, exp) ((float)ldexp(x, exp))
#endif

#if !HAVE_LLRINT
#undef llrint
#define llrint(x) ((long long)rint(x))
#endif /* HAVE_LLRINT */

#if !HAVE_LLRINTF
#undef llrintf
#define llrintf(x) ((long long)rint(x))
#endif /* HAVE_LLRINT */

#if !HAVE_LOG2
#undef log2
#define log2(x) (log(x) * 1.44269504088896340736)
#endif /* HAVE_LOG2 */

#if !HAVE_LOG2F
#undef log2f
#define log2f(x) ((float)log2(x))
#endif /* HAVE_LOG2F */

#if !HAVE_LOG10F
#undef log10f
#define log10f(x) ((float)log10(x))
#endif

#if !HAVE_SINF
#undef sinf
#define sinf(x) ((float)sin(x))
#endif

#if !HAVE_RINT
inline double rint(double x)
{
    return x >= 0 ? floor(x + 0.5) : ceil(x - 0.5);
}
#endif /* HAVE_RINT */

#if !HAVE_LRINT
ttv_always_inline ttv_const long int lrint(double x)
{
    return rint(x);
}
#endif /* HAVE_LRINT */

#if !HAVE_LRINTF
ttv_always_inline ttv_const long int lrintf(float x)
{
    return (int)(rint(x));
}
#endif /* HAVE_LRINTF */

#if !HAVE_ROUND
ttv_always_inline ttv_const double round(double x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}
#endif /* HAVE_ROUND */

#if !HAVE_ROUNDF
ttv_always_inline ttv_const float roundf(float x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}
#endif /* HAVE_ROUNDF */

#if !HAVE_TRUNC
ttv_always_inline ttv_const double trunc(double x)
{
    return (x > 0) ? floor(x) : ceil(x);
}
#endif /* HAVE_TRUNC */

#if !HAVE_TRUNCF
ttv_always_inline ttv_const float truncf(float x)
{
    return (x > 0) ? floor(x) : ceil(x);
}
#endif /* HAVE_TRUNCF */

#endif /* __TTPOD_TT_LIBM_H_ */
