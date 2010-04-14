
#ifndef Py_PYTHREAD_H
#define Py_PYTHREAD_H

typedef void *PyThread_type_lock;
typedef void *PyThread_type_sema;

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(void) PyThread_init_thread(void);
PyAPI_FUNC(long) PyThread_start_new_thread(void (*)(void *), void *);
PyAPI_FUNC(void) PyThread_exit_thread(void);
PyAPI_FUNC(long) PyThread_get_thread_ident(void);

PyAPI_FUNC(PyThread_type_lock) PyThread_allocate_lock(void);
PyAPI_FUNC(void) PyThread_free_lock(PyThread_type_lock);
PyAPI_FUNC(int) PyThread_acquire_lock(PyThread_type_lock, int);
#define WAIT_LOCK	1
#define NOWAIT_LOCK	0

/* PY_TIMEOUT_T is the integral type used to specify timeouts when waiting
   on a lock (see PyThread_acquire_lock_timed() below).
   PY_TIMEOUT_MAX is the highest usable value (in microseconds) of that
   type, and depends on the system threading API.
   
   NOTE: this isn't the same value as `_thread.TIMEOUT_MAX`.  The _thread
   module exposes a higher-level API, with timeouts expressed in seconds
   and floating-point numbers allowed.
*/
#if defined(HAVE_LONG_LONG)
#define PY_TIMEOUT_T PY_LONG_LONG
#define PY_TIMEOUT_MAX PY_LLONG_MAX
#else
#define PY_TIMEOUT_T long
#define PY_TIMEOUT_MAX LONG_MAX
#endif

/* In the NT API, the timeout is a DWORD and is expressed in milliseconds */
#if defined (NT_THREADS)
#if (0xFFFFFFFFLL * 1000 < PY_TIMEOUT_MAX)
#undef PY_TIMEOUT_MAX
#define PY_TIMEOUT_MAX (0xFFFFFFFFLL * 1000)
#endif
#endif

/* If microseconds == 0, the call is non-blocking: it returns immediately
   even when the lock can't be acquired.
   If microseconds > 0, the call waits up to the specified duration.
   If microseconds < 0, the call waits until success (or abnormal failure)
   
   microseconds must be less than PY_TIMEOUT_MAX. Behaviour otherwise is
   undefined. */
PyAPI_FUNC(int) PyThread_acquire_lock_timed(PyThread_type_lock,
					    PY_TIMEOUT_T microseconds);
PyAPI_FUNC(void) PyThread_release_lock(PyThread_type_lock);

PyAPI_FUNC(size_t) PyThread_get_stacksize(void);
PyAPI_FUNC(int) PyThread_set_stacksize(size_t);

/* Thread Local Storage (TLS) API */
PyAPI_FUNC(int) PyThread_create_key(void);
PyAPI_FUNC(void) PyThread_delete_key(int);
PyAPI_FUNC(int) PyThread_set_key_value(int, void *);
PyAPI_FUNC(void *) PyThread_get_key_value(int);
PyAPI_FUNC(void) PyThread_delete_key_value(int key);

/* Cleanup after a fork */
PyAPI_FUNC(void) PyThread_ReInitTLS(void);

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYTHREAD_H */
