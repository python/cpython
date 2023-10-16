#ifndef Py_CPYTHON_PYTHREAD_H
#  error "this header file must not be included directly"
#endif

/* Thread joining APIs.
 *
 * These APIs have a strict contract:
 *  - Either PyThread_join_thread or PyThread_detach_thread must be called
 *    exactly once with the given handle.
 *  - Calling neither PyThread_join_thread nor PyThread_detach_thread results
 *    in a resource leak until the end of the process.
 *  - Any other usage, such as calling both PyThread_join_thread and
 *    PyThread_detach_thread, or calling them more than once (including
 *    simultaneously), results in undefined behavior.
 */
PyAPI_FUNC(unsigned long) PyThread_start_joinable_thread(void (*func)(void *),
                                                         void *arg,
                                                         Py_uintptr_t* handle);
/*
 * Join a thread started with `PyThread_start_joinable_thread`.
 * This function cannot be interrupted. It returns 0 on success,
 * a non-zero value on failure.
 */
PyAPI_FUNC(int) PyThread_join_thread(Py_uintptr_t);
/*
 * Detach a thread started with `PyThread_start_joinable_thread`, such
 * that its resources are relased as soon as it exits.
 * This function cannot be interrupted. It returns 0 on success,
 * a non-zero value on failure.
 */
PyAPI_FUNC(int) PyThread_detach_thread(Py_uintptr_t);

// PY_TIMEOUT_MAX is the highest usable value (in microseconds) of PY_TIMEOUT_T
// type, and depends on the system threading API.
//
// NOTE: this isn't the same value as `_thread.TIMEOUT_MAX`. The _thread module
// exposes a higher-level API, with timeouts expressed in seconds and
// floating-point numbers allowed.
PyAPI_DATA(const long long) PY_TIMEOUT_MAX;

#define PYTHREAD_INVALID_THREAD_ID ((unsigned long)-1)

#ifdef HAVE_PTHREAD_H
    /* Darwin needs pthread.h to know type name the pthread_key_t. */
#   include <pthread.h>
#   define NATIVE_TSS_KEY_T     pthread_key_t
#elif defined(NT_THREADS)
    /* In Windows, native TSS key type is DWORD,
       but hardcode the unsigned long to avoid errors for include directive.
    */
#   define NATIVE_TSS_KEY_T     unsigned long
#elif defined(HAVE_PTHREAD_STUBS)
#   include "cpython/pthread_stubs.h"
#   define NATIVE_TSS_KEY_T     pthread_key_t
#else
#   error "Require native threads. See https://bugs.python.org/issue31370"
#endif

/* When Py_LIMITED_API is not defined, the type layout of Py_tss_t is
   exposed to allow static allocation in the API clients.  Even in this case,
   you must handle TSS keys through API functions due to compatibility.
*/
struct _Py_tss_t {
    int _is_initialized;
    NATIVE_TSS_KEY_T _key;
};

#undef NATIVE_TSS_KEY_T

/* When static allocation, you must initialize with Py_tss_NEEDS_INIT. */
#define Py_tss_NEEDS_INIT   {0}
