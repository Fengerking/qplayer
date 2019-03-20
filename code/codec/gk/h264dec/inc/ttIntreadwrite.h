#ifndef __TTPOD_TT_INTREADWRITE_H_
#define __TTPOD_TT_INTREADWRITE_H_

#include <stdint.h>
#include "ttAttributes.h"

typedef union {
    uint64_t u64;
    uint32_t u32[2];
    uint16_t u16[4];
    uint8_t  u8 [8];
    double   f64;
    float    f32[2];
} ttv_alias ttv_alias64;

typedef union {
    uint32_t u32;
    uint16_t u16[2];
    uint8_t  u8 [4];
    float    f32;
} ttv_alias ttv_alias32;

typedef union {
    uint16_t u16;
    uint8_t  u8 [2];
} ttv_alias ttv_alias16;

/*
 * Arch-specific headers can provide any combination of
 * TTV_[RW][BLN](16|24|32|48|64) and TTV_(COPY|SWAP|ZERO)(64|128) macros.
 * Preprocessor symbols must be defined, even if these are implemented
 * as inline functions.
 *
 * R/W means read/write, B/L/N means big/little/native endianness.
 * The following macros require aligned access, compared to their
 * unaligned variants: TTV_(COPY|SWAP|ZERO)(64|128), TTV_[RW]N[8-64]A.
 * Incorrect usage may range from abysmal performance to crash
 * depending on the platform.
 *
 * The unaligned variants are TTV_[RW][BLN][8-64] and TTV_COPY*U.
 */
#include "config.h"
#ifdef HAVE_TTV_CONFIG_H

#if   ARCH_ARM
#   include "arm/intreadwrite.h"
#elif ARCH_AVR32
#   include "avr32/intreadwrite.h"
#elif ARCH_MIPS
#   include "mips/intreadwrite.h"
#elif ARCH_PPC
#   include "ppc/intreadwrite.h"
#elif ARCH_TOMI
#   include "tomi/intreadwrite.h"
#elif ARCH_X86
#   include "x86/intreadwrite.h"
#endif

#endif /* HAVE_TTV_CONFIG_H */

/*
 * Map TTV_RNXX <-> TTV_R[BL]XX for all variants provided by per-arch headers.
 */

#if TTV_HAVE_BIGENDIAN

#   if    defined(TTV_RN16) && !defined(TTV_RB16)
#       define TTV_RB16(p) TTV_RN16(p)
#   elif !defined(TTV_RN16) &&  defined(TTV_RB16)
#       define TTV_RN16(p) TTV_RB16(p)
#   endif

#   if    defined(TTV_WN16) && !defined(TTV_WB16)
#       define TTV_WB16(p, v) TTV_WN16(p, v)
#   elif !defined(TTV_WN16) &&  defined(TTV_WB16)
#       define TTV_WN16(p, v) TTV_WB16(p, v)
#   endif

#   if    defined(TTV_RN24) && !defined(TTV_RB24)
#       define TTV_RB24(p) TTV_RN24(p)
#   elif !defined(TTV_RN24) &&  defined(TTV_RB24)
#       define TTV_RN24(p) TTV_RB24(p)
#   endif

#   if    defined(TTV_WN24) && !defined(TTV_WB24)
#       define TTV_WB24(p, v) TTV_WN24(p, v)
#   elif !defined(TTV_WN24) &&  defined(TTV_WB24)
#       define TTV_WN24(p, v) TTV_WB24(p, v)
#   endif

#   if    defined(TTV_RN32) && !defined(TTV_RB32)
#       define TTV_RB32(p) TTV_RN32(p)
#   elif !defined(TTV_RN32) &&  defined(TTV_RB32)
#       define TTV_RN32(p) TTV_RB32(p)
#   endif

#   if    defined(TTV_WN32) && !defined(TTV_WB32)
#       define TTV_WB32(p, v) TTV_WN32(p, v)
#   elif !defined(TTV_WN32) &&  defined(TTV_WB32)
#       define TTV_WN32(p, v) TTV_WB32(p, v)
#   endif

#   if    defined(TTV_RN48) && !defined(TTV_RB48)
#       define TTV_RB48(p) TTV_RN48(p)
#   elif !defined(TTV_RN48) &&  defined(TTV_RB48)
#       define TTV_RN48(p) TTV_RB48(p)
#   endif

#   if    defined(TTV_WN48) && !defined(TTV_WB48)
#       define TTV_WB48(p, v) TTV_WN48(p, v)
#   elif !defined(TTV_WN48) &&  defined(TTV_WB48)
#       define TTV_WN48(p, v) TTV_WB48(p, v)
#   endif

