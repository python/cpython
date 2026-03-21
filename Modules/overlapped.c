/*
 * Support for overlapped IO
 *
 * Some code borrowed from Modules/_winapi.c of CPython
 */

/* XXX check overflow and DWORD <-> Py_ssize_t conversions
   Check itemsize */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#define WINDOWS_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#if defined(MS_WIN32) && !defined(MS_WIN64)
#  define F_POINTER "k"
#  define T_POINTER Py_T_ULONG
#else
#  define F_POINTER "K"
#  define T_POINTER Py_T_ULONGLONG
#endif

#define F_HANDLE F_POINTER
#define F_ULONG_PTR F_POINTER
#define F_DWORD "k"
#define F_BOOL "i"
#define F_UINT "I"

#define T_HANDLE T_POINTER

/*[python input]
class pointer_converter(CConverter):
    format_unit = '"F_POINTER"'

    def parse_arg(self, argname, displayname, *, limited_capi):
        return self.format_code("""
            {paramname} = PyLong_AsVoidPtr({argname});
            if (!{paramname} && PyErr_Occurred()) {{{{
                goto exit;
            }}}}
            """,
            argname=argname)

class OVERLAPPED_converter(pointer_converter):
    type = 'OVERLAPPED *'

class HANDLE_converter(pointer_converter):
    type = 'HANDLE'

class ULONG_PTR_converter(pointer_converter):
    type = 'ULONG_PTR'

    def parse_arg(self, argname, displayname, *, limited_capi):
        return self.format_code("""
            {paramname} = (uintptr_t)PyLong_AsVoidPtr({argname});
            if (!{paramname} && PyErr_Occurred()) {{{{
                goto exit;
            }}}}
            """,
            argname=argname)

class DWORD_converter(unsigned_long_converter):
    type = 'DWORD'

class BOOL_converter(int_converter):
    type = 'BOOL'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=436f4440630a304c]*/

/*[clinic input]
module _overlapped
class _overlapped.Overlapped "OverlappedObject *" "&OverlappedType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=92e5a799db35b96c]*/


enum {TYPE_NONE, TYPE_NOT_STARTED, TYPE_READ, TYPE_READINTO, TYPE_WRITE,
      TYPE_ACCEPT, TYPE_CONNECT, TYPE_DISCONNECT, TYPE_CONNECT_NAMED_PIPE,
      TYPE_WAIT_NAMED_PIPE_AND_CONNECT, TYPE_TRANSMIT_FILE, TYPE_READ_FROM,
      TYPE_WRITE_TO, TYPE_READ_FROM_INTO};

typedef struct {
    PyObject_HEAD
    OVERLAPPED overlapped;
    /* For convenience, we store the file handle too */
    HANDLE handle;
    /* Error returned by last method call */
    DWORD error;
    /* Type of operation */
    DWORD type;
    union {
        /* Buffer allocated by us: TYPE_READ and TYPE_ACCEPT */
        PyObject *allocated_buffer;
        /* Buffer passed by the user: TYPE_WRITE, TYPE_WRITE_TO, and TYPE_READINTO */
        Py_buffer user_buffer;

        /* Data used for reading from a connectionless socket:
           TYPE_READ_FROM */
        struct {
            // A (buffer, (host, port)) tuple
            PyObject *result;
            // The actual read buffer
            PyObject *allocated_buffer;
            struct sockaddr_in6 address;
            int address_length;
        } read_from;

        /* Data used for reading from a connectionless socket:
           TYPE_READ_FROM_INTO */
        struct {
            // A (number of bytes read, (host, port)) tuple
            PyObject* result;
            /* Buffer passed by the user */
            Py_buffer user_buffer;
            struct sockaddr_in6 address;
            int address_length;
        } read_from_into;
    };
} OverlappedObject;

#define OverlappedObject_CAST(op)   ((OverlappedObject *)(op))


static inline void
steal_buffer(Py_buffer * dst, Py_buffer * src)
{
    memcpy(dst, src, sizeof(Py_buffer));
    memset(src, 0, sizeof(Py_buffer));
}

/*
 * Map Windows error codes to subclasses of OSError
 */

static PyObject *
SetFromWindowsErr(DWORD err)
{
    PyObject *exception_type;

    if (err == 0)
        err = GetLastError();
    switch (err) {
        case ERROR_CONNECTION_REFUSED:
            exception_type = PyExc_ConnectionRefusedError;
            break;
        case ERROR_CONNECTION_ABORTED:
            exception_type = PyExc_ConnectionAbortedError;
            break;
        default:
            exception_type = PyExc_OSError;
    }
    return PyErr_SetExcFromWindowsErr(exception_type, err);
}

/*
 * Some functions should be loaded at runtime
 */

static LPFN_ACCEPTEX Py_AcceptEx = NULL;
static LPFN_CONNECTEX Py_ConnectEx = NULL;
static LPFN_DISCONNECTEX Py_DisconnectEx = NULL;
static LPFN_TRANSMITFILE Py_TransmitFile = NULL;

#define GET_WSA_POINTER(s, x)                                           \
    (SOCKET_ERROR != WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,    \
                              &Guid##x, sizeof(Guid##x), &Py_##x,       \
                              sizeof(Py_##x), &dwBytes, NULL, NULL))

static int
initialize_function_pointers(void)
{
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    GUID GuidConnectEx = WSAID_CONNECTEX;
    GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
    GUID GuidTransmitFile = WSAID_TRANSMITFILE;
    SOCKET s;
    DWORD dwBytes;

    if (Py_AcceptEx != NULL &&
        Py_ConnectEx != NULL &&
        Py_DisconnectEx != NULL &&
        Py_TransmitFile != NULL)
    {
        // All function pointers are initialized already
        // by previous module import
        return 0;
    }

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        SetFromWindowsErr(WSAGetLastError());
        return -1;
    }

    if (!GET_WSA_POINTER(s, AcceptEx) ||
        !GET_WSA_POINTER(s, ConnectEx) ||
        !GET_WSA_POINTER(s, DisconnectEx) ||
        !GET_WSA_POINTER(s, TransmitFile))
    {
        closesocket(s);
        SetFromWindowsErr(WSAGetLastError());
        return -1;
    }

    closesocket(s);
    return 0;
}

/*
 * Completion port stuff
 */

/*[clinic input]
_overlapped.CreateIoCompletionPort

    handle as FileHandle: HANDLE
    port as ExistingCompletionPort: HANDLE
    key as CompletionKey: ULONG_PTR
    concurrency as NumberOfConcurrentThreads: DWORD
    /

Create a completion port or register a handle with a port.
[clinic start generated code]*/

static PyObject *
_overlapped_CreateIoCompletionPort_impl(PyObject *module, HANDLE FileHandle,
                                        HANDLE ExistingCompletionPort,
                                        ULONG_PTR CompletionKey,
                                        DWORD NumberOfConcurrentThreads)
/*[clinic end generated code: output=24ede2b0f05e5433 input=847bae4d0efe1976]*/
{
    HANDLE ret;

    Py_BEGIN_ALLOW_THREADS
    ret = CreateIoCompletionPort(FileHandle, ExistingCompletionPort,
                                 CompletionKey, NumberOfConcurrentThreads);
    Py_END_ALLOW_THREADS

    if (ret == NULL)
        return SetFromWindowsErr(0);
    return Py_BuildValue(F_HANDLE, ret);
}

/*[clinic input]
_overlapped.GetQueuedCompletionStatus

    port as CompletionPort: HANDLE
    msecs as Milliseconds: DWORD
    /

Get a message from completion port.

Wait for up to msecs milliseconds.
[clinic start generated code]*/

static PyObject *
_overlapped_GetQueuedCompletionStatus_impl(PyObject *module,
                                           HANDLE CompletionPort,
                                           DWORD Milliseconds)
/*[clinic end generated code: output=68314171628dddb7 input=94a042d14c4f6410]*/
{
    DWORD NumberOfBytes = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED *Overlapped = NULL;
    DWORD err;
    BOOL ret;

    Py_BEGIN_ALLOW_THREADS
    ret = GetQueuedCompletionStatus(CompletionPort, &NumberOfBytes,
                                    &CompletionKey, &Overlapped, Milliseconds);
    Py_END_ALLOW_THREADS

    err = ret ? ERROR_SUCCESS : GetLastError();
    if (Overlapped == NULL) {
        if (err == WAIT_TIMEOUT)
            Py_RETURN_NONE;
        else
            return SetFromWindowsErr(err);
    }
    return Py_BuildValue(F_DWORD F_DWORD F_ULONG_PTR F_POINTER,
                         err, NumberOfBytes, CompletionKey, Overlapped);
}

/*[clinic input]
_overlapped.PostQueuedCompletionStatus

    port as CompletionPort: HANDLE
    bytes as NumberOfBytes: DWORD
    key as CompletionKey: ULONG_PTR
    address as Overlapped: OVERLAPPED
    /

Post a message to completion port.
[clinic start generated code]*/

static PyObject *
_overlapped_PostQueuedCompletionStatus_impl(PyObject *module,
                                            HANDLE CompletionPort,
                                            DWORD NumberOfBytes,
                                            ULONG_PTR CompletionKey,
                                            OVERLAPPED *Overlapped)
/*[clinic end generated code: output=93e73f2933a43e9e input=e936202d87937aca]*/
{
    BOOL ret;

    Py_BEGIN_ALLOW_THREADS
    ret = PostQueuedCompletionStatus(CompletionPort, NumberOfBytes,
                                     CompletionKey, Overlapped);
    Py_END_ALLOW_THREADS

    if (!ret)
        return SetFromWindowsErr(0);
    Py_RETURN_NONE;
}

/*
 * Wait for a handle
 */

struct PostCallbackData {
    HANDLE CompletionPort;
    LPOVERLAPPED Overlapped;
};

static VOID CALLBACK
PostToQueueCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    struct PostCallbackData *p = (struct PostCallbackData*) lpParameter;

    PostQueuedCompletionStatus(p->CompletionPort, TimerOrWaitFired,
                               0, p->Overlapped);
    /* ignore possible error! */
    PyMem_RawFree(p);
}

