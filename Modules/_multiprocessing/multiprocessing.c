/*
 * Extension module used by multiprocessing package
 *
 * multiprocessing.c
 *
 * Copyright (c) 2006-2008, R Oudkerk --- see COPYING.txt
 */

#include "multiprocessing.h"

#ifdef SCM_RIGHTS
    #define HAVE_FD_TRANSFER 1
#else
    #define HAVE_FD_TRANSFER 0
#endif

PyObject *create_win32_namespace(void);

PyObject *pickle_dumps, *pickle_loads, *pickle_protocol;
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
    case MP_END_OF_FILE:
        PyErr_SetNone(PyExc_EOFError);
        break;
    case MP_EARLY_END_OF_FILE:
        PyErr_SetString(PyExc_IOError,
                        "got end of file during message");
        break;
    case MP_BAD_MESSAGE_LENGTH:
        PyErr_SetString(PyExc_IOError, "bad message length");
        break;
    case MP_EXCEPTION_HAS_BEEN_SET:
        break;
    default:
        PyErr_Format(PyExc_RuntimeError,
                     "unkown error number %d", num);
    }
    return NULL;
}


/*
 * Windows only
 */

#ifdef MS_WINDOWS

/* On Windows we set an event to signal Ctrl-C; compare with timemodule.c */

HANDLE sigint_event = NULL;

static BOOL WINAPI
ProcessingCtrlHandler(DWORD dwCtrlType)
{
    SetEvent(sigint_event);
    return FALSE;
}

/*
 * Unix only
 */

#else /* !MS_WINDOWS */

#if HAVE_FD_TRANSFER

/* Functions for transferring file descriptors between processes.
   Reimplements some of the functionality of the fdcred
   module at http://www.mca-ltd.com/resources/fdcred_1.tgz. */

static PyObject *
multiprocessing_sendfd(PyObject *self, PyObject *args)
{
    int conn, fd, res;
    char dummy_char;
    char buf[CMSG_SPACE(sizeof(int))];
    struct msghdr msg = {0};
    struct iovec dummy_iov;
    struct cmsghdr *cmsg;

    if (!PyArg_ParseTuple(args, "ii", &conn, &fd))
        return NULL;

    dummy_iov.iov_base = &dummy_char;
    dummy_iov.iov_len = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);
    msg.msg_iov = &dummy_iov;
    msg.msg_iovlen = 1;
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    msg.msg_controllen = cmsg->cmsg_len;
    *(int*)CMSG_DATA(cmsg) = fd;

    Py_BEGIN_ALLOW_THREADS
    res = sendmsg(conn, &msg, 0);
    Py_END_ALLOW_THREADS

    if (res < 0)
        return PyErr_SetFromErrno(PyExc_OSError);
    Py_RETURN_NONE;
}

static PyObject *
multiprocessing_recvfd(PyObject *self, PyObject *args)
{
    int conn, fd, res;
    char dummy_char;
    char buf[CMSG_SPACE(sizeof(int))];
    struct msghdr msg = {0};
    struct iovec dummy_iov;
    struct cmsghdr *cmsg;

    if (!PyArg_ParseTuple(args, "i", &conn))
        return NULL;

    dummy_iov.iov_base = &dummy_char;
    dummy_iov.iov_len = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);
    msg.msg_iov = &dummy_iov;
    msg.msg_iovlen = 1;
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    msg.msg_controllen = cmsg->cmsg_len;

    Py_BEGIN_ALLOW_THREADS
    res = recvmsg(conn, &msg, 0);
    Py_END_ALLOW_THREADS

    if (res < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    fd = *(int*)CMSG_DATA(cmsg);
    return Py_BuildValue("i", fd);
}

#endif /* HAVE_FD_TRANSFER */

#endif /* !MS_WINDOWS */


/*
 * All platforms
 */

static PyObject*
multiprocessing_address_of_buffer(PyObject *self, PyObject *obj)
{
    void *buffer;
    Py_ssize_t buffer_len;

    if (PyObject_AsWriteBuffer(obj, &buffer, &buffer_len) < 0)
        return NULL;

    return Py_BuildValue("N" F_PY_SSIZE_T,
                         PyLong_FromVoidPtr(buffer), buffer_len);
}


/*
 * Function table
 */

static PyMethodDef module_methods[] = {
    {"address_of_buffer", multiprocessing_address_of_buffer, METH_O,
     "address_of_buffer(obj) -> int\n"
     "Return address of obj assuming obj supports buffer inteface"},
#if HAVE_FD_TRANSFER
    {"sendfd", multiprocessing_sendfd, METH_VARARGS,
     "sendfd(sockfd, fd) -> None\n"
     "Send file descriptor given by fd over the unix domain socket\n"
     "whose file decriptor is sockfd"},
    {"recvfd", multiprocessing_recvfd, METH_VARARGS,
     "recvfd(sockfd) -> fd\n"
     "Receive a file descriptor over a unix domain socket\n"
     "whose file decriptor is sockfd"},
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
    PyObject *module, *temp, *value;

    /* Initialize module */
    module = PyModule_Create(&multiprocessing_module);
    if (!module)
        return NULL;

    /* Get copy of objects from pickle */
    temp = PyImport_ImportModule(PICKLE_MODULE);
    if (!temp)
        return NULL;
    pickle_dumps = PyObject_GetAttrString(temp, "dumps");
    pickle_loads = PyObject_GetAttrString(temp, "loads");
    pickle_protocol = PyObject_GetAttrString(temp, "HIGHEST_PROTOCOL");
    Py_XDECREF(temp);

    /* Get copy of BufferTooShort */
    temp = PyImport_ImportModule("multiprocessing");
    if (!temp)
        return NULL;
    BufferTooShort = PyObject_GetAttrString(temp, "BufferTooShort");
    Py_XDECREF(temp);

    /* Add connection type to module */
    if (PyType_Ready(&ConnectionType) < 0)
        return NULL;
    Py_INCREF(&ConnectionType);
    PyModule_AddObject(module, "Connection", (PyObject*)&ConnectionType);

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
    /* Add PipeConnection to module */
    if (PyType_Ready(&PipeConnectionType) < 0)
        return NULL;
    Py_INCREF(&PipeConnectionType);
    PyModule_AddObject(module, "PipeConnection",
                       (PyObject*)&PipeConnectionType);

    /* Initialize win32 class and add to multiprocessing */
    temp = create_win32_namespace();
    if (!temp)
        return NULL;
    PyModule_AddObject(module, "win32", temp);

    /* Initialize the event handle used to signal Ctrl-C */
    sigint_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!sigint_event) {
        PyErr_SetFromWindowsErr(0);
        return NULL;
    }
    if (!SetConsoleCtrlHandler(ProcessingCtrlHandler, TRUE)) {
        PyErr_SetFromWindowsErr(0);
        return NULL;
    }
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
#ifdef HAVE_FD_TRANSFER
    ADD_FLAG(HAVE_FD_TRANSFER);
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
