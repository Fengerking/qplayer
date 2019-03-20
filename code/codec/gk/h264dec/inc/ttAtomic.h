#ifndef __TTPOD_TT_ATOMIC_H_
#define __TTPOD_TT_ATOMIC_H_

#include "config.h"

#if HAVE_ATOMICS_NATIVE

#if HAVE_ATOMICS_GCC
#include "ttAtomicGcc.h"
#elif HAVE_ATOMICS_WIN32
#include "atomic_win32.h"
#elif HAVE_ATOMICS_SUNCC
#include "atomic_suncc.h"
#endif

#else

/**
 * Load the current value stored in an atomic integer.
 *
 * @param ptr atomic integer
 * @return the current value of the atomic integer
 * @note This acts as a memory barrier.
 */
int ttpriv_atomic_int_get(volatile int *ptr);

/**
 * Store a new value in an atomic integer.
 *
 * @param ptr atomic integer
 * @param val the value to store in the atomic integer
 * @note This acts as a memory barrier.
 */
void ttpriv_atomic_int_set(volatile int *ptr, int val);

/**
 * Add a value to an atomic integer.
 *
 * @param ptr atomic integer
 * @param inc the value to add to the atomic integer (may be negative)
 * @return the new value of the atomic integer.
 * @note This does NOT act as a memory barrier. This is primarily
 *       intended for reference counting.
 */
int ttpriv_atomic_int_add_and_fetch(volatile int *ptr, int inc);

/**
 * Atomic pointer compare and swap.
 *
 * @param ptr pointer to the pointer to operate on
 * @param oldval do the swap if the current value of *ptr equals to oldval
 * @param newval value to replace *ptr with
 * @return the value of *ptr before comparison
 */
void *ttpriv_atomic_ptr_cas(void * volatile *ptr, void *oldval, void *newval);

#endif /* HAVE_ATOMICS_NATIVE */

#endif /* __TTPOD_TT_ATOMIC_H_ */
