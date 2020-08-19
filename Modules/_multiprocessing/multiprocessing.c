/*
 * Extension module used by multiprocessing package
 *
 * multiprocessing.c
 *
 * Copyright (c) 2006-2008, R Oudkerk
 * Licensed to PSF under a Contributor Agreement.
 */

#include "multiprocessing.h"

/*[python input]
class HANDLE_converter(CConverter):
    type = "HANDLE"
    format_unit = '"F_HANDLE"'

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=9fad6080b79ace91]*/

/*[clinic input]
module _multiprocessing
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=01e0745f380ac6e3]*/

#include "clinic/multiprocessing.c.h"

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
/*[clinic input]
_multiprocessing.closesocket

    handle: HANDLE
    /

[clinic start generated code]*/

static PyObject *
_multiprocessing_closesocket_impl(PyObject *module, HANDLE handle)
/*[clinic end generated code: output=214f359f900966f4 input=8a20706dd386c6cc]*/
{
    int ret;

    Py_BEGIN_ALLOW_THREADS
    ret = closesocket((SOCKET) handle);
    Py_END_ALLOW_THREADS

    if (ret)
        return PyErr_SetExcFromWindowsErr(PyExc_OSError, WSAGetLastError());
    Py_RETURN_NONE;
}

/*[clinic input]
_multiprocessing.recv

    handle: HANDLE
    size: int
    /

[clinic start generated code]*/

static PyObject *
_multiprocessing_recv_impl(PyObject *module, HANDLE handle, int size)
/*[clinic end generated code: output=92322781ba9ff598 input=6a5b0834372cee5b]*/
{
    int nread;
    PyObject *buf;

    buf = PyBytes_FromStringAndSize(NULL, size);
    if (!buf)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    nread = recv((SOCKET) handle, PyBytes_AS_STRING(buf), size, 0);
    Py_END_ALLOW_THREADS

    if (nread < 0) {
        Py_DECREF(buf);
        return PyErr_SetExcFromWindowsErr(PyExc_OSError, WSAGetLastError());
    }
    _PyBytes_Resize(&buf, nread);
    return buf;
}

/*[clinic input]
_multiprocessing.send

    handle: HANDLE
    buf: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_multiprocessing_send_impl(PyObject *module, HANDLE handle, Py_buffer *buf)
/*[clinic end generated code: output=52d7df0519c596cb input=41dce742f98d2210]*/
{
    int ret, length;

    length = (int)Py_MIN(buf->len, INT_MAX);

    Py_BEGIN_ALLOW_THREADS
    ret = send((SOCKET) handle, buf->buf, length, 0);
    Py_END_ALLOW_THREADS

    if (ret < 0)
        return PyErr_SetExcFromWindowsErr(PyExc_OSError, WSAGetLastError());
    return PyLong_FromLong(ret);
}

#endif

/*[clinic input]
_multiprocessing.sem_unlink

    name: str
    /

[clinic start generated code]*/

static PyObject *
_multiprocessing_sem_unlink_impl(PyObject *module, const char *name)
/*[clinic end generated code: output=fcbfeb1ed255e647 input=bf939aff9564f1d5]*/
{
    return _PyMp_sem_unlink(name);
}

/*
 * Function table
 */

static PyMethodDef module_methods[] = {
#ifdef MS_WINDOWS
    _MULTIPROCESSING_CLOSESOCKET_METHODDEF
    _MULTIPROCESSING_RECV_METHODDEF
    _MULTIPROCESSING_SEND_METHODDEF
#endif
#if !defined(POSIX_SEMAPHORES_NOT_ENABLED) && !defined(__ANDROID__)
    _MULTIPROCESSING_SEM_UNLINK_METHODDEF
#endif
    {NULL}
};


/*
 * Initialize
 */

static int
multiprocessing_exec(PyObject *module)
{
#if defined(MS_WINDOWS) ||                                              \
  (defined(HAVE_SEM_OPEN) && !defined(POSIX_SEMAPHORES_NOT_ENABLED))

    /* Add _PyMp_SemLock type to module */
    if (PyModule_AddType(module, &_PyMp_SemLockType) < 0) {
        return -1;
    }

    {
        PyObject *py_sem_value_max;
        /* Some systems define SEM_VALUE_MAX as an unsigned value that
         * causes it to be negative when used as an int (NetBSD).
         *
         * Issue #28152: Use (0) instead of 0 to fix a warning on dead code
         * when using clang -Wunreachable-code. */
        if ((int)(SEM_VALUE_MAX) < (0))
            py_sem_value_max = PyLong_FromLong(INT_MAX);
        else
            py_sem_value_max = PyLong_FromLong(SEM_VALUE_MAX);

        if (py_sem_value_max == NULL) {
            Py_DECREF(py_sem_value_max);
            return -1;
        }
        if (PyDict_SetItemString(_PyMp_SemLockType.tp_dict, "SEM_VALUE_MAX",
                             py_sem_value_max) < 0) {
            Py_DECREF(py_sem_value_max);
            return -1;
        }
        Py_DECREF(py_sem_value_max);
    }

#endif

    /* Add configuration macros */
    PyObject *flags = PyDict_New();
    if (!flags) {
        return -1;
    }

#define ADD_FLAG(name)                                          \
    do {                                                        \
        PyObject *value = PyLong_FromLong(name);                \
        if (value == NULL) {                                    \
            Py_DECREF(flags);                                   \
            return -1;                                          \
        }                                                       \
        if (PyDict_SetItemString(flags, #name, value) < 0) {    \
            Py_DECREF(flags);                                   \
            Py_DECREF(value);                                   \
            return -1;                                          \
        }                                                       \
        Py_DECREF(value);                                       \
    } while (0)

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

    if (PyModule_AddObject(module, "flags", flags) < 0) {
        Py_DECREF(flags);
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot multiprocessing_slots[] = {
    {Py_mod_exec, multiprocessing_exec},
    {0, NULL}
};

static struct PyModuleDef multiprocessing_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_multiprocessing",
    .m_methods = module_methods,
    .m_slots = multiprocessing_slots,
};

PyMODINIT_FUNC
PyInit__multiprocessing(void)
{
    return PyModuleDef_Init(&multiprocessing_module);
}
