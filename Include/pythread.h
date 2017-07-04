
#ifndef Py_PYTHREAD_H
#define Py_PYTHREAD_H

#if !(defined(_POSIX_THREADS) || defined(NT_THREADS))
#   error "Require native thread feature. See https://bugs.python.org/issue30832"
#endif

#include <stdbool.h>  /* necessary for TSS key */

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

#ifndef Py_LIMITED_API
#define PYTHREAD_INVALID_THREAD_ID ((unsigned long)-1)
#endif

PyAPI_FUNC(void) PyThread_init_thread(void);
PyAPI_FUNC(unsigned long) PyThread_start_new_thread(void (*)(void *), void *);
PyAPI_FUNC(void) PyThread_exit_thread(void);
PyAPI_FUNC(unsigned long) PyThread_get_thread_ident(void);

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


/* Thread Local Storage (TLS) API
   TLS API is DEPRECATED.  Use Thread Specific Storage (TSS) API.

   Since existing TLS API has assumed the thread key type to int, but it is
   not compatible with POSIX (see PEP 539).  Therefore, new TSS API uses
   opaque data type for the thread key to ensure cross-platform.
*/
PyAPI_FUNC(int) PyThread_create_key(void) Py_DEPRECATED(3.7);
PyAPI_FUNC(void) PyThread_delete_key(int key) Py_DEPRECATED(3.7);
PyAPI_FUNC(int) PyThread_set_key_value(int key, void *value) Py_DEPRECATED(3.7);
PyAPI_FUNC(void *) PyThread_get_key_value(int key) Py_DEPRECATED(3.7);
PyAPI_FUNC(void) PyThread_delete_key_value(int key) Py_DEPRECATED(3.7);

/* Cleanup after a fork */
PyAPI_FUNC(void) PyThread_ReInitTLS(void) Py_DEPRECATED(3.7);


/* Thread Specific Storage (TSS) API */

/* Py_tss_t is an opaque data type the definition of which depends on the
   underlying TSS implementation.
*/
#ifdef Py_LIMITED_API
typedef struct _py_tss_t Py_tss_t;
#else

#if defined(_POSIX_THREADS)
    /* Darwin needs pthread.h to know type name the pthread_key_t. */
#   include <pthread.h>
#   define NATIVE_TSS_KEY_T     pthread_key_t
#elif defined(NT_THREADS)
    /* In Windows, native TSS key type is DWORD,
       but hardcode the unsigned long to avoid errors for include directive.
    */
#   define NATIVE_TSS_KEY_T     unsigned long
#endif

/* When Py_LIMITED_API is not defined, the type layout of Py_tss_t is in
   public to allow for static declarations in API clients.  Even in this case,
   you must handle TSS key through API functions due to compatibility.
*/
typedef struct _py_tss_t {
    bool _is_initialized;
    NATIVE_TSS_KEY_T _key;
} Py_tss_t;

#undef NATIVE_TSS_KEY_T

/* In static declaration, you must initialize with Py_tss_NEEDS_INIT. */
#define Py_tss_NEEDS_INIT   {._is_initialized = false}
#endif

PyAPI_FUNC(int) PyThread_tss_create(Py_tss_t *key);
PyAPI_FUNC(void) PyThread_tss_delete(Py_tss_t *key);
PyAPI_FUNC(int) PyThread_tss_set(Py_tss_t *key, void *value);
PyAPI_FUNC(void *) PyThread_tss_get(Py_tss_t *key);
PyAPI_FUNC(void) PyThread_tss_delete_value(Py_tss_t *key);

/* In the limited API, Py_tss_t value must be allocated through a pointer by
   PyThread_tss_alloc, and free by PyThread_tss_free at the life cycle end of
   the CPython interpreter.
*/
PyAPI_FUNC(Py_tss_t *) PyThread_tss_alloc(void);
PyAPI_FUNC(void) PyThread_tss_free(Py_tss_t *key);

/* When you'd check whether the key is created, use PyThread_tss_is_created. */
PyAPI_FUNC(bool) PyThread_tss_is_created(Py_tss_t *key);

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYTHREAD_H */
