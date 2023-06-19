
/* Thread package.
   This is intended to be usable independently from Python.
   The implementation for system foobar is in a file thread_foobar.h
   which is included by this file dependent on config settings.
   Stuff shared by all thread_*.h files is collected here. */

#include "Python.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_structseq.h"     // _PyStructSequence_FiniBuiltin()
#include "pycore_pythread.h"

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

#include <stdlib.h>


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
