#ifndef Py_CRITICAL_SECTION_H
#define Py_CRITICAL_SECTION_H
#ifdef __cplusplus
extern "C" {
#endif

// Python critical sections
//
// Conceptually, critical sections are a deadlock avoidance layer on top of
// per-object locks. These helpers, in combination with those locks, replace
// our usage of the global interpreter lock to provide thread-safety for
// otherwise thread-unsafe objects, such as dict.
//
// NOTE: These APIs are no-ops in non-free-threaded builds.
//
// NOTE: Only the top-most critical section is guaranteed to be active.
// Operations that need to lock two objects at once must use
// `Py_BEGIN_CRITICAL_SECTION2()`. You *CANNOT* use nested critical sections
// to lock more than one object at once, because the inner critical section
// may suspend the outer critical sections. This API does not provide a way
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

// NOTE: the contents of this struct are private and their meaning may
// change betweeen Python releases without a deprecation period.
typedef struct PyCriticalSection {
    // Tagged pointer to an outer active critical section (or 0).
    uintptr_t _cs_prev;

    // Mutex used to protect critical section
    struct PyMutex *_cs_mutex;
} PyCriticalSection;

// A critical section protected by two mutexes. Use
// Py_BEGIN_CRITICAL_SECTION2 and Py_END_CRITICAL_SECTION2.
// NOTE: the contents of this struct are private and may change betweeen
// Python releases without a deprecation period.
typedef struct PyCriticalSection2 {
    PyCriticalSection _cs_base;

    struct PyMutex *_cs_mutex2;
} PyCriticalSection2;

PyAPI_FUNC(void)
PyCriticalSection_Begin(PyCriticalSection *c, PyObject *op);

PyAPI_FUNC(void)
PyCriticalSection_End(PyCriticalSection *c);

PyAPI_FUNC(void)
PyCriticalSection2_Begin(PyCriticalSection2 *c, PyObject *a, PyObject *b);

PyAPI_FUNC(void)
PyCriticalSection2_End(PyCriticalSection2 *c);

// These are definitions for the stable ABI. For GIL-ful builds they're
// conditionally redefined as no-ops in cpython/critical_section.h.

# define Py_BEGIN_CRITICAL_SECTION(op)                                  \
    {                                                                   \
        PyCriticalSection _py_cs;                                       \
        PyCriticalSection_Begin(&_py_cs, _PyObject_CAST(op))

# define Py_END_CRITICAL_SECTION()                                      \
        PyCriticalSection_End(&_py_cs);                                 \
    }

# define Py_BEGIN_CRITICAL_SECTION2(a, b)                               \
    {                                                                   \
        PyCriticalSection2 _py_cs2;                                     \
        PyCriticalSection2_Begin(&_py_cs2, _PyObject_CAST(a), _PyObject_CAST(b))

# define Py_END_CRITICAL_SECTION2()                                     \
        PyCriticalSection2_End(&_py_cs2);                               \
    }

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_CRITICAL_SECTION_H
#  include "cpython/critical_section.h"
#  undef Py_CPYTHON_CRITICAL_SECTION_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_CRITICAL_SECTION_H */