#   if    defined(TTV_RN64) && !defined(TTV_RB64)
#       define TTV_RB64(p) TTV_RN64(p)
#   elif !defined(TTV_RN64) &&  defined(TTV_RB64)
#       define TTV_RN64(p) TTV_RB64(p)
#   endif

#   if    defined(TTV_WN64) && !defined(TTV_WB64)
#       define TTV_WB64(p, v) TTV_WN64(p, v)
#   elif !defined(TTV_WN64) &&  defined(TTV_WB64)
#       define TTV_WN64(p, v) TTV_WB64(p, v)
#   endif

#else /* TTV_HAVE_BIGENDIAN */

#   if    defined(TTV_RN16) && !defined(TTV_RL16)
#       define TTV_RL16(p) TTV_RN16(p)
#   elif !defined(TTV_RN16) &&  defined(TTV_RL16)
#       define TTV_RN16(p) TTV_RL16(p)
#   endif

#   if    defined(TTV_WN16) && !defined(TTV_WL16)
#       define TTV_WL16(p, v) TTV_WN16(p, v)
#   elif !defined(TTV_WN16) &&  defined(TTV_WL16)
#       define TTV_WN16(p, v) TTV_WL16(p, v)
#   endif

#   if    defined(TTV_RN24) && !defined(TTV_RL24)
#       define TTV_RL24(p) TTV_RN24(p)
#   elif !defined(TTV_RN24) &&  defined(TTV_RL24)
#       define TTV_RN24(p) TTV_RL24(p)
#   endif

#   if    defined(TTV_WN24) && !defined(TTV_WL24)
#       define TTV_WL24(p, v) TTV_WN24(p, v)
#   elif !defined(TTV_WN24) &&  defined(TTV_WL24)
#       define TTV_WN24(p, v) TTV_WL24(p, v)
#   endif

#   if    defined(TTV_RN32) && !defined(TTV_RL32)
#       define TTV_RL32(p) TTV_RN32(p)
#   elif !defined(TTV_RN32) &&  defined(TTV_RL32)
#       define TTV_RN32(p) TTV_RL32(p)
#   endif

#   if    defined(TTV_WN32) && !defined(TTV_WL32)
#       define TTV_WL32(p, v) TTV_WN32(p, v)
#   elif !defined(TTV_WN32) &&  defined(TTV_WL32)
#       define TTV_WN32(p, v) TTV_WL32(p, v)
#   endif

#   if    defined(TTV_RN48) && !defined(TTV_RL48)
#       define TTV_RL48(p) TTV_RN48(p)
#   elif !defined(TTV_RN48) &&  defined(TTV_RL48)
#       define TTV_RN48(p) TTV_RL48(p)
#   endif

#   if    defined(TTV_WN48) && !defined(TTV_WL48)
#       define TTV_WL48(p, v) TTV_WN48(p, v)
#   elif !defined(TTV_WN48) &&  defined(TTV_WL48)
#       define TTV_WN48(p, v) TTV_WL48(p, v)
#   endif

#   if    defined(TTV_RN64) && !defined(TTV_RL64)
#       define TTV_RL64(p) TTV_RN64(p)
#   elif !defined(TTV_RN64) &&  defined(TTV_RL64)
#       define TTV_RN64(p) TTV_RL64(p)
#   endif

#   if    defined(TTV_WN64) && !defined(TTV_WL64)
#       define TTV_WL64(p, v) TTV_WN64(p, v)
#   elif !defined(TTV_WN64) &&  defined(TTV_WL64)
#       define TTV_WN64(p, v) TTV_WL64(p, v)
#   endif

#endif /* !TTV_HAVE_BIGENDIAN */

/*
 * Define TTV_[RW]N helper macros to simplify definitions not provided
 * by per-arch headers.
 */

#if defined(__GNUC__) && !defined(__TI_COMPILER_VERSION__)

union unaligned_64 { uint64_t l; } __attribute__((packed)) ttv_alias;
union unaligned_32 { uint32_t l; } __attribute__((packed)) ttv_alias;
union unaligned_16 { uint16_t l; } __attribute__((packed)) ttv_alias;

#   define TTV_RN(s, p) (((const union unaligned_##s *) (p))->l)
#   define TTV_WN(s, p, v) ((((union unaligned_##s *) (p))->l) = (v))

#elif defined(__DECC)

#   define TTV_RN(s, p) (*((const __unaligned uint##s##_t*)(p)))
#   define TTV_WN(s, p, v) (*((__unaligned uint##s##_t*)(p)) = (v))

