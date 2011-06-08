/*
 * Win32 functions used by multiprocessing package
 *
 * win32_functions.c
 *
 * Copyright (c) 2006-2008, R Oudkerk --- see COPYING.txt
 */

#include "multiprocessing.h"


#define WIN32_FUNCTION(func) \
    {#func, (PyCFunction)win32_ ## func, METH_VARARGS | METH_STATIC, ""}

#define WIN32_KWARGS_FUNCTION(func) \
    {#func, (PyCFunction)win32_ ## func, METH_VARARGS | METH_KEYWORDS | METH_STATIC, ""}

#define WIN32_CONSTANT(fmt, con) \
    PyDict_SetItemString(Win32Type.tp_dict, #con, Py_BuildValue(fmt, con))


/* Grab CancelIoEx dynamically from kernel32 */
static int has_CancelIoEx = -1;
static BOOL (CALLBACK *Py_CancelIoEx)(HANDLE, LPOVERLAPPED);

static int
check_CancelIoEx()
{
    if (has_CancelIoEx == -1)
    {
        HINSTANCE hKernel32 = GetModuleHandle("KERNEL32");
        * (FARPROC *) &Py_CancelIoEx = GetProcAddress(hKernel32,
                                                      "CancelIoEx");
        has_CancelIoEx = (Py_CancelIoEx != NULL);
    }
    return has_CancelIoEx;
}


/*
 * A Python object wrapping an OVERLAPPED structure and other useful data
 * for overlapped I/O
 */

typedef struct {
    PyObject_HEAD
    OVERLAPPED overlapped;
    /* For convenience, we store the file handle too */
    HANDLE handle;
    /* Whether there's I/O in flight */
    int pending;
    /* Whether I/O completed successfully */
    int completed;
    /* Buffer used for reading (optional) */
    PyObject *read_buffer;
    /* Buffer used for writing (optional) */
    Py_buffer write_buffer;
} OverlappedObject;

static void
overlapped_dealloc(OverlappedObject *self)
{
    int err = GetLastError();
    if (self->pending) {
        if (check_CancelIoEx())
            Py_CancelIoEx(self->handle, &self->overlapped);
        else {
            PyErr_SetString(PyExc_RuntimeError,
                            "I/O operations still in flight while destroying "
                            "Overlapped object, the process may crash");
            PyErr_WriteUnraisable(NULL);
        }
    }
    CloseHandle(self->overlapped.hEvent);
    SetLastError(err);
    if (self->write_buffer.obj)
        PyBuffer_Release(&self->write_buffer);
    Py_CLEAR(self->read_buffer);
    PyObject_Del(self);
}

static PyObject *
overlapped_GetOverlappedResult(OverlappedObject *self, PyObject *waitobj)
{
    int wait;
    BOOL res;
    DWORD transferred = 0;

    wait = PyObject_IsTrue(waitobj);
    if (wait < 0)
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = GetOverlappedResult(self->handle, &self->overlapped, &transferred,
                              wait != 0);
    Py_END_ALLOW_THREADS

    if (!res) {
        int err = GetLastError();
        if (err == ERROR_IO_INCOMPLETE)
            Py_RETURN_NONE;
        if (err != ERROR_MORE_DATA) {
            self->pending = 0;
            return PyErr_SetExcFromWindowsErr(PyExc_IOError, err);
        }
    }
    self->pending = 0;
    self->completed = 1;
    if (self->read_buffer) {
        assert(PyBytes_CheckExact(self->read_buffer));
        if (_PyBytes_Resize(&self->read_buffer, transferred))
            return NULL;
    }
    return Py_BuildValue("lN", (long) transferred, PyBool_FromLong(res));
}

static PyObject *
overlapped_getbuffer(OverlappedObject *self)
{
    PyObject *res;
    if (!self->completed) {
        PyErr_SetString(PyExc_ValueError,
                        "can't get read buffer before GetOverlappedResult() "
                        "signals the operation completed");
        return NULL;
    }
    res = self->read_buffer ? self->read_buffer : Py_None;
    Py_INCREF(res);
    return res;
}

static PyObject *
overlapped_cancel(OverlappedObject *self)
{
    BOOL res = TRUE;

    if (self->pending) {
        Py_BEGIN_ALLOW_THREADS
        if (check_CancelIoEx())
            res = Py_CancelIoEx(self->handle, &self->overlapped);
        else
            res = CancelIo(self->handle);
        Py_END_ALLOW_THREADS
    }

    /* CancelIoEx returns ERROR_NOT_FOUND if the I/O completed in-between */
    if (!res && GetLastError() != ERROR_NOT_FOUND)
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
    self->pending = 0;
    Py_RETURN_NONE;
}

static PyMethodDef overlapped_methods[] = {
    {"GetOverlappedResult", (PyCFunction) overlapped_GetOverlappedResult,
                            METH_O, NULL},
    {"getbuffer", (PyCFunction) overlapped_getbuffer, METH_NOARGS, NULL},
    {"cancel", (PyCFunction) overlapped_cancel, METH_NOARGS, NULL},
    {NULL}
};

static PyMemberDef overlapped_members[] = {
    {"event", T_HANDLE,
     offsetof(OverlappedObject, overlapped) + offsetof(OVERLAPPED, hEvent),
     READONLY, "overlapped event handle"},
    {NULL}
};

PyTypeObject OverlappedType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    /* tp_name           */ "_multiprocessing.win32.Overlapped",
    /* tp_basicsize      */ sizeof(OverlappedObject),
    /* tp_itemsize       */ 0,
    /* tp_dealloc        */ (destructor) overlapped_dealloc,
    /* tp_print          */ 0,
    /* tp_getattr        */ 0,
    /* tp_setattr        */ 0,
    /* tp_reserved       */ 0,
    /* tp_repr           */ 0,
    /* tp_as_number      */ 0,
    /* tp_as_sequence    */ 0,
    /* tp_as_mapping     */ 0,
    /* tp_hash           */ 0,
    /* tp_call           */ 0,
    /* tp_str            */ 0,
    /* tp_getattro       */ 0,
    /* tp_setattro       */ 0,
    /* tp_as_buffer      */ 0,
    /* tp_flags          */ Py_TPFLAGS_DEFAULT,
    /* tp_doc            */ "OVERLAPPED structure wrapper",
    /* tp_traverse       */ 0,
    /* tp_clear          */ 0,
    /* tp_richcompare    */ 0,
    /* tp_weaklistoffset */ 0,
    /* tp_iter           */ 0,
    /* tp_iternext       */ 0,
    /* tp_methods        */ overlapped_methods,
    /* tp_members        */ overlapped_members,
    /* tp_getset         */ 0,
    /* tp_base           */ 0,
    /* tp_dict           */ 0,
    /* tp_descr_get      */ 0,
    /* tp_descr_set      */ 0,
    /* tp_dictoffset     */ 0,
    /* tp_init           */ 0,
    /* tp_alloc          */ 0,
    /* tp_new            */ 0,
};

static OverlappedObject *
new_overlapped(HANDLE handle)
{
    OverlappedObject *self;

    self = PyObject_New(OverlappedObject, &OverlappedType);
    if (!self)
        return NULL;
    self->handle = handle;
    self->read_buffer = NULL;
    self->pending = 0;
    self->completed = 0;
    memset(&self->overlapped, 0, sizeof(OVERLAPPED));
    memset(&self->write_buffer, 0, sizeof(Py_buffer));
    /* Manual reset, initially non-signalled */
    self->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    return self;
}


/*
 * Module functions
 */

static PyObject *
win32_CloseHandle(PyObject *self, PyObject *args)
{
    HANDLE hObject;
    BOOL success;

    if (!PyArg_ParseTuple(args, F_HANDLE, &hObject))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    success = CloseHandle(hObject);
    Py_END_ALLOW_THREADS

    if (!success)
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}

static PyObject *
win32_ConnectNamedPipe(PyObject *self, PyObject *args, PyObject *kwds)
{
    HANDLE hNamedPipe;
    int use_overlapped = 0;
    BOOL success;
    OverlappedObject *overlapped = NULL;
    static char *kwlist[] = {"handle", "overlapped", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     F_HANDLE "|i", kwlist,
                                     &hNamedPipe, &use_overlapped))
        return NULL;

    if (use_overlapped) {
        overlapped = new_overlapped(hNamedPipe);
        if (!overlapped)
            return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    success = ConnectNamedPipe(hNamedPipe,
                               overlapped ? &overlapped->overlapped : NULL);
    Py_END_ALLOW_THREADS

    if (overlapped) {
        int err = GetLastError();
        /* Overlapped ConnectNamedPipe never returns a success code */
        assert(success == 0);
        if (err == ERROR_IO_PENDING)
            overlapped->pending = 1;
        else if (err == ERROR_PIPE_CONNECTED)
            SetEvent(overlapped->overlapped.hEvent);
        else {
            Py_DECREF(overlapped);
            return PyErr_SetFromWindowsErr(err);
        }
        return (PyObject *) overlapped;
    }
    if (!success)
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}

static PyObject *
win32_CreateFile(PyObject *self, PyObject *args)
{
    LPCTSTR lpFileName;
    DWORD dwDesiredAccess;
    DWORD dwShareMode;
    LPSECURITY_ATTRIBUTES lpSecurityAttributes;
    DWORD dwCreationDisposition;
    DWORD dwFlagsAndAttributes;
    HANDLE hTemplateFile;
    HANDLE handle;

    if (!PyArg_ParseTuple(args, "s" F_DWORD F_DWORD F_POINTER
                          F_DWORD F_DWORD F_HANDLE,
                          &lpFileName, &dwDesiredAccess, &dwShareMode,
                          &lpSecurityAttributes, &dwCreationDisposition,
                          &dwFlagsAndAttributes, &hTemplateFile))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    handle = CreateFile(lpFileName, dwDesiredAccess,
                        dwShareMode, lpSecurityAttributes,
                        dwCreationDisposition,
                        dwFlagsAndAttributes, hTemplateFile);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE)
        return PyErr_SetFromWindowsErr(0);

    return Py_BuildValue(F_HANDLE, handle);
}

static PyObject *
win32_CreateNamedPipe(PyObject *self, PyObject *args)
{
    LPCTSTR lpName;
    DWORD dwOpenMode;
    DWORD dwPipeMode;
    DWORD nMaxInstances;
    DWORD nOutBufferSize;
    DWORD nInBufferSize;
    DWORD nDefaultTimeOut;
    LPSECURITY_ATTRIBUTES lpSecurityAttributes;
    HANDLE handle;

    if (!PyArg_ParseTuple(args, "s" F_DWORD F_DWORD F_DWORD
                          F_DWORD F_DWORD F_DWORD F_POINTER,
                          &lpName, &dwOpenMode, &dwPipeMode,
                          &nMaxInstances, &nOutBufferSize,
                          &nInBufferSize, &nDefaultTimeOut,
                          &lpSecurityAttributes))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    handle = CreateNamedPipe(lpName, dwOpenMode, dwPipeMode,
                             nMaxInstances, nOutBufferSize,
                             nInBufferSize, nDefaultTimeOut,
                             lpSecurityAttributes);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE)
        return PyErr_SetFromWindowsErr(0);

    return Py_BuildValue(F_HANDLE, handle);
}

