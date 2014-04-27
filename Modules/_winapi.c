/*
 * Support routines from the Windows API
 *
 * This module was originally created by merging PC/_subprocess.c with
 * Modules/_multiprocessing/win32_functions.c.
 *
 * Copyright (c) 2004 by Fredrik Lundh <fredrik@pythonware.com>
 * Copyright (c) 2004 by Secret Labs AB, http://www.pythonware.com
 * Copyright (c) 2004 by Peter Astrand <astrand@lysator.liu.se>
 *
 * By obtaining, using, and/or copying this software and/or its
 * associated documentation, you agree that you have read, understood,
 * and will comply with the following terms and conditions:
 *
 * Permission to use, copy, modify, and distribute this software and
 * its associated documentation for any purpose and without fee is
 * hereby granted, provided that the above copyright notice appears in
 * all copies, and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of the
 * authors not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* Licensed to PSF under a Contributor Agreement. */
/* See http://www.python.org/2.4/license for licensing details. */

#include "Python.h"
#include "structmember.h"

#define WINDOWS_LEAN_AND_MEAN
#include "windows.h"
#include <crtdbg.h>

#if defined(MS_WIN32) && !defined(MS_WIN64)
#define HANDLE_TO_PYNUM(handle) \
    PyLong_FromUnsignedLong((unsigned long) handle)
#define PYNUM_TO_HANDLE(obj) ((HANDLE)PyLong_AsUnsignedLong(obj))
#define F_POINTER "k"
#define T_POINTER T_ULONG
#else
#define HANDLE_TO_PYNUM(handle) \
    PyLong_FromUnsignedLongLong((unsigned long long) handle)
#define PYNUM_TO_HANDLE(obj) ((HANDLE)PyLong_AsUnsignedLongLong(obj))
#define F_POINTER "K"
#define T_POINTER T_ULONGLONG
#endif

#define F_HANDLE F_POINTER
#define F_DWORD "k"
#define F_BOOL "i"
#define F_UINT "I"

#define T_HANDLE T_POINTER

