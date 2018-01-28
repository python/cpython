/*[clinic input]
preserve
[clinic start generated code]*/

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
    return _ssl__SSLSocket_do_handshake_impl(self);
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

    if (!PyArg_Parse(arg, "O&:_test_decode_cert", PyUnicode_FSConverter, &path)) {
        goto exit;
    }
    return_value = _ssl__test_decode_cert_impl(module, path);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_peer_certificate__doc__,
"peer_certificate($self, der=False, /)\n"
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

#define _SSL__SSLSOCKET_PEER_CERTIFICATE_METHODDEF    \
    {"peer_certificate", (PyCFunction)_ssl__SSLSocket_peer_certificate, METH_FASTCALL, _ssl__SSLSocket_peer_certificate__doc__},

static PyObject *
_ssl__SSLSocket_peer_certificate_impl(PySSLSocket *self, int binary_mode);

static PyObject *
_ssl__SSLSocket_peer_certificate(PySSLSocket *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int binary_mode = 0;

    if (!_PyArg_ParseStack(args, nargs, "|p:peer_certificate",
        &binary_mode)) {
        goto exit;
    }
    return_value = _ssl__SSLSocket_peer_certificate_impl(self, binary_mode);

exit:
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
    return _ssl__SSLSocket_shared_ciphers_impl(self);
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
    return _ssl__SSLSocket_cipher_impl(self);
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
    return _ssl__SSLSocket_version_impl(self);
}

#if (defined(OPENSSL_NPN_NEGOTIATED) && !defined(OPENSSL_NO_NEXTPROTONEG))

PyDoc_STRVAR(_ssl__SSLSocket_selected_npn_protocol__doc__,
"selected_npn_protocol($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_SELECTED_NPN_PROTOCOL_METHODDEF    \
    {"selected_npn_protocol", (PyCFunction)_ssl__SSLSocket_selected_npn_protocol, METH_NOARGS, _ssl__SSLSocket_selected_npn_protocol__doc__},

static PyObject *
_ssl__SSLSocket_selected_npn_protocol_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_selected_npn_protocol(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    return _ssl__SSLSocket_selected_npn_protocol_impl(self);
}

#endif /* (defined(OPENSSL_NPN_NEGOTIATED) && !defined(OPENSSL_NO_NEXTPROTONEG)) */

#if defined(HAVE_ALPN)

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
    return _ssl__SSLSocket_selected_alpn_protocol_impl(self);
}

#endif /* defined(HAVE_ALPN) */

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

    if (!PyArg_Parse(arg, "y*:write", &b)) {
        goto exit;
    }
    return_value = _ssl__SSLSocket_write_impl(self, &b);

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
    return _ssl__SSLSocket_pending_impl(self);
}

PyDoc_STRVAR(_ssl__SSLSocket_read__doc__,
"read(size, [buffer])\n"
"Read up to size bytes from the SSL socket.");

#define _SSL__SSLSOCKET_READ_METHODDEF    \
    {"read", (PyCFunction)_ssl__SSLSocket_read, METH_VARARGS, _ssl__SSLSocket_read__doc__},

static PyObject *
_ssl__SSLSocket_read_impl(PySSLSocket *self, int len, int group_right_1,
                          Py_buffer *buffer);

static PyObject *
_ssl__SSLSocket_read(PySSLSocket *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int len;
    int group_right_1 = 0;
    Py_buffer buffer = {NULL, NULL};

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "i:read", &len)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "iw*:read", &len, &buffer)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_ssl._SSLSocket.read requires 1 to 2 arguments");
            goto exit;
    }
    return_value = _ssl__SSLSocket_read_impl(self, len, group_right_1, &buffer);

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
"Does the SSL shutdown handshake with the remote end.\n"
"\n"
"Returns the underlying socket object.");

#define _SSL__SSLSOCKET_SHUTDOWN_METHODDEF    \
    {"shutdown", (PyCFunction)_ssl__SSLSocket_shutdown, METH_NOARGS, _ssl__SSLSocket_shutdown__doc__},

static PyObject *
_ssl__SSLSocket_shutdown_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_shutdown(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    return _ssl__SSLSocket_shutdown_impl(self);
}

PyDoc_STRVAR(_ssl__SSLSocket_tls_unique_cb__doc__,
"tls_unique_cb($self, /)\n"
"--\n"
"\n"
"Returns the \'tls-unique\' channel binding data, as defined by RFC 5929.\n"
"\n"
"If the TLS handshake is not yet complete, None is returned.");

