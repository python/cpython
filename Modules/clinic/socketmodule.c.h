/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_long.h"          // _PyLong_UInt16_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

#if (defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4))

PyDoc_STRVAR(_socket_socket__accept__doc__,
"_accept($self, /)\n"
"--\n"
"\n"
"Wait for an incoming connection.\n"
"\n"
"Return a new socket file descriptor representing the connection, and\n"
"the address of the client.  For IP sockets, the address info is a\n"
"pair (hostaddr, port).");

#define _SOCKET_SOCKET__ACCEPT_METHODDEF    \
    {"_accept", (PyCFunction)_socket_socket__accept, METH_NOARGS, _socket_socket__accept__doc__},

static PyObject *
_socket_socket__accept_impl(PySocketSockObject *s);

static PyObject *
_socket_socket__accept(PyObject *s, PyObject *Py_UNUSED(ignored))
{
    return _socket_socket__accept_impl((PySocketSockObject *)s);
}

#endif /* (defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4)) */

PyDoc_STRVAR(_socket_socket_setblocking__doc__,
"setblocking($self, flag, /)\n"
"--\n"
"\n"
"Set the socket to blocking (flag is true) or non-blocking (false).\n"
"\n"
"setblocking(True) is equivalent to settimeout(None);\n"
"setblocking(False) is equivalent to settimeout(0.0).");

#define _SOCKET_SOCKET_SETBLOCKING_METHODDEF    \
    {"setblocking", (PyCFunction)_socket_socket_setblocking, METH_O, _socket_socket_setblocking__doc__},

static PyObject *
_socket_socket_setblocking_impl(PySocketSockObject *s, int flag);

