/*
 * Extension module used by multiprocessing package
 *
 * multiprocessing.c
 *
 * Copyright (c) 2006-2008, R Oudkerk --- see COPYING.txt
 */

#include "multiprocessing.h"


PyObject *create_win32_namespace(void);

PyObject *ProcessError, *BufferTooShort;

/*
 * Function which raises exceptions based on error codes
 */

PyObject *
mp_SetError(PyObject *Type, int num)
{
    switch (num) {
#ifdef MS_WINDOWS
    case MP_STANDARD_ERROR:
        if (Type == NULL)
            Type = PyExc_WindowsError;
        PyErr_SetExcFromWindowsErr(Type, 0);
        break;
    case MP_SOCKET_ERROR:
        if (Type == NULL)
            Type = PyExc_WindowsError;
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
                     "unkown error number %d", num);
    }
    return NULL;
}


static PyObject*
multiprocessing_address_of_buffer(PyObject *self, PyObject *obj)
{
    void *buffer;
    Py_ssize_t buffer_len;

    if (PyObject_AsWriteBuffer(obj, &buffer, &buffer_len) < 0)
        return NULL;

    return Py_BuildValue("Nn",
                         PyLong_FromVoidPtr(buffer), buffer_len);
}


/*
 * Function table
 */

static PyMethodDef module_methods[] = {
    {"address_of_buffer", multiprocessing_address_of_buffer, METH_O,
     "address_of_buffer(obj) -> int\n"
     "Return address of obj assuming obj supports buffer inteface"},
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

    /* Get copy of BufferTooShort */
    temp = PyImport_ImportModule("multiprocessing");
    if (!temp)
        return NULL;
    BufferTooShort = PyObject_GetAttrString(temp, "BufferTooShort");
    Py_XDECREF(temp);

#if defined(MS_WINDOWS) ||                                              \
  (defined(HAVE_SEM_OPEN) && !defined(POSIX_SEMAPHORES_NOT_ENABLED))
    /* Add SemLock type to module */
    if (PyType_Ready(&SemLockType) < 0)
        return NULL;
    Py_INCREF(&SemLockType);
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
        PyDict_SetItemString(SemLockType.tp_dict, "SEM_VALUE_MAX",
                             py_sem_value_max);
    }
    PyModule_AddObject(module, "SemLock", (PyObject*)&SemLockType);
#endif

#ifdef MS_WINDOWS
    /* Initialize win32 class and add to multiprocessing */
    temp = create_win32_namespace();
    if (!temp)
        return NULL;
    PyModule_AddObject(module, "win32", temp);
#endif

    /* Add configuration macros */
    temp = PyDict_New();
    if (!temp)
        return NULL;

#define ADD_FLAG(name)                                            \
    value = Py_BuildValue("i", name);                             \
    if (value == NULL) { Py_DECREF(temp); return NULL; }          \
    if (PyDict_SetItemString(temp, #name, value) < 0) {           \
        Py_DECREF(temp); Py_DECREF(value); return NULL; }                 \
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
