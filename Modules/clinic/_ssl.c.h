/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_ssl__SSLSocket_do_handshake__doc__,
"do_handshake($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_DO_HANDSHAKE_METHODDEF    \
    {"do_handshake", (PyCFunction)_ssl__SSLSocket_do_handshake, METH_NOARGS, _ssl__SSLSocket_do_handshake__doc__},

static PyObject *
_ssl__SSLSocket_do_handshake_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_do_handshake(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_do_handshake_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__test_decode_cert__doc__,
"_test_decode_cert($module, path, /)\n"
"--\n"
"\n");

#define _SSL__TEST_DECODE_CERT_METHODDEF    \
    {"_test_decode_cert", (PyCFunction)_ssl__test_decode_cert, METH_O, _ssl__test_decode_cert__doc__},

static PyObject *
_ssl__test_decode_cert_impl(PyObject *module, PyObject *path);

static PyObject *
_ssl__test_decode_cert(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *path;

    if (!PyUnicode_FSConverter(arg, &path)) {
        goto exit;
    }
    return_value = _ssl__test_decode_cert_impl(module, path);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_getpeercert__doc__,
"getpeercert($self, der=False, /)\n"
"--\n"
"\n"
"Returns the certificate for the peer.\n"
"\n"
"If no certificate was provided, returns None.  If a certificate was\n"
"provided, but not validated, returns an empty dictionary.  Otherwise\n"
"returns a dict containing information about the peer certificate.\n"
"\n"
"If the optional argument is True, returns a DER-encoded copy of the\n"
"peer certificate, or None if no certificate was provided.  This will\n"
"return the certificate even if it wasn\'t validated.");

#define _SSL__SSLSOCKET_GETPEERCERT_METHODDEF    \
    {"getpeercert", _PyCFunction_CAST(_ssl__SSLSocket_getpeercert), METH_FASTCALL, _ssl__SSLSocket_getpeercert__doc__},

static PyObject *
_ssl__SSLSocket_getpeercert_impl(PySSLSocket *self, int binary_mode);

