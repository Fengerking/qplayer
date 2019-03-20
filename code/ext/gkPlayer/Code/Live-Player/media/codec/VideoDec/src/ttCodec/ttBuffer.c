#include <stdint.h>
#include <string.h>

#include "ttAtomic.h"
#include "ttBufferInternal.h"
#include "ttInternal.h"
#include "ttCommon.h"
#include "ttMem.h"

TTBufferRef *ttv_buffer_create(uint8_t *data, int size,
                              void (*free)(void *opaque, uint8_t *data),
                              void *opaque, int flags)
{
    TTBufferRef *ref = NULL;
    AVBuffer    *buf = NULL;

    buf = ttv_mallocz(sizeof(*buf));
    if (!buf)
        return NULL;

    buf->data     = data;
    buf->size     = size;
    buf->free     = free ? free : ttv_buffer_default_free;
    buf->opaque   = opaque;
    buf->refcount = 1;

    if (flags & TTV_BUFFER_FLAG_READONLY)
        buf->flags |= BUFFER_FLAG_READONLY;

    ref = ttv_mallocz(sizeof(*ref));
    if (!ref) {
        ttv_freep(&buf);
        return NULL;
    }

    ref->buffer = buf;
    ref->data   = data;
    ref->size   = size;

    return ref;
}

void ttv_buffer_default_free(void *opaque, uint8_t *data)
{
    ttv_free(data);
}

TTBufferRef *ttv_buffer_alloc(int size)
{
    TTBufferRef *ret = NULL;
    uint8_t    *data = NULL;

    data = ttv_malloc(size);
    if (!data)
        return NULL;

    ret = ttv_buffer_create(data, size, ttv_buffer_default_free, NULL, 0);
    if (!ret)
        ttv_freep(&data);

    return ret;
}

TTBufferRef *ttv_buffer_allocz(int size)
{
    TTBufferRef *ret = ttv_buffer_alloc(size);
    if (!ret)
        return NULL;

    memset(ret->data, 0, size);
    return ret;
}

TTBufferRef *ttv_buffer_ref(TTBufferRef *buf)
{
    TTBufferRef *ret = ttv_mallocz(sizeof(*ret));

    if (!ret)
        return NULL;

    *ret = *buf;

    ttpriv_atomic_int_add_and_fetch(&buf->buffer->refcount, 1);

    return ret;
}

void ttv_buffer_unref(TTBufferRef **buf)
{
    AVBuffer *b;

    if (!buf || !*buf)
        return;
    b = (*buf)->buffer;
    ttv_freep(buf);

    if (!ttpriv_atomic_int_add_and_fetch(&b->refcount, -1)) {
        b->free(b->opaque, b->data);
        ttv_freep(&b);
    }
}

int ttv_buffer_is_writable(const TTBufferRef *buf)
{
    if (buf->buffer->flags & TTV_BUFFER_FLAG_READONLY)
        return 0;

    return ttpriv_atomic_int_get(&buf->buffer->refcount) == 1;
}

int ttv_buffer_get_ref_count(const TTBufferRef *buf)
{
    return buf->buffer->refcount;
}


int ttv_buffer_realloc(TTBufferRef **pbuf, int size)
{
    TTBufferRef *buf = *pbuf;
    uint8_t *tmp;

    if (!buf) {
        /* allocate a new buffer with ttv_realloc(), so it will be reallocatable
         * later */
        uint8_t *data = ttv_realloc(NULL, size);
        if (!data)
            return AVERROR(ENOMEM);

        buf = ttv_buffer_create(data, size, ttv_buffer_default_free, NULL, 0);
        if (!buf) {
            ttv_freep(&data);
            return AVERROR(ENOMEM);
        }

        buf->buffer->flags |= BUFFER_FLAG_REALLOCATABLE;
        *pbuf = buf;

        return 0;
    } else if (buf->size == size)
        return 0;

    if (!(buf->buffer->flags & BUFFER_FLAG_REALLOCATABLE) ||
        !ttv_buffer_is_writable(buf)) {
        /* cannot realloc, allocate a new reallocable buffer and copy data */
        TTBufferRef *new = NULL;

        ttv_buffer_realloc(&new, size);
        if (!new)
            return AVERROR(ENOMEM);

        memcpy(new->data, buf->data, FFMIN(size, buf->size));

        ttv_buffer_unref(pbuf);
        *pbuf = new;
        return 0;
    }

    tmp = ttv_realloc(buf->buffer->data, size);
    if (!tmp)
        return AVERROR(ENOMEM);

    buf->buffer->data = buf->data = tmp;
    buf->buffer->size = buf->size = size;
    return 0;
}

