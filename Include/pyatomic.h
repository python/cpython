#ifndef Py_LIMITED_API
#ifndef Py_ATOMIC_H
#define Py_ATOMIC_H
/* XXX: When compilers start offering a stdatomic.h with lock-free
   atomic_int and atomic_address types, include that here and rewrite
   the atomic operations in terms of it. */

#include "dynamic_annotations.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is modeled after the atomics interface from C1x, according to
 * the draft at
 * http://www.open-std.org/JTC1/SC22/wg14/www/docs/n1425.pdf.
 * Operations and types are named the same except with a _Py_ prefix
 * and have the same semantics.
 *
 * Beware, the implementations here are deep magic.
 */

typedef enum _Py_memory_order {
    _Py_memory_order_relaxed,
    _Py_memory_order_acquire,
    _Py_memory_order_release,
    _Py_memory_order_acq_rel,
    _Py_memory_order_seq_cst
} _Py_memory_order;

typedef struct _Py_atomic_address {
    void *_value;
} _Py_atomic_address;

typedef struct _Py_atomic_int {
    int _value;
} _Py_atomic_int;

/* Only support GCC (for expression statements) and x86 (for simple
 * atomic semantics) for now */
#if defined(__GNUC__) && (defined(__i386__) || defined(__amd64))

static __inline__ void
_Py_atomic_signal_fence(_Py_memory_order order)
{
    if (order != _Py_memory_order_relaxed)
        __asm__ volatile("":::"memory");
}

static __inline__ void
_Py_atomic_thread_fence(_Py_memory_order order)
{
    if (order != _Py_memory_order_relaxed)
        __asm__ volatile("mfence":::"memory");
}

/* Tell the race checker about this operation's effects. */
static __inline__ void
_Py_ANNOTATE_MEMORY_ORDER(const volatile void *address, _Py_memory_order order)
{
    switch(order) {
    case _Py_memory_order_release:
    case _Py_memory_order_acq_rel:
    case _Py_memory_order_seq_cst:
        _Py_ANNOTATE_HAPPENS_BEFORE(address);
        break;
    default:
        break;
    }
    switch(order) {
    case _Py_memory_order_acquire:
    case _Py_memory_order_acq_rel:
    case _Py_memory_order_seq_cst:
        _Py_ANNOTATE_HAPPENS_AFTER(address);
        break;
    default:
        break;
    }
}

#define _Py_atomic_store_explicit(ATOMIC_VAL, NEW_VAL, ORDER) \
    __extension__ ({ \
        __typeof__(ATOMIC_VAL) atomic_val = ATOMIC_VAL; \
        __typeof__(atomic_val->_value) new_val = NEW_VAL;\
        volatile __typeof__(new_val) *volatile_data = &atomic_val->_value; \
        _Py_memory_order order = ORDER; \
        _Py_ANNOTATE_MEMORY_ORDER(atomic_val, order); \
        \
        /* Perform the operation. */ \
        _Py_ANNOTATE_IGNORE_WRITES_BEGIN(); \
        switch(order) { \
        case _Py_memory_order_release: \
            _Py_atomic_signal_fence(_Py_memory_order_release); \
            /* fallthrough */ \
        case _Py_memory_order_relaxed: \
            *volatile_data = new_val; \
            break; \
        \
        case _Py_memory_order_acquire: \
        case _Py_memory_order_acq_rel: \
        case _Py_memory_order_seq_cst: \
            __asm__ volatile("xchg %0, %1" \
                         : "+r"(new_val) \
                         : "m"(atomic_val->_value) \
                         : "memory"); \
            break; \
        } \
        _Py_ANNOTATE_IGNORE_WRITES_END(); \
    })

#define _Py_atomic_load_explicit(ATOMIC_VAL, ORDER) \
    __extension__ ({  \
        __typeof__(ATOMIC_VAL) atomic_val = ATOMIC_VAL; \
        __typeof__(atomic_val->_value) result; \
        volatile __typeof__(result) *volatile_data = &atomic_val->_value; \
        _Py_memory_order order = ORDER; \
        _Py_ANNOTATE_MEMORY_ORDER(atomic_val, order); \
        \
        /* Perform the operation. */ \
        _Py_ANNOTATE_IGNORE_READS_BEGIN(); \
        switch(order) { \
        case _Py_memory_order_release: \
        case _Py_memory_order_acq_rel: \
        case _Py_memory_order_seq_cst: \
            /* Loads on x86 are not releases by default, so need a */ \
            /* thread fence. */ \
            _Py_atomic_thread_fence(_Py_memory_order_release); \
            break; \
        default: \
            /* No fence */ \
            break; \
        } \
        result = *volatile_data; \
        switch(order) { \
        case _Py_memory_order_acquire: \
        case _Py_memory_order_acq_rel: \
        case _Py_memory_order_seq_cst: \
            /* Loads on x86 are automatically acquire operations so */ \
            /* can get by with just a compiler fence. */ \
            _Py_atomic_signal_fence(_Py_memory_order_acquire); \
            break; \
        default: \
            /* No fence */ \
            break; \
        } \
        _Py_ANNOTATE_IGNORE_READS_END(); \
        result; \
    })

#else  /* !gcc x86 */
/* Fall back to other compilers and processors by assuming that simple
   volatile accesses are atomic.  This is false, so people should port
   this. */
#define _Py_atomic_signal_fence(/*memory_order*/ ORDER) ((void)0)
#define _Py_atomic_thread_fence(/*memory_order*/ ORDER) ((void)0)
#define _Py_atomic_store_explicit(ATOMIC_VAL, NEW_VAL, ORDER) \
    ((ATOMIC_VAL)->_value = NEW_VAL)
#define _Py_atomic_load_explicit(ATOMIC_VAL, ORDER) \
    ((ATOMIC_VAL)->_value)

#endif  /* !gcc x86 */

/* Standardized shortcuts. */
#define _Py_atomic_store(ATOMIC_VAL, NEW_VAL) \
    _Py_atomic_store_explicit(ATOMIC_VAL, NEW_VAL, _Py_memory_order_seq_cst)
#define _Py_atomic_load(ATOMIC_VAL) \
    _Py_atomic_load_explicit(ATOMIC_VAL, _Py_memory_order_seq_cst)

/* Python-local extensions */

#define _Py_atomic_store_relaxed(ATOMIC_VAL, NEW_VAL) \
    _Py_atomic_store_explicit(ATOMIC_VAL, NEW_VAL, _Py_memory_order_relaxed)
#define _Py_atomic_load_relaxed(ATOMIC_VAL) \
    _Py_atomic_load_explicit(ATOMIC_VAL, _Py_memory_order_relaxed)

#ifdef __cplusplus
}
#endif

#endif  /* Py_ATOMIC_H */
#endif  /* Py_LIMITED_API */
