
/* Thread package.
   This is intended to be usable independently from Python.
   The implementation for system foobar is in a file thread_foobar.h
   which is included by this file dependent on config settings.
   Stuff shared by all thread_*.h files is collected here. */

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_MakePendingCalls()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_structseq.h"     // _PyStructSequence_FiniBuiltin()
#include "pycore_pythread.h"      // _POSIX_THREADS

#ifndef DONT_HAVE_STDIO_H
#  include <stdio.h>
#endif

#include <stdlib.h>


// Define PY_TIMEOUT_MAX constant.
#ifdef _POSIX_THREADS
   // PyThread_acquire_lock_timed() uses _PyTime_FromNanoseconds(us * 1000),
   // convert microseconds to nanoseconds.
#  define PY_TIMEOUT_MAX_VALUE (LLONG_MAX / 1000)
#elif defined (NT_THREADS)
   // WaitForSingleObject() accepts timeout in milliseconds in the range
   // [0; 0xFFFFFFFE] (DWORD type). INFINITE value (0xFFFFFFFF) means no
   // timeout. 0xFFFFFFFE milliseconds is around 49.7 days.
#  if 0xFFFFFFFELL < LLONG_MAX / 1000
#    define PY_TIMEOUT_MAX_VALUE (0xFFFFFFFELL * 1000)
#  else
#    define PY_TIMEOUT_MAX_VALUE LLONG_MAX
#  endif
#else
#  define PY_TIMEOUT_MAX_VALUE LLONG_MAX
#endif
const long long PY_TIMEOUT_MAX = PY_TIMEOUT_MAX_VALUE;


static void PyThread__init_thread(void); /* Forward */

#define initialized _PyRuntime.threads.initialized

void
PyThread_init_thread(void)
{
    if (initialized) {
        return;
    }
    initialized = 1;
    PyThread__init_thread();
}

#if defined(HAVE_PTHREAD_STUBS)
#   define PYTHREAD_NAME "pthread-stubs"
#   include "thread_pthread_stubs.h"
#elif defined(_USE_PTHREADS)  /* AKA _PTHREADS */
#   if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
#     define PYTHREAD_NAME "pthread-stubs"
#   else
#     define PYTHREAD_NAME "pthread"
#   endif
#   include "thread_pthread.h"
#elif defined(NT_THREADS)
#   define PYTHREAD_NAME "nt"
#   include "thread_nt.h"
#else
#   error "Require native threads. See https://bugs.python.org/issue31370"
#endif


/* return the current thread stack size */
size_t
PyThread_get_stacksize(void)
{
    return _PyInterpreterState_GET()->threads.stacksize;
}

/* Only platforms defining a THREAD_SET_STACKSIZE() macro
   in thread_<platform>.h support changing the stack size.
   Return 0 if stack size is valid,
      -1 if stack size value is invalid,
      -2 if setting stack size is not supported. */
int
PyThread_set_stacksize(size_t size)
{
#if defined(THREAD_SET_STACKSIZE)
    return THREAD_SET_STACKSIZE(size);
#else
    return -2;
#endif
}


int
PyThread_ParseTimeoutArg(PyObject *arg, int blocking, PY_TIMEOUT_T *timeout_p)
{
    assert(_PyTime_FromSeconds(-1) == PyThread_UNSET_TIMEOUT);
    if (arg == NULL || arg == Py_None) {
        *timeout_p = blocking ? PyThread_UNSET_TIMEOUT : 0;
        return 0;
    }
    if (!blocking) {
        PyErr_SetString(PyExc_ValueError,
                        "can't specify a timeout for a non-blocking call");
        return -1;
    }

    _PyTime_t timeout;
    if (_PyTime_FromSecondsObject(&timeout, arg, _PyTime_ROUND_TIMEOUT) < 0) {
        return -1;
    }
    if (timeout < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "timeout value must be a non-negative number");
        return -1;
    }

    if (_PyTime_AsMicroseconds(timeout,
                               _PyTime_ROUND_TIMEOUT) > PY_TIMEOUT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "timeout value is too large");
        return -1;
    }
    *timeout_p = timeout;
    return 0;
}

