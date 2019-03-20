#ifndef __TTPOD_TT_INTERNAL_H_
#define __TTPOD_TT_INTERNAL_H_

#if !defined(DEBUG) && !defined(NDEBUG)
#    define NDEBUG
#endif

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "config.h"
#include "ttAttributes.h"
#include "ttCpu.h"
#include "ttDict.h"
#include "ttPixfmt.h"
#include "ttVersion.h"
#include "ttFrame.h"

#if ARCH_X86
#   include "x86/emms.h"
#endif

#ifndef emms_c
#   define emms_c()
#endif

#ifndef attribute_align_arg
#if ARCH_X86_32 && TTV_GCC_VERSION_AT_LEAST(4,2)
#    define attribute_align_arg __attribute__((force_align_arg_pointer))
#else
#    define attribute_align_arg
#endif
#endif

#if defined(_MSC_VER) && CONFIG_SHARED
#    define ttv_export __declspec(dllimport)
#else
#    define ttv_export
#endif

#if HAVE_PRAGMA_DEPRECATED
#    if defined(__ICL) || defined (__INTEL_COMPILER)
#        define TT_DISABLE_DEPRECATION_WARNINGS __pragma(warning(push)) __pragma(warning(disable:1478))
#        define TT_ENABLE_DEPRECATION_WARNINGS  __pragma(warning(pop))
#    elif defined(_MSC_VER)
#        define TT_DISABLE_DEPRECATION_WARNINGS __pragma(warning(push)) __pragma(warning(disable:4996))
#        define TT_ENABLE_DEPRECATION_WARNINGS  __pragma(warning(pop))
#    else
#        define TT_DISABLE_DEPRECATION_WARNINGS _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#        define TT_ENABLE_DEPRECATION_WARNINGS  _Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")
#    endif
#else
#    define TT_DISABLE_DEPRECATION_WARNINGS
#    define TT_ENABLE_DEPRECATION_WARNINGS
#endif

#ifndef INT_BIT
#    define INT_BIT (CHAR_BIT * sizeof(int))
#endif

#define TT_MEMORY_POISON 0x2a

#define TTV_CHECK_OFFSET(s, m, o) struct check_##o {    \
	int x_##o[offsetof(s, m) == o? 1: -1];         \
	}


#define MAKE_ACCESSORS(str, name, type, field) \
    type ttv_##name##_get_##field(const str *s) { return s->field; } \
    void ttv_##name##_set_##field(str *s, type v) { s->field = v; }

// Some broken preprocessors need a second expansion
// to be forced to tokenize __VA_ARGS__
#define E1(x) x