static PyObject *
win32_ExitProcess(PyObject *self, PyObject *args)
{
    UINT uExitCode;

    if (!PyArg_ParseTuple(args, "I", &uExitCode))
        return NULL;

    #if defined(Py_DEBUG)
        SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    #endif


    ExitProcess(uExitCode);

    return NULL;
}

static PyObject *
win32_GetLastError(PyObject *self, PyObject *args)
{
    return Py_BuildValue(F_DWORD, GetLastError());
}

static PyObject *
win32_OpenProcess(PyObject *self, PyObject *args)
{
    DWORD dwDesiredAccess;
    BOOL bInheritHandle;
    DWORD dwProcessId;
    HANDLE handle;

    if (!PyArg_ParseTuple(args, F_DWORD "i" F_DWORD,
                          &dwDesiredAccess, &bInheritHandle, &dwProcessId))
        return NULL;

    handle = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
    if (handle == NULL)
        return PyErr_SetFromWindowsErr(0);

    return Py_BuildValue(F_HANDLE, handle);
}

static PyObject *
win32_SetNamedPipeHandleState(PyObject *self, PyObject *args)
{
    HANDLE hNamedPipe;
    PyObject *oArgs[3];
    DWORD dwArgs[3], *pArgs[3] = {NULL, NULL, NULL};
    int i;

    if (!PyArg_ParseTuple(args, F_HANDLE "OOO",
                          &hNamedPipe, &oArgs[0], &oArgs[1], &oArgs[2]))
        return NULL;

    PyErr_Clear();

    for (i = 0 ; i < 3 ; i++) {
        if (oArgs[i] != Py_None) {
            dwArgs[i] = PyLong_AsUnsignedLongMask(oArgs[i]);
            if (PyErr_Occurred())
                return NULL;
            pArgs[i] = &dwArgs[i];
        }
    }

    if (!SetNamedPipeHandleState(hNamedPipe, pArgs[0], pArgs[1], pArgs[2]))
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}

