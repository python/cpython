#ifndef Py_INTERNAL_CONDVAR_H
#define Py_INTERNAL_CONDVAR_H

#ifndef _POSIX_THREADS
/* This means pthreads are not implemented in libc headers, hence the macro
   not present in unistd.h. But they still can be implemented as an external
   library (e.g. gnu pth in pthread emulation) */
# ifdef HAVE_PTHREAD_H
#  include <pthread.h> /* _POSIX_THREADS */
# endif
#endif

#ifdef _POSIX_THREADS
/*
 * POSIX support
 */
#define Py_HAVE_CONDVAR

#include <pthread.h>

#define PyMUTEX_T pthread_mutex_t
#define PyCOND_T pthread_cond_t

#elif defined(NT_THREADS)
/*
 * Windows (XP, 2003 server and later, as well as (hopefully) CE) support
 *
 * Emulated condition variables ones that work with XP and later, plus
 * example native support on VISTA and onwards.
 */
#define Py_HAVE_CONDVAR

/* include windows if it hasn't been done before */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Use native Win7 primitives */

/* SRWLOCK is faster and better than CriticalSection */
typedef SRWLOCK PyMUTEX_T;

typedef CONDITION_VARIABLE  PyCOND_T;

#endif /* _POSIX_THREADS, NT_THREADS */

#endif /* Py_INTERNAL_CONDVAR_H */
