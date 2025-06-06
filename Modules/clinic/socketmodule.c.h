/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_long.h"          // _PyLong_UInt16_Converter()
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

#if (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS))

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
    PyObject *oname;

    if (!PyUnicode_FSConverter(arg, &oname)) {
        goto exit;
    }
    return_value = _socket_if_nametoindex_impl(module, oname);

exit:
    return return_value;
}

#endif /* (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS)) */

#if (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS))

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

#endif /* (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS)) */

#ifndef _SOCKET_INET_NTOA_METHODDEF
    #define _SOCKET_INET_NTOA_METHODDEF
#endif /* !defined(_SOCKET_INET_NTOA_METHODDEF) */

#ifndef _SOCKET_IF_NAMETOINDEX_METHODDEF
    #define _SOCKET_IF_NAMETOINDEX_METHODDEF
#endif /* !defined(_SOCKET_IF_NAMETOINDEX_METHODDEF) */

#ifndef _SOCKET_IF_INDEXTONAME_METHODDEF
    #define _SOCKET_IF_INDEXTONAME_METHODDEF
#endif /* !defined(_SOCKET_IF_INDEXTONAME_METHODDEF) */
/*[clinic end generated code: output=07776dd21d1e3b56 input=a9049054013a1b77]*/