#define _SSL__SSLSOCKET_TLS_UNIQUE_CB_METHODDEF    \
    {"tls_unique_cb", (PyCFunction)_ssl__SSLSocket_tls_unique_cb, METH_NOARGS, _ssl__SSLSocket_tls_unique_cb__doc__},

static PyObject *
_ssl__SSLSocket_tls_unique_cb_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_tls_unique_cb(PySSLSocket *self, PyObject *Py_UNUSED(ignored))
{
    return _ssl__SSLSocket_tls_unique_cb_impl(self);
}

static PyObject *
_ssl__SSLContext_impl(PyTypeObject *type, int proto_version);

static PyObject *
_ssl__SSLContext(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    int proto_version;

    if ((type == &PySSLContext_Type) &&
        !_PyArg_NoKeywords("_SSLContext", kwargs)) {
        goto exit;
    }
    if (!PyArg_ParseTuple(args, "i:_SSLContext",
        &proto_version)) {
        goto exit;
    }
    return_value = _ssl__SSLContext_impl(type, proto_version);

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

    if (!PyArg_Parse(arg, "s:set_ciphers", &cipherlist)) {
        goto exit;
    }
    return_value = _ssl__SSLContext_set_ciphers_impl(self, cipherlist);

exit:
    return return_value;
}

#if (OPENSSL_VERSION_NUMBER >= 0x10002000UL)

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
    return _ssl__SSLContext_get_ciphers_impl(self);
}

#endif /* (OPENSSL_VERSION_NUMBER >= 0x10002000UL) */

PyDoc_STRVAR(_ssl__SSLContext__set_npn_protocols__doc__,
"_set_npn_protocols($self, protos, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__SET_NPN_PROTOCOLS_METHODDEF    \
    {"_set_npn_protocols", (PyCFunction)_ssl__SSLContext__set_npn_protocols, METH_O, _ssl__SSLContext__set_npn_protocols__doc__},

static PyObject *
_ssl__SSLContext__set_npn_protocols_impl(PySSLContext *self,
                                         Py_buffer *protos);

static PyObject *
_ssl__SSLContext__set_npn_protocols(PySSLContext *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer protos = {NULL, NULL};

    if (!PyArg_Parse(arg, "y*:_set_npn_protocols", &protos)) {
        goto exit;
    }
    return_value = _ssl__SSLContext__set_npn_protocols_impl(self, &protos);

exit:
    /* Cleanup for protos */
    if (protos.obj) {
       PyBuffer_Release(&protos);
    }

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

    if (!PyArg_Parse(arg, "y*:_set_alpn_protocols", &protos)) {
        goto exit;
    }
    return_value = _ssl__SSLContext__set_alpn_protocols_impl(self, &protos);

exit:
    /* Cleanup for protos */
    if (protos.obj) {
       PyBuffer_Release(&protos);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_cert_chain__doc__,
"load_cert_chain($self, /, certfile, keyfile=None, password=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_CERT_CHAIN_METHODDEF    \
    {"load_cert_chain", (PyCFunction)_ssl__SSLContext_load_cert_chain, METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext_load_cert_chain__doc__},

static PyObject *
_ssl__SSLContext_load_cert_chain_impl(PySSLContext *self, PyObject *certfile,
                                      PyObject *keyfile, PyObject *password);

static PyObject *
_ssl__SSLContext_load_cert_chain(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"certfile", "keyfile", "password", NULL};
    static _PyArg_Parser _parser = {"O|OO:load_cert_chain", _keywords, 0};
    PyObject *certfile;
    PyObject *keyfile = NULL;
    PyObject *password = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &certfile, &keyfile, &password)) {
        goto exit;
    }
    return_value = _ssl__SSLContext_load_cert_chain_impl(self, certfile, keyfile, password);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_verify_locations__doc__,
"load_verify_locations($self, /, cafile=None, capath=None, cadata=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_VERIFY_LOCATIONS_METHODDEF    \
    {"load_verify_locations", (PyCFunction)_ssl__SSLContext_load_verify_locations, METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext_load_verify_locations__doc__},

static PyObject *
_ssl__SSLContext_load_verify_locations_impl(PySSLContext *self,
                                            PyObject *cafile,
                                            PyObject *capath,
                                            PyObject *cadata);

static PyObject *
_ssl__SSLContext_load_verify_locations(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"cafile", "capath", "cadata", NULL};
    static _PyArg_Parser _parser = {"|OOO:load_verify_locations", _keywords, 0};
    PyObject *cafile = NULL;
    PyObject *capath = NULL;
    PyObject *cadata = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &cafile, &capath, &cadata)) {
        goto exit;
    }
    return_value = _ssl__SSLContext_load_verify_locations_impl(self, cafile, capath, cadata);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_dh_params__doc__,