PyLockStatus
PyThread_acquire_lock_timed_with_retries(PyThread_type_lock lock,
                                         PY_TIMEOUT_T timeout)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyTime_t endtime = 0;
    if (timeout > 0) {
        endtime = _PyDeadline_Init(timeout);
    }

    PyLockStatus r;
    do {
        _PyTime_t microseconds;
        microseconds = _PyTime_AsMicroseconds(timeout, _PyTime_ROUND_CEILING);

        /* first a simple non-blocking try without releasing the GIL */
        r = PyThread_acquire_lock_timed(lock, 0, 0);
        if (r == PY_LOCK_FAILURE && microseconds != 0) {
            Py_BEGIN_ALLOW_THREADS
            r = PyThread_acquire_lock_timed(lock, microseconds, 1);
            Py_END_ALLOW_THREADS
        }

        if (r == PY_LOCK_INTR) {
            /* Run signal handlers if we were interrupted.  Propagate
             * exceptions from signal handlers, such as KeyboardInterrupt, by
             * passing up PY_LOCK_INTR.  */
            if (_PyEval_MakePendingCalls(tstate) < 0) {
                return PY_LOCK_INTR;
            }

            /* If we're using a timeout, recompute the timeout after processing
             * signals, since those can take time.  */
            if (timeout > 0) {
                timeout = _PyDeadline_Get(endtime);

                /* Check for negative values, since those mean block forever.
                 */
                if (timeout < 0) {
                    r = PY_LOCK_FAILURE;
                }
            }
        }
    } while (r == PY_LOCK_INTR);  /* Retry if we were interrupted. */

    return r;
}


/* Thread Specific Storage (TSS) API

   Cross-platform components of TSS API implementation.
*/

Py_tss_t *
PyThread_tss_alloc(void)
{
    Py_tss_t *new_key = (Py_tss_t *)PyMem_RawMalloc(sizeof(Py_tss_t));
    if (new_key == NULL) {
        return NULL;
    }
    new_key->_is_initialized = 0;
    return new_key;
}

void
PyThread_tss_free(Py_tss_t *key)
{
    if (key != NULL) {
        PyThread_tss_delete(key);
        PyMem_RawFree((void *)key);
    }
}

int
PyThread_tss_is_created(Py_tss_t *key)
{
    assert(key != NULL);
    return key->_is_initialized;
}


PyDoc_STRVAR(threadinfo__doc__,
"sys.thread_info\n\
\n\
A named tuple holding information about the thread implementation.");

static PyStructSequence_Field threadinfo_fields[] = {
    {"name",    "name of the thread implementation"},
    {"lock",    "name of the lock implementation"},
    {"version", "name and version of the thread library"},
    {0}
};

static PyStructSequence_Desc threadinfo_desc = {
    "sys.thread_info",           /* name */
    threadinfo__doc__,           /* doc */
    threadinfo_fields,           /* fields */
    3
};

static PyTypeObject ThreadInfoType;

PyObject*
PyThread_GetInfo(void)
{
    PyObject *threadinfo, *value;
    int pos = 0;
#if (defined(_POSIX_THREADS) && defined(HAVE_CONFSTR) \
     && defined(_CS_GNU_LIBPTHREAD_VERSION))
    char buffer[255];
    int len;
#endif

    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (_PyStructSequence_InitBuiltin(interp, &ThreadInfoType, &threadinfo_desc) < 0) {
        return NULL;
    }

    threadinfo = PyStructSequence_New(&ThreadInfoType);
    if (threadinfo == NULL)
        return NULL;

    value = PyUnicode_FromString(PYTHREAD_NAME);
    if (value == NULL) {
        Py_DECREF(threadinfo);
        return NULL;
    }
    PyStructSequence_SET_ITEM(threadinfo, pos++, value);

#ifdef HAVE_PTHREAD_STUBS
    value = Py_NewRef(Py_None);
#elif defined(_POSIX_THREADS)
#ifdef USE_SEMAPHORES
    value = PyUnicode_FromString("semaphore");
#else
    value = PyUnicode_FromString("mutex+cond");
#endif
    if (value == NULL) {
        Py_DECREF(threadinfo);
        return NULL;
    }
#else
    value = Py_NewRef(Py_None);
#endif
    PyStructSequence_SET_ITEM(threadinfo, pos++, value);

#if (defined(_POSIX_THREADS) && defined(HAVE_CONFSTR) \
     && defined(_CS_GNU_LIBPTHREAD_VERSION))
    value = NULL;
    len = confstr(_CS_GNU_LIBPTHREAD_VERSION, buffer, sizeof(buffer));
    if (1 < len && (size_t)len < sizeof(buffer)) {
        value = PyUnicode_DecodeFSDefaultAndSize(buffer, len-1);
        if (value == NULL)
            PyErr_Clear();
    }
    if (value == NULL)
#endif
    {
        value = Py_NewRef(Py_None);
    }
    PyStructSequence_SET_ITEM(threadinfo, pos++, value);
    return threadinfo;
}


void
_PyThread_FiniType(PyInterpreterState *interp)
{
    _PyStructSequence_FiniBuiltin(interp, &ThreadInfoType);
}