/*[clinic input]
_overlapped.RegisterWaitWithQueue

    Object: HANDLE
    CompletionPort: HANDLE
    Overlapped: OVERLAPPED
    Timeout as Milliseconds: DWORD
    /

Register wait for Object; when complete CompletionPort is notified.
[clinic start generated code]*/

static PyObject *
_overlapped_RegisterWaitWithQueue_impl(PyObject *module, HANDLE Object,
                                       HANDLE CompletionPort,
                                       OVERLAPPED *Overlapped,
                                       DWORD Milliseconds)
/*[clinic end generated code: output=c2ace732e447fe45 input=2dd4efee44abe8ee]*/
{
    HANDLE NewWaitObject;
    struct PostCallbackData data = {CompletionPort, Overlapped}, *pdata;

    /* Use PyMem_RawMalloc() rather than PyMem_Malloc(), since
       PostToQueueCallback() will call PyMem_Free() from a new C thread
       which doesn't hold the GIL. */
    pdata = PyMem_RawMalloc(sizeof(struct PostCallbackData));
    if (pdata == NULL)
        return SetFromWindowsErr(0);

    *pdata = data;

    if (!RegisterWaitForSingleObject(
            &NewWaitObject, Object, PostToQueueCallback, pdata, Milliseconds,
            WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE))
    {
        SetFromWindowsErr(0);
        PyMem_RawFree(pdata);
        return NULL;
    }

    return Py_BuildValue(F_HANDLE, NewWaitObject);
}

/*[clinic input]
_overlapped.UnregisterWait

    WaitHandle: HANDLE
    /

Unregister wait handle.
[clinic start generated code]*/

static PyObject *
_overlapped_UnregisterWait_impl(PyObject *module, HANDLE WaitHandle)
/*[clinic end generated code: output=ec90cd955a9a617d input=a56709544cb2df0f]*/
{
    BOOL ret;

    Py_BEGIN_ALLOW_THREADS
    ret = UnregisterWait(WaitHandle);
    Py_END_ALLOW_THREADS

    if (!ret)
        return SetFromWindowsErr(0);
    Py_RETURN_NONE;
}

/*[clinic input]
_overlapped.UnregisterWaitEx

    WaitHandle: HANDLE
    Event: HANDLE
    /

Unregister wait handle.
[clinic start generated code]*/

static PyObject *
_overlapped_UnregisterWaitEx_impl(PyObject *module, HANDLE WaitHandle,
                                  HANDLE Event)
/*[clinic end generated code: output=2e3d84c1d5f65b92 input=953cddc1de50fab9]*/
{
    BOOL ret;

    Py_BEGIN_ALLOW_THREADS
    ret = UnregisterWaitEx(WaitHandle, Event);
    Py_END_ALLOW_THREADS

    if (!ret)
        return SetFromWindowsErr(0);
    Py_RETURN_NONE;
}

/*
 * Event functions -- currently only used by tests
 */

/*[clinic input]
_overlapped.CreateEvent

    EventAttributes: object
    ManualReset: BOOL
    InitialState: BOOL
    Name: Py_UNICODE(accept={str, NoneType})
    /

Create an event.

EventAttributes must be None.
[clinic start generated code]*/

static PyObject *
_overlapped_CreateEvent_impl(PyObject *module, PyObject *EventAttributes,
                             BOOL ManualReset, BOOL InitialState,
                             const wchar_t *Name)
/*[clinic end generated code: output=b17ddc5fd506972d input=dbc36ae14375ba24]*/
{
    HANDLE Event;

    if (EventAttributes != Py_None) {
        PyErr_SetString(PyExc_ValueError, "EventAttributes must be None");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    Event = CreateEventW(NULL, ManualReset, InitialState, Name);
    Py_END_ALLOW_THREADS

    if (Event == NULL)
        return SetFromWindowsErr(0);
    return Py_BuildValue(F_HANDLE, Event);
}

/*[clinic input]
_overlapped.SetEvent

    Handle: HANDLE
    /

Set event.
[clinic start generated code]*/

static PyObject *
_overlapped_SetEvent_impl(PyObject *module, HANDLE Handle)
/*[clinic end generated code: output=5b8d974216b0e569 input=d8b0d26eb7391e80]*/
{
    BOOL ret;

    Py_BEGIN_ALLOW_THREADS
    ret = SetEvent(Handle);
    Py_END_ALLOW_THREADS

    if (!ret)
        return SetFromWindowsErr(0);
    Py_RETURN_NONE;
}

/*[clinic input]
_overlapped.ResetEvent

    Handle: HANDLE
    /

Reset event.
[clinic start generated code]*/

static PyObject *
_overlapped_ResetEvent_impl(PyObject *module, HANDLE Handle)
/*[clinic end generated code: output=066537a8405cddb2 input=d4e089c9ba84ff2f]*/
{
    BOOL ret;

    Py_BEGIN_ALLOW_THREADS
    ret = ResetEvent(Handle);
    Py_END_ALLOW_THREADS

    if (!ret)
        return SetFromWindowsErr(0);
    Py_RETURN_NONE;
}

/*
 * Bind socket handle to local port without doing slow getaddrinfo()
 */

/*[clinic input]
_overlapped.BindLocal

    handle as Socket: HANDLE
    family as Family: int
    /

Bind a socket handle to an arbitrary local port.

family should be AF_INET or AF_INET6.
[clinic start generated code]*/

static PyObject *
_overlapped_BindLocal_impl(PyObject *module, HANDLE Socket, int Family)
/*[clinic end generated code: output=edb93862697aed9c input=a0e7b5c2f541170c]*/
{
    BOOL ret;

    if (Family == AF_INET) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = 0;
        addr.sin_addr.S_un.S_addr = INADDR_ANY;
        ret = bind((SOCKET)Socket, (SOCKADDR*)&addr, sizeof(addr))
                != SOCKET_ERROR;
    } else if (Family == AF_INET6) {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = 0;
        addr.sin6_addr = in6addr_any;
        ret = bind((SOCKET)Socket, (SOCKADDR*)&addr, sizeof(addr))
                != SOCKET_ERROR;
    } else {
        PyErr_SetString(PyExc_ValueError, "Only AF_INET and AF_INET6 families are supported");
        return NULL;
    }

    if (!ret)
        return SetFromWindowsErr(WSAGetLastError());
    Py_RETURN_NONE;
}

