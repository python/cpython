/* Debug helpers */

#ifndef SSL3_MT_CHANGE_CIPHER_SPEC
/* Dummy message type for handling CCS like a normal handshake message
 * not defined in OpenSSL 1.0.2
 */
#define SSL3_MT_CHANGE_CIPHER_SPEC              0x0101
#endif

static void
_PySSL_msg_callback(int write_p, int version, int content_type,
                    const void *buf, size_t len, SSL *ssl, void *arg)
{
    const char *cbuf = (const char *)buf;
    PyGILState_STATE threadstate;
    PyObject *res = NULL;
    PySSLSocket *ssl_obj = NULL;  /* ssl._SSLSocket, borrowed ref */
    int msg_type;

    threadstate = PyGILState_Ensure();

    ssl_obj = (PySSLSocket *)SSL_get_app_data(ssl);
    assert(Py_IS_TYPE(ssl_obj, get_state_sock(ssl_obj)->PySSLSocket_Type));
    if (ssl_obj->ctx->msg_cb == NULL) {
        PyGILState_Release(threadstate);
        return;
    }

    PyObject *ssl_socket;  /* ssl.SSLSocket or ssl.SSLObject */
    if (ssl_obj->owner)
        PyWeakref_GetRef(ssl_obj->owner, &ssl_socket);
    else if (ssl_obj->Socket)
        PyWeakref_GetRef(ssl_obj->Socket, &ssl_socket);
    else
        ssl_socket = (PyObject *)Py_NewRef(ssl_obj);
    assert(ssl_socket != NULL);  // PyWeakref_GetRef() can return NULL

    /* assume that OpenSSL verifies all payload and buf len is of sufficient
       length */
    switch(content_type) {
      case SSL3_RT_CHANGE_CIPHER_SPEC:
        msg_type = SSL3_MT_CHANGE_CIPHER_SPEC;
        break;
      case SSL3_RT_ALERT:
        /* byte 0: level */
        /* byte 1: alert type */
        msg_type = (int)cbuf[1];
        break;
      case SSL3_RT_HANDSHAKE:
        msg_type = (int)cbuf[0];
        break;
#ifdef SSL3_RT_HEADER
      case SSL3_RT_HEADER:
        /* frame header encodes version in bytes 1..2 */
        version = cbuf[1] << 8 | cbuf[2];
        msg_type = (int)cbuf[0];
        break;
#endif
#ifdef SSL3_RT_INNER_CONTENT_TYPE
      case SSL3_RT_INNER_CONTENT_TYPE:
        msg_type = (int)cbuf[0];
        break;
#endif
      default:
        /* never SSL3_RT_APPLICATION_DATA */
        msg_type = -1;
        break;
    }

    res = PyObject_CallFunction(
        ssl_obj->ctx->msg_cb, "Osiiiy#",
        ssl_socket, write_p ? "write" : "read",
        version, content_type, msg_type,
        buf, len
    );
    if (res == NULL) {
        ssl_obj->exc = PyErr_GetRaisedException();
    } else {
        Py_DECREF(res);
    }
    Py_XDECREF(ssl_socket);

    PyGILState_Release(threadstate);
}


static PyObject *
_PySSLContext_get_msg_callback(PyObject *op, void *Py_UNUSED(closure))
{
    PySSLContext *self = PySSLContext_CAST(op);
    if (self->msg_cb != NULL) {
        return Py_NewRef(self->msg_cb);
    } else {
        Py_RETURN_NONE;
    }
}

static int
_PySSLContext_set_msg_callback(PyObject *op, PyObject *arg,
                               void *Py_UNUSED(closure))
{
    PySSLContext *self = PySSLContext_CAST(op);
    Py_CLEAR(self->msg_cb);
    if (arg == Py_None) {
        SSL_CTX_set_msg_callback(self->ctx, NULL);
    }
    else {
        if (!PyCallable_Check(arg)) {
            SSL_CTX_set_msg_callback(self->ctx, NULL);
            PyErr_SetString(PyExc_TypeError,
                            "not a callable object");
            return -1;
        }
        self->msg_cb = Py_NewRef(arg);
        SSL_CTX_set_msg_callback(self->ctx, _PySSL_msg_callback);
    }
    return 0;
}

