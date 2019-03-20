#ifndef __TTPOD_TT_AVASSERT_H_
#define __TTPOD_TT_AVASSERT_H_

#include "ttAvutil.h"
#include "ttLog.h"

#define TTV_TOSTRING(s) #s
#define TTV_STRINGIFY(s)         TTV_TOSTRING(s)

/**
 * assert() equivalent, that is always enabled.
 */
#define ttv_assert0(cond) do {                                           \
    if (!(cond)) {                                                      \
        ttv_log(NULL, TTV_LOG_FATAL, "Assertion %s failed at %s:%d\n",    \
               TTV_STRINGIFY(cond), __FILE__, __LINE__);                 \
        abort();                                                        \
    }                                                                   \
} while (0)


/**
 * assert() equivalent, that does not lie in speed critical code.
 * These asserts() thus can be enabled without fearing speedloss.
 */
#if defined(ASSERT_LEVEL) && ASSERT_LEVEL > 0
#define ttv_assert1(cond) ttv_assert0(cond)
#else
#define ttv_assert1(cond) ((void)0)
#endif


/**
 * assert() equivalent, that does lie in speed critical code.
 */
#if defined(ASSERT_LEVEL) && ASSERT_LEVEL > 1
#define ttv_assert2(cond) ttv_assert0(cond)
#else
#define ttv_assert2(cond) ((void)0)
#endif

#endif /* __TTPOD_TT_AVASSERT_H_ */
