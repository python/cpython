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
#include "winreparse.h"

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

#define T_HANDLE T_POINTER

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
        else if (_Py_IsFinalizing())
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

/*[clinic input]
module _winapi
class _winapi.Overlapped "OverlappedObject *" "&OverlappedType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c13d3f5fd1dabb84]*/

/*[python input]
def create_converter(type_, format_unit):
    name = type_ + '_converter'
    # registered upon creation by CConverter's metaclass
    type(name, (CConverter,), {'type': type_, 'format_unit': format_unit})

# format unit differs between platforms for these
create_converter('HANDLE', '" F_HANDLE "')
create_converter('HMODULE', '" F_HANDLE "')
create_converter('LPSECURITY_ATTRIBUTES', '" F_POINTER "')

create_converter('BOOL', 'i') # F_BOOL used previously (always 'i')
create_converter('DWORD', 'k') # F_DWORD is always "k" (which is much shorter)
create_converter('LPCTSTR', 's')
create_converter('LPWSTR', 'u')
create_converter('UINT', 'I') # F_UINT used previously (always 'I')

class HANDLE_return_converter(CReturnConverter):
    type = 'HANDLE'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if("_return_value == INVALID_HANDLE_VALUE", data)
        data.return_conversion.append(
            'if (_return_value == NULL) {\n    Py_RETURN_NONE;\n}\n')
        data.return_conversion.append(
            'return_value = HANDLE_TO_PYNUM(_return_value);\n')

class DWORD_return_converter(CReturnConverter):
    type = 'DWORD'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if("_return_value == PY_DWORD_MAX", data)
        data.return_conversion.append(
            'return_value = Py_BuildValue("k", _return_value);\n')
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=4527052fe06e5823]*/

#include "clinic/_winapi.c.h"

/*[clinic input]
_winapi.Overlapped.GetOverlappedResult

    wait: bool
    /
[clinic start generated code]*/

static PyObject *
_winapi_Overlapped_GetOverlappedResult_impl(OverlappedObject *self, int wait)
/*[clinic end generated code: output=bdd0c1ed6518cd03 input=194505ee8e0e3565]*/
{
    BOOL res;
    DWORD transferred = 0;
    DWORD err;

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
            return PyErr_SetExcFromWindowsErr(PyExc_OSError, err);
    }
    if (self->completed && self->read_buffer != NULL) {
        assert(PyBytes_CheckExact(self->read_buffer));
        if (transferred != PyBytes_GET_SIZE(self->read_buffer) &&
            _PyBytes_Resize(&self->read_buffer, transferred))
            return NULL;
    }
    return Py_BuildValue("II", (unsigned) transferred, (unsigned) err);
}

/*[clinic input]
_winapi.Overlapped.getbuffer
[clinic start generated code]*/

static PyObject *
_winapi_Overlapped_getbuffer_impl(OverlappedObject *self)
/*[clinic end generated code: output=95a3eceefae0f748 input=347fcfd56b4ceabd]*/
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

/*[clinic input]
_winapi.Overlapped.cancel
[clinic start generated code]*/

static PyObject *
_winapi_Overlapped_cancel_impl(OverlappedObject *self)
/*[clinic end generated code: output=fcb9ab5df4ebdae5 input=cbf3da142290039f]*/
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
        return PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
    self->pending = 0;
    Py_RETURN_NONE;
}