static PyObject *
_ssl__SSLSocket_getpeercert(PySSLSocket *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int binary_mode = 0;

    if (!_PyArg_CheckPositional("getpeercert", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    binary_mode = PyObject_IsTrue(args[0]);
    if (binary_mode < 0) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_getpeercert_impl(self, binary_mode);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_get_verified_chain__doc__,
"get_verified_chain($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_GET_VERIFIED_CHAIN_METHODDEF    \
    {"get_verified_chain", (PyCFunction)_ssl__SSLSocket_get_verified_chain, METH_NOARGS, _ssl__SSLSocket_get_verified_chain__doc__},

static PyObject *
_ssl__SSLSocket_get_verified_chain_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_get_verified_chain(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_get_verified_chain_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_get_unverified_chain__doc__,
"get_unverified_chain($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_GET_UNVERIFIED_CHAIN_METHODDEF    \
    {"get_unverified_chain", (PyCFunction)_ssl__SSLSocket_get_unverified_chain, METH_NOARGS, _ssl__SSLSocket_get_unverified_chain__doc__},

static PyObject *
_ssl__SSLSocket_get_unverified_chain_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_get_unverified_chain(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_get_unverified_chain_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_shared_ciphers__doc__,
"shared_ciphers($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_SHARED_CIPHERS_METHODDEF    \
    {"shared_ciphers", (PyCFunction)_ssl__SSLSocket_shared_ciphers, METH_NOARGS, _ssl__SSLSocket_shared_ciphers__doc__},

static PyObject *
_ssl__SSLSocket_shared_ciphers_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_shared_ciphers(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_shared_ciphers_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_cipher__doc__,
"cipher($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_CIPHER_METHODDEF    \
    {"cipher", (PyCFunction)_ssl__SSLSocket_cipher, METH_NOARGS, _ssl__SSLSocket_cipher__doc__},

static PyObject *
_ssl__SSLSocket_cipher_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_cipher(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_cipher_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_version__doc__,
"version($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_VERSION_METHODDEF    \
    {"version", (PyCFunction)_ssl__SSLSocket_version, METH_NOARGS, _ssl__SSLSocket_version__doc__},

static PyObject *
_ssl__SSLSocket_version_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_version(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_version_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_selected_alpn_protocol__doc__,
"selected_alpn_protocol($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_SELECTED_ALPN_PROTOCOL_METHODDEF    \
    {"selected_alpn_protocol", (PyCFunction)_ssl__SSLSocket_selected_alpn_protocol, METH_NOARGS, _ssl__SSLSocket_selected_alpn_protocol__doc__},

static PyObject *
_ssl__SSLSocket_selected_alpn_protocol_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_selected_alpn_protocol(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_selected_alpn_protocol_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_compression__doc__,
"compression($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_COMPRESSION_METHODDEF    \
    {"compression", (PyCFunction)_ssl__SSLSocket_compression, METH_NOARGS, _ssl__SSLSocket_compression__doc__},

static PyObject *
_ssl__SSLSocket_compression_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_compression(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    return _ssl__SSLSocket_compression_impl(self);
}

PyDoc_STRVAR(_ssl__SSLSocket_context__doc__,
"This changes the context associated with the SSLSocket.\n"
"\n"
"This is typically used from within a callback function set by the sni_callback\n"
"on the SSLContext to change the certificate information associated with the\n"
"SSLSocket before the cryptographic exchange handshake messages.");
#if defined(_ssl__SSLSocket_context_DOCSTR)
#   undef _ssl__SSLSocket_context_DOCSTR
#endif
#define _ssl__SSLSocket_context_DOCSTR _ssl__SSLSocket_context__doc__

#if !defined(_ssl__SSLSocket_context_DOCSTR)
#  define _ssl__SSLSocket_context_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_CONTEXT_GETSETDEF)
#  undef _SSL__SSLSOCKET_CONTEXT_GETSETDEF
#  define _SSL__SSLSOCKET_CONTEXT_GETSETDEF {"context", (getter)_ssl__SSLSocket_context_get, (setter)_ssl__SSLSocket_context_set, _ssl__SSLSocket_context_DOCSTR},
#else
#  define _SSL__SSLSOCKET_CONTEXT_GETSETDEF {"context", (getter)_ssl__SSLSocket_context_get, NULL, _ssl__SSLSocket_context_DOCSTR},
#endif

static PyObject *
_ssl__SSLSocket_context_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_context_get(PySSLSocket *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_context_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLSocket_context_DOCSTR)
#  define _ssl__SSLSocket_context_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_CONTEXT_GETSETDEF)
#  undef _SSL__SSLSOCKET_CONTEXT_GETSETDEF
#  define _SSL__SSLSOCKET_CONTEXT_GETSETDEF {"context", (getter)_ssl__SSLSocket_context_get, (setter)_ssl__SSLSocket_context_set, _ssl__SSLSocket_context_DOCSTR},
#else
#  define _SSL__SSLSOCKET_CONTEXT_GETSETDEF {"context", NULL, (setter)_ssl__SSLSocket_context_set, NULL},
#endif

static int
_ssl__SSLSocket_context_set_impl(PySSLSocket *self, PyObject *value);

static int
_ssl__SSLSocket_context_set(PySSLSocket *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_context_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_server_side__doc__,
"Whether this is a server-side socket.");
#if defined(_ssl__SSLSocket_server_side_DOCSTR)
#   undef _ssl__SSLSocket_server_side_DOCSTR
#endif
#define _ssl__SSLSocket_server_side_DOCSTR _ssl__SSLSocket_server_side__doc__

#if !defined(_ssl__SSLSocket_server_side_DOCSTR)
#  define _ssl__SSLSocket_server_side_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_SERVER_SIDE_GETSETDEF)
#  undef _SSL__SSLSOCKET_SERVER_SIDE_GETSETDEF
#  define _SSL__SSLSOCKET_SERVER_SIDE_GETSETDEF {"server_side", (getter)_ssl__SSLSocket_server_side_get, (setter)_ssl__SSLSocket_server_side_set, _ssl__SSLSocket_server_side_DOCSTR},
#else
#  define _SSL__SSLSOCKET_SERVER_SIDE_GETSETDEF {"server_side", (getter)_ssl__SSLSocket_server_side_get, NULL, _ssl__SSLSocket_server_side_DOCSTR},
#endif

static PyObject *
_ssl__SSLSocket_server_side_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_server_side_get(PySSLSocket *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_server_side_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_server_hostname__doc__,
"The currently set server hostname (for SNI).");
#if defined(_ssl__SSLSocket_server_hostname_DOCSTR)
#   undef _ssl__SSLSocket_server_hostname_DOCSTR
#endif
#define _ssl__SSLSocket_server_hostname_DOCSTR _ssl__SSLSocket_server_hostname__doc__

#if !defined(_ssl__SSLSocket_server_hostname_DOCSTR)
#  define _ssl__SSLSocket_server_hostname_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_SERVER_HOSTNAME_GETSETDEF)
#  undef _SSL__SSLSOCKET_SERVER_HOSTNAME_GETSETDEF
#  define _SSL__SSLSOCKET_SERVER_HOSTNAME_GETSETDEF {"server_hostname", (getter)_ssl__SSLSocket_server_hostname_get, (setter)_ssl__SSLSocket_server_hostname_set, _ssl__SSLSocket_server_hostname_DOCSTR},
#else
#  define _SSL__SSLSOCKET_SERVER_HOSTNAME_GETSETDEF {"server_hostname", (getter)_ssl__SSLSocket_server_hostname_get, NULL, _ssl__SSLSocket_server_hostname_DOCSTR},
#endif

static PyObject *
_ssl__SSLSocket_server_hostname_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_server_hostname_get(PySSLSocket *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_server_hostname_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_owner__doc__,
"The Python-level owner of this object.\n"
"\n"
"Passed as \"self\" in servername callback.");
#if defined(_ssl__SSLSocket_owner_DOCSTR)
#   undef _ssl__SSLSocket_owner_DOCSTR
#endif
#define _ssl__SSLSocket_owner_DOCSTR _ssl__SSLSocket_owner__doc__

#if !defined(_ssl__SSLSocket_owner_DOCSTR)
#  define _ssl__SSLSocket_owner_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_OWNER_GETSETDEF)
#  undef _SSL__SSLSOCKET_OWNER_GETSETDEF
#  define _SSL__SSLSOCKET_OWNER_GETSETDEF {"owner", (getter)_ssl__SSLSocket_owner_get, (setter)_ssl__SSLSocket_owner_set, _ssl__SSLSocket_owner_DOCSTR},
#else
#  define _SSL__SSLSOCKET_OWNER_GETSETDEF {"owner", (getter)_ssl__SSLSocket_owner_get, NULL, _ssl__SSLSocket_owner_DOCSTR},
#endif

static PyObject *
_ssl__SSLSocket_owner_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_owner_get(PySSLSocket *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_owner_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLSocket_owner_DOCSTR)
#  define _ssl__SSLSocket_owner_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_OWNER_GETSETDEF)
#  undef _SSL__SSLSOCKET_OWNER_GETSETDEF
#  define _SSL__SSLSOCKET_OWNER_GETSETDEF {"owner", (getter)_ssl__SSLSocket_owner_get, (setter)_ssl__SSLSocket_owner_set, _ssl__SSLSocket_owner_DOCSTR},
#else
#  define _SSL__SSLSOCKET_OWNER_GETSETDEF {"owner", NULL, (setter)_ssl__SSLSocket_owner_set, NULL},
#endif

static int
_ssl__SSLSocket_owner_set_impl(PySSLSocket *self, PyObject *value);

static int
_ssl__SSLSocket_owner_set(PySSLSocket *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_owner_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Writes the bytes-like object b into the SSL object.\n"
"\n"
"Returns the number of bytes written.");

#define _SSL__SSLSOCKET_WRITE_METHODDEF    \
    {"write", (PyCFunction)_ssl__SSLSocket_write, METH_O, _ssl__SSLSocket_write__doc__},

static PyObject *
_ssl__SSLSocket_write_impl(PySSLSocket *self, Py_buffer *b);

static PyObject *
_ssl__SSLSocket_write(PySSLSocket *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer b = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &b, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_write_impl(self, &b);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_pending__doc__,
"pending($self, /)\n"
"--\n"
"\n"
"Returns the number of already decrypted bytes available for read, pending on the connection.");

#define _SSL__SSLSOCKET_PENDING_METHODDEF    \
    {"pending", (PyCFunction)_ssl__SSLSocket_pending, METH_NOARGS, _ssl__SSLSocket_pending__doc__},

static PyObject *
_ssl__SSLSocket_pending_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_pending(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_pending_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_read__doc__,
"read(size, [buffer])\n"
"Read up to size bytes from the SSL socket.");

#define _SSL__SSLSOCKET_READ_METHODDEF    \
    {"read", (PyCFunction)_ssl__SSLSocket_read, METH_VARARGS, _ssl__SSLSocket_read__doc__},

static PyObject *
_ssl__SSLSocket_read_impl(PySSLSocket *self, Py_ssize_t len,
                          int group_right_1, Py_buffer *buffer);

static PyObject *
_ssl__SSLSocket_read(PySSLSocket *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t len;
    int group_right_1 = 0;
    Py_buffer buffer = {NULL, NULL};

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "n:read", &len)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "nw*:read", &len, &buffer)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_ssl._SSLSocket.read requires 1 to 2 arguments");
            goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_read_impl(self, len, group_right_1, &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_shutdown__doc__,
"shutdown($self, /)\n"
"--\n"
"\n"
"Does the SSL shutdown handshake with the remote end.");

#define _SSL__SSLSOCKET_SHUTDOWN_METHODDEF    \
    {"shutdown", (PyCFunction)_ssl__SSLSocket_shutdown, METH_NOARGS, _ssl__SSLSocket_shutdown__doc__},

static PyObject *
_ssl__SSLSocket_shutdown_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_shutdown(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_shutdown_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_get_channel_binding__doc__,
"get_channel_binding($self, /, cb_type=\'tls-unique\')\n"
"--\n"
"\n"
"Get channel binding data for current connection.\n"
"\n"
"Raise ValueError if the requested `cb_type` is not supported.  Return bytes\n"
"of the data or None if the data is not available (e.g. before the handshake).\n"
"Only \'tls-unique\' channel binding data from RFC 5929 is supported.");

#define _SSL__SSLSOCKET_GET_CHANNEL_BINDING_METHODDEF    \
    {"get_channel_binding", _PyCFunction_CAST(_ssl__SSLSocket_get_channel_binding), METH_FASTCALL|METH_KEYWORDS, _ssl__SSLSocket_get_channel_binding__doc__},

static PyObject *
_ssl__SSLSocket_get_channel_binding_impl(PySSLSocket *self,
                                         const char *cb_type);

static PyObject *
_ssl__SSLSocket_get_channel_binding(PySSLSocket *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cb_type), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cb_type", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_channel_binding",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const char *cb_type = "tls-unique";

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("get_channel_binding", "argument 'cb_type'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t cb_type_length;
    cb_type = PyUnicode_AsUTF8AndSize(args[0], &cb_type_length);
    if (cb_type == NULL) {
        goto exit;
    }
    if (strlen(cb_type) != (size_t)cb_type_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_get_channel_binding_impl(self, cb_type);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_verify_client_post_handshake__doc__,
"verify_client_post_handshake($self, /)\n"
"--\n"
"\n"
"Initiate TLS 1.3 post-handshake authentication");

#define _SSL__SSLSOCKET_VERIFY_CLIENT_POST_HANDSHAKE_METHODDEF    \
    {"verify_client_post_handshake", (PyCFunction)_ssl__SSLSocket_verify_client_post_handshake, METH_NOARGS, _ssl__SSLSocket_verify_client_post_handshake__doc__},

static PyObject *
_ssl__SSLSocket_verify_client_post_handshake_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_verify_client_post_handshake(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_verify_client_post_handshake_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_session__doc__,
"The underlying SSLSession object.");
#if defined(_ssl__SSLSocket_session_DOCSTR)
#   undef _ssl__SSLSocket_session_DOCSTR
#endif
#define _ssl__SSLSocket_session_DOCSTR _ssl__SSLSocket_session__doc__

#if !defined(_ssl__SSLSocket_session_DOCSTR)
#  define _ssl__SSLSocket_session_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_SESSION_GETSETDEF)
#  undef _SSL__SSLSOCKET_SESSION_GETSETDEF
#  define _SSL__SSLSOCKET_SESSION_GETSETDEF {"session", (getter)_ssl__SSLSocket_session_get, (setter)_ssl__SSLSocket_session_set, _ssl__SSLSocket_session_DOCSTR},
#else
#  define _SSL__SSLSOCKET_SESSION_GETSETDEF {"session", (getter)_ssl__SSLSocket_session_get, NULL, _ssl__SSLSocket_session_DOCSTR},
#endif

static PyObject *
_ssl__SSLSocket_session_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_session_get(PySSLSocket *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_session_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLSocket_session_DOCSTR)
#  define _ssl__SSLSocket_session_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_SESSION_GETSETDEF)
#  undef _SSL__SSLSOCKET_SESSION_GETSETDEF
#  define _SSL__SSLSOCKET_SESSION_GETSETDEF {"session", (getter)_ssl__SSLSocket_session_get, (setter)_ssl__SSLSocket_session_set, _ssl__SSLSocket_session_DOCSTR},
#else
#  define _SSL__SSLSOCKET_SESSION_GETSETDEF {"session", NULL, (setter)_ssl__SSLSocket_session_set, NULL},
#endif

static int
_ssl__SSLSocket_session_set_impl(PySSLSocket *self, PyObject *value);

static int
_ssl__SSLSocket_session_set(PySSLSocket *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_session_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_session_reused__doc__,
"Was the client session reused during handshake?");
#if defined(_ssl__SSLSocket_session_reused_DOCSTR)
#   undef _ssl__SSLSocket_session_reused_DOCSTR
#endif
#define _ssl__SSLSocket_session_reused_DOCSTR _ssl__SSLSocket_session_reused__doc__

#if !defined(_ssl__SSLSocket_session_reused_DOCSTR)
#  define _ssl__SSLSocket_session_reused_DOCSTR NULL
#endif
#if defined(_SSL__SSLSOCKET_SESSION_REUSED_GETSETDEF)
#  undef _SSL__SSLSOCKET_SESSION_REUSED_GETSETDEF
#  define _SSL__SSLSOCKET_SESSION_REUSED_GETSETDEF {"session_reused", (getter)_ssl__SSLSocket_session_reused_get, (setter)_ssl__SSLSocket_session_reused_set, _ssl__SSLSocket_session_reused_DOCSTR},
#else
#  define _SSL__SSLSOCKET_SESSION_REUSED_GETSETDEF {"session_reused", (getter)_ssl__SSLSocket_session_reused_get, NULL, _ssl__SSLSocket_session_reused_DOCSTR},
#endif

static PyObject *
_ssl__SSLSocket_session_reused_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_session_reused_get(PySSLSocket *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_session_reused_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_ssl__SSLContext_impl(PyTypeObject *type, int proto_version);

static PyObject *
_ssl__SSLContext(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = get_state_type(type)->PySSLContext_Type;
    int proto_version;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("_SSLContext", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_SSLContext", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    proto_version = PyLong_AsInt(PyTuple_GET_ITEM(args, 0));
    if (proto_version == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(type);
    return_value = _ssl__SSLContext_impl(type, proto_version);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_ciphers__doc__,
"set_ciphers($self, cipherlist, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_CIPHERS_METHODDEF    \
    {"set_ciphers", (PyCFunction)_ssl__SSLContext_set_ciphers, METH_O, _ssl__SSLContext_set_ciphers__doc__},

static PyObject *
_ssl__SSLContext_set_ciphers_impl(PySSLContext *self, const char *cipherlist);

static PyObject *
_ssl__SSLContext_set_ciphers(PySSLContext *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *cipherlist;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("set_ciphers", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t cipherlist_length;
    cipherlist = PyUnicode_AsUTF8AndSize(arg, &cipherlist_length);
    if (cipherlist == NULL) {
        goto exit;
    }
    if (strlen(cipherlist) != (size_t)cipherlist_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_set_ciphers_impl(self, cipherlist);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_get_ciphers__doc__,
"get_ciphers($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_GET_CIPHERS_METHODDEF    \
    {"get_ciphers", (PyCFunction)_ssl__SSLContext_get_ciphers, METH_NOARGS, _ssl__SSLContext_get_ciphers__doc__},

static PyObject *
_ssl__SSLContext_get_ciphers_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_get_ciphers(PySSLContext *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_get_ciphers_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext__set_alpn_protocols__doc__,
"_set_alpn_protocols($self, protos, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__SET_ALPN_PROTOCOLS_METHODDEF    \
    {"_set_alpn_protocols", (PyCFunction)_ssl__SSLContext__set_alpn_protocols, METH_O, _ssl__SSLContext__set_alpn_protocols__doc__},

static PyObject *
_ssl__SSLContext__set_alpn_protocols_impl(PySSLContext *self,
                                          Py_buffer *protos);

static PyObject *
_ssl__SSLContext__set_alpn_protocols(PySSLContext *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer protos = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &protos, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext__set_alpn_protocols_impl(self, &protos);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for protos */
    if (protos.obj) {
       PyBuffer_Release(&protos);
    }

    return return_value;
}

#if !defined(_ssl__SSLContext_verify_mode_DOCSTR)
#  define _ssl__SSLContext_verify_mode_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF)
#  undef _SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF
#  define _SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF {"verify_mode", (getter)_ssl__SSLContext_verify_mode_get, (setter)_ssl__SSLContext_verify_mode_set, _ssl__SSLContext_verify_mode_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF {"verify_mode", (getter)_ssl__SSLContext_verify_mode_get, NULL, _ssl__SSLContext_verify_mode_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_verify_mode_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_verify_mode_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_verify_mode_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_verify_mode_DOCSTR)
#  define _ssl__SSLContext_verify_mode_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF)
#  undef _SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF
#  define _SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF {"verify_mode", (getter)_ssl__SSLContext_verify_mode_get, (setter)_ssl__SSLContext_verify_mode_set, _ssl__SSLContext_verify_mode_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF {"verify_mode", NULL, (setter)_ssl__SSLContext_verify_mode_set, NULL},
#endif

static int
_ssl__SSLContext_verify_mode_set_impl(PySSLContext *self, PyObject *value);

static int
_ssl__SSLContext_verify_mode_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_verify_mode_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_verify_flags_DOCSTR)
#  define _ssl__SSLContext_verify_flags_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF)
#  undef _SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF
#  define _SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF {"verify_flags", (getter)_ssl__SSLContext_verify_flags_get, (setter)_ssl__SSLContext_verify_flags_set, _ssl__SSLContext_verify_flags_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF {"verify_flags", (getter)_ssl__SSLContext_verify_flags_get, NULL, _ssl__SSLContext_verify_flags_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_verify_flags_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_verify_flags_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_verify_flags_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_verify_flags_DOCSTR)
#  define _ssl__SSLContext_verify_flags_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF)
#  undef _SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF
#  define _SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF {"verify_flags", (getter)_ssl__SSLContext_verify_flags_get, (setter)_ssl__SSLContext_verify_flags_set, _ssl__SSLContext_verify_flags_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF {"verify_flags", NULL, (setter)_ssl__SSLContext_verify_flags_set, NULL},
#endif

static int
_ssl__SSLContext_verify_flags_set_impl(PySSLContext *self, PyObject *value);

static int
_ssl__SSLContext_verify_flags_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_verify_flags_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_minimum_version_DOCSTR)
#  define _ssl__SSLContext_minimum_version_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF)
#  undef _SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF
#  define _SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF {"minimum_version", (getter)_ssl__SSLContext_minimum_version_get, (setter)_ssl__SSLContext_minimum_version_set, _ssl__SSLContext_minimum_version_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF {"minimum_version", (getter)_ssl__SSLContext_minimum_version_get, NULL, _ssl__SSLContext_minimum_version_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_minimum_version_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_minimum_version_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_minimum_version_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_minimum_version_DOCSTR)
#  define _ssl__SSLContext_minimum_version_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF)
#  undef _SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF
#  define _SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF {"minimum_version", (getter)_ssl__SSLContext_minimum_version_get, (setter)_ssl__SSLContext_minimum_version_set, _ssl__SSLContext_minimum_version_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF {"minimum_version", NULL, (setter)_ssl__SSLContext_minimum_version_set, NULL},
#endif

static int
_ssl__SSLContext_minimum_version_set_impl(PySSLContext *self,
                                          PyObject *value);

static int
_ssl__SSLContext_minimum_version_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_minimum_version_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_maximum_version_DOCSTR)
#  define _ssl__SSLContext_maximum_version_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF)
#  undef _SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF
#  define _SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF {"maximum_version", (getter)_ssl__SSLContext_maximum_version_get, (setter)_ssl__SSLContext_maximum_version_set, _ssl__SSLContext_maximum_version_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF {"maximum_version", (getter)_ssl__SSLContext_maximum_version_get, NULL, _ssl__SSLContext_maximum_version_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_maximum_version_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_maximum_version_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_maximum_version_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_maximum_version_DOCSTR)
#  define _ssl__SSLContext_maximum_version_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF)
#  undef _SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF
#  define _SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF {"maximum_version", (getter)_ssl__SSLContext_maximum_version_get, (setter)_ssl__SSLContext_maximum_version_set, _ssl__SSLContext_maximum_version_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF {"maximum_version", NULL, (setter)_ssl__SSLContext_maximum_version_set, NULL},
#endif

static int
_ssl__SSLContext_maximum_version_set_impl(PySSLContext *self,
                                          PyObject *value);

static int
_ssl__SSLContext_maximum_version_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_maximum_version_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_num_tickets__doc__,
"Control the number of TLSv1.3 session tickets.");
#if defined(_ssl__SSLContext_num_tickets_DOCSTR)
#   undef _ssl__SSLContext_num_tickets_DOCSTR
#endif
#define _ssl__SSLContext_num_tickets_DOCSTR _ssl__SSLContext_num_tickets__doc__

#if !defined(_ssl__SSLContext_num_tickets_DOCSTR)
#  define _ssl__SSLContext_num_tickets_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF)
#  undef _SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF
#  define _SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF {"num_tickets", (getter)_ssl__SSLContext_num_tickets_get, (setter)_ssl__SSLContext_num_tickets_set, _ssl__SSLContext_num_tickets_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF {"num_tickets", (getter)_ssl__SSLContext_num_tickets_get, NULL, _ssl__SSLContext_num_tickets_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_num_tickets_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_num_tickets_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_num_tickets_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_num_tickets_DOCSTR)
#  define _ssl__SSLContext_num_tickets_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF)
#  undef _SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF
#  define _SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF {"num_tickets", (getter)_ssl__SSLContext_num_tickets_get, (setter)_ssl__SSLContext_num_tickets_set, _ssl__SSLContext_num_tickets_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF {"num_tickets", NULL, (setter)_ssl__SSLContext_num_tickets_set, NULL},
#endif

static int
_ssl__SSLContext_num_tickets_set_impl(PySSLContext *self, PyObject *value);

static int
_ssl__SSLContext_num_tickets_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_num_tickets_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_security_level__doc__,
"The current security level.");
#if defined(_ssl__SSLContext_security_level_DOCSTR)
#   undef _ssl__SSLContext_security_level_DOCSTR
#endif
#define _ssl__SSLContext_security_level_DOCSTR _ssl__SSLContext_security_level__doc__

#if !defined(_ssl__SSLContext_security_level_DOCSTR)
#  define _ssl__SSLContext_security_level_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_SECURITY_LEVEL_GETSETDEF)
#  undef _SSL__SSLCONTEXT_SECURITY_LEVEL_GETSETDEF
#  define _SSL__SSLCONTEXT_SECURITY_LEVEL_GETSETDEF {"security_level", (getter)_ssl__SSLContext_security_level_get, (setter)_ssl__SSLContext_security_level_set, _ssl__SSLContext_security_level_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_SECURITY_LEVEL_GETSETDEF {"security_level", (getter)_ssl__SSLContext_security_level_get, NULL, _ssl__SSLContext_security_level_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_security_level_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_security_level_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_security_level_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_options_DOCSTR)
#  define _ssl__SSLContext_options_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_OPTIONS_GETSETDEF)
#  undef _SSL__SSLCONTEXT_OPTIONS_GETSETDEF
#  define _SSL__SSLCONTEXT_OPTIONS_GETSETDEF {"options", (getter)_ssl__SSLContext_options_get, (setter)_ssl__SSLContext_options_set, _ssl__SSLContext_options_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_OPTIONS_GETSETDEF {"options", (getter)_ssl__SSLContext_options_get, NULL, _ssl__SSLContext_options_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_options_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_options_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_options_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_options_DOCSTR)
#  define _ssl__SSLContext_options_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_OPTIONS_GETSETDEF)
#  undef _SSL__SSLCONTEXT_OPTIONS_GETSETDEF
#  define _SSL__SSLCONTEXT_OPTIONS_GETSETDEF {"options", (getter)_ssl__SSLContext_options_get, (setter)_ssl__SSLContext_options_set, _ssl__SSLContext_options_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_OPTIONS_GETSETDEF {"options", NULL, (setter)_ssl__SSLContext_options_set, NULL},
#endif

static int
_ssl__SSLContext_options_set_impl(PySSLContext *self, PyObject *value);

static int
_ssl__SSLContext_options_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_options_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext__host_flags_DOCSTR)
#  define _ssl__SSLContext__host_flags_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF)
#  undef _SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF
#  define _SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF {"_host_flags", (getter)_ssl__SSLContext__host_flags_get, (setter)_ssl__SSLContext__host_flags_set, _ssl__SSLContext__host_flags_DOCSTR},
#else
#  define _SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF {"_host_flags", (getter)_ssl__SSLContext__host_flags_get, NULL, _ssl__SSLContext__host_flags_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext__host_flags_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext__host_flags_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext__host_flags_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext__host_flags_DOCSTR)
#  define _ssl__SSLContext__host_flags_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF)
#  undef _SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF
#  define _SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF {"_host_flags", (getter)_ssl__SSLContext__host_flags_get, (setter)_ssl__SSLContext__host_flags_set, _ssl__SSLContext__host_flags_DOCSTR},
#else
#  define _SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF {"_host_flags", NULL, (setter)_ssl__SSLContext__host_flags_set, NULL},
#endif

static int
_ssl__SSLContext__host_flags_set_impl(PySSLContext *self, PyObject *value);

static int
_ssl__SSLContext__host_flags_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext__host_flags_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_check_hostname_DOCSTR)
#  define _ssl__SSLContext_check_hostname_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF)
#  undef _SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF
#  define _SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF {"check_hostname", (getter)_ssl__SSLContext_check_hostname_get, (setter)_ssl__SSLContext_check_hostname_set, _ssl__SSLContext_check_hostname_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF {"check_hostname", (getter)_ssl__SSLContext_check_hostname_get, NULL, _ssl__SSLContext_check_hostname_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_check_hostname_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_check_hostname_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_check_hostname_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_check_hostname_DOCSTR)
#  define _ssl__SSLContext_check_hostname_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF)
#  undef _SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF
#  define _SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF {"check_hostname", (getter)_ssl__SSLContext_check_hostname_get, (setter)_ssl__SSLContext_check_hostname_set, _ssl__SSLContext_check_hostname_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF {"check_hostname", NULL, (setter)_ssl__SSLContext_check_hostname_set, NULL},
#endif

static int
_ssl__SSLContext_check_hostname_set_impl(PySSLContext *self, PyObject *value);

static int
_ssl__SSLContext_check_hostname_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_check_hostname_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_protocol_DOCSTR)
#  define _ssl__SSLContext_protocol_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_PROTOCOL_GETSETDEF)
#  undef _SSL__SSLCONTEXT_PROTOCOL_GETSETDEF
#  define _SSL__SSLCONTEXT_PROTOCOL_GETSETDEF {"protocol", (getter)_ssl__SSLContext_protocol_get, (setter)_ssl__SSLContext_protocol_set, _ssl__SSLContext_protocol_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_PROTOCOL_GETSETDEF {"protocol", (getter)_ssl__SSLContext_protocol_get, NULL, _ssl__SSLContext_protocol_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_protocol_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_protocol_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_protocol_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_cert_chain__doc__,
"load_cert_chain($self, /, certfile, keyfile=None, password=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_CERT_CHAIN_METHODDEF    \
    {"load_cert_chain", _PyCFunction_CAST(_ssl__SSLContext_load_cert_chain), METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext_load_cert_chain__doc__},

static PyObject *
_ssl__SSLContext_load_cert_chain_impl(PySSLContext *self, PyObject *certfile,
                                      PyObject *keyfile, PyObject *password);

static PyObject *
_ssl__SSLContext_load_cert_chain(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(certfile), &_Py_ID(keyfile), &_Py_ID(password), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"certfile", "keyfile", "password", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "load_cert_chain",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *certfile;
    PyObject *keyfile = Py_None;
    PyObject *password = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    certfile = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        keyfile = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    password = args[2];
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_load_cert_chain_impl(self, certfile, keyfile, password);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_verify_locations__doc__,
"load_verify_locations($self, /, cafile=None, capath=None, cadata=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_VERIFY_LOCATIONS_METHODDEF    \
    {"load_verify_locations", _PyCFunction_CAST(_ssl__SSLContext_load_verify_locations), METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext_load_verify_locations__doc__},

static PyObject *
_ssl__SSLContext_load_verify_locations_impl(PySSLContext *self,
                                            PyObject *cafile,
                                            PyObject *capath,
                                            PyObject *cadata);

static PyObject *
_ssl__SSLContext_load_verify_locations(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(cafile), &_Py_ID(capath), &_Py_ID(cadata), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cafile", "capath", "cadata", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "load_verify_locations",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *cafile = Py_None;
    PyObject *capath = Py_None;
    PyObject *cadata = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        cafile = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        capath = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    cadata = args[2];
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_load_verify_locations_impl(self, cafile, capath, cadata);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_dh_params__doc__,
"load_dh_params($self, path, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_DH_PARAMS_METHODDEF    \
    {"load_dh_params", (PyCFunction)_ssl__SSLContext_load_dh_params, METH_O, _ssl__SSLContext_load_dh_params__doc__},

static PyObject *
_ssl__SSLContext_load_dh_params_impl(PySSLContext *self, PyObject *filepath);

static PyObject *
_ssl__SSLContext_load_dh_params(PySSLContext *self, PyObject *filepath)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_load_dh_params_impl(self, filepath);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext__wrap_socket__doc__,
"_wrap_socket($self, /, sock, server_side, server_hostname=None, *,\n"
"             owner=None, session=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__WRAP_SOCKET_METHODDEF    \
    {"_wrap_socket", _PyCFunction_CAST(_ssl__SSLContext__wrap_socket), METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext__wrap_socket__doc__},

static PyObject *
_ssl__SSLContext__wrap_socket_impl(PySSLContext *self, PyObject *sock,
                                   int server_side, PyObject *hostname_obj,
                                   PyObject *owner, PyObject *session);

static PyObject *
_ssl__SSLContext__wrap_socket(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(sock), &_Py_ID(server_side), &_Py_ID(server_hostname), &_Py_ID(owner), &_Py_ID(session), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sock", "server_side", "server_hostname", "owner", "session", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_wrap_socket",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *sock;
    int server_side;
    PyObject *hostname_obj = Py_None;
    PyObject *owner = Py_None;
    PyObject *session = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], get_state_ctx(self)->Sock_Type)) {
        _PyArg_BadArgument("_wrap_socket", "argument 'sock'", (get_state_ctx(self)->Sock_Type)->tp_name, args[0]);
        goto exit;
    }
    sock = args[0];
    server_side = PyObject_IsTrue(args[1]);
    if (server_side < 0) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        hostname_obj = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        owner = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    session = args[4];
skip_optional_kwonly:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext__wrap_socket_impl(self, sock, server_side, hostname_obj, owner, session);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext__wrap_bio__doc__,
"_wrap_bio($self, /, incoming, outgoing, server_side,\n"
"          server_hostname=None, *, owner=None, session=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__WRAP_BIO_METHODDEF    \
    {"_wrap_bio", _PyCFunction_CAST(_ssl__SSLContext__wrap_bio), METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext__wrap_bio__doc__},

static PyObject *
_ssl__SSLContext__wrap_bio_impl(PySSLContext *self, PySSLMemoryBIO *incoming,
                                PySSLMemoryBIO *outgoing, int server_side,
                                PyObject *hostname_obj, PyObject *owner,
                                PyObject *session);

static PyObject *
_ssl__SSLContext__wrap_bio(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(incoming), &_Py_ID(outgoing), &_Py_ID(server_side), &_Py_ID(server_hostname), &_Py_ID(owner), &_Py_ID(session), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"incoming", "outgoing", "server_side", "server_hostname", "owner", "session", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_wrap_bio",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    PySSLMemoryBIO *incoming;
    PySSLMemoryBIO *outgoing;
    int server_side;
    PyObject *hostname_obj = Py_None;
    PyObject *owner = Py_None;
    PyObject *session = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], get_state_ctx(self)->PySSLMemoryBIO_Type)) {
        _PyArg_BadArgument("_wrap_bio", "argument 'incoming'", (get_state_ctx(self)->PySSLMemoryBIO_Type)->tp_name, args[0]);
        goto exit;
    }
    incoming = (PySSLMemoryBIO *)args[0];
    if (!PyObject_TypeCheck(args[1], get_state_ctx(self)->PySSLMemoryBIO_Type)) {
        _PyArg_BadArgument("_wrap_bio", "argument 'outgoing'", (get_state_ctx(self)->PySSLMemoryBIO_Type)->tp_name, args[1]);
        goto exit;
    }
    outgoing = (PySSLMemoryBIO *)args[1];
    server_side = PyObject_IsTrue(args[2]);
    if (server_side < 0) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[3]) {
        hostname_obj = args[3];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[4]) {
        owner = args[4];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    session = args[5];
skip_optional_kwonly:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext__wrap_bio_impl(self, incoming, outgoing, server_side, hostname_obj, owner, session);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_session_stats__doc__,
"session_stats($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SESSION_STATS_METHODDEF    \
    {"session_stats", (PyCFunction)_ssl__SSLContext_session_stats, METH_NOARGS, _ssl__SSLContext_session_stats__doc__},

static PyObject *
_ssl__SSLContext_session_stats_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_session_stats(PySSLContext *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_session_stats_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_default_verify_paths__doc__,
"set_default_verify_paths($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_DEFAULT_VERIFY_PATHS_METHODDEF    \
    {"set_default_verify_paths", (PyCFunction)_ssl__SSLContext_set_default_verify_paths, METH_NOARGS, _ssl__SSLContext_set_default_verify_paths__doc__},

static PyObject *
_ssl__SSLContext_set_default_verify_paths_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_set_default_verify_paths(PySSLContext *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_set_default_verify_paths_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_ecdh_curve__doc__,
"set_ecdh_curve($self, name, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_ECDH_CURVE_METHODDEF    \
    {"set_ecdh_curve", (PyCFunction)_ssl__SSLContext_set_ecdh_curve, METH_O, _ssl__SSLContext_set_ecdh_curve__doc__},

static PyObject *
_ssl__SSLContext_set_ecdh_curve_impl(PySSLContext *self, PyObject *name);

static PyObject *
_ssl__SSLContext_set_ecdh_curve(PySSLContext *self, PyObject *name)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_set_ecdh_curve_impl(self, name);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_sni_callback__doc__,
"Set a callback that will be called when a server name is provided by the SSL/TLS client in the SNI extension.\n"
"\n"
"If the argument is None then the callback is disabled. The method is called\n"
"with the SSLSocket, the server name as a string, and the SSLContext object.\n"
"\n"
"See RFC 6066 for details of the SNI extension.");
#if defined(_ssl__SSLContext_sni_callback_DOCSTR)
#   undef _ssl__SSLContext_sni_callback_DOCSTR
#endif
#define _ssl__SSLContext_sni_callback_DOCSTR _ssl__SSLContext_sni_callback__doc__

#if !defined(_ssl__SSLContext_sni_callback_DOCSTR)
#  define _ssl__SSLContext_sni_callback_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF)
#  undef _SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF
#  define _SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF {"sni_callback", (getter)_ssl__SSLContext_sni_callback_get, (setter)_ssl__SSLContext_sni_callback_set, _ssl__SSLContext_sni_callback_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF {"sni_callback", (getter)_ssl__SSLContext_sni_callback_get, NULL, _ssl__SSLContext_sni_callback_DOCSTR},
#endif

static PyObject *
_ssl__SSLContext_sni_callback_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_sni_callback_get(PySSLContext *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_sni_callback_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_ssl__SSLContext_sni_callback_DOCSTR)
#  define _ssl__SSLContext_sni_callback_DOCSTR NULL
#endif
#if defined(_SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF)
#  undef _SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF
#  define _SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF {"sni_callback", (getter)_ssl__SSLContext_sni_callback_get, (setter)_ssl__SSLContext_sni_callback_set, _ssl__SSLContext_sni_callback_DOCSTR},
#else
#  define _SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF {"sni_callback", NULL, (setter)_ssl__SSLContext_sni_callback_set, NULL},
#endif

static int
_ssl__SSLContext_sni_callback_set_impl(PySSLContext *self, PyObject *value);

static int
_ssl__SSLContext_sni_callback_set(PySSLContext *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_sni_callback_set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_cert_store_stats__doc__,
"cert_store_stats($self, /)\n"
"--\n"
"\n"
"Returns quantities of loaded X.509 certificates.\n"
"\n"
"X.509 certificates with a CA extension and certificate revocation lists\n"
"inside the context\'s cert store.\n"
"\n"
"NOTE: Certificates in a capath directory aren\'t loaded unless they have\n"
"been used at least once.");

#define _SSL__SSLCONTEXT_CERT_STORE_STATS_METHODDEF    \
    {"cert_store_stats", (PyCFunction)_ssl__SSLContext_cert_store_stats, METH_NOARGS, _ssl__SSLContext_cert_store_stats__doc__},

static PyObject *
_ssl__SSLContext_cert_store_stats_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_cert_store_stats(PySSLContext *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_cert_store_stats_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_get_ca_certs__doc__,
"get_ca_certs($self, /, binary_form=False)\n"
"--\n"
"\n"
"Returns a list of dicts with information of loaded CA certs.\n"
"\n"
"If the optional argument is True, returns a DER-encoded copy of the CA\n"
"certificate.\n"
"\n"
"NOTE: Certificates in a capath directory aren\'t loaded unless they have\n"
"been used at least once.");

#define _SSL__SSLCONTEXT_GET_CA_CERTS_METHODDEF    \
    {"get_ca_certs", _PyCFunction_CAST(_ssl__SSLContext_get_ca_certs), METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext_get_ca_certs__doc__},

static PyObject *
_ssl__SSLContext_get_ca_certs_impl(PySSLContext *self, int binary_form);

static PyObject *
_ssl__SSLContext_get_ca_certs(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(binary_form), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"binary_form", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_ca_certs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int binary_form = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    binary_form = PyObject_IsTrue(args[0]);
    if (binary_form < 0) {
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_get_ca_certs_impl(self, binary_form);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_psk_client_callback__doc__,
"set_psk_client_callback($self, /, callback)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_PSK_CLIENT_CALLBACK_METHODDEF    \
    {"set_psk_client_callback", _PyCFunction_CAST(_ssl__SSLContext_set_psk_client_callback), METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext_set_psk_client_callback__doc__},

static PyObject *
_ssl__SSLContext_set_psk_client_callback_impl(PySSLContext *self,
                                              PyObject *callback);

static PyObject *
_ssl__SSLContext_set_psk_client_callback(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(callback), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"callback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_psk_client_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *callback;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    callback = args[0];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_set_psk_client_callback_impl(self, callback);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_psk_server_callback__doc__,
"set_psk_server_callback($self, /, callback, identity_hint=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_PSK_SERVER_CALLBACK_METHODDEF    \
    {"set_psk_server_callback", _PyCFunction_CAST(_ssl__SSLContext_set_psk_server_callback), METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext_set_psk_server_callback__doc__},

static PyObject *
_ssl__SSLContext_set_psk_server_callback_impl(PySSLContext *self,
                                              PyObject *callback,
                                              const char *identity_hint);

static PyObject *
_ssl__SSLContext_set_psk_server_callback(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(callback), &_Py_ID(identity_hint), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"callback", "identity_hint", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_psk_server_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *callback;
    const char *identity_hint = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    callback = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1] == Py_None) {
        identity_hint = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t identity_hint_length;
        identity_hint = PyUnicode_AsUTF8AndSize(args[1], &identity_hint_length);
        if (identity_hint == NULL) {
            goto exit;
        }
        if (strlen(identity_hint) != (size_t)identity_hint_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("set_psk_server_callback", "argument 'identity_hint'", "str or None", args[1]);
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_set_psk_server_callback_impl(self, callback, identity_hint);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
_ssl_MemoryBIO_impl(PyTypeObject *type);

static PyObject *
_ssl_MemoryBIO(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = get_state_type(type)->PySSLMemoryBIO_Type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositional("MemoryBIO", args)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("MemoryBIO", kwargs)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(type);
    return_value = _ssl_MemoryBIO_impl(type);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_pending__doc__,
"The number of bytes pending in the memory BIO.");
#if defined(_ssl_MemoryBIO_pending_DOCSTR)
#   undef _ssl_MemoryBIO_pending_DOCSTR
#endif
#define _ssl_MemoryBIO_pending_DOCSTR _ssl_MemoryBIO_pending__doc__

#if !defined(_ssl_MemoryBIO_pending_DOCSTR)
#  define _ssl_MemoryBIO_pending_DOCSTR NULL
#endif
#if defined(_SSL_MEMORYBIO_PENDING_GETSETDEF)
#  undef _SSL_MEMORYBIO_PENDING_GETSETDEF
#  define _SSL_MEMORYBIO_PENDING_GETSETDEF {"pending", (getter)_ssl_MemoryBIO_pending_get, (setter)_ssl_MemoryBIO_pending_set, _ssl_MemoryBIO_pending_DOCSTR},
#else
#  define _SSL_MEMORYBIO_PENDING_GETSETDEF {"pending", (getter)_ssl_MemoryBIO_pending_get, NULL, _ssl_MemoryBIO_pending_DOCSTR},
#endif

static PyObject *
_ssl_MemoryBIO_pending_get_impl(PySSLMemoryBIO *self);

static PyObject *
_ssl_MemoryBIO_pending_get(PySSLMemoryBIO *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_MemoryBIO_pending_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_eof__doc__,
"Whether the memory BIO is at EOF.");
#if defined(_ssl_MemoryBIO_eof_DOCSTR)
#   undef _ssl_MemoryBIO_eof_DOCSTR
#endif
#define _ssl_MemoryBIO_eof_DOCSTR _ssl_MemoryBIO_eof__doc__

#if !defined(_ssl_MemoryBIO_eof_DOCSTR)
#  define _ssl_MemoryBIO_eof_DOCSTR NULL
#endif
#if defined(_SSL_MEMORYBIO_EOF_GETSETDEF)
#  undef _SSL_MEMORYBIO_EOF_GETSETDEF
#  define _SSL_MEMORYBIO_EOF_GETSETDEF {"eof", (getter)_ssl_MemoryBIO_eof_get, (setter)_ssl_MemoryBIO_eof_set, _ssl_MemoryBIO_eof_DOCSTR},
#else
#  define _SSL_MEMORYBIO_EOF_GETSETDEF {"eof", (getter)_ssl_MemoryBIO_eof_get, NULL, _ssl_MemoryBIO_eof_DOCSTR},
#endif

static PyObject *
_ssl_MemoryBIO_eof_get_impl(PySSLMemoryBIO *self);

static PyObject *
_ssl_MemoryBIO_eof_get(PySSLMemoryBIO *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_MemoryBIO_eof_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read up to size bytes from the memory BIO.\n"
"\n"
"If size is not specified, read the entire buffer.\n"
"If the return value is an empty bytes instance, this means either\n"
"EOF or that no data is available. Use the \"eof\" property to\n"
"distinguish between the two.");

#define _SSL_MEMORYBIO_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_ssl_MemoryBIO_read), METH_FASTCALL, _ssl_MemoryBIO_read__doc__},

static PyObject *
_ssl_MemoryBIO_read_impl(PySSLMemoryBIO *self, int len);

static PyObject *
_ssl_MemoryBIO_read(PySSLMemoryBIO *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int len = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    len = PyLong_AsInt(args[0]);
    if (len == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_MemoryBIO_read_impl(self, len);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Writes the bytes b into the memory BIO.\n"
"\n"
"Returns the number of bytes written.");

#define _SSL_MEMORYBIO_WRITE_METHODDEF    \
    {"write", (PyCFunction)_ssl_MemoryBIO_write, METH_O, _ssl_MemoryBIO_write__doc__},

static PyObject *
_ssl_MemoryBIO_write_impl(PySSLMemoryBIO *self, Py_buffer *b);

static PyObject *
_ssl_MemoryBIO_write(PySSLMemoryBIO *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer b = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &b, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_MemoryBIO_write_impl(self, &b);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_write_eof__doc__,
"write_eof($self, /)\n"
"--\n"
"\n"
"Write an EOF marker to the memory BIO.\n"
"\n"
"When all data has been read, the \"eof\" property will be True.");

#define _SSL_MEMORYBIO_WRITE_EOF_METHODDEF    \
    {"write_eof", (PyCFunction)_ssl_MemoryBIO_write_eof, METH_NOARGS, _ssl_MemoryBIO_write_eof__doc__},

static PyObject *
_ssl_MemoryBIO_write_eof_impl(PySSLMemoryBIO *self);

static PyObject *
_ssl_MemoryBIO_write_eof(PySSLMemoryBIO *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_MemoryBIO_write_eof_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_time__doc__,
"Session creation time (seconds since epoch).");
#if defined(_ssl_SSLSession_time_DOCSTR)
#   undef _ssl_SSLSession_time_DOCSTR
#endif
#define _ssl_SSLSession_time_DOCSTR _ssl_SSLSession_time__doc__

#if !defined(_ssl_SSLSession_time_DOCSTR)
#  define _ssl_SSLSession_time_DOCSTR NULL
#endif
#if defined(_SSL_SSLSESSION_TIME_GETSETDEF)
#  undef _SSL_SSLSESSION_TIME_GETSETDEF
#  define _SSL_SSLSESSION_TIME_GETSETDEF {"time", (getter)_ssl_SSLSession_time_get, (setter)_ssl_SSLSession_time_set, _ssl_SSLSession_time_DOCSTR},
#else
#  define _SSL_SSLSESSION_TIME_GETSETDEF {"time", (getter)_ssl_SSLSession_time_get, NULL, _ssl_SSLSession_time_DOCSTR},
#endif

static PyObject *
_ssl_SSLSession_time_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_time_get(PySSLSession *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_time_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_timeout__doc__,
"Session timeout (delta in seconds).");
#if defined(_ssl_SSLSession_timeout_DOCSTR)
#   undef _ssl_SSLSession_timeout_DOCSTR
#endif
#define _ssl_SSLSession_timeout_DOCSTR _ssl_SSLSession_timeout__doc__

#if !defined(_ssl_SSLSession_timeout_DOCSTR)
#  define _ssl_SSLSession_timeout_DOCSTR NULL
#endif
#if defined(_SSL_SSLSESSION_TIMEOUT_GETSETDEF)
#  undef _SSL_SSLSESSION_TIMEOUT_GETSETDEF
#  define _SSL_SSLSESSION_TIMEOUT_GETSETDEF {"timeout", (getter)_ssl_SSLSession_timeout_get, (setter)_ssl_SSLSession_timeout_set, _ssl_SSLSession_timeout_DOCSTR},
#else
#  define _SSL_SSLSESSION_TIMEOUT_GETSETDEF {"timeout", (getter)_ssl_SSLSession_timeout_get, NULL, _ssl_SSLSession_timeout_DOCSTR},
#endif

static PyObject *
_ssl_SSLSession_timeout_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_timeout_get(PySSLSession *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_timeout_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_ticket_lifetime_hint__doc__,
"Ticket life time hint.");
#if defined(_ssl_SSLSession_ticket_lifetime_hint_DOCSTR)
#   undef _ssl_SSLSession_ticket_lifetime_hint_DOCSTR
#endif
#define _ssl_SSLSession_ticket_lifetime_hint_DOCSTR _ssl_SSLSession_ticket_lifetime_hint__doc__

#if !defined(_ssl_SSLSession_ticket_lifetime_hint_DOCSTR)
#  define _ssl_SSLSession_ticket_lifetime_hint_DOCSTR NULL
#endif
#if defined(_SSL_SSLSESSION_TICKET_LIFETIME_HINT_GETSETDEF)
#  undef _SSL_SSLSESSION_TICKET_LIFETIME_HINT_GETSETDEF
#  define _SSL_SSLSESSION_TICKET_LIFETIME_HINT_GETSETDEF {"ticket_lifetime_hint", (getter)_ssl_SSLSession_ticket_lifetime_hint_get, (setter)_ssl_SSLSession_ticket_lifetime_hint_set, _ssl_SSLSession_ticket_lifetime_hint_DOCSTR},
#else
#  define _SSL_SSLSESSION_TICKET_LIFETIME_HINT_GETSETDEF {"ticket_lifetime_hint", (getter)_ssl_SSLSession_ticket_lifetime_hint_get, NULL, _ssl_SSLSession_ticket_lifetime_hint_DOCSTR},
#endif

static PyObject *
_ssl_SSLSession_ticket_lifetime_hint_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_ticket_lifetime_hint_get(PySSLSession *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_ticket_lifetime_hint_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_id__doc__,
"Session ID.");
#if defined(_ssl_SSLSession_id_DOCSTR)
#   undef _ssl_SSLSession_id_DOCSTR
#endif
#define _ssl_SSLSession_id_DOCSTR _ssl_SSLSession_id__doc__

#if !defined(_ssl_SSLSession_id_DOCSTR)
#  define _ssl_SSLSession_id_DOCSTR NULL
#endif
#if defined(_SSL_SSLSESSION_ID_GETSETDEF)
#  undef _SSL_SSLSESSION_ID_GETSETDEF
#  define _SSL_SSLSESSION_ID_GETSETDEF {"id", (getter)_ssl_SSLSession_id_get, (setter)_ssl_SSLSession_id_set, _ssl_SSLSession_id_DOCSTR},
#else
#  define _SSL_SSLSESSION_ID_GETSETDEF {"id", (getter)_ssl_SSLSession_id_get, NULL, _ssl_SSLSession_id_DOCSTR},
#endif

static PyObject *
_ssl_SSLSession_id_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_id_get(PySSLSession *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_id_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_has_ticket__doc__,
"Does the session contain a ticket?");
#if defined(_ssl_SSLSession_has_ticket_DOCSTR)
#   undef _ssl_SSLSession_has_ticket_DOCSTR
#endif
#define _ssl_SSLSession_has_ticket_DOCSTR _ssl_SSLSession_has_ticket__doc__

#if !defined(_ssl_SSLSession_has_ticket_DOCSTR)
#  define _ssl_SSLSession_has_ticket_DOCSTR NULL
#endif
#if defined(_SSL_SSLSESSION_HAS_TICKET_GETSETDEF)
#  undef _SSL_SSLSESSION_HAS_TICKET_GETSETDEF
#  define _SSL_SSLSESSION_HAS_TICKET_GETSETDEF {"has_ticket", (getter)_ssl_SSLSession_has_ticket_get, (setter)_ssl_SSLSession_has_ticket_set, _ssl_SSLSession_has_ticket_DOCSTR},
#else
#  define _SSL_SSLSESSION_HAS_TICKET_GETSETDEF {"has_ticket", (getter)_ssl_SSLSession_has_ticket_get, NULL, _ssl_SSLSession_has_ticket_DOCSTR},
#endif

static PyObject *
_ssl_SSLSession_has_ticket_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_has_ticket_get(PySSLSession *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_has_ticket_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_RAND_add__doc__,
"RAND_add($module, string, entropy, /)\n"
"--\n"
"\n"
"Mix string into the OpenSSL PRNG state.\n"
"\n"
"entropy (a float) is a lower bound on the entropy contained in\n"
"string.  See RFC 4086.");

#define _SSL_RAND_ADD_METHODDEF    \
    {"RAND_add", _PyCFunction_CAST(_ssl_RAND_add), METH_FASTCALL, _ssl_RAND_add__doc__},

static PyObject *
_ssl_RAND_add_impl(PyObject *module, Py_buffer *view, double entropy);

static PyObject *
_ssl_RAND_add(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer view = {NULL, NULL};
    double entropy;

    if (!_PyArg_CheckPositional("RAND_add", nargs, 2, 2)) {
        goto exit;
    }
    if (PyUnicode_Check(args[0])) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(args[0], &len);
        if (ptr == NULL) {
            goto exit;
        }
        if (PyBuffer_FillInfo(&view, args[0], (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
            goto exit;
        }
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(args[0], &view, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        entropy = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        entropy = PyFloat_AsDouble(args[1]);
        if (entropy == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _ssl_RAND_add_impl(module, &view, entropy);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for view */
    if (view.obj) {
       PyBuffer_Release(&view);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl_RAND_bytes__doc__,
"RAND_bytes($module, n, /)\n"
"--\n"
"\n"
"Generate n cryptographically strong pseudo-random bytes.");

#define _SSL_RAND_BYTES_METHODDEF    \
    {"RAND_bytes", (PyCFunction)_ssl_RAND_bytes, METH_O, _ssl_RAND_bytes__doc__},

static PyObject *
_ssl_RAND_bytes_impl(PyObject *module, int n);

static PyObject *
_ssl_RAND_bytes(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int n;

    n = PyLong_AsInt(arg);
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _ssl_RAND_bytes_impl(module, n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_RAND_status__doc__,
"RAND_status($module, /)\n"
"--\n"
"\n"
"Returns True if the OpenSSL PRNG has been seeded with enough data and False if not.\n"
"\n"
"It is necessary to seed the PRNG with RAND_add() on some platforms before\n"
"using the ssl() function.");

#define _SSL_RAND_STATUS_METHODDEF    \
    {"RAND_status", (PyCFunction)_ssl_RAND_status, METH_NOARGS, _ssl_RAND_status__doc__},

static PyObject *
_ssl_RAND_status_impl(PyObject *module);

static PyObject *
_ssl_RAND_status(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _ssl_RAND_status_impl(module);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_get_default_verify_paths__doc__,
"get_default_verify_paths($module, /)\n"
"--\n"
"\n"
"Return search paths and environment vars that are used by SSLContext\'s set_default_verify_paths() to load default CAs.\n"
"\n"
"The values are \'cert_file_env\', \'cert_file\', \'cert_dir_env\', \'cert_dir\'.");

#define _SSL_GET_DEFAULT_VERIFY_PATHS_METHODDEF    \
    {"get_default_verify_paths", (PyCFunction)_ssl_get_default_verify_paths, METH_NOARGS, _ssl_get_default_verify_paths__doc__},

static PyObject *
_ssl_get_default_verify_paths_impl(PyObject *module);

static PyObject *
_ssl_get_default_verify_paths(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _ssl_get_default_verify_paths_impl(module);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_txt2obj__doc__,
"txt2obj($module, /, txt, name=False)\n"
"--\n"
"\n"
"Lookup NID, short name, long name and OID of an ASN1_OBJECT.\n"
"\n"
"By default objects are looked up by OID. With name=True short and\n"
"long name are also matched.");

#define _SSL_TXT2OBJ_METHODDEF    \
    {"txt2obj", _PyCFunction_CAST(_ssl_txt2obj), METH_FASTCALL|METH_KEYWORDS, _ssl_txt2obj__doc__},

static PyObject *
_ssl_txt2obj_impl(PyObject *module, const char *txt, int name);

static PyObject *
_ssl_txt2obj(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(txt), &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"txt", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "txt2obj",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    const char *txt;
    int name = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("txt2obj", "argument 'txt'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t txt_length;
    txt = PyUnicode_AsUTF8AndSize(args[0], &txt_length);
    if (txt == NULL) {
        goto exit;
    }
    if (strlen(txt) != (size_t)txt_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    name = PyObject_IsTrue(args[1]);
    if (name < 0) {
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _ssl_txt2obj_impl(module, txt, name);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_nid2obj__doc__,
"nid2obj($module, nid, /)\n"
"--\n"
"\n"
"Lookup NID, short name, long name and OID of an ASN1_OBJECT by NID.");

#define _SSL_NID2OBJ_METHODDEF    \
    {"nid2obj", (PyCFunction)_ssl_nid2obj, METH_O, _ssl_nid2obj__doc__},

static PyObject *
_ssl_nid2obj_impl(PyObject *module, int nid);

static PyObject *
_ssl_nid2obj(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int nid;

    nid = PyLong_AsInt(arg);
    if (nid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _ssl_nid2obj_impl(module, nid);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#if defined(_MSC_VER)

PyDoc_STRVAR(_ssl_enum_certificates__doc__,
"enum_certificates($module, /, store_name)\n"
"--\n"
"\n"
"Retrieve certificates from Windows\' cert store.\n"
"\n"
"store_name may be one of \'CA\', \'ROOT\' or \'MY\'.  The system may provide\n"
"more cert storages, too.  The function returns a list of (bytes,\n"
"encoding_type, trust) tuples.  The encoding_type flag can be interpreted\n"
"with X509_ASN_ENCODING or PKCS_7_ASN_ENCODING. The trust setting is either\n"
"a set of OIDs or the boolean True.");

#define _SSL_ENUM_CERTIFICATES_METHODDEF    \
    {"enum_certificates", _PyCFunction_CAST(_ssl_enum_certificates), METH_FASTCALL|METH_KEYWORDS, _ssl_enum_certificates__doc__},

static PyObject *
_ssl_enum_certificates_impl(PyObject *module, const char *store_name);

static PyObject *
_ssl_enum_certificates(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(store_name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"store_name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enum_certificates",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    const char *store_name;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("enum_certificates", "argument 'store_name'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t store_name_length;
    store_name = PyUnicode_AsUTF8AndSize(args[0], &store_name_length);
    if (store_name == NULL) {
        goto exit;
    }
    if (strlen(store_name) != (size_t)store_name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _ssl_enum_certificates_impl(module, store_name);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(_MSC_VER) */

#if defined(_MSC_VER)

PyDoc_STRVAR(_ssl_enum_crls__doc__,
"enum_crls($module, /, store_name)\n"
"--\n"
"\n"
"Retrieve CRLs from Windows\' cert store.\n"
"\n"
"store_name may be one of \'CA\', \'ROOT\' or \'MY\'.  The system may provide\n"
"more cert storages, too.  The function returns a list of (bytes,\n"
"encoding_type) tuples.  The encoding_type flag can be interpreted with\n"
"X509_ASN_ENCODING or PKCS_7_ASN_ENCODING.");

#define _SSL_ENUM_CRLS_METHODDEF    \
    {"enum_crls", _PyCFunction_CAST(_ssl_enum_crls), METH_FASTCALL|METH_KEYWORDS, _ssl_enum_crls__doc__},

static PyObject *
_ssl_enum_crls_impl(PyObject *module, const char *store_name);

static PyObject *
_ssl_enum_crls(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(store_name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"store_name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enum_crls",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    const char *store_name;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("enum_crls", "argument 'store_name'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t store_name_length;
    store_name = PyUnicode_AsUTF8AndSize(args[0], &store_name_length);
    if (store_name == NULL) {
        goto exit;
    }
    if (strlen(store_name) != (size_t)store_name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _ssl_enum_crls_impl(module, store_name);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(_MSC_VER) */

#ifndef _SSL_ENUM_CERTIFICATES_METHODDEF
    #define _SSL_ENUM_CERTIFICATES_METHODDEF
#endif /* !defined(_SSL_ENUM_CERTIFICATES_METHODDEF) */

#ifndef _SSL_ENUM_CRLS_METHODDEF
    #define _SSL_ENUM_CRLS_METHODDEF
#endif /* !defined(_SSL_ENUM_CRLS_METHODDEF) */
/*[clinic end generated code: output=8c4a1e44702afeb7 input=a9049054013a1b77]*/