"load_dh_params($self, path, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_DH_PARAMS_METHODDEF    \
    {"load_dh_params", (PyCFunction)_ssl__SSLContext_load_dh_params, METH_O, _ssl__SSLContext_load_dh_params__doc__},

PyDoc_STRVAR(_ssl__SSLContext__wrap_socket__doc__,
"_wrap_socket($self, /, sock, server_side, server_hostname=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__WRAP_SOCKET_METHODDEF    \
    {"_wrap_socket", (PyCFunction)_ssl__SSLContext__wrap_socket, METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext__wrap_socket__doc__},

static PyObject *
_ssl__SSLContext__wrap_socket_impl(PySSLContext *self, PyObject *sock,
                                   int server_side, PyObject *hostname_obj);

static PyObject *
_ssl__SSLContext__wrap_socket(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sock", "server_side", "server_hostname", NULL};
    static _PyArg_Parser _parser = {"O!i|O:_wrap_socket", _keywords, 0};
    PyObject *sock;
    int server_side;
    PyObject *hostname_obj = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        PySocketModule.Sock_Type, &sock, &server_side, &hostname_obj)) {
        goto exit;
    }
    return_value = _ssl__SSLContext__wrap_socket_impl(self, sock, server_side, hostname_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext__wrap_bio__doc__,
"_wrap_bio($self, /, incoming, outgoing, server_side,\n"
"          server_hostname=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__WRAP_BIO_METHODDEF    \
    {"_wrap_bio", (PyCFunction)_ssl__SSLContext__wrap_bio, METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext__wrap_bio__doc__},

static PyObject *
_ssl__SSLContext__wrap_bio_impl(PySSLContext *self, PySSLMemoryBIO *incoming,
                                PySSLMemoryBIO *outgoing, int server_side,
                                PyObject *hostname_obj);

static PyObject *
_ssl__SSLContext__wrap_bio(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"incoming", "outgoing", "server_side", "server_hostname", NULL};
    static _PyArg_Parser _parser = {"O!O!i|O:_wrap_bio", _keywords, 0};
    PySSLMemoryBIO *incoming;
    PySSLMemoryBIO *outgoing;
    int server_side;
    PyObject *hostname_obj = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &PySSLMemoryBIO_Type, &incoming, &PySSLMemoryBIO_Type, &outgoing, &server_side, &hostname_obj)) {
        goto exit;
    }
    return_value = _ssl__SSLContext__wrap_bio_impl(self, incoming, outgoing, server_side, hostname_obj);

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
    return _ssl__SSLContext_session_stats_impl(self);
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
    return _ssl__SSLContext_set_default_verify_paths_impl(self);
}

#if !defined(OPENSSL_NO_ECDH)

PyDoc_STRVAR(_ssl__SSLContext_set_ecdh_curve__doc__,
"set_ecdh_curve($self, name, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_ECDH_CURVE_METHODDEF    \
    {"set_ecdh_curve", (PyCFunction)_ssl__SSLContext_set_ecdh_curve, METH_O, _ssl__SSLContext_set_ecdh_curve__doc__},

#endif /* !defined(OPENSSL_NO_ECDH) */

PyDoc_STRVAR(_ssl__SSLContext_set_servername_callback__doc__,
"set_servername_callback($self, method, /)\n"
"--\n"
"\n"
"Set a callback that will be called when a server name is provided by the SSL/TLS client in the SNI extension.\n"
"\n"
"If the argument is None then the callback is disabled. The method is called\n"
"with the SSLSocket, the server name as a string, and the SSLContext object.\n"
"See RFC 6066 for details of the SNI extension.");

#define _SSL__SSLCONTEXT_SET_SERVERNAME_CALLBACK_METHODDEF    \
    {"set_servername_callback", (PyCFunction)_ssl__SSLContext_set_servername_callback, METH_O, _ssl__SSLContext_set_servername_callback__doc__},

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
    return _ssl__SSLContext_cert_store_stats_impl(self);
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
    {"get_ca_certs", (PyCFunction)_ssl__SSLContext_get_ca_certs, METH_FASTCALL|METH_KEYWORDS, _ssl__SSLContext_get_ca_certs__doc__},

static PyObject *
_ssl__SSLContext_get_ca_certs_impl(PySSLContext *self, int binary_form);

static PyObject *
_ssl__SSLContext_get_ca_certs(PySSLContext *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"binary_form", NULL};
    static _PyArg_Parser _parser = {"|p:get_ca_certs", _keywords, 0};
    int binary_form = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &binary_form)) {
        goto exit;
    }
    return_value = _ssl__SSLContext_get_ca_certs_impl(self, binary_form);