/*
 * Windows equivalent of os.strerror() -- compare _ctypes/callproc.c
 */

/*[clinic input]
_overlapped.FormatMessage

    error_code as code: DWORD
    /

Return error message for an error code.
[clinic start generated code]*/

static PyObject *
_overlapped_FormatMessage_impl(PyObject *module, DWORD code)
/*[clinic end generated code: output=02c964ff22407c6b input=644bb5b80326179e]*/
{
    DWORD n;
    WCHAR *lpMsgBuf;
    PyObject *res;

    n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       code,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR) &lpMsgBuf,
                       0,
                       NULL);
    if (n) {
        while (iswspace(lpMsgBuf[n-1]))
            --n;
        res = PyUnicode_FromWideChar(lpMsgBuf, n);
    } else {
        res = PyUnicode_FromFormat("unknown error code %u", code);
    }
    LocalFree(lpMsgBuf);
    return res;
}


/*
 * Mark operation as completed - used when reading produces ERROR_BROKEN_PIPE
 */

static inline void
mark_as_completed(OVERLAPPED *ov)
{
    ov->Internal = 0;
    if (ov->hEvent != NULL)
        SetEvent(ov->hEvent);
}

/*
 * A Python object wrapping an OVERLAPPED structure and other useful data
 * for overlapped I/O
 */

/*[clinic input]
@classmethod
_overlapped.Overlapped.__new__

    event: HANDLE(c_default='INVALID_HANDLE_VALUE') = _overlapped.INVALID_HANDLE_VALUE

OVERLAPPED structure wrapper.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_impl(PyTypeObject *type, HANDLE event)
/*[clinic end generated code: output=6da60504a18eb421 input=26b8a7429e629e95]*/
{
    OverlappedObject *self;

    if (event == INVALID_HANDLE_VALUE) {
        event = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (event == NULL)
            return SetFromWindowsErr(0);
    }

    self = PyObject_New(OverlappedObject, type);
    if (self == NULL) {
        if (event != NULL)
            CloseHandle(event);
        return NULL;
    }

    self->handle = NULL;
    self->error = 0;
    self->type = TYPE_NONE;
    self->allocated_buffer = NULL;
    memset(&self->overlapped, 0, sizeof(OVERLAPPED));
    memset(&self->user_buffer, 0, sizeof(Py_buffer));
    if (event)
        self->overlapped.hEvent = event;
    return (PyObject *)self;
}


/* Note (bpo-32710): OverlappedType.tp_clear is not defined to not release
 * buffers while overlapped are still running, to prevent a crash.
 *
 * Note (gh-111178): Since OverlappedType.tp_clear is not used, we do not
 * need to prevent an undefined behaviour by changing the type of 'self'.
 * To avoid suppressing unused return values, we however make this function
 * return nothing instead of 0, as we never use it.
 */
static void
Overlapped_clear(OverlappedObject *self)
{
    switch (self->type) {
        case TYPE_READ:
        case TYPE_ACCEPT: {
            Py_CLEAR(self->allocated_buffer);
            break;
        }
        case TYPE_READ_FROM: {
            // An initial call to WSARecvFrom will only allocate the buffer.
            // The result tuple of (message, address) is only
            // allocated _after_ a message has been received.
            if(self->read_from.result) {
                // We've received a message, free the result tuple.
                Py_CLEAR(self->read_from.result);
            }
            if(self->read_from.allocated_buffer) {
                Py_CLEAR(self->read_from.allocated_buffer);
            }
            break;
        }
        case TYPE_READ_FROM_INTO: {
            if (self->read_from_into.result) {
                // We've received a message, free the result tuple.
                Py_CLEAR(self->read_from_into.result);
            }
            if (self->read_from_into.user_buffer.obj) {
                PyBuffer_Release(&self->read_from_into.user_buffer);
            }
            break;
        }
        case TYPE_WRITE:
        case TYPE_WRITE_TO:
        case TYPE_READINTO: {
            if (self->user_buffer.obj) {
                PyBuffer_Release(&self->user_buffer);
            }
            break;
        }
    }
    self->type = TYPE_NOT_STARTED;
}

static void
Overlapped_dealloc(PyObject *op)
{
    DWORD bytes;
    DWORD olderr = GetLastError();
    BOOL wait = FALSE;
    BOOL ret;
    OverlappedObject *self = OverlappedObject_CAST(op);

    if (!HasOverlappedIoCompleted(&self->overlapped) &&
        self->type != TYPE_NOT_STARTED)
    {
        // NOTE: We should not get here, if we do then something is wrong in
        // the IocpProactor or ProactorEventLoop. Since everything uses IOCP if
        // the overlapped IO hasn't completed yet then we should not be
        // deallocating!
        //
        // The problem is likely that this OverlappedObject was removed from
        // the IocpProactor._cache before it was complete. The _cache holds a
        // reference while IO is pending so that it does not get deallocated
        // while the kernel has retained the OVERLAPPED structure.
        //
        // CancelIoEx (likely called from self.cancel()) may have successfully
        // completed, but the OVERLAPPED is still in use until either
        // HasOverlappedIoCompleted() is true or GetQueuedCompletionStatus has
        // returned this OVERLAPPED object.
        //
        // NOTE: Waiting when IOCP is in use can hang indefinitely, but this
        // CancelIoEx is superfluous in that self.cancel() was already called,
        // so I've only ever seen this return FALSE with GLE=ERROR_NOT_FOUND
        Py_BEGIN_ALLOW_THREADS
        if (CancelIoEx(self->handle, &self->overlapped))
            wait = TRUE;

        ret = GetOverlappedResult(self->handle, &self->overlapped,
                                  &bytes, wait);
        Py_END_ALLOW_THREADS

        switch (ret ? ERROR_SUCCESS : GetLastError()) {
            case ERROR_SUCCESS:
            case ERROR_NOT_FOUND:
            case ERROR_OPERATION_ABORTED:
                break;
            default:
                PyErr_Format(
                    PyExc_RuntimeError,
                    "%R still has pending operation at "
                    "deallocation, the process may crash", self);
                PyErr_FormatUnraisable("Exception ignored while deallocating "
                                       "overlapped operation %R", self);
        }
    }

    if (self->overlapped.hEvent != NULL) {
        CloseHandle(self->overlapped.hEvent);
    }

    Overlapped_clear(self);
    SetLastError(olderr);

    PyTypeObject *tp = Py_TYPE(self);
    PyObject_Free(self);
    Py_DECREF(tp);
}