#define LOCAL_ALIGNED_A(a, t, v, s, o, ...)             \
    uint8_t la_##v[sizeof(t s o) + (a)];                \
    t (*v) o = (void *)FFALIGN((uintptr_t)la_##v, a)

#define LOCAL_ALIGNED_D(a, t, v, s, o, ...)             \
    DECLARE_ALIGNED(a, t, la_##v) s o;                  \
    t (*v) o = la_##v

#define LOCAL_ALIGNED(a, t, v, ...) E1(LOCAL_ALIGNED_A(a, t, v, __VA_ARGS__,,))

#if HAVE_LOCAL_ALIGNED_8
#   define LOCAL_ALIGNED_8(t, v, ...) E1(LOCAL_ALIGNED_D(8, t, v, __VA_ARGS__,,))
#else
#   define LOCAL_ALIGNED_8(t, v, ...) LOCAL_ALIGNED(8, t, v, __VA_ARGS__)
#endif

#define TT_ALLOC_OR_GOTO(ctx, p, size, label)\
{\
    p = ttv_malloc(size);\
    if (!(p) && (size) != 0) {\
        ttv_log(ctx, TTV_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#define TT_ALLOCZ_OR_GOTO(ctx, p, size, label)\
{\
    p = ttv_mallocz(size);\
    if (!(p) && (size) != 0) {\
        ttv_log(ctx, TTV_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#define TT_ALLOC_ARRAY_OR_GOTO(ctx, p, nelem, elsize, label)\
{\
    p = ttv_malloc_array(nelem, elsize);\
    if (!p) {\
        ttv_log(ctx, TTV_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#define TT_ALLOCZ_ARRAY_OR_GOTO(ctx, p, nelem, elsize, label)\
{\
    p = ttv_mallocz_array(nelem, elsize);\
    if (!p) {\
        ttv_log(ctx, TTV_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#include "ttLibm.h"


#if CONFIG_SMALL
#   define NULL_IF_CONFIG_SMALL(x) NULL
#else
#   define NULL_IF_CONFIG_SMALL(x) x
#endif

/**
 * Define a function with only the non-default version specified.
 *
 * On systems with ELF shared libraries, all symbols exported from
 * FFmpeg libraries are tagged with the name and major version of the
 * library to which they belong.  If a function is moved from one
 * library to another, a wrapper must be retained in the original
 * location to preserve binary compatibility.
 *
 * Functions defined with this macro will never be used to resolve
 * symbols by the build-time linker.
 *
 * @param type return type of function
 * @param name name of function
 * @param args argument list of function
 * @param ver  version tag to assign function
 */
#if HAVE_SYMVER_ASM_LABEL
#   define TT_SYMVER(type, name, args, ver)                     \
    type tt_##name args __asm__ (EXTERN_PREFIX #name "@" ver);  \
    type tt_##name args
#elif HAVE_SYMVER_GNU_ASM
#   define TT_SYMVER(type, name, args, ver)                             \
    __asm__ (".symver tt_" #name "," EXTERN_PREFIX #name "@" ver);      \
    type tt_##name args;                                                \
    type tt_##name args
#endif

/**
 * Return NULL if a threading library has not been enabled.
 * Used to disable threading functions in TTCodec definitions
 * when not needed.
 */
#if HAVE_THREADS
#   define ONLY_IF_THREADS_ENABLED(x) x
#else
#   define ONLY_IF_THREADS_ENABLED(x) NULL
#endif

#if HAVE_LIBC_MSVCRT
#define ttpriv_open tt_open
#define PTRDITT_SPECIFIER "Id"
#define SIZE_SPECIFIER "Iu"
#else
#define PTRDITT_SPECIFIER "td"
#define SIZE_SPECIFIER "zu"
#endif



int ttpriv_set_systematic_pal2(uint32_t pal[256], enum TTPixelFormat pix_fmt);



#include <stdint.h>

#include "ttBuffer.h"
#include "ttMathematics.h"
#include "ttPixfmt.h"
#include "ttAvcodec.h"
#include "config.h"

#define TT_SANE_NB_CHANNELS 63U

#define TT_SIGNBIT(x) ((x) >> CHAR_BIT * sizeof(x) - 1)

#if HAVE_AVX
#   define STRIDE_ALIGN 32
#elif HAVE_SIMD_ALIGN_16
#   define STRIDE_ALIGN 16
#else
#   define STRIDE_ALIGN 8
#endif

typedef struct FramePool {
    /**
     * Pools for each data plane. For audio all the planes have the same size,
     * so only pools[0] is used.
     */
    AVBufferPool *pools[4];

    /*
     * Pool parameters
     */
    int format;
    int width, height;
    int stride_align[TTV_NUM_DATA_POINTERS];
    int linesize[4];
    int planes;
    int channels;
    int samples;
} FramePool;

typedef struct TTCodecInternal {
    /**
     * Whether the parent TTCodecContext is a copy of the context which had
     * init() called on it.
     * This is used by multithreading - shared tables and picture pointers
     * should be freed from the original context only.
     */
    int is_copy;

    /**
     * Whether to allocate progress for frame threading.
     *
     * The codec must set it to 1 if it uses tt_thread_await/report_progress(),
     * then progress will be allocated in tt_thread_get_buffer(). The frames
     * then MUST be freed with tt_thread_release_buffer().
     *
     * If the codec does not need to call the progress functions (there are no
     * dependencies between the frames), it should leave this at 0. Then it can
     * decode straight to the user-provided frames (which the user will then
     * free with ttv_frame_unref()), there is no need to call
     * tt_thread_release_buffer().
     */
    int allocate_progress;

#if TT_API_OLD_ENCODE_AUDIO
    /**
     * Internal sample count used by ttcodec_encode_audio() to fabricate pts.
     * Can be removed along with ttcodec_encode_audio().
     */
    int64_t sample_count;
#endif

    /**
     * An audio frame with less than required samples has been submitted and
     * padded with silence. Reject all subsequent frames.
     */
    int last_audio_frame;

    TTFrame *to_free;

    FramePool *pool;

    void *thread_ctx;

    /**
     * Current packet as passed into the decoder, to avoid having to pass the
     * packet into every function.
     */
    TTPacket *pkt;

    /**
     * temporary buffer used for encoders to store their bitstream
     */
    uint8_t *byte_buffer;
    unsigned int byte_buffer_size;

    void *frame_thread_encoder;

    /**
     * Number of audio samples to skip at the start of the next decoded frame
     */
    int skip_samples;

    /**
     * hwaccel-specific private data
     */
    void *hwaccel_priv_data;
} TTCodecInternal;



int tt_init_buffer_info(TTCodecContext *s, TTFrame *frame);


void ttpriv_color_frame(TTFrame *frame, const int color[4]);

extern volatile int tt_ttcodec_locked;
int tt_lock_avcodec(TTCodecContext *log_ctx);
int tt_unlock_avcodec(void);


#define TT_MAX_EXTRADATA_SIZE ((1 << 28) - TT_INPUT_BUFFER_PADDING_SIZE)



int tt_get_buffer(TTCodecContext *avctx, TTFrame *frame, int flags);

int tt_thread_can_start_frame(TTCodecContext *avctx);

const uint8_t *ttpriv_find_start_code(const uint8_t *p,
                                      const uint8_t *end,
                                      uint32_t *state);

/**
 * Check that the provided frame dimensions are valid and set them on the codec
 * context.
 */
int tt_set_dimensions(TTCodecContext *s, int width, int height);

/**
 * Check that the provided sample aspect ratio is valid and set it on the codec
 * context.
 */
int tt_set_sar(TTCodecContext *avctx, TTRational sar);

/**
 * Select the (possibly hardware accelerated) pixel format.
 * This is a wrapper around TTCodecContext.get_format() and should be used
 * instead of calling get_format() directly.
 */
int tt_get_format(TTCodecContext *avctx, const enum TTPixelFormat *fmt);

/**
 * Set various frame properties from the codec context / packet data.
 */
int tt_decode_frame_props(TTCodecContext *avctx, TTFrame *frame);

#endif /* __TTPOD_TT_INTERNAL_H_ */
