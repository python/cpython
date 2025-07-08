#ifndef Py_CPYTHON_CRITICAL_SECTION_H
#  error "this header file must not be included directly"
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

typedef struct PyCriticalSection PyCriticalSection;
typedef struct PyCriticalSection2 PyCriticalSection2;

PyAPI_FUNC(void)
PyCriticalSection_Begin(PyCriticalSection *c, PyObject *op);

PyAPI_FUNC(void)
PyCriticalSection_End(PyCriticalSection *c);

PyAPI_FUNC(void)
PyCriticalSection2_Begin(PyCriticalSection2 *c, PyObject *a, PyObject *b);

PyAPI_FUNC(void)
PyCriticalSection2_End(PyCriticalSection2 *c);

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
    uintptr_t _cs_prev;

    // Mutex used to protect critical section
    PyMutex *_cs_mutex;
};

// A critical section protected by two mutexes. Use
// Py_BEGIN_CRITICAL_SECTION2 and Py_END_CRITICAL_SECTION2.
// NOTE: the contents of this struct are private and may change betweeen
// Python releases without a deprecation period.
struct PyCriticalSection2 {
    PyCriticalSection _cs_base;

    PyMutex *_cs_mutex2;
};

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

#endif