exit:
    return return_value;
}

static PyObject *
_ssl_MemoryBIO_impl(PyTypeObject *type);

static PyObject *
_ssl_MemoryBIO(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;

    if ((type == &PySSLMemoryBIO_Type) &&
        !_PyArg_NoPositional("MemoryBIO", args)) {
        goto exit;
    }
    if ((type == &PySSLMemoryBIO_Type) &&
        !_PyArg_NoKeywords("MemoryBIO", kwargs)) {
        goto exit;
    }
    return_value = _ssl_MemoryBIO_impl(type);

exit:
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
    {"read", (PyCFunction)_ssl_MemoryBIO_read, METH_FASTCALL, _ssl_MemoryBIO_read__doc__},

static PyObject *
_ssl_MemoryBIO_read_impl(PySSLMemoryBIO *self, int len);

static PyObject *
_ssl_MemoryBIO_read(PySSLMemoryBIO *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int len = -1;

    if (!_PyArg_ParseStack(args, nargs, "|i:read",
        &len)) {
        goto exit;
    }
    return_value = _ssl_MemoryBIO_read_impl(self, len);

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

    if (!PyArg_Parse(arg, "y*:write", &b)) {
        goto exit;
    }
    return_value = _ssl_MemoryBIO_write_impl(self, &b);

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
    return _ssl_MemoryBIO_write_eof_impl(self);
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
    {"RAND_add", (PyCFunction)_ssl_RAND_add, METH_FASTCALL, _ssl_RAND_add__doc__},

static PyObject *
_ssl_RAND_add_impl(PyObject *module, Py_buffer *view, double entropy);

static PyObject *
_ssl_RAND_add(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer view = {NULL, NULL};
    double entropy;

    if (!_PyArg_ParseStack(args, nargs, "s*d:RAND_add",
        &view, &entropy)) {
        goto exit;
    }
    return_value = _ssl_RAND_add_impl(module, &view, entropy);

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

    if (!PyArg_Parse(arg, "i:RAND_bytes", &n)) {
        goto exit;
    }
    return_value = _ssl_RAND_bytes_impl(module, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_RAND_pseudo_bytes__doc__,
"RAND_pseudo_bytes($module, n, /)\n"
"--\n"
"\n"
"Generate n pseudo-random bytes.\n"
"\n"
"Return a pair (bytes, is_cryptographic).  is_cryptographic is True\n"
"if the bytes generated are cryptographically strong.");

#define _SSL_RAND_PSEUDO_BYTES_METHODDEF    \
    {"RAND_pseudo_bytes", (PyCFunction)_ssl_RAND_pseudo_bytes, METH_O, _ssl_RAND_pseudo_bytes__doc__},

static PyObject *
_ssl_RAND_pseudo_bytes_impl(PyObject *module, int n);

static PyObject *
_ssl_RAND_pseudo_bytes(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int n;

    if (!PyArg_Parse(arg, "i:RAND_pseudo_bytes", &n)) {
        goto exit;
    }
    return_value = _ssl_RAND_pseudo_bytes_impl(module, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_RAND_status__doc__,
"RAND_status($module, /)\n"
"--\n"
"\n"
"Returns 1 if the OpenSSL PRNG has been seeded with enough data and 0 if not.\n"
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
    return _ssl_RAND_status_impl(module);
}

#if !defined(OPENSSL_NO_EGD)

PyDoc_STRVAR(_ssl_RAND_egd__doc__,
"RAND_egd($module, path, /)\n"
"--\n"
"\n"
"Queries the entropy gather daemon (EGD) on the socket named by \'path\'.\n"
"\n"
"Returns number of bytes read.  Raises SSLError if connection to EGD\n"
"fails or if it does not provide enough data to seed PRNG.");

#define _SSL_RAND_EGD_METHODDEF    \
    {"RAND_egd", (PyCFunction)_ssl_RAND_egd, METH_O, _ssl_RAND_egd__doc__},

static PyObject *
_ssl_RAND_egd_impl(PyObject *module, PyObject *path);

static PyObject *
_ssl_RAND_egd(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *path;

    if (!PyArg_Parse(arg, "O&:RAND_egd", PyUnicode_FSConverter, &path)) {
        goto exit;
    }
    return_value = _ssl_RAND_egd_impl(module, path);

exit:
    return return_value;
}

#endif /* !defined(OPENSSL_NO_EGD) */

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
    return _ssl_get_default_verify_paths_impl(module);
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
    {"txt2obj", (PyCFunction)_ssl_txt2obj, METH_FASTCALL|METH_KEYWORDS, _ssl_txt2obj__doc__},

static PyObject *
_ssl_txt2obj_impl(PyObject *module, const char *txt, int name);

static PyObject *
_ssl_txt2obj(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"txt", "name", NULL};
    static _PyArg_Parser _parser = {"s|p:txt2obj", _keywords, 0};
    const char *txt;
    int name = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &txt, &name)) {
        goto exit;
    }
    return_value = _ssl_txt2obj_impl(module, txt, name);

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

    if (!PyArg_Parse(arg, "i:nid2obj", &nid)) {
        goto exit;
    }
    return_value = _ssl_nid2obj_impl(module, nid);

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
    {"enum_certificates", (PyCFunction)_ssl_enum_certificates, METH_FASTCALL|METH_KEYWORDS, _ssl_enum_certificates__doc__},

static PyObject *
_ssl_enum_certificates_impl(PyObject *module, const char *store_name);

static PyObject *
_ssl_enum_certificates(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"store_name", NULL};
    static _PyArg_Parser _parser = {"s:enum_certificates", _keywords, 0};
    const char *store_name;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &store_name)) {
        goto exit;
    }
    return_value = _ssl_enum_certificates_impl(module, store_name);

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
    {"enum_crls", (PyCFunction)_ssl_enum_crls, METH_FASTCALL|METH_KEYWORDS, _ssl_enum_crls__doc__},

