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
/* See https://www.python.org/2.4/license for licensing details. */

#include "Python.h"
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_pylifecycle.h"   // _Py_IsInterpreterFinalizing()
#include "pycore_pystate.h"       // _PyInterpreterState_GET



#ifndef WINDOWS_LEAN_AND_MEAN
#define WINDOWS_LEAN_AND_MEAN
#endif
#include "windows.h"
#include <winioctl.h>
#include <crtdbg.h>
#include "winreparse.h"

#if defined(MS_WIN32) && !defined(MS_WIN64)
#define HANDLE_TO_PYNUM(handle) \
    PyLong_FromUnsignedLong((unsigned long) handle)
#define PYNUM_TO_HANDLE(obj) ((HANDLE)PyLong_AsUnsignedLong(obj))
#define F_POINTER "k"
#define T_POINTER Py_T_ULONG
#else
#define HANDLE_TO_PYNUM(handle) \
    PyLong_FromUnsignedLongLong((unsigned long long) handle)
#define PYNUM_TO_HANDLE(obj) ((HANDLE)PyLong_AsUnsignedLongLong(obj))
#define F_POINTER "K"
#define T_POINTER Py_T_ULONGLONG
#endif

#define F_HANDLE F_POINTER
#define F_DWORD "k"

#define T_HANDLE T_POINTER

// winbase.h limits the STARTF_* flags to the desktop API as of 10.0.19041.
#ifndef STARTF_USESHOWWINDOW
#define STARTF_USESHOWWINDOW 0x00000001
#endif
#ifndef STARTF_USESIZE
#define STARTF_USESIZE 0x00000002
#endif
#ifndef STARTF_USEPOSITION
#define STARTF_USEPOSITION 0x00000004
#endif
#ifndef STARTF_USECOUNTCHARS
#define STARTF_USECOUNTCHARS 0x00000008
#endif
#ifndef STARTF_USEFILLATTRIBUTE
#define STARTF_USEFILLATTRIBUTE 0x00000010
#endif
#ifndef STARTF_RUNFULLSCREEN
#define STARTF_RUNFULLSCREEN 0x00000020
#endif
#ifndef STARTF_FORCEONFEEDBACK
#define STARTF_FORCEONFEEDBACK 0x00000040
#endif
#ifndef STARTF_FORCEOFFFEEDBACK
#define STARTF_FORCEOFFFEEDBACK 0x00000080
#endif
#ifndef STARTF_USESTDHANDLES
#define STARTF_USESTDHANDLES 0x00000100
#endif
#ifndef STARTF_USEHOTKEY
#define STARTF_USEHOTKEY 0x00000200
#endif
#ifndef STARTF_TITLEISLINKNAME
#define STARTF_TITLEISLINKNAME 0x00000800
#endif
#ifndef STARTF_TITLEISAPPID
#define STARTF_TITLEISAPPID 0x00001000
#endif
#ifndef STARTF_PREVENTPINNING
#define STARTF_PREVENTPINNING 0x00002000
#endif
#ifndef STARTF_UNTRUSTEDSOURCE
#define STARTF_UNTRUSTEDSOURCE 0x00008000
#endif

typedef struct {
    PyTypeObject *overlapped_type;
} WinApiState;

static inline WinApiState*
winapi_get_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (WinApiState *)state;
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

/*
Note: tp_clear (overlapped_clear) is not implemented because it
requires cancelling the IO operation if it's pending and the cancellation is
quite complex and can fail (see: overlapped_dealloc).
*/
static int
overlapped_traverse(OverlappedObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->read_buffer);
    Py_VISIT(self->write_buffer.obj);
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static void
overlapped_dealloc(OverlappedObject *self)
{
    DWORD bytes;
    int err = GetLastError();

    PyObject_GC_UnTrack(self);
    if (self->pending) {
        if (CancelIoEx(self->handle, &self->overlapped) &&
            GetOverlappedResult(self->handle, &self->overlapped, &bytes, TRUE))
        {
            /* The operation is no longer pending -- nothing to do. */
        }
        else if (_Py_IsInterpreterFinalizing(_PyInterpreterState_GET()))
        {
            /* The operation is still pending -- give a warning.  This
               will probably only happen on Windows XP. */
            PyErr_SetString(PyExc_PythonFinalizationError,
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
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
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
create_converter('LPCVOID', '" F_POINTER "')

create_converter('BOOL', 'i') # F_BOOL used previously (always 'i')
create_converter('DWORD', 'k') # F_DWORD is always "k" (which is much shorter)
create_converter('UINT', 'I') # F_UINT used previously (always 'I')

class LPCWSTR_converter(Py_UNICODE_converter):
    type = 'LPCWSTR'

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
            'return_value = PyLong_FromUnsignedLong(_return_value);\n')

class LPVOID_return_converter(CReturnConverter):
    type = 'LPVOID'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if("_return_value == NULL", data)
        data.return_conversion.append(
            'return_value = HANDLE_TO_PYNUM(_return_value);\n')
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=da0a4db751936ee7]*/

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
    return Py_NewRef(res);
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
        res = CancelIoEx(self->handle, &self->overlapped);
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
     Py_READONLY, "overlapped event handle"},
    {NULL}
};

static PyType_Slot winapi_overlapped_type_slots[] = {
    {Py_tp_traverse, overlapped_traverse},
    {Py_tp_dealloc, overlapped_dealloc},
    {Py_tp_doc, "OVERLAPPED structure wrapper"},
    {Py_tp_methods, overlapped_methods},
    {Py_tp_members, overlapped_members},
    {0,0}
};

