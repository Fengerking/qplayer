#ifndef __TTPOD_ATTRIBUTES_H_
#define __TTPOD_ATTRIBUTES_H_

#ifdef __GNUC__
#    define TTV_GCC_VERSION_AT_LEAST(x,y) (__GNUC__ > x || __GNUC__ == x && __GNUC_MINOR__ >= y)
#else
#    define TTV_GCC_VERSION_AT_LEAST(x,y) 0
#endif

#ifndef ttv_always_inline
#if TTV_GCC_VERSION_AT_LEAST(3,1)
#    define ttv_always_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#    define ttv_always_inline __forceinline
#else
#    define ttv_always_inline inline
#endif
#endif

#ifndef ttv_extern_inline
#if defined(__ICL) && __ICL >= 1210 || defined(__GNUC_STDC_INLINE__)
#    define ttv_extern_inline extern inline
#else
#    define ttv_extern_inline inline
#endif
#endif

#if TTV_GCC_VERSION_AT_LEAST(3,1)
#    define ttv_noinline __attribute__((noinline))
#elif defined(_MSC_VER)
#    define ttv_noinline __declspec(noinline)
#else
#    define ttv_noinline
#endif

#if TTV_GCC_VERSION_AT_LEAST(3,1)
#    define ttv_pure __attribute__((pure))
#else
#    define ttv_pure
#endif

#if TTV_GCC_VERSION_AT_LEAST(2,6)
#    define ttv_const __attribute__((const))
#else
#    define ttv_const
#endif

#if TTV_GCC_VERSION_AT_LEAST(4,3)
#    define ttv_cold __attribute__((cold))
#else
#    define ttv_cold
#endif

#if TTV_GCC_VERSION_AT_LEAST(4,1) && !defined(__llvm__)
#    define ttv_flatten __attribute__((flatten))
#else
#    define ttv_flatten
#endif

#if TTV_GCC_VERSION_AT_LEAST(3,1)
#    define attribute_deprecated __attribute__((deprecated))
#elif defined(_MSC_VER)
#    define attribute_deprecated __declspec(deprecated)
#else
#    define attribute_deprecated
#endif

/**
 * Disable warnings about deprecated features
 * This is useful for sections of code kept for backward compatibility and
 * scheduled for removal.
 */
#ifndef TTV_NOWARN_DEPRECATED
#if TTV_GCC_VERSION_AT_LEAST(4,6)
#    define TTV_NOWARN_DEPRECATED(code) \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
        code \
        _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#    define TTV_NOWARN_DEPRECATED(code) \
        __pragma(warning(push)) \
        __pragma(warning(disable : 4996)) \
        code; \
        __pragma(warning(pop))
#else
#    define TTV_NOWARN_DEPRECATED(code) code
#endif
#endif


#if defined(__GNUC__)
#    define ttv_unused __attribute__((unused))
#else
#    define ttv_unused
#endif

/**
 * Mark a variable as used and prevent the compiler from optimizing it
 * away.  This is useful for variables accessed only from inline
 * assembler without the compiler being aware.
 */
#if TTV_GCC_VERSION_AT_LEAST(3,1)
#    define ttv_used __attribute__((used))
#else
#    define ttv_used
#endif

#if TTV_GCC_VERSION_AT_LEAST(3,3)
#   define ttv_alias __attribute__((may_alias))
#else
#   define ttv_alias
#endif

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
#    define ttv_uninit(x) x=x
#else
#    define ttv_uninit(x) x
#endif

#ifdef __GNUC__
#    define ttv_builtin_constant_p __builtin_constant_p
#    define ttv_printf_format(fmtpos, attrpos) __attribute__((__format__(__printf__, fmtpos, attrpos)))
#else
#    define ttv_builtin_constant_p(x) 0
#    define ttv_printf_format(fmtpos, attrpos)
#endif

#if TTV_GCC_VERSION_AT_LEAST(2,5)
#    define ttv_noreturn __attribute__((noreturn))
#else
#    define ttv_noreturn
#endif

#endif /* __TTPOD_ATTRIBUTES_H_ */