static PyObject *
win32_WaitNamedPipe(PyObject *self, PyObject *args)
{
    LPCTSTR lpNamedPipeName;
    DWORD nTimeOut;
    BOOL success;

    if (!PyArg_ParseTuple(args, "s" F_DWORD, &lpNamedPipeName, &nTimeOut))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    success = WaitNamedPipe(lpNamedPipeName, nTimeOut);
    Py_END_ALLOW_THREADS

    if (!success)
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}

static PyObject *
win32_closesocket(PyObject *self, PyObject *args)
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
win32_recv(PyObject *self, PyObject *args)
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
win32_send(PyObject *self, PyObject *args)
{
    HANDLE handle;
    Py_buffer buf;
    int ret;

    if (!PyArg_ParseTuple(args, F_HANDLE "y*:send" , &handle, &buf))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ret = send((SOCKET) handle, buf.buf, buf.len, 0);
    Py_END_ALLOW_THREADS

    PyBuffer_Release(&buf);
    if (ret < 0)
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, WSAGetLastError());
    return PyLong_FromLong(ret);
}

static PyObject *
win32_WriteFile(PyObject *self, PyObject *args, PyObject *kwds)
{
    HANDLE handle;
    Py_buffer _buf, *buf;
    PyObject *bufobj;
    int written;
    BOOL ret;
    int use_overlapped = 0;
    OverlappedObject *overlapped = NULL;
    static char *kwlist[] = {"handle", "buffer", "overlapped", NULL};

    /* First get handle and use_overlapped to know which Py_buffer to use */
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     F_HANDLE "O|i:WriteFile", kwlist,
                                     &handle, &bufobj, &use_overlapped))
        return NULL;

    if (use_overlapped) {
        overlapped = new_overlapped(handle);
        if (!overlapped)
            return NULL;
        buf = &overlapped->write_buffer;
    }
    else
        buf = &_buf;

    if (!PyArg_Parse(bufobj, "y*", buf)) {
        Py_XDECREF(overlapped);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    ret = WriteFile(handle, buf->buf, buf->len, &written,
                    overlapped ? &overlapped->overlapped : NULL);
    Py_END_ALLOW_THREADS

    if (overlapped) {
        int err = GetLastError();
        if (!ret) {
            if (err == ERROR_IO_PENDING)
                overlapped->pending = 1;
            else {
                Py_DECREF(overlapped);
                return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
            }
        }
        return (PyObject *) overlapped;
    }

    PyBuffer_Release(buf);
    if (!ret)
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
    return PyLong_FromLong(written);
}