/* Convert IPv4 sockaddr to a Python str. */

static PyObject *
make_ipv4_addr(const struct sockaddr_in *addr)
{
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf)) == NULL) {
                PyErr_SetFromErrno(PyExc_OSError);
                return NULL;
        }
        return PyUnicode_FromString(buf);
}

/* Convert IPv6 sockaddr to a Python str. */

static PyObject *
make_ipv6_addr(const struct sockaddr_in6 *addr)
{
        char buf[INET6_ADDRSTRLEN];
        if (inet_ntop(AF_INET6, &addr->sin6_addr, buf, sizeof(buf)) == NULL) {
                PyErr_SetFromErrno(PyExc_OSError);
                return NULL;
        }
        return PyUnicode_FromString(buf);
}

static PyObject*
unparse_address(LPSOCKADDR Address, DWORD Length)
{
        /* The function is adopted from mocketmodule.c makesockaddr()*/

    switch(Address->sa_family) {
        case AF_INET: {
            const struct sockaddr_in *a = (const struct sockaddr_in *)Address;
            PyObject *addrobj = make_ipv4_addr(a);
            PyObject *ret = NULL;
            if (addrobj) {
                ret = Py_BuildValue("Oi", addrobj, ntohs(a->sin_port));
                Py_DECREF(addrobj);
            }
            return ret;
        }
        case AF_INET6: {
            const struct sockaddr_in6 *a = (const struct sockaddr_in6 *)Address;
            PyObject *addrobj = make_ipv6_addr(a);
            PyObject *ret = NULL;
            if (addrobj) {
                ret = Py_BuildValue("OiII",
                                    addrobj,
                                    ntohs(a->sin6_port),
                                    ntohl(a->sin6_flowinfo),
                                    a->sin6_scope_id);
                Py_DECREF(addrobj);
            }
            return ret;
        }
        default: {
            PyErr_SetString(PyExc_ValueError, "recvfrom returned unsupported address family");
            return NULL;
        }
    }
}

/*[clinic input]
_overlapped.Overlapped.cancel

Cancel overlapped operation.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_cancel_impl(OverlappedObject *self)
/*[clinic end generated code: output=54ad7aeece89901c input=80eb67c7b57dbcf1]*/
{
    BOOL ret = TRUE;

    if (self->type == TYPE_NOT_STARTED
        || self->type == TYPE_WAIT_NAMED_PIPE_AND_CONNECT)
        Py_RETURN_NONE;

    if (!HasOverlappedIoCompleted(&self->overlapped)) {
        Py_BEGIN_ALLOW_THREADS
        ret = CancelIoEx(self->handle, &self->overlapped);
        Py_END_ALLOW_THREADS
    }

    /* CancelIoEx returns ERROR_NOT_FOUND if the I/O completed in-between */
    if (!ret && GetLastError() != ERROR_NOT_FOUND)
        return SetFromWindowsErr(0);
    Py_RETURN_NONE;
}

