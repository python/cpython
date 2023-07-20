#ifndef Py_INTERNAL_PYTHREAD_H
#define Py_INTERNAL_PYTHREAD_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#ifndef _POSIX_THREADS
/* This means pthreads are not implemented in libc headers, hence the macro
   not present in unistd.h. But they still can be implemented as an external
   library (e.g. gnu pth in pthread emulation) */
# ifdef HAVE_PTHREAD_H
#  include <pthread.h> /* _POSIX_THREADS */
# endif
# ifndef _POSIX_THREADS
/* Check if we're running on HP-UX and _SC_THREADS is defined. If so, then
   enough of the Posix threads package is implemented to support python
   threads.

   This is valid for HP-UX 11.23 running on an ia64 system. If needed, add
   a check of __ia64 to verify that we're running on an ia64 system instead
   of a pa-risc system.
*/
#  ifdef __hpux
#   ifdef _SC_THREADS
#    define _POSIX_THREADS
#   endif
#  endif
# endif /* _POSIX_THREADS */
#endif /* _POSIX_THREADS */

#if defined(_POSIX_THREADS) || defined(HAVE_PTHREAD_STUBS)
# define _USE_PTHREADS
#endif

#if defined(_USE_PTHREADS) && defined(HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
// monotonic is supported statically.  It doesn't mean it works on runtime.
# define CONDATTR_MONOTONIC
#endif


#if defined(HAVE_PTHREAD_STUBS)
// pthread_key
struct py_stub_tls_entry {
    bool in_use;
    void *value;
};
#endif

struct _pythread_runtime_state {
    int initialized;

#ifdef _USE_PTHREADS
    // This matches when thread_pthread.h is used.
    struct {
        /* NULL when pthread_condattr_setclock(CLOCK_MONOTONIC) is not supported. */
        pthread_condattr_t *ptr;
# ifdef CONDATTR_MONOTONIC
    /* The value to which condattr_monotonic is set. */
        pthread_condattr_t val;
# endif
    } _condattr_monotonic;

#endif  // USE_PTHREADS

#if defined(HAVE_PTHREAD_STUBS)
    struct {
        struct py_stub_tls_entry tls_entries[PTHREAD_KEYS_MAX];
    } stubs;
#endif
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYTHREAD_H */
