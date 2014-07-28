/*
 * Extension module used by multiprocessing package
 *
 * multiprocessing.c
 *
 * Copyright (c) 2006-2008, R Oudkerk
 * Licensed to PSF under a Contributor Agreement.
 */

#include "multiprocessing.h"


/*
 * Function which raises exceptions based on error codes
 */

PyObject *
_PyMp_SetError(PyObject *Type, int num)
{
    switch (num) {
#ifdef MS_WINDOWS
    case MP_STANDARD_ERROR:
        if (Type == NULL)
            Type = PyExc_OSError;
        PyErr_SetExcFromWindowsErr(Type, 0);
        break;
    case MP_SOCKET_ERROR:
        if (Type == NULL)
            Type = PyExc_OSError;
        PyErr_SetExcFromWindowsErr(Type, WSAGetLastError());
        break;
#else /* !MS_WINDOWS */
    case MP_STANDARD_ERROR:
    case MP_SOCKET_ERROR:
        if (Type == NULL)
            Type = PyExc_OSError;
        PyErr_SetFromErrno(Type);
        break;
#endif /* !MS_WINDOWS */
    case MP_MEMORY_ERROR:
        PyErr_NoMemory();
        break;
    case MP_EXCEPTION_HAS_BEEN_SET:
        break;
    default:
        PyErr_Format(PyExc_RuntimeError,
                     "unknown error number %d", num);
    }
    return NULL;
}

#ifdef MS_WINDOWS
static PyObject *
multiprocessing_closesocket(PyObject *self, PyObject *args)
{
    HANDLE handle;
    int ret;

    if (!PyArg_ParseTuple(args, F_HANDLE ":closesocket" , &handle))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ret = closesocket((SOCKET) handle);
    Py_END_ALLOW_THREADS

    if (ret)
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, WSAGetLastError());
    Py_RETURN_NONE;
}

static PyObject *
multiprocessing_recv(PyObject *self, PyObject *args)
{
    HANDLE handle;
    int size, nread;
    PyObject *buf;

    if (!PyArg_ParseTuple(args, F_HANDLE "i:recv" , &handle, &size))
        return NULL;

    buf = PyBytes_FromStringAndSize(NULL, size);
    if (!buf)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    nread = recv((SOCKET) handle, PyBytes_AS_STRING(buf), size, 0);
    Py_END_ALLOW_THREADS

    if (nread < 0) {
        Py_DECREF(buf);
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, WSAGetLastError());
    }
    _PyBytes_Resize(&buf, nread);
    return buf;
}

static PyObject *
multiprocessing_send(PyObject *self, PyObject *args)
{
    HANDLE handle;
    Py_buffer buf;
    int ret, length;

    if (!PyArg_ParseTuple(args, F_HANDLE "y*:send" , &handle, &buf))
        return NULL;

    length = (int)Py_MIN(buf.len, INT_MAX);

    Py_BEGIN_ALLOW_THREADS
    ret = send((SOCKET) handle, buf.buf, length, 0);
    Py_END_ALLOW_THREADS

    PyBuffer_Release(&buf);
    if (ret < 0)
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, WSAGetLastError());
    return PyLong_FromLong(ret);
}

#endif

/*
 * Function table
 */

static PyMethodDef module_methods[] = {
#ifdef MS_WINDOWS
    {"closesocket", multiprocessing_closesocket, METH_VARARGS, ""},
    {"recv", multiprocessing_recv, METH_VARARGS, ""},
    {"send", multiprocessing_send, METH_VARARGS, ""},
#endif
#ifndef POSIX_SEMAPHORES_NOT_ENABLED
    {"sem_unlink", _PyMp_sem_unlink, METH_VARARGS, ""},
#endif
    {NULL}
};


/*
 * Initialize
 */

static struct PyModuleDef multiprocessing_module = {
    PyModuleDef_HEAD_INIT,
    "_multiprocessing",
    NULL,
    -1,
    module_methods,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__multiprocessing(void)
{
    PyObject *module, *temp, *value = NULL;

    /* Initialize module */
    module = PyModule_Create(&multiprocessing_module);
    if (!module)
        return NULL;

#if defined(MS_WINDOWS) ||                                              \
  (defined(HAVE_SEM_OPEN) && !defined(POSIX_SEMAPHORES_NOT_ENABLED))
    /* Add _PyMp_SemLock type to module */
    if (PyType_Ready(&_PyMp_SemLockType) < 0)
        return NULL;
    Py_INCREF(&_PyMp_SemLockType);
    {
        PyObject *py_sem_value_max;
        /* Some systems define SEM_VALUE_MAX as an unsigned value that
         * causes it to be negative when used as an int (NetBSD). */
        if ((int)(SEM_VALUE_MAX) < 0)
            py_sem_value_max = PyLong_FromLong(INT_MAX);
        else
            py_sem_value_max = PyLong_FromLong(SEM_VALUE_MAX);
        if (py_sem_value_max == NULL)
            return NULL;
        PyDict_SetItemString(_PyMp_SemLockType.tp_dict, "SEM_VALUE_MAX",
                             py_sem_value_max);
    }
    PyModule_AddObject(module, "SemLock", (PyObject*)&_PyMp_SemLockType);
#endif

    /* Add configuration macros */
    temp = PyDict_New();
    if (!temp)
        return NULL;

#define ADD_FLAG(name)                                            \
    value = Py_BuildValue("i", name);                             \
    if (value == NULL) { Py_DECREF(temp); return NULL; }          \
    if (PyDict_SetItemString(temp, #name, value) < 0) {           \
        Py_DECREF(temp); Py_DECREF(value); return NULL; }         \
    Py_DECREF(value)

#if defined(HAVE_SEM_OPEN) && !defined(POSIX_SEMAPHORES_NOT_ENABLED)
    ADD_FLAG(HAVE_SEM_OPEN);
#endif
#ifdef HAVE_SEM_TIMEDWAIT
    ADD_FLAG(HAVE_SEM_TIMEDWAIT);
#endif
#ifdef HAVE_BROKEN_SEM_GETVALUE
    ADD_FLAG(HAVE_BROKEN_SEM_GETVALUE);
#endif
#ifdef HAVE_BROKEN_SEM_UNLINK
    ADD_FLAG(HAVE_BROKEN_SEM_UNLINK);
#endif

    if (PyModule_AddObject(module, "flags", temp) < 0)
        return NULL;

    return module;
}