#elif TTV_HAVE_FAST_UNALIGNED

#   define TTV_RN(s, p) (((const ttv_alias##s*)(p))->u##s)
#   define TTV_WN(s, p, v) (((ttv_alias##s*)(p))->u##s = (v))

#else

#ifndef TTV_RB16
#   define TTV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])
#endif
#ifndef TTV_WB16
#   define TTV_WB16(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[1] = (d);               \
        ((uint8_t*)(p))[0] = (d)>>8;            \
    } while(0)
#endif

#ifndef TTV_RL16
#   define TTV_RL16(x)                           \
    ((((const uint8_t*)(x))[1] << 8) |          \
      ((const uint8_t*)(x))[0])
#endif
#ifndef TTV_WL16
#   define TTV_WL16(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
    } while(0)
#endif

#ifndef TTV_RB32
#   define TTV_RB32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])
#endif
#ifndef TTV_WB32
#   define TTV_WB32(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif

#ifndef TTV_RL32
#   define TTV_RL32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[3] << 24) |    \
               (((const uint8_t*)(x))[2] << 16) |    \
               (((const uint8_t*)(x))[1] <<  8) |    \
                ((const uint8_t*)(x))[0])
#endif
#ifndef TTV_WL32
#   define TTV_WL32(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
    } while(0)
#endif

#ifndef TTV_RB64
#   define TTV_RB64(x)                                   \
    (((uint64_t)((const uint8_t*)(x))[0] << 56) |       \
     ((uint64_t)((const uint8_t*)(x))[1] << 48) |       \
     ((uint64_t)((const uint8_t*)(x))[2] << 40) |       \
     ((uint64_t)((const uint8_t*)(x))[3] << 32) |       \
     ((uint64_t)((const uint8_t*)(x))[4] << 24) |       \
     ((uint64_t)((const uint8_t*)(x))[5] << 16) |       \
     ((uint64_t)((const uint8_t*)(x))[6] <<  8) |       \
      (uint64_t)((const uint8_t*)(x))[7])
#endif
#ifndef TTV_WB64
#   define TTV_WB64(p, darg) do {                \
        uint64_t d = (darg);                    \
        ((uint8_t*)(p))[7] = (d);               \
        ((uint8_t*)(p))[6] = (d)>>8;            \
        ((uint8_t*)(p))[5] = (d)>>16;           \
        ((uint8_t*)(p))[4] = (d)>>24;           \
        ((uint8_t*)(p))[3] = (d)>>32;           \
        ((uint8_t*)(p))[2] = (d)>>40;           \
        ((uint8_t*)(p))[1] = (d)>>48;           \
        ((uint8_t*)(p))[0] = (d)>>56;           \
    } while(0)
#endif

#ifndef TTV_RL64
#   define TTV_RL64(x)                                   \
    (((uint64_t)((const uint8_t*)(x))[7] << 56) |       \
     ((uint64_t)((const uint8_t*)(x))[6] << 48) |       \
     ((uint64_t)((const uint8_t*)(x))[5] << 40) |       \
     ((uint64_t)((const uint8_t*)(x))[4] << 32) |       \
     ((uint64_t)((const uint8_t*)(x))[3] << 24) |       \
     ((uint64_t)((const uint8_t*)(x))[2] << 16) |       \
     ((uint64_t)((const uint8_t*)(x))[1] <<  8) |       \
      (uint64_t)((const uint8_t*)(x))[0])
#endif
#ifndef TTV_WL64
#   define TTV_WL64(p, darg) do {                \
        uint64_t d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
        ((uint8_t*)(p))[4] = (d)>>32;           \
        ((uint8_t*)(p))[5] = (d)>>40;           \
        ((uint8_t*)(p))[6] = (d)>>48;           \
        ((uint8_t*)(p))[7] = (d)>>56;           \
    } while(0)
#endif

#if TTV_HAVE_BIGENDIAN
#   define TTV_RN(s, p)    TTV_RB##s(p)
#   define TTV_WN(s, p, v) TTV_WB##s(p, v)
#else
#   define TTV_RN(s, p)    TTV_RL##s(p)
#   define TTV_WN(s, p, v) TTV_WL##s(p, v)
#endif

#endif /* HAVE_FAST_UNALIGNED */

#ifndef TTV_RN16
#   define TTV_RN16(p) TTV_RN(16, p)
#endif

#ifndef TTV_RN32
#   define TTV_RN32(p) TTV_RN(32, p)
#endif

#ifndef TTV_RN64
#   define TTV_RN64(p) TTV_RN(64, p)
#endif

