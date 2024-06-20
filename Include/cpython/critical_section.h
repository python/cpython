#ifndef Py_CPYTHON_CRITICAL_SECTION_H
#  error "this header file must not be included directly"
#endif

// Python critical sections.
//
// These operations are no-ops in the default build. See
// pycore_critical_section.h for details.

typedef struct PyCriticalSection PyCriticalSection;
typedef struct PyCriticalSection2 PyCriticalSection2;

#ifndef Py_GIL_DISABLED
# define Py_BEGIN_CRITICAL_SECTION(op)      \
    {
# define Py_END_CRITICAL_SECTION()          \
    }
# define Py_BEGIN_CRITICAL_SECTION2(a, b)   \
    {
# define Py_END_CRITICAL_SECTION2()         \
    }
#else /* !Py_GIL_DISABLED */

// NOTE: the contents of this struct are private and may change betweeen
// Python releases without a deprecation period.
struct PyCriticalSection {
    // Tagged pointer to an outer active critical section (or 0).
    uintptr_t prev;

    // Mutex used to protect critical section
    PyMutex *mutex;
};

// A critical section protected by two mutexes. Use
// Py_BEGIN_CRITICAL_SECTION2 and Py_END_CRITICAL_SECTION2.
// NOTE: the contents of this struct are private and may change betweeen
// Python releases without a deprecation period.
struct PyCriticalSection2 {
    PyCriticalSection base;

    PyMutex *mutex2;
};

# define Py_BEGIN_CRITICAL_SECTION(op)                                  \
    {                                                                   \
        PyCriticalSection _cs;                                         \
        PyCriticalSection_Begin(&_cs, _PyObject_CAST(op))

# define Py_END_CRITICAL_SECTION()                                      \
        PyCriticalSection_End(&_cs);                                   \
    }

# define Py_BEGIN_CRITICAL_SECTION2(a, b)                               \
    {                                                                   \
        PyCriticalSection2 _cs2;                                       \
        PyCriticalSection2_Begin(&_cs2, _PyObject_CAST(a), _PyObject_CAST(b))

# define Py_END_CRITICAL_SECTION2()                                     \
        PyCriticalSection2_End(&_cs2);                                 \
    }

#endif

// (private)
PyAPI_FUNC(void)
PyCriticalSection_Begin(PyCriticalSection *c, PyObject *op);

// (private)
PyAPI_FUNC(void)
PyCriticalSection_End(PyCriticalSection *c);

// (private)
PyAPI_FUNC(void)
PyCriticalSection2_Begin(PyCriticalSection2 *c, PyObject *a, PyObject *b);

// (private)
PyAPI_FUNC(void)
PyCriticalSection2_End(PyCriticalSection2 *c);

// CPython internals should use pycore_critical_section.h instead.
#ifdef Py_BUILD_CORE
# undef Py_BEGIN_CRITICAL_SECTION
# undef Py_END_CRITICAL_SECTION
# undef Py_BEGIN_CRITICAL_SECTION2
# undef Py_END_CRITICAL_SECTION2
#endif