static PyObject *
_socket_socket_setblocking(PyObject *s, PyObject *arg)
{
    PyObject *return_value = NULL;
    int flag;

    flag = PyObject_IsTrue(arg);
    if (flag < 0) {
        goto exit;
    }
    return_value = _socket_socket_setblocking_impl((PySocketSockObject *)s, flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_socket_getblocking__doc__,
"getblocking($self, /)\n"
"--\n"
"\n"
"Return True if socket is in blocking mode, False if in non-blocking.");

#define _SOCKET_SOCKET_GETBLOCKING_METHODDEF    \
    {"getblocking", (PyCFunction)_socket_socket_getblocking, METH_NOARGS, _socket_socket_getblocking__doc__},

static PyObject *
_socket_socket_getblocking_impl(PySocketSockObject *s);

static PyObject *
_socket_socket_getblocking(PyObject *s, PyObject *Py_UNUSED(ignored))
{
    return _socket_socket_getblocking_impl((PySocketSockObject *)s);
}

PyDoc_STRVAR(_socket_socket_settimeout__doc__,
"settimeout($self, timeout, /)\n"
"--\n"
"\n"
"Set a timeout on socket operations.\n"
"\n"
"\'timeout\' can be a float, giving in seconds, or None.  Setting a\n"
"timeout of None disables the timeout feature and is equivalent to\n"
"setblocking(1).  Setting a timeout of zero is the same as\n"
"setblocking(0).");

#define _SOCKET_SOCKET_SETTIMEOUT_METHODDEF    \
    {"settimeout", (PyCFunction)_socket_socket_settimeout, METH_O, _socket_socket_settimeout__doc__},

static PyObject *
_socket_socket_settimeout_impl(PySocketSockObject *s, PyObject *arg);

static PyObject *
_socket_socket_settimeout(PyObject *s, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = _socket_socket_settimeout_impl((PySocketSockObject *)s, arg);

    return return_value;
}

PyDoc_STRVAR(_socket_socket_gettimeout__doc__,
"gettimeout($self, /)\n"
"--\n"
"\n"
"Return the timeout in seconds (float) of socket operations.\n"
"\n"
"A timeout of None indicates that timeouts on socket operations are\n"
"disabled.");

#define _SOCKET_SOCKET_GETTIMEOUT_METHODDEF    \
    {"gettimeout", (PyCFunction)_socket_socket_gettimeout, METH_NOARGS, _socket_socket_gettimeout__doc__},

static PyObject *
_socket_socket_gettimeout_impl(PySocketSockObject *self);

static PyObject *
_socket_socket_gettimeout(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _socket_socket_gettimeout_impl((PySocketSockObject *)self);
}

PyDoc_STRVAR(_socket_socket_getsockopt__doc__,
"getsockopt($self, level, option, buffersize=0, /)\n"
"--\n"
"\n"
"Get a socket option.\n"
"\n"
"See the Unix manual for level and option.  If a nonzero buffersize\n"
"argument is given, the return value is a bytes object of that\n"
"length; otherwise it is an integer.");

#define _SOCKET_SOCKET_GETSOCKOPT_METHODDEF    \
    {"getsockopt", _PyCFunction_CAST(_socket_socket_getsockopt), METH_FASTCALL, _socket_socket_getsockopt__doc__},

static PyObject *
_socket_socket_getsockopt_impl(PySocketSockObject *s, int level, int optname,
                               int buflen);

static PyObject *
_socket_socket_getsockopt(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int level;
    int optname;
    int buflen = 0;

    if (!_PyArg_CheckPositional("getsockopt", nargs, 2, 3)) {
        goto exit;
    }
    level = PyLong_AsInt(args[0]);
    if (level == -1 && PyErr_Occurred()) {
        goto exit;
    }
    optname = PyLong_AsInt(args[1]);
    if (optname == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    buflen = PyLong_AsInt(args[2]);
    if (buflen == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_getsockopt_impl((PySocketSockObject *)s, level, optname, buflen);

exit:
    return return_value;
}

#if defined(HAVE_BIND)

PyDoc_STRVAR(_socket_socket_bind__doc__,
"bind($self, address, /)\n"
"--\n"
"\n"
"Bind the socket to a local address.\n"
"\n"
"For IP sockets, the address is a pair (host, port); the host must\n"
"refer to the local host.  For raw packet sockets the address is a\n"
"tuple (ifname, proto [,pkttype [,hatype [,addr]]]).");

#define _SOCKET_SOCKET_BIND_METHODDEF    \
    {"bind", (PyCFunction)_socket_socket_bind, METH_O, _socket_socket_bind__doc__},

static PyObject *
_socket_socket_bind_impl(PySocketSockObject *s, PyObject *addro);

static PyObject *
_socket_socket_bind(PyObject *s, PyObject *addro)
{
    PyObject *return_value = NULL;

    return_value = _socket_socket_bind_impl((PySocketSockObject *)s, addro);

    return return_value;
}

#endif /* defined(HAVE_BIND) */

PyDoc_STRVAR(_socket_socket_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"close()\n"
"\n"
"Close the socket.  It cannot be used after this call.");

#define _SOCKET_SOCKET_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_socket_socket_close, METH_NOARGS, _socket_socket_close__doc__},

static PyObject *
_socket_socket_close_impl(PySocketSockObject *s);

static PyObject *
_socket_socket_close(PyObject *s, PyObject *Py_UNUSED(ignored))
{
    return _socket_socket_close_impl((PySocketSockObject *)s);
}

PyDoc_STRVAR(_socket_socket_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n"
"Close the socket object without closing the file descriptor.\n"
"\n"
"The object cannot be used after this call, but the file descriptor\n"
"can be reused for other purposes.  The file descriptor is returned.");

#define _SOCKET_SOCKET_DETACH_METHODDEF    \
    {"detach", (PyCFunction)_socket_socket_detach, METH_NOARGS, _socket_socket_detach__doc__},

static PyObject *
_socket_socket_detach_impl(PySocketSockObject *s);

static PyObject *
_socket_socket_detach(PyObject *s, PyObject *Py_UNUSED(ignored))
{
    return _socket_socket_detach_impl((PySocketSockObject *)s);
}

#if defined(HAVE_CONNECT)

PyDoc_STRVAR(_socket_socket_connect__doc__,
"connect($self, address, /)\n"
"--\n"
"\n"
"Connect the socket to a remote address.\n"
"\n"
"For IP sockets, the address is a pair (host, port).");

#define _SOCKET_SOCKET_CONNECT_METHODDEF    \
    {"connect", (PyCFunction)_socket_socket_connect, METH_O, _socket_socket_connect__doc__},

static PyObject *
_socket_socket_connect_impl(PySocketSockObject *s, PyObject *addro);

static PyObject *
_socket_socket_connect(PyObject *s, PyObject *addro)
{
    PyObject *return_value = NULL;

    return_value = _socket_socket_connect_impl((PySocketSockObject *)s, addro);

    return return_value;
}

#endif /* defined(HAVE_CONNECT) */

#if defined(HAVE_CONNECT)

PyDoc_STRVAR(_socket_socket_connect_ex__doc__,
"connect_ex($self, address, /)\n"
"--\n"
"\n"
"Connect the socket to a remote address.\n"
"\n"
"This is like connect(address), but returns an error code (the errno\n"
"value) instead of raising an exception when an error occurs.");

#define _SOCKET_SOCKET_CONNECT_EX_METHODDEF    \
    {"connect_ex", (PyCFunction)_socket_socket_connect_ex, METH_O, _socket_socket_connect_ex__doc__},

static PyObject *
_socket_socket_connect_ex_impl(PySocketSockObject *s, PyObject *addro);

static PyObject *
_socket_socket_connect_ex(PyObject *s, PyObject *addro)
{
    PyObject *return_value = NULL;

    return_value = _socket_socket_connect_ex_impl((PySocketSockObject *)s, addro);

    return return_value;
}

#endif /* defined(HAVE_CONNECT) */

PyDoc_STRVAR(_socket_socket_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the integer file descriptor of the socket.");

#define _SOCKET_SOCKET_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)_socket_socket_fileno, METH_NOARGS, _socket_socket_fileno__doc__},

static PyObject *
_socket_socket_fileno_impl(PySocketSockObject *s);

static PyObject *
_socket_socket_fileno(PyObject *s, PyObject *Py_UNUSED(ignored))
{
    return _socket_socket_fileno_impl((PySocketSockObject *)s);
}

#if defined(HAVE_GETSOCKNAME)

PyDoc_STRVAR(_socket_socket_getsockname__doc__,
"getsockname($self, /)\n"
"--\n"
"\n"
"Return the address of the local endpoint.\n"
"\n"
"The format depends on the address family.  For IPv4 sockets, the\n"
"address info is a pair (hostaddr, port). For IPv6 sockets, the\n"
"address info is a 4-tuple (hostaddr, port, flowinfo, scope_id).");

#define _SOCKET_SOCKET_GETSOCKNAME_METHODDEF    \
    {"getsockname", (PyCFunction)_socket_socket_getsockname, METH_NOARGS, _socket_socket_getsockname__doc__},

static PyObject *
_socket_socket_getsockname_impl(PySocketSockObject *s);

static PyObject *
_socket_socket_getsockname(PyObject *s, PyObject *Py_UNUSED(ignored))
{
    return _socket_socket_getsockname_impl((PySocketSockObject *)s);
}

#endif /* defined(HAVE_GETSOCKNAME) */

#if defined(HAVE_GETPEERNAME)

PyDoc_STRVAR(_socket_socket_getpeername__doc__,
"getpeername($self, /)\n"
"--\n"
"\n"
"Return the address of the remote endpoint.\n"
"\n"
"For IP sockets, the address info is a pair (hostaddr, port).");

#define _SOCKET_SOCKET_GETPEERNAME_METHODDEF    \
    {"getpeername", (PyCFunction)_socket_socket_getpeername, METH_NOARGS, _socket_socket_getpeername__doc__},

static PyObject *
_socket_socket_getpeername_impl(PySocketSockObject *s);

static PyObject *
_socket_socket_getpeername(PyObject *s, PyObject *Py_UNUSED(ignored))
{
    return _socket_socket_getpeername_impl((PySocketSockObject *)s);
}

#endif /* defined(HAVE_GETPEERNAME) */

#if defined(HAVE_LISTEN)

PyDoc_STRVAR(_socket_socket_listen__doc__,
"listen($self, backlog=min(SOMAXCONN, 128), /)\n"
"--\n"
"\n"
"Enable a server to accept connections.\n"
"\n"
"If backlog is specified, it must be at least 0 (if it is lower, it\n"
"is set to 0); it specifies the number of unaccepted connections that\n"
"the system will allow before refusing new connections. If not\n"
"specified, a default reasonable value is chosen.");

#define _SOCKET_SOCKET_LISTEN_METHODDEF    \
    {"listen", _PyCFunction_CAST(_socket_socket_listen), METH_FASTCALL, _socket_socket_listen__doc__},

static PyObject *
_socket_socket_listen_impl(PySocketSockObject *s, int backlog);

static PyObject *
_socket_socket_listen(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int backlog = Py_MIN(SOMAXCONN, 128);

    if (!_PyArg_CheckPositional("listen", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    backlog = PyLong_AsInt(args[0]);
    if (backlog == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_listen_impl((PySocketSockObject *)s, backlog);

exit:
    return return_value;
}

#endif /* defined(HAVE_LISTEN) */

PyDoc_STRVAR(_socket_socket_recv__doc__,
"recv($self, buffersize, flags=0, /)\n"
"--\n"
"\n"
"Receive up to buffersize bytes from the socket.\n"
"\n"
"For the optional flags argument, see the Unix manual. When no data\n"
"is available, block until at least one byte is available or until\n"
"the remote end is closed. When the remote end is closed and all data\n"
"is read, return the empty string.");

#define _SOCKET_SOCKET_RECV_METHODDEF    \
    {"recv", _PyCFunction_CAST(_socket_socket_recv), METH_FASTCALL, _socket_socket_recv__doc__},

static PyObject *
_socket_socket_recv_impl(PySocketSockObject *s, Py_ssize_t recvlen,
                         int flags);

static PyObject *
_socket_socket_recv(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t recvlen;
    int flags = 0;

    if (!_PyArg_CheckPositional("recv", nargs, 1, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        recvlen = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_recv_impl((PySocketSockObject *)s, recvlen, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_socket_recv_into__doc__,
"recv_into($self, /, buffer, nbytes=0, flags=0)\n"
"--\n"
"\n"
"Receive up to nbytes bytes from the socket, storing into a buffer.\n"
"\n"
"A version of recv() that stores its data into a buffer rather than\n"
"creating a new bytes object.  If nbytes is not specified (or 0),\n"
"receive up to the size available in the given buffer.\n"
"\n"
"See recv() for documentation about the flags.");

#define _SOCKET_SOCKET_RECV_INTO_METHODDEF    \
    {"recv_into", _PyCFunction_CAST(_socket_socket_recv_into), METH_FASTCALL|METH_KEYWORDS, _socket_socket_recv_into__doc__},

static PyObject *
_socket_socket_recv_into_impl(PySocketSockObject *s, Py_buffer *pbuf,
                              Py_ssize_t recvlen, int flags);

static PyObject *
_socket_socket_recv_into(PyObject *s, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(buffer), &_Py_ID(nbytes), &_Py_ID(flags), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"buffer", "nbytes", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "recv_into",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer pbuf = {NULL, NULL};
    Py_ssize_t recvlen = 0;
    int flags = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &pbuf, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("recv_into", "argument 'buffer'", "read-write bytes-like object", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            recvlen = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    flags = PyLong_AsInt(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _socket_socket_recv_into_impl((PySocketSockObject *)s, &pbuf, recvlen, flags);

exit:
    /* Cleanup for pbuf */
    if (pbuf.obj) {
       PyBuffer_Release(&pbuf);
    }

    return return_value;
}

#if defined(HAVE_RECVFROM)

PyDoc_STRVAR(_socket_socket_recvfrom__doc__,
"recvfrom($self, buffersize, flags=0, /)\n"
"--\n"
"\n"
"Receive up to buffersize bytes and the sender\'s address info.\n"
"\n"
"Like recv(buffersize, flags), but return a (data, address info)\n"
"pair.");

#define _SOCKET_SOCKET_RECVFROM_METHODDEF    \
    {"recvfrom", _PyCFunction_CAST(_socket_socket_recvfrom), METH_FASTCALL, _socket_socket_recvfrom__doc__},

static PyObject *
_socket_socket_recvfrom_impl(PySocketSockObject *s, Py_ssize_t recvlen,
                             int flags);

static PyObject *
_socket_socket_recvfrom(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t recvlen;
    int flags = 0;

    if (!_PyArg_CheckPositional("recvfrom", nargs, 1, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        recvlen = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_recvfrom_impl((PySocketSockObject *)s, recvlen, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_RECVFROM) */

#if defined(HAVE_RECVFROM)

PyDoc_STRVAR(_socket_socket_recvfrom_into__doc__,
"recvfrom_into($self, /, buffer, nbytes=0, flags=0)\n"
"--\n"
"\n"
"Receive up to nbytes bytes and the sender\'s address info.\n"
"\n"
"Like recv_into(buffer, nbytes, flags), but return a (nbytes, address\n"
"info) pair.");

#define _SOCKET_SOCKET_RECVFROM_INTO_METHODDEF    \
    {"recvfrom_into", _PyCFunction_CAST(_socket_socket_recvfrom_into), METH_FASTCALL|METH_KEYWORDS, _socket_socket_recvfrom_into__doc__},

static PyObject *
_socket_socket_recvfrom_into_impl(PySocketSockObject *s, Py_buffer *pbuf,
                                  Py_ssize_t recvlen, int flags);

static PyObject *
_socket_socket_recvfrom_into(PyObject *s, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(buffer), &_Py_ID(nbytes), &_Py_ID(flags), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"buffer", "nbytes", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "recvfrom_into",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer pbuf = {NULL, NULL};
    Py_ssize_t recvlen = 0;
    int flags = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &pbuf, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("recvfrom_into", "argument 'buffer'", "read-write bytes-like object", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            recvlen = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    flags = PyLong_AsInt(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _socket_socket_recvfrom_into_impl((PySocketSockObject *)s, &pbuf, recvlen, flags);

exit:
    /* Cleanup for pbuf */
    if (pbuf.obj) {
       PyBuffer_Release(&pbuf);
    }

    return return_value;
}

#endif /* defined(HAVE_RECVFROM) */

#if defined(CMSG_LEN)

PyDoc_STRVAR(_socket_socket_recvmsg__doc__,
"recvmsg($self, bufsize, ancbufsize=0, flags=0, /)\n"
"--\n"
"\n"
"Receive normal data and ancillary data from the socket.\n"
"\n"
"Receive up to bufsize bytes of normal data.  The ancbufsize argument\n"
"sets the size in bytes of the internal buffer used to receive the\n"
"ancillary data; it defaults to 0, meaning that no ancillary data\n"
"will be received.  Appropriate buffer sizes for ancillary data can\n"
"be calculated using CMSG_SPACE() or CMSG_LEN(), and items which do\n"
"not fit into the buffer might be truncated or discarded.  The flags\n"
"argument defaults to 0 and has the same meaning as for recv().\n"
"\n"
"The return value is a 4-tuple: (data, ancdata, msg_flags, address).\n"
"The data item is a bytes object holding the non-ancillary data\n"
"received.  The ancdata item is a list of zero or more tuples\n"
"(cmsg_level, cmsg_type, cmsg_data) representing the ancillary data\n"
"(control messages) received: cmsg_level and cmsg_type are integers\n"
"specifying the protocol level and protocol-specific type\n"
"respectively, and cmsg_data is a bytes object holding the associated\n"
"data.  The msg_flags item is the bitwise OR of various flags\n"
"indicating conditions on the received message; see your system\n"
"documentation for details.  If the receiving socket is unconnected,\n"
"address is the address of the sending socket, if available;\n"
"otherwise, its value is unspecified.\n"
"\n"
"If recvmsg() raises an exception after the system call returns, it\n"
"will first attempt to close any file descriptors received via the\n"
"SCM_RIGHTS mechanism.");

#define _SOCKET_SOCKET_RECVMSG_METHODDEF    \
    {"recvmsg", _PyCFunction_CAST(_socket_socket_recvmsg), METH_FASTCALL, _socket_socket_recvmsg__doc__},

static PyObject *
_socket_socket_recvmsg_impl(PySocketSockObject *s, Py_ssize_t bufsize,
                            Py_ssize_t ancbufsize, int flags);

static PyObject *
_socket_socket_recvmsg(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t bufsize;
    Py_ssize_t ancbufsize = 0;
    int flags = 0;

    if (!_PyArg_CheckPositional("recvmsg", nargs, 1, 3)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        bufsize = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        ancbufsize = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    flags = PyLong_AsInt(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_recvmsg_impl((PySocketSockObject *)s, bufsize, ancbufsize, flags);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) */

#if defined(CMSG_LEN)

PyDoc_STRVAR(_socket_socket_recvmsg_into__doc__,
"recvmsg_into($self, buffers, ancbufsize=0, flags=0, /)\n"
"--\n"
"\n"
"Receive normal and ancillary data, scattering the normal data.\n"
"\n"
"The buffers argument must be an iterable of objects that export\n"
"writable buffers (e.g.  bytearray objects); these will be filled\n"
"with successive chunks of the non-ancillary data until it has all\n"
"been written or there are no more buffers.  The ancbufsize argument\n"
"sets the size in bytes of the internal buffer used to receive the\n"
"ancillary data; it defaults to 0, meaning that no ancillary data\n"
"will be received.  Appropriate buffer sizes for ancillary data can\n"
"be calculated using CMSG_SPACE() or CMSG_LEN(), and items which do\n"
"not fit into the buffer might be truncated or discarded.  The flags\n"
"argument defaults to 0 and has the same meaning as for recv().\n"
"\n"
"The return value is a 4-tuple: (nbytes, ancdata, msg_flags,\n"
"address). The nbytes item is the total number of bytes of\n"
"non-ancillary data written into the buffers.  The ancdata item is a\n"
"list of zero or more tuples (cmsg_level, cmsg_type, cmsg_data)\n"
"representing the ancillary data (control messages) received:\n"
"cmsg_level and cmsg_type are integers specifying the protocol level\n"
"and protocol-specific type respectively, and cmsg_data is a bytes\n"
"object holding the associated data.  The msg_flags item is the\n"
"bitwise OR of various flags indicating conditions on the received\n"
"message; see your system documentation for details.  If the\n"
"receiving socket is unconnected, address is the address of the\n"
"sending socket, if available; otherwise, its value is unspecified.\n"
"\n"
"If recvmsg_into() raises an exception after the system call returns,\n"
"it will first attempt to close any file descriptors received via the\n"
"SCM_RIGHTS mechanism.");

#define _SOCKET_SOCKET_RECVMSG_INTO_METHODDEF    \
    {"recvmsg_into", _PyCFunction_CAST(_socket_socket_recvmsg_into), METH_FASTCALL, _socket_socket_recvmsg_into__doc__},

static PyObject *
_socket_socket_recvmsg_into_impl(PySocketSockObject *s,
                                 PyObject *buffers_arg,
                                 Py_ssize_t ancbufsize, int flags);

static PyObject *
_socket_socket_recvmsg_into(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *buffers_arg;
    Py_ssize_t ancbufsize = 0;
    int flags = 0;

    if (!_PyArg_CheckPositional("recvmsg_into", nargs, 1, 3)) {
        goto exit;
    }
    buffers_arg = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        ancbufsize = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    flags = PyLong_AsInt(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_recvmsg_into_impl((PySocketSockObject *)s, buffers_arg, ancbufsize, flags);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) */

PyDoc_STRVAR(_socket_socket_send__doc__,
"send($self, data, flags=0, /)\n"
"--\n"
"\n"
"Send a data string to the socket.\n"
"\n"
"For the optional flags argument, see the Unix manual.\n"
"Return the number of bytes sent; this may be less than len(data) if\n"
"the network is busy.");

#define _SOCKET_SOCKET_SEND_METHODDEF    \
    {"send", _PyCFunction_CAST(_socket_socket_send), METH_FASTCALL, _socket_socket_send__doc__},

static PyObject *
_socket_socket_send_impl(PySocketSockObject *s, Py_buffer *pbuf, int flags);

static PyObject *
_socket_socket_send(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer pbuf = {NULL, NULL};
    int flags = 0;

    if (!_PyArg_CheckPositional("send", nargs, 1, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &pbuf, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_send_impl((PySocketSockObject *)s, &pbuf, flags);

exit:
    /* Cleanup for pbuf */
    if (pbuf.obj) {
       PyBuffer_Release(&pbuf);
    }

    return return_value;
}

PyDoc_STRVAR(_socket_socket_sendall__doc__,
"sendall($self, data, flags=0, /)\n"
"--\n"
"\n"
"Send a data string to the socket.\n"
"\n"
"For the optional flags argument, see the Unix manual.\n"
"This calls send() repeatedly until all data is sent.\n"
"If an error occurs, it\'s impossible to tell how much data has been\n"
"sent.");

#define _SOCKET_SOCKET_SENDALL_METHODDEF    \
    {"sendall", _PyCFunction_CAST(_socket_socket_sendall), METH_FASTCALL, _socket_socket_sendall__doc__},

static PyObject *
_socket_socket_sendall_impl(PySocketSockObject *s, Py_buffer *pbuf,
                            int flags);

static PyObject *
_socket_socket_sendall(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer pbuf = {NULL, NULL};
    int flags = 0;

    if (!_PyArg_CheckPositional("sendall", nargs, 1, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &pbuf, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_sendall_impl((PySocketSockObject *)s, &pbuf, flags);

exit:
    /* Cleanup for pbuf */
    if (pbuf.obj) {
       PyBuffer_Release(&pbuf);
    }

    return return_value;
}

#if defined(CMSG_LEN)

PyDoc_STRVAR(_socket_socket_sendmsg__doc__,
"sendmsg($self, buffers, ancdata=(), flags=0, address=None, /)\n"
"--\n"
"\n"
"Send normal and ancillary data to the socket.\n"
"\n"
"It gathering the non-ancillary data from a series of buffers\n"
"and concatenating it into a single message.\n"
"The buffers argument specifies the non-ancillary\n"
"data as an iterable of bytes-like objects (e.g. bytes objects).\n"
"The ancdata argument specifies the ancillary data (control messages)\n"
"as an iterable of zero or more tuples (cmsg_level, cmsg_type,\n"
"cmsg_data), where cmsg_level and cmsg_type are integers specifying\n"
"the protocol level and protocol-specific type respectively, and\n"
"cmsg_data is a bytes-like object holding the associated data.  The\n"
"flags argument defaults to 0 and has the same meaning as for send().\n"
"If address is supplied and not None, it sets a destination address\n"
"for the message.  The return value is the number of bytes of\n"
"non-ancillary data sent.");

#define _SOCKET_SOCKET_SENDMSG_METHODDEF    \
    {"sendmsg", _PyCFunction_CAST(_socket_socket_sendmsg), METH_FASTCALL, _socket_socket_sendmsg__doc__},

static PyObject *
_socket_socket_sendmsg_impl(PySocketSockObject *s, PyObject *data_arg,
                            PyObject *cmsg_arg, int flags,
                            PyObject *addr_arg);

static PyObject *
_socket_socket_sendmsg(PyObject *s, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *data_arg;
    PyObject *cmsg_arg = NULL;
    int flags = 0;
    PyObject *addr_arg = NULL;

    if (!_PyArg_CheckPositional("sendmsg", nargs, 1, 4)) {
        goto exit;
    }
    data_arg = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    cmsg_arg = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    flags = PyLong_AsInt(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    addr_arg = args[3];
skip_optional:
    return_value = _socket_socket_sendmsg_impl((PySocketSockObject *)s, data_arg, cmsg_arg, flags, addr_arg);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) */

#if defined(HAVE_SOCKADDR_ALG)

PyDoc_STRVAR(_socket_socket_sendmsg_afalg__doc__,
"sendmsg_afalg($self, /, msg=<unrepresentable>, *, op=<unrepresentable>,\n"
"              iv=<unrepresentable>, assoclen=<unrepresentable>, flags=0)\n"
"--\n"
"\n"
"Set operation mode, IV and associated data length.\n"
"\n"
"For an AF_ALG operation socket.");

#define _SOCKET_SOCKET_SENDMSG_AFALG_METHODDEF    \
    {"sendmsg_afalg", _PyCFunction_CAST(_socket_socket_sendmsg_afalg), METH_FASTCALL|METH_KEYWORDS, _socket_socket_sendmsg_afalg__doc__},

static PyObject *
_socket_socket_sendmsg_afalg_impl(PySocketSockObject *self,
                                  PyObject *data_arg, PyObject *opobj,
                                  Py_buffer *iv, PyObject *assoclenobj,
                                  int flags);

static PyObject *
_socket_socket_sendmsg_afalg(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(msg), &_Py_ID(op), &_Py_ID(iv), &_Py_ID(assoclen), &_Py_ID(flags), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"msg", "op", "iv", "assoclen", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sendmsg_afalg",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_arg = NULL;
    PyObject *opobj = NULL;
    Py_buffer iv = {NULL, NULL};
    PyObject *assoclenobj = NULL;
    int flags = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_arg = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        if (!PyLong_Check(args[1])) {
            _PyArg_BadArgument("sendmsg_afalg", "argument 'op'", "int", args[1]);
            goto exit;
        }
        opobj = args[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        if (PyObject_GetBuffer(args[2], &iv, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        if (!PyLong_Check(args[3])) {
            _PyArg_BadArgument("sendmsg_afalg", "argument 'assoclen'", "int", args[3]);
            goto exit;
        }
        assoclenobj = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    flags = PyLong_AsInt(args[4]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _socket_socket_sendmsg_afalg_impl((PySocketSockObject *)self, data_arg, opobj, &iv, assoclenobj, flags);

exit:
    /* Cleanup for iv */
    if (iv.obj) {
       PyBuffer_Release(&iv);
    }

    return return_value;
}

#endif /* defined(HAVE_SOCKADDR_ALG) */

#if defined(HAVE_SHUTDOWN)

PyDoc_STRVAR(_socket_socket_shutdown__doc__,
"shutdown($self, flag, /)\n"
"--\n"
"\n"
"Shut down one or both halves of the connection.\n"
"\n"
"Shut down the reading side of the socket (flag == SHUT_RD), the\n"
"writing side of the socket (flag == SHUT_WR), or both ends (flag ==\n"
"SHUT_RDWR).");

#define _SOCKET_SOCKET_SHUTDOWN_METHODDEF    \
    {"shutdown", (PyCFunction)_socket_socket_shutdown, METH_O, _socket_socket_shutdown__doc__},

static PyObject *
_socket_socket_shutdown_impl(PySocketSockObject *s, PyObject *arg);

static PyObject *
_socket_socket_shutdown(PyObject *s, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = _socket_socket_shutdown_impl((PySocketSockObject *)s, arg);

    return return_value;
}

#endif /* defined(HAVE_SHUTDOWN) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_socket_socket_share__doc__,
"share($self, process_id, /)\n"
"--\n"
"\n"
"Share the socket with another process.\n"
"\n"
"The target process id must be provided and the resulting bytes\n"
"object passed to the target process.  There the shared socket can be\n"
"instantiated by calling socket.fromshare().");

#define _SOCKET_SOCKET_SHARE_METHODDEF    \
    {"share", (PyCFunction)_socket_socket_share, METH_O, _socket_socket_share__doc__},

static PyObject *
_socket_socket_share_impl(PySocketSockObject *s, unsigned int processId);

static PyObject *
_socket_socket_share(PyObject *s, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned int processId;

    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(arg, &processId, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    return_value = _socket_socket_share_impl((PySocketSockObject *)s, processId);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

static int
sock_initobj_impl(PySocketSockObject *self, int family, int type, int proto,
                  PyObject *fdobj);

static int
sock_initobj(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(family), &_Py_ID(type), &_Py_ID(proto), &_Py_ID(fileno), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"family", "type", "proto", "fileno", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "socket",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    int family = -1;
    int type = -1;
    int proto = -1;
    PyObject *fdobj = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        family = PyLong_AsInt(fastargs[0]);
        if (family == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        type = PyLong_AsInt(fastargs[1]);
        if (type == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        proto = PyLong_AsInt(fastargs[2]);
        if (proto == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    fdobj = fastargs[3];
skip_optional_pos:
    return_value = sock_initobj_impl((PySocketSockObject *)self, family, type, proto, fdobj);

exit:
    return return_value;
}

#if defined(HAVE_GETHOSTNAME)

PyDoc_STRVAR(_socket_gethostname__doc__,
"gethostname($module, /)\n"
"--\n"
"\n"
"Return the current host name.");

#define _SOCKET_GETHOSTNAME_METHODDEF    \
    {"gethostname", (PyCFunction)_socket_gethostname, METH_NOARGS, _socket_gethostname__doc__},

static PyObject *
_socket_gethostname_impl(PyObject *module);

static PyObject *
_socket_gethostname(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _socket_gethostname_impl(module);
}

#endif /* defined(HAVE_GETHOSTNAME) */

#if defined(HAVE_SETHOSTNAME)

PyDoc_STRVAR(_socket_sethostname__doc__,
"sethostname($module, name, /)\n"
"--\n"
"\n"
"Set the hostname to name.");

#define _SOCKET_SETHOSTNAME_METHODDEF    \
    {"sethostname", (PyCFunction)_socket_sethostname, METH_O, _socket_sethostname__doc__},

#endif /* defined(HAVE_SETHOSTNAME) */

#if defined(HAVE_GETADDRINFO)

PyDoc_STRVAR(_socket_gethostbyname__doc__,
"gethostbyname($module, host, /)\n"
"--\n"
"\n"
"Return the IP address for a host.\n"
"\n"
"The address is a string of the form \'255.255.255.255\'.");

#define _SOCKET_GETHOSTBYNAME_METHODDEF    \
    {"gethostbyname", (PyCFunction)_socket_gethostbyname, METH_O, _socket_gethostbyname__doc__},

#endif /* defined(HAVE_GETADDRINFO) */

#if (defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYNAME))

PyDoc_STRVAR(_socket_gethostbyname_ex__doc__,
"gethostbyname_ex($module, host, /)\n"
"--\n"
"\n"
"Map a host name to its IP number.\n"
"\n"
"Return the true host name, a list of aliases, and a list of IP\n"
"addresses, which are strings of the form \'255.255.255.255\'.");

#define _SOCKET_GETHOSTBYNAME_EX_METHODDEF    \
    {"gethostbyname_ex", (PyCFunction)_socket_gethostbyname_ex, METH_O, _socket_gethostbyname_ex__doc__},

#endif /* (defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYNAME)) */

#if (defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYADDR))

PyDoc_STRVAR(_socket_gethostbyaddr__doc__,
"gethostbyaddr($module, host, /)\n"
"--\n"
"\n"
"Map an IP number or host name to the true host name and addresses.\n"
"\n"
"Return the true host name, a list of aliases, and a list of IP\n"
"addresses.  The host argument is a string giving a host name or IP\n"
"number.");

#define _SOCKET_GETHOSTBYADDR_METHODDEF    \
    {"gethostbyaddr", (PyCFunction)_socket_gethostbyaddr, METH_O, _socket_gethostbyaddr__doc__},

#endif /* (defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYADDR)) */

#if defined(HAVE_GETSERVBYNAME)

PyDoc_STRVAR(_socket_getservbyname__doc__,
"getservbyname($module, servicename, protocolname=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return a port number from a service name and protocol name.\n"
"\n"
"The optional protocol name, if given, should be \'tcp\' or \'udp\',\n"
"otherwise any protocol will match.");

#define _SOCKET_GETSERVBYNAME_METHODDEF    \
    {"getservbyname", _PyCFunction_CAST(_socket_getservbyname), METH_FASTCALL, _socket_getservbyname__doc__},

static PyObject *
_socket_getservbyname_impl(PyObject *module, const char *name,
                           const char *proto);

static PyObject *
_socket_getservbyname(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *name;
    const char *proto = NULL;

    if (!_PyArg_CheckPositional("getservbyname", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("getservbyname", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(args[0], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("getservbyname", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t proto_length;
    proto = PyUnicode_AsUTF8AndSize(args[1], &proto_length);
    if (proto == NULL) {
        goto exit;
    }
    if (strlen(proto) != (size_t)proto_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional:
    return_value = _socket_getservbyname_impl(module, name, proto);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETSERVBYNAME) */

#if defined(HAVE_GETSERVBYPORT)

PyDoc_STRVAR(_socket_getservbyport__doc__,
"getservbyport($module, port, protocolname=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return the service name from a port number and protocol name.\n"
"\n"
"The optional protocol name, if given, should be \'tcp\' or \'udp\',\n"
"otherwise any protocol will match.");

#define _SOCKET_GETSERVBYPORT_METHODDEF    \
    {"getservbyport", _PyCFunction_CAST(_socket_getservbyport), METH_FASTCALL, _socket_getservbyport__doc__},

static PyObject *
_socket_getservbyport_impl(PyObject *module, int port, const char *proto);

static PyObject *
_socket_getservbyport(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int port;
    const char *proto = NULL;

    if (!_PyArg_CheckPositional("getservbyport", nargs, 1, 2)) {
        goto exit;
    }
    port = PyLong_AsInt(args[0]);
    if (port == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("getservbyport", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t proto_length;
    proto = PyUnicode_AsUTF8AndSize(args[1], &proto_length);
    if (proto == NULL) {
        goto exit;
    }
    if (strlen(proto) != (size_t)proto_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional:
    return_value = _socket_getservbyport_impl(module, port, proto);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETSERVBYPORT) */

#if defined(HAVE_GETPROTOBYNAME)

PyDoc_STRVAR(_socket_getprotobyname__doc__,
"getprotobyname($module, name, /)\n"
"--\n"
"\n"
"Return the protocol number for the named protocol.");

#define _SOCKET_GETPROTOBYNAME_METHODDEF    \
    {"getprotobyname", (PyCFunction)_socket_getprotobyname, METH_O, _socket_getprotobyname__doc__},

static PyObject *
_socket_getprotobyname_impl(PyObject *module, const char *name);

static PyObject *
_socket_getprotobyname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *name;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("getprotobyname", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(arg, &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _socket_getprotobyname_impl(module, name);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETPROTOBYNAME) */

PyDoc_STRVAR(_socket_close__doc__,
"close($module, fd, /)\n"
"--\n"
"\n"
"Close a socket fd.\n"
"\n"
"This is like os.close(), but for sockets; on some platforms os.close()\n"
"won\'t work for socket file descriptors.");

#define _SOCKET_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_socket_close, METH_O, _socket_close__doc__},

#if !defined(NO_DUP)

PyDoc_STRVAR(_socket_dup__doc__,
"dup($module, fd, /)\n"
"--\n"
"\n"
"Duplicate a socket descriptor.\n"
"\n"
"The new descriptor is non-inheritable.  This is like os.dup(), but for\n"
"sockets; on some platforms os.dup() won\'t work for socket file\n"
"descriptors.");

#define _SOCKET_DUP_METHODDEF    \
    {"dup", (PyCFunction)_socket_dup, METH_O, _socket_dup__doc__},

#endif /* !defined(NO_DUP) */

#if defined(HAVE_SOCKETPAIR)

PyDoc_STRVAR(_socket_socketpair__doc__,
"socketpair($module, family=AF_UNIX, type=SOCK_STREAM, proto=0, /)\n"
"--\n"
"\n"
"Create a pair of connected socket objects.\n"
"\n"
"The sockets are returned by the platform socketpair() function.  The\n"
"arguments are the same as for socket(), except that the default\n"
"family is AF_UNIX if defined on the platform; otherwise, the default\n"
"is AF_INET.");

#define _SOCKET_SOCKETPAIR_METHODDEF    \
    {"socketpair", _PyCFunction_CAST(_socket_socketpair), METH_FASTCALL, _socket_socketpair__doc__},

static PyObject *
_socket_socketpair_impl(PyObject *module, int family, int type, int proto);

static PyObject *
_socket_socketpair(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int family = SOCKETPAIR_DEFAULT_FAMILY;
    int type = SOCK_STREAM;
    int proto = 0;

    if (!_PyArg_CheckPositional("socketpair", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    family = PyLong_AsInt(args[0]);
    if (family == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    type = PyLong_AsInt(args[1]);
    if (type == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    proto = PyLong_AsInt(args[2]);
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socketpair_impl(module, family, type, proto);

exit:
    return return_value;
}

#endif /* defined(HAVE_SOCKETPAIR) */

PyDoc_STRVAR(_socket_ntohs__doc__,
"ntohs($module, integer, /)\n"
"--\n"
"\n"
"Convert a 16-bit unsigned integer from network to host byte order.");

#define _SOCKET_NTOHS_METHODDEF    \
    {"ntohs", (PyCFunction)_socket_ntohs, METH_O, _socket_ntohs__doc__},

static PyObject *
_socket_ntohs_impl(PyObject *module, uint16_t x);

static PyObject *
_socket_ntohs(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    uint16_t x;

    if (!_PyLong_UInt16_Converter(arg, &x)) {
        goto exit;
    }
    return_value = _socket_ntohs_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_ntohl__doc__,
"ntohl($module, integer, /)\n"
"--\n"
"\n"
"Convert a 32-bit unsigned integer from network to host byte order.");

#define _SOCKET_NTOHL_METHODDEF    \
    {"ntohl", (PyCFunction)_socket_ntohl, METH_O, _socket_ntohl__doc__},

static PyObject *
_socket_ntohl_impl(PyObject *module, uint32_t x);

static PyObject *
_socket_ntohl(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    uint32_t x;

    if (!_PyLong_UInt32_Converter(arg, &x)) {
        goto exit;
    }
    return_value = _socket_ntohl_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_htons__doc__,
"htons($module, integer, /)\n"
"--\n"
"\n"
"Convert a 16-bit unsigned integer from host to network byte order.");

#define _SOCKET_HTONS_METHODDEF    \
    {"htons", (PyCFunction)_socket_htons, METH_O, _socket_htons__doc__},

static PyObject *
_socket_htons_impl(PyObject *module, uint16_t x);

static PyObject *
_socket_htons(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    uint16_t x;

    if (!_PyLong_UInt16_Converter(arg, &x)) {
        goto exit;
    }
    return_value = _socket_htons_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_htonl__doc__,
"htonl($module, integer, /)\n"
"--\n"
"\n"
"Convert a 32-bit unsigned integer from host to network byte order.");

#define _SOCKET_HTONL_METHODDEF    \
    {"htonl", (PyCFunction)_socket_htonl, METH_O, _socket_htonl__doc__},

static PyObject *
_socket_htonl_impl(PyObject *module, uint32_t x);

static PyObject *
_socket_htonl(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    uint32_t x;

    if (!_PyLong_UInt32_Converter(arg, &x)) {
        goto exit;
    }
    return_value = _socket_htonl_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_inet_aton__doc__,
"inet_aton($module, ip_addr, /)\n"
"--\n"
"\n"
"Convert an IP address in string format (123.45.67.89) to the 32-bit packed binary format used in low-level network functions.");

#define _SOCKET_INET_ATON_METHODDEF    \
    {"inet_aton", (PyCFunction)_socket_inet_aton, METH_O, _socket_inet_aton__doc__},

static PyObject *
_socket_inet_aton_impl(PyObject *module, const char *ip_addr);

static PyObject *
_socket_inet_aton(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *ip_addr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("inet_aton", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t ip_addr_length;
    ip_addr = PyUnicode_AsUTF8AndSize(arg, &ip_addr_length);
    if (ip_addr == NULL) {
        goto exit;
    }
    if (strlen(ip_addr) != (size_t)ip_addr_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _socket_inet_aton_impl(module, ip_addr);

exit:
    return return_value;
}

#if defined(HAVE_INET_NTOA)

PyDoc_STRVAR(_socket_inet_ntoa__doc__,
"inet_ntoa($module, packed_ip, /)\n"
"--\n"
"\n"
"Convert an IP address from 32-bit packed binary format to string format.");

#define _SOCKET_INET_NTOA_METHODDEF    \
    {"inet_ntoa", (PyCFunction)_socket_inet_ntoa, METH_O, _socket_inet_ntoa__doc__},

static PyObject *
_socket_inet_ntoa_impl(PyObject *module, Py_buffer *packed_ip);

static PyObject *
_socket_inet_ntoa(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer packed_ip = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &packed_ip, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _socket_inet_ntoa_impl(module, &packed_ip);

exit:
    /* Cleanup for packed_ip */
    if (packed_ip.obj) {
       PyBuffer_Release(&packed_ip);
    }

    return return_value;
}

#endif /* defined(HAVE_INET_NTOA) */

#if defined(HAVE_INET_PTON)

PyDoc_STRVAR(_socket_inet_pton__doc__,
"inet_pton($module, af, ip, /)\n"
"--\n"
"\n"
"Convert an IP address from string format to a packed string.\n"
"\n"
"The string is suitable for use with low-level network functions.");

#define _SOCKET_INET_PTON_METHODDEF    \
    {"inet_pton", _PyCFunction_CAST(_socket_inet_pton), METH_FASTCALL, _socket_inet_pton__doc__},

static PyObject *
_socket_inet_pton_impl(PyObject *module, int af, const char *ip);

static PyObject *
_socket_inet_pton(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int af;
    const char *ip;

    if (!_PyArg_CheckPositional("inet_pton", nargs, 2, 2)) {
        goto exit;
    }
    af = PyLong_AsInt(args[0]);
    if (af == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("inet_pton", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t ip_length;
    ip = PyUnicode_AsUTF8AndSize(args[1], &ip_length);
    if (ip == NULL) {
        goto exit;
    }
    if (strlen(ip) != (size_t)ip_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _socket_inet_pton_impl(module, af, ip);

exit:
    return return_value;
}

#endif /* defined(HAVE_INET_PTON) */

#if defined(HAVE_INET_PTON)

PyDoc_STRVAR(_socket_inet_ntop__doc__,
"inet_ntop($module, af, packed_ip, /)\n"
"--\n"
"\n"
"Convert a packed IP address of the given family to string format.");

#define _SOCKET_INET_NTOP_METHODDEF    \
    {"inet_ntop", _PyCFunction_CAST(_socket_inet_ntop), METH_FASTCALL, _socket_inet_ntop__doc__},

static PyObject *
_socket_inet_ntop_impl(PyObject *module, int af, Py_buffer *packed_ip);

static PyObject *
_socket_inet_ntop(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int af;
    Py_buffer packed_ip = {NULL, NULL};

    if (!_PyArg_CheckPositional("inet_ntop", nargs, 2, 2)) {
        goto exit;
    }
    af = PyLong_AsInt(args[0]);
    if (af == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &packed_ip, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _socket_inet_ntop_impl(module, af, &packed_ip);

exit:
    /* Cleanup for packed_ip */
    if (packed_ip.obj) {
       PyBuffer_Release(&packed_ip);
    }

    return return_value;
}

#endif /* defined(HAVE_INET_PTON) */

#if defined(HAVE_GETADDRINFO)

PyDoc_STRVAR(_socket_getaddrinfo__doc__,
"getaddrinfo($module, /, host, port, family=AF_UNSPEC, type=0, proto=0,\n"
"            flags=0)\n"
"--\n"
"\n"
"Resolve host and port into a list of 5-tuples.\n"
"\n"
"Each tuple is (family, type, proto, canonname, sockaddr).");

#define _SOCKET_GETADDRINFO_METHODDEF    \
    {"getaddrinfo", _PyCFunction_CAST(_socket_getaddrinfo), METH_FASTCALL|METH_KEYWORDS, _socket_getaddrinfo__doc__},

static PyObject *
_socket_getaddrinfo_impl(PyObject *module, PyObject *hobj, PyObject *pobj,
                         int family, int socktype, int protocol, int flags);

static PyObject *
_socket_getaddrinfo(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(host), &_Py_ID(port), &_Py_ID(family), &_Py_ID(type), &_Py_ID(proto), &_Py_ID(flags), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"host", "port", "family", "type", "proto", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getaddrinfo",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *hobj;
    PyObject *pobj;
    int family = AF_UNSPEC;
    int socktype = 0;
    int protocol = 0;
    int flags = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    hobj = args[0];
    pobj = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        family = PyLong_AsInt(args[2]);
        if (family == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        socktype = PyLong_AsInt(args[3]);
        if (socktype == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[4]) {
        protocol = PyLong_AsInt(args[4]);
        if (protocol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    flags = PyLong_AsInt(args[5]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _socket_getaddrinfo_impl(module, hobj, pobj, family, socktype, protocol, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETADDRINFO) */

#if defined(HAVE_GETNAMEINFO)

PyDoc_STRVAR(_socket_getnameinfo__doc__,
"getnameinfo($module, sockaddr, flags, /)\n"
"--\n"
"\n"
"Get host and port for a sockaddr.");

#define _SOCKET_GETNAMEINFO_METHODDEF    \
    {"getnameinfo", _PyCFunction_CAST(_socket_getnameinfo), METH_FASTCALL, _socket_getnameinfo__doc__},

static PyObject *
_socket_getnameinfo_impl(PyObject *module, PyObject *sa, int flags);

static PyObject *
_socket_getnameinfo(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sa;
    int flags;

    if (!_PyArg_CheckPositional("getnameinfo", nargs, 2, 2)) {
        goto exit;
    }
    sa = args[0];
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _socket_getnameinfo_impl(module, sa, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETNAMEINFO) */

PyDoc_STRVAR(_socket_getdefaulttimeout__doc__,
"getdefaulttimeout($module, /)\n"
"--\n"
"\n"
"Return the default timeout in seconds for new socket objects.\n"
"\n"
"A value of None indicates that new socket objects have no timeout.\n"
"When the socket module is first imported, the default is None.");

#define _SOCKET_GETDEFAULTTIMEOUT_METHODDEF    \
    {"getdefaulttimeout", (PyCFunction)_socket_getdefaulttimeout, METH_NOARGS, _socket_getdefaulttimeout__doc__},

static PyObject *
_socket_getdefaulttimeout_impl(PyObject *module);

static PyObject *
_socket_getdefaulttimeout(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _socket_getdefaulttimeout_impl(module);
}

PyDoc_STRVAR(_socket_setdefaulttimeout__doc__,
"setdefaulttimeout($module, timeout, /)\n"
"--\n"
"\n"
"Set the default timeout in seconds for new socket objects.\n"
"\n"
"A value of None indicates that new socket objects have no timeout.\n"
"When the socket module is first imported, the default is None.");

#define _SOCKET_SETDEFAULTTIMEOUT_METHODDEF    \
    {"setdefaulttimeout", (PyCFunction)_socket_setdefaulttimeout, METH_O, _socket_setdefaulttimeout__doc__},

#if (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS))

PyDoc_STRVAR(_socket_if_nameindex__doc__,
"if_nameindex($module, /)\n"
"--\n"
"\n"
"Return a list of network interface information (index, name) tuples.");

#define _SOCKET_IF_NAMEINDEX_METHODDEF    \
    {"if_nameindex", (PyCFunction)_socket_if_nameindex, METH_NOARGS, _socket_if_nameindex__doc__},

static PyObject *
_socket_if_nameindex_impl(PyObject *module);

static PyObject *
_socket_if_nameindex(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _socket_if_nameindex_impl(module);
}

#endif /* (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS)) */

#if (defined(HAVE_IF_NAMETOINDEX) || defined(MS_WINDOWS))

PyDoc_STRVAR(_socket_if_nametoindex__doc__,
"if_nametoindex($module, oname, /)\n"
"--\n"
"\n"
"Returns the interface index corresponding to the interface name if_name.");

#define _SOCKET_IF_NAMETOINDEX_METHODDEF    \
    {"if_nametoindex", (PyCFunction)_socket_if_nametoindex, METH_O, _socket_if_nametoindex__doc__},

static PyObject *
_socket_if_nametoindex_impl(PyObject *module, PyObject *oname);

static PyObject *
_socket_if_nametoindex(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *oname = NULL;

    if (!PyUnicode_FSConverter(arg, &oname)) {
        goto exit;
    }
    return_value = _socket_if_nametoindex_impl(module, oname);

exit:
    /* Cleanup for oname */
    Py_XDECREF(oname);

    return return_value;
}

#endif /* (defined(HAVE_IF_NAMETOINDEX) || defined(MS_WINDOWS)) */

#if (defined(HAVE_IF_INDEXTONAME) || defined(MS_WINDOWS))

PyDoc_STRVAR(_socket_if_indextoname__doc__,
"if_indextoname($module, if_index, /)\n"
"--\n"
"\n"
"Returns the interface name corresponding to the interface index if_index.");

#define _SOCKET_IF_INDEXTONAME_METHODDEF    \
    {"if_indextoname", (PyCFunction)_socket_if_indextoname, METH_O, _socket_if_indextoname__doc__},

static PyObject *
_socket_if_indextoname_impl(PyObject *module, NET_IFINDEX index);

static PyObject *
_socket_if_indextoname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    NET_IFINDEX index;

    if (!_PyLong_NetIfindex_Converter(arg, &index)) {
        goto exit;
    }
    return_value = _socket_if_indextoname_impl(module, index);

exit:
    return return_value;
}

#endif /* (defined(HAVE_IF_INDEXTONAME) || defined(MS_WINDOWS)) */

#if defined(CMSG_LEN)

PyDoc_STRVAR(_socket_CMSG_LEN__doc__,
"CMSG_LEN($module, length, /)\n"
"--\n"
"\n"
"Return the total length of an ancillary data item with associated data.\n"
"\n"
"The associated data has the given length.  This value can often be\n"
"used as the buffer size for recvmsg() to receive a single item of\n"
"ancillary data, but RFC 3542 requires portable applications to use\n"
"CMSG_SPACE() and thus include space for padding, even when the item\n"
"will be the last in the buffer.  Raises OverflowError if length is\n"
"outside the permissible range of values.");

#define _SOCKET_CMSG_LEN_METHODDEF    \
    {"CMSG_LEN", (PyCFunction)_socket_CMSG_LEN, METH_O, _socket_CMSG_LEN__doc__},

static PyObject *
_socket_CMSG_LEN_impl(PyObject *module, Py_ssize_t length);

static PyObject *
_socket_CMSG_LEN(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t length;

    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    return_value = _socket_CMSG_LEN_impl(module, length);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) */

#if defined(CMSG_LEN) && defined(CMSG_SPACE)

PyDoc_STRVAR(_socket_CMSG_SPACE__doc__,
"CMSG_SPACE($module, length, /)\n"
"--\n"
"\n"
"Return the buffer size needed to receive an ancillary data item.\n"
"\n"
"The item has associated data of the given length, and the size\n"
"includes any trailing padding.  The buffer space needed to receive\n"
"multiple items is the sum of the CMSG_SPACE() values for their\n"
"associated data lengths.  Raises OverflowError if length is outside\n"
"the permissible range of values.");

#define _SOCKET_CMSG_SPACE_METHODDEF    \
    {"CMSG_SPACE", (PyCFunction)_socket_CMSG_SPACE, METH_O, _socket_CMSG_SPACE__doc__},

static PyObject *
_socket_CMSG_SPACE_impl(PyObject *module, Py_ssize_t length);

static PyObject *
_socket_CMSG_SPACE(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t length;

    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    return_value = _socket_CMSG_SPACE_impl(module, length);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) && defined(CMSG_SPACE) */

#ifndef _SOCKET_SOCKET__ACCEPT_METHODDEF
    #define _SOCKET_SOCKET__ACCEPT_METHODDEF
#endif /* !defined(_SOCKET_SOCKET__ACCEPT_METHODDEF) */

#ifndef _SOCKET_SOCKET_BIND_METHODDEF
    #define _SOCKET_SOCKET_BIND_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_BIND_METHODDEF) */

#ifndef _SOCKET_SOCKET_CONNECT_METHODDEF
    #define _SOCKET_SOCKET_CONNECT_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_CONNECT_METHODDEF) */

#ifndef _SOCKET_SOCKET_CONNECT_EX_METHODDEF
    #define _SOCKET_SOCKET_CONNECT_EX_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_CONNECT_EX_METHODDEF) */

#ifndef _SOCKET_SOCKET_GETSOCKNAME_METHODDEF
    #define _SOCKET_SOCKET_GETSOCKNAME_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_GETSOCKNAME_METHODDEF) */

#ifndef _SOCKET_SOCKET_GETPEERNAME_METHODDEF
    #define _SOCKET_SOCKET_GETPEERNAME_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_GETPEERNAME_METHODDEF) */

#ifndef _SOCKET_SOCKET_LISTEN_METHODDEF
    #define _SOCKET_SOCKET_LISTEN_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_LISTEN_METHODDEF) */

#ifndef _SOCKET_SOCKET_RECVFROM_METHODDEF
    #define _SOCKET_SOCKET_RECVFROM_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_RECVFROM_METHODDEF) */

#ifndef _SOCKET_SOCKET_RECVFROM_INTO_METHODDEF
    #define _SOCKET_SOCKET_RECVFROM_INTO_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_RECVFROM_INTO_METHODDEF) */

#ifndef _SOCKET_SOCKET_RECVMSG_METHODDEF
    #define _SOCKET_SOCKET_RECVMSG_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_RECVMSG_METHODDEF) */

#ifndef _SOCKET_SOCKET_RECVMSG_INTO_METHODDEF
    #define _SOCKET_SOCKET_RECVMSG_INTO_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_RECVMSG_INTO_METHODDEF) */

#ifndef _SOCKET_SOCKET_SENDMSG_METHODDEF
    #define _SOCKET_SOCKET_SENDMSG_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_SENDMSG_METHODDEF) */

#ifndef _SOCKET_SOCKET_SENDMSG_AFALG_METHODDEF
    #define _SOCKET_SOCKET_SENDMSG_AFALG_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_SENDMSG_AFALG_METHODDEF) */

#ifndef _SOCKET_SOCKET_SHUTDOWN_METHODDEF
    #define _SOCKET_SOCKET_SHUTDOWN_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_SHUTDOWN_METHODDEF) */

#ifndef _SOCKET_SOCKET_SHARE_METHODDEF
    #define _SOCKET_SOCKET_SHARE_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_SHARE_METHODDEF) */

#ifndef _SOCKET_GETHOSTNAME_METHODDEF
    #define _SOCKET_GETHOSTNAME_METHODDEF
#endif /* !defined(_SOCKET_GETHOSTNAME_METHODDEF) */

#ifndef _SOCKET_SETHOSTNAME_METHODDEF
    #define _SOCKET_SETHOSTNAME_METHODDEF
#endif /* !defined(_SOCKET_SETHOSTNAME_METHODDEF) */

#ifndef _SOCKET_GETHOSTBYNAME_METHODDEF
    #define _SOCKET_GETHOSTBYNAME_METHODDEF
#endif /* !defined(_SOCKET_GETHOSTBYNAME_METHODDEF) */

#ifndef _SOCKET_GETHOSTBYNAME_EX_METHODDEF
    #define _SOCKET_GETHOSTBYNAME_EX_METHODDEF
#endif /* !defined(_SOCKET_GETHOSTBYNAME_EX_METHODDEF) */

#ifndef _SOCKET_GETHOSTBYADDR_METHODDEF
    #define _SOCKET_GETHOSTBYADDR_METHODDEF
#endif /* !defined(_SOCKET_GETHOSTBYADDR_METHODDEF) */

#ifndef _SOCKET_GETSERVBYNAME_METHODDEF
    #define _SOCKET_GETSERVBYNAME_METHODDEF
#endif /* !defined(_SOCKET_GETSERVBYNAME_METHODDEF) */

#ifndef _SOCKET_GETSERVBYPORT_METHODDEF
    #define _SOCKET_GETSERVBYPORT_METHODDEF
#endif /* !defined(_SOCKET_GETSERVBYPORT_METHODDEF) */

#ifndef _SOCKET_GETPROTOBYNAME_METHODDEF
    #define _SOCKET_GETPROTOBYNAME_METHODDEF
#endif /* !defined(_SOCKET_GETPROTOBYNAME_METHODDEF) */

#ifndef _SOCKET_DUP_METHODDEF
    #define _SOCKET_DUP_METHODDEF
#endif /* !defined(_SOCKET_DUP_METHODDEF) */

#ifndef _SOCKET_SOCKETPAIR_METHODDEF
    #define _SOCKET_SOCKETPAIR_METHODDEF
#endif /* !defined(_SOCKET_SOCKETPAIR_METHODDEF) */

#ifndef _SOCKET_INET_NTOA_METHODDEF
    #define _SOCKET_INET_NTOA_METHODDEF
#endif /* !defined(_SOCKET_INET_NTOA_METHODDEF) */

#ifndef _SOCKET_INET_PTON_METHODDEF
    #define _SOCKET_INET_PTON_METHODDEF
#endif /* !defined(_SOCKET_INET_PTON_METHODDEF) */

#ifndef _SOCKET_INET_NTOP_METHODDEF
    #define _SOCKET_INET_NTOP_METHODDEF
#endif /* !defined(_SOCKET_INET_NTOP_METHODDEF) */

#ifndef _SOCKET_GETADDRINFO_METHODDEF
    #define _SOCKET_GETADDRINFO_METHODDEF
#endif /* !defined(_SOCKET_GETADDRINFO_METHODDEF) */

#ifndef _SOCKET_GETNAMEINFO_METHODDEF
    #define _SOCKET_GETNAMEINFO_METHODDEF
#endif /* !defined(_SOCKET_GETNAMEINFO_METHODDEF) */

#ifndef _SOCKET_IF_NAMEINDEX_METHODDEF
    #define _SOCKET_IF_NAMEINDEX_METHODDEF
#endif /* !defined(_SOCKET_IF_NAMEINDEX_METHODDEF) */

#ifndef _SOCKET_IF_NAMETOINDEX_METHODDEF
    #define _SOCKET_IF_NAMETOINDEX_METHODDEF
#endif /* !defined(_SOCKET_IF_NAMETOINDEX_METHODDEF) */

#ifndef _SOCKET_IF_INDEXTONAME_METHODDEF
    #define _SOCKET_IF_INDEXTONAME_METHODDEF
#endif /* !defined(_SOCKET_IF_INDEXTONAME_METHODDEF) */

#ifndef _SOCKET_CMSG_LEN_METHODDEF
    #define _SOCKET_CMSG_LEN_METHODDEF
#endif /* !defined(_SOCKET_CMSG_LEN_METHODDEF) */

#ifndef _SOCKET_CMSG_SPACE_METHODDEF
    #define _SOCKET_CMSG_SPACE_METHODDEF
#endif /* !defined(_SOCKET_CMSG_SPACE_METHODDEF) */
/*[clinic end generated code: output=aa9082b592c39fa5 input=a9049054013a1b77]*/