static void
_PySSL_keylog_callback(const SSL *ssl, const char *line)
{
    PyGILState_STATE threadstate;
    PySSLSocket *ssl_obj = NULL;  /* ssl._SSLSocket, borrowed ref */
    int res, e;

    threadstate = PyGILState_Ensure();

    ssl_obj = (PySSLSocket *)SSL_get_app_data(ssl);
    assert(Py_IS_TYPE(ssl_obj, get_state_sock(ssl_obj)->PySSLSocket_Type));
    PyThread_type_lock lock = get_state_sock(ssl_obj)->keylog_lock;
    assert(lock != NULL);
    if (ssl_obj->ctx->keylog_bio == NULL) {
        return;
    }
    /*
     * The lock is neither released on exit nor on fork(). The lock is
     * also shared between all SSLContexts although contexts may write to
     * their own files. IMHO that's good enough for a non-performance
     * critical debug helper.
     */

    PySSL_BEGIN_ALLOW_THREADS
    PyThread_acquire_lock(lock, 1);
    res = BIO_printf(ssl_obj->ctx->keylog_bio, "%s\n", line);
    e = errno;
    (void)BIO_flush(ssl_obj->ctx->keylog_bio);
    PyThread_release_lock(lock);
    PySSL_END_ALLOW_THREADS

    if (res == -1) {
        errno = e;
        PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError,
                                             ssl_obj->ctx->keylog_filename);
        ssl_obj->exc = PyErr_GetRaisedException();
    }
    PyGILState_Release(threadstate);
}

static PyObject *
_PySSLContext_get_keylog_filename(PyObject *op, void *Py_UNUSED(closure))
{
    PySSLContext *self = PySSLContext_CAST(op);
    if (self->keylog_filename != NULL) {
        return Py_NewRef(self->keylog_filename);
    } else {
        Py_RETURN_NONE;
    }
}

static int
_PySSLContext_set_keylog_filename(PyObject *op, PyObject *arg,
                                  void *Py_UNUSED(closure))
{
    PySSLContext *self = PySSLContext_CAST(op);
    FILE *fp;

#if defined(MS_WINDOWS) && defined(_DEBUG)
    PyErr_SetString(PyExc_NotImplementedError,
                    "set_keylog_filename: unavailable on Windows debug build");
    return -1;
#endif

    /* Reset variables and callback first */
    SSL_CTX_set_keylog_callback(self->ctx, NULL);
    Py_CLEAR(self->keylog_filename);
    if (self->keylog_bio != NULL) {
        BIO *bio = self->keylog_bio;
        self->keylog_bio = NULL;
        PySSL_BEGIN_ALLOW_THREADS
        BIO_free_all(bio);
        PySSL_END_ALLOW_THREADS
    }

    if (arg == Py_None) {
        /* None disables the callback */
        return 0;
    }

    /* Py_fopen() also checks that arg is of proper type. */
    fp = Py_fopen(arg, "a" PY_STDIOTEXTMODE);
    if (fp == NULL)
        return -1;

    self->keylog_bio = BIO_new_fp(fp, BIO_CLOSE | BIO_FP_TEXT);
    if (self->keylog_bio == NULL) {
        PyErr_SetString(get_state_ctx(self)->PySSLErrorObject,
                        "Can't malloc memory for keylog file");
        return -1;
    }
    self->keylog_filename = Py_NewRef(arg);

    /* Write a header for seekable, empty files (this excludes pipes). */
    PySSL_BEGIN_ALLOW_THREADS
    if (BIO_tell(self->keylog_bio) == 0) {
        BIO_puts(self->keylog_bio,
                 "# TLS secrets log file, generated by OpenSSL / Python\n");
        (void)BIO_flush(self->keylog_bio);
    }
    PySSL_END_ALLOW_THREADS
    SSL_CTX_set_keylog_callback(self->ctx, _PySSL_keylog_callback);
    return 0;
}