#ifndef TTV_WN16
#   define TTV_WN16(p, v) TTV_WN(16, p, v)
#endif

#ifndef TTV_WN32
#   define TTV_WN32(p, v) TTV_WN(32, p, v)
#endif

#ifndef TTV_WN64
#   define TTV_WN64(p, v) TTV_WN(64, p, v)
#endif

#if TTV_HAVE_BIGENDIAN
#   define TTV_RB(s, p)    TTV_RN##s(p)
#   define TTV_WB(s, p, v) TTV_WN##s(p, v)
#   define TTV_RL(s, p)    ttv_bswap##s(TTV_RN##s(p))
#   define TTV_WL(s, p, v) TTV_WN##s(p, ttv_bswap##s(v))
#else
#   define TTV_RB(s, p)    ttv_bswap##s(TTV_RN##s(p))
#   define TTV_WB(s, p, v) TTV_WN##s(p, ttv_bswap##s(v))
#   define TTV_RL(s, p)    TTV_RN##s(p)
#   define TTV_WL(s, p, v) TTV_WN##s(p, v)
#endif

#define TTV_RB8(x)     (((const uint8_t*)(x))[0])
#define TTV_WB8(p, d)  do { ((uint8_t*)(p))[0] = (d); } while(0)

#define TTV_RL8(x)     TTV_RB8(x)
#define TTV_WL8(p, d)  TTV_WB8(p, d)

#ifndef TTV_RB16
#   define TTV_RB16(p)    TTV_RB(16, p)
#endif
#ifndef TTV_WB16
#   define TTV_WB16(p, v) TTV_WB(16, p, v)
#endif

#ifndef TTV_RL16
#   define TTV_RL16(p)    TTV_RL(16, p)
#endif
#ifndef TTV_WL16
#   define TTV_WL16(p, v) TTV_WL(16, p, v)
#endif

#ifndef TTV_RB32
#   define TTV_RB32(p)    TTV_RB(32, p)
#endif
#ifndef TTV_WB32
#   define TTV_WB32(p, v) TTV_WB(32, p, v)
#endif

#ifndef TTV_RL32
#   define TTV_RL32(p)    TTV_RL(32, p)
#endif
#ifndef TTV_WL32
#   define TTV_WL32(p, v) TTV_WL(32, p, v)
#endif

#ifndef TTV_RB64
#   define TTV_RB64(p)    TTV_RB(64, p)
#endif
#ifndef TTV_WB64
#   define TTV_WB64(p, v) TTV_WB(64, p, v)
#endif

#ifndef TTV_RL64
#   define TTV_RL64(p)    TTV_RL(64, p)
#endif
#ifndef TTV_WL64
#   define TTV_WL64(p, v) TTV_WL(64, p, v)
#endif

#ifndef TTV_RB24
#   define TTV_RB24(x)                           \
    ((((const uint8_t*)(x))[0] << 16) |         \
     (((const uint8_t*)(x))[1] <<  8) |         \
      ((const uint8_t*)(x))[2])
#endif
#ifndef TTV_WB24
#   define TTV_WB24(p, d) do {                   \
        ((uint8_t*)(p))[2] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[0] = (d)>>16;           \
    } while(0)
#endif

#ifndef TTV_RL24
#   define TTV_RL24(x)                           \
    ((((const uint8_t*)(x))[2] << 16) |         \
     (((const uint8_t*)(x))[1] <<  8) |         \
      ((const uint8_t*)(x))[0])
#endif
#ifndef TTV_WL24
#   define TTV_WL24(p, d) do {                   \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
    } while(0)
#endif

#ifndef TTV_RB48
#   define TTV_RB48(x)                                     \
    (((uint64_t)((const uint8_t*)(x))[0] << 40) |         \
     ((uint64_t)((const uint8_t*)(x))[1] << 32) |         \
     ((uint64_t)((const uint8_t*)(x))[2] << 24) |         \
     ((uint64_t)((const uint8_t*)(x))[3] << 16) |         \
     ((uint64_t)((const uint8_t*)(x))[4] <<  8) |         \
      (uint64_t)((const uint8_t*)(x))[5])
#endif
#ifndef TTV_WB48
#   define TTV_WB48(p, darg) do {                \
        uint64_t d = (darg);                    \
        ((uint8_t*)(p))[5] = (d);               \
        ((uint8_t*)(p))[4] = (d)>>8;            \
        ((uint8_t*)(p))[3] = (d)>>16;           \
        ((uint8_t*)(p))[2] = (d)>>24;           \
        ((uint8_t*)(p))[1] = (d)>>32;           \
        ((uint8_t*)(p))[0] = (d)>>40;           \
    } while(0)