static PyObject *
win32_ReadFile(PyObject *self, PyObject *args, PyObject *kwds)
{
    HANDLE handle;
    int size;
    DWORD nread;
    PyObject *buf;
    BOOL ret;
    int use_overlapped = 0;
    OverlappedObject *overlapped = NULL;
    static char *kwlist[] = {"handle", "size", "overlapped", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     F_HANDLE "i|i:ReadFile", kwlist,
                                     &handle, &size, &use_overlapped))
        return NULL;

    buf = PyBytes_FromStringAndSize(NULL, size);
    if (!buf)
        return NULL;
    if (use_overlapped) {
        overlapped = new_overlapped(handle);
        if (!overlapped) {
            Py_DECREF(buf);
            return NULL;
        }
        /* Steals reference to buf */
        overlapped->read_buffer = buf;
    }

    Py_BEGIN_ALLOW_THREADS
    ret = ReadFile(handle, PyBytes_AS_STRING(buf), size, &nread,
                   overlapped ? &overlapped->overlapped : NULL);
    Py_END_ALLOW_THREADS

    if (overlapped) {
        int err = GetLastError();
        if (!ret) {
            if (err == ERROR_IO_PENDING)
                overlapped->pending = 1;
            else if (err != ERROR_MORE_DATA) {
                Py_DECREF(overlapped);
                return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
            }
        }
        return (PyObject *) overlapped;
    }

    if (!ret && GetLastError() != ERROR_MORE_DATA) {
        Py_DECREF(buf);
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
    }
    if (_PyBytes_Resize(&buf, nread))
        return NULL;
    return Py_BuildValue("NN", buf, PyBool_FromLong(ret));
}

