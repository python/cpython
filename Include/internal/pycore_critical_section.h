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

// Implementation of Python critical sections: helpers to replace the global
// interpreter lock with fine grained locking.
//
// A Python critical section is a region of code that can only be executed by
// a single thread at at time. The regions begin with a call to
// Py_BEGIN_CRITICAL_SECTION and ends with Py_END_CRITICAL_SECTION. Active
// critical sections are *implicitly* suspended whenever the thread calls
// `_PyThreadState_Detach()` (i.e., at tiems when the interpreter would have
// released the GIL).
//
// The most recent critical section is resumed when `_PyThreadState_Attach()`
// is called. See `_PyCriticalSection_Resume()`.
//
// The purpose of implicitly ending critical sections is to avoid deadlocks
// when locking multiple objects. Any time a thread would have released the
// GIL, it releases all locks from active critical sections. This includes
// times when a thread blocks while attempting to acquire a lock.
//
// Note that the macros Py_BEGIN_CRITICAL_SECTION and Py_END_CRITICAL_SECTION
// are no-ops in non-free-threaded builds.
//
// Example usage:
//  Py_BEGIN_CRITICAL_SECTION(op);
//  ...
//  Py_END_CRITICAL_SECTION;
//
// To lock two objects at once:
//  Py_BEGIN_CRITICAL_SECTION2(op1, op2);
//  ...
//  Py_END_CRITICAL_SECTION2;


// Tagged pointers to critical sections use the two least significant bits to
// mark if the pointed-to critical section is inactive and whether it is a
// _PyCriticalSection2 object.
#define _Py_CRITICAL_SECTION_INACTIVE       0x1
#define _Py_CRITICAL_SECTION_TWO_MUTEXES    0x2
#define _Py_CRITICAL_SECTION_MASK           0x3

#ifdef Py_NOGIL
#define Py_BEGIN_CRITICAL_SECTION(op) {                             \
    _PyCriticalSection _cs;                                          \
    _PyCriticalSection_Begin(&_cs, &_PyObject_CAST(op)->ob_mutex)

#define Py_END_CRITICAL_SECTION                                     \
    _PyCriticalSection_End(&_cs);                                    \
}

#define Py_BEGIN_CRITICAL_SECTION2(a, b) {                          \
    _PyCriticalSection2 _cs2;                                        \
    _PyCriticalSection2_Begin(&_cs2, &_PyObject_CAST(a)->ob_mutex, &_PyObject_CAST(b)->ob_mutex)

#define Py_END_CRITICAL_SECTION2                                    \
    _PyCriticalSection2_End(&_cs2);                                  \
}
#else
// The critical section APIs are no-ops with the GIL.
#define Py_BEGIN_CRITICAL_SECTION(op)
#define Py_END_CRITICAL_SECTION
#define Py_BEGIN_CRITICAL_SECTION2(a, b)
#define Py_END_CRITICAL_SECTION2
#endif

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

// Removes the top-most critical section from the thread's of critical
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
