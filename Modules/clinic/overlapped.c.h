/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_long.h"          // _PyLong_UnsignedLong_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_overlapped_CreateIoCompletionPort__doc__,
"CreateIoCompletionPort($module, handle, port, key, concurrency, /)\n"
"--\n"
"\n"
"Create a completion port or register a handle with a port.");

#define _OVERLAPPED_CREATEIOCOMPLETIONPORT_METHODDEF    \
    {"CreateIoCompletionPort", _PyCFunction_CAST(_overlapped_CreateIoCompletionPort), METH_FASTCALL, _overlapped_CreateIoCompletionPort__doc__},

static PyObject *
_overlapped_CreateIoCompletionPort_impl(PyObject *module, HANDLE FileHandle,
                                        HANDLE ExistingCompletionPort,
                                        ULONG_PTR CompletionKey,
                                        DWORD NumberOfConcurrentThreads);

static PyObject *
_overlapped_CreateIoCompletionPort(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE FileHandle;
    HANDLE ExistingCompletionPort;
    ULONG_PTR CompletionKey;
    DWORD NumberOfConcurrentThreads;

    if (!_PyArg_CheckPositional("CreateIoCompletionPort", nargs, 4, 4)) {
        goto exit;
    }
    FileHandle = PyLong_AsVoidPtr(args[0]);
    if (!FileHandle && PyErr_Occurred()) {
        goto exit;
    }
    ExistingCompletionPort = PyLong_AsVoidPtr(args[1]);
    if (!ExistingCompletionPort && PyErr_Occurred()) {
        goto exit;
    }
    CompletionKey = (uintptr_t)PyLong_AsVoidPtr(args[2]);
    if (!CompletionKey && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[3], &NumberOfConcurrentThreads)) {
        goto exit;
    }
    return_value = _overlapped_CreateIoCompletionPort_impl(module, FileHandle, ExistingCompletionPort, CompletionKey, NumberOfConcurrentThreads);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_GetQueuedCompletionStatus__doc__,
"GetQueuedCompletionStatus($module, port, msecs, /)\n"
"--\n"
"\n"
"Get a message from completion port.\n"
"\n"
"Wait for up to msecs milliseconds.");

#define _OVERLAPPED_GETQUEUEDCOMPLETIONSTATUS_METHODDEF    \
    {"GetQueuedCompletionStatus", _PyCFunction_CAST(_overlapped_GetQueuedCompletionStatus), METH_FASTCALL, _overlapped_GetQueuedCompletionStatus__doc__},

static PyObject *
_overlapped_GetQueuedCompletionStatus_impl(PyObject *module,
                                           HANDLE CompletionPort,
                                           DWORD Milliseconds);

static PyObject *
_overlapped_GetQueuedCompletionStatus(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE CompletionPort;
    DWORD Milliseconds;

    if (!_PyArg_CheckPositional("GetQueuedCompletionStatus", nargs, 2, 2)) {
        goto exit;
    }
    CompletionPort = PyLong_AsVoidPtr(args[0]);
    if (!CompletionPort && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[1], &Milliseconds)) {
        goto exit;
    }
    return_value = _overlapped_GetQueuedCompletionStatus_impl(module, CompletionPort, Milliseconds);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_PostQueuedCompletionStatus__doc__,
"PostQueuedCompletionStatus($module, port, bytes, key, address, /)\n"
"--\n"
"\n"
"Post a message to completion port.");

#define _OVERLAPPED_POSTQUEUEDCOMPLETIONSTATUS_METHODDEF    \
    {"PostQueuedCompletionStatus", _PyCFunction_CAST(_overlapped_PostQueuedCompletionStatus), METH_FASTCALL, _overlapped_PostQueuedCompletionStatus__doc__},

static PyObject *
_overlapped_PostQueuedCompletionStatus_impl(PyObject *module,
                                            HANDLE CompletionPort,
                                            DWORD NumberOfBytes,
                                            ULONG_PTR CompletionKey,
                                            OVERLAPPED *Overlapped);

