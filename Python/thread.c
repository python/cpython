
/* Thread package.
   This is intended to be usable independently from Python.
   The implementation for system foobar is in a file thread_foobar.h
   which is included by this file dependent on config settings.
   Stuff shared by all thread_*.h files is collected here. */

#include "Python.h"
#include "pycore_pystate.h"

#ifndef _POSIX_THREADS
/* This means pthreads are not implemented in libc headers, hence the macro
   not present in unistd.h. But they still can be implemented as an external
   library (e.g. gnu pth in pthread emulation) */
# ifdef HAVE_PTHREAD_H
#  include <pthread.h> /* _POSIX_THREADS */
# endif
#endif

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

#include <stdlib.h>

#include "pythread.h"

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


#ifdef Py_DEBUG
static int thread_debug = 0;
#define dprintf(args)   (void)((thread_debug & 1) && printf args)
#define d2printf(args)  ((thread_debug & 8) && printf args)
#else
#define dprintf(args)
#define d2printf(args)
#endif

static int initialized;

static void PyThread__init_thread(void); /* Forward */

void
PyThread_init_thread(void)
{
#ifdef Py_DEBUG
    const char *p = Py_GETENV("PYTHONTHREADDEBUG");

    if (p) {
        if (*p)
            thread_debug = atoi(p);
        else
            thread_debug = 1;
    }
#endif /* Py_DEBUG */
    if (initialized)
        return;
    initialized = 1;
    dprintf(("PyThread_init_thread called\n"));
    PyThread__init_thread();
}

#if defined(_POSIX_THREADS)
#   define PYTHREAD_NAME "pthread"
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
    return _PyInterpreterState_Get()->pythread_stacksize;
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

    if (ThreadInfoType.tp_name == 0) {
        if (PyStructSequence_InitType2(&ThreadInfoType, &threadinfo_desc) < 0)
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

#ifdef _POSIX_THREADS
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
    Py_INCREF(Py_None);
    value = Py_None;
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
        Py_INCREF(Py_None);
        value = Py_None;
    }
    PyStructSequence_SET_ITEM(threadinfo, pos++, value);
    return threadinfo;
}
