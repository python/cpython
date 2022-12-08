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
#endif

#ifndef _POSIX_THREADS

/* Check if we're running on HP-UX and _SC_THREADS is defined. If so, then
   enough of the Posix threads package is implemented to support python
   threads.

   This is valid for HP-UX 11.23 running on an ia64 system. If needed, add
   a check of __ia64 to verify that we're running on an ia64 system instead
   of a pa-risc system.
*/
#ifdef __hpux
#ifdef _SC_THREADS
#define _POSIX_THREADS
#endif
#endif

#endif /* _POSIX_THREADS */


struct _pythread_runtime_state {
    int initialized;
#if defined(_POSIX_THREADS) && !defined(HAVE_PTHREAD_STUBS)
    // This matches when thread_pthread.h is used.
    /* NULL when pthread_condattr_setclock(CLOCK_MONOTONIC) is not supported. */
    pthread_condattr_t *condattr_monotonic;
#endif
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYTHREAD_H */