static PyObject *
_overlapped_PostQueuedCompletionStatus(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE CompletionPort;
    DWORD NumberOfBytes;
    ULONG_PTR CompletionKey;
    OVERLAPPED *Overlapped;

    if (!_PyArg_CheckPositional("PostQueuedCompletionStatus", nargs, 4, 4)) {
        goto exit;
    }
    CompletionPort = PyLong_AsVoidPtr(args[0]);
    if (!CompletionPort && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[1], &NumberOfBytes)) {
        goto exit;
    }
    CompletionKey = (uintptr_t)PyLong_AsVoidPtr(args[2]);
    if (!CompletionKey && PyErr_Occurred()) {
        goto exit;
    }
    Overlapped = PyLong_AsVoidPtr(args[3]);
    if (!Overlapped && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _overlapped_PostQueuedCompletionStatus_impl(module, CompletionPort, NumberOfBytes, CompletionKey, Overlapped);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_RegisterWaitWithQueue__doc__,
"RegisterWaitWithQueue($module, Object, CompletionPort, Overlapped,\n"
"                      Timeout, /)\n"
"--\n"
"\n"
"Register wait for Object; when complete CompletionPort is notified.");

#define _OVERLAPPED_REGISTERWAITWITHQUEUE_METHODDEF    \
    {"RegisterWaitWithQueue", _PyCFunction_CAST(_overlapped_RegisterWaitWithQueue), METH_FASTCALL, _overlapped_RegisterWaitWithQueue__doc__},

static PyObject *
_overlapped_RegisterWaitWithQueue_impl(PyObject *module, HANDLE Object,
                                       HANDLE CompletionPort,
                                       OVERLAPPED *Overlapped,
                                       DWORD Milliseconds);

static PyObject *
_overlapped_RegisterWaitWithQueue(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE Object;
    HANDLE CompletionPort;
    OVERLAPPED *Overlapped;
    DWORD Milliseconds;

    if (!_PyArg_CheckPositional("RegisterWaitWithQueue", nargs, 4, 4)) {
        goto exit;
    }
    Object = PyLong_AsVoidPtr(args[0]);
    if (!Object && PyErr_Occurred()) {
        goto exit;
    }
    CompletionPort = PyLong_AsVoidPtr(args[1]);
    if (!CompletionPort && PyErr_Occurred()) {
        goto exit;
    }
    Overlapped = PyLong_AsVoidPtr(args[2]);
    if (!Overlapped && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[3], &Milliseconds)) {
        goto exit;
    }
    return_value = _overlapped_RegisterWaitWithQueue_impl(module, Object, CompletionPort, Overlapped, Milliseconds);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_UnregisterWait__doc__,
"UnregisterWait($module, WaitHandle, /)\n"
"--\n"
"\n"
"Unregister wait handle.");

#define _OVERLAPPED_UNREGISTERWAIT_METHODDEF    \
    {"UnregisterWait", (PyCFunction)_overlapped_UnregisterWait, METH_O, _overlapped_UnregisterWait__doc__},

static PyObject *
_overlapped_UnregisterWait_impl(PyObject *module, HANDLE WaitHandle);