AVBufferPool *ttv_buffer_pool_init(int size, TTBufferRef* (*alloc)(int size))
{
    AVBufferPool *pool = ttv_mallocz(sizeof(*pool));
    if (!pool)
        return NULL;

    pool->size     = size;
    pool->alloc    = alloc ? alloc : ttv_buffer_alloc;

    ttpriv_atomic_int_set(&pool->refcount, 1);

    return pool;
}

/*
 * This function gets called when the pool has been uninited and
 * all the buffers returned to it.
 */
static void buffer_pool_free(AVBufferPool *pool)
{
    while (pool->pool) {
        BufferPoolEntry *buf = pool->pool;
        pool->pool = buf->next;

        buf->free(buf->opaque, buf->data);
        ttv_freep(&buf);
    }
    ttv_freep(&pool);
}

void ttv_buffer_pool_uninit(AVBufferPool **ppool)
{
    AVBufferPool *pool;

    if (!ppool || !*ppool)
        return;
    pool   = *ppool;
    *ppool = NULL;

    if (!ttpriv_atomic_int_add_and_fetch(&pool->refcount, -1))
        buffer_pool_free(pool);
}

/* remove the whole buffer list from the pool and return it */
static BufferPoolEntry *get_pool(AVBufferPool *pool)
{
    BufferPoolEntry *cur = *(void * volatile *)&pool->pool, *last = NULL;

    while (cur != last) {
        last = cur;
        cur = ttpriv_atomic_ptr_cas((void * volatile *)&pool->pool, last, NULL);
        if (!cur)
            return NULL;
    }

    return cur;
}

static void add_to_pool(BufferPoolEntry *buf)
{
    AVBufferPool *pool;
    BufferPoolEntry *cur, *end = buf;

    if (!buf)
        return;
    pool = buf->pool;

    while (end->next)
        end = end->next;

    while (ttpriv_atomic_ptr_cas((void * volatile *)&pool->pool, NULL, buf)) {
        /* pool is not empty, retrieve it and append it to our list */
        cur = get_pool(pool);
        end->next = cur;
        while (end->next)
            end = end->next;
    }
}

static void pool_release_buffer(void *opaque, uint8_t *data)
{
    BufferPoolEntry *buf = opaque;
    AVBufferPool *pool = buf->pool;

    if(CONFIG_MEMORY_POISONING)
        memset(buf->data, TT_MEMORY_POISON, pool->size);

    add_to_pool(buf);
    if (!ttpriv_atomic_int_add_and_fetch(&pool->refcount, -1))
        buffer_pool_free(pool);
}

/* allocate a new buffer and override its free() callback so that
 * it is returned to the pool on free */
static TTBufferRef *pool_alloc_buffer(AVBufferPool *pool)
{
    BufferPoolEntry *buf;
    TTBufferRef     *ret;

    ret = pool->alloc(pool->size);
    if (!ret)
        return NULL;

    buf = ttv_mallocz(sizeof(*buf));
    if (!buf) {
        ttv_buffer_unref(&ret);
        return NULL;
    }

    buf->data   = ret->buffer->data;
    buf->opaque = ret->buffer->opaque;
    buf->free   = ret->buffer->free;
    buf->pool   = pool;

    ret->buffer->opaque = buf;
    ret->buffer->free   = pool_release_buffer;

    ttpriv_atomic_int_add_and_fetch(&pool->refcount, 1);
    ttpriv_atomic_int_add_and_fetch(&pool->nb_allocated, 1);

    return ret;
}

TTBufferRef *ttv_buffer_pool_get(AVBufferPool *pool)
{
    TTBufferRef *ret;
    BufferPoolEntry *buf;

    /* check whether the pool is empty */
    buf = get_pool(pool);
    if (!buf && pool->refcount <= pool->nb_allocated) {
        ttv_log(NULL, TTV_LOG_DEBUG, "Pool race dectected, spining to avoid overallocation and eventual OOM\n");
        while (!buf && ttpriv_atomic_int_get(&pool->refcount) <= ttpriv_atomic_int_get(&pool->nb_allocated))
            buf = get_pool(pool);
    }

    if (!buf)
        return pool_alloc_buffer(pool);

    /* keep the first entry, return the rest of the list to the pool */
    add_to_pool(buf->next);
    buf->next = NULL;

    ret = ttv_buffer_create(buf->data, pool->size, pool_release_buffer,
                           buf, 0);
    if (!ret) {
        add_to_pool(buf);
        return NULL;
    }
    ttpriv_atomic_int_add_and_fetch(&pool->refcount, 1);

    return ret;
}
