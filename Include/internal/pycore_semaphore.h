// The _PySemaphore API a simplified cross-platform semaphore used to implement
// wakeup/sleep.
#ifndef Py_INTERNAL_SEMAPHORE_H
#define Py_INTERNAL_SEMAPHORE_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pythread.h"      // _POSIX_SEMAPHORES

#ifdef MS_WINDOWS
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#elif defined(HAVE_PTHREAD_H)
#   include <pthread.h>
#elif defined(HAVE_PTHREAD_STUBS)
#   include "cpython/pthread_stubs.h"
#else
#   error "Require native threads. See https://bugs.python.org/issue31370"
#endif

#if (defined(_POSIX_SEMAPHORES) && (_POSIX_SEMAPHORES+0) != -1 && \
        defined(HAVE_SEM_TIMEDWAIT))
#   define _Py_USE_SEMAPHORES
#   include <semaphore.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PySemaphore {
#if defined(MS_WINDOWS)
    HANDLE platform_sem;
#elif defined(_Py_USE_SEMAPHORES)
    sem_t platform_sem;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int counter;
#endif
} _PySemaphore;

// Puts the current thread to sleep until _PySemaphore_Wakeup() is called.
PyAPI_FUNC(int)
_PySemaphore_Wait(_PySemaphore *sema, PyTime_t timeout_ns);

// Wakes up a single thread waiting on sema. Note that _PySemaphore_Wakeup()
// can be called before _PySemaphore_Wait().
PyAPI_FUNC(void)
_PySemaphore_Wakeup(_PySemaphore *sema);

// Initializes/destroys a semaphore
PyAPI_FUNC(void) _PySemaphore_Init(_PySemaphore *sema);
PyAPI_FUNC(void) _PySemaphore_Destroy(_PySemaphore *sema);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_SEMAPHORE_H */
