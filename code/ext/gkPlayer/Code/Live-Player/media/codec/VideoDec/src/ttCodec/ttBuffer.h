#ifndef __TTPOD_TT_BUFFER_H_
#define __TTPOD_TT_BUFFER_H_

#include <stdint.h>



/**
 * A reference counted buffer type. It is opaque and is meant to be used through
 * references (TTBufferRef).
 */
typedef struct AVBuffer AVBuffer;

/**
 * A reference to a data buffer.
 *
 * The size of this struct is not a part of the public ABI and it is not meant
 * to be allocated directly.
 */
typedef struct TTBufferRef {
    AVBuffer *buffer;

    /**
     * The data buffer. It is considered writable if and only if
     * this is the only reference to the buffer, in which case
     * ttv_buffer_is_writable() returns 1.
     */
    uint8_t *data;
    /**
     * Size of data in bytes.
     */
    int      size;
} TTBufferRef;

/**
 * Allocate an AVBuffer of the given size using ttv_malloc().
 *
 * @return an TTBufferRef of given size or NULL when out of memory
 */
TTBufferRef *ttv_buffer_alloc(int size);

/**
 * Same as ttv_buffer_alloc(), except the returned buffer will be initialized
 * to zero.
 */
TTBufferRef *ttv_buffer_allocz(int size);

/**
 * Always treat the buffer as read-only, even when it has only one
 * reference.
 */
#define TTV_BUFFER_FLAG_READONLY (1 << 0)

/**
 * Create an AVBuffer from an existing array.
 *
 * If this function is successful, data is owned by the AVBuffer. The caller may
 * only access data through the returned TTBufferRef and references derived from
 * it.
 * If this function fails, data is left untouched.
 * @param data   data array
 * @param size   size of data in bytes
 * @param free   a callback for freeing this buffer's data
 * @param opaque parameter to be got for processing or passed to free
 * @param flags  a combination of TTV_BUFFER_FLAG_*
 *
 * @return an TTBufferRef referring to data on success, NULL on failure.
 */
TTBufferRef *ttv_buffer_create(uint8_t *data, int size,
                              void (*free)(void *opaque, uint8_t *data),
                              void *opaque, int flags);

/**
 * Default free callback, which calls ttv_free() on the buffer data.
 * This function is meant to be passed to ttv_buffer_create(), not called
 * directly.
 */
void ttv_buffer_default_free(void *opaque, uint8_t *data);

/**
 * Create a new reference to an AVBuffer.
 *
 * @return a new TTBufferRef referring to the same AVBuffer as buf or NULL on
 * failure.
 */
TTBufferRef *ttv_buffer_ref(TTBufferRef *buf);

/**
 * Free a given reference and automatically free the buffer if there are no more
 * references to it.
 *
 * @param buf the reference to be freed. The pointer is set to NULL on return.
 */
void ttv_buffer_unref(TTBufferRef **buf);

/**
 * @return 1 if the caller may write to the data referred to by buf (which is
 * true if and only if buf is the only reference to the underlying AVBuffer).
 * Return 0 otherwise.
 * A positive answer is valid until ttv_buffer_ref() is called on buf.
 */
int ttv_buffer_is_writable(const TTBufferRef *buf);


int ttv_buffer_get_ref_count(const TTBufferRef *buf);


/**
 * Reallocate a given buffer.
 *
 * @param buf  a buffer reference to reallocate. On success, buf will be
 *             unreferenced and a new reference with the required size will be
 *             written in its place. On failure buf will be left untouched. *buf
 *             may be NULL, then a new buffer is allocated.
 * @param size required new buffer size.
 * @return 0 on success, a negative AVERROR on failure.
 *
 * @note the buffer is actually reallocated with ttv_realloc() only if it was
 * initially allocated through ttv_buffer_realloc(NULL) and there is only one
 * reference to it (i.e. the one passed to this function). In all other cases
 * a new buffer is allocated and the data is copied.
 */
int ttv_buffer_realloc(TTBufferRef **buf, int size);

/**
 * @}
 */

/**
 * @defgroup lavu_bufferpool AVBufferPool
 * @ingroup lavu_data
 *
 * @{
 * AVBufferPool is an API for a lock-free thread-safe pool of AVBuffers.
 *
 * Frequently allocating and freeing large buffers may be slow. AVBufferPool is
 * meant to solve this in cases when the caller needs a set of buffers of the
 * same size (the most obvious use case being buffers for raw video or audio
 * frames).
 *
 * At the beginning, the user must call ttv_buffer_pool_init() to create the
 * buffer pool. Then whenever a buffer is needed, call ttv_buffer_pool_get() to
 * get a reference to a new buffer, similar to ttv_buffer_alloc(). This new
 * reference works in all aspects the same way as the one created by
 * ttv_buffer_alloc(). However, when the last reference to this buffer is
 * unreferenced, it is returned to the pool instead of being freed and will be
 * reused for subsequent ttv_buffer_pool_get() calls.
 *
 * When the caller is done with the pool and no longer needs to allocate any new
 * buffers, ttv_buffer_pool_uninit() must be called to mark the pool as freeable.
 * Once all the buffers are released, it will automatically be freed.
 *
 * Allocating and releasing buffers with this API is thread-safe as long as
 * either the default alloc callback is used, or the user-supplied one is
 * thread-safe.
 */

/**
 * The buffer pool. This structure is opaque and not meant to be accessed
 * directly. It is allocated with ttv_buffer_pool_init() and freed with
 * ttv_buffer_pool_uninit().
 */
typedef struct AVBufferPool AVBufferPool;

/**
 * Allocate and initialize a buffer pool.
 *
 * @param size size of each buffer in this pool
 * @param alloc a function that will be used to allocate new buffers when the
 * pool is empty. May be NULL, then the default allocator will be used
 * (ttv_buffer_alloc()).
 * @return newly created buffer pool on success, NULL on error.
 */
AVBufferPool *ttv_buffer_pool_init(int size, TTBufferRef* (*alloc)(int size));

/**
 * Mark the pool as being available for freeing. It will actually be freed only
 * once all the allocated buffers associated with the pool are released. Thus it
 * is safe to call this function while some of the allocated buffers are still
 * in use.
 *
 * @param pool pointer to the pool to be freed. It will be set to NULL.
 * @see ttv_buffer_pool_can_uninit()
 */
void ttv_buffer_pool_uninit(AVBufferPool **pool);

/**
 * Allocate a new AVBuffer, reusing an old buffer from the pool when available.
 * This function may be called simultaneously from multiple threads.
 *
 * @return a reference to the new buffer on success, NULL on error.
 */
TTBufferRef *ttv_buffer_pool_get(AVBufferPool *pool);

/**
 * @}
 */

#endif /* __TTPOD_TT_BUFFER_H_ */