static PyObject *
win32_PeekNamedPipe(PyObject *self, PyObject *args)
{
    HANDLE handle;
    int size = 0;
    PyObject *buf = NULL;
    DWORD nread, navail, nleft;
    BOOL ret;

    if (!PyArg_ParseTuple(args, F_HANDLE "|i:PeekNamedPipe" , &handle, &size))
        return NULL;

    if (size < 0) {
        PyErr_SetString(PyExc_ValueError, "negative size");
        return NULL;
    }

    if (size) {
        buf = PyBytes_FromStringAndSize(NULL, size);
        if (!buf)
            return NULL;
        Py_BEGIN_ALLOW_THREADS
        ret = PeekNamedPipe(handle, PyBytes_AS_STRING(buf), size, &nread,
                            &navail, &nleft);
        Py_END_ALLOW_THREADS
        if (!ret) {
            Py_DECREF(buf);
            return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
        }
        if (_PyBytes_Resize(&buf, nread))
            return NULL;
        return Py_BuildValue("Nii", buf, navail, nleft);
    }
    else {
        Py_BEGIN_ALLOW_THREADS
        ret = PeekNamedPipe(handle, NULL, 0, NULL, &navail, &nleft);
        Py_END_ALLOW_THREADS
        if (!ret) {
            return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
        }
        return Py_BuildValue("ii", navail, nleft);
    }
}

static PyObject *
win32_WaitForMultipleObjects(PyObject* self, PyObject* args)
{
    DWORD result;
    PyObject *handle_seq;
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    Py_ssize_t nhandles, i;
    int wait_flag;
    int milliseconds = INFINITE;

    if (!PyArg_ParseTuple(args, "Oi|i:WaitForMultipleObjects",
                                &handle_seq, &wait_flag, &milliseconds))
        return NULL;

    if (!PySequence_Check(handle_seq)) {
        PyErr_Format(PyExc_TypeError,
                     "sequence type expected, got '%s'",
                     Py_TYPE(handle_seq)->tp_doc);
        return NULL;
    }
    nhandles = PySequence_Length(handle_seq);
    if (nhandles == -1)
        return NULL;
    if (nhandles < 0 || nhandles >= MAXIMUM_WAIT_OBJECTS) {
        PyErr_Format(PyExc_ValueError,
                     "need at most %zd handles, got a sequence of length %zd",
                     MAXIMUM_WAIT_OBJECTS, nhandles);
        return NULL;
    }
    for (i = 0; i < nhandles; i++) {
        HANDLE h;
        PyObject *v = PySequence_GetItem(handle_seq, i);
        if (v == NULL)
            return NULL;
        if (!PyArg_Parse(v, F_HANDLE, &h))
            return NULL;
        handles[i] = h;
    }

    Py_BEGIN_ALLOW_THREADS
    result = WaitForMultipleObjects((DWORD) nhandles, handles,
                                    (BOOL) wait_flag, (DWORD) milliseconds);
    Py_END_ALLOW_THREADS

    if (result == WAIT_FAILED)
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);

    return PyLong_FromLong((int) result);
}


