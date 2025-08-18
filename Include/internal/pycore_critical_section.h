#ifndef Py_INTERNAL_CRITICAL_SECTION_H
#define Py_INTERNAL_CRITICAL_SECTION_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"        // PyMutex_LockFast()
#include "pycore_pystate.h"     // _PyThreadState_GET()
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tagged pointers to critical sections use the two least significant bits to
// mark if the pointed-to critical section is inactive and whether it is a
// PyCriticalSection2 object.
#define _Py_CRITICAL_SECTION_INACTIVE       0x1
#define _Py_CRITICAL_SECTION_TWO_MUTEXES    0x2
#define _Py_CRITICAL_SECTION_MASK           0x3

#ifdef Py_GIL_DISABLED
// Specialized version of critical section locking to safely use
// PySequence_Fast APIs without the GIL. For performance, the argument *to*
// PySequence_Fast() is provided to the macro, not the *result* of
// PySequence_Fast(), which would require an extra test to determine if the
// lock must be acquired.
# define Py_BEGIN_CRITICAL_SECTION_SEQUENCE_FAST(original)              \
    {                                                                   \
        PyObject *_orig_seq = _PyObject_CAST(original);                 \
        const bool _should_lock_cs = PyList_CheckExact(_orig_seq);      \
        PyCriticalSection _cs;                                          \
        if (_should_lock_cs) {                                          \
            _PyCriticalSection_Begin(&_cs, _orig_seq);                  \
        }

# define Py_END_CRITICAL_SECTION_SEQUENCE_FAST()                        \
        if (_should_lock_cs) {                                          \
            PyCriticalSection_End(&_cs);                                \
        }                                                               \
    }

// Asserts that the mutex is locked.  The mutex must be held by the
// top-most critical section otherwise there's the possibility
// that the mutex would be swalled out in some code paths.
#define _Py_CRITICAL_SECTION_ASSERT_MUTEX_LOCKED(mutex) \
    _PyCriticalSection_AssertHeld(mutex)

// Asserts that the mutex for the given object is locked. The mutex must
// be held by the top-most critical section otherwise there's the
// possibility that the mutex would be swalled out in some code paths.
#ifdef Py_DEBUG

# define _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op)                           \
    if (Py_REFCNT(op) != 1) {                                                    \
        _PyCriticalSection_AssertHeldObj(_PyObject_CAST(op)); \
    }

#else   /* Py_DEBUG */

# define _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op)

#endif  /* Py_DEBUG */

#else  /* !Py_GIL_DISABLED */
// The critical section APIs are no-ops with the GIL.
# define Py_BEGIN_CRITICAL_SECTION_SEQUENCE_FAST(original) {
# define Py_END_CRITICAL_SECTION_SEQUENCE_FAST() }
# define _Py_CRITICAL_SECTION_ASSERT_MUTEX_LOCKED(mutex)
# define _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op)
#endif  /* !Py_GIL_DISABLED */

// Resumes the top-most critical section.
PyAPI_FUNC(void)
_PyCriticalSection_Resume(PyThreadState *tstate);

// (private) slow path for locking the mutex
PyAPI_FUNC(void)
_PyCriticalSection_BeginSlow(PyCriticalSection *c, PyMutex *m);

PyAPI_FUNC(void)
_PyCriticalSection2_BeginSlow(PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2,
                             int is_m1_locked);

PyAPI_FUNC(void)
_PyCriticalSection_SuspendAll(PyThreadState *tstate);

#ifdef Py_GIL_DISABLED

static inline int
_PyCriticalSection_IsActive(uintptr_t tag)
{
    return tag != 0 && (tag & _Py_CRITICAL_SECTION_INACTIVE) == 0;
}

static inline void
_PyCriticalSection_BeginMutex(PyCriticalSection *c, PyMutex *m)
{
    if (PyMutex_LockFast(m)) {
        PyThreadState *tstate = _PyThreadState_GET();
        c->_cs_mutex = m;
        c->_cs_prev = tstate->critical_section;
        tstate->critical_section = (uintptr_t)c;
    }
    else {
        _PyCriticalSection_BeginSlow(c, m);
    }
}
#define PyCriticalSection_BeginMutex _PyCriticalSection_BeginMutex

static inline void
_PyCriticalSection_Begin(PyCriticalSection *c, PyObject *op)
{
    _PyCriticalSection_BeginMutex(c, &op->ob_mutex);
}
#define PyCriticalSection_Begin _PyCriticalSection_Begin

