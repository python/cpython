#ifndef Py_INTERNAL_CRITICAL_SECTION_H
#define Py_INTERNAL_CRITICAL_SECTION_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"        // PyMutex
#include "pycore_pystate.h"     // _PyThreadState_GET()
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Implementation of Python critical sections
//
// Conceptually, critical sections are a deadlock avoidance layer on top of
// per-object locks. These helpers, in combination with those locks, replace
// our usage of the global interpreter lock to provide thread-safety for
// otherwise thread-unsafe objects, such as dict.
//
// NOTE: These APIs are no-ops in non-free-threaded builds.
//
// Straightforward per-object locking could introduce deadlocks that were not
// present when running with the GIL. Threads may hold locks for multiple
// objects simultaneously because Python operations can nest. If threads were
// to acquire the same locks in different orders, they would deadlock.
//
// One way to avoid deadlocks is to allow threads to hold only the lock (or
// locks) for a single operation at a time (typically a single lock, but some
// operations involve two locks). When a thread begins a nested operation it
// could suspend the locks for any outer operation: before beginning the nested
// operation, the locks for the outer operation are released and when the
// nested operation completes, the locks for the outer operation are
// reacquired.
//
// To improve performance, this API uses a variation of the above scheme.
// Instead of immediately suspending locks any time a nested operation begins,
// locks are only suspended if the thread would block. This reduces the number
// of lock acquisitions and releases for nested operations, while still
// avoiding deadlocks.
//
// Additionally, the locks for any active operation are suspended around
// other potentially blocking operations, such as I/O. This is because the
// interaction between locks and blocking operations can lead to deadlocks in
// the same way as the interaction between multiple locks.
//
// Each thread's critical sections and their corresponding locks are tracked in
// a stack in `PyThreadState.critical_section`. When a thread calls
// `_PyThreadState_Detach()`, such as before a blocking I/O operation or when
// waiting to acquire a lock, the thread suspends all of its active critical
// sections, temporarily releasing the associated locks. When the thread calls
// `_PyThreadState_Attach()`, it resumes the top-most (i.e., most recent)
// critical section by reacquiring the associated lock or locks.  See
// `_PyCriticalSection_Resume()`.
//
// NOTE: Only the top-most critical section is guaranteed to be active.
// Operations that need to lock two objects at once must use
// `Py_BEGIN_CRITICAL_SECTION2()`. You *CANNOT* use nested critical sections
// to lock more than one object at once, because the inner critical section
// may  suspend the outer critical sections. This API does not provide a way
// to lock more than two objects at once (though it could be added later
// if actually needed).
//
// NOTE: Critical sections implicitly behave like reentrant locks because
// attempting to acquire the same lock will suspend any outer (earlier)
// critical sections. However, they are less efficient for this use case than
// purposefully designed reentrant locks.
//
// Example usage:
//  Py_BEGIN_CRITICAL_SECTION(op);
//  ...
//  Py_END_CRITICAL_SECTION();
//
// To lock two objects at once:
//  Py_BEGIN_CRITICAL_SECTION2(op1, op2);
//  ...
//  Py_END_CRITICAL_SECTION2();


// Tagged pointers to critical sections use the two least significant bits to
// mark if the pointed-to critical section is inactive and whether it is a
// _PyCriticalSection2 object.
#define _Py_CRITICAL_SECTION_INACTIVE       0x1
#define _Py_CRITICAL_SECTION_TWO_MUTEXES    0x2
#define _Py_CRITICAL_SECTION_MASK           0x3

#ifdef Py_GIL_DISABLED
# define Py_BEGIN_CRITICAL_SECTION(op)                                  \
    {                                                                   \
        _PyCriticalSection _cs;                                         \
        _PyCriticalSection_Begin(&_cs, &_PyObject_CAST(op)->ob_mutex)

# define Py_END_CRITICAL_SECTION()                                      \
        _PyCriticalSection_End(&_cs);                                   \
    }

# define Py_BEGIN_CRITICAL_SECTION2(a, b)                               \
    {                                                                   \
        _PyCriticalSection2 _cs2;                                       \
        _PyCriticalSection2_Begin(&_cs2, &_PyObject_CAST(a)->ob_mutex, &_PyObject_CAST(b)->ob_mutex)

# define Py_END_CRITICAL_SECTION2()                                     \
        _PyCriticalSection2_End(&_cs2);                                 \
    }