static PyObject *
_ssl_enum_crls_impl(PyObject *module, const char *store_name);

static PyObject *
_ssl_enum_crls(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"store_name", NULL};
    static _PyArg_Parser _parser = {"s:enum_crls", _keywords, 0};
    const char *store_name;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &store_name)) {
        goto exit;
    }
    return_value = _ssl_enum_crls_impl(module, store_name);

exit:
    return return_value;
}

#endif /* defined(_MSC_VER) */

#ifndef _SSL__SSLSOCKET_SELECTED_NPN_PROTOCOL_METHODDEF
    #define _SSL__SSLSOCKET_SELECTED_NPN_PROTOCOL_METHODDEF
#endif /* !defined(_SSL__SSLSOCKET_SELECTED_NPN_PROTOCOL_METHODDEF) */

#ifndef _SSL__SSLSOCKET_SELECTED_ALPN_PROTOCOL_METHODDEF
    #define _SSL__SSLSOCKET_SELECTED_ALPN_PROTOCOL_METHODDEF
#endif /* !defined(_SSL__SSLSOCKET_SELECTED_ALPN_PROTOCOL_METHODDEF) */

#ifndef _SSL__SSLCONTEXT_GET_CIPHERS_METHODDEF
    #define _SSL__SSLCONTEXT_GET_CIPHERS_METHODDEF
#endif /* !defined(_SSL__SSLCONTEXT_GET_CIPHERS_METHODDEF) */

#ifndef _SSL__SSLCONTEXT_SET_ECDH_CURVE_METHODDEF
    #define _SSL__SSLCONTEXT_SET_ECDH_CURVE_METHODDEF
#endif /* !defined(_SSL__SSLCONTEXT_SET_ECDH_CURVE_METHODDEF) */

#ifndef _SSL_RAND_EGD_METHODDEF
    #define _SSL_RAND_EGD_METHODDEF
#endif /* !defined(_SSL_RAND_EGD_METHODDEF) */

#ifndef _SSL_ENUM_CERTIFICATES_METHODDEF
    #define _SSL_ENUM_CERTIFICATES_METHODDEF
#endif /* !defined(_SSL_ENUM_CERTIFICATES_METHODDEF) */

#ifndef _SSL_ENUM_CRLS_METHODDEF
    #define _SSL_ENUM_CRLS_METHODDEF
#endif /* !defined(_SSL_ENUM_CRLS_METHODDEF) */
/*[clinic end generated code: output=3d42305ed0ad162a input=a9049054013a1b77]*/