// Removes the top-most critical section from the thread's stack of critical
// sections. If the new top-most critical section is inactive, then it is
// resumed.
static inline void
_PyCriticalSection_Pop(PyCriticalSection *c)
{
    PyThreadState *tstate = _PyThreadState_GET();
    uintptr_t prev = c->_cs_prev;
    tstate->critical_section = prev;

    if ((prev & _Py_CRITICAL_SECTION_INACTIVE) != 0) {
        _PyCriticalSection_Resume(tstate);
    }
}

static inline void
_PyCriticalSection_End(PyCriticalSection *c)
{
    // If the mutex is NULL, we used the fast path in
    // _PyCriticalSection_BeginSlow for locks already held in the top-most
    // critical section, and we shouldn't unlock or pop this critical section.
    if (c->_cs_mutex == NULL) {
        return;
    }
    PyMutex_Unlock(c->_cs_mutex);
    _PyCriticalSection_Pop(c);
}
#define PyCriticalSection_End _PyCriticalSection_End

static inline void
_PyCriticalSection2_BeginMutex(PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2)
{
    if (m1 == m2) {
        // If the two mutex arguments are the same, treat this as a critical
        // section with a single mutex.
        c->_cs_mutex2 = NULL;
        _PyCriticalSection_BeginMutex(&c->_cs_base, m1);
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

    if (PyMutex_LockFast(m1)) {
        if (PyMutex_LockFast(m2)) {
            PyThreadState *tstate = _PyThreadState_GET();
            c->_cs_base._cs_mutex = m1;
            c->_cs_mutex2 = m2;
            c->_cs_base._cs_prev = tstate->critical_section;

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
#define PyCriticalSection2_BeginMutex _PyCriticalSection2_BeginMutex

static inline void
_PyCriticalSection2_Begin(PyCriticalSection2 *c, PyObject *a, PyObject *b)
{
    _PyCriticalSection2_BeginMutex(c, &a->ob_mutex, &b->ob_mutex);
}
#define PyCriticalSection2_Begin _PyCriticalSection2_Begin

static inline void
_PyCriticalSection2_End(PyCriticalSection2 *c)
{
    // if mutex1 is NULL, we used the fast path in
    // _PyCriticalSection_BeginSlow for mutexes that are already held,
    // which should only happen when mutex1 and mutex2 were the same mutex,
    // and mutex2 should also be NULL.
    if (c->_cs_base._cs_mutex == NULL) {
        assert(c->_cs_mutex2 == NULL);
        return;
    }
    if (c->_cs_mutex2) {
        PyMutex_Unlock(c->_cs_mutex2);
    }
    PyMutex_Unlock(c->_cs_base._cs_mutex);
    _PyCriticalSection_Pop(&c->_cs_base);
}
#define PyCriticalSection2_End _PyCriticalSection2_End

static inline void
_PyCriticalSection_AssertHeld(PyMutex *mutex)
{
#ifdef Py_DEBUG
    PyThreadState *tstate = _PyThreadState_GET();
    uintptr_t prev = tstate->critical_section;
    if (prev & _Py_CRITICAL_SECTION_TWO_MUTEXES) {
        PyCriticalSection2 *cs = (PyCriticalSection2 *)(prev & ~_Py_CRITICAL_SECTION_MASK);
        assert(cs != NULL && (cs->_cs_base._cs_mutex == mutex || cs->_cs_mutex2 == mutex));
    }
    else {
        PyCriticalSection *cs = (PyCriticalSection *)(tstate->critical_section & ~_Py_CRITICAL_SECTION_MASK);
        assert(cs != NULL && cs->_cs_mutex == mutex);
    }

#endif
}

static inline void
_PyCriticalSection_AssertHeldObj(PyObject *op)
{
#ifdef Py_DEBUG
    PyMutex *mutex = &_PyObject_CAST(op)->ob_mutex;
    PyThreadState *tstate = _PyThreadState_GET();
    uintptr_t prev = tstate->critical_section;
    if (prev & _Py_CRITICAL_SECTION_TWO_MUTEXES) {
        PyCriticalSection2 *cs = (PyCriticalSection2 *)(prev & ~_Py_CRITICAL_SECTION_MASK);
        _PyObject_ASSERT_WITH_MSG(op,
            (cs != NULL && (cs->_cs_base._cs_mutex == mutex || cs->_cs_mutex2 == mutex)),
            "Critical section of object is not held");
    }
    else {
        PyCriticalSection *cs = (PyCriticalSection *)(prev & ~_Py_CRITICAL_SECTION_MASK);
        _PyObject_ASSERT_WITH_MSG(op,
            (cs != NULL && cs->_cs_mutex == mutex),
            "Critical section of object is not held");
    }

#endif
}
#endif /* Py_GIL_DISABLED */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CRITICAL_SECTION_H */
