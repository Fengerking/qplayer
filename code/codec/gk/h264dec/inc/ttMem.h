#ifndef __TTPOD_TT_MEM_H_
#define __TTPOD_TT_MEM_H_

#include <limits.h>
#include <stdint.h>

#include "ttAttributes.h"
#include "ttError.h"
#include "ttAvutil.h"

#if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1110 || defined(__SUNPRO_C)
    #define DECLARE_ALIGNED(n,t,v)      t __attribute__ ((aligned (n))) v
    #define DECLARE_ASM_CONST(n,t,v)    const t __attribute__ ((aligned (n))) v
#elif defined(__TI_COMPILER_VERSION__)
    #define DECLARE_ALIGNED(n,t,v)                      \
        TTV_PRAGMA(DATA_ALIGN(v,n))                      \
        t __attribute__((aligned(n))) v
    #define DECLARE_ASM_CONST(n,t,v)                    \
        TTV_PRAGMA(DATA_ALIGN(v,n))                      \
        static const t __attribute__((aligned(n))) v
#elif defined(__GNUC__)
    #define DECLARE_ALIGNED(n,t,v)      t __attribute__ ((aligned (n))) v
    #define DECLARE_ASM_CONST(n,t,v)    static const t ttv_used __attribute__ ((aligned (n))) v
#elif defined(_MSC_VER)
    #define DECLARE_ALIGNED(n,t,v)      __declspec(align(n)) t v
    #define DECLARE_ASM_CONST(n,t,v)    __declspec(align(n)) static const t v
#else
    #define DECLARE_ALIGNED(n,t,v)      t v
    #define DECLARE_ASM_CONST(n,t,v)    static const t v
#endif

#if TTV_GCC_VERSION_AT_LEAST(3,1)
    #define ttv_malloc_attrib __attribute__((__malloc__))
#else
    #define ttv_malloc_attrib
#endif

#if TTV_GCC_VERSION_AT_LEAST(4,3)
    #define ttv_alloc_size(...) __attribute__((alloc_size(__VA_ARGS__)))
#else
    #define ttv_alloc_size(...)
#endif

void *ttv_malloc(size_t size) ttv_malloc_attrib ttv_alloc_size(1);

ttv_alloc_size(1, 2) static inline void *ttv_malloc_array(size_t nmemb, size_t size)
{
    if (!size || nmemb >= INT_MAX / size)
        return NULL;
    return ttv_malloc(nmemb * size);
}

void *ttv_realloc(void *ptr, size_t size) ttv_alloc_size(2);

void *ttv_realloc_f(void *ptr, size_t nelem, size_t elsize);

ttv_alloc_size(2, 3) int ttv_reallocp_array(void *ptr, size_t nmemb, size_t size);

void ttv_free(void *ptr);

void *ttv_mallocz(size_t size) ttv_malloc_attrib ttv_alloc_size(1);

ttv_alloc_size(1, 2) static inline void *ttv_mallocz_array(size_t nmemb, size_t size)
{
    if (!size || nmemb >= INT_MAX / size)
        return NULL;
    return ttv_mallocz(nmemb * size);
}

char *ttv_strdup(const char *s) ttv_malloc_attrib;

void ttv_freep(void *ptr);


static inline int ttv_size_mult(size_t a, size_t b, size_t *r)
{
    size_t t = a * b;
    /* Hack inspired from glibc: only try the division if nelem and elsize
     * are both greater than sqrt(SIZE_MAX). */
    if ((a | b) >= ((size_t)1 << (sizeof(size_t) * 4)) && a && t / a != b)
        return AVERROR(EINVAL);
    *r = t;
    return 0;
}


void *ttv_fast_realloc(void *ptr, unsigned int *size, size_t min_size);


void ttv_fast_malloc(void *ptr, unsigned int *size, size_t min_size);


#endif /* __TTPOD_TT_MEM_H_ */
