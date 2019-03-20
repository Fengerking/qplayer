#ifndef __TTPOD_TT_RATIONAL_H_
#define __TTPOD_TT_RATIONAL_H_

#include <stdint.h>
#include <limits.h>
#include "config.h"
#include "ttAttributes.h"

typedef struct TTRational{
    int num; ///< numerator
    int den; ///< denominator
} TTRational;

static inline TTRational ttv_make_q(int num, int den)
{
    TTRational r = { num, den };
    return r;
}

/**
 * Compare two rationals.
 * @param a first rational
 * @param b second rational
 * @return 0 if a==b, 1 if a>b, -1 if a<b, and INT_MIN if one of the
 * values is of the form 0/0
 */
static inline int ttv_cmp_q(TTRational a, TTRational b){
    const int64_t tmp= a.num * (int64_t)b.den - b.num * (int64_t)a.den;

    if(tmp) return (int)((tmp ^ a.den ^ b.den)>>63)|1;
    else if(b.den && a.den) return 0;
    else if(a.num && b.num) return (a.num>>31) - (b.num>>31);
    else                    return INT_MIN;
}


/**
 * Reduce a fraction.
 * This is useful for framerate calculations.
 * @param dst_num destination numerator
 * @param dst_den destination denominator
 * @param num source numerator
 * @param den source denominator
 * @param max the maximum allowed for dst_num & dst_den
 * @return 1 if exact, 0 otherwise
 */
int ttv_reduce(int *dst_num, int *dst_den, int64_t num, int64_t den, int64_t max);

/**
 * Multiply two rationals.
 * @param b first rational
 * @param c second rational
 * @return b*c
 */
TTRational ttv_mul_q(TTRational b, TTRational c) ttv_const;



/**
 * Invert a rational.
 * @param q value
 * @return 1 / q
 */
static ttv_always_inline TTRational ttv_inv_q(TTRational q)
{
    TTRational r = { q.den, q.num };
    return r;
}

/**
 * Convert a double precision floating point number to a rational.
 * inf is expressed as {1,0} or {-1,0} depending on the sign.
 *
 * @param d double to convert
 * @param max the maximum allowed numerator and denominator
 * @return (TTRational) d
 */
TTRational ttv_d2q(double d, int max) ttv_const;



#endif /* __TTPOD_TT_RATIONAL_H_ */