static PyMethodDef overlapped_methods[] = {
    _WINAPI_OVERLAPPED_GETOVERLAPPEDRESULT_METHODDEF
    _WINAPI_OVERLAPPED_GETBUFFER_METHODDEF
    _WINAPI_OVERLAPPED_CANCEL_METHODDEF
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

/*[clinic input]
_winapi.CloseHandle

    handle: HANDLE
    /

Close handle.
[clinic start generated code]*/

static PyObject *
_winapi_CloseHandle_impl(PyObject *module, HANDLE handle)
/*[clinic end generated code: output=7ad37345f07bd782 input=7f0e4ac36e0352b8]*/
{
    BOOL success;

    Py_BEGIN_ALLOW_THREADS
    success = CloseHandle(handle);
    Py_END_ALLOW_THREADS

    if (!success)
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.ConnectNamedPipe

    handle: HANDLE
    overlapped as use_overlapped: bool(accept={int}) = False
[clinic start generated code]*/

static PyObject *
_winapi_ConnectNamedPipe_impl(PyObject *module, HANDLE handle,
                              int use_overlapped)
/*[clinic end generated code: output=335a0e7086800671 input=34f937c1c86e5e68]*/
{
    BOOL success;
    OverlappedObject *overlapped = NULL;

    if (use_overlapped) {
        overlapped = new_overlapped(handle);
        if (!overlapped)
            return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    success = ConnectNamedPipe(handle,
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

/*[clinic input]
_winapi.CreateFile -> HANDLE

    file_name: LPCTSTR
    desired_access: DWORD
    share_mode: DWORD
    security_attributes: LPSECURITY_ATTRIBUTES
    creation_disposition: DWORD
    flags_and_attributes: DWORD
    template_file: HANDLE
    /
[clinic start generated code]*/

static HANDLE
_winapi_CreateFile_impl(PyObject *module, LPCTSTR file_name,
                        DWORD desired_access, DWORD share_mode,
                        LPSECURITY_ATTRIBUTES security_attributes,
                        DWORD creation_disposition,
                        DWORD flags_and_attributes, HANDLE template_file)
/*[clinic end generated code: output=417ddcebfc5a3d53 input=6423c3e40372dbd5]*/
{
    HANDLE handle;

    Py_BEGIN_ALLOW_THREADS
    handle = CreateFile(file_name, desired_access,
                        share_mode, security_attributes,
                        creation_disposition,
                        flags_and_attributes, template_file);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE)
        PyErr_SetFromWindowsErr(0);

    return handle;
}

/*[clinic input]
_winapi.CreateJunction

    src_path: LPWSTR
    dst_path: LPWSTR
    /
[clinic start generated code]*/

static PyObject *
_winapi_CreateJunction_impl(PyObject *module, LPWSTR src_path,
                            LPWSTR dst_path)
/*[clinic end generated code: output=66b7eb746e1dfa25 input=8cd1f9964b6e3d36]*/
{
    /* Privilege adjustment */
    HANDLE token = NULL;
    TOKEN_PRIVILEGES tp;

    /* Reparse data buffer */
    const USHORT prefix_len = 4;
    USHORT print_len = 0;
    USHORT rdb_size = 0;
    _Py_PREPARSE_DATA_BUFFER rdb = NULL;

    /* Junction point creation */
    HANDLE junction = NULL;
    DWORD ret = 0;

    if (src_path == NULL || dst_path == NULL)
        return PyErr_SetFromWindowsErr(ERROR_INVALID_PARAMETER);

    if (wcsncmp(src_path, L"\\??\\", prefix_len) == 0)
        return PyErr_SetFromWindowsErr(ERROR_INVALID_PARAMETER);

    /* Adjust privileges to allow rewriting directory entry as a
       junction point. */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
        goto cleanup;

    if (!LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &tp.Privileges[0].Luid))
        goto cleanup;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                               NULL, NULL))
        goto cleanup;

    if (GetFileAttributesW(src_path) == INVALID_FILE_ATTRIBUTES)
        goto cleanup;

    /* Store the absolute link target path length in print_len. */
    print_len = (USHORT)GetFullPathNameW(src_path, 0, NULL, NULL);
    if (print_len == 0)
        goto cleanup;

    /* NUL terminator should not be part of print_len. */
    --print_len;

    /* REPARSE_DATA_BUFFER usage is heavily under-documented, especially for
       junction points. Here's what I've learned along the way:
       - A junction point has two components: a print name and a substitute
         name. They both describe the link target, but the substitute name is
         the physical target and the print name is shown in directory listings.
       - The print name must be a native name, prefixed with "\??\".
       - Both names are stored after each other in the same buffer (the
         PathBuffer) and both must be NUL-terminated.
       - There are four members defining their respective offset and length
         inside PathBuffer: SubstituteNameOffset, SubstituteNameLength,
         PrintNameOffset and PrintNameLength.
       - The total size we need to allocate for the REPARSE_DATA_BUFFER, thus,
         is the sum of:
         - the fixed header size (REPARSE_DATA_BUFFER_HEADER_SIZE)
         - the size of the MountPointReparseBuffer member without the PathBuffer
         - the size of the prefix ("\??\") in bytes
         - the size of the print name in bytes
         - the size of the substitute name in bytes
         - the size of two NUL terminators in bytes */
    rdb_size = _Py_REPARSE_DATA_BUFFER_HEADER_SIZE +
        sizeof(rdb->MountPointReparseBuffer) -
        sizeof(rdb->MountPointReparseBuffer.PathBuffer) +
        /* Two +1's for NUL terminators. */
        (prefix_len + print_len + 1 + print_len + 1) * sizeof(WCHAR);
    rdb = (_Py_PREPARSE_DATA_BUFFER)PyMem_RawMalloc(rdb_size);
    if (rdb == NULL)
        goto cleanup;

    memset(rdb, 0, rdb_size);
    rdb->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    rdb->ReparseDataLength = rdb_size - _Py_REPARSE_DATA_BUFFER_HEADER_SIZE;
    rdb->MountPointReparseBuffer.SubstituteNameOffset = 0;
    rdb->MountPointReparseBuffer.SubstituteNameLength =
        (prefix_len + print_len) * sizeof(WCHAR);
    rdb->MountPointReparseBuffer.PrintNameOffset =
        rdb->MountPointReparseBuffer.SubstituteNameLength + sizeof(WCHAR);
    rdb->MountPointReparseBuffer.PrintNameLength = print_len * sizeof(WCHAR);

    /* Store the full native path of link target at the substitute name
       offset (0). */
    wcscpy(rdb->MountPointReparseBuffer.PathBuffer, L"\\??\\");
    if (GetFullPathNameW(src_path, print_len + 1,
                         rdb->MountPointReparseBuffer.PathBuffer + prefix_len,
                         NULL) == 0)
        goto cleanup;

    /* Copy everything but the native prefix to the print name offset. */
    wcscpy(rdb->MountPointReparseBuffer.PathBuffer +
             prefix_len + print_len + 1,
             rdb->MountPointReparseBuffer.PathBuffer + prefix_len);

    /* Create a directory for the junction point. */
    if (!CreateDirectoryW(dst_path, NULL))
        goto cleanup;

    junction = CreateFileW(dst_path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (junction == INVALID_HANDLE_VALUE)
        goto cleanup;

    /* Make the directory entry a junction point. */
    if (!DeviceIoControl(junction, FSCTL_SET_REPARSE_POINT, rdb, rdb_size,
                         NULL, 0, &ret, NULL))
        goto cleanup;

cleanup:
    ret = GetLastError();

    CloseHandle(token);
    CloseHandle(junction);
    PyMem_RawFree(rdb);

    if (ret != 0)
        return PyErr_SetFromWindowsErr(ret);

    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.CreateNamedPipe -> HANDLE

    name: LPCTSTR
    open_mode: DWORD
    pipe_mode: DWORD
    max_instances: DWORD
    out_buffer_size: DWORD
    in_buffer_size: DWORD
    default_timeout: DWORD
    security_attributes: LPSECURITY_ATTRIBUTES
    /
[clinic start generated code]*/

static HANDLE
_winapi_CreateNamedPipe_impl(PyObject *module, LPCTSTR name, DWORD open_mode,
                             DWORD pipe_mode, DWORD max_instances,
                             DWORD out_buffer_size, DWORD in_buffer_size,
                             DWORD default_timeout,
                             LPSECURITY_ATTRIBUTES security_attributes)
/*[clinic end generated code: output=80f8c07346a94fbc input=5a73530b84d8bc37]*/
{
    HANDLE handle;

    Py_BEGIN_ALLOW_THREADS
    handle = CreateNamedPipe(name, open_mode, pipe_mode,
                             max_instances, out_buffer_size,
                             in_buffer_size, default_timeout,
                             security_attributes);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE)
        PyErr_SetFromWindowsErr(0);

    return handle;
}

/*[clinic input]
_winapi.CreatePipe

    pipe_attrs: object
        Ignored internally, can be None.
    size: DWORD
    /

Create an anonymous pipe.

Returns a 2-tuple of handles, to the read and write ends of the pipe.
[clinic start generated code]*/

static PyObject *
_winapi_CreatePipe_impl(PyObject *module, PyObject *pipe_attrs, DWORD size)
/*[clinic end generated code: output=1c4411d8699f0925 input=c4f2cfa56ef68d90]*/
{
    HANDLE read_pipe;
    HANDLE write_pipe;
    BOOL result;

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
getulong(PyObject* obj, const char* name)
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
gethandle(PyObject* obj, const char* name)
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

    keys = PyMapping_Keys(environment);
    if (!keys) {
        return NULL;
    }
    values = PyMapping_Values(environment);
    if (!values) {
        goto error;
    }

    envsize = PySequence_Fast_GET_SIZE(keys);
    if (PySequence_Fast_GET_SIZE(values) != envsize) {
        PyErr_SetString(PyExc_RuntimeError,
            "environment changed size during iteration");
        goto error;
    }

    totalsize = 1; /* trailing null character */
    for (i = 0; i < envsize; i++) {
        PyObject* key = PySequence_Fast_GET_ITEM(keys, i);
        PyObject* value = PySequence_Fast_GET_ITEM(values, i);

        if (! PyUnicode_Check(key) || ! PyUnicode_Check(value)) {
            PyErr_SetString(PyExc_TypeError,
                "environment can only contain strings");
            goto error;
        }
        if (PyUnicode_FindChar(key, '\0', 0, PyUnicode_GET_LENGTH(key), 1) != -1 ||
            PyUnicode_FindChar(value, '\0', 0, PyUnicode_GET_LENGTH(value), 1) != -1)
        {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto error;
        }
        /* Search from index 1 because on Windows starting '=' is allowed for
           defining hidden environment variables. */
        if (PyUnicode_GET_LENGTH(key) == 0 ||
            PyUnicode_FindChar(key, '=', 1, PyUnicode_GET_LENGTH(key), 1) != -1)
        {
            PyErr_SetString(PyExc_ValueError, "illegal environment variable name");
            goto error;
        }
        if (totalsize > PY_SSIZE_T_MAX - PyUnicode_GET_LENGTH(key) - 1) {
            PyErr_SetString(PyExc_OverflowError, "environment too long");
            goto error;
        }
        totalsize += PyUnicode_GET_LENGTH(key) + 1;    /* +1 for '=' */
        if (totalsize > PY_SSIZE_T_MAX - PyUnicode_GET_LENGTH(value) - 1) {
            PyErr_SetString(PyExc_OverflowError, "environment too long");
            goto error;
        }
        totalsize += PyUnicode_GET_LENGTH(value) + 1;  /* +1 for '\0' */
    }

    buffer = PyMem_NEW(Py_UCS4, totalsize);
    if (! buffer) {
        PyErr_NoMemory();
        goto error;
    }
    p = buffer;
    end = buffer + totalsize;

    for (i = 0; i < envsize; i++) {
        PyObject* key = PySequence_Fast_GET_ITEM(keys, i);
        PyObject* value = PySequence_Fast_GET_ITEM(values, i);
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

static LPHANDLE
gethandlelist(PyObject *mapping, const char *name, Py_ssize_t *size)
{
    LPHANDLE ret = NULL;
    PyObject *value_fast = NULL;
    PyObject *value;
    Py_ssize_t i;

    value = PyMapping_GetItemString(mapping, name);
    if (!value) {
        PyErr_Clear();
        return NULL;
    }

    if (value == Py_None) {
        goto cleanup;
    }

    value_fast = PySequence_Fast(value, "handle_list must be a sequence or None");
    if (value_fast == NULL)
        goto cleanup;

    *size = PySequence_Fast_GET_SIZE(value_fast) * sizeof(HANDLE);

    /* Passing an empty array causes CreateProcess to fail so just don't set it */
    if (*size == 0) {
        goto cleanup;
    }

    ret = PyMem_Malloc(*size);
    if (ret == NULL)
        goto cleanup;

    for (i = 0; i < PySequence_Fast_GET_SIZE(value_fast); i++) {
        ret[i] = PYNUM_TO_HANDLE(PySequence_Fast_GET_ITEM(value_fast, i));
        if (ret[i] == (HANDLE)-1 && PyErr_Occurred()) {
            PyMem_Free(ret);
            ret = NULL;
            goto cleanup;
        }
    }

cleanup:
    Py_DECREF(value);
    Py_XDECREF(value_fast);
    return ret;
}

typedef struct {
    LPPROC_THREAD_ATTRIBUTE_LIST attribute_list;
    LPHANDLE handle_list;
} AttributeList;

static void
freeattributelist(AttributeList *attribute_list)
{
    if (attribute_list->attribute_list != NULL) {
        DeleteProcThreadAttributeList(attribute_list->attribute_list);
        PyMem_Free(attribute_list->attribute_list);
    }

    PyMem_Free(attribute_list->handle_list);

    memset(attribute_list, 0, sizeof(*attribute_list));
}

static int
getattributelist(PyObject *obj, const char *name, AttributeList *attribute_list)
{
    int ret = 0;
    DWORD err;
    BOOL result;
    PyObject *value;
    Py_ssize_t handle_list_size;
    DWORD attribute_count = 0;
    SIZE_T attribute_list_size = 0;

    value = PyObject_GetAttrString(obj, name);
    if (!value) {
        PyErr_Clear(); /* FIXME: propagate error? */
        return 0;
    }

    if (value == Py_None) {
        ret = 0;
        goto cleanup;
    }

    if (!PyMapping_Check(value)) {
        ret = -1;
        PyErr_Format(PyExc_TypeError, "%s must be a mapping or None", name);
        goto cleanup;
    }

    attribute_list->handle_list = gethandlelist(value, "handle_list", &handle_list_size);
    if (attribute_list->handle_list == NULL && PyErr_Occurred()) {
        ret = -1;
        goto cleanup;
    }

    if (attribute_list->handle_list != NULL)
        ++attribute_count;

    /* Get how many bytes we need for the attribute list */
    result = InitializeProcThreadAttributeList(NULL, attribute_count, 0, &attribute_list_size);
    if (result || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        ret = -1;
        PyErr_SetFromWindowsErr(GetLastError());
        goto cleanup;
    }

    attribute_list->attribute_list = PyMem_Malloc(attribute_list_size);
    if (attribute_list->attribute_list == NULL) {
        ret = -1;
        goto cleanup;
    }

    result = InitializeProcThreadAttributeList(
        attribute_list->attribute_list,
        attribute_count,
        0,
        &attribute_list_size);
    if (!result) {
        err = GetLastError();

        /* So that we won't call DeleteProcThreadAttributeList */
        PyMem_Free(attribute_list->attribute_list);
        attribute_list->attribute_list = NULL;

        ret = -1;
        PyErr_SetFromWindowsErr(err);
        goto cleanup;
    }

    if (attribute_list->handle_list != NULL) {
        result = UpdateProcThreadAttribute(
            attribute_list->attribute_list,
            0,
            PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
            attribute_list->handle_list,
            handle_list_size,
            NULL,
            NULL);
        if (!result) {
            ret = -1;
            PyErr_SetFromWindowsErr(GetLastError());
            goto cleanup;
        }
    }

cleanup:
    Py_DECREF(value);

    if (ret < 0)
        freeattributelist(attribute_list);

    return ret;
}

/*[clinic input]
_winapi.CreateProcess

    application_name: Py_UNICODE(accept={str, NoneType})
    command_line: object
        Can be str or None
    proc_attrs: object
        Ignored internally, can be None.
    thread_attrs: object
        Ignored internally, can be None.
    inherit_handles: BOOL
    creation_flags: DWORD
    env_mapping: object
    current_directory: Py_UNICODE(accept={str, NoneType})
    startup_info: object
    /

Create a new process and its primary thread.

The return value is a tuple of the process handle, thread handle,
process ID, and thread ID.
[clinic start generated code]*/

static PyObject *
_winapi_CreateProcess_impl(PyObject *module,
                           const Py_UNICODE *application_name,
                           PyObject *command_line, PyObject *proc_attrs,
                           PyObject *thread_attrs, BOOL inherit_handles,
                           DWORD creation_flags, PyObject *env_mapping,
                           const Py_UNICODE *current_directory,
                           PyObject *startup_info)
/*[clinic end generated code: output=9b2423a609230132 input=42ac293eaea03fc4]*/
{
    PyObject *ret = NULL;
    BOOL result;
    PROCESS_INFORMATION pi;
    STARTUPINFOEXW si;
    PyObject *environment = NULL;
    wchar_t *wenvironment;
    wchar_t *command_line_copy = NULL;
    AttributeList attribute_list = {0};

    ZeroMemory(&si, sizeof(si));
    si.StartupInfo.cb = sizeof(si);

    /* note: we only support a small subset of all SI attributes */
    si.StartupInfo.dwFlags = getulong(startup_info, "dwFlags");
    si.StartupInfo.wShowWindow = (WORD)getulong(startup_info, "wShowWindow");
    si.StartupInfo.hStdInput = gethandle(startup_info, "hStdInput");
    si.StartupInfo.hStdOutput = gethandle(startup_info, "hStdOutput");
    si.StartupInfo.hStdError = gethandle(startup_info, "hStdError");
    if (PyErr_Occurred())
        goto cleanup;

    if (env_mapping != Py_None) {
        environment = getenvironment(env_mapping);
        if (environment == NULL) {
            goto cleanup;
        }
        /* contains embedded null characters */
        wenvironment = PyUnicode_AsUnicode(environment);
        if (wenvironment == NULL) {
            goto cleanup;
        }
    }
    else {
        environment = NULL;
        wenvironment = NULL;
    }

    if (getattributelist(startup_info, "lpAttributeList", &attribute_list) < 0)
        goto cleanup;

    si.lpAttributeList = attribute_list.attribute_list;
    if (PyUnicode_Check(command_line)) {
        command_line_copy = PyUnicode_AsWideCharString(command_line, NULL);
        if (command_line_copy == NULL) {
            goto cleanup;
        }
    }
    else if (command_line != Py_None) {
        PyErr_Format(PyExc_TypeError, 
                     "CreateProcess() argument 2 must be str or None, not %s", 
                     Py_TYPE(command_line)->tp_name);
        goto cleanup;
    }


    Py_BEGIN_ALLOW_THREADS
    result = CreateProcessW(application_name,
                           command_line_copy,
                           NULL,
                           NULL,
                           inherit_handles,
                           creation_flags | EXTENDED_STARTUPINFO_PRESENT |
                           CREATE_UNICODE_ENVIRONMENT,
                           wenvironment,
                           current_directory,
                           (LPSTARTUPINFOW)&si,
                           &pi);
    Py_END_ALLOW_THREADS

    if (!result) {
        PyErr_SetFromWindowsErr(GetLastError());
        goto cleanup;
    }

    ret = Py_BuildValue("NNkk",
                        HANDLE_TO_PYNUM(pi.hProcess),
                        HANDLE_TO_PYNUM(pi.hThread),
                        pi.dwProcessId,
                        pi.dwThreadId);

cleanup:
    PyMem_Free(command_line_copy);
    Py_XDECREF(environment);
    freeattributelist(&attribute_list);

    return ret;
}

/*[clinic input]
_winapi.DuplicateHandle -> HANDLE

    source_process_handle: HANDLE
    source_handle: HANDLE
    target_process_handle: HANDLE
    desired_access: DWORD
    inherit_handle: BOOL
    options: DWORD = 0
    /

Return a duplicate handle object.

The duplicate handle refers to the same object as the original
handle. Therefore, any changes to the object are reflected
through both handles.
[clinic start generated code]*/

static HANDLE
_winapi_DuplicateHandle_impl(PyObject *module, HANDLE source_process_handle,
                             HANDLE source_handle,
                             HANDLE target_process_handle,
                             DWORD desired_access, BOOL inherit_handle,
                             DWORD options)
/*[clinic end generated code: output=ad9711397b5dcd4e input=b933e3f2356a8c12]*/
{
    HANDLE target_handle;
    BOOL result;

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

    if (! result) {
        PyErr_SetFromWindowsErr(GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    return target_handle;
}

/*[clinic input]
_winapi.ExitProcess

    ExitCode: UINT
    /

[clinic start generated code]*/

static PyObject *
_winapi_ExitProcess_impl(PyObject *module, UINT ExitCode)
/*[clinic end generated code: output=a387deb651175301 input=4f05466a9406c558]*/
{
    #if defined(Py_DEBUG)
        SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT|
                     SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    #endif

    ExitProcess(ExitCode);

    return NULL;
}

/*[clinic input]
_winapi.GetCurrentProcess -> HANDLE

Return a handle object for the current process.
[clinic start generated code]*/

static HANDLE
_winapi_GetCurrentProcess_impl(PyObject *module)
/*[clinic end generated code: output=ddeb4dd2ffadf344 input=b213403fd4b96b41]*/
{
    return GetCurrentProcess();
}

/*[clinic input]
_winapi.GetExitCodeProcess -> DWORD

    process: HANDLE
    /

Return the termination status of the specified process.
[clinic start generated code]*/

static DWORD
_winapi_GetExitCodeProcess_impl(PyObject *module, HANDLE process)
/*[clinic end generated code: output=b4620bdf2bccf36b input=61b6bfc7dc2ee374]*/
{
    DWORD exit_code;
    BOOL result;

    result = GetExitCodeProcess(process, &exit_code);

    if (! result) {
        PyErr_SetFromWindowsErr(GetLastError());
        exit_code = PY_DWORD_MAX;
    }

    return exit_code;
}

/*[clinic input]
_winapi.GetLastError -> DWORD
[clinic start generated code]*/

static DWORD
_winapi_GetLastError_impl(PyObject *module)
/*[clinic end generated code: output=8585b827cb1a92c5 input=62d47fb9bce038ba]*/
{
    return GetLastError();
}

/*[clinic input]
_winapi.GetModuleFileName

    module_handle: HMODULE
    /

Return the fully-qualified path for the file that contains module.

The module must have been loaded by the current process.

The module parameter should be a handle to the loaded module
whose path is being requested. If this parameter is 0,
GetModuleFileName retrieves the path of the executable file
of the current process.
[clinic start generated code]*/

static PyObject *
_winapi_GetModuleFileName_impl(PyObject *module, HMODULE module_handle)
/*[clinic end generated code: output=85b4b728c5160306 input=6d66ff7deca5d11f]*/
{
    BOOL result;
    WCHAR filename[MAX_PATH];

    result = GetModuleFileNameW(module_handle, filename, MAX_PATH);
    filename[MAX_PATH-1] = '\0';

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return PyUnicode_FromWideChar(filename, wcslen(filename));
}

/*[clinic input]
_winapi.GetStdHandle -> HANDLE

    std_handle: DWORD
        One of STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, or STD_ERROR_HANDLE.
    /

Return a handle to the specified standard device.

The integer associated with the handle object is returned.
[clinic start generated code]*/

static HANDLE
_winapi_GetStdHandle_impl(PyObject *module, DWORD std_handle)
/*[clinic end generated code: output=0e613001e73ab614 input=07016b06a2fc8826]*/
{
    HANDLE handle;

    Py_BEGIN_ALLOW_THREADS
    handle = GetStdHandle(std_handle);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE)
        PyErr_SetFromWindowsErr(GetLastError());

    return handle;
}

/*[clinic input]
_winapi.GetVersion -> long

Return the version number of the current operating system.
[clinic start generated code]*/

static long
_winapi_GetVersion_impl(PyObject *module)
/*[clinic end generated code: output=e41f0db5a3b82682 input=e21dff8d0baeded2]*/
/* Disable deprecation warnings about GetVersionEx as the result is
   being passed straight through to the caller, who is responsible for
   using it correctly. */
#pragma warning(push)
#pragma warning(disable:4996)

{
    return GetVersion();
}

#pragma warning(pop)

/*[clinic input]
_winapi.OpenProcess -> HANDLE

    desired_access: DWORD
    inherit_handle: BOOL
    process_id: DWORD
    /
[clinic start generated code]*/

static HANDLE
_winapi_OpenProcess_impl(PyObject *module, DWORD desired_access,
                         BOOL inherit_handle, DWORD process_id)
/*[clinic end generated code: output=b42b6b81ea5a0fc3 input=ec98c4cf4ea2ec36]*/
{
    HANDLE handle;

    handle = OpenProcess(desired_access, inherit_handle, process_id);
    if (handle == NULL) {
        PyErr_SetFromWindowsErr(0);
        handle = INVALID_HANDLE_VALUE;
    }

    return handle;
}

/*[clinic input]
_winapi.PeekNamedPipe

    handle: HANDLE
    size: int = 0
    /
[clinic start generated code]*/

static PyObject *
_winapi_PeekNamedPipe_impl(PyObject *module, HANDLE handle, int size)
/*[clinic end generated code: output=d0c3e29e49d323dd input=c7aa53bfbce69d70]*/
{
    PyObject *buf = NULL;
    DWORD nread, navail, nleft;
    BOOL ret;

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
            return PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
        }
        if (_PyBytes_Resize(&buf, nread))
            return NULL;
        return Py_BuildValue("NII", buf, navail, nleft);
    }
    else {
        Py_BEGIN_ALLOW_THREADS
        ret = PeekNamedPipe(handle, NULL, 0, NULL, &navail, &nleft);
        Py_END_ALLOW_THREADS
        if (!ret) {
            return PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
        }
        return Py_BuildValue("II", navail, nleft);
    }
}

/*[clinic input]
_winapi.ReadFile

    handle: HANDLE
    size: DWORD
    overlapped as use_overlapped: bool(accept={int}) = False
[clinic start generated code]*/

static PyObject *
_winapi_ReadFile_impl(PyObject *module, HANDLE handle, DWORD size,
                      int use_overlapped)
/*[clinic end generated code: output=d3d5b44a8201b944 input=08c439d03a11aac5]*/
{
    DWORD nread;
    PyObject *buf;
    BOOL ret;
    DWORD err;
    OverlappedObject *overlapped = NULL;

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
                return PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
            }
        }
        return Py_BuildValue("NI", (PyObject *) overlapped, err);
    }

    if (!ret && err != ERROR_MORE_DATA) {
        Py_DECREF(buf);
        return PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
    }
    if (_PyBytes_Resize(&buf, nread))
        return NULL;
    return Py_BuildValue("NI", buf, err);
}