static PyType_Spec winapi_overlapped_type_spec = {
    .name = "_winapi.Overlapped",
    .basicsize = sizeof(OverlappedObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = winapi_overlapped_type_slots,
};

static OverlappedObject *
new_overlapped(PyObject *module, HANDLE handle)
{
    WinApiState *st = winapi_get_state(module);
    OverlappedObject *self = PyObject_GC_New(OverlappedObject, st->overlapped_type);
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

    PyObject_GC_Track(self);
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
    overlapped as use_overlapped: bool = False
[clinic start generated code]*/

static PyObject *
_winapi_ConnectNamedPipe_impl(PyObject *module, HANDLE handle,
                              int use_overlapped)
/*[clinic end generated code: output=335a0e7086800671 input=a80e56e8bd370e31]*/
{
    BOOL success;
    OverlappedObject *overlapped = NULL;

    if (use_overlapped) {
        overlapped = new_overlapped(module, handle);
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
_winapi.CreateEventW -> HANDLE

    security_attributes: LPSECURITY_ATTRIBUTES
    manual_reset: BOOL
    initial_state: BOOL
    name: LPCWSTR(accept={str, NoneType})
[clinic start generated code]*/

static HANDLE
_winapi_CreateEventW_impl(PyObject *module,
                          LPSECURITY_ATTRIBUTES security_attributes,
                          BOOL manual_reset, BOOL initial_state,
                          LPCWSTR name)
/*[clinic end generated code: output=2d4c7d5852ecb298 input=4187cee28ac763f8]*/
{
    HANDLE handle;

    if (PySys_Audit("_winapi.CreateEventW", "bbu", manual_reset, initial_state, name) < 0) {
        return INVALID_HANDLE_VALUE;
    }

    Py_BEGIN_ALLOW_THREADS
    handle = CreateEventW(security_attributes, manual_reset, initial_state, name);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
    }

    return handle;
}

/*[clinic input]
_winapi.CreateFile -> HANDLE

    file_name: LPCWSTR
    desired_access: DWORD
    share_mode: DWORD
    security_attributes: LPSECURITY_ATTRIBUTES
    creation_disposition: DWORD
    flags_and_attributes: DWORD
    template_file: HANDLE
    /
[clinic start generated code]*/

static HANDLE
_winapi_CreateFile_impl(PyObject *module, LPCWSTR file_name,
                        DWORD desired_access, DWORD share_mode,
                        LPSECURITY_ATTRIBUTES security_attributes,
                        DWORD creation_disposition,
                        DWORD flags_and_attributes, HANDLE template_file)
/*[clinic end generated code: output=818c811e5e04d550 input=1fa870ed1c2e3d69]*/
{
    HANDLE handle;

    if (PySys_Audit("_winapi.CreateFile", "ukkkk",
                    file_name, desired_access, share_mode,
                    creation_disposition, flags_and_attributes) < 0) {
        return INVALID_HANDLE_VALUE;
    }

    Py_BEGIN_ALLOW_THREADS
    handle = CreateFileW(file_name, desired_access,
                         share_mode, security_attributes,
                         creation_disposition,
                         flags_and_attributes, template_file);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
    }

    return handle;
}

/*[clinic input]
_winapi.CreateFileMapping -> HANDLE

    file_handle: HANDLE
    security_attributes: LPSECURITY_ATTRIBUTES
    protect: DWORD
    max_size_high: DWORD
    max_size_low: DWORD
    name: LPCWSTR
    /
[clinic start generated code]*/

static HANDLE
_winapi_CreateFileMapping_impl(PyObject *module, HANDLE file_handle,
                               LPSECURITY_ATTRIBUTES security_attributes,
                               DWORD protect, DWORD max_size_high,
                               DWORD max_size_low, LPCWSTR name)
/*[clinic end generated code: output=6c0a4d5cf7f6fcc6 input=3dc5cf762a74dee8]*/
{
    HANDLE handle;

    Py_BEGIN_ALLOW_THREADS
    handle = CreateFileMappingW(file_handle, security_attributes,
                                protect, max_size_high, max_size_low,
                                name);
    Py_END_ALLOW_THREADS

    if (handle == NULL) {
        PyObject *temp = PyUnicode_FromWideChar(name, -1);
        PyErr_SetExcFromWindowsErrWithFilenameObject(PyExc_OSError, 0, temp);
        Py_XDECREF(temp);
        handle = INVALID_HANDLE_VALUE;
    }

    return handle;
}

/*[clinic input]
_winapi.CreateJunction

    src_path: LPCWSTR
    dst_path: LPCWSTR
    /
[clinic start generated code]*/

static PyObject *
_winapi_CreateJunction_impl(PyObject *module, LPCWSTR src_path,
                            LPCWSTR dst_path)
/*[clinic end generated code: output=44b3f5e9bbcc4271 input=963d29b44b9384a7]*/
{
    /* Privilege adjustment */
    HANDLE token = NULL;
    struct {
        TOKEN_PRIVILEGES base;
        /* overallocate by a few array elements */
        LUID_AND_ATTRIBUTES privs[4];
    } tp, previousTp;
    int previousTpSize = 0;

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

    if (PySys_Audit("_winapi.CreateJunction", "uu", src_path, dst_path) < 0) {
        return NULL;
    }

    /* Adjust privileges to allow rewriting directory entry as a
       junction point. */
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        goto cleanup;
    }

    if (!LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &tp.base.Privileges[0].Luid)) {
        goto cleanup;
    }

    tp.base.PrivilegeCount = 1;
    tp.base.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(token, FALSE, &tp.base, sizeof(previousTp),
                               &previousTp.base, &previousTpSize)) {
        goto cleanup;
    }

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
    rdb = (_Py_PREPARSE_DATA_BUFFER)PyMem_RawCalloc(1, rdb_size);
    if (rdb == NULL)
        goto cleanup;

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

    if (previousTpSize) {
        AdjustTokenPrivileges(token, FALSE, &previousTp.base, previousTpSize,
                              NULL, NULL);
    }

    if (token != NULL)
        CloseHandle(token);
    if (junction != NULL)
        CloseHandle(junction);
    PyMem_RawFree(rdb);

    if (ret != 0)
        return PyErr_SetFromWindowsErr(ret);

    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.CreateMutexW -> HANDLE

    security_attributes: LPSECURITY_ATTRIBUTES
    initial_owner: BOOL
    name: LPCWSTR(accept={str, NoneType})
[clinic start generated code]*/

static HANDLE
_winapi_CreateMutexW_impl(PyObject *module,
                          LPSECURITY_ATTRIBUTES security_attributes,
                          BOOL initial_owner, LPCWSTR name)
/*[clinic end generated code: output=31b9ee8fc37e49a5 input=7d54b921e723254a]*/
{
    HANDLE handle;

    if (PySys_Audit("_winapi.CreateMutexW", "bu", initial_owner, name) < 0) {
        return INVALID_HANDLE_VALUE;
    }

    Py_BEGIN_ALLOW_THREADS
    handle = CreateMutexW(security_attributes, initial_owner, name);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
    }

    return handle;
}

/*[clinic input]
_winapi.CreateNamedPipe -> HANDLE

    name: LPCWSTR
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
_winapi_CreateNamedPipe_impl(PyObject *module, LPCWSTR name, DWORD open_mode,
                             DWORD pipe_mode, DWORD max_instances,
                             DWORD out_buffer_size, DWORD in_buffer_size,
                             DWORD default_timeout,
                             LPSECURITY_ATTRIBUTES security_attributes)
/*[clinic end generated code: output=7d6fde93227680ba input=5bd4e4a55639ee02]*/
{
    HANDLE handle;

    if (PySys_Audit("_winapi.CreateNamedPipe", "ukk",
                    name, open_mode, pipe_mode) < 0) {
        return INVALID_HANDLE_VALUE;
    }

    Py_BEGIN_ALLOW_THREADS
    handle = CreateNamedPipeW(name, open_mode, pipe_mode,
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

    if (PySys_Audit("_winapi.CreatePipe", NULL) < 0) {
        return NULL;
    }

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

static PyObject *
sortenvironmentkey(PyObject *module, PyObject *item)
{
    return _winapi_LCMapStringEx_impl(NULL, LOCALE_NAME_INVARIANT,
                                      LCMAP_UPPERCASE, item);
}

static PyMethodDef sortenvironmentkey_def = {
    "sortenvironmentkey", _PyCFunction_CAST(sortenvironmentkey), METH_O, "",
};

static int
sort_environment_keys(PyObject *keys)
{
    PyObject *keyfunc = PyCFunction_New(&sortenvironmentkey_def, NULL);
    if (keyfunc == NULL) {
        return -1;
    }
    PyObject *kwnames = Py_BuildValue("(s)", "key");
    if (kwnames == NULL) {
        Py_DECREF(keyfunc);
        return -1;
    }
    PyObject *args[] = { keys, keyfunc };
    PyObject *ret = PyObject_VectorcallMethod(&_Py_ID(sort), args, 1, kwnames);
    Py_DECREF(keyfunc);
    Py_DECREF(kwnames);
    if (ret == NULL) {
        return -1;
    }
    Py_DECREF(ret);

    return 0;
}

static int
compare_string_ordinal(PyObject *str1, PyObject *str2, int *result)
{
    wchar_t *s1 = PyUnicode_AsWideCharString(str1, NULL);
    if (s1 == NULL) {
        return -1;
    }
    wchar_t *s2 = PyUnicode_AsWideCharString(str2, NULL);
    if (s2 == NULL) {
        PyMem_Free(s1);
        return -1;
    }
    *result = CompareStringOrdinal(s1, -1, s2, -1, TRUE);
    PyMem_Free(s1);
    PyMem_Free(s2);
    return 0;
}

static PyObject *
dedup_environment_keys(PyObject *keys)
{
    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }

    // Iterate over the pre-ordered keys, check whether the current key is equal
    // to the next key (ignoring case), if different, insert the current value
    // into the result list. If they are equal, do nothing because we always
    // want to keep the last inserted one.
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(keys); i++) {
        PyObject *key = PyList_GET_ITEM(keys, i);

        // The last key will always be kept.
        if (i + 1 == PyList_GET_SIZE(keys)) {
            if (PyList_Append(result, key) < 0) {
                Py_DECREF(result);
                return NULL;
            }
            continue;
        }

        PyObject *next_key = PyList_GET_ITEM(keys, i + 1);
        int compare_result;
        if (compare_string_ordinal(key, next_key, &compare_result) < 0) {
            Py_DECREF(result);
            return NULL;
        }
        if (compare_result == CSTR_EQUAL) {
            continue;
        }
        if (PyList_Append(result, key) < 0) {
            Py_DECREF(result);
            return NULL;
        }
    }

    return result;
}