#else  /* !Py_GIL_DISABLED */
// The critical section APIs are no-ops with the GIL.
# define Py_BEGIN_CRITICAL_SECTION(op)
# define Py_END_CRITICAL_SECTION()
# define Py_BEGIN_CRITICAL_SECTION2(a, b)
# define Py_END_CRITICAL_SECTION2()
#endif  /* !Py_GIL_DISABLED */

typedef struct {
    // Tagged pointer to an outer active critical section (or 0).
    // The two least-significant-bits indicate whether the pointed-to critical
    // section is inactive and whether it is a _PyCriticalSection2 object.
    uintptr_t prev;

    // Mutex used to protect critical section
    PyMutex *mutex;
} _PyCriticalSection;

// A critical section protected by two mutexes. Use
// _PyCriticalSection2_Begin and _PyCriticalSection2_End.
typedef struct {
    _PyCriticalSection base;

    PyMutex *mutex2;
} _PyCriticalSection2;

static inline int
_PyCriticalSection_IsActive(uintptr_t tag)
{
    return tag != 0 && (tag & _Py_CRITICAL_SECTION_INACTIVE) == 0;
}

// Resumes the top-most critical section.
PyAPI_FUNC(void)
_PyCriticalSection_Resume(PyThreadState *tstate);

// (private) slow path for locking the mutex
PyAPI_FUNC(void)
_PyCriticalSection_BeginSlow(_PyCriticalSection *c, PyMutex *m);

PyAPI_FUNC(void)
_PyCriticalSection2_BeginSlow(_PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2,
                             int is_m1_locked);

static inline void
_PyCriticalSection_Begin(_PyCriticalSection *c, PyMutex *m)
{
    if (PyMutex_LockFast(&m->v)) {
        PyThreadState *tstate = _PyThreadState_GET();
        c->mutex = m;
        c->prev = tstate->critical_section;
        tstate->critical_section = (uintptr_t)c;
    }
    else {
        _PyCriticalSection_BeginSlow(c, m);
    }
}

// Removes the top-most critical section from the thread's stack of critical
// sections. If the new top-most critical section is inactive, then it is
// resumed.
static inline void
_PyCriticalSection_Pop(_PyCriticalSection *c)
{
    PyThreadState *tstate = _PyThreadState_GET();
    uintptr_t prev = c->prev;
    tstate->critical_section = prev;

    if ((prev & _Py_CRITICAL_SECTION_INACTIVE) != 0) {
        _PyCriticalSection_Resume(tstate);
    }
}

static inline void
_PyCriticalSection_End(_PyCriticalSection *c)
{
    PyMutex_Unlock(c->mutex);
    _PyCriticalSection_Pop(c);
}

static inline void
_PyCriticalSection2_Begin(_PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2)
{
    if (m1 == m2) {
        // If the two mutex arguments are the same, treat this as a critical
        // section with a single mutex.
        c->mutex2 = NULL;
        _PyCriticalSection_Begin(&c->base, m1);
        return;
    }

    if ((uintptr_t)m2 < (uintptr_t)m1) {
        // Sort the mutexes so that the lower address is locked first.
        // The exact order does not matter, but we need to acquire the mutexes
        // in a consistent order to avoid lock ordering deadlocks.
        PyMutex *tmp = m1;
        m1 = m2;
        m2 = tmp;
    }

    if (PyMutex_LockFast(&m1->v)) {
        if (PyMutex_LockFast(&m2->v)) {
            PyThreadState *tstate = _PyThreadState_GET();
            c->base.mutex = m1;
            c->mutex2 = m2;
            c->base.prev = tstate->critical_section;

            uintptr_t p = (uintptr_t)c | _Py_CRITICAL_SECTION_TWO_MUTEXES;
            tstate->critical_section = p;
        }
        else {
            _PyCriticalSection2_BeginSlow(c, m1, m2, 1);
        }
    }
    else {
        _PyCriticalSection2_BeginSlow(c, m1, m2, 0);
    }
}

static inline void
_PyCriticalSection2_End(_PyCriticalSection2 *c)
{
    if (c->mutex2) {
        PyMutex_Unlock(c->mutex2);
    }
    PyMutex_Unlock(c->base.mutex);
    _PyCriticalSection_Pop(&c->base);
}

PyAPI_FUNC(void)
_PyCriticalSection_SuspendAll(PyThreadState *tstate);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CRITICAL_SECTION_H */