/*[clinic input]
_winapi.SetNamedPipeHandleState

    named_pipe: HANDLE
    mode: object
    max_collection_count: object
    collect_data_timeout: object
    /
[clinic start generated code]*/

static PyObject *
_winapi_SetNamedPipeHandleState_impl(PyObject *module, HANDLE named_pipe,
                                     PyObject *mode,
                                     PyObject *max_collection_count,
                                     PyObject *collect_data_timeout)
/*[clinic end generated code: output=f2129d222cbfa095 input=9142d72163d0faa6]*/
{
    PyObject *oArgs[3] = {mode, max_collection_count, collect_data_timeout};
    DWORD dwArgs[3], *pArgs[3] = {NULL, NULL, NULL};
    int i;

    PyErr_Clear();

    for (i = 0 ; i < 3 ; i++) {
        if (oArgs[i] != Py_None) {
            dwArgs[i] = PyLong_AsUnsignedLongMask(oArgs[i]);
            if (PyErr_Occurred())
                return NULL;
            pArgs[i] = &dwArgs[i];
        }
    }

    if (!SetNamedPipeHandleState(named_pipe, pArgs[0], pArgs[1], pArgs[2]))
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}


/*[clinic input]
_winapi.TerminateProcess

    handle: HANDLE
    exit_code: UINT
    /

Terminate the specified process and all of its threads.
[clinic start generated code]*/

