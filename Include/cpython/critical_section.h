#ifndef Py_CPYTHON_CRITICAL_SECTION_H
#  error "this header file must not be included directly"
#endif

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

PyAPI_FUNC(void)
PyCriticalSection_BeginMutex(PyCriticalSection *c, PyMutex *m);

PyAPI_FUNC(void)
PyCriticalSection2_BeginMutex(PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2);

#ifndef Py_GIL_DISABLED
#undef Py_BEGIN_CRITICAL_SECTION
#undef Py_END_CRITICAL_SECTION
#undef Py_BEGIN_CRITICAL_SECTION2
#undef Py_END_CRITICAL_SECTION2

# define Py_BEGIN_CRITICAL_SECTION(op)      \
    {
# define Py_BEGIN_CRITICAL_SECTION_MUTEX(mutex)    \
    {
# define Py_END_CRITICAL_SECTION()          \
    }
# define Py_BEGIN_CRITICAL_SECTION2(a, b)   \
    {
# define Py_BEGIN_CRITICAL_SECTION2_MUTEX(m1, m2)  \
    {
# define Py_END_CRITICAL_SECTION2()         \
    }

#else /* !Py_GIL_DISABLED */

# define Py_BEGIN_CRITICAL_SECTION_MUTEX(mutex)                         \
    {                                                                   \
        PyCriticalSection _py_cs;                                       \
        PyCriticalSection_BeginMutex(&_py_cs, mutex)

# define Py_BEGIN_CRITICAL_SECTION2_MUTEX(m1, m2)                       \
    {                                                                   \
        PyCriticalSection2 _py_cs2;                                     \
        PyCriticalSection2_BeginMutex(&_py_cs2, m1, m2)

#endif /* !Py_GIL_DISABLED */