static PyObject *
normalize_environment(PyObject *environment)
{
    PyObject *keys = PyMapping_Keys(environment);
    if (keys == NULL) {
        return NULL;
    }

    if (sort_environment_keys(keys) < 0) {
        Py_DECREF(keys);
        return NULL;
    }

    PyObject *normalized_keys = dedup_environment_keys(keys);
    Py_DECREF(keys);
    if (normalized_keys == NULL) {
        return NULL;
    }

    PyObject *result = PyDict_New();
    if (result == NULL) {
        Py_DECREF(normalized_keys);
        return NULL;
    }

    for (int i = 0; i < PyList_GET_SIZE(normalized_keys); i++) {
        PyObject *key = PyList_GET_ITEM(normalized_keys, i);
        PyObject *value = PyObject_GetItem(environment, key);
        if (value == NULL) {
            Py_DECREF(normalized_keys);
            Py_DECREF(result);
            return NULL;
        }

        int ret = PyObject_SetItem(result, key, value);
        Py_DECREF(value);
        if (ret < 0) {
            Py_DECREF(normalized_keys);
            Py_DECREF(result);
            return NULL;
        }
    }

    Py_DECREF(normalized_keys);

    return result;
}

static wchar_t *
getenvironment(PyObject* environment)
{
    Py_ssize_t i, envsize, totalsize;
    wchar_t *buffer = NULL, *p, *end;
    PyObject *normalized_environment = NULL;
    PyObject *keys = NULL;
    PyObject *values = NULL;

    /* convert environment dictionary to windows environment string */
    if (! PyMapping_Check(environment)) {
        PyErr_SetString(
            PyExc_TypeError, "environment must be dictionary or None");
        return NULL;
    }

    normalized_environment = normalize_environment(environment);
    if (normalized_environment == NULL) {
        return NULL;
    }

    keys = PyMapping_Keys(normalized_environment);
    if (!keys) {
        goto error;
    }
    values = PyMapping_Values(normalized_environment);
    if (!values) {
        goto error;
    }

    envsize = PyList_GET_SIZE(keys);

    if (envsize == 0) {
        // A environment block must be terminated by two null characters --
        // one for the last string and one for the block.
        buffer = PyMem_Calloc(2, sizeof(wchar_t));
        if (!buffer) {
            PyErr_NoMemory();
        }
        goto cleanup;
    }

    if (PyList_GET_SIZE(values) != envsize) {
        PyErr_SetString(PyExc_RuntimeError,
            "environment changed size during iteration");
        goto error;
    }

    totalsize = 1; /* trailing null character */
    for (i = 0; i < envsize; i++) {
        PyObject* key = PyList_GET_ITEM(keys, i);
        PyObject* value = PyList_GET_ITEM(values, i);
        Py_ssize_t size;

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

        size = PyUnicode_AsWideChar(key, NULL, 0);
        assert(size > 1);
        if (totalsize > PY_SSIZE_T_MAX - size) {
            PyErr_SetString(PyExc_OverflowError, "environment too long");
            goto error;
        }
        totalsize += size;    /* including '=' */

        size = PyUnicode_AsWideChar(value, NULL, 0);
        assert(size > 0);
        if (totalsize > PY_SSIZE_T_MAX - size) {
            PyErr_SetString(PyExc_OverflowError, "environment too long");
            goto error;
        }
        totalsize += size;  /* including trailing '\0' */
    }

    buffer = PyMem_NEW(wchar_t, totalsize);
    if (! buffer) {
        PyErr_NoMemory();
        goto error;
    }
    p = buffer;
    end = buffer + totalsize;

    for (i = 0; i < envsize; i++) {
        PyObject* key = PyList_GET_ITEM(keys, i);
        PyObject* value = PyList_GET_ITEM(values, i);
        Py_ssize_t size = PyUnicode_AsWideChar(key, p, end - p);
        assert(1 <= size && size < end - p);
        p += size;
        *p++ = L'=';
        size = PyUnicode_AsWideChar(value, p, end - p);
        assert(0 <= size && size < end - p);
        p += size + 1;
    }

    /* add trailing null character */
    *p++ = L'\0';
    assert(p == end);

cleanup:
error:
    Py_XDECREF(normalized_environment);
    Py_XDECREF(keys);
    Py_XDECREF(values);
    return buffer;
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
_winapi_CreateProcess_impl(PyObject *module, const wchar_t *application_name,
                           PyObject *command_line, PyObject *proc_attrs,
                           PyObject *thread_attrs, BOOL inherit_handles,
                           DWORD creation_flags, PyObject *env_mapping,
                           const wchar_t *current_directory,
                           PyObject *startup_info)
/*[clinic end generated code: output=a25c8e49ea1d6427 input=42ac293eaea03fc4]*/
{
    PyObject *ret = NULL;
    BOOL result;
    PROCESS_INFORMATION pi;
    STARTUPINFOEXW si;
    wchar_t *wenvironment = NULL;
    wchar_t *command_line_copy = NULL;
    AttributeList attribute_list = {0};

    if (PySys_Audit("_winapi.CreateProcess", "uuu", application_name,
                    command_line, current_directory) < 0) {
        return NULL;
    }

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
        wenvironment = getenvironment(env_mapping);
        if (wenvironment == NULL) {
            goto cleanup;
        }
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
    PyMem_Free(wenvironment);
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
#ifdef MS_WINDOWS_DESKTOP
        SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT|
                     SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
#endif
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
_winapi.GetLongPathName

    path: LPCWSTR

Return the long version of the provided path.

If the path is already in its long form, returns the same value.

The path must already be a 'str'. If the type is not known, use
os.fsdecode before calling this function.
[clinic start generated code]*/

static PyObject *
_winapi_GetLongPathName_impl(PyObject *module, LPCWSTR path)
/*[clinic end generated code: output=c4774b080275a2d0 input=9872e211e3a4a88f]*/
{
    DWORD cchBuffer;
    PyObject *result = NULL;

    Py_BEGIN_ALLOW_THREADS
    cchBuffer = GetLongPathNameW(path, NULL, 0);
    Py_END_ALLOW_THREADS
    if (cchBuffer) {
        WCHAR *buffer = (WCHAR *)PyMem_Malloc(cchBuffer * sizeof(WCHAR));
        if (buffer) {
            Py_BEGIN_ALLOW_THREADS
            cchBuffer = GetLongPathNameW(path, buffer, cchBuffer);
            Py_END_ALLOW_THREADS
            if (cchBuffer) {
                result = PyUnicode_FromWideChar(buffer, cchBuffer);
            } else {
                PyErr_SetFromWindowsErr(0);
            }
            PyMem_Free((void *)buffer);
        }
    } else {
        PyErr_SetFromWindowsErr(0);
    }
    return result;
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

    Py_BEGIN_ALLOW_THREADS
    result = GetModuleFileNameW(module_handle, filename, MAX_PATH);
    filename[MAX_PATH-1] = '\0';
    Py_END_ALLOW_THREADS

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return PyUnicode_FromWideChar(filename, wcslen(filename));
}

/*[clinic input]
_winapi.GetShortPathName

    path: LPCWSTR

Return the short version of the provided path.

If the path is already in its short form, returns the same value.

The path must already be a 'str'. If the type is not known, use
os.fsdecode before calling this function.
[clinic start generated code]*/

static PyObject *
_winapi_GetShortPathName_impl(PyObject *module, LPCWSTR path)
/*[clinic end generated code: output=dab6ae494c621e81 input=43fa349aaf2ac718]*/
{
    DWORD cchBuffer;
    PyObject *result = NULL;

    Py_BEGIN_ALLOW_THREADS
    cchBuffer = GetShortPathNameW(path, NULL, 0);
    Py_END_ALLOW_THREADS
    if (cchBuffer) {
        WCHAR *buffer = (WCHAR *)PyMem_Malloc(cchBuffer * sizeof(WCHAR));
        if (buffer) {
            Py_BEGIN_ALLOW_THREADS
            cchBuffer = GetShortPathNameW(path, buffer, cchBuffer);
            Py_END_ALLOW_THREADS
            if (cchBuffer) {
                result = PyUnicode_FromWideChar(buffer, cchBuffer);
            } else {
                PyErr_SetFromWindowsErr(0);
            }
            PyMem_Free((void *)buffer);
        }
    } else {
        PyErr_SetFromWindowsErr(0);
    }
    return result;
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
_winapi.MapViewOfFile -> LPVOID

    file_map: HANDLE
    desired_access: DWORD
    file_offset_high: DWORD
    file_offset_low: DWORD
    number_bytes: size_t
    /
[clinic start generated code]*/

static LPVOID
_winapi_MapViewOfFile_impl(PyObject *module, HANDLE file_map,
                           DWORD desired_access, DWORD file_offset_high,
                           DWORD file_offset_low, size_t number_bytes)
/*[clinic end generated code: output=f23b1ee4823663e3 input=177471073be1a103]*/
{
    LPVOID address;

    Py_BEGIN_ALLOW_THREADS
    address = MapViewOfFile(file_map, desired_access, file_offset_high,
                            file_offset_low, number_bytes);
    Py_END_ALLOW_THREADS

    if (address == NULL)
        PyErr_SetFromWindowsErr(0);

    return address;
}

/*[clinic input]
_winapi.UnmapViewOfFile

    address: LPCVOID
    /
[clinic start generated code]*/

static PyObject *
_winapi_UnmapViewOfFile_impl(PyObject *module, LPCVOID address)
/*[clinic end generated code: output=4f7e18ac75d19744 input=8c4b6119ad9288a3]*/
{
    BOOL success;

    Py_BEGIN_ALLOW_THREADS
    success = UnmapViewOfFile(address);
    Py_END_ALLOW_THREADS

    if (!success) {
        return PyErr_SetFromWindowsErr(0);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.OpenEventW -> HANDLE

    desired_access: DWORD
    inherit_handle: BOOL
    name: LPCWSTR
[clinic start generated code]*/

static HANDLE
_winapi_OpenEventW_impl(PyObject *module, DWORD desired_access,
                        BOOL inherit_handle, LPCWSTR name)
/*[clinic end generated code: output=c4a45e95545a4bd2 input=dec26598748d35aa]*/
{
    HANDLE handle;

    if (PySys_Audit("_winapi.OpenEventW", "ku", desired_access, name) < 0) {
        return INVALID_HANDLE_VALUE;
    }

    Py_BEGIN_ALLOW_THREADS
    handle = OpenEventW(desired_access, inherit_handle, name);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
    }

    return handle;
}


/*[clinic input]
_winapi.OpenMutexW -> HANDLE

    desired_access: DWORD
    inherit_handle: BOOL
    name: LPCWSTR
[clinic start generated code]*/

static HANDLE
_winapi_OpenMutexW_impl(PyObject *module, DWORD desired_access,
                        BOOL inherit_handle, LPCWSTR name)
/*[clinic end generated code: output=dda39d7844397bf0 input=f3a7b466c5307712]*/
{
    HANDLE handle;

    if (PySys_Audit("_winapi.OpenMutexW", "ku", desired_access, name) < 0) {
        return INVALID_HANDLE_VALUE;
    }

    Py_BEGIN_ALLOW_THREADS
    handle = OpenMutexW(desired_access, inherit_handle, name);
    Py_END_ALLOW_THREADS

    if (handle == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
    }

    return handle;
}

/*[clinic input]
_winapi.OpenFileMapping -> HANDLE

    desired_access: DWORD
    inherit_handle: BOOL
    name: LPCWSTR
    /
[clinic start generated code]*/

static HANDLE
_winapi_OpenFileMapping_impl(PyObject *module, DWORD desired_access,
                             BOOL inherit_handle, LPCWSTR name)
/*[clinic end generated code: output=08cc44def1cb11f1 input=131f2a405359de7f]*/
{
    HANDLE handle;

    Py_BEGIN_ALLOW_THREADS
    handle = OpenFileMappingW(desired_access, inherit_handle, name);
    Py_END_ALLOW_THREADS

    if (handle == NULL) {
        PyObject *temp = PyUnicode_FromWideChar(name, -1);
        PyErr_SetExcFromWindowsErrWithFilenameObject(PyExc_OSError, 0, temp);
        Py_XDECREF(temp);
        handle = INVALID_HANDLE_VALUE;
    }

    return handle;
}

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

    if (PySys_Audit("_winapi.OpenProcess", "kk",
                    process_id, desired_access) < 0) {
        return INVALID_HANDLE_VALUE;
    }

    Py_BEGIN_ALLOW_THREADS
    handle = OpenProcess(desired_access, inherit_handle, process_id);
    Py_END_ALLOW_THREADS
    if (handle == NULL) {
        PyErr_SetFromWindowsErr(GetLastError());
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
_winapi.LCMapStringEx

    locale: LPCWSTR
    flags: DWORD
    src: unicode

[clinic start generated code]*/

static PyObject *
_winapi_LCMapStringEx_impl(PyObject *module, LPCWSTR locale, DWORD flags,
                           PyObject *src)
/*[clinic end generated code: output=b90e6b26e028ff0a input=3e3dcd9b8164012f]*/
{
    if (flags & (LCMAP_SORTHANDLE | LCMAP_HASH | LCMAP_BYTEREV |
                 LCMAP_SORTKEY)) {
        return PyErr_Format(PyExc_ValueError, "unsupported flags");
    }

    Py_ssize_t src_size;
    wchar_t *src_ = PyUnicode_AsWideCharString(src, &src_size);
    if (!src_) {
        return NULL;
    }
    if (src_size > INT_MAX) {
        PyMem_Free(src_);
        PyErr_SetString(PyExc_OverflowError, "input string is too long");
        return NULL;
    }

    int dest_size = LCMapStringEx(locale, flags, src_, (int)src_size, NULL, 0,
                                  NULL, NULL, 0);
    if (dest_size <= 0) {
        DWORD error = GetLastError();
        PyMem_Free(src_);
        return PyErr_SetFromWindowsErr(error);
    }

    wchar_t* dest = PyMem_NEW(wchar_t, dest_size);
    if (dest == NULL) {
        PyMem_Free(src_);
        return PyErr_NoMemory();
    }

    int nmapped = LCMapStringEx(locale, flags, src_, (int)src_size, dest, dest_size,
                                NULL, NULL, 0);
    if (nmapped <= 0) {
        DWORD error = GetLastError();
        PyMem_Free(src_);
        PyMem_DEL(dest);
        return PyErr_SetFromWindowsErr(error);
    }

    PyMem_Free(src_);
    PyObject *ret = PyUnicode_FromWideChar(dest, nmapped);
    PyMem_DEL(dest);

    return ret;
}

/*[clinic input]
_winapi.ReadFile

    handle: HANDLE
    size: DWORD
    overlapped as use_overlapped: bool = False
[clinic start generated code]*/

static PyObject *
_winapi_ReadFile_impl(PyObject *module, HANDLE handle, DWORD size,
                      int use_overlapped)
/*[clinic end generated code: output=d3d5b44a8201b944 input=4f82f8e909ad91ad]*/
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
        overlapped = new_overlapped(module, handle);
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
_winapi.ReleaseMutex

    mutex: HANDLE
[clinic start generated code]*/

static PyObject *
_winapi_ReleaseMutex_impl(PyObject *module, HANDLE mutex)
/*[clinic end generated code: output=5b9001a72dd8af37 input=49e9d20de3559d84]*/
{
    int err = 0;

    Py_BEGIN_ALLOW_THREADS
    if (!ReleaseMutex(mutex)) {
        err = GetLastError();
    }
    Py_END_ALLOW_THREADS
    if (err) {
        return PyErr_SetFromWindowsErr(err);
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.ResetEvent

    event: HANDLE
[clinic start generated code]*/

static PyObject *
_winapi_ResetEvent_impl(PyObject *module, HANDLE event)
/*[clinic end generated code: output=81c8501d57c0530d input=e2d42d990322e87a]*/
{
    int err = 0;

    Py_BEGIN_ALLOW_THREADS
    if (!ResetEvent(event)) {
        err = GetLastError();
    }
    Py_END_ALLOW_THREADS
    if (err) {
        return PyErr_SetFromWindowsErr(err);
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.SetEvent

    event: HANDLE
[clinic start generated code]*/

static PyObject *
_winapi_SetEvent_impl(PyObject *module, HANDLE event)
/*[clinic end generated code: output=c18ba09eb9aa774d input=e660e830a37c09f8]*/
{
    int err = 0;

    Py_BEGIN_ALLOW_THREADS
    if (!SetEvent(event)) {
        err = GetLastError();
    }
    Py_END_ALLOW_THREADS
    if (err) {
        return PyErr_SetFromWindowsErr(err);
    }
    Py_RETURN_NONE;
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
    BOOL b;

    for (i = 0 ; i < 3 ; i++) {
        if (oArgs[i] != Py_None) {
            dwArgs[i] = PyLong_AsUnsignedLongMask(oArgs[i]);
            if (PyErr_Occurred())
                return NULL;
            pArgs[i] = &dwArgs[i];
        }
    }

    Py_BEGIN_ALLOW_THREADS
    b = SetNamedPipeHandleState(named_pipe, pArgs[0], pArgs[1], pArgs[2]);
    Py_END_ALLOW_THREADS

    if (!b)
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

    if (PySys_Audit("_winapi.TerminateProcess", "nI",
                    (Py_ssize_t)handle, exit_code) < 0) {
        return NULL;
    }

    result = TerminateProcess(handle, exit_code);

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    Py_RETURN_NONE;
}

/*[clinic input]
_winapi.VirtualQuerySize -> size_t

    address: LPCVOID
    /
[clinic start generated code]*/

static size_t
_winapi_VirtualQuerySize_impl(PyObject *module, LPCVOID address)
/*[clinic end generated code: output=40c8e0ff5ec964df input=6b784a69755d0bb6]*/
{
    SIZE_T size_of_buf;
    MEMORY_BASIC_INFORMATION mem_basic_info;
    SIZE_T region_size;

    Py_BEGIN_ALLOW_THREADS
    size_of_buf = VirtualQuery(address, &mem_basic_info, sizeof(mem_basic_info));
    Py_END_ALLOW_THREADS

    if (size_of_buf == 0)
        PyErr_SetFromWindowsErr(0);

    region_size = mem_basic_info.RegionSize;
    return region_size;
}

/*[clinic input]
_winapi.WaitNamedPipe

    name: LPCWSTR
    timeout: DWORD
    /
[clinic start generated code]*/

static PyObject *
_winapi_WaitNamedPipe_impl(PyObject *module, LPCWSTR name, DWORD timeout)
/*[clinic end generated code: output=e161e2e630b3e9c2 input=099a4746544488fa]*/
{
    BOOL success;

    Py_BEGIN_ALLOW_THREADS
    success = WaitNamedPipeW(name, timeout);
    Py_END_ALLOW_THREADS

    if (!success)
        return PyErr_SetFromWindowsErr(0);

    Py_RETURN_NONE;
}


typedef struct {
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    HANDLE cancel_event;
    DWORD handle_base;
    DWORD handle_count;
    HANDLE thread;
    volatile DWORD result;
} BatchedWaitData;

static DWORD WINAPI
_batched_WaitForMultipleObjects_thread(LPVOID param)
{
    BatchedWaitData *data = (BatchedWaitData *)param;
    data->result = WaitForMultipleObjects(
        data->handle_count,
        data->handles,
        FALSE,
        INFINITE
    );
    if (data->result == WAIT_FAILED) {
        DWORD err = GetLastError();
        SetEvent(data->cancel_event);
        return err;
    } else if (data->result >= WAIT_ABANDONED_0 && data->result < WAIT_ABANDONED_0 + MAXIMUM_WAIT_OBJECTS) {
        data->result = WAIT_FAILED;
        SetEvent(data->cancel_event);
        return ERROR_ABANDONED_WAIT_0;
    }
    return 0;
}

/*[clinic input]
_winapi.BatchedWaitForMultipleObjects

    handle_seq: object
    wait_all: BOOL
    milliseconds: DWORD(c_default='INFINITE') = _winapi.INFINITE

Supports a larger number of handles than WaitForMultipleObjects

Note that the handles may be waited on other threads, which could cause
issues for objects like mutexes that become associated with the thread
that was waiting for them. Objects may also be left signalled, even if
the wait fails.

It is recommended to use WaitForMultipleObjects whenever possible, and
only switch to BatchedWaitForMultipleObjects for scenarios where you
control all the handles involved, such as your own thread pool or
files, and all wait objects are left unmodified by a wait (for example,
manual reset events, threads, and files/pipes).

Overlapped handles returned from this module use manual reset events.
[clinic start generated code]*/

static PyObject *
_winapi_BatchedWaitForMultipleObjects_impl(PyObject *module,
                                           PyObject *handle_seq,
                                           BOOL wait_all, DWORD milliseconds)
/*[clinic end generated code: output=d21c1a4ad0a252fd input=7e196f29005dc77b]*/
{
    Py_ssize_t thread_count = 0, handle_count = 0, i;
    Py_ssize_t nhandles;
    BatchedWaitData *thread_data[MAXIMUM_WAIT_OBJECTS];
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    HANDLE sigint_event = NULL;
    HANDLE cancel_event = NULL;
    DWORD result;

    const Py_ssize_t _MAXIMUM_TOTAL_OBJECTS = (MAXIMUM_WAIT_OBJECTS - 1) * (MAXIMUM_WAIT_OBJECTS - 1);

    if (!PySequence_Check(handle_seq)) {
        PyErr_Format(PyExc_TypeError,
                     "sequence type expected, got '%s'",
                     Py_TYPE(handle_seq)->tp_name);
        return NULL;
    }
    nhandles = PySequence_Length(handle_seq);
    if (nhandles == -1) {
        return NULL;
    }
    if (nhandles == 0) {
        return wait_all ? Py_NewRef(Py_None) : PyList_New(0);
    }

    /* If this is the main thread then make the wait interruptible
       by Ctrl-C. When waiting for *all* handles, it is only checked
       in between batches. */
    if (_PyOS_IsMainThread()) {
        sigint_event = _PyOS_SigintEvent();
        assert(sigint_event != NULL);
    }

    if (nhandles < 0 || nhandles > _MAXIMUM_TOTAL_OBJECTS) {
        PyErr_Format(PyExc_ValueError,
                     "need at most %zd handles, got a sequence of length %zd",
                     _MAXIMUM_TOTAL_OBJECTS, nhandles);
        return NULL;
    }

    if (!wait_all) {
        cancel_event = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!cancel_event) {
            PyErr_SetExcFromWindowsErr(PyExc_OSError, 0);
            return NULL;
        }
    }

    i = 0;
    while (i < nhandles) {
        BatchedWaitData *data = (BatchedWaitData*)PyMem_Malloc(sizeof(BatchedWaitData));
        if (!data) {
            goto error;
        }
        thread_data[thread_count++] = data;
        data->thread = NULL;
        data->cancel_event = cancel_event;
        data->handle_base = Py_SAFE_DOWNCAST(i, Py_ssize_t, DWORD);
        data->handle_count = Py_SAFE_DOWNCAST(nhandles - i, Py_ssize_t, DWORD);
        if (data->handle_count > MAXIMUM_WAIT_OBJECTS - 1) {
            data->handle_count = MAXIMUM_WAIT_OBJECTS - 1;
        }
        for (DWORD j = 0; j < data->handle_count; ++i, ++j) {
            PyObject *v = PySequence_GetItem(handle_seq, i);
            if (!v || !PyArg_Parse(v, F_HANDLE, &data->handles[j])) {
                Py_XDECREF(v);
                goto error;
            }
            Py_DECREF(v);
        }
        if (!wait_all) {
            data->handles[data->handle_count++] = cancel_event;
        }
    }

    DWORD err = 0;

    /* We need to use different strategies when waiting for ALL handles
       as opposed to ANY handle. This is because there is no way to
       (safely) interrupt a thread that is waiting for all handles in a
       group. So for ALL handles, we loop over each set and wait. For
       ANY handle, we use threads and wait on them. */
    if (wait_all) {
        Py_BEGIN_ALLOW_THREADS
        long long deadline = 0;
        if (milliseconds != INFINITE) {
            deadline = (long long)GetTickCount64() + milliseconds;
        }

        for (i = 0; !err && i < thread_count; ++i) {
            DWORD timeout = milliseconds;
            if (deadline) {
                long long time_to_deadline = deadline - GetTickCount64();
                if (time_to_deadline <= 0) {
                    err = WAIT_TIMEOUT;
                    break;
                } else if (time_to_deadline < UINT_MAX) {
                    timeout = (DWORD)time_to_deadline;
                }
            }
            result = WaitForMultipleObjects(thread_data[i]->handle_count,
                                            thread_data[i]->handles, TRUE, timeout);
            // ABANDONED is not possible here because we own all the handles
            if (result == WAIT_FAILED) {
                err = GetLastError();
            } else if (result == WAIT_TIMEOUT) {
                err = WAIT_TIMEOUT;
            }

            if (!err && sigint_event) {
                result = WaitForSingleObject(sigint_event, 0);
                if (result == WAIT_OBJECT_0) {
                    err = ERROR_CONTROL_C_EXIT;
                } else if (result == WAIT_FAILED) {
                    err = GetLastError();
                }
            }
        }

        CloseHandle(cancel_event);

        Py_END_ALLOW_THREADS
    } else {
        Py_BEGIN_ALLOW_THREADS

        for (i = 0; i < thread_count; ++i) {
            BatchedWaitData *data = thread_data[i];
            data->thread = CreateThread(
                NULL,
                1,  // smallest possible initial stack
                _batched_WaitForMultipleObjects_thread,
                (LPVOID)data,
                CREATE_SUSPENDED,
                NULL
            );
            if (!data->thread) {
                err = GetLastError();
                break;
            }
            handles[handle_count++] = data->thread;
        }
        Py_END_ALLOW_THREADS

        if (err) {
            PyErr_SetExcFromWindowsErr(PyExc_OSError, err);
            goto error;
        }
        if (handle_count > MAXIMUM_WAIT_OBJECTS - 1) {
            // basically an assert, but stronger
            PyErr_SetString(PyExc_SystemError, "allocated too many wait objects");
            goto error;
        }

        Py_BEGIN_ALLOW_THREADS

        // Once we start resuming threads, can no longer "goto error"
        for (i = 0; i < thread_count; ++i) {
            ResumeThread(thread_data[i]->thread);
        }
        if (sigint_event) {
            handles[handle_count++] = sigint_event;
        }
        result = WaitForMultipleObjects((DWORD)handle_count, handles, wait_all, milliseconds);
        // ABANDONED is not possible here because we own all the handles
        if (result == WAIT_FAILED) {
            err = GetLastError();
        } else if (result == WAIT_TIMEOUT) {
            err = WAIT_TIMEOUT;
        } else if (sigint_event && result == WAIT_OBJECT_0 + handle_count) {
            err = ERROR_CONTROL_C_EXIT;
        }

        SetEvent(cancel_event);

        // Wait for all threads to finish before we start freeing their memory
        if (sigint_event) {
            handle_count -= 1;
        }
        WaitForMultipleObjects((DWORD)handle_count, handles, TRUE, INFINITE);

        for (i = 0; i < thread_count; ++i) {
            if (!err && thread_data[i]->result == WAIT_FAILED) {
                if (!GetExitCodeThread(thread_data[i]->thread, &err)) {
                    err = GetLastError();
                }
            }
            CloseHandle(thread_data[i]->thread);
        }

        CloseHandle(cancel_event);

        Py_END_ALLOW_THREADS

    }

    PyObject *triggered_indices;
    if (sigint_event != NULL && err == ERROR_CONTROL_C_EXIT) {
        errno = EINTR;
        PyErr_SetFromErrno(PyExc_OSError);
        triggered_indices = NULL;
    } else if (err) {
        PyErr_SetExcFromWindowsErr(PyExc_OSError, err);
        triggered_indices = NULL;
    } else if (wait_all) {
        triggered_indices = Py_NewRef(Py_None);
    } else {
        triggered_indices = PyList_New(0);
        if (triggered_indices) {
            for (i = 0; i < thread_count; ++i) {
                Py_ssize_t triggered = (Py_ssize_t)thread_data[i]->result - WAIT_OBJECT_0;
                if (triggered >= 0 && (size_t)triggered < thread_data[i]->handle_count - 1) {
                    PyObject *v = PyLong_FromSsize_t(thread_data[i]->handle_base + triggered);
                    if (!v || PyList_Append(triggered_indices, v) < 0) {
                        Py_XDECREF(v);
                        Py_CLEAR(triggered_indices);
                        break;
                    }
                    Py_DECREF(v);
                }
            }
        }
    }

    for (i = 0; i < thread_count; ++i) {
        PyMem_Free((void *)thread_data[i]);
    }

    return triggered_indices;

error:
    // We should only enter here before any threads start running.
    // Once we start resuming threads, different cleanup is required
    CloseHandle(cancel_event);
    while (--thread_count >= 0) {
        HANDLE t = thread_data[thread_count]->thread;
        if (t) {
            TerminateThread(t, WAIT_ABANDONED_0);
            CloseHandle(t);
        }
        PyMem_Free((void *)thread_data[thread_count]);
    }
    return NULL;
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
    if (nhandles < 0 || nhandles > MAXIMUM_WAIT_OBJECTS - 1) {
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
    overlapped as use_overlapped: bool = False
[clinic start generated code]*/

static PyObject *
_winapi_WriteFile_impl(PyObject *module, HANDLE handle, PyObject *buffer,
                       int use_overlapped)
/*[clinic end generated code: output=2ca80f6bf3fa92e3 input=2badb008c8a2e2a0]*/
{
    Py_buffer _buf, *buf;
    DWORD len, written;
    BOOL ret;
    DWORD err;
    OverlappedObject *overlapped = NULL;

    if (use_overlapped) {
        overlapped = new_overlapped(module, handle);
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


/*[clinic input]
_winapi._mimetypes_read_windows_registry

    on_type_read: object

Optimized function for reading all known MIME types from the registry.

*on_type_read* is a callable taking *type* and *ext* arguments, as for
MimeTypes.add_type.
[clinic start generated code]*/

static PyObject *
_winapi__mimetypes_read_windows_registry_impl(PyObject *module,
                                              PyObject *on_type_read)
/*[clinic end generated code: output=20829f00bebce55b input=cd357896d6501f68]*/
{
#define CCH_EXT 128
#define CB_TYPE 510
    struct {
        wchar_t ext[CCH_EXT];
        wchar_t type[CB_TYPE / sizeof(wchar_t) + 1];
    } entries[64];
    int entry = 0;
    HKEY hkcr = NULL;
    LRESULT err;

    Py_BEGIN_ALLOW_THREADS
    err = RegOpenKeyExW(HKEY_CLASSES_ROOT, NULL, 0, KEY_READ, &hkcr);
    for (DWORD i = 0; err == ERROR_SUCCESS || err == ERROR_MORE_DATA; ++i) {
        LPWSTR ext = entries[entry].ext;
        LPWSTR type = entries[entry].type;
        DWORD cchExt = CCH_EXT;
        DWORD cbType = CB_TYPE;
        HKEY subkey;
        DWORD regType;

        err = RegEnumKeyExW(hkcr, i, ext, &cchExt, NULL, NULL, NULL, NULL);
        if (err != ERROR_SUCCESS || (cchExt && ext[0] != L'.')) {
            continue;
        }

        err = RegOpenKeyExW(hkcr, ext, 0, KEY_READ, &subkey);
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_ACCESS_DENIED) {
            err = ERROR_SUCCESS;
            continue;
        } else if (err != ERROR_SUCCESS) {
            continue;
        }

        err = RegQueryValueExW(subkey, L"Content Type", NULL,
                              &regType, (LPBYTE)type, &cbType);
        RegCloseKey(subkey);
        if (err == ERROR_FILE_NOT_FOUND) {
            err = ERROR_SUCCESS;
            continue;
        } else if (err != ERROR_SUCCESS) {
            continue;
        } else if (regType != REG_SZ || !cbType) {
            continue;
        }
        type[cbType / sizeof(wchar_t)] = L'\0';

        entry += 1;

        /* Flush our cached entries if we are full */
        if (entry == sizeof(entries) / sizeof(entries[0])) {
            Py_BLOCK_THREADS
            for (int j = 0; j < entry; ++j) {
                PyObject *r = PyObject_CallFunction(
                    on_type_read, "uu", entries[j].type, entries[j].ext
                );
                if (!r) {
                    /* We blocked threads, so safe to return from here */
                    RegCloseKey(hkcr);
                    return NULL;
                }
                Py_DECREF(r);
            }
            Py_UNBLOCK_THREADS
            entry = 0;
        }
    }
    if (hkcr) {
        RegCloseKey(hkcr);
    }
    Py_END_ALLOW_THREADS

    if (err != ERROR_SUCCESS && err != ERROR_NO_MORE_ITEMS) {
        PyErr_SetFromWindowsErr((int)err);
        return NULL;
    }

    for (int j = 0; j < entry; ++j) {
        PyObject *r = PyObject_CallFunction(
            on_type_read, "uu", entries[j].type, entries[j].ext
        );
        if (!r) {
            return NULL;
        }
        Py_DECREF(r);
    }

    Py_RETURN_NONE;
#undef CCH_EXT
#undef CB_TYPE
}

/*[clinic input]
_winapi.NeedCurrentDirectoryForExePath -> bool

    exe_name: LPCWSTR
    /
[clinic start generated code]*/

static int
_winapi_NeedCurrentDirectoryForExePath_impl(PyObject *module,
                                            LPCWSTR exe_name)
/*[clinic end generated code: output=a65ec879502b58fc input=972aac88a1ec2f00]*/
{
    BOOL result;

    Py_BEGIN_ALLOW_THREADS
    result = NeedCurrentDirectoryForExePathW(exe_name);
    Py_END_ALLOW_THREADS

    return result;
}


/*[clinic input]
_winapi.CopyFile2

    existing_file_name: LPCWSTR
    new_file_name: LPCWSTR
    flags: DWORD
    progress_routine: object = None

Copies a file from one name to a new name.

This is implemented using the CopyFile2 API, which preserves all stat
and metadata information apart from security attributes.

progress_routine is reserved for future use, but is currently not
implemented. Its value is ignored.
[clinic start generated code]*/

static PyObject *
_winapi_CopyFile2_impl(PyObject *module, LPCWSTR existing_file_name,
                       LPCWSTR new_file_name, DWORD flags,
                       PyObject *progress_routine)
/*[clinic end generated code: output=43d960d9df73d984 input=fb976b8d1492d130]*/
{
    HRESULT hr;
    COPYFILE2_EXTENDED_PARAMETERS params = { sizeof(COPYFILE2_EXTENDED_PARAMETERS) };

    if (PySys_Audit("_winapi.CopyFile2", "uuk",
                    existing_file_name, new_file_name, flags) < 0) {
        return NULL;
    }

    params.dwCopyFlags = flags;
    /* For future implementation. We ignore the value for now so that
       users only have to test for 'CopyFile2' existing and not whether
       the additional parameter exists.
    if (progress_routine != Py_None) {
        params.pProgressRoutine = _winapi_CopyFile2ProgressRoutine;
        params.pvCallbackContext = Py_NewRef(progress_routine);
    }
    */
    Py_BEGIN_ALLOW_THREADS;
    hr = CopyFile2(existing_file_name, new_file_name, &params);
    Py_END_ALLOW_THREADS;
    /* For future implementation.
    if (progress_routine != Py_None) {
        Py_DECREF(progress_routine);
    }
    */
    if (FAILED(hr)) {
        if ((hr & 0xFFFF0000) == 0x80070000) {
            PyErr_SetFromWindowsErr(hr & 0xFFFF);
        } else {
            PyErr_SetFromWindowsErr(hr);
        }
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef winapi_functions[] = {
    _WINAPI_CLOSEHANDLE_METHODDEF
    _WINAPI_CONNECTNAMEDPIPE_METHODDEF
    _WINAPI_CREATEEVENTW_METHODDEF
    _WINAPI_CREATEFILE_METHODDEF
    _WINAPI_CREATEFILEMAPPING_METHODDEF
    _WINAPI_CREATEMUTEXW_METHODDEF
    _WINAPI_CREATENAMEDPIPE_METHODDEF
    _WINAPI_CREATEPIPE_METHODDEF
    _WINAPI_CREATEPROCESS_METHODDEF
    _WINAPI_CREATEJUNCTION_METHODDEF
    _WINAPI_DUPLICATEHANDLE_METHODDEF
    _WINAPI_EXITPROCESS_METHODDEF
    _WINAPI_GETCURRENTPROCESS_METHODDEF
    _WINAPI_GETEXITCODEPROCESS_METHODDEF
    _WINAPI_GETLASTERROR_METHODDEF
    _WINAPI_GETLONGPATHNAME_METHODDEF
    _WINAPI_GETMODULEFILENAME_METHODDEF
    _WINAPI_GETSHORTPATHNAME_METHODDEF
    _WINAPI_GETSTDHANDLE_METHODDEF
    _WINAPI_GETVERSION_METHODDEF
    _WINAPI_MAPVIEWOFFILE_METHODDEF
    _WINAPI_OPENEVENTW_METHODDEF
    _WINAPI_OPENFILEMAPPING_METHODDEF
    _WINAPI_OPENMUTEXW_METHODDEF
    _WINAPI_OPENPROCESS_METHODDEF
    _WINAPI_PEEKNAMEDPIPE_METHODDEF
    _WINAPI_LCMAPSTRINGEX_METHODDEF
    _WINAPI_READFILE_METHODDEF
    _WINAPI_RELEASEMUTEX_METHODDEF
    _WINAPI_RESETEVENT_METHODDEF
    _WINAPI_SETEVENT_METHODDEF
    _WINAPI_SETNAMEDPIPEHANDLESTATE_METHODDEF
    _WINAPI_TERMINATEPROCESS_METHODDEF
    _WINAPI_UNMAPVIEWOFFILE_METHODDEF
    _WINAPI_VIRTUALQUERYSIZE_METHODDEF
    _WINAPI_WAITNAMEDPIPE_METHODDEF
    _WINAPI_WAITFORMULTIPLEOBJECTS_METHODDEF
    _WINAPI_BATCHEDWAITFORMULTIPLEOBJECTS_METHODDEF
    _WINAPI_WAITFORSINGLEOBJECT_METHODDEF
    _WINAPI_WRITEFILE_METHODDEF
    _WINAPI_GETACP_METHODDEF
    _WINAPI_GETFILETYPE_METHODDEF
    _WINAPI__MIMETYPES_READ_WINDOWS_REGISTRY_METHODDEF
    _WINAPI_NEEDCURRENTDIRECTORYFOREXEPATH_METHODDEF
    _WINAPI_COPYFILE2_METHODDEF
    {NULL, NULL}
};

#define WINAPI_CONSTANT(fmt, con) \
    do { \
        PyObject *value = Py_BuildValue(fmt, con); \
        if (value == NULL) { \
            return -1; \
        } \
        if (PyDict_SetItemString(d, #con, value) < 0) { \
            Py_DECREF(value); \
            return -1; \
        } \
        Py_DECREF(value); \
    } while (0)

static int winapi_exec(PyObject *m)
{
    WinApiState *st = winapi_get_state(m);

    st->overlapped_type = (PyTypeObject *)PyType_FromModuleAndSpec(m, &winapi_overlapped_type_spec, NULL);
    if (st->overlapped_type == NULL) {
        return -1;
    }

    if (PyModule_AddType(m, st->overlapped_type) < 0) {
        return -1;
    }

    PyObject *d = PyModule_GetDict(m);

    /* constants */
    WINAPI_CONSTANT(F_DWORD, CREATE_NEW_CONSOLE);
    WINAPI_CONSTANT(F_DWORD, CREATE_NEW_PROCESS_GROUP);
    WINAPI_CONSTANT(F_DWORD, DUPLICATE_SAME_ACCESS);
    WINAPI_CONSTANT(F_DWORD, DUPLICATE_CLOSE_SOURCE);
    WINAPI_CONSTANT(F_DWORD, ERROR_ACCESS_DENIED);
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
    WINAPI_CONSTANT(F_DWORD, ERROR_PRIVILEGE_NOT_HELD);
    WINAPI_CONSTANT(F_DWORD, ERROR_SEM_TIMEOUT);
    WINAPI_CONSTANT(F_DWORD, FILE_FLAG_FIRST_PIPE_INSTANCE);
    WINAPI_CONSTANT(F_DWORD, FILE_FLAG_OVERLAPPED);
    WINAPI_CONSTANT(F_DWORD, FILE_GENERIC_READ);
    WINAPI_CONSTANT(F_DWORD, FILE_GENERIC_WRITE);
    WINAPI_CONSTANT(F_DWORD, FILE_MAP_ALL_ACCESS);
    WINAPI_CONSTANT(F_DWORD, FILE_MAP_COPY);
    WINAPI_CONSTANT(F_DWORD, FILE_MAP_EXECUTE);
    WINAPI_CONSTANT(F_DWORD, FILE_MAP_READ);
    WINAPI_CONSTANT(F_DWORD, FILE_MAP_WRITE);
    WINAPI_CONSTANT(F_DWORD, GENERIC_READ);
    WINAPI_CONSTANT(F_DWORD, GENERIC_WRITE);
    WINAPI_CONSTANT(F_DWORD, INFINITE);
    WINAPI_CONSTANT(F_HANDLE, INVALID_HANDLE_VALUE);
    WINAPI_CONSTANT(F_DWORD, MEM_COMMIT);
    WINAPI_CONSTANT(F_DWORD, MEM_FREE);
    WINAPI_CONSTANT(F_DWORD, MEM_IMAGE);
    WINAPI_CONSTANT(F_DWORD, MEM_MAPPED);
    WINAPI_CONSTANT(F_DWORD, MEM_PRIVATE);
    WINAPI_CONSTANT(F_DWORD, MEM_RESERVE);
    WINAPI_CONSTANT(F_DWORD, NMPWAIT_WAIT_FOREVER);
    WINAPI_CONSTANT(F_DWORD, OPEN_EXISTING);
    WINAPI_CONSTANT(F_DWORD, PAGE_EXECUTE);
    WINAPI_CONSTANT(F_DWORD, PAGE_EXECUTE_READ);
    WINAPI_CONSTANT(F_DWORD, PAGE_EXECUTE_READWRITE);
    WINAPI_CONSTANT(F_DWORD, PAGE_EXECUTE_WRITECOPY);
    WINAPI_CONSTANT(F_DWORD, PAGE_GUARD);
    WINAPI_CONSTANT(F_DWORD, PAGE_NOACCESS);
    WINAPI_CONSTANT(F_DWORD, PAGE_NOCACHE);
    WINAPI_CONSTANT(F_DWORD, PAGE_READONLY);
    WINAPI_CONSTANT(F_DWORD, PAGE_READWRITE);
    WINAPI_CONSTANT(F_DWORD, PAGE_WRITECOMBINE);
    WINAPI_CONSTANT(F_DWORD, PAGE_WRITECOPY);
    WINAPI_CONSTANT(F_DWORD, PIPE_ACCESS_DUPLEX);
    WINAPI_CONSTANT(F_DWORD, PIPE_ACCESS_INBOUND);
    WINAPI_CONSTANT(F_DWORD, PIPE_READMODE_MESSAGE);
    WINAPI_CONSTANT(F_DWORD, PIPE_TYPE_MESSAGE);
    WINAPI_CONSTANT(F_DWORD, PIPE_UNLIMITED_INSTANCES);
    WINAPI_CONSTANT(F_DWORD, PIPE_WAIT);
    WINAPI_CONSTANT(F_DWORD, PROCESS_ALL_ACCESS);
    WINAPI_CONSTANT(F_DWORD, SYNCHRONIZE);
    WINAPI_CONSTANT(F_DWORD, PROCESS_DUP_HANDLE);
    WINAPI_CONSTANT(F_DWORD, SEC_COMMIT);
    WINAPI_CONSTANT(F_DWORD, SEC_IMAGE);
    WINAPI_CONSTANT(F_DWORD, SEC_LARGE_PAGES);
    WINAPI_CONSTANT(F_DWORD, SEC_NOCACHE);
    WINAPI_CONSTANT(F_DWORD, SEC_RESERVE);
    WINAPI_CONSTANT(F_DWORD, SEC_WRITECOMBINE);
    WINAPI_CONSTANT(F_DWORD, STARTF_USESHOWWINDOW);
    WINAPI_CONSTANT(F_DWORD, STARTF_USESIZE);
    WINAPI_CONSTANT(F_DWORD, STARTF_USEPOSITION);
    WINAPI_CONSTANT(F_DWORD, STARTF_USECOUNTCHARS);
    WINAPI_CONSTANT(F_DWORD, STARTF_USEFILLATTRIBUTE);
    WINAPI_CONSTANT(F_DWORD, STARTF_RUNFULLSCREEN);
    WINAPI_CONSTANT(F_DWORD, STARTF_FORCEONFEEDBACK);
    WINAPI_CONSTANT(F_DWORD, STARTF_FORCEOFFFEEDBACK);
    WINAPI_CONSTANT(F_DWORD, STARTF_USESTDHANDLES);
    WINAPI_CONSTANT(F_DWORD, STARTF_USEHOTKEY);
    WINAPI_CONSTANT(F_DWORD, STARTF_TITLEISLINKNAME);
    WINAPI_CONSTANT(F_DWORD, STARTF_TITLEISAPPID);
    WINAPI_CONSTANT(F_DWORD, STARTF_PREVENTPINNING);
    WINAPI_CONSTANT(F_DWORD, STARTF_UNTRUSTEDSOURCE);
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

    WINAPI_CONSTANT("u", LOCALE_NAME_INVARIANT);
    WINAPI_CONSTANT(F_DWORD, LOCALE_NAME_MAX_LENGTH);
    WINAPI_CONSTANT("u", LOCALE_NAME_SYSTEM_DEFAULT);
    WINAPI_CONSTANT("u", LOCALE_NAME_USER_DEFAULT);

    WINAPI_CONSTANT(F_DWORD, LCMAP_FULLWIDTH);
    WINAPI_CONSTANT(F_DWORD, LCMAP_HALFWIDTH);
    WINAPI_CONSTANT(F_DWORD, LCMAP_HIRAGANA);
    WINAPI_CONSTANT(F_DWORD, LCMAP_KATAKANA);
    WINAPI_CONSTANT(F_DWORD, LCMAP_LINGUISTIC_CASING);
    WINAPI_CONSTANT(F_DWORD, LCMAP_LOWERCASE);
    WINAPI_CONSTANT(F_DWORD, LCMAP_SIMPLIFIED_CHINESE);
    WINAPI_CONSTANT(F_DWORD, LCMAP_TITLECASE);
    WINAPI_CONSTANT(F_DWORD, LCMAP_TRADITIONAL_CHINESE);
    WINAPI_CONSTANT(F_DWORD, LCMAP_UPPERCASE);

    WINAPI_CONSTANT(F_DWORD, COPY_FILE_ALLOW_DECRYPTED_DESTINATION);
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_COPY_SYMLINK);
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_FAIL_IF_EXISTS);
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_NO_BUFFERING);
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_NO_OFFLOAD);
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_OPEN_SOURCE_FOR_WRITE);
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_RESTARTABLE);
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_REQUEST_SECURITY_PRIVILEGES);
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_RESUME_FROM_PAUSE);
#ifndef COPY_FILE_REQUEST_COMPRESSED_TRAFFIC
    // Only defined in newer WinSDKs
    #define COPY_FILE_REQUEST_COMPRESSED_TRAFFIC 0x10000000
#endif
    WINAPI_CONSTANT(F_DWORD, COPY_FILE_REQUEST_COMPRESSED_TRAFFIC);

    WINAPI_CONSTANT(F_DWORD, COPYFILE2_CALLBACK_CHUNK_STARTED);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_CALLBACK_CHUNK_FINISHED);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_CALLBACK_STREAM_STARTED);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_CALLBACK_STREAM_FINISHED);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_CALLBACK_POLL_CONTINUE);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_CALLBACK_ERROR);

    WINAPI_CONSTANT(F_DWORD, COPYFILE2_PROGRESS_CONTINUE);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_PROGRESS_CANCEL);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_PROGRESS_STOP);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_PROGRESS_QUIET);
    WINAPI_CONSTANT(F_DWORD, COPYFILE2_PROGRESS_PAUSE);

    WINAPI_CONSTANT("i", NULL);

    return 0;
}

static PyModuleDef_Slot winapi_slots[] = {
    {Py_mod_exec, winapi_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static int
winapi_traverse(PyObject *module, visitproc visit, void *arg)
{
    WinApiState *st = winapi_get_state(module);
    Py_VISIT(st->overlapped_type);
    return 0;
}

static int
winapi_clear(PyObject *module)
{
    WinApiState *st = winapi_get_state(module);
    Py_CLEAR(st->overlapped_type);
    return 0;
}

static void
winapi_free(void *module)
{
    winapi_clear((PyObject *)module);
}

static struct PyModuleDef winapi_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_winapi",
    .m_size = sizeof(WinApiState),
    .m_methods = winapi_functions,
    .m_slots = winapi_slots,
    .m_traverse = winapi_traverse,
    .m_clear = winapi_clear,
    .m_free = winapi_free,
};

PyMODINIT_FUNC
PyInit__winapi(void)
{
    return PyModuleDef_Init(&winapi_module);
}