static PyObject *
_winapi_TerminateProcess_impl(PyObject *module, HANDLE handle,
                              UINT exit_code)
/*[clinic end generated code: output=f4e99ac3f0b1f34a input=d6bc0aa1ee3bb4df]*/
{
    BOOL result;

    result = TerminateProcess(handle, exit_code);

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.WaitNamedPipe

    name: LPCTSTR
    timeout: DWORD
    /
[clinic start generated code]*/

static PyObject *
_winapi_WaitNamedPipe_impl(PyObject *module, LPCTSTR name, DWORD timeout)
/*[clinic end generated code: output=c2866f4439b1fe38 input=36fc781291b1862c]*/
{
    BOOL success;

    Py_BEGIN_ALLOW_THREADS
    success = WaitNamedPipe(name, timeout);
    Py_END_ALLOW_THREADS

    if (!success)
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.WaitForMultipleObjects

    handle_seq: object
    wait_flag: BOOL
    milliseconds: DWORD(c_default='INFINITE') = _winapi.INFINITE
    /
[clinic start generated code]*/

static PyObject *
_winapi_WaitForMultipleObjects_impl(PyObject *module, PyObject *handle_seq,
                                    BOOL wait_flag, DWORD milliseconds)
/*[clinic end generated code: output=295e3f00b8e45899 input=36f76ca057cd28a0]*/
{
    DWORD result;
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    HANDLE sigint_event = NULL;
    Py_ssize_t nhandles, i;

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
        return PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
    else if (sigint_event != NULL && result == WAIT_OBJECT_0 + nhandles - 1) {
        errno = EINTR;
        return PyErr_SetFromErrno(PyExc_OSError);
    }

    return PyLong_FromLong((int) result);
}

/*[clinic input]
_winapi.WaitForSingleObject -> long

    handle: HANDLE
    milliseconds: DWORD
    /

Wait for a single object.

Wait until the specified object is in the signaled state or
the time-out interval elapses. The timeout value is specified
in milliseconds.
[clinic start generated code]*/

static long
_winapi_WaitForSingleObject_impl(PyObject *module, HANDLE handle,
                                 DWORD milliseconds)
/*[clinic end generated code: output=3c4715d8f1b39859 input=443d1ab076edc7b1]*/
{
    DWORD result;

    Py_BEGIN_ALLOW_THREADS
    result = WaitForSingleObject(handle, milliseconds);
    Py_END_ALLOW_THREADS

    if (result == WAIT_FAILED) {
        PyErr_SetFromWindowsErr(GetLastError());
        return -1;
    }

    return result;
}

/*[clinic input]
_winapi.WriteFile

    handle: HANDLE
    buffer: object
    overlapped as use_overlapped: bool(accept={int}) = False
[clinic start generated code]*/

static PyObject *
_winapi_WriteFile_impl(PyObject *module, HANDLE handle, PyObject *buffer,
                       int use_overlapped)
/*[clinic end generated code: output=2ca80f6bf3fa92e3 input=11eae2a03aa32731]*/
{
    Py_buffer _buf, *buf;
    DWORD len, written;
    BOOL ret;
    DWORD err;
    OverlappedObject *overlapped = NULL;

    if (use_overlapped) {
        overlapped = new_overlapped(handle);
        if (!overlapped)
            return NULL;
        buf = &overlapped->write_buffer;
    }
    else
        buf = &_buf;

    if (!PyArg_Parse(buffer, "y*", buf)) {
        Py_XDECREF(overlapped);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    len = (DWORD)Py_MIN(buf->len, PY_DWORD_MAX);
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
                return PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
            }
        }
        return Py_BuildValue("NI", (PyObject *) overlapped, err);
    }

    PyBuffer_Release(buf);
    if (!ret)
        return PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
    return Py_BuildValue("II", written, err);
}

