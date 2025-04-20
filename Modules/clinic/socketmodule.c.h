/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

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

PyDoc_STRVAR(_socket_socket_ntohs__doc__,
"ntohs($self, x, /)\n"
"--\n"
"\n"
"Convert a 16-bit unsigned integer from network to host byte order.");

#define _SOCKET_SOCKET_NTOHS_METHODDEF    \
    {"ntohs", (PyCFunction)_socket_socket_ntohs, METH_O, _socket_socket_ntohs__doc__},

static PyObject *
_socket_socket_ntohs_impl(PySocketSockObject *self, int x);

static PyObject *
_socket_socket_ntohs(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int x;

    x = PyLong_AsInt(arg);
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _socket_socket_ntohs_impl((PySocketSockObject *)self, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_socket_htons__doc__,
"htons($self, x, /)\n"
"--\n"
"\n"
"Convert a 16-bit unsigned integer from host to network byte order.");

#define _SOCKET_SOCKET_HTONS_METHODDEF    \
    {"htons", (PyCFunction)_socket_socket_htons, METH_O, _socket_socket_htons__doc__},

static PyObject *
_socket_socket_htons_impl(PySocketSockObject *self, int x);

static PyObject *
_socket_socket_htons(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int x;

    x = PyLong_AsInt(arg);
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _socket_socket_htons_impl((PySocketSockObject *)self, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_socket_inet_aton__doc__,
"inet_aton($self, ip_addr, /)\n"
"--\n"
"\n"
"Convert an IP address in string format (123.45.67.89) to the 32-bit packed binary format used in low-level network functions.");

#define _SOCKET_SOCKET_INET_ATON_METHODDEF    \
    {"inet_aton", (PyCFunction)_socket_socket_inet_aton, METH_O, _socket_socket_inet_aton__doc__},

static PyObject *
_socket_socket_inet_aton_impl(PySocketSockObject *self, const char *ip_addr);

static PyObject *
_socket_socket_inet_aton(PyObject *self, PyObject *arg)
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
    return_value = _socket_socket_inet_aton_impl((PySocketSockObject *)self, ip_addr);

exit:
    return return_value;
}

#if defined(HAVE_INET_NTOA)

PyDoc_STRVAR(_socket_socket_inet_ntoa__doc__,
"inet_ntoa($self, packed_ip, /)\n"
"--\n"
"\n"
"Convert an IP address from 32-bit packed binary format to string format.");

#define _SOCKET_SOCKET_INET_NTOA_METHODDEF    \
    {"inet_ntoa", (PyCFunction)_socket_socket_inet_ntoa, METH_O, _socket_socket_inet_ntoa__doc__},

static PyObject *
_socket_socket_inet_ntoa_impl(PySocketSockObject *self, Py_buffer *packed_ip);

static PyObject *
_socket_socket_inet_ntoa(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer packed_ip = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &packed_ip, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _socket_socket_inet_ntoa_impl((PySocketSockObject *)self, &packed_ip);

exit:
    /* Cleanup for packed_ip */
    if (packed_ip.obj) {
       PyBuffer_Release(&packed_ip);
    }

    return return_value;
}

#endif /* defined(HAVE_INET_NTOA) */

#if (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS))

PyDoc_STRVAR(_socket_socket_if_nametoindex__doc__,
"if_nametoindex($self, oname, /)\n"
"--\n"
"\n"
"Returns the interface index corresponding to the interface name if_name.");

#define _SOCKET_SOCKET_IF_NAMETOINDEX_METHODDEF    \
    {"if_nametoindex", (PyCFunction)_socket_socket_if_nametoindex, METH_O, _socket_socket_if_nametoindex__doc__},

static PyObject *
_socket_socket_if_nametoindex_impl(PySocketSockObject *self, PyObject *oname);

static PyObject *
_socket_socket_if_nametoindex(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *oname;

    if (!PyUnicode_FSConverter(arg, &oname)) {
        goto exit;
    }
    return_value = _socket_socket_if_nametoindex_impl((PySocketSockObject *)self, oname);

exit:
    return return_value;
}

#endif /* (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS)) */

#ifndef _SOCKET_SOCKET_INET_NTOA_METHODDEF
    #define _SOCKET_SOCKET_INET_NTOA_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_INET_NTOA_METHODDEF) */

#ifndef _SOCKET_SOCKET_IF_NAMETOINDEX_METHODDEF
    #define _SOCKET_SOCKET_IF_NAMETOINDEX_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_IF_NAMETOINDEX_METHODDEF) */
/*[clinic end generated code: output=27bc54006551ab0c input=a9049054013a1b77]*/
