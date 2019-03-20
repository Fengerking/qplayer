#include "ttAvassert.h"
#include <limits.h>

#include "ttCommon.h"
#include "ttMathematics.h"
#include "ttRational.h"
#include "ttLibm.h"


union ttv_intfloat32 {
	uint32_t i;
	float    f;
};

static ttv_always_inline uint32_t ttv_float2int(float f)
{
	union ttv_intfloat32 v;
	v.f = f;
	return v.i;
}

int ttv_reduce(int *dst_num, int *dst_den,
              int64_t num, int64_t den, int64_t max)
{
    TTRational a0 = { 0, 1 }, a1 = { 1, 0 };
    int sign = (num < 0) ^ (den < 0);
    int64_t gcd = ttv_gcd(FFABS(num), FFABS(den));

    if (gcd) {
        num = FFABS(num) / gcd;
        den = FFABS(den) / gcd;
    }
    if (num <= max && den <= max) {
        a1 = ttv_make_q(num, den);
        den = 0;
    }

    while (den) {
        uint64_t x        = num / den;
        int64_t next_den  = num - den * x;
        int64_t a2n       = x * a1.num + a0.num;
        int64_t a2d       = x * a1.den + a0.den;

        if (a2n > max || a2d > max) {
            if (a1.num) x =          (max - a0.num) / a1.num;
            if (a1.den) x = FFMIN(x, (max - a0.den) / a1.den);

            if (den * (2 * x * a1.den + a0.den) > num * a1.den)
                a1 = ttv_make_q(x * a1.num + a0.num, x * a1.den + a0.den);
            break;
        }

        a0  = a1;
        a1  = ttv_make_q(a2n, a2d);
        num = den;
        den = next_den;
    }
    ttv_assert2(ttv_gcd(a1.num, a1.den) <= 1U);

    *dst_num = sign ? -a1.num : a1.num;
    *dst_den = a1.den;

    return den == 0;
}

TTRational ttv_mul_q(TTRational b, TTRational c)
{
    ttv_reduce(&b.num, &b.den,
               b.num * (int64_t) c.num,
               b.den * (int64_t) c.den, INT_MAX);
    return b;
}


TTRational ttv_d2q(double d, int max)
{
    TTRational a;
#define LOG2  0.69314718055994530941723212145817656807550013436025
    int exponent;
    int64_t den;
    if (isnan(d))
        return ttv_make_q( 0,0);
    if (fabs(d) > INT_MAX + 3LL)
        return ttv_make_q(d < 0 ? -1 : 1, 0);
    exponent = FFMAX( (int)(log(fabs(d) + 1e-20)/LOG2), 0);
    den = 1LL << (61 - exponent);
    // (int64_t)rint() and llrint() do not work with gcc on ia64 and sparc64
    ttv_reduce(&a.num, &a.den, floor(d * den + 0.5), den, max);
    if ((!a.num || !a.den) && d && max>0 && max<INT_MAX)
        ttv_reduce(&a.num, &a.den, floor(d * den + 0.5), den, INT_MAX);

    return a;
}