/*[clinic input]
_overlapped.Overlapped.getresult

    wait: BOOL(c_default='FALSE') = False
    /

Retrieve result of operation.

If wait is true then it blocks until the operation is finished.  If wait
is false and the operation is still pending then an error is raised.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_getresult_impl(OverlappedObject *self, BOOL wait)
/*[clinic end generated code: output=8c9bd04d08994f6c input=aa5b03e9897ca074]*/
{
    DWORD transferred = 0;
    BOOL ret;
    DWORD err;
    PyObject *addr;

    if (self->type == TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation not yet attempted");
        return NULL;
    }

    if (self->type == TYPE_NOT_STARTED) {
        PyErr_SetString(PyExc_ValueError, "operation failed to start");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    ret = GetOverlappedResult(self->handle, &self->overlapped, &transferred,
                              wait);
    Py_END_ALLOW_THREADS

    self->error = err = ret ? ERROR_SUCCESS : GetLastError();
    switch (err) {
        case ERROR_SUCCESS:
        case ERROR_MORE_DATA:
            break;
        case ERROR_BROKEN_PIPE:
            if (self->type == TYPE_READ || self->type == TYPE_READINTO) {
                break;
            }
            else if (self->type == TYPE_READ_FROM &&
                     (self->read_from.result != NULL ||
                      self->read_from.allocated_buffer != NULL))
            {
                break;
            }
            else if (self->type == TYPE_READ_FROM_INTO &&
                     self->read_from_into.result != NULL)
            {
                break;
            }
            _Py_FALLTHROUGH;
        default:
            return SetFromWindowsErr(err);
    }

    switch (self->type) {
        case TYPE_READ:
            assert(PyBytes_CheckExact(self->allocated_buffer));
            if (transferred != PyBytes_GET_SIZE(self->allocated_buffer) &&
                _PyBytes_Resize(&self->allocated_buffer, transferred))
                return NULL;

            return Py_NewRef(self->allocated_buffer);
        case TYPE_READ_FROM:
            assert(PyBytes_CheckExact(self->read_from.allocated_buffer));

            if (transferred != PyBytes_GET_SIZE(
                    self->read_from.allocated_buffer) &&
                _PyBytes_Resize(&self->read_from.allocated_buffer, transferred))
            {
                return NULL;
            }

            // unparse the address
            addr = unparse_address((SOCKADDR*)&self->read_from.address,
                                   self->read_from.address_length);

            if (addr == NULL) {
                return NULL;
            }

            // The result is a two item tuple: (message, address)
            self->read_from.result = PyTuple_New(2);
            if (self->read_from.result == NULL) {
                Py_CLEAR(addr);
                return NULL;
            }

            // first item: message
            PyTuple_SET_ITEM(self->read_from.result, 0,
                             Py_NewRef(self->read_from.allocated_buffer));
            // second item: address
            PyTuple_SET_ITEM(self->read_from.result, 1, addr);

            return Py_NewRef(self->read_from.result);
        case TYPE_READ_FROM_INTO:
            // unparse the address
            addr = unparse_address((SOCKADDR*)&self->read_from_into.address,
                self->read_from_into.address_length);

            if (addr == NULL) {
                return NULL;
            }

            // The result is a two item tuple: (number of bytes read, address)
            self->read_from_into.result = PyTuple_New(2);
            if (self->read_from_into.result == NULL) {
                Py_CLEAR(addr);
                return NULL;
            }

            // first item: number of bytes read
            PyTuple_SET_ITEM(self->read_from_into.result, 0,
                PyLong_FromUnsignedLong((unsigned long)transferred));
            // second item: address
            PyTuple_SET_ITEM(self->read_from_into.result, 1, addr);

            return Py_NewRef(self->read_from_into.result);
        default:
            return PyLong_FromUnsignedLong((unsigned long) transferred);
    }
}

static PyObject *
do_ReadFile(OverlappedObject *self, HANDLE handle,
            char *bufstart, DWORD buflen)
{
    DWORD nread;
    int ret;
    DWORD err;

    Py_BEGIN_ALLOW_THREADS
    ret = ReadFile(handle, bufstart, buflen, &nread,
                   &self->overlapped);
    Py_END_ALLOW_THREADS

    self->error = err = ret ? ERROR_SUCCESS : GetLastError();
    switch (err) {
        case ERROR_BROKEN_PIPE:
            mark_as_completed(&self->overlapped);
            return SetFromWindowsErr(err);
        case ERROR_SUCCESS:
        case ERROR_MORE_DATA:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}

/*[clinic input]
_overlapped.Overlapped.ReadFile

    handle: HANDLE
    size: DWORD
    /

Start overlapped read.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_ReadFile_impl(OverlappedObject *self, HANDLE handle,
                                     DWORD size)
/*[clinic end generated code: output=4c8557e16941e4ae input=98c495baa0342425]*/
{
    PyObject *buf;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T <= SIZEOF_LONG
    size = Py_MIN(size, (DWORD)PY_SSIZE_T_MAX);
#endif
    buf = PyBytes_FromStringAndSize(NULL, Py_MAX(size, 1));
    if (buf == NULL)
        return NULL;

    self->type = TYPE_READ;
    self->handle = handle;
    self->allocated_buffer = buf;

    return do_ReadFile(self, handle, PyBytes_AS_STRING(buf), size);
}

/*[clinic input]
_overlapped.Overlapped.ReadFileInto

    handle: HANDLE
    buf as bufobj: Py_buffer
    /

Start overlapped receive.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_ReadFileInto_impl(OverlappedObject *self,
                                         HANDLE handle, Py_buffer *bufobj)
/*[clinic end generated code: output=8754744506023071 input=4f037ba09939e32d]*/
{
    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T > SIZEOF_LONG
    if (bufobj->len > (Py_ssize_t)ULONG_MAX) {
        PyErr_SetString(PyExc_ValueError, "buffer too large");
        return NULL;
    }
#endif
    steal_buffer(&self->user_buffer, bufobj);

    self->type = TYPE_READINTO;
    self->handle = handle;

    return do_ReadFile(self, handle, self->user_buffer.buf,
                       (DWORD)self->user_buffer.len);
}

static PyObject *
do_WSARecv(OverlappedObject *self, HANDLE handle,
           char *bufstart, DWORD buflen, DWORD flags)
{
    DWORD nread;
    WSABUF wsabuf;
    int ret;
    DWORD err;

    wsabuf.buf = bufstart;
    wsabuf.len = buflen;

    Py_BEGIN_ALLOW_THREADS
    ret = WSARecv((SOCKET)handle, &wsabuf, 1, &nread, &flags,
                  &self->overlapped, NULL);
    Py_END_ALLOW_THREADS

    self->error = err = (ret < 0 ? WSAGetLastError() : ERROR_SUCCESS);
    switch (err) {
        case ERROR_BROKEN_PIPE:
            mark_as_completed(&self->overlapped);
            return SetFromWindowsErr(err);
        case ERROR_SUCCESS:
        case ERROR_MORE_DATA:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}


/*[clinic input]
_overlapped.Overlapped.WSARecv

    handle: HANDLE
    size: DWORD
    flags: DWORD = 0
    /

Start overlapped receive.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_WSARecv_impl(OverlappedObject *self, HANDLE handle,
                                    DWORD size, DWORD flags)
/*[clinic end generated code: output=3a5e9c61ff040906 input=8c04e506cc3d741a]*/
{
    PyObject *buf;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T <= SIZEOF_LONG
    size = Py_MIN(size, (DWORD)PY_SSIZE_T_MAX);
#endif
    buf = PyBytes_FromStringAndSize(NULL, Py_MAX(size, 1));
    if (buf == NULL)
        return NULL;

    self->type = TYPE_READ;
    self->handle = handle;
    self->allocated_buffer = buf;

    return do_WSARecv(self, handle, PyBytes_AS_STRING(buf), size, flags);
}

/*[clinic input]
_overlapped.Overlapped.WSARecvInto

    handle: HANDLE
    buf as bufobj: Py_buffer
    flags: DWORD
    /

Start overlapped receive.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_WSARecvInto_impl(OverlappedObject *self,
                                        HANDLE handle, Py_buffer *bufobj,
                                        DWORD flags)
/*[clinic end generated code: output=59ae7688786cf86b input=73e7fa00db633edd]*/
{
    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T > SIZEOF_LONG
    if (bufobj->len > (Py_ssize_t)ULONG_MAX) {
        PyErr_SetString(PyExc_ValueError, "buffer too large");
        return NULL;
    }
#endif
    steal_buffer(&self->user_buffer, bufobj);

    self->type = TYPE_READINTO;
    self->handle = handle;

    return do_WSARecv(self, handle, self->user_buffer.buf,
                      (DWORD)self->user_buffer.len, flags);
}

/*[clinic input]
_overlapped.Overlapped.WriteFile

    handle: HANDLE
    buf as bufobj: Py_buffer
    /

Start overlapped write.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_WriteFile_impl(OverlappedObject *self, HANDLE handle,
                                      Py_buffer *bufobj)
/*[clinic end generated code: output=fa5d5880a1bf04b1 input=ac54424c362abfc1]*/
{
    DWORD written;
    BOOL ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T > SIZEOF_LONG
    if (bufobj->len > (Py_ssize_t)ULONG_MAX) {
        PyErr_SetString(PyExc_ValueError, "buffer too large");
        return NULL;
    }
#endif
    steal_buffer(&self->user_buffer, bufobj);

    self->type = TYPE_WRITE;
    self->handle = handle;

    Py_BEGIN_ALLOW_THREADS
    ret = WriteFile(handle, self->user_buffer.buf,
                    (DWORD)self->user_buffer.len,
                    &written, &self->overlapped);
    Py_END_ALLOW_THREADS

    self->error = err = ret ? ERROR_SUCCESS : GetLastError();
    switch (err) {
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}

/*[clinic input]
_overlapped.Overlapped.WSASend

    handle: HANDLE
    buf as bufobj: Py_buffer
    flags: DWORD
    /

Start overlapped send.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_WSASend_impl(OverlappedObject *self, HANDLE handle,
                                    Py_buffer *bufobj, DWORD flags)
/*[clinic end generated code: output=3baaa6e1f7fe229e input=c4167420ba2f93d8]*/
{
    DWORD written;
    WSABUF wsabuf;
    int ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T > SIZEOF_LONG
    if (bufobj->len > (Py_ssize_t)ULONG_MAX) {
        PyErr_SetString(PyExc_ValueError, "buffer too large");
        return NULL;
    }
#endif
    steal_buffer(&self->user_buffer, bufobj);

    self->type = TYPE_WRITE;
    self->handle = handle;
    wsabuf.len = (DWORD)self->user_buffer.len;
    wsabuf.buf = self->user_buffer.buf;

    Py_BEGIN_ALLOW_THREADS
    ret = WSASend((SOCKET)handle, &wsabuf, 1, &written, flags,
                  &self->overlapped, NULL);
    Py_END_ALLOW_THREADS

    self->error = err = (ret < 0 ? WSAGetLastError() : ERROR_SUCCESS);
    switch (err) {
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}

/*[clinic input]
_overlapped.Overlapped.AcceptEx

    listen_handle as ListenSocket: HANDLE
    accept_handle as AcceptSocket: HANDLE
    /

Start overlapped wait for client to connect.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_AcceptEx_impl(OverlappedObject *self,
                                     HANDLE ListenSocket,
                                     HANDLE AcceptSocket)
/*[clinic end generated code: output=9a7381d4232af889 input=b83473224fc3a1c5]*/
{
    DWORD BytesReceived;
    DWORD size;
    PyObject *buf;
    BOOL ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

    size = sizeof(struct sockaddr_in6) + 16;
    buf = PyBytes_FromStringAndSize(NULL, size*2);
    if (!buf)
        return NULL;

    self->type = TYPE_ACCEPT;
    self->handle = ListenSocket;
    self->allocated_buffer = buf;

    Py_BEGIN_ALLOW_THREADS
    ret = Py_AcceptEx((SOCKET)ListenSocket, (SOCKET)AcceptSocket,
                      PyBytes_AS_STRING(buf), 0, size, size, &BytesReceived,
                      &self->overlapped);
    Py_END_ALLOW_THREADS

    self->error = err = ret ? ERROR_SUCCESS : WSAGetLastError();
    switch (err) {
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}


static int
parse_address(PyObject *obj, SOCKADDR *Address, int Length)
{
    PyObject *Host_obj;
    wchar_t *Host;
    unsigned short Port;
    unsigned long FlowInfo;
    unsigned long ScopeId;

    memset(Address, 0, Length);

    switch (PyTuple_GET_SIZE(obj)) {
    case 2: {
        if (!PyArg_ParseTuple(obj, "UH", &Host_obj, &Port)) {
            return -1;
        }
        Host = PyUnicode_AsWideCharString(Host_obj, NULL);
        if (Host == NULL) {
            return -1;
        }
        Address->sa_family = AF_INET;
        if (WSAStringToAddressW(Host, AF_INET, NULL, Address, &Length) < 0) {
            SetFromWindowsErr(WSAGetLastError());
            Length = -1;
        }
        else {
            ((SOCKADDR_IN*)Address)->sin_port = htons(Port);
        }
        PyMem_Free(Host);
        return Length;
    }
    case 4: {
        if (!PyArg_ParseTuple(obj,
                "UHkk;ConnectEx(): illegal address_as_bytes argument",
                &Host_obj, &Port, &FlowInfo, &ScopeId))
        {
            return -1;
        }
        Host = PyUnicode_AsWideCharString(Host_obj, NULL);
        if (Host == NULL) {
            return -1;
        }
        Address->sa_family = AF_INET6;
        if (WSAStringToAddressW(Host, AF_INET6, NULL, Address, &Length) < 0) {
            SetFromWindowsErr(WSAGetLastError());
            Length = -1;
        }
        else {
            ((SOCKADDR_IN6*)Address)->sin6_port = htons(Port);
            ((SOCKADDR_IN6*)Address)->sin6_flowinfo = FlowInfo;
            ((SOCKADDR_IN6*)Address)->sin6_scope_id = ScopeId;
        }
        PyMem_Free(Host);
        return Length;
    }
    default:
        PyErr_SetString(PyExc_ValueError, "illegal address_as_bytes argument");
        return -1;
    }
}

/*[clinic input]
_overlapped.Overlapped.ConnectEx

    client_handle as ConnectSocket: HANDLE
    address_as_bytes as AddressObj: object(subclass_of='&PyTuple_Type')
    /

Start overlapped connect.

client_handle should be unbound.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_ConnectEx_impl(OverlappedObject *self,
                                      HANDLE ConnectSocket,
                                      PyObject *AddressObj)
/*[clinic end generated code: output=5aebbbdb4f022833 input=d6bbd2d84b156fc1]*/
{
    char AddressBuf[sizeof(struct sockaddr_in6)];
    SOCKADDR *Address = (SOCKADDR*)AddressBuf;
    int Length;
    BOOL ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

    Length = sizeof(AddressBuf);
    Length = parse_address(AddressObj, Address, Length);
    if (Length < 0)
        return NULL;

    self->type = TYPE_CONNECT;
    self->handle = ConnectSocket;

    Py_BEGIN_ALLOW_THREADS
    ret = Py_ConnectEx((SOCKET)ConnectSocket, Address, Length,
                       NULL, 0, NULL, &self->overlapped);
    Py_END_ALLOW_THREADS

    self->error = err = ret ? ERROR_SUCCESS : WSAGetLastError();
    switch (err) {
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}

/*[clinic input]
_overlapped.Overlapped.DisconnectEx

    handle as Socket: HANDLE
    flags: DWORD
    /

[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_DisconnectEx_impl(OverlappedObject *self,
                                         HANDLE Socket, DWORD flags)
/*[clinic end generated code: output=8d64ddb8c93c2126 input=680845cdcdf820eb]*/
{
    BOOL ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

    self->type = TYPE_DISCONNECT;
    self->handle = Socket;

    Py_BEGIN_ALLOW_THREADS
    ret = Py_DisconnectEx((SOCKET)Socket, &self->overlapped, flags, 0);
    Py_END_ALLOW_THREADS

    self->error = err = ret ? ERROR_SUCCESS : WSAGetLastError();
    switch (err) {
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}

/*[clinic input]
_overlapped.Overlapped.TransmitFile

    socket as Socket: HANDLE
    file as File: HANDLE
    offset: DWORD
    offset_high: DWORD
    count_to_write: DWORD
    count_per_send: DWORD
    flags: DWORD
    /

Transmit file data over a connected socket.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_TransmitFile_impl(OverlappedObject *self,
                                         HANDLE Socket, HANDLE File,
                                         DWORD offset, DWORD offset_high,
                                         DWORD count_to_write,
                                         DWORD count_per_send, DWORD flags)
/*[clinic end generated code: output=03f3ca5512e678fd input=7e6f97b391f60e8c]*/
{
    BOOL ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

    self->type = TYPE_TRANSMIT_FILE;
    self->handle = Socket;
    self->overlapped.Offset = offset;
    self->overlapped.OffsetHigh = offset_high;

    Py_BEGIN_ALLOW_THREADS
    ret = Py_TransmitFile((SOCKET)Socket, File, count_to_write,
                          count_per_send, &self->overlapped, NULL, flags);
    Py_END_ALLOW_THREADS

    self->error = err = ret ? ERROR_SUCCESS : WSAGetLastError();
    switch (err) {
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}

/*[clinic input]
_overlapped.Overlapped.ConnectNamedPipe

    handle as Pipe: HANDLE
    /

Start overlapped wait for a client to connect.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_ConnectNamedPipe_impl(OverlappedObject *self,
                                             HANDLE Pipe)
/*[clinic end generated code: output=3e69adfe55818abe input=8b0d4cef8a72f7bc]*/
{
    BOOL ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

    self->type = TYPE_CONNECT_NAMED_PIPE;
    self->handle = Pipe;

    Py_BEGIN_ALLOW_THREADS
    ret = ConnectNamedPipe(Pipe, &self->overlapped);
    Py_END_ALLOW_THREADS

    self->error = err = ret ? ERROR_SUCCESS : GetLastError();
    switch (err) {
        case ERROR_PIPE_CONNECTED:
            mark_as_completed(&self->overlapped);
            Py_RETURN_TRUE;
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            Py_RETURN_FALSE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}

/*[clinic input]
_overlapped.Overlapped.ConnectPipe

    addr as Address: Py_UNICODE
    /

Connect to the pipe for asynchronous I/O (overlapped).
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_ConnectPipe_impl(OverlappedObject *self,
                                        const wchar_t *Address)
/*[clinic end generated code: output=67cbd8e4d3a57855 input=167c06a274efcefc]*/
{
    HANDLE PipeHandle;

    Py_BEGIN_ALLOW_THREADS
    PipeHandle = CreateFileW(Address,
                             GENERIC_READ | GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED, NULL);
    Py_END_ALLOW_THREADS

    if (PipeHandle == INVALID_HANDLE_VALUE)
        return SetFromWindowsErr(0);
    return Py_BuildValue(F_HANDLE, PipeHandle);
}

static PyObject*
Overlapped_getaddress(PyObject *op, void *Py_UNUSED(closure))
{
    OverlappedObject *self = OverlappedObject_CAST(op);
    return PyLong_FromVoidPtr(&self->overlapped);
}

static PyObject*
Overlapped_getpending(PyObject *op, void *Py_UNUSED(closure))
{
    OverlappedObject *self = OverlappedObject_CAST(op);
    return PyBool_FromLong(!HasOverlappedIoCompleted(&self->overlapped) &&
                           self->type != TYPE_NOT_STARTED);
}

static int
Overlapped_traverse(PyObject *op, visitproc visit, void *arg)
{
    OverlappedObject *self = OverlappedObject_CAST(op);
    switch (self->type) {
    case TYPE_READ:
    case TYPE_ACCEPT:
        Py_VISIT(self->allocated_buffer);
        break;
    case TYPE_WRITE:
    case TYPE_WRITE_TO:
    case TYPE_READINTO:
        if (self->user_buffer.obj) {
            Py_VISIT(&self->user_buffer.obj);
        }
        break;
    case TYPE_READ_FROM:
        Py_VISIT(self->read_from.result);
        Py_VISIT(self->read_from.allocated_buffer);
        break;
    case TYPE_READ_FROM_INTO:
        Py_VISIT(self->read_from_into.result);
        if (self->read_from_into.user_buffer.obj) {
            Py_VISIT(&self->read_from_into.user_buffer.obj);
        }
        break;
    }
    return 0;
}

// UDP functions

/*
 * Note: WSAConnect does not support Overlapped I/O so this function should
 * _only_ be used for connectionless sockets (UDP).
 */

/*[clinic input]
_overlapped.WSAConnect

    client_handle as ConnectSocket: HANDLE
    address_as_bytes as AddressObj: object(subclass_of='&PyTuple_Type')
    /

Bind a remote address to a connectionless (UDP) socket.
[clinic start generated code]*/

static PyObject *
_overlapped_WSAConnect_impl(PyObject *module, HANDLE ConnectSocket,
                            PyObject *AddressObj)
/*[clinic end generated code: output=ea0b4391e94dad63 input=7cf65313d49c015a]*/
{
    char AddressBuf[sizeof(struct sockaddr_in6)];
    SOCKADDR *Address = (SOCKADDR*)AddressBuf;
    int Length;
    int err;

    Length = sizeof(AddressBuf);
    Length = parse_address(AddressObj, Address, Length);
    if (Length < 0) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    // WSAConnect does not support overlapped I/O so this call will
    // successfully complete immediately.
    err = WSAConnect((SOCKET)ConnectSocket, Address, Length,
                        NULL, NULL, NULL, NULL);
    Py_END_ALLOW_THREADS

    if (err == 0) {
        Py_RETURN_NONE;
    }
    else {
        return SetFromWindowsErr(WSAGetLastError());
    }
}

/*[clinic input]
_overlapped.Overlapped.WSASendTo

    handle: HANDLE
    buf as bufobj: Py_buffer
    flags: DWORD
    address_as_bytes as AddressObj: object(subclass_of='&PyTuple_Type')
    /

Start overlapped sendto over a connectionless (UDP) socket.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_WSASendTo_impl(OverlappedObject *self, HANDLE handle,
                                      Py_buffer *bufobj, DWORD flags,
                                      PyObject *AddressObj)
/*[clinic end generated code: output=3cdedc4cfaeb70cd input=31f44cd4ab92fc33]*/
{
    char AddressBuf[sizeof(struct sockaddr_in6)];
    SOCKADDR *Address = (SOCKADDR*)AddressBuf;
    int AddressLength;
    DWORD written;
    WSABUF wsabuf;
    int ret;
    DWORD err;

    // Parse the "to" address
    AddressLength = sizeof(AddressBuf);
    AddressLength = parse_address(AddressObj, Address, AddressLength);
    if (AddressLength < 0) {
        return NULL;
    }

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T > SIZEOF_LONG
    if (bufobj->len > (Py_ssize_t)ULONG_MAX) {
        PyErr_SetString(PyExc_ValueError, "buffer too large");
        return NULL;
    }
#endif
    steal_buffer(&self->user_buffer, bufobj);

    self->type = TYPE_WRITE_TO;
    self->handle = handle;
    wsabuf.len = (DWORD)self->user_buffer.len;
    wsabuf.buf = self->user_buffer.buf;

    Py_BEGIN_ALLOW_THREADS
    ret = WSASendTo((SOCKET)handle, &wsabuf, 1, &written, flags,
                    Address, AddressLength, &self->overlapped, NULL);
    Py_END_ALLOW_THREADS

    self->error = err = (ret == SOCKET_ERROR ? WSAGetLastError() :
                                               ERROR_SUCCESS);

    switch(err) {
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            Py_RETURN_NONE;
        default:
            Overlapped_clear(self);
            return SetFromWindowsErr(err);
    }
}

/*[clinic input]
_overlapped.Overlapped.WSARecvFrom

    handle: HANDLE
    size: DWORD
    flags: DWORD = 0
    /

Start overlapped receive.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_WSARecvFrom_impl(OverlappedObject *self,
                                        HANDLE handle, DWORD size,
                                        DWORD flags)
/*[clinic end generated code: output=13832a2025b86860 input=1b2663fa130e0286]*/
{
    PyObject *buf;
    DWORD nread;
    WSABUF wsabuf;
    int ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T <= SIZEOF_LONG
    size = Py_MIN(size, (DWORD)PY_SSIZE_T_MAX);
#endif
    buf = PyBytes_FromStringAndSize(NULL, Py_MAX(size, 1));
    if (buf == NULL) {
        return NULL;
    }

    wsabuf.buf = PyBytes_AS_STRING(buf);
    wsabuf.len = size;

    self->type = TYPE_READ_FROM;
    self->handle = handle;
    self->read_from.allocated_buffer = buf;
    memset(&self->read_from.address, 0, sizeof(self->read_from.address));
    self->read_from.address_length = sizeof(self->read_from.address);

    Py_BEGIN_ALLOW_THREADS
    ret = WSARecvFrom((SOCKET)handle, &wsabuf, 1, &nread, &flags,
                      (SOCKADDR*)&self->read_from.address,
                      &self->read_from.address_length,
                      &self->overlapped, NULL);
    Py_END_ALLOW_THREADS

    self->error = err = (ret < 0 ? WSAGetLastError() : ERROR_SUCCESS);
    switch (err) {
    case ERROR_BROKEN_PIPE:
        mark_as_completed(&self->overlapped);
        return SetFromWindowsErr(err);
    case ERROR_SUCCESS:
    case ERROR_MORE_DATA:
    case ERROR_IO_PENDING:
        Py_RETURN_NONE;
    default:
        Overlapped_clear(self);
        return SetFromWindowsErr(err);
    }
}


/*[clinic input]
_overlapped.Overlapped.WSARecvFromInto

    handle: HANDLE
    buf as bufobj: Py_buffer
    size: DWORD
    flags: DWORD = 0
    /

Start overlapped receive.
[clinic start generated code]*/

static PyObject *
_overlapped_Overlapped_WSARecvFromInto_impl(OverlappedObject *self,
                                            HANDLE handle, Py_buffer *bufobj,
                                            DWORD size, DWORD flags)
/*[clinic end generated code: output=30c7ea171a691757 input=4be4b08d03531e76]*/
{
    DWORD nread;
    WSABUF wsabuf;
    int ret;
    DWORD err;

    if (self->type != TYPE_NONE) {
        PyErr_SetString(PyExc_ValueError, "operation already attempted");
        return NULL;
    }

#if SIZEOF_SIZE_T > SIZEOF_LONG
    if (bufobj->len > (Py_ssize_t)ULONG_MAX) {
        PyErr_SetString(PyExc_ValueError, "buffer too large");
        return NULL;
    }
#endif

    wsabuf.buf = bufobj->buf;
    wsabuf.len = size;

    self->type = TYPE_READ_FROM_INTO;
    self->handle = handle;
    steal_buffer(&self->read_from_into.user_buffer, bufobj);
    memset(&self->read_from_into.address, 0, sizeof(self->read_from_into.address));
    self->read_from_into.address_length = sizeof(self->read_from_into.address);

    Py_BEGIN_ALLOW_THREADS
    ret = WSARecvFrom((SOCKET)handle, &wsabuf, 1, &nread, &flags,
                      (SOCKADDR*)&self->read_from_into.address,
                      &self->read_from_into.address_length,
                      &self->overlapped, NULL);
    Py_END_ALLOW_THREADS

    self->error = err = (ret < 0 ? WSAGetLastError() : ERROR_SUCCESS);
    switch (err) {
    case ERROR_BROKEN_PIPE:
        mark_as_completed(&self->overlapped);
        return SetFromWindowsErr(err);
    case ERROR_SUCCESS:
    case ERROR_MORE_DATA:
    case ERROR_IO_PENDING:
        Py_RETURN_NONE;
    default:
        Overlapped_clear(self);
        return SetFromWindowsErr(err);
    }
}


#include "clinic/overlapped.c.h"

static PyMethodDef Overlapped_methods[] = {
    _OVERLAPPED_OVERLAPPED_GETRESULT_METHODDEF
    _OVERLAPPED_OVERLAPPED_CANCEL_METHODDEF
    _OVERLAPPED_OVERLAPPED_READFILE_METHODDEF
    _OVERLAPPED_OVERLAPPED_READFILEINTO_METHODDEF
    _OVERLAPPED_OVERLAPPED_WSARECV_METHODDEF
    _OVERLAPPED_OVERLAPPED_WSARECVINTO_METHODDEF
    _OVERLAPPED_OVERLAPPED_WSARECVFROM_METHODDEF
    _OVERLAPPED_OVERLAPPED_WSARECVFROMINTO_METHODDEF
    _OVERLAPPED_OVERLAPPED_WRITEFILE_METHODDEF
    _OVERLAPPED_OVERLAPPED_WSASEND_METHODDEF
    _OVERLAPPED_OVERLAPPED_ACCEPTEX_METHODDEF
    _OVERLAPPED_OVERLAPPED_CONNECTEX_METHODDEF
    _OVERLAPPED_OVERLAPPED_DISCONNECTEX_METHODDEF
    _OVERLAPPED_OVERLAPPED_TRANSMITFILE_METHODDEF
    _OVERLAPPED_OVERLAPPED_CONNECTNAMEDPIPE_METHODDEF
    _OVERLAPPED_OVERLAPPED_WSARECVFROM_METHODDEF
    _OVERLAPPED_OVERLAPPED_WSASENDTO_METHODDEF
    {NULL}
};

static PyMemberDef Overlapped_members[] = {
    {"error", Py_T_ULONG,
     offsetof(OverlappedObject, error),
     Py_READONLY, "Error from last operation"},
    {"event", T_HANDLE,
     offsetof(OverlappedObject, overlapped) + offsetof(OVERLAPPED, hEvent),
     Py_READONLY, "Overlapped event handle"},
    {NULL}
};

static PyGetSetDef Overlapped_getsets[] = {
    {"address", Overlapped_getaddress, NULL,
     "Address of overlapped structure"},
    {"pending", Overlapped_getpending, NULL,
     "Whether the operation is pending"},
    {NULL},
};

static PyType_Slot overlapped_type_slots[] = {
    {Py_tp_dealloc, Overlapped_dealloc},
    {Py_tp_doc, (char *)_overlapped_Overlapped__doc__},
    {Py_tp_traverse, Overlapped_traverse},
    {Py_tp_methods, Overlapped_methods},
    {Py_tp_members, Overlapped_members},
    {Py_tp_getset, Overlapped_getsets},
    {Py_tp_new, _overlapped_Overlapped},
    {0,0}
};

static PyType_Spec overlapped_type_spec = {
    .name = "_overlapped.Overlapped",
    .basicsize = sizeof(OverlappedObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = overlapped_type_slots
};

static PyMethodDef overlapped_functions[] = {
    _OVERLAPPED_CREATEIOCOMPLETIONPORT_METHODDEF
    _OVERLAPPED_GETQUEUEDCOMPLETIONSTATUS_METHODDEF
    _OVERLAPPED_POSTQUEUEDCOMPLETIONSTATUS_METHODDEF
    _OVERLAPPED_FORMATMESSAGE_METHODDEF
    _OVERLAPPED_BINDLOCAL_METHODDEF
    _OVERLAPPED_REGISTERWAITWITHQUEUE_METHODDEF
    _OVERLAPPED_UNREGISTERWAIT_METHODDEF
    _OVERLAPPED_UNREGISTERWAITEX_METHODDEF
    _OVERLAPPED_CREATEEVENT_METHODDEF
    _OVERLAPPED_SETEVENT_METHODDEF
    _OVERLAPPED_RESETEVENT_METHODDEF
    _OVERLAPPED_OVERLAPPED_CONNECTPIPE_METHODDEF
    _OVERLAPPED_WSACONNECT_METHODDEF
    {NULL}
};

#define WINAPI_CONSTANT(fmt, con) \
    do { \
        if (PyModule_Add(module, #con, Py_BuildValue(fmt, con)) < 0 ) { \
            return -1; \
        } \
    } while (0)

static int
overlapped_exec(PyObject *module)
{
    /* Ensure WSAStartup() called before initializing function pointers */
    PyObject *socket_module = PyImport_ImportModule("_socket");
    if (!socket_module) {
        return -1;
    }

    Py_DECREF(socket_module);

    if (initialize_function_pointers() < 0) {
        return -1;
    }

    PyTypeObject *overlapped_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &overlapped_type_spec, NULL);
    if (overlapped_type == NULL) {
        return -1;
    }

    int rc = PyModule_AddType(module, overlapped_type);
    Py_DECREF(overlapped_type);
    if (rc < 0) {
        return -1;
    }

    WINAPI_CONSTANT(F_DWORD,  ERROR_IO_PENDING);
    WINAPI_CONSTANT(F_DWORD,  ERROR_NETNAME_DELETED);
    WINAPI_CONSTANT(F_DWORD,  ERROR_OPERATION_ABORTED);
    WINAPI_CONSTANT(F_DWORD,  ERROR_SEM_TIMEOUT);
    WINAPI_CONSTANT(F_DWORD,  ERROR_PIPE_BUSY);
    WINAPI_CONSTANT(F_DWORD,  ERROR_PORT_UNREACHABLE);
    WINAPI_CONSTANT(F_DWORD,  INFINITE);
    WINAPI_CONSTANT(F_HANDLE, INVALID_HANDLE_VALUE);
    WINAPI_CONSTANT(F_HANDLE, NULL);
    WINAPI_CONSTANT(F_DWORD,  SO_UPDATE_ACCEPT_CONTEXT);
    WINAPI_CONSTANT(F_DWORD,  SO_UPDATE_CONNECT_CONTEXT);
    WINAPI_CONSTANT(F_DWORD,  TF_REUSE_SOCKET);

    return 0;
}

static PyModuleDef_Slot overlapped_slots[] = {
    {Py_mod_exec, overlapped_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef overlapped_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_overlapped",
    .m_methods = overlapped_functions,
    .m_slots = overlapped_slots,
};

PyMODINIT_FUNC
PyInit__overlapped(void)
{
    return PyModuleDef_Init(&overlapped_module);
}
