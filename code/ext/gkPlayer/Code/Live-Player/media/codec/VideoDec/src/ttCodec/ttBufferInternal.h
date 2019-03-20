#ifndef __TTPOD_TT_BUFFER_INTERNAL_H_
#define __TTPOD_TT_BUFFER_INTERNAL_H_

#include <stdint.h>

#include "ttBuffer.h"

/**
 * The buffer is always treated as read-only.
 */
#define BUFFER_FLAG_READONLY      (1 << 0)
/**
 * The buffer was ttv_realloc()ed, so it is reallocatable.
 */
#define BUFFER_FLAG_REALLOCATABLE (1 << 1)

struct AVBuffer {
    uint8_t *data; /**< data described by this buffer */
    int      size; /**< size of data in bytes */

    /**
     *  number of existing TTBufferRef instances referring to this buffer
     */
    volatile int refcount;

    /**
     * a callback for freeing the data
     */
    void (*free)(void *opaque, uint8_t *data);

    /**
     * an opaque pointer, to be used by the freeing callback
     */
    void *opaque;

    /**
     * A combination of BUFFER_FLAG_*
     */
    int flags;
};

typedef struct BufferPoolEntry {
    uint8_t *data;

    /*
     * Backups of the original opaque/free of the AVBuffer corresponding to
     * data. They will be used to free the buffer when the pool is freed.
     */
    void *opaque;
    void (*free)(void *opaque, uint8_t *data);

    AVBufferPool *pool;
    struct BufferPoolEntry * volatile next;
} BufferPoolEntry;

struct AVBufferPool {
    BufferPoolEntry * volatile pool;

    /*
     * This is used to track when the pool is to be freed.
     * The pointer to the pool itself held by the caller is considered to
     * be one reference. Each buffer requested by the caller increases refcount
     * by one, returning the buffer to the pool decreases it by one.
     * refcount reaches zero when the buffer has been uninited AND all the
     * buffers have been released, then it's safe to free the pool and all
     * the buffers in it.
     */
    volatile int refcount;

    volatile int nb_allocated;

    int size;
    TTBufferRef* (*alloc)(int size);
};

#endif /* __TTPOD_TT_BUFFER_INTERNAL_H_ */
