#ifndef Py_CPYTHON_PYTHREAD_H
#  error "this header file must not be included directly"
#endif

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
