#include "config.h"
#include "ttAtomic.h"

#if !HAVE_ATOMICS_NATIVE

#if HAVE_PTHREADS

#include <pthread.h>

static pthread_mutex_t atomic_lock = PTHREAD_MUTEX_INITIALIZER;

int ttpriv_atomic_int_get(volatile int *ptr)
{
    int res;

    pthread_mutex_lock(&atomic_lock);
    res = *ptr;
    pthread_mutex_unlock(&atomic_lock);

    return res;
}

void ttpriv_atomic_int_set(volatile int *ptr, int val)
{
    pthread_mutex_lock(&atomic_lock);
    *ptr = val;
    pthread_mutex_unlock(&atomic_lock);
}

int ttpriv_atomic_int_add_and_fetch(volatile int *ptr, int inc)
{
    int res;

    pthread_mutex_lock(&atomic_lock);
    *ptr += inc;
    res = *ptr;
    pthread_mutex_unlock(&atomic_lock);

    return res;
}

void *ttpriv_atomic_ptr_cas(void * volatile *ptr, void *oldval, void *newval)
{
    void *ret;
    pthread_mutex_lock(&atomic_lock);
    ret = *ptr;
    if (*ptr == oldval)
        *ptr = newval;
    pthread_mutex_unlock(&atomic_lock);
    return ret;
}

#elif !HAVE_THREADS

int ttpriv_atomic_int_get(volatile int *ptr)
{
    return *ptr;
}

void ttpriv_atomic_int_set(volatile int *ptr, int val)
{
    *ptr = val;
}

int ttpriv_atomic_int_add_and_fetch(volatile int *ptr, int inc)
{
    *ptr += inc;
    return *ptr;
}

void *ttpriv_atomic_ptr_cas(void * volatile *ptr, void *oldval, void *newval)
{
    if (*ptr == oldval) {
        *ptr = newval;
        return oldval;
    }
    return *ptr;
}

#else /* HAVE_THREADS */

/* This should never trigger, unless a new threading implementation
 * without correct atomics dependencies in configure or a corresponding
 * atomics implementation is added. */
#error "Threading is enabled, but there is no implementation of atomic operations available"

#endif /* HAVE_PTHREADS */

#endif /* !HAVE_ATOMICS_NATIVE */

