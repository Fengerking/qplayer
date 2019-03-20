#ifndef __TTPOD_TT_CPU_H_
#define __TTPOD_TT_CPU_H_

#include "ttAttributes.h"

#define TTV_CPU_FLAG_FORCE    0x80000000 /* force usage of selected flags (OR) */

    /* lower 16 bits - CPU features */
#define TTV_CPU_FLAG_MMX          0x0001 ///< standard MMX
#define TTV_CPU_FLAG_MMXEXT       0x0002 ///< SSE integer functions or AMD MMX ext
#define TTV_CPU_FLAG_MMX2         0x0002 ///< SSE integer functions or AMD MMX ext
#define TTV_CPU_FLAG_3DNOW        0x0004 ///< AMD 3DNOW
#define TTV_CPU_FLAG_SSE          0x0008 ///< SSE functions
#define TTV_CPU_FLAG_SSE2         0x0010 ///< PIV SSE2 functions
#define TTV_CPU_FLAG_SSE2SLOW 0x40000000 ///< SSE2 supported, but usually not faster
                                        ///< than regular MMX/SSE (e.g. Core1)
#define TTV_CPU_FLAG_3DNOWEXT     0x0020 ///< AMD 3DNowExt
#define TTV_CPU_FLAG_SSE3         0x0040 ///< Prescott SSE3 functions
#define TTV_CPU_FLAG_SSE3SLOW 0x20000000 ///< SSE3 supported, but usually not faster
                                        ///< than regular MMX/SSE (e.g. Core1)
#define TTV_CPU_FLAG_SSSE3        0x0080 ///< Conroe SSSE3 functions
#define TTV_CPU_FLAG_ATOM     0x10000000 ///< Atom processor, some SSSE3 instructions are slower
#define TTV_CPU_FLAG_SSE4         0x0100 ///< Penryn SSE4.1 functions
#define TTV_CPU_FLAG_SSE42        0x0200 ///< Nehalem SSE4.2 functions
#define TTV_CPU_FLAG_AVX          0x4000 ///< AVX functions: requires OS support even if YMM registers aren't used
#define TTV_CPU_FLAG_XOP          0x0400 ///< Bulldozer XOP functions
#define TTV_CPU_FLAG_FMA4         0x0800 ///< Bulldozer FMA4 functions
// #if LIB__TTPOD_TT_VERSION_MAJOR <52
#define TTV_CPU_FLAG_CMOV      0x1001000 ///< supports cmov instruction
// #else
// #define TTV_CPU_FLAG_CMOV         0x1000 ///< supports cmov instruction
// #endif
#define TTV_CPU_FLAG_AVX2         0x8000 ///< AVX2 functions: requires OS support even if YMM registers aren't used
#define TTV_CPU_FLAG_FMA3        0x10000 ///< Haswell FMA3 functions
#define TTV_CPU_FLAG_BMI1        0x20000 ///< Bit Manipulation Instruction Set 1
#define TTV_CPU_FLAG_BMI2        0x40000 ///< Bit Manipulation Instruction Set 2

#define TTV_CPU_FLAG_ALTIVEC      0x0001 ///< standard

#define TTV_CPU_FLAG_ARMV5TE      (1 << 0)
#define TTV_CPU_FLAG_ARMV6        (1 << 1)
#define TTV_CPU_FLAG_ARMV6T2      (1 << 2)
#define TTV_CPU_FLAG_VFP          (1 << 3)
#define TTV_CPU_FLAG_VFPV3        (1 << 4)
#define TTV_CPU_FLAG_NEON         (1 << 5)
#define TTV_CPU_FLAG_ARMV8        (1 << 6)
#define TTV_CPU_FLAG_SETEND       (1 <<16)

/**
 * Return the flags which specify extensions supported by the CPU.
 * The returned value is affected by ttv_force_cpu_flags() if that was used
 * before. So ttv_get_cpu_flags() can easily be used in a application to
 * detect the enabled cpu flags.
 */
int ttv_get_cpu_flags(void);


/**
 * @return the number of logical CPU cores present.
 */
int ttv_cpu_count(void);

#endif /* __TTPOD_TT_CPU_H_ */
