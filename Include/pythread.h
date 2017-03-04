
#ifndef Py_PYTHREAD_H
#define Py_PYTHREAD_H

#ifdef __CYGWIN__
/* For Cygwin use TSS instead of TLS. */
#include <stdbool.h>  /* necessary for TSS key */
#endif

typedef void *PyThread_type_lock;
typedef void *PyThread_type_sema;

#ifdef __cplusplus
extern "C" {
#endif

/* Return status codes for Python lock acquisition.  Chosen for maximum
 * backwards compatibility, ie failure -> 0, success -> 1.  */
typedef enum PyLockStatus {
    PY_LOCK_FAILURE = 0,
    PY_LOCK_ACQUIRED = 1,
    PY_LOCK_INTR
} PyLockStatus;

PyAPI_FUNC(void) PyThread_init_thread(void);
PyAPI_FUNC(long) PyThread_start_new_thread(void (*)(void *), void *);
PyAPI_FUNC(void) PyThread_exit_thread(void);
PyAPI_FUNC(long) PyThread_get_thread_ident(void);

PyAPI_FUNC(PyThread_type_lock) PyThread_allocate_lock(void);
PyAPI_FUNC(void) PyThread_free_lock(PyThread_type_lock);
PyAPI_FUNC(int) PyThread_acquire_lock(PyThread_type_lock, int);
#define WAIT_LOCK       1
#define NOWAIT_LOCK     0

/* PY_TIMEOUT_T is the integral type used to specify timeouts when waiting
   on a lock (see PyThread_acquire_lock_timed() below).
   PY_TIMEOUT_MAX is the highest usable value (in microseconds) of that
   type, and depends on the system threading API.

   NOTE: this isn't the same value as `_thread.TIMEOUT_MAX`.  The _thread
   module exposes a higher-level API, with timeouts expressed in seconds
   and floating-point numbers allowed.
*/
#define PY_TIMEOUT_T long long
#define PY_TIMEOUT_MAX PY_LLONG_MAX

/* In the NT API, the timeout is a DWORD and is expressed in milliseconds */
#if defined (NT_THREADS)
#if 0xFFFFFFFFLL * 1000 < PY_TIMEOUT_MAX
#undef PY_TIMEOUT_MAX
#define PY_TIMEOUT_MAX (0xFFFFFFFFLL * 1000)
#endif
#endif

/* If microseconds == 0, the call is non-blocking: it returns immediately
   even when the lock can't be acquired.
   If microseconds > 0, the call waits up to the specified duration.
   If microseconds < 0, the call waits until success (or abnormal failure)

   microseconds must be less than PY_TIMEOUT_MAX. Behaviour otherwise is
   undefined.

   If intr_flag is true and the acquire is interrupted by a signal, then the
   call will return PY_LOCK_INTR.  The caller may reattempt to acquire the
   lock.
*/
PyAPI_FUNC(PyLockStatus) PyThread_acquire_lock_timed(PyThread_type_lock,
                                                     PY_TIMEOUT_T microseconds,
                                                     int intr_flag);

PyAPI_FUNC(void) PyThread_release_lock(PyThread_type_lock);

PyAPI_FUNC(size_t) PyThread_get_stacksize(void);
PyAPI_FUNC(int) PyThread_set_stacksize(size_t);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
PyAPI_FUNC(PyObject*) PyThread_GetInfo(void);
#endif

#ifdef __CYGWIN__
/* Thread Local Storage (TLS) API
   TLS API is DEPRECATED.  Use Thread Specific Storage API.
*/
PyAPI_FUNC(long) PyThread_create_key(void) Py_DEPRECATED(3.7);
PyAPI_FUNC(void) PyThread_delete_key(long key) Py_DEPRECATED(3.7);
PyAPI_FUNC(int) PyThread_set_key_value(long key, void *value) Py_DEPRECATED(3.7);
PyAPI_FUNC(void *) PyThread_get_key_value(long key) Py_DEPRECATED(3.7);
PyAPI_FUNC(void) PyThread_delete_key_value(long key) Py_DEPRECATED(3.7);

/* Cleanup after a fork */
PyAPI_FUNC(void) PyThread_ReInitTLS(void) Py_DEPRECATED(3.7);

/* Thread Specific Storage (TSS) API

   POSIX hasn't defined that pthread_key_t is compatible with int
   (for details, see PEP 539).  Therefore, TSS API uses opaque type to cover
   the key details.
*/

#if defined(_POSIX_THREADS)
#   define NATIVE_TLS_KEY_T     pthread_key_t
#elif defined(NT_THREADS)
#   define NATIVE_TLS_KEY_T     DWORD
#else  /* For the platform that has not supplied native TLS */
#   define NATIVE_TLS_KEY_T     int
#endif

/* Py_tss_t is opaque type and you *must not* directly read and write.
   When you'd check whether the key is created, use PyThread_tss_is_created.
*/
typedef struct {
    bool _is_initialized;
    NATIVE_TLS_KEY_T _key;
} Py_tss_t;

#undef NATIVE_TLS_KEY_T

static inline bool
PyThread_tss_is_created(Py_tss_t key)
{
    return key._is_initialized;
}

/* Py_tss_NEEDS_INIT is the defined invalid value, and you *must* initialize
   the Py_tss_t variable by this value to use TSS API.

   For example:
   static Py_tss_t thekey = Py_tss_NEEDS_INIT;
   int fail = PyThread_tss_create(&thekey);
*/
#define Py_tss_NEEDS_INIT   {._is_initialized = false}

PyAPI_FUNC(int) PyThread_tss_create(Py_tss_t *key);
PyAPI_FUNC(void) PyThread_tss_delete(Py_tss_t *key);
PyAPI_FUNC(int) PyThread_tss_set(Py_tss_t key, void *value);
PyAPI_FUNC(void *) PyThread_tss_get(Py_tss_t key);
PyAPI_FUNC(void) PyThread_tss_delete_value(Py_tss_t key);

/* Cleanup after a fork */
PyAPI_FUNC(void) PyThread_ReInitTSS(void);
#else
/* Thread Local Storage (TLS) API */
PyAPI_FUNC(int) PyThread_create_key(void);
PyAPI_FUNC(void) PyThread_delete_key(int);
PyAPI_FUNC(int) PyThread_set_key_value(int, void *);
PyAPI_FUNC(void *) PyThread_get_key_value(int);
PyAPI_FUNC(void) PyThread_delete_key_value(int key);

/* Cleanup after a fork */
PyAPI_FUNC(void) PyThread_ReInitTLS(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYTHREAD_H */