#endif

#ifndef TTV_RL48
#   define TTV_RL48(x)                                     \
    (((uint64_t)((const uint8_t*)(x))[5] << 40) |         \
     ((uint64_t)((const uint8_t*)(x))[4] << 32) |         \
     ((uint64_t)((const uint8_t*)(x))[3] << 24) |         \
     ((uint64_t)((const uint8_t*)(x))[2] << 16) |         \
     ((uint64_t)((const uint8_t*)(x))[1] <<  8) |         \
      (uint64_t)((const uint8_t*)(x))[0])
#endif
#ifndef TTV_WL48
#   define TTV_WL48(p, darg) do {                \
        uint64_t d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
        ((uint8_t*)(p))[4] = (d)>>32;           \
        ((uint8_t*)(p))[5] = (d)>>40;           \
    } while(0)
#endif

/*
 * The TTV_[RW]NA macros access naturally aligned data
 * in a type-safe way.
 */

#define TTV_RNA(s, p)    (((const ttv_alias##s*)(p))->u##s)
#define TTV_WNA(s, p, v) (((ttv_alias##s*)(p))->u##s = (v))

#ifndef TTV_RN16A
#   define TTV_RN16A(p) TTV_RNA(16, p)
#endif

#ifndef TTV_RN32A
#   define TTV_RN32A(p) TTV_RNA(32, p)
#endif

#ifndef TTV_RN64A
#   define TTV_RN64A(p) TTV_RNA(64, p)
#endif

#ifndef TTV_WN16A
#   define TTV_WN16A(p, v) TTV_WNA(16, p, v)
#endif

#ifndef TTV_WN32A
#   define TTV_WN32A(p, v) TTV_WNA(32, p, v)
#endif

#ifndef TTV_WN64A
#   define TTV_WN64A(p, v) TTV_WNA(64, p, v)
#endif


#define TTV_COPYU(n, d, s) TTV_WN##n(d, TTV_RN##n(s));

#ifndef TTV_COPY64U
#   define TTV_COPY64U(d, s) TTV_COPYU(64, d, s)
#endif

#ifndef TTV_COPY128U
#   define TTV_COPY128U(d, s)                                    \
    do {                                                        \
        TTV_COPY64U(d, s);                                       \
        TTV_COPY64U((char *)(d) + 8, (const char *)(s) + 8);     \
    } while(0)
#endif

/* Parameters for TTV_COPY*, TTV_SWAP*, TTV_ZERO* must be
 * naturally aligned. They may be implemented using MMX,
 * so emms_c() must be called before using any float code
 * afterwards.
 */

#define TTV_COPY(n, d, s) \
    (((ttv_alias##n*)(d))->u##n = ((const ttv_alias##n*)(s))->u##n)

#ifndef TTV_COPY16
#   define TTV_COPY16(d, s) TTV_COPY(16, d, s)
#endif

#ifndef TTV_COPY32
#   define TTV_COPY32(d, s) TTV_COPY(32, d, s)
#endif

#ifndef TTV_COPY64
#   define TTV_COPY64(d, s) TTV_COPY(64, d, s)
#endif

#ifndef TTV_COPY128
#   define TTV_COPY128(d, s)                    \
    do {                                       \
        TTV_COPY64(d, s);                       \
        TTV_COPY64((char*)(d)+8, (char*)(s)+8); \
    } while(0)
#endif

#define TTV_SWAP(n, a, b) FFSWAP(ttv_alias##n, *(ttv_alias##n*)(a), *(ttv_alias##n*)(b))

#ifndef TTV_SWAP64
#   define TTV_SWAP64(a, b) TTV_SWAP(64, a, b)
#endif

#define TTV_ZERO(n, d) (((ttv_alias##n*)(d))->u##n = 0)

#ifndef TTV_ZERO16
#   define TTV_ZERO16(d) TTV_ZERO(16, d)
#endif

#ifndef TTV_ZERO32
#   define TTV_ZERO32(d) TTV_ZERO(32, d)
#endif

#ifndef TTV_ZERO64
#   define TTV_ZERO64(d) TTV_ZERO(64, d)
#endif

#ifndef TTV_ZERO128
#   define TTV_ZERO128(d)         \
    do {                         \
        TTV_ZERO64(d);            \
        TTV_ZERO64((char*)(d)+8); \
    } while(0)
#endif

#endif /* __TTPOD_TT_INTREADWRITE_H_ */