static PyMethodDef win32_methods[] = {
    WIN32_FUNCTION(CloseHandle),
    WIN32_FUNCTION(GetLastError),
    WIN32_FUNCTION(OpenProcess),
    WIN32_FUNCTION(ExitProcess),
    WIN32_KWARGS_FUNCTION(ConnectNamedPipe),
    WIN32_FUNCTION(CreateFile),
    WIN32_FUNCTION(CreateNamedPipe),
    WIN32_KWARGS_FUNCTION(ReadFile),
    WIN32_FUNCTION(PeekNamedPipe),
    WIN32_FUNCTION(SetNamedPipeHandleState),
    WIN32_FUNCTION(WaitForMultipleObjects),
    WIN32_FUNCTION(WaitNamedPipe),
    WIN32_KWARGS_FUNCTION(WriteFile),
    WIN32_FUNCTION(closesocket),
    WIN32_FUNCTION(recv),
    WIN32_FUNCTION(send),
    {NULL}
};


PyTypeObject Win32Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};


PyObject *
create_win32_namespace(void)
{
    Win32Type.tp_name = "_multiprocessing.win32";
    Win32Type.tp_methods = win32_methods;
    if (PyType_Ready(&Win32Type) < 0)
        return NULL;
    Py_INCREF(&Win32Type);

    if (PyType_Ready(&OverlappedType) < 0)
        return NULL;
    PyDict_SetItemString(Win32Type.tp_dict, "Overlapped",
                         (PyObject *) &OverlappedType);

    WIN32_CONSTANT(F_DWORD, ERROR_ALREADY_EXISTS);
    WIN32_CONSTANT(F_DWORD, ERROR_BROKEN_PIPE);
    WIN32_CONSTANT(F_DWORD, ERROR_NO_SYSTEM_RESOURCES);
    WIN32_CONSTANT(F_DWORD, ERROR_PIPE_BUSY);
    WIN32_CONSTANT(F_DWORD, ERROR_PIPE_CONNECTED);
    WIN32_CONSTANT(F_DWORD, ERROR_SEM_TIMEOUT);
    WIN32_CONSTANT(F_DWORD, FILE_FLAG_OVERLAPPED);
    WIN32_CONSTANT(F_DWORD, GENERIC_READ);
    WIN32_CONSTANT(F_DWORD, GENERIC_WRITE);
    WIN32_CONSTANT(F_DWORD, INFINITE);
    WIN32_CONSTANT(F_DWORD, NMPWAIT_WAIT_FOREVER);
    WIN32_CONSTANT(F_DWORD, OPEN_EXISTING);
    WIN32_CONSTANT(F_DWORD, PIPE_ACCESS_DUPLEX);
    WIN32_CONSTANT(F_DWORD, PIPE_ACCESS_INBOUND);
    WIN32_CONSTANT(F_DWORD, PIPE_READMODE_MESSAGE);
    WIN32_CONSTANT(F_DWORD, PIPE_TYPE_MESSAGE);
    WIN32_CONSTANT(F_DWORD, PIPE_UNLIMITED_INSTANCES);
    WIN32_CONSTANT(F_DWORD, PIPE_WAIT);
    WIN32_CONSTANT(F_DWORD, PROCESS_ALL_ACCESS);

    WIN32_CONSTANT("i", NULL);

    return (PyObject*)&Win32Type;
}