/*[clinic input]
_winapi.GetACP

Get the current Windows ANSI code page identifier.
[clinic start generated code]*/

static PyObject *
_winapi_GetACP_impl(PyObject *module)
/*[clinic end generated code: output=f7ee24bf705dbb88 input=1433c96d03a05229]*/
{
    return PyLong_FromUnsignedLong(GetACP());
}

/*[clinic input]
_winapi.GetFileType -> DWORD

    handle: HANDLE
[clinic start generated code]*/

static DWORD
_winapi_GetFileType_impl(PyObject *module, HANDLE handle)
/*[clinic end generated code: output=92b8466ac76ecc17 input=0058366bc40bbfbf]*/
{
    DWORD result;

    Py_BEGIN_ALLOW_THREADS
    result = GetFileType(handle);
    Py_END_ALLOW_THREADS

    if (result == FILE_TYPE_UNKNOWN && GetLastError() != NO_ERROR) {
        PyErr_SetFromWindowsErr(0);
        return -1;
    }

    return result;
}


static PyMethodDef winapi_functions[] = {
    _WINAPI_CLOSEHANDLE_METHODDEF
    _WINAPI_CONNECTNAMEDPIPE_METHODDEF
    _WINAPI_CREATEFILE_METHODDEF
    _WINAPI_CREATENAMEDPIPE_METHODDEF
    _WINAPI_CREATEPIPE_METHODDEF
    _WINAPI_CREATEPROCESS_METHODDEF
    _WINAPI_CREATEJUNCTION_METHODDEF
    _WINAPI_DUPLICATEHANDLE_METHODDEF
    _WINAPI_EXITPROCESS_METHODDEF
    _WINAPI_GETCURRENTPROCESS_METHODDEF
    _WINAPI_GETEXITCODEPROCESS_METHODDEF
    _WINAPI_GETLASTERROR_METHODDEF
    _WINAPI_GETMODULEFILENAME_METHODDEF
    _WINAPI_GETSTDHANDLE_METHODDEF
    _WINAPI_GETVERSION_METHODDEF
    _WINAPI_OPENPROCESS_METHODDEF
    _WINAPI_PEEKNAMEDPIPE_METHODDEF
    _WINAPI_READFILE_METHODDEF
    _WINAPI_SETNAMEDPIPEHANDLESTATE_METHODDEF
    _WINAPI_TERMINATEPROCESS_METHODDEF
    _WINAPI_WAITNAMEDPIPE_METHODDEF
    _WINAPI_WAITFORMULTIPLEOBJECTS_METHODDEF
    _WINAPI_WAITFORSINGLEOBJECT_METHODDEF
    _WINAPI_WRITEFILE_METHODDEF
    _WINAPI_GETACP_METHODDEF
    _WINAPI_GETFILETYPE_METHODDEF
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

    WINAPI_CONSTANT(F_DWORD, ABOVE_NORMAL_PRIORITY_CLASS);
    WINAPI_CONSTANT(F_DWORD, BELOW_NORMAL_PRIORITY_CLASS);
    WINAPI_CONSTANT(F_DWORD, HIGH_PRIORITY_CLASS);
    WINAPI_CONSTANT(F_DWORD, IDLE_PRIORITY_CLASS);
    WINAPI_CONSTANT(F_DWORD, NORMAL_PRIORITY_CLASS);
    WINAPI_CONSTANT(F_DWORD, REALTIME_PRIORITY_CLASS);

    WINAPI_CONSTANT(F_DWORD, CREATE_NO_WINDOW);
    WINAPI_CONSTANT(F_DWORD, DETACHED_PROCESS);
    WINAPI_CONSTANT(F_DWORD, CREATE_DEFAULT_ERROR_MODE);
    WINAPI_CONSTANT(F_DWORD, CREATE_BREAKAWAY_FROM_JOB);

    WINAPI_CONSTANT(F_DWORD, FILE_TYPE_UNKNOWN);
    WINAPI_CONSTANT(F_DWORD, FILE_TYPE_DISK);
    WINAPI_CONSTANT(F_DWORD, FILE_TYPE_CHAR);
    WINAPI_CONSTANT(F_DWORD, FILE_TYPE_PIPE);
    WINAPI_CONSTANT(F_DWORD, FILE_TYPE_REMOTE);

    WINAPI_CONSTANT("i", NULL);

    return m;
}