#define DWORD_MAX 4294967295U

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
    DWORD bytes;
    int err = GetLastError();

    if (self->pending) {
        if (check_CancelIoEx() &&
            Py_CancelIoEx(self->handle, &self->overlapped) &&
            GetOverlappedResult(self->handle, &self->overlapped, &bytes, TRUE))
        {
            /* The operation is no longer pending -- nothing to do. */
        }
        else if (_Py_Finalizing == NULL)
        {
            /* The operation is still pending -- give a warning.  This
               will probably only happen on Windows XP. */
            PyErr_SetString(PyExc_RuntimeError,
                            "I/O operations still in flight while destroying "
                            "Overlapped object, the process may crash");
            PyErr_WriteUnraisable(NULL);
        }
        else
        {
            /* The operation is still pending, but the process is
               probably about to exit, so we need not worry too much
               about memory leaks.  Leaking self prevents a potential
               crash.  This can happen when a daemon thread is cleaned
               up at exit -- see #19565.  We only expect to get here
               on Windows XP. */
            CloseHandle(self->overlapped.hEvent);
            SetLastError(err);
            return;
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
    DWORD err;

    wait = PyObject_IsTrue(waitobj);
    if (wait < 0)
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = GetOverlappedResult(self->handle, &self->overlapped, &transferred,
                              wait != 0);
    Py_END_ALLOW_THREADS

    err = res ? ERROR_SUCCESS : GetLastError();
    switch (err) {
        case ERROR_SUCCESS:
        case ERROR_MORE_DATA:
        case ERROR_OPERATION_ABORTED:
            self->completed = 1;
            self->pending = 0;
            break;
        case ERROR_IO_INCOMPLETE:
            break;
        default:
            self->pending = 0;
            return PyErr_SetExcFromWindowsErr(PyExc_IOError, err);
    }
    if (self->completed && self->read_buffer != NULL) {
        assert(PyBytes_CheckExact(self->read_buffer));
        if (transferred != PyBytes_GET_SIZE(self->read_buffer) &&
            _PyBytes_Resize(&self->read_buffer, transferred))
            return NULL;
    }
    return Py_BuildValue("II", (unsigned) transferred, (unsigned) err);
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
    /* tp_name           */ "_winapi.Overlapped",
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

/* -------------------------------------------------------------------- */
/* windows API functions */

PyDoc_STRVAR(CloseHandle_doc,
"CloseHandle(handle) -> None\n\
\n\
Close handle.");

static PyObject *
winapi_CloseHandle(PyObject *self, PyObject *args)
{
    HANDLE hObject;
    BOOL success;

    if (!PyArg_ParseTuple(args, F_HANDLE ":CloseHandle", &hObject))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    success = CloseHandle(hObject);
    Py_END_ALLOW_THREADS

    if (!success)
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}

static PyObject *
winapi_ConnectNamedPipe(PyObject *self, PyObject *args, PyObject *kwds)
{
    HANDLE hNamedPipe;
    int use_overlapped = 0;
    BOOL success;
    OverlappedObject *overlapped = NULL;
    static char *kwlist[] = {"handle", "overlapped", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     F_HANDLE "|" F_BOOL, kwlist,
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
winapi_CreateFile(PyObject *self, PyObject *args)
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
winapi_CreateNamedPipe(PyObject *self, PyObject *args)
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

PyDoc_STRVAR(CreatePipe_doc,
"CreatePipe(pipe_attrs, size) -> (read_handle, write_handle)\n\
\n\
Create an anonymous pipe, and return handles to the read and\n\
write ends of the pipe.\n\
\n\
pipe_attrs is ignored internally and can be None.");

static PyObject *
winapi_CreatePipe(PyObject* self, PyObject* args)
{
    HANDLE read_pipe;
    HANDLE write_pipe;
    BOOL result;

    PyObject* pipe_attributes; /* ignored */
    DWORD size;

    if (! PyArg_ParseTuple(args, "O" F_DWORD ":CreatePipe",
                           &pipe_attributes, &size))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    result = CreatePipe(&read_pipe, &write_pipe, NULL, size);
    Py_END_ALLOW_THREADS

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return Py_BuildValue(
        "NN", HANDLE_TO_PYNUM(read_pipe), HANDLE_TO_PYNUM(write_pipe));
}

/* helpers for createprocess */

static unsigned long
getulong(PyObject* obj, char* name)
{
    PyObject* value;
    unsigned long ret;

    value = PyObject_GetAttrString(obj, name);
    if (! value) {
        PyErr_Clear(); /* FIXME: propagate error? */
        return 0;
    }
    ret = PyLong_AsUnsignedLong(value);
    Py_DECREF(value);
    return ret;
}

static HANDLE
gethandle(PyObject* obj, char* name)
{
    PyObject* value;
    HANDLE ret;

    value = PyObject_GetAttrString(obj, name);
    if (! value) {
        PyErr_Clear(); /* FIXME: propagate error? */
        return NULL;
    }
    if (value == Py_None)
        ret = NULL;
    else
        ret = PYNUM_TO_HANDLE(value);
    Py_DECREF(value);
    return ret;
}

static PyObject*
getenvironment(PyObject* environment)
{
    Py_ssize_t i, envsize, totalsize;
    Py_UCS4 *buffer = NULL, *p, *end;
    PyObject *keys, *values, *res;

    /* convert environment dictionary to windows environment string */
    if (! PyMapping_Check(environment)) {
        PyErr_SetString(
            PyExc_TypeError, "environment must be dictionary or None");
        return NULL;
    }

    envsize = PyMapping_Length(environment);

    keys = PyMapping_Keys(environment);
    values = PyMapping_Values(environment);
    if (!keys || !values)
        goto error;

    totalsize = 1; /* trailing null character */
    for (i = 0; i < envsize; i++) {
        PyObject* key = PyList_GET_ITEM(keys, i);
        PyObject* value = PyList_GET_ITEM(values, i);

        if (! PyUnicode_Check(key) || ! PyUnicode_Check(value)) {
            PyErr_SetString(PyExc_TypeError,
                "environment can only contain strings");
            goto error;
        }
        totalsize += PyUnicode_GET_LENGTH(key) + 1;    /* +1 for '=' */
        totalsize += PyUnicode_GET_LENGTH(value) + 1;  /* +1 for '\0' */
    }

    buffer = PyMem_Malloc(totalsize * sizeof(Py_UCS4));
    if (! buffer)
        goto error;
    p = buffer;
    end = buffer + totalsize;

    for (i = 0; i < envsize; i++) {
        PyObject* key = PyList_GET_ITEM(keys, i);
        PyObject* value = PyList_GET_ITEM(values, i);
        if (!PyUnicode_AsUCS4(key, p, end - p, 0))
            goto error;
        p += PyUnicode_GET_LENGTH(key);
        *p++ = '=';
        if (!PyUnicode_AsUCS4(value, p, end - p, 0))
            goto error;
        p += PyUnicode_GET_LENGTH(value);
        *p++ = '\0';
    }

    /* add trailing null byte */
    *p++ = '\0';
    assert(p == end);

    Py_XDECREF(keys);
    Py_XDECREF(values);

    res = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, buffer, p - buffer);
    PyMem_Free(buffer);
    return res;

 error:
    PyMem_Free(buffer);
    Py_XDECREF(keys);
    Py_XDECREF(values);
    return NULL;
}

PyDoc_STRVAR(CreateProcess_doc,
"CreateProcess(app_name, cmd_line, proc_attrs, thread_attrs,\n\
               inherit, flags, env_mapping, curdir,\n\
               startup_info) -> (proc_handle, thread_handle,\n\
                                 pid, tid)\n\
\n\
Create a new process and its primary thread. The return\n\
value is a tuple of the process handle, thread handle,\n\
process ID, and thread ID.\n\
\n\
proc_attrs and thread_attrs are ignored internally and can be None.");

static PyObject *
winapi_CreateProcess(PyObject* self, PyObject* args)
{
    BOOL result;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    PyObject* environment;
    wchar_t *wenvironment;

    wchar_t* application_name;
    wchar_t* command_line;
    PyObject* process_attributes; /* ignored */
    PyObject* thread_attributes; /* ignored */
    BOOL inherit_handles;
    DWORD creation_flags;
    PyObject* env_mapping;
    wchar_t* current_directory;
    PyObject* startup_info;

    if (! PyArg_ParseTuple(args, "ZZOO" F_BOOL F_DWORD "OZO:CreateProcess",
                           &application_name,
                           &command_line,
                           &process_attributes,
                           &thread_attributes,
                           &inherit_handles,
                           &creation_flags,
                           &env_mapping,
                           &current_directory,
                           &startup_info))
        return NULL;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    /* note: we only support a small subset of all SI attributes */
    si.dwFlags = getulong(startup_info, "dwFlags");
    si.wShowWindow = (WORD)getulong(startup_info, "wShowWindow");
    si.hStdInput = gethandle(startup_info, "hStdInput");
    si.hStdOutput = gethandle(startup_info, "hStdOutput");
    si.hStdError = gethandle(startup_info, "hStdError");
    if (PyErr_Occurred())
        return NULL;

    if (env_mapping != Py_None) {
        environment = getenvironment(env_mapping);
        if (! environment)
            return NULL;
        wenvironment = PyUnicode_AsUnicode(environment);
        if (wenvironment == NULL)
        {
            Py_XDECREF(environment);
            return NULL;
        }
    }
    else {
        environment = NULL;
        wenvironment = NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    result = CreateProcessW(application_name,
                           command_line,
                           NULL,
                           NULL,
                           inherit_handles,
                           creation_flags | CREATE_UNICODE_ENVIRONMENT,
                           wenvironment,
                           current_directory,
                           &si,
                           &pi);
    Py_END_ALLOW_THREADS

    Py_XDECREF(environment);

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return Py_BuildValue("NNkk",
                         HANDLE_TO_PYNUM(pi.hProcess),
                         HANDLE_TO_PYNUM(pi.hThread),
                         pi.dwProcessId,
                         pi.dwThreadId);
}

PyDoc_STRVAR(DuplicateHandle_doc,
"DuplicateHandle(source_proc_handle, source_handle,\n\
                 target_proc_handle, target_handle, access,\n\
                 inherit[, options]) -> handle\n\
\n\
Return a duplicate handle object.\n\
\n\
The duplicate handle refers to the same object as the original\n\
handle. Therefore, any changes to the object are reflected\n\
through both handles.");

static PyObject *
winapi_DuplicateHandle(PyObject* self, PyObject* args)
{
    HANDLE target_handle;
    BOOL result;

    HANDLE source_process_handle;
    HANDLE source_handle;
    HANDLE target_process_handle;
    DWORD desired_access;
    BOOL inherit_handle;
    DWORD options = 0;

    if (! PyArg_ParseTuple(args,
                           F_HANDLE F_HANDLE F_HANDLE F_DWORD F_BOOL F_DWORD
                           ":DuplicateHandle",
                           &source_process_handle,
                           &source_handle,
                           &target_process_handle,
                           &desired_access,
                           &inherit_handle,
                           &options))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    result = DuplicateHandle(
        source_process_handle,
        source_handle,
        target_process_handle,
        &target_handle,
        desired_access,
        inherit_handle,
        options
    );
    Py_END_ALLOW_THREADS

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return HANDLE_TO_PYNUM(target_handle);
}

static PyObject *
winapi_ExitProcess(PyObject *self, PyObject *args)
{
    UINT uExitCode;

    if (!PyArg_ParseTuple(args, F_UINT, &uExitCode))
        return NULL;

    #if defined(Py_DEBUG)
        SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT|
                     SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    #endif

    ExitProcess(uExitCode);

    return NULL;
}

PyDoc_STRVAR(GetCurrentProcess_doc,
"GetCurrentProcess() -> handle\n\
\n\
Return a handle object for the current process.");

static PyObject *
winapi_GetCurrentProcess(PyObject* self, PyObject* args)
{
    if (! PyArg_ParseTuple(args, ":GetCurrentProcess"))
        return NULL;

    return HANDLE_TO_PYNUM(GetCurrentProcess());
}

PyDoc_STRVAR(GetExitCodeProcess_doc,
"GetExitCodeProcess(handle) -> Exit code\n\
\n\
Return the termination status of the specified process.");

static PyObject *
winapi_GetExitCodeProcess(PyObject* self, PyObject* args)
{
    DWORD exit_code;
    BOOL result;

    HANDLE process;
    if (! PyArg_ParseTuple(args, F_HANDLE ":GetExitCodeProcess", &process))
        return NULL;

    result = GetExitCodeProcess(process, &exit_code);

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return PyLong_FromUnsignedLong(exit_code);
}

static PyObject *
winapi_GetLastError(PyObject *self, PyObject *args)
{
    return Py_BuildValue(F_DWORD, GetLastError());
}

PyDoc_STRVAR(GetModuleFileName_doc,
"GetModuleFileName(module) -> path\n\
\n\
Return the fully-qualified path for the file that contains\n\
the specified module. The module must have been loaded by the\n\
current process.\n\
\n\
The module parameter should be a handle to the loaded module\n\
whose path is being requested. If this parameter is 0, \n\
GetModuleFileName retrieves the path of the executable file\n\
of the current process.");

static PyObject *
winapi_GetModuleFileName(PyObject* self, PyObject* args)
{
    BOOL result;
    HMODULE module;
    WCHAR filename[MAX_PATH];

    if (! PyArg_ParseTuple(args, F_HANDLE ":GetModuleFileName",
                           &module))
        return NULL;

    result = GetModuleFileNameW(module, filename, MAX_PATH);
    filename[MAX_PATH-1] = '\0';

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return PyUnicode_FromWideChar(filename, wcslen(filename));
}

PyDoc_STRVAR(GetStdHandle_doc,
"GetStdHandle(handle) -> integer\n\
\n\
Return a handle to the specified standard device\n\
(STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE).\n\
The integer associated with the handle object is returned.");

static PyObject *
winapi_GetStdHandle(PyObject* self, PyObject* args)
{
    HANDLE handle;
    DWORD std_handle;

    if (! PyArg_ParseTuple(args, F_DWORD ":GetStdHandle", &std_handle))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    handle = GetStdHandle(std_handle);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE)
        return PyErr_SetFromWindowsErr(GetLastError());

    if (! handle) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    /* note: returns integer, not handle object */
    return HANDLE_TO_PYNUM(handle);
}

PyDoc_STRVAR(GetVersion_doc,
"GetVersion() -> version\n\
\n\
Return the version number of the current operating system.");

static PyObject *
winapi_GetVersion(PyObject* self, PyObject* args)
{
    if (! PyArg_ParseTuple(args, ":GetVersion"))
        return NULL;

    return PyLong_FromUnsignedLong(GetVersion());
}

static PyObject *
winapi_OpenProcess(PyObject *self, PyObject *args)
{
    DWORD dwDesiredAccess;
    BOOL bInheritHandle;
    DWORD dwProcessId;
    HANDLE handle;

    if (!PyArg_ParseTuple(args, F_DWORD F_BOOL F_DWORD,
                          &dwDesiredAccess, &bInheritHandle, &dwProcessId))
        return NULL;

    handle = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
    if (handle == NULL)
        return PyErr_SetFromWindowsErr(0);

    return Py_BuildValue(F_HANDLE, handle);
}

static PyObject *
winapi_PeekNamedPipe(PyObject *self, PyObject *args)
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
winapi_ReadFile(PyObject *self, PyObject *args, PyObject *kwds)
{
    HANDLE handle;
    int size;
    DWORD nread;
    PyObject *buf;
    BOOL ret;
    int use_overlapped = 0;
    DWORD err;
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

    err = ret ? 0 : GetLastError();

    if (overlapped) {
        if (!ret) {
            if (err == ERROR_IO_PENDING)
                overlapped->pending = 1;
            else if (err != ERROR_MORE_DATA) {
                Py_DECREF(overlapped);
                return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
            }
        }
        return Py_BuildValue("NI", (PyObject *) overlapped, err);
    }

    if (!ret && err != ERROR_MORE_DATA) {
        Py_DECREF(buf);
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
    }
    if (_PyBytes_Resize(&buf, nread))
        return NULL;
    return Py_BuildValue("NI", buf, err);
}

static PyObject *
winapi_SetNamedPipeHandleState(PyObject *self, PyObject *args)
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

PyDoc_STRVAR(TerminateProcess_doc,
"TerminateProcess(handle, exit_code) -> None\n\
\n\
Terminate the specified process and all of its threads.");

static PyObject *
winapi_TerminateProcess(PyObject* self, PyObject* args)
{
    BOOL result;

    HANDLE process;
    UINT exit_code;
    if (! PyArg_ParseTuple(args, F_HANDLE F_UINT ":TerminateProcess",
                           &process, &exit_code))
        return NULL;

    result = TerminateProcess(process, exit_code);

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
winapi_WaitNamedPipe(PyObject *self, PyObject *args)
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
winapi_WaitForMultipleObjects(PyObject* self, PyObject* args)
{
    DWORD result;
    PyObject *handle_seq;
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    HANDLE sigint_event = NULL;
    Py_ssize_t nhandles, i;
    BOOL wait_flag;
    DWORD milliseconds = INFINITE;

    if (!PyArg_ParseTuple(args, "O" F_BOOL "|" F_DWORD
                          ":WaitForMultipleObjects",
                          &handle_seq, &wait_flag, &milliseconds))
        return NULL;

    if (!PySequence_Check(handle_seq)) {
        PyErr_Format(PyExc_TypeError,
                     "sequence type expected, got '%s'",
                     Py_TYPE(handle_seq)->tp_name);
        return NULL;
    }
    nhandles = PySequence_Length(handle_seq);
    if (nhandles == -1)
        return NULL;
    if (nhandles < 0 || nhandles >= MAXIMUM_WAIT_OBJECTS - 1) {
        PyErr_Format(PyExc_ValueError,
                     "need at most %zd handles, got a sequence of length %zd",
                     MAXIMUM_WAIT_OBJECTS - 1, nhandles);
        return NULL;
    }
    for (i = 0; i < nhandles; i++) {
        HANDLE h;
        PyObject *v = PySequence_GetItem(handle_seq, i);
        if (v == NULL)
            return NULL;
        if (!PyArg_Parse(v, F_HANDLE, &h)) {
            Py_DECREF(v);
            return NULL;
        }
        handles[i] = h;
        Py_DECREF(v);
    }
    /* If this is the main thread then make the wait interruptible
       by Ctrl-C unless we are waiting for *all* handles */
    if (!wait_flag && _PyOS_IsMainThread()) {
        sigint_event = _PyOS_SigintEvent();
        assert(sigint_event != NULL);
        handles[nhandles++] = sigint_event;
    }

    Py_BEGIN_ALLOW_THREADS
    if (sigint_event != NULL)
        ResetEvent(sigint_event);
    result = WaitForMultipleObjects((DWORD) nhandles, handles,
                                    wait_flag, milliseconds);
    Py_END_ALLOW_THREADS

    if (result == WAIT_FAILED)
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
    else if (sigint_event != NULL && result == WAIT_OBJECT_0 + nhandles - 1) {
        errno = EINTR;
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    return PyLong_FromLong((int) result);
}

PyDoc_STRVAR(WaitForSingleObject_doc,
"WaitForSingleObject(handle, timeout) -> result\n\
\n\
Wait until the specified object is in the signaled state or\n\
the time-out interval elapses. The timeout value is specified\n\
in milliseconds.");

static PyObject *
winapi_WaitForSingleObject(PyObject* self, PyObject* args)
{
    DWORD result;

    HANDLE handle;
    DWORD milliseconds;
    if (! PyArg_ParseTuple(args, F_HANDLE F_DWORD ":WaitForSingleObject",
                                 &handle,
                                 &milliseconds))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    result = WaitForSingleObject(handle, milliseconds);
    Py_END_ALLOW_THREADS

    if (result == WAIT_FAILED)
        return PyErr_SetFromWindowsErr(GetLastError());

    return PyLong_FromUnsignedLong(result);
}

static PyObject *
winapi_WriteFile(PyObject *self, PyObject *args, PyObject *kwds)
{
    HANDLE handle;
    Py_buffer _buf, *buf;
    PyObject *bufobj;
    DWORD len, written;
    BOOL ret;
    int use_overlapped = 0;
    DWORD err;
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
    len = (DWORD)Py_MIN(buf->len, DWORD_MAX);
    ret = WriteFile(handle, buf->buf, len, &written,
                    overlapped ? &overlapped->overlapped : NULL);
    Py_END_ALLOW_THREADS

    err = ret ? 0 : GetLastError();

    if (overlapped) {
        if (!ret) {
            if (err == ERROR_IO_PENDING)
                overlapped->pending = 1;
            else {
                Py_DECREF(overlapped);
                return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
            }
        }
        return Py_BuildValue("NI", (PyObject *) overlapped, err);
    }

    PyBuffer_Release(buf);
    if (!ret)
        return PyErr_SetExcFromWindowsErr(PyExc_IOError, 0);
    return Py_BuildValue("II", written, err);
}


static PyMethodDef winapi_functions[] = {
    {"CloseHandle", winapi_CloseHandle, METH_VARARGS,
     CloseHandle_doc},
    {"ConnectNamedPipe", (PyCFunction)winapi_ConnectNamedPipe,
     METH_VARARGS | METH_KEYWORDS, ""},
    {"CreateFile", winapi_CreateFile, METH_VARARGS,
     ""},
    {"CreateNamedPipe", winapi_CreateNamedPipe, METH_VARARGS,
     ""},
    {"CreatePipe", winapi_CreatePipe, METH_VARARGS,
     CreatePipe_doc},
    {"CreateProcess", winapi_CreateProcess, METH_VARARGS,
     CreateProcess_doc},
    {"DuplicateHandle", winapi_DuplicateHandle, METH_VARARGS,
     DuplicateHandle_doc},
    {"ExitProcess", winapi_ExitProcess, METH_VARARGS,
     ""},
    {"GetCurrentProcess", winapi_GetCurrentProcess, METH_VARARGS,
     GetCurrentProcess_doc},
    {"GetExitCodeProcess", winapi_GetExitCodeProcess, METH_VARARGS,
     GetExitCodeProcess_doc},
    {"GetLastError", winapi_GetLastError, METH_NOARGS,
     GetCurrentProcess_doc},
    {"GetModuleFileName", winapi_GetModuleFileName, METH_VARARGS,
     GetModuleFileName_doc},
    {"GetStdHandle", winapi_GetStdHandle, METH_VARARGS,
     GetStdHandle_doc},
    {"GetVersion", winapi_GetVersion, METH_VARARGS,
     GetVersion_doc},
    {"OpenProcess", winapi_OpenProcess, METH_VARARGS,
     ""},
    {"PeekNamedPipe", winapi_PeekNamedPipe, METH_VARARGS,
     ""},
    {"ReadFile", (PyCFunction)winapi_ReadFile, METH_VARARGS | METH_KEYWORDS,
     ""},
    {"SetNamedPipeHandleState", winapi_SetNamedPipeHandleState, METH_VARARGS,
     ""},
    {"TerminateProcess", winapi_TerminateProcess, METH_VARARGS,
     TerminateProcess_doc},
    {"WaitNamedPipe", winapi_WaitNamedPipe, METH_VARARGS,
     ""},
    {"WaitForMultipleObjects", winapi_WaitForMultipleObjects, METH_VARARGS,
     ""},
    {"WaitForSingleObject", winapi_WaitForSingleObject, METH_VARARGS,
     WaitForSingleObject_doc},
    {"WriteFile", (PyCFunction)winapi_WriteFile, METH_VARARGS | METH_KEYWORDS,
     ""},
    {NULL, NULL}
};

static struct PyModuleDef winapi_module = {
    PyModuleDef_HEAD_INIT,
    "_winapi",
    NULL,
    -1,
    winapi_functions,
    NULL,
    NULL,
    NULL,
    NULL
};

#define WINAPI_CONSTANT(fmt, con) \
    PyDict_SetItemString(d, #con, Py_BuildValue(fmt, con))

PyMODINIT_FUNC
PyInit__winapi(void)
{
    PyObject *d;
    PyObject *m;

    if (PyType_Ready(&OverlappedType) < 0)
        return NULL;

    m = PyModule_Create(&winapi_module);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);

    PyDict_SetItemString(d, "Overlapped", (PyObject *) &OverlappedType);

    /* constants */
    WINAPI_CONSTANT(F_DWORD, CREATE_NEW_CONSOLE);
    WINAPI_CONSTANT(F_DWORD, CREATE_NEW_PROCESS_GROUP);
    WINAPI_CONSTANT(F_DWORD, DUPLICATE_SAME_ACCESS);
    WINAPI_CONSTANT(F_DWORD, DUPLICATE_CLOSE_SOURCE);
    WINAPI_CONSTANT(F_DWORD, ERROR_ALREADY_EXISTS);
    WINAPI_CONSTANT(F_DWORD, ERROR_BROKEN_PIPE);
    WINAPI_CONSTANT(F_DWORD, ERROR_IO_PENDING);
    WINAPI_CONSTANT(F_DWORD, ERROR_MORE_DATA);
    WINAPI_CONSTANT(F_DWORD, ERROR_NETNAME_DELETED);
    WINAPI_CONSTANT(F_DWORD, ERROR_NO_SYSTEM_RESOURCES);
    WINAPI_CONSTANT(F_DWORD, ERROR_MORE_DATA);
    WINAPI_CONSTANT(F_DWORD, ERROR_NETNAME_DELETED);
    WINAPI_CONSTANT(F_DWORD, ERROR_NO_DATA);
    WINAPI_CONSTANT(F_DWORD, ERROR_NO_SYSTEM_RESOURCES);
    WINAPI_CONSTANT(F_DWORD, ERROR_OPERATION_ABORTED);
    WINAPI_CONSTANT(F_DWORD, ERROR_PIPE_BUSY);
    WINAPI_CONSTANT(F_DWORD, ERROR_PIPE_CONNECTED);
    WINAPI_CONSTANT(F_DWORD, ERROR_SEM_TIMEOUT);
    WINAPI_CONSTANT(F_DWORD, FILE_FLAG_FIRST_PIPE_INSTANCE);
    WINAPI_CONSTANT(F_DWORD, FILE_FLAG_OVERLAPPED);
    WINAPI_CONSTANT(F_DWORD, FILE_GENERIC_READ);
    WINAPI_CONSTANT(F_DWORD, FILE_GENERIC_WRITE);
    WINAPI_CONSTANT(F_DWORD, GENERIC_READ);
    WINAPI_CONSTANT(F_DWORD, GENERIC_WRITE);
    WINAPI_CONSTANT(F_DWORD, INFINITE);
    WINAPI_CONSTANT(F_DWORD, NMPWAIT_WAIT_FOREVER);
    WINAPI_CONSTANT(F_DWORD, OPEN_EXISTING);
    WINAPI_CONSTANT(F_DWORD, PIPE_ACCESS_DUPLEX);
    WINAPI_CONSTANT(F_DWORD, PIPE_ACCESS_INBOUND);
    WINAPI_CONSTANT(F_DWORD, PIPE_READMODE_MESSAGE);
    WINAPI_CONSTANT(F_DWORD, PIPE_TYPE_MESSAGE);
    WINAPI_CONSTANT(F_DWORD, PIPE_UNLIMITED_INSTANCES);
    WINAPI_CONSTANT(F_DWORD, PIPE_WAIT);
    WINAPI_CONSTANT(F_DWORD, PROCESS_ALL_ACCESS);
    WINAPI_CONSTANT(F_DWORD, PROCESS_DUP_HANDLE);
    WINAPI_CONSTANT(F_DWORD, STARTF_USESHOWWINDOW);
    WINAPI_CONSTANT(F_DWORD, STARTF_USESTDHANDLES);
    WINAPI_CONSTANT(F_DWORD, STD_INPUT_HANDLE);
    WINAPI_CONSTANT(F_DWORD, STD_OUTPUT_HANDLE);
    WINAPI_CONSTANT(F_DWORD, STD_ERROR_HANDLE);
    WINAPI_CONSTANT(F_DWORD, STILL_ACTIVE);
    WINAPI_CONSTANT(F_DWORD, SW_HIDE);
    WINAPI_CONSTANT(F_DWORD, WAIT_OBJECT_0);
    WINAPI_CONSTANT(F_DWORD, WAIT_ABANDONED_0);
    WINAPI_CONSTANT(F_DWORD, WAIT_TIMEOUT);

    WINAPI_CONSTANT("i", NULL);

    return m;
}