static PyObject *
_overlapped_UnregisterWait(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HANDLE WaitHandle;

    WaitHandle = PyLong_AsVoidPtr(arg);
    if (!WaitHandle && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _overlapped_UnregisterWait_impl(module, WaitHandle);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_UnregisterWaitEx__doc__,
"UnregisterWaitEx($module, WaitHandle, Event, /)\n"
"--\n"
"\n"
"Unregister wait handle.");

#define _OVERLAPPED_UNREGISTERWAITEX_METHODDEF    \
    {"UnregisterWaitEx", _PyCFunction_CAST(_overlapped_UnregisterWaitEx), METH_FASTCALL, _overlapped_UnregisterWaitEx__doc__},

static PyObject *
_overlapped_UnregisterWaitEx_impl(PyObject *module, HANDLE WaitHandle,
                                  HANDLE Event);

static PyObject *
_overlapped_UnregisterWaitEx(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE WaitHandle;
    HANDLE Event;

    if (!_PyArg_CheckPositional("UnregisterWaitEx", nargs, 2, 2)) {
        goto exit;
    }
    WaitHandle = PyLong_AsVoidPtr(args[0]);
    if (!WaitHandle && PyErr_Occurred()) {
        goto exit;
    }
    Event = PyLong_AsVoidPtr(args[1]);
    if (!Event && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _overlapped_UnregisterWaitEx_impl(module, WaitHandle, Event);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_CreateEvent__doc__,
"CreateEvent($module, EventAttributes, ManualReset, InitialState, Name,\n"
"            /)\n"
"--\n"
"\n"
"Create an event.\n"
"\n"
"EventAttributes must be None.");

#define _OVERLAPPED_CREATEEVENT_METHODDEF    \
    {"CreateEvent", _PyCFunction_CAST(_overlapped_CreateEvent), METH_FASTCALL, _overlapped_CreateEvent__doc__},

static PyObject *
_overlapped_CreateEvent_impl(PyObject *module, PyObject *EventAttributes,
                             BOOL ManualReset, BOOL InitialState,
                             const wchar_t *Name);

static PyObject *
_overlapped_CreateEvent(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *EventAttributes;
    BOOL ManualReset;
    BOOL InitialState;
    const wchar_t *Name = NULL;

    if (!_PyArg_CheckPositional("CreateEvent", nargs, 4, 4)) {
        goto exit;
    }
    EventAttributes = args[0];
    ManualReset = PyLong_AsInt(args[1]);
    if (ManualReset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    InitialState = PyLong_AsInt(args[2]);
    if (InitialState == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (args[3] == Py_None) {
        Name = NULL;
    }
    else if (PyUnicode_Check(args[3])) {
        Name = PyUnicode_AsWideCharString(args[3], NULL);
        if (Name == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("CreateEvent", "argument 4", "str or None", args[3]);
        goto exit;
    }
    return_value = _overlapped_CreateEvent_impl(module, EventAttributes, ManualReset, InitialState, Name);

exit:
    /* Cleanup for Name */
    PyMem_Free((void *)Name);

    return return_value;
}

PyDoc_STRVAR(_overlapped_SetEvent__doc__,
"SetEvent($module, Handle, /)\n"
"--\n"
"\n"
"Set event.");

#define _OVERLAPPED_SETEVENT_METHODDEF    \
    {"SetEvent", (PyCFunction)_overlapped_SetEvent, METH_O, _overlapped_SetEvent__doc__},

static PyObject *
_overlapped_SetEvent_impl(PyObject *module, HANDLE Handle);

static PyObject *
_overlapped_SetEvent(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HANDLE Handle;

    Handle = PyLong_AsVoidPtr(arg);
    if (!Handle && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _overlapped_SetEvent_impl(module, Handle);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_ResetEvent__doc__,
"ResetEvent($module, Handle, /)\n"
"--\n"
"\n"
"Reset event.");

#define _OVERLAPPED_RESETEVENT_METHODDEF    \
    {"ResetEvent", (PyCFunction)_overlapped_ResetEvent, METH_O, _overlapped_ResetEvent__doc__},

static PyObject *
_overlapped_ResetEvent_impl(PyObject *module, HANDLE Handle);

static PyObject *
_overlapped_ResetEvent(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HANDLE Handle;

    Handle = PyLong_AsVoidPtr(arg);
    if (!Handle && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _overlapped_ResetEvent_impl(module, Handle);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_BindLocal__doc__,
"BindLocal($module, handle, family, /)\n"
"--\n"
"\n"
"Bind a socket handle to an arbitrary local port.\n"
"\n"
"family should be AF_INET or AF_INET6.");

#define _OVERLAPPED_BINDLOCAL_METHODDEF    \
    {"BindLocal", _PyCFunction_CAST(_overlapped_BindLocal), METH_FASTCALL, _overlapped_BindLocal__doc__},

static PyObject *
_overlapped_BindLocal_impl(PyObject *module, HANDLE Socket, int Family);

static PyObject *
_overlapped_BindLocal(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE Socket;
    int Family;

    if (!_PyArg_CheckPositional("BindLocal", nargs, 2, 2)) {
        goto exit;
    }
    Socket = PyLong_AsVoidPtr(args[0]);
    if (!Socket && PyErr_Occurred()) {
        goto exit;
    }
    Family = PyLong_AsInt(args[1]);
    if (Family == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _overlapped_BindLocal_impl(module, Socket, Family);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_FormatMessage__doc__,
"FormatMessage($module, error_code, /)\n"
"--\n"
"\n"
"Return error message for an error code.");

#define _OVERLAPPED_FORMATMESSAGE_METHODDEF    \
    {"FormatMessage", (PyCFunction)_overlapped_FormatMessage, METH_O, _overlapped_FormatMessage__doc__},

static PyObject *
_overlapped_FormatMessage_impl(PyObject *module, DWORD code);

static PyObject *
_overlapped_FormatMessage(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    DWORD code;

    if (!_PyLong_UnsignedLong_Converter(arg, &code)) {
        goto exit;
    }
    return_value = _overlapped_FormatMessage_impl(module, code);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped__doc__,
"Overlapped(event=_overlapped.INVALID_HANDLE_VALUE)\n"
"--\n"
"\n"
"OVERLAPPED structure wrapper.");

static PyObject *
_overlapped_Overlapped_impl(PyTypeObject *type, HANDLE event);

static PyObject *
_overlapped_Overlapped(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(event), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"event", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Overlapped",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    HANDLE event = INVALID_HANDLE_VALUE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    event = PyLong_AsVoidPtr(fastargs[0]);
    if (!event && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _overlapped_Overlapped_impl(type, event);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_cancel__doc__,
"cancel($self, /)\n"
"--\n"
"\n"
"Cancel overlapped operation.");

#define _OVERLAPPED_OVERLAPPED_CANCEL_METHODDEF    \
    {"cancel", (PyCFunction)_overlapped_Overlapped_cancel, METH_NOARGS, _overlapped_Overlapped_cancel__doc__},

static PyObject *
_overlapped_Overlapped_cancel_impl(OverlappedObject *self);

static PyObject *
_overlapped_Overlapped_cancel(OverlappedObject *self, PyObject *Py_UNUSED(ignored))
{
    return _overlapped_Overlapped_cancel_impl(self);
}

PyDoc_STRVAR(_overlapped_Overlapped_getresult__doc__,
"getresult($self, wait=False, /)\n"
"--\n"
"\n"
"Retrieve result of operation.\n"
"\n"
"If wait is true then it blocks until the operation is finished.  If wait\n"
"is false and the operation is still pending then an error is raised.");

#define _OVERLAPPED_OVERLAPPED_GETRESULT_METHODDEF    \
    {"getresult", _PyCFunction_CAST(_overlapped_Overlapped_getresult), METH_FASTCALL, _overlapped_Overlapped_getresult__doc__},

static PyObject *
_overlapped_Overlapped_getresult_impl(OverlappedObject *self, BOOL wait);

static PyObject *
_overlapped_Overlapped_getresult(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    BOOL wait = FALSE;

    if (!_PyArg_CheckPositional("getresult", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    wait = PyLong_AsInt(args[0]);
    if (wait == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _overlapped_Overlapped_getresult_impl(self, wait);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ReadFile__doc__,
"ReadFile($self, handle, size, /)\n"
"--\n"
"\n"
"Start overlapped read.");

#define _OVERLAPPED_OVERLAPPED_READFILE_METHODDEF    \
    {"ReadFile", _PyCFunction_CAST(_overlapped_Overlapped_ReadFile), METH_FASTCALL, _overlapped_Overlapped_ReadFile__doc__},

static PyObject *
_overlapped_Overlapped_ReadFile_impl(OverlappedObject *self, HANDLE handle,
                                     DWORD size);

static PyObject *
_overlapped_Overlapped_ReadFile(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    DWORD size;

    if (!_PyArg_CheckPositional("ReadFile", nargs, 2, 2)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[1], &size)) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_ReadFile_impl(self, handle, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ReadFileInto__doc__,
"ReadFileInto($self, handle, buf, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_READFILEINTO_METHODDEF    \
    {"ReadFileInto", _PyCFunction_CAST(_overlapped_Overlapped_ReadFileInto), METH_FASTCALL, _overlapped_Overlapped_ReadFileInto__doc__},

static PyObject *
_overlapped_Overlapped_ReadFileInto_impl(OverlappedObject *self,
                                         HANDLE handle, Py_buffer *bufobj);

static PyObject *
_overlapped_Overlapped_ReadFileInto(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};

    if (!_PyArg_CheckPositional("ReadFileInto", nargs, 2, 2)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &bufobj, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_ReadFileInto_impl(self, handle, &bufobj);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSARecv__doc__,
"WSARecv($self, handle, size, flags=0, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_WSARECV_METHODDEF    \
    {"WSARecv", _PyCFunction_CAST(_overlapped_Overlapped_WSARecv), METH_FASTCALL, _overlapped_Overlapped_WSARecv__doc__},

static PyObject *
_overlapped_Overlapped_WSARecv_impl(OverlappedObject *self, HANDLE handle,
                                    DWORD size, DWORD flags);

static PyObject *
_overlapped_Overlapped_WSARecv(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    DWORD size;
    DWORD flags = 0;

    if (!_PyArg_CheckPositional("WSARecv", nargs, 2, 3)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[1], &size)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedLong_Converter(args[2], &flags)) {
        goto exit;
    }
skip_optional:
    return_value = _overlapped_Overlapped_WSARecv_impl(self, handle, size, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSARecvInto__doc__,
"WSARecvInto($self, handle, buf, flags, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_WSARECVINTO_METHODDEF    \
    {"WSARecvInto", _PyCFunction_CAST(_overlapped_Overlapped_WSARecvInto), METH_FASTCALL, _overlapped_Overlapped_WSARecvInto__doc__},

static PyObject *
_overlapped_Overlapped_WSARecvInto_impl(OverlappedObject *self,
                                        HANDLE handle, Py_buffer *bufobj,
                                        DWORD flags);

static PyObject *
_overlapped_Overlapped_WSARecvInto(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    DWORD flags;

    if (!_PyArg_CheckPositional("WSARecvInto", nargs, 3, 3)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &bufobj, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[2], &flags)) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_WSARecvInto_impl(self, handle, &bufobj, flags);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WriteFile__doc__,
"WriteFile($self, handle, buf, /)\n"
"--\n"
"\n"
"Start overlapped write.");

#define _OVERLAPPED_OVERLAPPED_WRITEFILE_METHODDEF    \
    {"WriteFile", _PyCFunction_CAST(_overlapped_Overlapped_WriteFile), METH_FASTCALL, _overlapped_Overlapped_WriteFile__doc__},

static PyObject *
_overlapped_Overlapped_WriteFile_impl(OverlappedObject *self, HANDLE handle,
                                      Py_buffer *bufobj);

static PyObject *
_overlapped_Overlapped_WriteFile(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};

    if (!_PyArg_CheckPositional("WriteFile", nargs, 2, 2)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &bufobj, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_WriteFile_impl(self, handle, &bufobj);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSASend__doc__,
"WSASend($self, handle, buf, flags, /)\n"
"--\n"
"\n"
"Start overlapped send.");

#define _OVERLAPPED_OVERLAPPED_WSASEND_METHODDEF    \
    {"WSASend", _PyCFunction_CAST(_overlapped_Overlapped_WSASend), METH_FASTCALL, _overlapped_Overlapped_WSASend__doc__},

static PyObject *
_overlapped_Overlapped_WSASend_impl(OverlappedObject *self, HANDLE handle,
                                    Py_buffer *bufobj, DWORD flags);

static PyObject *
_overlapped_Overlapped_WSASend(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    DWORD flags;

    if (!_PyArg_CheckPositional("WSASend", nargs, 3, 3)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &bufobj, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[2], &flags)) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_WSASend_impl(self, handle, &bufobj, flags);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_AcceptEx__doc__,
"AcceptEx($self, listen_handle, accept_handle, /)\n"
"--\n"
"\n"
"Start overlapped wait for client to connect.");

#define _OVERLAPPED_OVERLAPPED_ACCEPTEX_METHODDEF    \
    {"AcceptEx", _PyCFunction_CAST(_overlapped_Overlapped_AcceptEx), METH_FASTCALL, _overlapped_Overlapped_AcceptEx__doc__},

static PyObject *
_overlapped_Overlapped_AcceptEx_impl(OverlappedObject *self,
                                     HANDLE ListenSocket,
                                     HANDLE AcceptSocket);

static PyObject *
_overlapped_Overlapped_AcceptEx(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE ListenSocket;
    HANDLE AcceptSocket;

    if (!_PyArg_CheckPositional("AcceptEx", nargs, 2, 2)) {
        goto exit;
    }
    ListenSocket = PyLong_AsVoidPtr(args[0]);
    if (!ListenSocket && PyErr_Occurred()) {
        goto exit;
    }
    AcceptSocket = PyLong_AsVoidPtr(args[1]);
    if (!AcceptSocket && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_AcceptEx_impl(self, ListenSocket, AcceptSocket);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ConnectEx__doc__,
"ConnectEx($self, client_handle, address_as_bytes, /)\n"
"--\n"
"\n"
"Start overlapped connect.\n"
"\n"
"client_handle should be unbound.");

#define _OVERLAPPED_OVERLAPPED_CONNECTEX_METHODDEF    \
    {"ConnectEx", _PyCFunction_CAST(_overlapped_Overlapped_ConnectEx), METH_FASTCALL, _overlapped_Overlapped_ConnectEx__doc__},

static PyObject *
_overlapped_Overlapped_ConnectEx_impl(OverlappedObject *self,
                                      HANDLE ConnectSocket,
                                      PyObject *AddressObj);

static PyObject *
_overlapped_Overlapped_ConnectEx(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE ConnectSocket;
    PyObject *AddressObj;

    if (!_PyArg_CheckPositional("ConnectEx", nargs, 2, 2)) {
        goto exit;
    }
    ConnectSocket = PyLong_AsVoidPtr(args[0]);
    if (!ConnectSocket && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyTuple_Check(args[1])) {
        _PyArg_BadArgument("ConnectEx", "argument 2", "tuple", args[1]);
        goto exit;
    }
    AddressObj = args[1];
    return_value = _overlapped_Overlapped_ConnectEx_impl(self, ConnectSocket, AddressObj);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_DisconnectEx__doc__,
"DisconnectEx($self, handle, flags, /)\n"
"--\n"
"\n");

#define _OVERLAPPED_OVERLAPPED_DISCONNECTEX_METHODDEF    \
    {"DisconnectEx", _PyCFunction_CAST(_overlapped_Overlapped_DisconnectEx), METH_FASTCALL, _overlapped_Overlapped_DisconnectEx__doc__},

static PyObject *
_overlapped_Overlapped_DisconnectEx_impl(OverlappedObject *self,
                                         HANDLE Socket, DWORD flags);

static PyObject *
_overlapped_Overlapped_DisconnectEx(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE Socket;
    DWORD flags;

    if (!_PyArg_CheckPositional("DisconnectEx", nargs, 2, 2)) {
        goto exit;
    }
    Socket = PyLong_AsVoidPtr(args[0]);
    if (!Socket && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[1], &flags)) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_DisconnectEx_impl(self, Socket, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_TransmitFile__doc__,
"TransmitFile($self, socket, file, offset, offset_high, count_to_write,\n"
"             count_per_send, flags, /)\n"
"--\n"
"\n"
"Transmit file data over a connected socket.");

#define _OVERLAPPED_OVERLAPPED_TRANSMITFILE_METHODDEF    \
    {"TransmitFile", _PyCFunction_CAST(_overlapped_Overlapped_TransmitFile), METH_FASTCALL, _overlapped_Overlapped_TransmitFile__doc__},

static PyObject *
_overlapped_Overlapped_TransmitFile_impl(OverlappedObject *self,
                                         HANDLE Socket, HANDLE File,
                                         DWORD offset, DWORD offset_high,
                                         DWORD count_to_write,
                                         DWORD count_per_send, DWORD flags);

static PyObject *
_overlapped_Overlapped_TransmitFile(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE Socket;
    HANDLE File;
    DWORD offset;
    DWORD offset_high;
    DWORD count_to_write;
    DWORD count_per_send;
    DWORD flags;

    if (!_PyArg_CheckPositional("TransmitFile", nargs, 7, 7)) {
        goto exit;
    }
    Socket = PyLong_AsVoidPtr(args[0]);
    if (!Socket && PyErr_Occurred()) {
        goto exit;
    }
    File = PyLong_AsVoidPtr(args[1]);
    if (!File && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[2], &offset)) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[3], &offset_high)) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[4], &count_to_write)) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[5], &count_per_send)) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[6], &flags)) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_TransmitFile_impl(self, Socket, File, offset, offset_high, count_to_write, count_per_send, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ConnectNamedPipe__doc__,
"ConnectNamedPipe($self, handle, /)\n"
"--\n"
"\n"
"Start overlapped wait for a client to connect.");

#define _OVERLAPPED_OVERLAPPED_CONNECTNAMEDPIPE_METHODDEF    \
    {"ConnectNamedPipe", (PyCFunction)_overlapped_Overlapped_ConnectNamedPipe, METH_O, _overlapped_Overlapped_ConnectNamedPipe__doc__},

static PyObject *
_overlapped_Overlapped_ConnectNamedPipe_impl(OverlappedObject *self,
                                             HANDLE Pipe);

static PyObject *
_overlapped_Overlapped_ConnectNamedPipe(OverlappedObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    HANDLE Pipe;

    Pipe = PyLong_AsVoidPtr(arg);
    if (!Pipe && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_ConnectNamedPipe_impl(self, Pipe);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ConnectPipe__doc__,
"ConnectPipe($self, addr, /)\n"
"--\n"
"\n"
"Connect to the pipe for asynchronous I/O (overlapped).");

#define _OVERLAPPED_OVERLAPPED_CONNECTPIPE_METHODDEF    \
    {"ConnectPipe", (PyCFunction)_overlapped_Overlapped_ConnectPipe, METH_O, _overlapped_Overlapped_ConnectPipe__doc__},

static PyObject *
_overlapped_Overlapped_ConnectPipe_impl(OverlappedObject *self,
                                        const wchar_t *Address);

static PyObject *
_overlapped_Overlapped_ConnectPipe(OverlappedObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const wchar_t *Address = NULL;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("ConnectPipe", "argument", "str", arg);
        goto exit;
    }
    Address = PyUnicode_AsWideCharString(arg, NULL);
    if (Address == NULL) {
        goto exit;
    }
    return_value = _overlapped_Overlapped_ConnectPipe_impl(self, Address);

exit:
    /* Cleanup for Address */
    PyMem_Free((void *)Address);

    return return_value;
}

PyDoc_STRVAR(_overlapped_WSAConnect__doc__,
"WSAConnect($module, client_handle, address_as_bytes, /)\n"
"--\n"
"\n"
"Bind a remote address to a connectionless (UDP) socket.");

#define _OVERLAPPED_WSACONNECT_METHODDEF    \
    {"WSAConnect", _PyCFunction_CAST(_overlapped_WSAConnect), METH_FASTCALL, _overlapped_WSAConnect__doc__},

static PyObject *
_overlapped_WSAConnect_impl(PyObject *module, HANDLE ConnectSocket,
                            PyObject *AddressObj);

static PyObject *
_overlapped_WSAConnect(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE ConnectSocket;
    PyObject *AddressObj;

    if (!_PyArg_CheckPositional("WSAConnect", nargs, 2, 2)) {
        goto exit;
    }
    ConnectSocket = PyLong_AsVoidPtr(args[0]);
    if (!ConnectSocket && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyTuple_Check(args[1])) {
        _PyArg_BadArgument("WSAConnect", "argument 2", "tuple", args[1]);
        goto exit;
    }
    AddressObj = args[1];
    return_value = _overlapped_WSAConnect_impl(module, ConnectSocket, AddressObj);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSASendTo__doc__,
"WSASendTo($self, handle, buf, flags, address_as_bytes, /)\n"
"--\n"
"\n"
"Start overlapped sendto over a connectionless (UDP) socket.");

#define _OVERLAPPED_OVERLAPPED_WSASENDTO_METHODDEF    \
    {"WSASendTo", _PyCFunction_CAST(_overlapped_Overlapped_WSASendTo), METH_FASTCALL, _overlapped_Overlapped_WSASendTo__doc__},

static PyObject *
_overlapped_Overlapped_WSASendTo_impl(OverlappedObject *self, HANDLE handle,
                                      Py_buffer *bufobj, DWORD flags,
                                      PyObject *AddressObj);

static PyObject *
_overlapped_Overlapped_WSASendTo(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    DWORD flags;
    PyObject *AddressObj;

    if (!_PyArg_CheckPositional("WSASendTo", nargs, 4, 4)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &bufobj, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[2], &flags)) {
        goto exit;
    }
    if (!PyTuple_Check(args[3])) {
        _PyArg_BadArgument("WSASendTo", "argument 4", "tuple", args[3]);
        goto exit;
    }
    AddressObj = args[3];
    return_value = _overlapped_Overlapped_WSASendTo_impl(self, handle, &bufobj, flags, AddressObj);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSARecvFrom__doc__,
"WSARecvFrom($self, handle, size, flags=0, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_WSARECVFROM_METHODDEF    \
    {"WSARecvFrom", _PyCFunction_CAST(_overlapped_Overlapped_WSARecvFrom), METH_FASTCALL, _overlapped_Overlapped_WSARecvFrom__doc__},

static PyObject *
_overlapped_Overlapped_WSARecvFrom_impl(OverlappedObject *self,
                                        HANDLE handle, DWORD size,
                                        DWORD flags);

static PyObject *
_overlapped_Overlapped_WSARecvFrom(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    DWORD size;
    DWORD flags = 0;

    if (!_PyArg_CheckPositional("WSARecvFrom", nargs, 2, 3)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[1], &size)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedLong_Converter(args[2], &flags)) {
        goto exit;
    }
skip_optional:
    return_value = _overlapped_Overlapped_WSARecvFrom_impl(self, handle, size, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSARecvFromInto__doc__,
"WSARecvFromInto($self, handle, buf, size, flags=0, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_WSARECVFROMINTO_METHODDEF    \
    {"WSARecvFromInto", _PyCFunction_CAST(_overlapped_Overlapped_WSARecvFromInto), METH_FASTCALL, _overlapped_Overlapped_WSARecvFromInto__doc__},

static PyObject *
_overlapped_Overlapped_WSARecvFromInto_impl(OverlappedObject *self,
                                            HANDLE handle, Py_buffer *bufobj,
                                            DWORD size, DWORD flags);

static PyObject *
_overlapped_Overlapped_WSARecvFromInto(OverlappedObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    DWORD size;
    DWORD flags = 0;

    if (!_PyArg_CheckPositional("WSARecvFromInto", nargs, 3, 4)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &bufobj, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[2], &size)) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedLong_Converter(args[3], &flags)) {
        goto exit;
    }
skip_optional:
    return_value = _overlapped_Overlapped_WSARecvFromInto_impl(self, handle, &bufobj, size, flags);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}
/*[clinic end generated code: output=958cbddbcc355f47 input=a9049054013a1b77]*/
