/* SSL socket module

   SSL support based on patches by Brian E Gallew and Laszlo Kovacs.
   Re-worked a bit by Bill Janssen to add server-side support and
   certificate decoding.  Chris Stawarz contributed some non-blocking
   patches.

   This module is imported by ssl.py. It should *not* be used
   directly.

   XXX should partial writes be enabled, SSL_MODE_ENABLE_PARTIAL_WRITE?

   XXX integrate several "shutdown modes" as suggested in
       http://bugs.python.org/issue8108#msg102867 ?
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#ifdef WITH_THREAD
#include "pythread.h"


#define PySSL_BEGIN_ALLOW_THREADS_S(save) \
    do { if (_ssl_locks_count>0) { (save) = PyEval_SaveThread(); } } while (0)
#define PySSL_END_ALLOW_THREADS_S(save) \
    do { if (_ssl_locks_count>0) { PyEval_RestoreThread(save); } } while (0)
#define PySSL_BEGIN_ALLOW_THREADS { \
            PyThreadState *_save = NULL;  \
            PySSL_BEGIN_ALLOW_THREADS_S(_save);
#define PySSL_BLOCK_THREADS     PySSL_END_ALLOW_THREADS_S(_save);
#define PySSL_UNBLOCK_THREADS   PySSL_BEGIN_ALLOW_THREADS_S(_save);
#define PySSL_END_ALLOW_THREADS PySSL_END_ALLOW_THREADS_S(_save); }

#else   /* no WITH_THREAD */

#define PySSL_BEGIN_ALLOW_THREADS_S(save)
#define PySSL_END_ALLOW_THREADS_S(save)
#define PySSL_BEGIN_ALLOW_THREADS
#define PySSL_BLOCK_THREADS
#define PySSL_UNBLOCK_THREADS
#define PySSL_END_ALLOW_THREADS

#endif

/* Include symbols from _socket module */
#include "socketmodule.h"

#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif

/* Include OpenSSL header files */
#include "openssl/rsa.h"
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/x509v3.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"

/* SSL error object */
static PyObject *PySSLErrorObject;
static PyObject *PySSLZeroReturnErrorObject;
static PyObject *PySSLWantReadErrorObject;
static PyObject *PySSLWantWriteErrorObject;
static PyObject *PySSLSyscallErrorObject;
static PyObject *PySSLEOFErrorObject;

/* Error mappings */
static PyObject *err_codes_to_names;
static PyObject *err_names_to_codes;
static PyObject *lib_codes_to_names;

struct py_ssl_error_code {
    const char *mnemonic;
    int library, reason;
};
struct py_ssl_library_code {
    const char *library;
    int code;
};

/* Include generated data (error codes) */
#include "_ssl_data.h"

/* Openssl comes with TLSv1.1 and TLSv1.2 between 1.0.0h and 1.0.1
    http://www.openssl.org/news/changelog.html
 */
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
# define HAVE_TLSv1_2 1
#else
# define HAVE_TLSv1_2 0
#endif

/* SNI support (client- and server-side) appeared in OpenSSL 1.0.0 and 0.9.8f
 * This includes the SSL_set_SSL_CTX() function.
 */
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
# define HAVE_SNI 1
#else
# define HAVE_SNI 0
#endif

/* ALPN added in OpenSSL 1.0.2 */
#if !defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x1000200fL && !defined(OPENSSL_NO_TLSEXT)
# define HAVE_ALPN
#endif

enum py_ssl_error {
    /* these mirror ssl.h */
    PY_SSL_ERROR_NONE,
    PY_SSL_ERROR_SSL,
    PY_SSL_ERROR_WANT_READ,
    PY_SSL_ERROR_WANT_WRITE,
    PY_SSL_ERROR_WANT_X509_LOOKUP,
    PY_SSL_ERROR_SYSCALL,     /* look at error stack/return value/errno */
    PY_SSL_ERROR_ZERO_RETURN,
    PY_SSL_ERROR_WANT_CONNECT,
    /* start of non ssl.h errorcodes */
    PY_SSL_ERROR_EOF,         /* special case of SSL_ERROR_SYSCALL */
    PY_SSL_ERROR_NO_SOCKET,   /* socket has been GC'd */
    PY_SSL_ERROR_INVALID_ERROR_CODE
};

enum py_ssl_server_or_client {
    PY_SSL_CLIENT,
    PY_SSL_SERVER
};

enum py_ssl_cert_requirements {
    PY_SSL_CERT_NONE,
    PY_SSL_CERT_OPTIONAL,
    PY_SSL_CERT_REQUIRED
};

enum py_ssl_version {
    PY_SSL_VERSION_SSL2,
    PY_SSL_VERSION_SSL3=1,
    PY_SSL_VERSION_SSL23,
#if HAVE_TLSv1_2
    PY_SSL_VERSION_TLS1,
    PY_SSL_VERSION_TLS1_1,
    PY_SSL_VERSION_TLS1_2
#else
    PY_SSL_VERSION_TLS1
#endif
};

#ifdef WITH_THREAD

/* serves as a flag to see whether we've initialized the SSL thread support. */
/* 0 means no, greater than 0 means yes */

static unsigned int _ssl_locks_count = 0;

#endif /* def WITH_THREAD */

/* SSL socket object */

#define X509_NAME_MAXLEN 256

/* RAND_* APIs got added to OpenSSL in 0.9.5 */
#if OPENSSL_VERSION_NUMBER >= 0x0090500fL
# define HAVE_OPENSSL_RAND 1
#else
# undef HAVE_OPENSSL_RAND
#endif

/* SSL_CTX_clear_options() and SSL_clear_options() were first added in
 * OpenSSL 0.9.8m but do not appear in some 0.9.9-dev versions such the
 * 0.9.9 from "May 2008" that NetBSD 5.0 uses. */
#if OPENSSL_VERSION_NUMBER >= 0x009080dfL && OPENSSL_VERSION_NUMBER != 0x00909000L
# define HAVE_SSL_CTX_CLEAR_OPTIONS
#else
# undef HAVE_SSL_CTX_CLEAR_OPTIONS
#endif

/* In case of 'tls-unique' it will be 12 bytes for TLS, 36 bytes for
 * older SSL, but let's be safe */
#define PySSL_CB_MAXLEN 128

/* SSL_get_finished got added to OpenSSL in 0.9.5 */
#if OPENSSL_VERSION_NUMBER >= 0x0090500fL
# define HAVE_OPENSSL_FINISHED 1
#else
# define HAVE_OPENSSL_FINISHED 0
#endif

/* ECDH support got added to OpenSSL in 0.9.8 */
#if OPENSSL_VERSION_NUMBER < 0x0090800fL && !defined(OPENSSL_NO_ECDH)
# define OPENSSL_NO_ECDH
#endif

/* compression support got added to OpenSSL in 0.9.8 */
#if OPENSSL_VERSION_NUMBER < 0x0090800fL && !defined(OPENSSL_NO_COMP)
# define OPENSSL_NO_COMP
#endif

/* X509_VERIFY_PARAM got added to OpenSSL in 0.9.8 */
#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
# define HAVE_OPENSSL_VERIFY_PARAM
#endif


typedef struct {
    PyObject_HEAD
    SSL_CTX *ctx;
#ifdef OPENSSL_NPN_NEGOTIATED
    unsigned char *npn_protocols;
    int npn_protocols_len;
#endif
#ifdef HAVE_ALPN
    unsigned char *alpn_protocols;
    int alpn_protocols_len;
#endif
#ifndef OPENSSL_NO_TLSEXT
    PyObject *set_hostname;
#endif
    int check_hostname;
} PySSLContext;

typedef struct {
    PyObject_HEAD
    PySocketSockObject *Socket;
    PyObject *ssl_sock;
    SSL *ssl;
    PySSLContext *ctx; /* weakref to SSL context */
    X509 *peer_cert;
    char shutdown_seen_zero;
    char handshake_done;
    enum py_ssl_server_or_client socket_type;
} PySSLSocket;

static PyTypeObject PySSLContext_Type;
static PyTypeObject PySSLSocket_Type;

static PyObject *PySSL_SSLwrite(PySSLSocket *self, PyObject *args);
static PyObject *PySSL_SSLread(PySSLSocket *self, PyObject *args);
static int check_socket_and_wait_for_timeout(PySocketSockObject *s,
                                             int writing);
static PyObject *PySSL_peercert(PySSLSocket *self, PyObject *args);
static PyObject *PySSL_cipher(PySSLSocket *self);

#define PySSLContext_Check(v)   (Py_TYPE(v) == &PySSLContext_Type)
#define PySSLSocket_Check(v)    (Py_TYPE(v) == &PySSLSocket_Type)

typedef enum {
    SOCKET_IS_NONBLOCKING,
    SOCKET_IS_BLOCKING,
    SOCKET_HAS_TIMED_OUT,
    SOCKET_HAS_BEEN_CLOSED,
    SOCKET_TOO_LARGE_FOR_SELECT,
    SOCKET_OPERATION_OK
} timeout_state;

/* Wrap error strings with filename and line # */
#define STRINGIFY1(x) #x
#define STRINGIFY2(x) STRINGIFY1(x)
#define ERRSTR1(x,y,z) (x ":" y ": " z)
#define ERRSTR(x) ERRSTR1("_ssl.c", STRINGIFY2(__LINE__), x)


/*
 * SSL errors.
 */

PyDoc_STRVAR(SSLError_doc,
"An error occurred in the SSL implementation.");

PyDoc_STRVAR(SSLZeroReturnError_doc,
"SSL/TLS session closed cleanly.");

PyDoc_STRVAR(SSLWantReadError_doc,
"Non-blocking SSL socket needs to read more data\n"
"before the requested operation can be completed.");

PyDoc_STRVAR(SSLWantWriteError_doc,
"Non-blocking SSL socket needs to write more data\n"
"before the requested operation can be completed.");

PyDoc_STRVAR(SSLSyscallError_doc,
"System error when attempting SSL operation.");

PyDoc_STRVAR(SSLEOFError_doc,
"SSL/TLS connection terminated abruptly.");


static PyObject *
SSLError_str(PyEnvironmentErrorObject *self)
{
    if (self->strerror != NULL) {
        Py_INCREF(self->strerror);
        return self->strerror;
    }
    else
        return PyObject_Str(self->args);
}

static void
fill_and_set_sslerror(PyObject *type, int ssl_errno, const char *errstr,
                      int lineno, unsigned long errcode)
{
    PyObject *err_value = NULL, *reason_obj = NULL, *lib_obj = NULL;
    PyObject *init_value, *msg, *key;

    if (errcode != 0) {
        int lib, reason;

        lib = ERR_GET_LIB(errcode);
        reason = ERR_GET_REASON(errcode);
        key = Py_BuildValue("ii", lib, reason);
        if (key == NULL)
            goto fail;
        reason_obj = PyDict_GetItem(err_codes_to_names, key);
        Py_DECREF(key);
        if (reason_obj == NULL) {
            /* XXX if reason < 100, it might reflect a library number (!!) */
            PyErr_Clear();
        }
        key = PyLong_FromLong(lib);
        if (key == NULL)
            goto fail;
        lib_obj = PyDict_GetItem(lib_codes_to_names, key);
        Py_DECREF(key);
        if (lib_obj == NULL) {
            PyErr_Clear();
        }
        if (errstr == NULL)
            errstr = ERR_reason_error_string(errcode);
    }
    if (errstr == NULL)
        errstr = "unknown error";

    if (reason_obj && lib_obj)
        msg = PyUnicode_FromFormat("[%S: %S] %s (_ssl.c:%d)",
                                   lib_obj, reason_obj, errstr, lineno);
    else if (lib_obj)
        msg = PyUnicode_FromFormat("[%S] %s (_ssl.c:%d)",
                                   lib_obj, errstr, lineno);
    else
        msg = PyUnicode_FromFormat("%s (_ssl.c:%d)", errstr, lineno);
    if (msg == NULL)
        goto fail;

    init_value = Py_BuildValue("iN", ssl_errno, msg);
    if (init_value == NULL)
        goto fail;

    err_value = PyObject_CallObject(type, init_value);
    Py_DECREF(init_value);
    if (err_value == NULL)
        goto fail;

    if (reason_obj == NULL)
        reason_obj = Py_None;
    if (PyObject_SetAttrString(err_value, "reason", reason_obj))
        goto fail;
    if (lib_obj == NULL)
        lib_obj = Py_None;
    if (PyObject_SetAttrString(err_value, "library", lib_obj))
        goto fail;
    PyErr_SetObject(type, err_value);
fail:
    Py_XDECREF(err_value);
}

static PyObject *
PySSL_SetError(PySSLSocket *obj, int ret, char *filename, int lineno)
{
    PyObject *type = PySSLErrorObject;
    char *errstr = NULL;
    int err;
    enum py_ssl_error p = PY_SSL_ERROR_NONE;
    unsigned long e = 0;

    assert(ret <= 0);
    e = ERR_peek_last_error();

    if (obj->ssl != NULL) {
        err = SSL_get_error(obj->ssl, ret);

        switch (err) {
        case SSL_ERROR_ZERO_RETURN:
            errstr = "TLS/SSL connection has been closed (EOF)";
            type = PySSLZeroReturnErrorObject;
            p = PY_SSL_ERROR_ZERO_RETURN;
            break;
        case SSL_ERROR_WANT_READ:
            errstr = "The operation did not complete (read)";
            type = PySSLWantReadErrorObject;
            p = PY_SSL_ERROR_WANT_READ;
            break;
        case SSL_ERROR_WANT_WRITE:
            p = PY_SSL_ERROR_WANT_WRITE;
            type = PySSLWantWriteErrorObject;
            errstr = "The operation did not complete (write)";
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            p = PY_SSL_ERROR_WANT_X509_LOOKUP;
            errstr = "The operation did not complete (X509 lookup)";
            break;
        case SSL_ERROR_WANT_CONNECT:
            p = PY_SSL_ERROR_WANT_CONNECT;
            errstr = "The operation did not complete (connect)";
            break;
        case SSL_ERROR_SYSCALL:
        {
            if (e == 0) {
                PySocketSockObject *s = obj->Socket;
                if (ret == 0) {
                    p = PY_SSL_ERROR_EOF;
                    type = PySSLEOFErrorObject;
                    errstr = "EOF occurred in violation of protocol";
                } else if (ret == -1) {
                    /* underlying BIO reported an I/O error */
                    Py_INCREF(s);
                    ERR_clear_error();
                    s->errorhandler();
                    Py_DECREF(s);
                    return NULL;
                } else { /* possible? */
                    p = PY_SSL_ERROR_SYSCALL;
                    type = PySSLSyscallErrorObject;
                    errstr = "Some I/O error occurred";
                }
            } else {
                p = PY_SSL_ERROR_SYSCALL;
            }
            break;
        }
        case SSL_ERROR_SSL:
        {
            p = PY_SSL_ERROR_SSL;
            if (e == 0)
                /* possible? */
                errstr = "A failure in the SSL library occurred";
            break;
        }
        default:
            p = PY_SSL_ERROR_INVALID_ERROR_CODE;
            errstr = "Invalid error code";
        }
    }
    fill_and_set_sslerror(type, p, errstr, lineno, e);
    ERR_clear_error();
    return NULL;
}

static PyObject *
_setSSLError (char *errstr, int errcode, char *filename, int lineno) {

    if (errstr == NULL)
        errcode = ERR_peek_last_error();
    else
        errcode = 0;
    fill_and_set_sslerror(PySSLErrorObject, errcode, errstr, lineno, errcode);
    ERR_clear_error();
    return NULL;
}

/*
 * SSL objects
 */

static PySSLSocket *
newPySSLSocket(PySSLContext *sslctx, PySocketSockObject *sock,
               enum py_ssl_server_or_client socket_type,
               char *server_hostname, PyObject *ssl_sock)
{
    PySSLSocket *self;
    SSL_CTX *ctx = sslctx->ctx;
    long mode;

    self = PyObject_New(PySSLSocket, &PySSLSocket_Type);
    if (self == NULL)
        return NULL;

    self->peer_cert = NULL;
    self->ssl = NULL;
    self->Socket = NULL;
    self->ssl_sock = NULL;
    self->ctx = sslctx;
    self->shutdown_seen_zero = 0;
    self->handshake_done = 0;
    Py_INCREF(sslctx);

    /* Make sure the SSL error state is initialized */
    (void) ERR_get_state();
    ERR_clear_error();

    PySSL_BEGIN_ALLOW_THREADS
    self->ssl = SSL_new(ctx);
    PySSL_END_ALLOW_THREADS
    SSL_set_app_data(self->ssl,self);
    SSL_set_fd(self->ssl, Py_SAFE_DOWNCAST(sock->sock_fd, SOCKET_T, int));
    mode = SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER;
#ifdef SSL_MODE_AUTO_RETRY
    mode |= SSL_MODE_AUTO_RETRY;
#endif
    SSL_set_mode(self->ssl, mode);

#if HAVE_SNI
    if (server_hostname != NULL)
        SSL_set_tlsext_host_name(self->ssl, server_hostname);
#endif

    /* If the socket is in non-blocking mode or timeout mode, set the BIO
     * to non-blocking mode (blocking is the default)
     */
    if (sock->sock_timeout >= 0.0) {
        BIO_set_nbio(SSL_get_rbio(self->ssl), 1);
        BIO_set_nbio(SSL_get_wbio(self->ssl), 1);
    }

    PySSL_BEGIN_ALLOW_THREADS
    if (socket_type == PY_SSL_CLIENT)
        SSL_set_connect_state(self->ssl);
    else
        SSL_set_accept_state(self->ssl);
    PySSL_END_ALLOW_THREADS

    self->socket_type = socket_type;
    self->Socket = sock;
    Py_INCREF(self->Socket);
    if (ssl_sock != Py_None) {
        self->ssl_sock = PyWeakref_NewRef(ssl_sock, NULL);
        if (self->ssl_sock == NULL) {
            Py_DECREF(self);
            return NULL;
        }
    }
    return self;
}


/* SSL object methods */

static PyObject *PySSL_SSLdo_handshake(PySSLSocket *self)
{
    int ret;
    int err;
    int sockstate, nonblocking;
    PySocketSockObject *sock = self->Socket;

    Py_INCREF(sock);

    /* just in case the blocking state of the socket has been changed */
    nonblocking = (sock->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);

    /* Actually negotiate SSL connection */
    /* XXX If SSL_do_handshake() returns 0, it's also a failure. */
    do {
        PySSL_BEGIN_ALLOW_THREADS
        ret = SSL_do_handshake(self->ssl);
        err = SSL_get_error(self->ssl, ret);
        PySSL_END_ALLOW_THREADS
        if (PyErr_CheckSignals())
            goto error;
        if (err == SSL_ERROR_WANT_READ) {
            sockstate = check_socket_and_wait_for_timeout(sock, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = check_socket_and_wait_for_timeout(sock, 1);
        } else {
            sockstate = SOCKET_OPERATION_OK;
        }
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PySSLErrorObject,
                            ERRSTR("The handshake operation timed out"));
            goto error;
        } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
            PyErr_SetString(PySSLErrorObject,
                            ERRSTR("Underlying socket has been closed."));
            goto error;
        } else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
            PyErr_SetString(PySSLErrorObject,
                            ERRSTR("Underlying socket too large for select()."));
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    Py_DECREF(sock);
    if (ret < 1)
        return PySSL_SetError(self, ret, __FILE__, __LINE__);

    if (self->peer_cert)
        X509_free (self->peer_cert);
    PySSL_BEGIN_ALLOW_THREADS
    self->peer_cert = SSL_get_peer_certificate(self->ssl);
    PySSL_END_ALLOW_THREADS
    self->handshake_done = 1;

    Py_INCREF(Py_None);
    return Py_None;

error:
    Py_DECREF(sock);
    return NULL;
}

static PyObject *
_create_tuple_for_attribute (ASN1_OBJECT *name, ASN1_STRING *value) {

    char namebuf[X509_NAME_MAXLEN];
    int buflen;
    PyObject *name_obj;
    PyObject *value_obj;
    PyObject *attr;
    unsigned char *valuebuf = NULL;

    buflen = OBJ_obj2txt(namebuf, sizeof(namebuf), name, 0);
    if (buflen < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto fail;
    }
    name_obj = PyString_FromStringAndSize(namebuf, buflen);
    if (name_obj == NULL)
        goto fail;

    buflen = ASN1_STRING_to_UTF8(&valuebuf, value);
    if (buflen < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        Py_DECREF(name_obj);
        goto fail;
    }
    value_obj = PyUnicode_DecodeUTF8((char *) valuebuf,
                                     buflen, "strict");
    OPENSSL_free(valuebuf);
    if (value_obj == NULL) {
        Py_DECREF(name_obj);
        goto fail;
    }
    attr = PyTuple_New(2);
    if (attr == NULL) {
        Py_DECREF(name_obj);
        Py_DECREF(value_obj);
        goto fail;
    }
    PyTuple_SET_ITEM(attr, 0, name_obj);
    PyTuple_SET_ITEM(attr, 1, value_obj);
    return attr;

  fail:
    return NULL;
}

static PyObject *
_create_tuple_for_X509_NAME (X509_NAME *xname)
{
    PyObject *dn = NULL;    /* tuple which represents the "distinguished name" */
    PyObject *rdn = NULL;   /* tuple to hold a "relative distinguished name" */
    PyObject *rdnt;
    PyObject *attr = NULL;   /* tuple to hold an attribute */
    int entry_count = X509_NAME_entry_count(xname);
    X509_NAME_ENTRY *entry;
    ASN1_OBJECT *name;
    ASN1_STRING *value;
    int index_counter;
    int rdn_level = -1;
    int retcode;

    dn = PyList_New(0);
    if (dn == NULL)
        return NULL;
    /* now create another tuple to hold the top-level RDN */
    rdn = PyList_New(0);
    if (rdn == NULL)
        goto fail0;

    for (index_counter = 0;
         index_counter < entry_count;
         index_counter++)
    {
        entry = X509_NAME_get_entry(xname, index_counter);

        /* check to see if we've gotten to a new RDN */
        if (rdn_level >= 0) {
            if (rdn_level != entry->set) {
                /* yes, new RDN */
                /* add old RDN to DN */
                rdnt = PyList_AsTuple(rdn);
                Py_DECREF(rdn);
                if (rdnt == NULL)
                    goto fail0;
                retcode = PyList_Append(dn, rdnt);
                Py_DECREF(rdnt);
                if (retcode < 0)
                    goto fail0;
                /* create new RDN */
                rdn = PyList_New(0);
                if (rdn == NULL)
                    goto fail0;
            }
        }
        rdn_level = entry->set;

        /* now add this attribute to the current RDN */
        name = X509_NAME_ENTRY_get_object(entry);
        value = X509_NAME_ENTRY_get_data(entry);
        attr = _create_tuple_for_attribute(name, value);
        /*
        fprintf(stderr, "RDN level %d, attribute %s: %s\n",
            entry->set,
            PyBytes_AS_STRING(PyTuple_GET_ITEM(attr, 0)),
            PyBytes_AS_STRING(PyTuple_GET_ITEM(attr, 1)));
        */
        if (attr == NULL)
            goto fail1;
        retcode = PyList_Append(rdn, attr);
        Py_DECREF(attr);
        if (retcode < 0)
            goto fail1;
    }
    /* now, there's typically a dangling RDN */
    if (rdn != NULL) {
        if (PyList_GET_SIZE(rdn) > 0) {
            rdnt = PyList_AsTuple(rdn);
            Py_DECREF(rdn);
            if (rdnt == NULL)
                goto fail0;
            retcode = PyList_Append(dn, rdnt);
            Py_DECREF(rdnt);
            if (retcode < 0)
                goto fail0;
        }
        else {
            Py_DECREF(rdn);
        }
    }

    /* convert list to tuple */
    rdnt = PyList_AsTuple(dn);
    Py_DECREF(dn);
    if (rdnt == NULL)
        return NULL;
    return rdnt;

  fail1:
    Py_XDECREF(rdn);

  fail0:
    Py_XDECREF(dn);
    return NULL;
}

static PyObject *
_get_peer_alt_names (X509 *certificate) {

    /* this code follows the procedure outlined in
       OpenSSL's crypto/x509v3/v3_prn.c:X509v3_EXT_print()
       function to extract the STACK_OF(GENERAL_NAME),
       then iterates through the stack to add the
       names. */

    int i, j;
    PyObject *peer_alt_names = Py_None;
    PyObject *v = NULL, *t;
    X509_EXTENSION *ext = NULL;
    GENERAL_NAMES *names = NULL;
    GENERAL_NAME *name;
    const X509V3_EXT_METHOD *method;
    BIO *biobuf = NULL;
    char buf[2048];
    char *vptr;
    int len;
    /* Issue #2973: ASN1_item_d2i() API changed in OpenSSL 0.9.6m */
#if OPENSSL_VERSION_NUMBER >= 0x009060dfL
    const unsigned char *p;
#else
    unsigned char *p;
#endif

    if (certificate == NULL)
        return peer_alt_names;

    /* get a memory buffer */
    biobuf = BIO_new(BIO_s_mem());

    i = -1;
    while ((i = X509_get_ext_by_NID(
                    certificate, NID_subject_alt_name, i)) >= 0) {

        if (peer_alt_names == Py_None) {
            peer_alt_names = PyList_New(0);
            if (peer_alt_names == NULL)
                goto fail;
        }

        /* now decode the altName */
        ext = X509_get_ext(certificate, i);
        if(!(method = X509V3_EXT_get(ext))) {
            PyErr_SetString
              (PySSLErrorObject,
               ERRSTR("No method for internalizing subjectAltName!"));
            goto fail;
        }

        p = ext->value->data;
        if (method->it)
            names = (GENERAL_NAMES*)
              (ASN1_item_d2i(NULL,
                             &p,
                             ext->value->length,
                             ASN1_ITEM_ptr(method->it)));
        else
            names = (GENERAL_NAMES*)
              (method->d2i(NULL,
                           &p,
                           ext->value->length));

        for(j = 0; j < sk_GENERAL_NAME_num(names); j++) {
            /* get a rendering of each name in the set of names */
            int gntype;
            ASN1_STRING *as = NULL;

            name = sk_GENERAL_NAME_value(names, j);
            gntype = name->type;
            switch (gntype) {
            case GEN_DIRNAME:
                /* we special-case DirName as a tuple of
                   tuples of attributes */

                t = PyTuple_New(2);
                if (t == NULL) {
                    goto fail;
                }

                v = PyString_FromString("DirName");
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);

                v = _create_tuple_for_X509_NAME (name->d.dirn);
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 1, v);
                break;

            case GEN_EMAIL:
            case GEN_DNS:
            case GEN_URI:
                /* GENERAL_NAME_print() doesn't handle NULL bytes in ASN1_string
                   correctly, CVE-2013-4238 */
                t = PyTuple_New(2);
                if (t == NULL)
                    goto fail;
                switch (gntype) {
                case GEN_EMAIL:
                    v = PyString_FromString("email");
                    as = name->d.rfc822Name;
                    break;
                case GEN_DNS:
                    v = PyString_FromString("DNS");
                    as = name->d.dNSName;
                    break;
                case GEN_URI:
                    v = PyString_FromString("URI");
                    as = name->d.uniformResourceIdentifier;
                    break;
                }
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);
                v = PyString_FromStringAndSize((char *)ASN1_STRING_data(as),
                                               ASN1_STRING_length(as));
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 1, v);
                break;

            default:
                /* for everything else, we use the OpenSSL print form */
                switch (gntype) {
                    /* check for new general name type */
                    case GEN_OTHERNAME:
                    case GEN_X400:
                    case GEN_EDIPARTY:
                    case GEN_IPADD:
                    case GEN_RID:
                        break;
                    default:
                        if (PyErr_Warn(PyExc_RuntimeWarning,
                                       "Unknown general name type") == -1) {
                            goto fail;
                        }
                        break;
                }
                (void) BIO_reset(biobuf);
                GENERAL_NAME_print(biobuf, name);
                len = BIO_gets(biobuf, buf, sizeof(buf)-1);
                if (len < 0) {
                    _setSSLError(NULL, 0, __FILE__, __LINE__);
                    goto fail;
                }
                vptr = strchr(buf, ':');
                if (vptr == NULL)
                    goto fail;
                t = PyTuple_New(2);
                if (t == NULL)
                    goto fail;
                v = PyString_FromStringAndSize(buf, (vptr - buf));
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);
                v = PyString_FromStringAndSize((vptr + 1), (len - (vptr - buf + 1)));
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 1, v);
                break;
            }

            /* and add that rendering to the list */

            if (PyList_Append(peer_alt_names, t) < 0) {
                Py_DECREF(t);
                goto fail;
            }
            Py_DECREF(t);
        }
        sk_GENERAL_NAME_pop_free(names, GENERAL_NAME_free);
    }
    BIO_free(biobuf);
    if (peer_alt_names != Py_None) {
        v = PyList_AsTuple(peer_alt_names);
        Py_DECREF(peer_alt_names);
        return v;
    } else {
        return peer_alt_names;
    }


  fail:
    if (biobuf != NULL)
        BIO_free(biobuf);

    if (peer_alt_names != Py_None) {
        Py_XDECREF(peer_alt_names);
    }

    return NULL;
}

static PyObject *
_get_aia_uri(X509 *certificate, int nid) {
    PyObject *lst = NULL, *ostr = NULL;
    int i, result;
    AUTHORITY_INFO_ACCESS *info;

    info = X509_get_ext_d2i(certificate, NID_info_access, NULL, NULL);
    if (info == NULL)
        return Py_None;
    if (sk_ACCESS_DESCRIPTION_num(info) == 0) {
        AUTHORITY_INFO_ACCESS_free(info);
        return Py_None;
    }

    if ((lst = PyList_New(0)) == NULL) {
        goto fail;
    }

    for (i = 0; i < sk_ACCESS_DESCRIPTION_num(info); i++) {
        ACCESS_DESCRIPTION *ad = sk_ACCESS_DESCRIPTION_value(info, i);
        ASN1_IA5STRING *uri;

        if ((OBJ_obj2nid(ad->method) != nid) ||
                (ad->location->type != GEN_URI)) {
            continue;
        }
        uri = ad->location->d.uniformResourceIdentifier;
        ostr = PyUnicode_FromStringAndSize((char *)uri->data,
                                           uri->length);
        if (ostr == NULL) {
            goto fail;
        }
        result = PyList_Append(lst, ostr);
        Py_DECREF(ostr);
        if (result < 0) {
            goto fail;
        }
    }
    AUTHORITY_INFO_ACCESS_free(info);

    /* convert to tuple or None */
    if (PyList_Size(lst) == 0) {
        Py_DECREF(lst);
        return Py_None;
    } else {
        PyObject *tup;
        tup = PyList_AsTuple(lst);
        Py_DECREF(lst);
        return tup;
    }

  fail:
    AUTHORITY_INFO_ACCESS_free(info);
    Py_XDECREF(lst);
    return NULL;
}

static PyObject *
_get_crl_dp(X509 *certificate) {
    STACK_OF(DIST_POINT) *dps;
    int i, j;
    PyObject *lst, *res = NULL;

#if OPENSSL_VERSION_NUMBER < 0x10001000L
    dps = X509_get_ext_d2i(certificate, NID_crl_distribution_points, NULL, NULL);
#else
    /* Calls x509v3_cache_extensions and sets up crldp */
    X509_check_ca(certificate);
    dps = certificate->crldp;
#endif

    if (dps == NULL)
        return Py_None;

    lst = PyList_New(0);
    if (lst == NULL)
        goto done;

    for (i=0; i < sk_DIST_POINT_num(dps); i++) {
        DIST_POINT *dp;
        STACK_OF(GENERAL_NAME) *gns;

        dp = sk_DIST_POINT_value(dps, i);
        gns = dp->distpoint->name.fullname;

        for (j=0; j < sk_GENERAL_NAME_num(gns); j++) {
            GENERAL_NAME *gn;
            ASN1_IA5STRING *uri;
            PyObject *ouri;
            int err;

            gn = sk_GENERAL_NAME_value(gns, j);
            if (gn->type != GEN_URI) {
                continue;
            }
            uri = gn->d.uniformResourceIdentifier;
            ouri = PyUnicode_FromStringAndSize((char *)uri->data,
                                               uri->length);
            if (ouri == NULL)
                goto done;

            err = PyList_Append(lst, ouri);
            Py_DECREF(ouri);
            if (err < 0)
                goto done;
        }
    }

    /* Convert to tuple. */
    res = (PyList_GET_SIZE(lst) > 0) ? PyList_AsTuple(lst) : Py_None;

  done:
    Py_XDECREF(lst);
#if OPENSSL_VERSION_NUMBER < 0x10001000L
    sk_DIST_POINT_free(dps);
#endif
    return res;
}

static PyObject *
_decode_certificate(X509 *certificate) {

    PyObject *retval = NULL;
    BIO *biobuf = NULL;
    PyObject *peer;
    PyObject *peer_alt_names = NULL;
    PyObject *issuer;
    PyObject *version;
    PyObject *sn_obj;
    PyObject *obj;
    ASN1_INTEGER *serialNumber;
    char buf[2048];
    int len, result;
    ASN1_TIME *notBefore, *notAfter;
    PyObject *pnotBefore, *pnotAfter;

    retval = PyDict_New();
    if (retval == NULL)
        return NULL;

    peer = _create_tuple_for_X509_NAME(
        X509_get_subject_name(certificate));
    if (peer == NULL)
        goto fail0;
    if (PyDict_SetItemString(retval, (const char *) "subject", peer) < 0) {
        Py_DECREF(peer);
        goto fail0;
    }
    Py_DECREF(peer);

    issuer = _create_tuple_for_X509_NAME(
        X509_get_issuer_name(certificate));
    if (issuer == NULL)
        goto fail0;
    if (PyDict_SetItemString(retval, (const char *)"issuer", issuer) < 0) {
        Py_DECREF(issuer);
        goto fail0;
    }
    Py_DECREF(issuer);

    version = PyLong_FromLong(X509_get_version(certificate) + 1);
    if (version == NULL)
        goto fail0;
    if (PyDict_SetItemString(retval, "version", version) < 0) {
        Py_DECREF(version);
        goto fail0;
    }
    Py_DECREF(version);

    /* get a memory buffer */
    biobuf = BIO_new(BIO_s_mem());

    (void) BIO_reset(biobuf);
    serialNumber = X509_get_serialNumber(certificate);
    /* should not exceed 20 octets, 160 bits, so buf is big enough */
    i2a_ASN1_INTEGER(biobuf, serialNumber);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto fail1;
    }
    sn_obj = PyUnicode_FromStringAndSize(buf, len);
    if (sn_obj == NULL)
        goto fail1;
    if (PyDict_SetItemString(retval, "serialNumber", sn_obj) < 0) {
        Py_DECREF(sn_obj);
        goto fail1;
    }
    Py_DECREF(sn_obj);

    (void) BIO_reset(biobuf);
    notBefore = X509_get_notBefore(certificate);
    ASN1_TIME_print(biobuf, notBefore);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto fail1;
    }
    pnotBefore = PyUnicode_FromStringAndSize(buf, len);
    if (pnotBefore == NULL)
        goto fail1;
    if (PyDict_SetItemString(retval, "notBefore", pnotBefore) < 0) {
        Py_DECREF(pnotBefore);
        goto fail1;
    }
    Py_DECREF(pnotBefore);

    (void) BIO_reset(biobuf);
    notAfter = X509_get_notAfter(certificate);
    ASN1_TIME_print(biobuf, notAfter);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto fail1;
    }
    pnotAfter = PyString_FromStringAndSize(buf, len);
    if (pnotAfter == NULL)
        goto fail1;
    if (PyDict_SetItemString(retval, "notAfter", pnotAfter) < 0) {
        Py_DECREF(pnotAfter);
        goto fail1;
    }
    Py_DECREF(pnotAfter);

    /* Now look for subjectAltName */

    peer_alt_names = _get_peer_alt_names(certificate);
    if (peer_alt_names == NULL)
        goto fail1;
    else if (peer_alt_names != Py_None) {
        if (PyDict_SetItemString(retval, "subjectAltName",
                                 peer_alt_names) < 0) {
            Py_DECREF(peer_alt_names);
            goto fail1;
        }
        Py_DECREF(peer_alt_names);
    }

    /* Authority Information Access: OCSP URIs */
    obj = _get_aia_uri(certificate, NID_ad_OCSP);
    if (obj == NULL) {
        goto fail1;
    } else if (obj != Py_None) {
        result = PyDict_SetItemString(retval, "OCSP", obj);
        Py_DECREF(obj);
        if (result < 0) {
            goto fail1;
        }
    }

    obj = _get_aia_uri(certificate, NID_ad_ca_issuers);
    if (obj == NULL) {
        goto fail1;
    } else if (obj != Py_None) {
        result = PyDict_SetItemString(retval, "caIssuers", obj);
        Py_DECREF(obj);
        if (result < 0) {
            goto fail1;
        }
    }

    /* CDP (CRL distribution points) */
    obj = _get_crl_dp(certificate);
    if (obj == NULL) {
        goto fail1;
    } else if (obj != Py_None) {
        result = PyDict_SetItemString(retval, "crlDistributionPoints", obj);
        Py_DECREF(obj);
        if (result < 0) {
            goto fail1;
        }
    }

    BIO_free(biobuf);
    return retval;

  fail1:
    if (biobuf != NULL)
        BIO_free(biobuf);
  fail0:
    Py_XDECREF(retval);
    return NULL;
}

static PyObject *
_certificate_to_der(X509 *certificate)
{
    unsigned char *bytes_buf = NULL;
    int len;
    PyObject *retval;

    bytes_buf = NULL;
    len = i2d_X509(certificate, &bytes_buf);
    if (len < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    /* this is actually an immutable bytes sequence */
    retval = PyBytes_FromStringAndSize((const char *) bytes_buf, len);
    OPENSSL_free(bytes_buf);
    return retval;
}

static PyObject *
PySSL_test_decode_certificate (PyObject *mod, PyObject *args) {

    PyObject *retval = NULL;
    char *filename = NULL;
    X509 *x=NULL;
    BIO *cert;

    if (!PyArg_ParseTuple(args, "s:test_decode_certificate", &filename))
        return NULL;

    if ((cert=BIO_new(BIO_s_file())) == NULL) {
        PyErr_SetString(PySSLErrorObject,
                        "Can't malloc memory to read file");
        goto fail0;
    }

    if (BIO_read_filename(cert,filename) <= 0) {
        PyErr_SetString(PySSLErrorObject,
                        "Can't open file");
        goto fail0;
    }

    x = PEM_read_bio_X509_AUX(cert,NULL, NULL, NULL);
    if (x == NULL) {
        PyErr_SetString(PySSLErrorObject,
                        "Error decoding PEM-encoded file");
        goto fail0;
    }

    retval = _decode_certificate(x);
    X509_free(x);

  fail0:

    if (cert != NULL) BIO_free(cert);
    return retval;
}


static PyObject *
PySSL_peercert(PySSLSocket *self, PyObject *args)
{
    int verification;
    PyObject *binary_mode = Py_None;
    int b;

    if (!PyArg_ParseTuple(args, "|O:peer_certificate", &binary_mode))
        return NULL;

    if (!self->handshake_done) {
        PyErr_SetString(PyExc_ValueError,
                        "handshake not done yet");
        return NULL;
    }
    if (!self->peer_cert)
        Py_RETURN_NONE;

    b = PyObject_IsTrue(binary_mode);
    if (b < 0)
        return NULL;
    if (b) {
        /* return cert in DER-encoded format */
        return _certificate_to_der(self->peer_cert);
    } else {
        verification = SSL_CTX_get_verify_mode(SSL_get_SSL_CTX(self->ssl));
        if ((verification & SSL_VERIFY_PEER) == 0)
            return PyDict_New();
        else
            return _decode_certificate(self->peer_cert);
    }
}

PyDoc_STRVAR(PySSL_peercert_doc,
"peer_certificate([der=False]) -> certificate\n\
\n\
Returns the certificate for the peer.  If no certificate was provided,\n\
returns None.  If a certificate was provided, but not validated, returns\n\
an empty dictionary.  Otherwise returns a dict containing information\n\
about the peer certificate.\n\
\n\
If the optional argument is True, returns a DER-encoded copy of the\n\
peer certificate, or None if no certificate was provided.  This will\n\
return the certificate even if it wasn't validated.");

static PyObject *PySSL_cipher (PySSLSocket *self) {

    PyObject *retval, *v;
    const SSL_CIPHER *current;
    char *cipher_name;
    char *cipher_protocol;

    if (self->ssl == NULL)
        Py_RETURN_NONE;
    current = SSL_get_current_cipher(self->ssl);
    if (current == NULL)
        Py_RETURN_NONE;

    retval = PyTuple_New(3);
    if (retval == NULL)
        return NULL;

    cipher_name = (char *) SSL_CIPHER_get_name(current);
    if (cipher_name == NULL) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(retval, 0, Py_None);
    } else {
        v = PyString_FromString(cipher_name);
        if (v == NULL)
            goto fail0;
        PyTuple_SET_ITEM(retval, 0, v);
    }
    cipher_protocol = (char *) SSL_CIPHER_get_version(current);
    if (cipher_protocol == NULL) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(retval, 1, Py_None);
    } else {
        v = PyString_FromString(cipher_protocol);
        if (v == NULL)
            goto fail0;
        PyTuple_SET_ITEM(retval, 1, v);
    }
    v = PyInt_FromLong(SSL_CIPHER_get_bits(current, NULL));
    if (v == NULL)
        goto fail0;
    PyTuple_SET_ITEM(retval, 2, v);
    return retval;

  fail0:
    Py_DECREF(retval);
    return NULL;
}

static PyObject *PySSL_version(PySSLSocket *self)
{
    const char *version;

    if (self->ssl == NULL)
        Py_RETURN_NONE;
    version = SSL_get_version(self->ssl);
    if (!strcmp(version, "unknown"))
        Py_RETURN_NONE;
    return PyUnicode_FromString(version);
}

#ifdef OPENSSL_NPN_NEGOTIATED
static PyObject *PySSL_selected_npn_protocol(PySSLSocket *self) {
    const unsigned char *out;
    unsigned int outlen;

    SSL_get0_next_proto_negotiated(self->ssl,
                                   &out, &outlen);

    if (out == NULL)
        Py_RETURN_NONE;
    return PyString_FromStringAndSize((char *)out, outlen);
}
#endif

#ifdef HAVE_ALPN
static PyObject *PySSL_selected_alpn_protocol(PySSLSocket *self) {
    const unsigned char *out;
    unsigned int outlen;

    SSL_get0_alpn_selected(self->ssl, &out, &outlen);

    if (out == NULL)
        Py_RETURN_NONE;
    return PyString_FromStringAndSize((char *)out, outlen);
}
#endif

static PyObject *PySSL_compression(PySSLSocket *self) {
#ifdef OPENSSL_NO_COMP
    Py_RETURN_NONE;
#else
    const COMP_METHOD *comp_method;
    const char *short_name;

    if (self->ssl == NULL)
        Py_RETURN_NONE;
    comp_method = SSL_get_current_compression(self->ssl);
    if (comp_method == NULL || comp_method->type == NID_undef)
        Py_RETURN_NONE;
    short_name = OBJ_nid2sn(comp_method->type);
    if (short_name == NULL)
        Py_RETURN_NONE;
    return PyBytes_FromString(short_name);
#endif
}

static PySSLContext *PySSL_get_context(PySSLSocket *self, void *closure) {
    Py_INCREF(self->ctx);
    return self->ctx;
}

static int PySSL_set_context(PySSLSocket *self, PyObject *value,
                                   void *closure) {

    if (PyObject_TypeCheck(value, &PySSLContext_Type)) {
#if !HAVE_SNI
        PyErr_SetString(PyExc_NotImplementedError, "setting a socket's "
                        "context is not supported by your OpenSSL library");
        return -1;
#else
        Py_INCREF(value);
        Py_SETREF(self->ctx, (PySSLContext *)value);
        SSL_set_SSL_CTX(self->ssl, self->ctx->ctx);
#endif
    } else {
        PyErr_SetString(PyExc_TypeError, "The value must be a SSLContext");
        return -1;
    }

    return 0;
}

PyDoc_STRVAR(PySSL_set_context_doc,
"_setter_context(ctx)\n\
\
This changes the context associated with the SSLSocket. This is typically\n\
used from within a callback function set by the set_servername_callback\n\
on the SSLContext to change the certificate information associated with the\n\
SSLSocket before the cryptographic exchange handshake messages\n");



static void PySSL_dealloc(PySSLSocket *self)
{
    if (self->peer_cert)        /* Possible not to have one? */
        X509_free (self->peer_cert);
    if (self->ssl)
        SSL_free(self->ssl);
    Py_XDECREF(self->Socket);
    Py_XDECREF(self->ssl_sock);
    Py_XDECREF(self->ctx);
    PyObject_Del(self);
}

/* If the socket has a timeout, do a select()/poll() on the socket.
   The argument writing indicates the direction.
   Returns one of the possibilities in the timeout_state enum (above).
 */

static int
check_socket_and_wait_for_timeout(PySocketSockObject *s, int writing)
{
    fd_set fds;
    struct timeval tv;
    int rc;

    /* Nothing to do unless we're in timeout mode (not non-blocking) */
    if (s->sock_timeout < 0.0)
        return SOCKET_IS_BLOCKING;
    else if (s->sock_timeout == 0.0)
        return SOCKET_IS_NONBLOCKING;

    /* Guard against closed socket */
    if (s->sock_fd < 0)
        return SOCKET_HAS_BEEN_CLOSED;

    /* Prefer poll, if available, since you can poll() any fd
     * which can't be done with select(). */
#ifdef HAVE_POLL
    {
        struct pollfd pollfd;
        int timeout;

        pollfd.fd = s->sock_fd;
        pollfd.events = writing ? POLLOUT : POLLIN;

        /* s->sock_timeout is in seconds, timeout in ms */
        timeout = (int)(s->sock_timeout * 1000 + 0.5);
        PySSL_BEGIN_ALLOW_THREADS
        rc = poll(&pollfd, 1, timeout);
        PySSL_END_ALLOW_THREADS

        goto normal_return;
    }
#endif

    /* Guard against socket too large for select*/
    if (!_PyIsSelectable_fd(s->sock_fd))
        return SOCKET_TOO_LARGE_FOR_SELECT;

    /* Construct the arguments to select */
    tv.tv_sec = (int)s->sock_timeout;
    tv.tv_usec = (int)((s->sock_timeout - tv.tv_sec) * 1e6);
    FD_ZERO(&fds);
    FD_SET(s->sock_fd, &fds);

    /* See if the socket is ready */
    PySSL_BEGIN_ALLOW_THREADS
    if (writing)
        rc = select(s->sock_fd+1, NULL, &fds, NULL, &tv);
    else
        rc = select(s->sock_fd+1, &fds, NULL, NULL, &tv);
    PySSL_END_ALLOW_THREADS

#ifdef HAVE_POLL
normal_return:
#endif
    /* Return SOCKET_TIMED_OUT on timeout, SOCKET_OPERATION_OK otherwise
       (when we are able to write or when there's something to read) */
    return rc == 0 ? SOCKET_HAS_TIMED_OUT : SOCKET_OPERATION_OK;
}

static PyObject *PySSL_SSLwrite(PySSLSocket *self, PyObject *args)
{
    Py_buffer buf;
    int len;
    int sockstate;
    int err;
    int nonblocking;
    PySocketSockObject *sock = self->Socket;

    Py_INCREF(sock);

    if (!PyArg_ParseTuple(args, "s*:write", &buf)) {
        Py_DECREF(sock);
        return NULL;
    }

    if (buf.len > INT_MAX) {
        PyErr_Format(PyExc_OverflowError,
                     "string longer than %d bytes", INT_MAX);
        goto error;
    }

    /* just in case the blocking state of the socket has been changed */
    nonblocking = (sock->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);

    sockstate = check_socket_and_wait_for_timeout(sock, 1);
    if (sockstate == SOCKET_HAS_TIMED_OUT) {
        PyErr_SetString(PySSLErrorObject,
                        "The write operation timed out");
        goto error;
    } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
        PyErr_SetString(PySSLErrorObject,
                        "Underlying socket has been closed.");
        goto error;
    } else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
        PyErr_SetString(PySSLErrorObject,
                        "Underlying socket too large for select().");
        goto error;
    }
    do {
        PySSL_BEGIN_ALLOW_THREADS
        len = SSL_write(self->ssl, buf.buf, (int)buf.len);
        err = SSL_get_error(self->ssl, len);
        PySSL_END_ALLOW_THREADS
        if (PyErr_CheckSignals()) {
            goto error;
        }
        if (err == SSL_ERROR_WANT_READ) {
            sockstate = check_socket_and_wait_for_timeout(sock, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = check_socket_and_wait_for_timeout(sock, 1);
        } else {
            sockstate = SOCKET_OPERATION_OK;
        }
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PySSLErrorObject,
                            "The write operation timed out");
            goto error;
        } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
            PyErr_SetString(PySSLErrorObject,
                            "Underlying socket has been closed.");
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);

    Py_DECREF(sock);
    PyBuffer_Release(&buf);
    if (len > 0)
        return PyInt_FromLong(len);
    else
        return PySSL_SetError(self, len, __FILE__, __LINE__);

error:
    Py_DECREF(sock);
    PyBuffer_Release(&buf);
    return NULL;
}

PyDoc_STRVAR(PySSL_SSLwrite_doc,
"write(s) -> len\n\
\n\
Writes the string s into the SSL object.  Returns the number\n\
of bytes written.");

static PyObject *PySSL_SSLpending(PySSLSocket *self)
{
    int count = 0;

    PySSL_BEGIN_ALLOW_THREADS
    count = SSL_pending(self->ssl);
    PySSL_END_ALLOW_THREADS
    if (count < 0)
        return PySSL_SetError(self, count, __FILE__, __LINE__);
    else
        return PyInt_FromLong(count);
}

PyDoc_STRVAR(PySSL_SSLpending_doc,
"pending() -> count\n\
\n\
Returns the number of already decrypted bytes available for read,\n\
pending on the connection.\n");

static PyObject *PySSL_SSLread(PySSLSocket *self, PyObject *args)
{
    PyObject *dest = NULL;
    Py_buffer buf;
    char *mem;
    int len, count;
    int buf_passed = 0;
    int sockstate;
    int err;
    int nonblocking;
    PySocketSockObject *sock = self->Socket;

    Py_INCREF(sock);

    buf.obj = NULL;
    buf.buf = NULL;
    if (!PyArg_ParseTuple(args, "i|w*:read", &len, &buf))
        goto error;

    if ((buf.buf == NULL) && (buf.obj == NULL)) {
        if (len < 0) {
            PyErr_SetString(PyExc_ValueError, "size should not be negative");
            goto error;
        }
        dest = PyBytes_FromStringAndSize(NULL, len);
        if (dest == NULL)
            goto error;
        mem = PyBytes_AS_STRING(dest);
    }
    else {
        buf_passed = 1;
        mem = buf.buf;
        if (len <= 0 || len > buf.len) {
            len = (int) buf.len;
            if (buf.len != len) {
                PyErr_SetString(PyExc_OverflowError,
                                "maximum length can't fit in a C 'int'");
                goto error;
            }
        }
    }

    /* just in case the blocking state of the socket has been changed */
    nonblocking = (sock->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);

    do {
        PySSL_BEGIN_ALLOW_THREADS
        count = SSL_read(self->ssl, mem, len);
        err = SSL_get_error(self->ssl, count);
        PySSL_END_ALLOW_THREADS
        if (PyErr_CheckSignals())
            goto error;
        if (err == SSL_ERROR_WANT_READ) {
            sockstate = check_socket_and_wait_for_timeout(sock, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = check_socket_and_wait_for_timeout(sock, 1);
        } else if ((err == SSL_ERROR_ZERO_RETURN) &&
                   (SSL_get_shutdown(self->ssl) ==
                    SSL_RECEIVED_SHUTDOWN))
        {
            count = 0;
            goto done;
        } else {
            sockstate = SOCKET_OPERATION_OK;
        }
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PySSLErrorObject,
                            "The read operation timed out");
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    if (count <= 0) {
        PySSL_SetError(self, count, __FILE__, __LINE__);
        goto error;
    }

done:
    Py_DECREF(sock);
    if (!buf_passed) {
        _PyBytes_Resize(&dest, count);
        return dest;
    }
    else {
        PyBuffer_Release(&buf);
        return PyLong_FromLong(count);
    }

error:
    Py_DECREF(sock);
    if (!buf_passed)
        Py_XDECREF(dest);
    else
        PyBuffer_Release(&buf);
    return NULL;
}

PyDoc_STRVAR(PySSL_SSLread_doc,
"read([len]) -> string\n\
\n\
Read up to len bytes from the SSL socket.");

static PyObject *PySSL_SSLshutdown(PySSLSocket *self)
{
    int err, ssl_err, sockstate, nonblocking;
    int zeros = 0;
    PySocketSockObject *sock = self->Socket;

    /* Guard against closed socket */
    if (sock->sock_fd < 0) {
        _setSSLError("Underlying socket connection gone",
                     PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
        return NULL;
    }
    Py_INCREF(sock);

    /* Just in case the blocking state of the socket has been changed */
    nonblocking = (sock->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);

    while (1) {
        PySSL_BEGIN_ALLOW_THREADS
        /* Disable read-ahead so that unwrap can work correctly.
         * Otherwise OpenSSL might read in too much data,
         * eating clear text data that happens to be
         * transmitted after the SSL shutdown.
         * Should be safe to call repeatedly every time this
         * function is used and the shutdown_seen_zero != 0
         * condition is met.
         */
        if (self->shutdown_seen_zero)
            SSL_set_read_ahead(self->ssl, 0);
        err = SSL_shutdown(self->ssl);
        PySSL_END_ALLOW_THREADS
        /* If err == 1, a secure shutdown with SSL_shutdown() is complete */
        if (err > 0)
            break;
        if (err == 0) {
            /* Don't loop endlessly; instead preserve legacy
               behaviour of trying SSL_shutdown() only twice.
               This looks necessary for OpenSSL < 0.9.8m */
            if (++zeros > 1)
                break;
            /* Shutdown was sent, now try receiving */
            self->shutdown_seen_zero = 1;
            continue;
        }

        /* Possibly retry shutdown until timeout or failure */
        ssl_err = SSL_get_error(self->ssl, err);
        if (ssl_err == SSL_ERROR_WANT_READ)
            sockstate = check_socket_and_wait_for_timeout(sock, 0);
        else if (ssl_err == SSL_ERROR_WANT_WRITE)
            sockstate = check_socket_and_wait_for_timeout(sock, 1);
        else
            break;
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            if (ssl_err == SSL_ERROR_WANT_READ)
                PyErr_SetString(PySSLErrorObject,
                                "The read operation timed out");
            else
                PyErr_SetString(PySSLErrorObject,
                                "The write operation timed out");
            goto error;
        }
        else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
            PyErr_SetString(PySSLErrorObject,
                            "Underlying socket too large for select().");
            goto error;
        }
        else if (sockstate != SOCKET_OPERATION_OK)
            /* Retain the SSL error code */
            break;
    }

    if (err < 0) {
        Py_DECREF(sock);
        return PySSL_SetError(self, err, __FILE__, __LINE__);
    }
    else
        /* It's already INCREF'ed */
        return (PyObject *) sock;

error:
    Py_DECREF(sock);
    return NULL;
}

PyDoc_STRVAR(PySSL_SSLshutdown_doc,
"shutdown(s) -> socket\n\
\n\
Does the SSL shutdown handshake with the remote end, and returns\n\
the underlying socket object.");

#if HAVE_OPENSSL_FINISHED
static PyObject *
PySSL_tls_unique_cb(PySSLSocket *self)
{
    PyObject *retval = NULL;
    char buf[PySSL_CB_MAXLEN];
    size_t len;

    if (SSL_session_reused(self->ssl) ^ !self->socket_type) {
        /* if session is resumed XOR we are the client */
        len = SSL_get_finished(self->ssl, buf, PySSL_CB_MAXLEN);
    }
    else {
        /* if a new session XOR we are the server */
        len = SSL_get_peer_finished(self->ssl, buf, PySSL_CB_MAXLEN);
    }

    /* It cannot be negative in current OpenSSL version as of July 2011 */
    if (len == 0)
        Py_RETURN_NONE;

    retval = PyBytes_FromStringAndSize(buf, len);

    return retval;
}

PyDoc_STRVAR(PySSL_tls_unique_cb_doc,
"tls_unique_cb() -> bytes\n\
\n\
Returns the 'tls-unique' channel binding data, as defined by RFC 5929.\n\
\n\
If the TLS handshake is not yet complete, None is returned");

#endif /* HAVE_OPENSSL_FINISHED */

static PyGetSetDef ssl_getsetlist[] = {
    {"context", (getter) PySSL_get_context,
                (setter) PySSL_set_context, PySSL_set_context_doc},
    {NULL},            /* sentinel */
};

static PyMethodDef PySSLMethods[] = {
    {"do_handshake", (PyCFunction)PySSL_SSLdo_handshake, METH_NOARGS},
    {"write", (PyCFunction)PySSL_SSLwrite, METH_VARARGS,
     PySSL_SSLwrite_doc},
    {"read", (PyCFunction)PySSL_SSLread, METH_VARARGS,
     PySSL_SSLread_doc},
    {"pending", (PyCFunction)PySSL_SSLpending, METH_NOARGS,
     PySSL_SSLpending_doc},
    {"peer_certificate", (PyCFunction)PySSL_peercert, METH_VARARGS,
     PySSL_peercert_doc},
    {"cipher", (PyCFunction)PySSL_cipher, METH_NOARGS},
    {"version", (PyCFunction)PySSL_version, METH_NOARGS},
#ifdef OPENSSL_NPN_NEGOTIATED
    {"selected_npn_protocol", (PyCFunction)PySSL_selected_npn_protocol, METH_NOARGS},
#endif
#ifdef HAVE_ALPN
    {"selected_alpn_protocol", (PyCFunction)PySSL_selected_alpn_protocol, METH_NOARGS},
#endif
    {"compression", (PyCFunction)PySSL_compression, METH_NOARGS},
    {"shutdown", (PyCFunction)PySSL_SSLshutdown, METH_NOARGS,
     PySSL_SSLshutdown_doc},
#if HAVE_OPENSSL_FINISHED
    {"tls_unique_cb", (PyCFunction)PySSL_tls_unique_cb, METH_NOARGS,
     PySSL_tls_unique_cb_doc},
#endif
    {NULL, NULL}
};

static PyTypeObject PySSLSocket_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ssl._SSLSocket",                  /*tp_name*/
    sizeof(PySSLSocket),                /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    /* methods */
    (destructor)PySSL_dealloc,          /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_reserved*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                 /*tp_flags*/
    0,                                  /*tp_doc*/
    0,                                  /*tp_traverse*/
    0,                                  /*tp_clear*/
    0,                                  /*tp_richcompare*/
    0,                                  /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    0,                                  /*tp_iternext*/
    PySSLMethods,                       /*tp_methods*/
    0,                                  /*tp_members*/
    ssl_getsetlist,                     /*tp_getset*/
};


/*
 * _SSLContext objects
 */

static PyObject *
context_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"protocol", NULL};
    PySSLContext *self;
    int proto_version = PY_SSL_VERSION_SSL23;
    long options;
    SSL_CTX *ctx = NULL;

    if (!PyArg_ParseTupleAndKeywords(
        args, kwds, "i:_SSLContext", kwlist,
        &proto_version))
        return NULL;

    PySSL_BEGIN_ALLOW_THREADS
    if (proto_version == PY_SSL_VERSION_TLS1)
        ctx = SSL_CTX_new(TLSv1_method());
#if HAVE_TLSv1_2
    else if (proto_version == PY_SSL_VERSION_TLS1_1)
        ctx = SSL_CTX_new(TLSv1_1_method());
    else if (proto_version == PY_SSL_VERSION_TLS1_2)
        ctx = SSL_CTX_new(TLSv1_2_method());
#endif
#ifndef OPENSSL_NO_SSL3
    else if (proto_version == PY_SSL_VERSION_SSL3)
        ctx = SSL_CTX_new(SSLv3_method());
#endif
#ifndef OPENSSL_NO_SSL2
    else if (proto_version == PY_SSL_VERSION_SSL2)
        ctx = SSL_CTX_new(SSLv2_method());
#endif
    else if (proto_version == PY_SSL_VERSION_SSL23)
        ctx = SSL_CTX_new(SSLv23_method());
    else
        proto_version = -1;
    PySSL_END_ALLOW_THREADS

    if (proto_version == -1) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid protocol version");
        return NULL;
    }
    if (ctx == NULL) {
        PyErr_SetString(PySSLErrorObject,
                        "failed to allocate SSL context");
        return NULL;
    }

    assert(type != NULL && type->tp_alloc != NULL);
    self = (PySSLContext *) type->tp_alloc(type, 0);
    if (self == NULL) {
        SSL_CTX_free(ctx);
        return NULL;
    }
    self->ctx = ctx;
#ifdef OPENSSL_NPN_NEGOTIATED
    self->npn_protocols = NULL;
#endif
#ifdef HAVE_ALPN
    self->alpn_protocols = NULL;
#endif
#ifndef OPENSSL_NO_TLSEXT
    self->set_hostname = NULL;
#endif
    /* Don't check host name by default */
    self->check_hostname = 0;
    /* Defaults */
    SSL_CTX_set_verify(self->ctx, SSL_VERIFY_NONE, NULL);
    options = SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    if (proto_version != PY_SSL_VERSION_SSL2)
        options |= SSL_OP_NO_SSLv2;
    if (proto_version != PY_SSL_VERSION_SSL3)
        options |= SSL_OP_NO_SSLv3;
    SSL_CTX_set_options(self->ctx, options);

#ifndef OPENSSL_NO_ECDH
    /* Allow automatic ECDH curve selection (on OpenSSL 1.0.2+), or use
       prime256v1 by default.  This is Apache mod_ssl's initialization
       policy, so we should be safe. */
#if defined(SSL_CTX_set_ecdh_auto)
    SSL_CTX_set_ecdh_auto(self->ctx, 1);
#else
    {
        EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        SSL_CTX_set_tmp_ecdh(self->ctx, key);
        EC_KEY_free(key);
    }
#endif
#endif

#define SID_CTX "Python"
    SSL_CTX_set_session_id_context(self->ctx, (const unsigned char *) SID_CTX,
                                   sizeof(SID_CTX));
#undef SID_CTX

#ifdef X509_V_FLAG_TRUSTED_FIRST
    {
        /* Improve trust chain building when cross-signed intermediate
           certificates are present. See https://bugs.python.org/issue23476. */
        X509_STORE *store = SSL_CTX_get_cert_store(self->ctx);
        X509_STORE_set_flags(store, X509_V_FLAG_TRUSTED_FIRST);
    }
#endif

    return (PyObject *)self;
}

static int
context_traverse(PySSLContext *self, visitproc visit, void *arg)
{
#ifndef OPENSSL_NO_TLSEXT
    Py_VISIT(self->set_hostname);
#endif
    return 0;
}

static int
context_clear(PySSLContext *self)
{
#ifndef OPENSSL_NO_TLSEXT
    Py_CLEAR(self->set_hostname);
#endif
    return 0;
}

static void
context_dealloc(PySSLContext *self)
{
    context_clear(self);
    SSL_CTX_free(self->ctx);
#ifdef OPENSSL_NPN_NEGOTIATED
    PyMem_FREE(self->npn_protocols);
#endif
#ifdef HAVE_ALPN
    PyMem_FREE(self->alpn_protocols);
#endif
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
set_ciphers(PySSLContext *self, PyObject *args)
{
    int ret;
    const char *cipherlist;

    if (!PyArg_ParseTuple(args, "s:set_ciphers", &cipherlist))
        return NULL;
    ret = SSL_CTX_set_cipher_list(self->ctx, cipherlist);
    if (ret == 0) {
        /* Clearing the error queue is necessary on some OpenSSL versions,
           otherwise the error will be reported again when another SSL call
           is done. */
        ERR_clear_error();
        PyErr_SetString(PySSLErrorObject,
                        "No cipher can be selected.");
        return NULL;
    }
    Py_RETURN_NONE;
}

#ifdef OPENSSL_NPN_NEGOTIATED
static int
do_protocol_selection(int alpn, unsigned char **out, unsigned char *outlen,
                      const unsigned char *server_protocols, unsigned int server_protocols_len,
                      const unsigned char *client_protocols, unsigned int client_protocols_len)
{
    int ret;
    if (client_protocols == NULL) {
        client_protocols = (unsigned char *)"";
        client_protocols_len = 0;
    }
    if (server_protocols == NULL) {
        server_protocols = (unsigned char *)"";
        server_protocols_len = 0;
    }

    ret = SSL_select_next_proto(out, outlen,
                                server_protocols, server_protocols_len,
                                client_protocols, client_protocols_len);
    if (alpn && ret != OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_NOACK;

    return SSL_TLSEXT_ERR_OK;
}

/* this callback gets passed to SSL_CTX_set_next_protos_advertise_cb */
static int
_advertiseNPN_cb(SSL *s,
                 const unsigned char **data, unsigned int *len,
                 void *args)
{
    PySSLContext *ssl_ctx = (PySSLContext *) args;

    if (ssl_ctx->npn_protocols == NULL) {
        *data = (unsigned char *)"";
        *len = 0;
    } else {
        *data = ssl_ctx->npn_protocols;
        *len = ssl_ctx->npn_protocols_len;
    }

    return SSL_TLSEXT_ERR_OK;
}
/* this callback gets passed to SSL_CTX_set_next_proto_select_cb */
static int
_selectNPN_cb(SSL *s,
              unsigned char **out, unsigned char *outlen,
              const unsigned char *server, unsigned int server_len,
              void *args)
{
    PySSLContext *ctx = (PySSLContext *)args;
    return do_protocol_selection(0, out, outlen, server, server_len,
                                 ctx->npn_protocols, ctx->npn_protocols_len);
}
#endif

static PyObject *
_set_npn_protocols(PySSLContext *self, PyObject *args)
{
#ifdef OPENSSL_NPN_NEGOTIATED
    Py_buffer protos;

    if (!PyArg_ParseTuple(args, "s*:set_npn_protocols", &protos))
        return NULL;

    if (self->npn_protocols != NULL) {
        PyMem_Free(self->npn_protocols);
    }

    self->npn_protocols = PyMem_Malloc(protos.len);
    if (self->npn_protocols == NULL) {
        PyBuffer_Release(&protos);
        return PyErr_NoMemory();
    }
    memcpy(self->npn_protocols, protos.buf, protos.len);
    self->npn_protocols_len = (int) protos.len;

    /* set both server and client callbacks, because the context can
     * be used to create both types of sockets */
    SSL_CTX_set_next_protos_advertised_cb(self->ctx,
                                          _advertiseNPN_cb,
                                          self);
    SSL_CTX_set_next_proto_select_cb(self->ctx,
                                     _selectNPN_cb,
                                     self);

    PyBuffer_Release(&protos);
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "The NPN extension requires OpenSSL 1.0.1 or later.");
    return NULL;
#endif
}

#ifdef HAVE_ALPN
static int
_selectALPN_cb(SSL *s,
              const unsigned char **out, unsigned char *outlen,
              const unsigned char *client_protocols, unsigned int client_protocols_len,
              void *args)
{
    PySSLContext *ctx = (PySSLContext *)args;
    return do_protocol_selection(1, (unsigned char **)out, outlen,
                                 ctx->alpn_protocols, ctx->alpn_protocols_len,
                                 client_protocols, client_protocols_len);
}
#endif

static PyObject *
_set_alpn_protocols(PySSLContext *self, PyObject *args)
{
#ifdef HAVE_ALPN
    Py_buffer protos;

    if (!PyArg_ParseTuple(args, "s*:set_npn_protocols", &protos))
        return NULL;

    PyMem_FREE(self->alpn_protocols);
    self->alpn_protocols = PyMem_Malloc(protos.len);
    if (!self->alpn_protocols)
        return PyErr_NoMemory();
    memcpy(self->alpn_protocols, protos.buf, protos.len);
    self->alpn_protocols_len = protos.len;
    PyBuffer_Release(&protos);

    if (SSL_CTX_set_alpn_protos(self->ctx, self->alpn_protocols, self->alpn_protocols_len))
        return PyErr_NoMemory();
    SSL_CTX_set_alpn_select_cb(self->ctx, _selectALPN_cb, self);

    PyBuffer_Release(&protos);
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "The ALPN extension requires OpenSSL 1.0.2 or later.");
    return NULL;
#endif
}

static PyObject *
get_verify_mode(PySSLContext *self, void *c)
{
    switch (SSL_CTX_get_verify_mode(self->ctx)) {
    case SSL_VERIFY_NONE:
        return PyLong_FromLong(PY_SSL_CERT_NONE);
    case SSL_VERIFY_PEER:
        return PyLong_FromLong(PY_SSL_CERT_OPTIONAL);
    case SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT:
        return PyLong_FromLong(PY_SSL_CERT_REQUIRED);
    }
    PyErr_SetString(PySSLErrorObject,
                    "invalid return value from SSL_CTX_get_verify_mode");
    return NULL;
}

static int
set_verify_mode(PySSLContext *self, PyObject *arg, void *c)
{
    int n, mode;
    if (!PyArg_Parse(arg, "i", &n))
        return -1;
    if (n == PY_SSL_CERT_NONE)
        mode = SSL_VERIFY_NONE;
    else if (n == PY_SSL_CERT_OPTIONAL)
        mode = SSL_VERIFY_PEER;
    else if (n == PY_SSL_CERT_REQUIRED)
        mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    else {
        PyErr_SetString(PyExc_ValueError,
                        "invalid value for verify_mode");
        return -1;
    }
    if (mode == SSL_VERIFY_NONE && self->check_hostname) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot set verify_mode to CERT_NONE when "
                        "check_hostname is enabled.");
        return -1;
    }
    SSL_CTX_set_verify(self->ctx, mode, NULL);
    return 0;
}

#ifdef HAVE_OPENSSL_VERIFY_PARAM
static PyObject *
get_verify_flags(PySSLContext *self, void *c)
{
    X509_STORE *store;
    unsigned long flags;

    store = SSL_CTX_get_cert_store(self->ctx);
    flags = X509_VERIFY_PARAM_get_flags(store->param);
    return PyLong_FromUnsignedLong(flags);
}

static int
set_verify_flags(PySSLContext *self, PyObject *arg, void *c)
{
    X509_STORE *store;
    unsigned long new_flags, flags, set, clear;

    if (!PyArg_Parse(arg, "k", &new_flags))
        return -1;
    store = SSL_CTX_get_cert_store(self->ctx);
    flags = X509_VERIFY_PARAM_get_flags(store->param);
    clear = flags & ~new_flags;
    set = ~flags & new_flags;
    if (clear) {
        if (!X509_VERIFY_PARAM_clear_flags(store->param, clear)) {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
            return -1;
        }
    }
    if (set) {
        if (!X509_VERIFY_PARAM_set_flags(store->param, set)) {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
            return -1;
        }
    }
    return 0;
}
#endif

static PyObject *
get_options(PySSLContext *self, void *c)
{
    return PyLong_FromLong(SSL_CTX_get_options(self->ctx));
}

static int
set_options(PySSLContext *self, PyObject *arg, void *c)
{
    long new_opts, opts, set, clear;
    if (!PyArg_Parse(arg, "l", &new_opts))
        return -1;
    opts = SSL_CTX_get_options(self->ctx);
    clear = opts & ~new_opts;
    set = ~opts & new_opts;
    if (clear) {
#ifdef HAVE_SSL_CTX_CLEAR_OPTIONS
        SSL_CTX_clear_options(self->ctx, clear);
#else
        PyErr_SetString(PyExc_ValueError,
                        "can't clear options before OpenSSL 0.9.8m");
        return -1;
#endif
    }
    if (set)
        SSL_CTX_set_options(self->ctx, set);
    return 0;
}

static PyObject *
get_check_hostname(PySSLContext *self, void *c)
{
    return PyBool_FromLong(self->check_hostname);
}

static int
set_check_hostname(PySSLContext *self, PyObject *arg, void *c)
{
    PyObject *py_check_hostname;
    int check_hostname;
    if (!PyArg_Parse(arg, "O", &py_check_hostname))
        return -1;

    check_hostname = PyObject_IsTrue(py_check_hostname);
    if (check_hostname < 0)
        return -1;
    if (check_hostname &&
            SSL_CTX_get_verify_mode(self->ctx) == SSL_VERIFY_NONE) {
        PyErr_SetString(PyExc_ValueError,
                        "check_hostname needs a SSL context with either "
                        "CERT_OPTIONAL or CERT_REQUIRED");
        return -1;
    }
    self->check_hostname = check_hostname;
    return 0;
}


typedef struct {
    PyThreadState *thread_state;
    PyObject *callable;
    char *password;
    int size;
    int error;
} _PySSLPasswordInfo;

static int
_pwinfo_set(_PySSLPasswordInfo *pw_info, PyObject* password,
            const char *bad_type_error)
{
    /* Set the password and size fields of a _PySSLPasswordInfo struct
       from a unicode, bytes, or byte array object.
       The password field will be dynamically allocated and must be freed
       by the caller */
    PyObject *password_bytes = NULL;
    const char *data = NULL;
    Py_ssize_t size;

    if (PyUnicode_Check(password)) {
        password_bytes = PyUnicode_AsEncodedString(password, NULL, NULL);
        if (!password_bytes) {
            goto error;
        }
        data = PyBytes_AS_STRING(password_bytes);
        size = PyBytes_GET_SIZE(password_bytes);
    } else if (PyBytes_Check(password)) {
        data = PyBytes_AS_STRING(password);
        size = PyBytes_GET_SIZE(password);
    } else if (PyByteArray_Check(password)) {
        data = PyByteArray_AS_STRING(password);
        size = PyByteArray_GET_SIZE(password);
    } else {
        PyErr_SetString(PyExc_TypeError, bad_type_error);
        goto error;
    }

    if (size > (Py_ssize_t)INT_MAX) {
        PyErr_Format(PyExc_ValueError,
                     "password cannot be longer than %d bytes", INT_MAX);
        goto error;
    }

    PyMem_Free(pw_info->password);
    pw_info->password = PyMem_Malloc(size);
    if (!pw_info->password) {
        PyErr_SetString(PyExc_MemoryError,
                        "unable to allocate password buffer");
        goto error;
    }
    memcpy(pw_info->password, data, size);
    pw_info->size = (int)size;

    Py_XDECREF(password_bytes);
    return 1;

error:
    Py_XDECREF(password_bytes);
    return 0;
}

static int
_password_callback(char *buf, int size, int rwflag, void *userdata)
{
    _PySSLPasswordInfo *pw_info = (_PySSLPasswordInfo*) userdata;
    PyObject *fn_ret = NULL;

    PySSL_END_ALLOW_THREADS_S(pw_info->thread_state);

    if (pw_info->callable) {
        fn_ret = PyObject_CallFunctionObjArgs(pw_info->callable, NULL);
        if (!fn_ret) {
            /* TODO: It would be nice to move _ctypes_add_traceback() into the
               core python API, so we could use it to add a frame here */
            goto error;
        }

        if (!_pwinfo_set(pw_info, fn_ret,
                         "password callback must return a string")) {
            goto error;
        }
        Py_CLEAR(fn_ret);
    }

    if (pw_info->size > size) {
        PyErr_Format(PyExc_ValueError,
                     "password cannot be longer than %d bytes", size);
        goto error;
    }

    PySSL_BEGIN_ALLOW_THREADS_S(pw_info->thread_state);
    memcpy(buf, pw_info->password, pw_info->size);
    return pw_info->size;

error:
    Py_XDECREF(fn_ret);
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info->thread_state);
    pw_info->error = 1;
    return -1;
}

static PyObject *
load_cert_chain(PySSLContext *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"certfile", "keyfile", "password", NULL};
    PyObject *keyfile = NULL, *keyfile_bytes = NULL, *password = NULL;
    char *certfile_bytes = NULL;
    pem_password_cb *orig_passwd_cb = self->ctx->default_passwd_callback;
    void *orig_passwd_userdata = self->ctx->default_passwd_callback_userdata;
    _PySSLPasswordInfo pw_info = { NULL, NULL, NULL, 0, 0 };
    int r;

    errno = 0;
    ERR_clear_error();
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
            "et|OO:load_cert_chain", kwlist,
            Py_FileSystemDefaultEncoding, &certfile_bytes,
            &keyfile, &password))
        return NULL;

    if (keyfile && keyfile != Py_None) {
        if (PyString_Check(keyfile)) {
            Py_INCREF(keyfile);
            keyfile_bytes = keyfile;
        } else {
            PyObject *u = PyUnicode_FromObject(keyfile);
            if (!u)
                goto error;
            keyfile_bytes = PyUnicode_AsEncodedString(
                u, Py_FileSystemDefaultEncoding, NULL);
            Py_DECREF(u);
            if (!keyfile_bytes)
                goto error;
        }
    }

    if (password && password != Py_None) {
        if (PyCallable_Check(password)) {
            pw_info.callable = password;
        } else if (!_pwinfo_set(&pw_info, password,
                                "password should be a string or callable")) {
            goto error;
        }
        SSL_CTX_set_default_passwd_cb(self->ctx, _password_callback);
        SSL_CTX_set_default_passwd_cb_userdata(self->ctx, &pw_info);
    }
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info.thread_state);
    r = SSL_CTX_use_certificate_chain_file(self->ctx, certfile_bytes);
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    if (r != 1) {
        if (pw_info.error) {
            ERR_clear_error();
            /* the password callback has already set the error information */
        }
        else if (errno != 0) {
            ERR_clear_error();
            PyErr_SetFromErrno(PyExc_IOError);
        }
        else {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
        }
        goto error;
    }
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info.thread_state);
    r = SSL_CTX_use_PrivateKey_file(self->ctx,
        keyfile_bytes ? PyBytes_AS_STRING(keyfile_bytes) : certfile_bytes,
        SSL_FILETYPE_PEM);
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    if (r != 1) {
        if (pw_info.error) {
            ERR_clear_error();
            /* the password callback has already set the error information */
        }
        else if (errno != 0) {
            ERR_clear_error();
            PyErr_SetFromErrno(PyExc_IOError);
        }
        else {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
        }
        goto error;
    }
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info.thread_state);
    r = SSL_CTX_check_private_key(self->ctx);
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    if (r != 1) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto error;
    }
    SSL_CTX_set_default_passwd_cb(self->ctx, orig_passwd_cb);
    SSL_CTX_set_default_passwd_cb_userdata(self->ctx, orig_passwd_userdata);
    PyMem_Free(pw_info.password);
    Py_RETURN_NONE;

error:
    SSL_CTX_set_default_passwd_cb(self->ctx, orig_passwd_cb);
    SSL_CTX_set_default_passwd_cb_userdata(self->ctx, orig_passwd_userdata);
    Py_XDECREF(keyfile_bytes);
    PyMem_Free(pw_info.password);
    PyMem_Free(certfile_bytes);
    return NULL;
}

/* internal helper function, returns -1 on error
 */
static int
_add_ca_certs(PySSLContext *self, void *data, Py_ssize_t len,
              int filetype)
{
    BIO *biobuf = NULL;
    X509_STORE *store;
    int retval = 0, err, loaded = 0;

    assert(filetype == SSL_FILETYPE_ASN1 || filetype == SSL_FILETYPE_PEM);

    if (len <= 0) {
        PyErr_SetString(PyExc_ValueError,
                        "Empty certificate data");
        return -1;
    } else if (len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Certificate data is too long.");
        return -1;
    }

    biobuf = BIO_new_mem_buf(data, (int)len);
    if (biobuf == NULL) {
        _setSSLError("Can't allocate buffer", 0, __FILE__, __LINE__);
        return -1;
    }

    store = SSL_CTX_get_cert_store(self->ctx);
    assert(store != NULL);

    while (1) {
        X509 *cert = NULL;
        int r;

        if (filetype == SSL_FILETYPE_ASN1) {
            cert = d2i_X509_bio(biobuf, NULL);
        } else {
            cert = PEM_read_bio_X509(biobuf, NULL,
                                     self->ctx->default_passwd_callback,
                                     self->ctx->default_passwd_callback_userdata);
        }
        if (cert == NULL) {
            break;
        }
        r = X509_STORE_add_cert(store, cert);
        X509_free(cert);
        if (!r) {
            err = ERR_peek_last_error();
            if ((ERR_GET_LIB(err) == ERR_LIB_X509) &&
                (ERR_GET_REASON(err) == X509_R_CERT_ALREADY_IN_HASH_TABLE)) {
                /* cert already in hash table, not an error */
                ERR_clear_error();
            } else {
                break;
            }
        }
        loaded++;
    }

    err = ERR_peek_last_error();
    if ((filetype == SSL_FILETYPE_ASN1) &&
            (loaded > 0) &&
            (ERR_GET_LIB(err) == ERR_LIB_ASN1) &&
            (ERR_GET_REASON(err) == ASN1_R_HEADER_TOO_LONG)) {
        /* EOF ASN1 file, not an error */
        ERR_clear_error();
        retval = 0;
    } else if ((filetype == SSL_FILETYPE_PEM) &&
                   (loaded > 0) &&
                   (ERR_GET_LIB(err) == ERR_LIB_PEM) &&
                   (ERR_GET_REASON(err) == PEM_R_NO_START_LINE)) {
        /* EOF PEM file, not an error */
        ERR_clear_error();
        retval = 0;
    } else {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        retval = -1;
    }

    BIO_free(biobuf);
    return retval;
}


static PyObject *
load_verify_locations(PySSLContext *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"cafile", "capath", "cadata", NULL};
    PyObject *cadata = NULL, *cafile = NULL, *capath = NULL;
    PyObject *cafile_bytes = NULL, *capath_bytes = NULL;
    const char *cafile_buf = NULL, *capath_buf = NULL;
    int r = 0, ok = 1;

    errno = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
            "|OOO:load_verify_locations", kwlist,
            &cafile, &capath, &cadata))
        return NULL;

    if (cafile == Py_None)
        cafile = NULL;
    if (capath == Py_None)
        capath = NULL;
    if (cadata == Py_None)
        cadata = NULL;

    if (cafile == NULL && capath == NULL && cadata == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "cafile, capath and cadata cannot be all omitted");
        goto error;
    }

    if (cafile) {
        if (PyString_Check(cafile)) {
            Py_INCREF(cafile);
            cafile_bytes = cafile;
        } else {
            PyObject *u = PyUnicode_FromObject(cafile);
            if (!u)
                goto error;
            cafile_bytes = PyUnicode_AsEncodedString(
                u, Py_FileSystemDefaultEncoding, NULL);
            Py_DECREF(u);
            if (!cafile_bytes)
                goto error;
        }
    }
    if (capath) {
        if (PyString_Check(capath)) {
            Py_INCREF(capath);
            capath_bytes = capath;
        } else {
            PyObject *u = PyUnicode_FromObject(capath);
            if (!u)
                goto error;
            capath_bytes = PyUnicode_AsEncodedString(
                u, Py_FileSystemDefaultEncoding, NULL);
            Py_DECREF(u);
            if (!capath_bytes)
                goto error;
        }
    }

    /* validata cadata type and load cadata */
    if (cadata) {
        Py_buffer buf;
        PyObject *cadata_ascii = NULL;

        if (!PyUnicode_Check(cadata) && PyObject_GetBuffer(cadata, &buf, PyBUF_SIMPLE) == 0) {
            if (!PyBuffer_IsContiguous(&buf, 'C') || buf.ndim > 1) {
                PyBuffer_Release(&buf);
                PyErr_SetString(PyExc_TypeError,
                                "cadata should be a contiguous buffer with "
                                "a single dimension");
                goto error;
            }
            r = _add_ca_certs(self, buf.buf, buf.len, SSL_FILETYPE_ASN1);
            PyBuffer_Release(&buf);
            if (r == -1) {
                goto error;
            }
        } else {
            PyErr_Clear();
            cadata_ascii = PyUnicode_AsASCIIString(cadata);
            if (cadata_ascii == NULL) {
                PyErr_SetString(PyExc_TypeError,
                                "cadata should be an ASCII string or a "
                                "bytes-like object");
                goto error;
            }
            r = _add_ca_certs(self,
                              PyBytes_AS_STRING(cadata_ascii),
                              PyBytes_GET_SIZE(cadata_ascii),
                              SSL_FILETYPE_PEM);
            Py_DECREF(cadata_ascii);
            if (r == -1) {
                goto error;
            }
        }
    }

    /* load cafile or capath */
    if (cafile_bytes || capath_bytes) {
        if (cafile)
            cafile_buf = PyBytes_AS_STRING(cafile_bytes);
        if (capath)
            capath_buf = PyBytes_AS_STRING(capath_bytes);
        PySSL_BEGIN_ALLOW_THREADS
        r = SSL_CTX_load_verify_locations(
            self->ctx,
            cafile_buf,
            capath_buf);
        PySSL_END_ALLOW_THREADS
        if (r != 1) {
            ok = 0;
            if (errno != 0) {
                ERR_clear_error();
                PyErr_SetFromErrno(PyExc_IOError);
            }
            else {
                _setSSLError(NULL, 0, __FILE__, __LINE__);
            }
            goto error;
        }
    }
    goto end;

  error:
    ok = 0;
  end:
    Py_XDECREF(cafile_bytes);
    Py_XDECREF(capath_bytes);
    if (ok) {
        Py_RETURN_NONE;
    } else {
        return NULL;
    }
}

static PyObject *
load_dh_params(PySSLContext *self, PyObject *filepath)
{
    BIO *bio;
    DH *dh;
    char *path = PyBytes_AsString(filepath);
    if (!path) {
        return NULL;
    }

    bio = BIO_new_file(path, "r");
    if (bio == NULL) {
        ERR_clear_error();
        PyErr_SetFromErrnoWithFilenameObject(PyExc_IOError, filepath);
        return NULL;
    }
    errno = 0;
    PySSL_BEGIN_ALLOW_THREADS
    dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
    BIO_free(bio);
    PySSL_END_ALLOW_THREADS
    if (dh == NULL) {
        if (errno != 0) {
            ERR_clear_error();
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, filepath);
        }
        else {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
        }
        return NULL;
    }
    if (SSL_CTX_set_tmp_dh(self->ctx, dh) == 0)
        _setSSLError(NULL, 0, __FILE__, __LINE__);
    DH_free(dh);
    Py_RETURN_NONE;
}

static PyObject *
context_wrap_socket(PySSLContext *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"sock", "server_side", "server_hostname", "ssl_sock", NULL};
    PySocketSockObject *sock;
    int server_side = 0;
    char *hostname = NULL;
    PyObject *hostname_obj, *ssl_sock = Py_None, *res;

    /* server_hostname is either None (or absent), or to be encoded
       using the idna encoding. */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!i|O!O:_wrap_socket", kwlist,
                                     PySocketModule.Sock_Type,
                                     &sock, &server_side,
                                     Py_TYPE(Py_None), &hostname_obj,
                                     &ssl_sock)) {
        PyErr_Clear();
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!iet|O:_wrap_socket", kwlist,
            PySocketModule.Sock_Type,
            &sock, &server_side,
            "idna", &hostname, &ssl_sock))
            return NULL;
    }

    res = (PyObject *) newPySSLSocket(self, sock, server_side,
                                      hostname, ssl_sock);
    if (hostname != NULL)
        PyMem_Free(hostname);
    return res;
}

static PyObject *
session_stats(PySSLContext *self, PyObject *unused)
{
    int r;
    PyObject *value, *stats = PyDict_New();
    if (!stats)
        return NULL;

#define ADD_STATS(SSL_NAME, KEY_NAME) \
    value = PyLong_FromLong(SSL_CTX_sess_ ## SSL_NAME (self->ctx)); \
    if (value == NULL) \
        goto error; \
    r = PyDict_SetItemString(stats, KEY_NAME, value); \
    Py_DECREF(value); \
    if (r < 0) \
        goto error;

    ADD_STATS(number, "number");
    ADD_STATS(connect, "connect");
    ADD_STATS(connect_good, "connect_good");
    ADD_STATS(connect_renegotiate, "connect_renegotiate");
    ADD_STATS(accept, "accept");
    ADD_STATS(accept_good, "accept_good");
    ADD_STATS(accept_renegotiate, "accept_renegotiate");
    ADD_STATS(accept, "accept");
    ADD_STATS(hits, "hits");
    ADD_STATS(misses, "misses");
    ADD_STATS(timeouts, "timeouts");
    ADD_STATS(cache_full, "cache_full");

#undef ADD_STATS

    return stats;

error:
    Py_DECREF(stats);
    return NULL;
}

static PyObject *
set_default_verify_paths(PySSLContext *self, PyObject *unused)
{
    if (!SSL_CTX_set_default_verify_paths(self->ctx)) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    Py_RETURN_NONE;
}

#ifndef OPENSSL_NO_ECDH
static PyObject *
set_ecdh_curve(PySSLContext *self, PyObject *name)
{
    char *name_bytes;
    int nid;
    EC_KEY *key;

    name_bytes = PyBytes_AsString(name);
    if (!name_bytes) {
        return NULL;
    }
    nid = OBJ_sn2nid(name_bytes);
    if (nid == 0) {
        PyObject *r = PyObject_Repr(name);
        if (!r)
            return NULL;
        PyErr_Format(PyExc_ValueError,
                     "unknown elliptic curve name %s", PyString_AS_STRING(r));
        Py_DECREF(r);
        return NULL;
    }
    key = EC_KEY_new_by_curve_name(nid);
    if (key == NULL) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    SSL_CTX_set_tmp_ecdh(self->ctx, key);
    EC_KEY_free(key);
    Py_RETURN_NONE;
}
#endif

#if HAVE_SNI && !defined(OPENSSL_NO_TLSEXT)
static int
_servername_callback(SSL *s, int *al, void *args)
{
    int ret;
    PySSLContext *ssl_ctx = (PySSLContext *) args;
    PySSLSocket *ssl;
    PyObject *servername_o;
    PyObject *servername_idna;
    PyObject *result;
    /* The high-level ssl.SSLSocket object */
    PyObject *ssl_socket;
    const char *servername = SSL_get_servername(s, TLSEXT_NAMETYPE_host_name);
#ifdef WITH_THREAD
    PyGILState_STATE gstate = PyGILState_Ensure();
#endif

    if (ssl_ctx->set_hostname == NULL) {
        /* remove race condition in this the call back while if removing the
         * callback is in progress */
#ifdef WITH_THREAD
        PyGILState_Release(gstate);
#endif
        return SSL_TLSEXT_ERR_OK;
    }

    ssl = SSL_get_app_data(s);
    assert(PySSLSocket_Check(ssl));
    if (ssl->ssl_sock == NULL) {
        ssl_socket = Py_None;
    } else {
        ssl_socket = PyWeakref_GetObject(ssl->ssl_sock);
        Py_INCREF(ssl_socket);
    }
    if (ssl_socket == Py_None) {
        goto error;
    }

    if (servername == NULL) {
        result = PyObject_CallFunctionObjArgs(ssl_ctx->set_hostname, ssl_socket,
                                              Py_None, ssl_ctx, NULL);
    }
    else {
        servername_o = PyBytes_FromString(servername);
        if (servername_o == NULL) {
            PyErr_WriteUnraisable((PyObject *) ssl_ctx);
            goto error;
        }
        servername_idna = PyUnicode_FromEncodedObject(servername_o, "idna", NULL);
        if (servername_idna == NULL) {
            PyErr_WriteUnraisable(servername_o);
            Py_DECREF(servername_o);
            goto error;
        }
        Py_DECREF(servername_o);
        result = PyObject_CallFunctionObjArgs(ssl_ctx->set_hostname, ssl_socket,
                                              servername_idna, ssl_ctx, NULL);
        Py_DECREF(servername_idna);
    }
    Py_DECREF(ssl_socket);

    if (result == NULL) {
        PyErr_WriteUnraisable(ssl_ctx->set_hostname);
        *al = SSL_AD_HANDSHAKE_FAILURE;
        ret = SSL_TLSEXT_ERR_ALERT_FATAL;
    }
    else {
        if (result != Py_None) {
            *al = (int) PyLong_AsLong(result);
            if (PyErr_Occurred()) {
                PyErr_WriteUnraisable(result);
                *al = SSL_AD_INTERNAL_ERROR;
            }
            ret = SSL_TLSEXT_ERR_ALERT_FATAL;
        }
        else {
            ret = SSL_TLSEXT_ERR_OK;
        }
        Py_DECREF(result);
    }

#ifdef WITH_THREAD
    PyGILState_Release(gstate);
#endif
    return ret;

error:
    Py_DECREF(ssl_socket);
    *al = SSL_AD_INTERNAL_ERROR;
    ret = SSL_TLSEXT_ERR_ALERT_FATAL;
#ifdef WITH_THREAD
    PyGILState_Release(gstate);
#endif
    return ret;
}
#endif

PyDoc_STRVAR(PySSL_set_servername_callback_doc,
"set_servername_callback(method)\n\
\n\
This sets a callback that will be called when a server name is provided by\n\
the SSL/TLS client in the SNI extension.\n\
\n\
If the argument is None then the callback is disabled. The method is called\n\
with the SSLSocket, the server name as a string, and the SSLContext object.\n\
See RFC 6066 for details of the SNI extension.");

static PyObject *
set_servername_callback(PySSLContext *self, PyObject *args)
{
#if HAVE_SNI && !defined(OPENSSL_NO_TLSEXT)
    PyObject *cb;

    if (!PyArg_ParseTuple(args, "O", &cb))
        return NULL;

    Py_CLEAR(self->set_hostname);
    if (cb == Py_None) {
        SSL_CTX_set_tlsext_servername_callback(self->ctx, NULL);
    }
    else {
        if (!PyCallable_Check(cb)) {
            SSL_CTX_set_tlsext_servername_callback(self->ctx, NULL);
            PyErr_SetString(PyExc_TypeError,
                            "not a callable object");
            return NULL;
        }
        Py_INCREF(cb);
        self->set_hostname = cb;
        SSL_CTX_set_tlsext_servername_callback(self->ctx, _servername_callback);
        SSL_CTX_set_tlsext_servername_arg(self->ctx, self);
    }
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "The TLS extension servername callback, "
                    "SSL_CTX_set_tlsext_servername_callback, "
                    "is not in the current OpenSSL library.");
    return NULL;
#endif
}

PyDoc_STRVAR(PySSL_get_stats_doc,
"cert_store_stats() -> {'crl': int, 'x509_ca': int, 'x509': int}\n\
\n\
Returns quantities of loaded X.509 certificates. X.509 certificates with a\n\
CA extension and certificate revocation lists inside the context's cert\n\
store.\n\
NOTE: Certificates in a capath directory aren't loaded unless they have\n\
been used at least once.");

static PyObject *
cert_store_stats(PySSLContext *self)
{
    X509_STORE *store;
    X509_OBJECT *obj;
    int x509 = 0, crl = 0, pkey = 0, ca = 0, i;

    store = SSL_CTX_get_cert_store(self->ctx);
    for (i = 0; i < sk_X509_OBJECT_num(store->objs); i++) {
        obj = sk_X509_OBJECT_value(store->objs, i);
        switch (obj->type) {
            case X509_LU_X509:
                x509++;
                if (X509_check_ca(obj->data.x509)) {
                    ca++;
                }
                break;
            case X509_LU_CRL:
                crl++;
                break;
            case X509_LU_PKEY:
                pkey++;
                break;
            default:
                /* Ignore X509_LU_FAIL, X509_LU_RETRY, X509_LU_PKEY.
                 * As far as I can tell they are internal states and never
                 * stored in a cert store */
                break;
        }
    }
    return Py_BuildValue("{sisisi}", "x509", x509, "crl", crl,
        "x509_ca", ca);
}

PyDoc_STRVAR(PySSL_get_ca_certs_doc,
"get_ca_certs(binary_form=False) -> list of loaded certificate\n\
\n\
Returns a list of dicts with information of loaded CA certs. If the\n\
optional argument is True, returns a DER-encoded copy of the CA certificate.\n\
NOTE: Certificates in a capath directory aren't loaded unless they have\n\
been used at least once.");

static PyObject *
get_ca_certs(PySSLContext *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"binary_form", NULL};
    X509_STORE *store;
    PyObject *ci = NULL, *rlist = NULL, *py_binary_mode = Py_False;
    int i;
    int binary_mode = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:get_ca_certs",
                                     kwlist, &py_binary_mode)) {
        return NULL;
    }
    binary_mode = PyObject_IsTrue(py_binary_mode);
    if (binary_mode < 0) {
        return NULL;
    }

    if ((rlist = PyList_New(0)) == NULL) {
        return NULL;
    }

    store = SSL_CTX_get_cert_store(self->ctx);
    for (i = 0; i < sk_X509_OBJECT_num(store->objs); i++) {
        X509_OBJECT *obj;
        X509 *cert;

        obj = sk_X509_OBJECT_value(store->objs, i);
        if (obj->type != X509_LU_X509) {
            /* not a x509 cert */
            continue;
        }
        /* CA for any purpose */
        cert = obj->data.x509;
        if (!X509_check_ca(cert)) {
            continue;
        }
        if (binary_mode) {
            ci = _certificate_to_der(cert);
        } else {
            ci = _decode_certificate(cert);
        }
        if (ci == NULL) {
            goto error;
        }
        if (PyList_Append(rlist, ci) == -1) {
            goto error;
        }
        Py_CLEAR(ci);
    }
    return rlist;

  error:
    Py_XDECREF(ci);
    Py_XDECREF(rlist);
    return NULL;
}


static PyGetSetDef context_getsetlist[] = {
    {"check_hostname", (getter) get_check_hostname,
                       (setter) set_check_hostname, NULL},
    {"options", (getter) get_options,
                (setter) set_options, NULL},
#ifdef HAVE_OPENSSL_VERIFY_PARAM
    {"verify_flags", (getter) get_verify_flags,
                     (setter) set_verify_flags, NULL},
#endif
    {"verify_mode", (getter) get_verify_mode,
                    (setter) set_verify_mode, NULL},
    {NULL},            /* sentinel */
};

static struct PyMethodDef context_methods[] = {
    {"_wrap_socket", (PyCFunction) context_wrap_socket,
                       METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_ciphers", (PyCFunction) set_ciphers,
                    METH_VARARGS, NULL},
    {"_set_alpn_protocols", (PyCFunction) _set_alpn_protocols,
                           METH_VARARGS, NULL},
    {"_set_npn_protocols", (PyCFunction) _set_npn_protocols,
                           METH_VARARGS, NULL},
    {"load_cert_chain", (PyCFunction) load_cert_chain,
                        METH_VARARGS | METH_KEYWORDS, NULL},
    {"load_dh_params", (PyCFunction) load_dh_params,
                       METH_O, NULL},
    {"load_verify_locations", (PyCFunction) load_verify_locations,
                              METH_VARARGS | METH_KEYWORDS, NULL},
    {"session_stats", (PyCFunction) session_stats,
                      METH_NOARGS, NULL},
    {"set_default_verify_paths", (PyCFunction) set_default_verify_paths,
                                 METH_NOARGS, NULL},
#ifndef OPENSSL_NO_ECDH
    {"set_ecdh_curve", (PyCFunction) set_ecdh_curve,
                       METH_O, NULL},
#endif
    {"set_servername_callback", (PyCFunction) set_servername_callback,
                    METH_VARARGS, PySSL_set_servername_callback_doc},
    {"cert_store_stats", (PyCFunction) cert_store_stats,
                    METH_NOARGS, PySSL_get_stats_doc},
    {"get_ca_certs", (PyCFunction) get_ca_certs,
                    METH_VARARGS | METH_KEYWORDS, PySSL_get_ca_certs_doc},
    {NULL, NULL}        /* sentinel */
};

static PyTypeObject PySSLContext_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ssl._SSLContext",                        /*tp_name*/
    sizeof(PySSLContext),                      /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)context_dealloc,               /*tp_dealloc*/
    0,                                         /*tp_print*/
    0,                                         /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_reserved*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
    0,                                         /*tp_call*/
    0,                                         /*tp_str*/
    0,                                         /*tp_getattro*/
    0,                                         /*tp_setattro*/
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,                                         /*tp_doc*/
    (traverseproc) context_traverse,           /*tp_traverse*/
    (inquiry) context_clear,                   /*tp_clear*/
    0,                                         /*tp_richcompare*/
    0,                                         /*tp_weaklistoffset*/
    0,                                         /*tp_iter*/
    0,                                         /*tp_iternext*/
    context_methods,                           /*tp_methods*/
    0,                                         /*tp_members*/
    context_getsetlist,                        /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    0,                                         /*tp_dictoffset*/
    0,                                         /*tp_init*/
    0,                                         /*tp_alloc*/
    context_new,                               /*tp_new*/
};



#ifdef HAVE_OPENSSL_RAND

/* helper routines for seeding the SSL PRNG */
static PyObject *
PySSL_RAND_add(PyObject *self, PyObject *args)
{
    char *buf;
    Py_ssize_t len, written;
    double entropy;

    if (!PyArg_ParseTuple(args, "s#d:RAND_add", &buf, &len, &entropy))
        return NULL;
    do {
        if (len >= INT_MAX) {
            written = INT_MAX;
        } else {
            written = len;
        }
        RAND_add(buf, (int)written, entropy);
        buf += written;
        len -= written;
    } while (len);
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(PySSL_RAND_add_doc,
"RAND_add(string, entropy)\n\
\n\
Mix string into the OpenSSL PRNG state.  entropy (a float) is a lower\n\
bound on the entropy contained in string.  See RFC 1750.");

static PyObject *
PySSL_RAND_status(PyObject *self)
{
    return PyLong_FromLong(RAND_status());
}

PyDoc_STRVAR(PySSL_RAND_status_doc,
"RAND_status() -> 0 or 1\n\
\n\
Returns 1 if the OpenSSL PRNG has been seeded with enough data and 0 if not.\n\
It is necessary to seed the PRNG with RAND_add() on some platforms before\n\
using the ssl() function.");

#endif /* HAVE_OPENSSL_RAND */


#ifdef HAVE_RAND_EGD

static PyObject *
PySSL_RAND_egd(PyObject *self, PyObject *arg)
{
    int bytes;

    if (!PyString_Check(arg))
        return PyErr_Format(PyExc_TypeError,
                            "RAND_egd() expected string, found %s",
                            Py_TYPE(arg)->tp_name);
    bytes = RAND_egd(PyString_AS_STRING(arg));
    if (bytes == -1) {
        PyErr_SetString(PySSLErrorObject,
                        "EGD connection failed or EGD did not return "
                        "enough data to seed the PRNG");
        return NULL;
    }
    return PyInt_FromLong(bytes);
}

PyDoc_STRVAR(PySSL_RAND_egd_doc,
"RAND_egd(path) -> bytes\n\
\n\
Queries the entropy gather daemon (EGD) on the socket named by 'path'.\n\
Returns number of bytes read.  Raises SSLError if connection to EGD\n\
fails or if it does not provide enough data to seed PRNG.");

#endif /* HAVE_RAND_EGD */


PyDoc_STRVAR(PySSL_get_default_verify_paths_doc,
"get_default_verify_paths() -> tuple\n\
\n\
Return search paths and environment vars that are used by SSLContext's\n\
set_default_verify_paths() to load default CAs. The values are\n\
'cert_file_env', 'cert_file', 'cert_dir_env', 'cert_dir'.");

static PyObject *
PySSL_get_default_verify_paths(PyObject *self)
{
    PyObject *ofile_env = NULL;
    PyObject *ofile = NULL;
    PyObject *odir_env = NULL;
    PyObject *odir = NULL;

#define CONVERT(info, target) { \
        const char *tmp = (info); \
        target = NULL; \
        if (!tmp) { Py_INCREF(Py_None); target = Py_None; } \
        else { target = PyBytes_FromString(tmp); } \
        if (!target) goto error; \
    }

    CONVERT(X509_get_default_cert_file_env(), ofile_env);
    CONVERT(X509_get_default_cert_file(), ofile);
    CONVERT(X509_get_default_cert_dir_env(), odir_env);
    CONVERT(X509_get_default_cert_dir(), odir);
#undef CONVERT

    return Py_BuildValue("NNNN", ofile_env, ofile, odir_env, odir);

  error:
    Py_XDECREF(ofile_env);
    Py_XDECREF(ofile);
    Py_XDECREF(odir_env);
    Py_XDECREF(odir);
    return NULL;
}

static PyObject*
asn1obj2py(ASN1_OBJECT *obj)
{
    int nid;
    const char *ln, *sn;
    char buf[100];
    Py_ssize_t buflen;

    nid = OBJ_obj2nid(obj);
    if (nid == NID_undef) {
        PyErr_Format(PyExc_ValueError, "Unknown object");
        return NULL;
    }
    sn = OBJ_nid2sn(nid);
    ln = OBJ_nid2ln(nid);
    buflen = OBJ_obj2txt(buf, sizeof(buf), obj, 1);
    if (buflen < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    if (buflen) {
        return Py_BuildValue("isss#", nid, sn, ln, buf, buflen);
    } else {
        return Py_BuildValue("issO", nid, sn, ln, Py_None);
    }
}

PyDoc_STRVAR(PySSL_txt2obj_doc,
"txt2obj(txt, name=False) -> (nid, shortname, longname, oid)\n\
\n\
Lookup NID, short name, long name and OID of an ASN1_OBJECT. By default\n\
objects are looked up by OID. With name=True short and long name are also\n\
matched.");

static PyObject*
PySSL_txt2obj(PyObject *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"txt", "name", NULL};
    PyObject *result = NULL;
    char *txt;
    PyObject *pyname = Py_None;
    int name = 0;
    ASN1_OBJECT *obj;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O:txt2obj",
                                     kwlist, &txt, &pyname)) {
        return NULL;
    }
    name = PyObject_IsTrue(pyname);
    if (name < 0)
        return NULL;
    obj = OBJ_txt2obj(txt, name ? 0 : 1);
    if (obj == NULL) {
        PyErr_Format(PyExc_ValueError, "unknown object '%.100s'", txt);
        return NULL;
    }
    result = asn1obj2py(obj);
    ASN1_OBJECT_free(obj);
    return result;
}

PyDoc_STRVAR(PySSL_nid2obj_doc,
"nid2obj(nid) -> (nid, shortname, longname, oid)\n\
\n\
Lookup NID, short name, long name and OID of an ASN1_OBJECT by NID.");

static PyObject*
PySSL_nid2obj(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    int nid;
    ASN1_OBJECT *obj;

    if (!PyArg_ParseTuple(args, "i:nid2obj", &nid)) {
        return NULL;
    }
    if (nid < NID_undef) {
        PyErr_SetString(PyExc_ValueError, "NID must be positive.");
        return NULL;
    }
    obj = OBJ_nid2obj(nid);
    if (obj == NULL) {
        PyErr_Format(PyExc_ValueError, "unknown NID %i", nid);
        return NULL;
    }
    result = asn1obj2py(obj);
    ASN1_OBJECT_free(obj);
    return result;
}

#ifdef _MSC_VER

static PyObject*
certEncodingType(DWORD encodingType)
{
    static PyObject *x509_asn = NULL;
    static PyObject *pkcs_7_asn = NULL;

    if (x509_asn == NULL) {
        x509_asn = PyString_InternFromString("x509_asn");
        if (x509_asn == NULL)
            return NULL;
    }
    if (pkcs_7_asn == NULL) {
        pkcs_7_asn = PyString_InternFromString("pkcs_7_asn");
        if (pkcs_7_asn == NULL)
            return NULL;
    }
    switch(encodingType) {
    case X509_ASN_ENCODING:
        Py_INCREF(x509_asn);
        return x509_asn;
    case PKCS_7_ASN_ENCODING:
        Py_INCREF(pkcs_7_asn);
        return pkcs_7_asn;
    default:
        return PyInt_FromLong(encodingType);
    }
}

static PyObject*
parseKeyUsage(PCCERT_CONTEXT pCertCtx, DWORD flags)
{
    CERT_ENHKEY_USAGE *usage;
    DWORD size, error, i;
    PyObject *retval;

    if (!CertGetEnhancedKeyUsage(pCertCtx, flags, NULL, &size)) {
        error = GetLastError();
        if (error == CRYPT_E_NOT_FOUND) {
            Py_RETURN_TRUE;
        }
        return PyErr_SetFromWindowsErr(error);
    }

    usage = (CERT_ENHKEY_USAGE*)PyMem_Malloc(size);
    if (usage == NULL) {
        return PyErr_NoMemory();
    }

    /* Now get the actual enhanced usage property */
    if (!CertGetEnhancedKeyUsage(pCertCtx, flags, usage, &size)) {
        PyMem_Free(usage);
        error = GetLastError();
        if (error == CRYPT_E_NOT_FOUND) {
            Py_RETURN_TRUE;
        }
        return PyErr_SetFromWindowsErr(error);
    }
    retval = PySet_New(NULL);
    if (retval == NULL) {
        goto error;
    }
    for (i = 0; i < usage->cUsageIdentifier; ++i) {
        if (usage->rgpszUsageIdentifier[i]) {
            PyObject *oid;
            int err;
            oid = PyString_FromString(usage->rgpszUsageIdentifier[i]);
            if (oid == NULL) {
                Py_CLEAR(retval);
                goto error;
            }
            err = PySet_Add(retval, oid);
            Py_DECREF(oid);
            if (err == -1) {
                Py_CLEAR(retval);
                goto error;
            }
        }
    }
  error:
    PyMem_Free(usage);
    return retval;
}

PyDoc_STRVAR(PySSL_enum_certificates_doc,
"enum_certificates(store_name) -> []\n\
\n\
Retrieve certificates from Windows' cert store. store_name may be one of\n\
'CA', 'ROOT' or 'MY'. The system may provide more cert storages, too.\n\
The function returns a list of (bytes, encoding_type, trust) tuples. The\n\
encoding_type flag can be interpreted with X509_ASN_ENCODING or\n\
PKCS_7_ASN_ENCODING. The trust setting is either a set of OIDs or the\n\
boolean True.");

static PyObject *
PySSL_enum_certificates(PyObject *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"store_name", NULL};
    char *store_name;
    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT pCertCtx = NULL;
    PyObject *keyusage = NULL, *cert = NULL, *enc = NULL, *tup = NULL;
    PyObject *result = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s:enum_certificates",
                                     kwlist, &store_name)) {
        return NULL;
    }
    result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }
    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, (HCRYPTPROV)NULL,
                            CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE,
                            store_name);
    if (hStore == NULL) {
        Py_DECREF(result);
        return PyErr_SetFromWindowsErr(GetLastError());
    }

    while (pCertCtx = CertEnumCertificatesInStore(hStore, pCertCtx)) {
        cert = PyBytes_FromStringAndSize((const char*)pCertCtx->pbCertEncoded,
                                            pCertCtx->cbCertEncoded);
        if (!cert) {
            Py_CLEAR(result);
            break;
        }
        if ((enc = certEncodingType(pCertCtx->dwCertEncodingType)) == NULL) {
            Py_CLEAR(result);
            break;
        }
        keyusage = parseKeyUsage(pCertCtx, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG);
        if (keyusage == Py_True) {
            Py_DECREF(keyusage);
            keyusage = parseKeyUsage(pCertCtx, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG);
        }
        if (keyusage == NULL) {
            Py_CLEAR(result);
            break;
        }
        if ((tup = PyTuple_New(3)) == NULL) {
            Py_CLEAR(result);
            break;
        }
        PyTuple_SET_ITEM(tup, 0, cert);
        cert = NULL;
        PyTuple_SET_ITEM(tup, 1, enc);
        enc = NULL;
        PyTuple_SET_ITEM(tup, 2, keyusage);
        keyusage = NULL;
        if (PyList_Append(result, tup) < 0) {
            Py_CLEAR(result);
            break;
        }
        Py_CLEAR(tup);
    }
    if (pCertCtx) {
        /* loop ended with an error, need to clean up context manually */
        CertFreeCertificateContext(pCertCtx);
    }

    /* In error cases cert, enc and tup may not be NULL */
    Py_XDECREF(cert);
    Py_XDECREF(enc);
    Py_XDECREF(keyusage);
    Py_XDECREF(tup);

    if (!CertCloseStore(hStore, 0)) {
        /* This error case might shadow another exception.*/
        Py_XDECREF(result);
        return PyErr_SetFromWindowsErr(GetLastError());
    }
    return result;
}

PyDoc_STRVAR(PySSL_enum_crls_doc,
"enum_crls(store_name) -> []\n\
\n\
Retrieve CRLs from Windows' cert store. store_name may be one of\n\
'CA', 'ROOT' or 'MY'. The system may provide more cert storages, too.\n\
The function returns a list of (bytes, encoding_type) tuples. The\n\
encoding_type flag can be interpreted with X509_ASN_ENCODING or\n\
PKCS_7_ASN_ENCODING.");

static PyObject *
PySSL_enum_crls(PyObject *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"store_name", NULL};
    char *store_name;
    HCERTSTORE hStore = NULL;
    PCCRL_CONTEXT pCrlCtx = NULL;
    PyObject *crl = NULL, *enc = NULL, *tup = NULL;
    PyObject *result = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s:enum_crls",
                                     kwlist, &store_name)) {
        return NULL;
    }
    result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }
    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, (HCRYPTPROV)NULL,
                            CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE,
                            store_name);
    if (hStore == NULL) {
        Py_DECREF(result);
        return PyErr_SetFromWindowsErr(GetLastError());
    }

    while (pCrlCtx = CertEnumCRLsInStore(hStore, pCrlCtx)) {
        crl = PyBytes_FromStringAndSize((const char*)pCrlCtx->pbCrlEncoded,
                                            pCrlCtx->cbCrlEncoded);
        if (!crl) {
            Py_CLEAR(result);
            break;
        }
        if ((enc = certEncodingType(pCrlCtx->dwCertEncodingType)) == NULL) {
            Py_CLEAR(result);
            break;
        }
        if ((tup = PyTuple_New(2)) == NULL) {
            Py_CLEAR(result);
            break;
        }
        PyTuple_SET_ITEM(tup, 0, crl);
        crl = NULL;
        PyTuple_SET_ITEM(tup, 1, enc);
        enc = NULL;

        if (PyList_Append(result, tup) < 0) {
            Py_CLEAR(result);
            break;
        }
        Py_CLEAR(tup);
    }
    if (pCrlCtx) {
        /* loop ended with an error, need to clean up context manually */
        CertFreeCRLContext(pCrlCtx);
    }

    /* In error cases cert, enc and tup may not be NULL */
    Py_XDECREF(crl);
    Py_XDECREF(enc);
    Py_XDECREF(tup);

    if (!CertCloseStore(hStore, 0)) {
        /* This error case might shadow another exception.*/
        Py_XDECREF(result);
        return PyErr_SetFromWindowsErr(GetLastError());
    }
    return result;
}

#endif /* _MSC_VER */

/* List of functions exported by this module. */

static PyMethodDef PySSL_methods[] = {
    {"_test_decode_cert",       PySSL_test_decode_certificate,
     METH_VARARGS},
#ifdef HAVE_OPENSSL_RAND
    {"RAND_add",            PySSL_RAND_add, METH_VARARGS,
     PySSL_RAND_add_doc},
    {"RAND_status",         (PyCFunction)PySSL_RAND_status, METH_NOARGS,
     PySSL_RAND_status_doc},
#endif
#ifdef HAVE_RAND_EGD
    {"RAND_egd",            PySSL_RAND_egd, METH_VARARGS,
     PySSL_RAND_egd_doc},
#endif
    {"get_default_verify_paths", (PyCFunction)PySSL_get_default_verify_paths,
     METH_NOARGS, PySSL_get_default_verify_paths_doc},
#ifdef _MSC_VER
    {"enum_certificates", (PyCFunction)PySSL_enum_certificates,
     METH_VARARGS | METH_KEYWORDS, PySSL_enum_certificates_doc},
    {"enum_crls", (PyCFunction)PySSL_enum_crls,
     METH_VARARGS | METH_KEYWORDS, PySSL_enum_crls_doc},
#endif
    {"txt2obj", (PyCFunction)PySSL_txt2obj,
     METH_VARARGS | METH_KEYWORDS, PySSL_txt2obj_doc},
    {"nid2obj", (PyCFunction)PySSL_nid2obj,
     METH_VARARGS, PySSL_nid2obj_doc},
    {NULL,                  NULL}            /* Sentinel */
};


#ifdef WITH_THREAD

/* an implementation of OpenSSL threading operations in terms
   of the Python C thread library */

static PyThread_type_lock *_ssl_locks = NULL;

#if OPENSSL_VERSION_NUMBER >= 0x10000000
/* use new CRYPTO_THREADID API. */
static void
_ssl_threadid_callback(CRYPTO_THREADID *id)
{
    CRYPTO_THREADID_set_numeric(id,
                                (unsigned long)PyThread_get_thread_ident());
}
#else
/* deprecated CRYPTO_set_id_callback() API. */
static unsigned long
_ssl_thread_id_function (void) {
    return PyThread_get_thread_ident();
}
#endif

static void _ssl_thread_locking_function
    (int mode, int n, const char *file, int line) {
    /* this function is needed to perform locking on shared data
       structures. (Note that OpenSSL uses a number of global data
       structures that will be implicitly shared whenever multiple
       threads use OpenSSL.) Multi-threaded applications will
       crash at random if it is not set.

       locking_function() must be able to handle up to
       CRYPTO_num_locks() different mutex locks. It sets the n-th
       lock if mode & CRYPTO_LOCK, and releases it otherwise.

       file and line are the file number of the function setting the
       lock. They can be useful for debugging.
    */

    if ((_ssl_locks == NULL) ||
        (n < 0) || ((unsigned)n >= _ssl_locks_count))
        return;

    if (mode & CRYPTO_LOCK) {
        PyThread_acquire_lock(_ssl_locks[n], 1);
    } else {
        PyThread_release_lock(_ssl_locks[n]);
    }
}

static int _setup_ssl_threads(void) {

    unsigned int i;

    if (_ssl_locks == NULL) {
        _ssl_locks_count = CRYPTO_num_locks();
        _ssl_locks = PyMem_New(PyThread_type_lock, _ssl_locks_count);
        if (_ssl_locks == NULL) {
            PyErr_NoMemory();
            return 0;
        }
        memset(_ssl_locks, 0,
               sizeof(PyThread_type_lock) * _ssl_locks_count);
        for (i = 0;  i < _ssl_locks_count;  i++) {
            _ssl_locks[i] = PyThread_allocate_lock();
            if (_ssl_locks[i] == NULL) {
                unsigned int j;
                for (j = 0;  j < i;  j++) {
                    PyThread_free_lock(_ssl_locks[j]);
                }
                PyMem_Free(_ssl_locks);
                return 0;
            }
        }
        CRYPTO_set_locking_callback(_ssl_thread_locking_function);
#if OPENSSL_VERSION_NUMBER >= 0x10000000
        CRYPTO_THREADID_set_callback(_ssl_threadid_callback);
#else
        CRYPTO_set_id_callback(_ssl_thread_id_function);
#endif
    }
    return 1;
}

#endif  /* def HAVE_THREAD */

PyDoc_STRVAR(module_doc,
"Implementation module for SSL socket operations.  See the socket module\n\
for documentation.");




static void
parse_openssl_version(unsigned long libver,
                      unsigned int *major, unsigned int *minor,
                      unsigned int *fix, unsigned int *patch,
                      unsigned int *status)
{
    *status = libver & 0xF;
    libver >>= 4;
    *patch = libver & 0xFF;
    libver >>= 8;
    *fix = libver & 0xFF;
    libver >>= 8;
    *minor = libver & 0xFF;
    libver >>= 8;
    *major = libver & 0xFF;
}

PyMODINIT_FUNC
init_ssl(void)
{
    PyObject *m, *d, *r;
    unsigned long libver;
    unsigned int major, minor, fix, patch, status;
    struct py_ssl_error_code *errcode;
    struct py_ssl_library_code *libcode;

    if (PyType_Ready(&PySSLContext_Type) < 0)
        return;
    if (PyType_Ready(&PySSLSocket_Type) < 0)
        return;

    m = Py_InitModule3("_ssl", PySSL_methods, module_doc);
    if (m == NULL)
        return;
    d = PyModule_GetDict(m);

    /* Load _socket module and its C API */
    if (PySocketModule_ImportModuleAndAPI())
        return;

    /* Init OpenSSL */
    SSL_load_error_strings();
    SSL_library_init();
#ifdef WITH_THREAD
    /* note that this will start threading if not already started */
    if (!_setup_ssl_threads()) {
        return;
    }
#endif
    OpenSSL_add_all_algorithms();

    /* Add symbols to module dict */
    PySSLErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLError", SSLError_doc,
        PySocketModule.error, NULL);
    if (PySSLErrorObject == NULL)
        return;
    ((PyTypeObject *)PySSLErrorObject)->tp_str = (reprfunc)SSLError_str;

    PySSLZeroReturnErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLZeroReturnError", SSLZeroReturnError_doc,
        PySSLErrorObject, NULL);
    PySSLWantReadErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLWantReadError", SSLWantReadError_doc,
        PySSLErrorObject, NULL);
    PySSLWantWriteErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLWantWriteError", SSLWantWriteError_doc,
        PySSLErrorObject, NULL);
    PySSLSyscallErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLSyscallError", SSLSyscallError_doc,
        PySSLErrorObject, NULL);
    PySSLEOFErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLEOFError", SSLEOFError_doc,
        PySSLErrorObject, NULL);
    if (PySSLZeroReturnErrorObject == NULL
        || PySSLWantReadErrorObject == NULL
        || PySSLWantWriteErrorObject == NULL
        || PySSLSyscallErrorObject == NULL
        || PySSLEOFErrorObject == NULL)
        return;

    ((PyTypeObject *)PySSLZeroReturnErrorObject)->tp_str = (reprfunc)SSLError_str;
    ((PyTypeObject *)PySSLWantReadErrorObject)->tp_str = (reprfunc)SSLError_str;
    ((PyTypeObject *)PySSLWantWriteErrorObject)->tp_str = (reprfunc)SSLError_str;
    ((PyTypeObject *)PySSLSyscallErrorObject)->tp_str = (reprfunc)SSLError_str;
    ((PyTypeObject *)PySSLEOFErrorObject)->tp_str = (reprfunc)SSLError_str;

    if (PyDict_SetItemString(d, "SSLError", PySSLErrorObject) != 0
        || PyDict_SetItemString(d, "SSLZeroReturnError", PySSLZeroReturnErrorObject) != 0
        || PyDict_SetItemString(d, "SSLWantReadError", PySSLWantReadErrorObject) != 0
        || PyDict_SetItemString(d, "SSLWantWriteError", PySSLWantWriteErrorObject) != 0
        || PyDict_SetItemString(d, "SSLSyscallError", PySSLSyscallErrorObject) != 0
        || PyDict_SetItemString(d, "SSLEOFError", PySSLEOFErrorObject) != 0)
        return;
    if (PyDict_SetItemString(d, "_SSLContext",
                             (PyObject *)&PySSLContext_Type) != 0)
        return;
    if (PyDict_SetItemString(d, "_SSLSocket",
                             (PyObject *)&PySSLSocket_Type) != 0)
        return;
    PyModule_AddIntConstant(m, "SSL_ERROR_ZERO_RETURN",
                            PY_SSL_ERROR_ZERO_RETURN);
    PyModule_AddIntConstant(m, "SSL_ERROR_WANT_READ",
                            PY_SSL_ERROR_WANT_READ);
    PyModule_AddIntConstant(m, "SSL_ERROR_WANT_WRITE",
                            PY_SSL_ERROR_WANT_WRITE);
    PyModule_AddIntConstant(m, "SSL_ERROR_WANT_X509_LOOKUP",
                            PY_SSL_ERROR_WANT_X509_LOOKUP);
    PyModule_AddIntConstant(m, "SSL_ERROR_SYSCALL",
                            PY_SSL_ERROR_SYSCALL);
    PyModule_AddIntConstant(m, "SSL_ERROR_SSL",
                            PY_SSL_ERROR_SSL);
    PyModule_AddIntConstant(m, "SSL_ERROR_WANT_CONNECT",
                            PY_SSL_ERROR_WANT_CONNECT);
    /* non ssl.h errorcodes */
    PyModule_AddIntConstant(m, "SSL_ERROR_EOF",
                            PY_SSL_ERROR_EOF);
    PyModule_AddIntConstant(m, "SSL_ERROR_INVALID_ERROR_CODE",
                            PY_SSL_ERROR_INVALID_ERROR_CODE);
    /* cert requirements */
    PyModule_AddIntConstant(m, "CERT_NONE",
                            PY_SSL_CERT_NONE);
    PyModule_AddIntConstant(m, "CERT_OPTIONAL",
                            PY_SSL_CERT_OPTIONAL);
    PyModule_AddIntConstant(m, "CERT_REQUIRED",
                            PY_SSL_CERT_REQUIRED);
    /* CRL verification for verification_flags */
    PyModule_AddIntConstant(m, "VERIFY_DEFAULT",
                            0);
    PyModule_AddIntConstant(m, "VERIFY_CRL_CHECK_LEAF",
                            X509_V_FLAG_CRL_CHECK);
    PyModule_AddIntConstant(m, "VERIFY_CRL_CHECK_CHAIN",
                            X509_V_FLAG_CRL_CHECK|X509_V_FLAG_CRL_CHECK_ALL);
    PyModule_AddIntConstant(m, "VERIFY_X509_STRICT",
                            X509_V_FLAG_X509_STRICT);
#ifdef X509_V_FLAG_TRUSTED_FIRST
    PyModule_AddIntConstant(m, "VERIFY_X509_TRUSTED_FIRST",
                            X509_V_FLAG_TRUSTED_FIRST);
#endif

    /* Alert Descriptions from ssl.h */
    /* note RESERVED constants no longer intended for use have been removed */
    /* http://www.iana.org/assignments/tls-parameters/tls-parameters.xml#tls-parameters-6 */

#define ADD_AD_CONSTANT(s) \
    PyModule_AddIntConstant(m, "ALERT_DESCRIPTION_"#s, \
                            SSL_AD_##s)

    ADD_AD_CONSTANT(CLOSE_NOTIFY);
    ADD_AD_CONSTANT(UNEXPECTED_MESSAGE);
    ADD_AD_CONSTANT(BAD_RECORD_MAC);
    ADD_AD_CONSTANT(RECORD_OVERFLOW);
    ADD_AD_CONSTANT(DECOMPRESSION_FAILURE);
    ADD_AD_CONSTANT(HANDSHAKE_FAILURE);
    ADD_AD_CONSTANT(BAD_CERTIFICATE);
    ADD_AD_CONSTANT(UNSUPPORTED_CERTIFICATE);
    ADD_AD_CONSTANT(CERTIFICATE_REVOKED);
    ADD_AD_CONSTANT(CERTIFICATE_EXPIRED);
    ADD_AD_CONSTANT(CERTIFICATE_UNKNOWN);
    ADD_AD_CONSTANT(ILLEGAL_PARAMETER);
    ADD_AD_CONSTANT(UNKNOWN_CA);
    ADD_AD_CONSTANT(ACCESS_DENIED);
    ADD_AD_CONSTANT(DECODE_ERROR);
    ADD_AD_CONSTANT(DECRYPT_ERROR);
    ADD_AD_CONSTANT(PROTOCOL_VERSION);
    ADD_AD_CONSTANT(INSUFFICIENT_SECURITY);
    ADD_AD_CONSTANT(INTERNAL_ERROR);
    ADD_AD_CONSTANT(USER_CANCELLED);
    ADD_AD_CONSTANT(NO_RENEGOTIATION);
    /* Not all constants are in old OpenSSL versions */
#ifdef SSL_AD_UNSUPPORTED_EXTENSION
    ADD_AD_CONSTANT(UNSUPPORTED_EXTENSION);
#endif
#ifdef SSL_AD_CERTIFICATE_UNOBTAINABLE
    ADD_AD_CONSTANT(CERTIFICATE_UNOBTAINABLE);
#endif
#ifdef SSL_AD_UNRECOGNIZED_NAME
    ADD_AD_CONSTANT(UNRECOGNIZED_NAME);
#endif
#ifdef SSL_AD_BAD_CERTIFICATE_STATUS_RESPONSE
    ADD_AD_CONSTANT(BAD_CERTIFICATE_STATUS_RESPONSE);
#endif
#ifdef SSL_AD_BAD_CERTIFICATE_HASH_VALUE
    ADD_AD_CONSTANT(BAD_CERTIFICATE_HASH_VALUE);
#endif
#ifdef SSL_AD_UNKNOWN_PSK_IDENTITY
    ADD_AD_CONSTANT(UNKNOWN_PSK_IDENTITY);
#endif

#undef ADD_AD_CONSTANT

    /* protocol versions */
#ifndef OPENSSL_NO_SSL2
    PyModule_AddIntConstant(m, "PROTOCOL_SSLv2",
                            PY_SSL_VERSION_SSL2);
#endif
#ifndef OPENSSL_NO_SSL3
    PyModule_AddIntConstant(m, "PROTOCOL_SSLv3",
                            PY_SSL_VERSION_SSL3);
#endif
    PyModule_AddIntConstant(m, "PROTOCOL_SSLv23",
                            PY_SSL_VERSION_SSL23);
    PyModule_AddIntConstant(m, "PROTOCOL_TLSv1",
                            PY_SSL_VERSION_TLS1);
#if HAVE_TLSv1_2
    PyModule_AddIntConstant(m, "PROTOCOL_TLSv1_1",
                            PY_SSL_VERSION_TLS1_1);
    PyModule_AddIntConstant(m, "PROTOCOL_TLSv1_2",
                            PY_SSL_VERSION_TLS1_2);
#endif

    /* protocol options */
    PyModule_AddIntConstant(m, "OP_ALL",
                            SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
    PyModule_AddIntConstant(m, "OP_NO_SSLv2", SSL_OP_NO_SSLv2);
    PyModule_AddIntConstant(m, "OP_NO_SSLv3", SSL_OP_NO_SSLv3);
    PyModule_AddIntConstant(m, "OP_NO_TLSv1", SSL_OP_NO_TLSv1);
#if HAVE_TLSv1_2
    PyModule_AddIntConstant(m, "OP_NO_TLSv1_1", SSL_OP_NO_TLSv1_1);
    PyModule_AddIntConstant(m, "OP_NO_TLSv1_2", SSL_OP_NO_TLSv1_2);
#endif
    PyModule_AddIntConstant(m, "OP_CIPHER_SERVER_PREFERENCE",
                            SSL_OP_CIPHER_SERVER_PREFERENCE);
    PyModule_AddIntConstant(m, "OP_SINGLE_DH_USE", SSL_OP_SINGLE_DH_USE);
#ifdef SSL_OP_SINGLE_ECDH_USE
    PyModule_AddIntConstant(m, "OP_SINGLE_ECDH_USE", SSL_OP_SINGLE_ECDH_USE);
#endif
#ifdef SSL_OP_NO_COMPRESSION
    PyModule_AddIntConstant(m, "OP_NO_COMPRESSION",
                            SSL_OP_NO_COMPRESSION);
#endif

#if HAVE_SNI
    r = Py_True;
#else
    r = Py_False;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_SNI", r);

#if HAVE_OPENSSL_FINISHED
    r = Py_True;
#else
    r = Py_False;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_TLS_UNIQUE", r);

#ifdef OPENSSL_NO_ECDH
    r = Py_False;
#else
    r = Py_True;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_ECDH", r);

#ifdef OPENSSL_NPN_NEGOTIATED
    r = Py_True;
#else
    r = Py_False;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_NPN", r);

#ifdef HAVE_ALPN
    r = Py_True;
#else
    r = Py_False;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_ALPN", r);

    /* Mappings for error codes */
    err_codes_to_names = PyDict_New();
    err_names_to_codes = PyDict_New();
    if (err_codes_to_names == NULL || err_names_to_codes == NULL)
        return;
    errcode = error_codes;
    while (errcode->mnemonic != NULL) {
        PyObject *mnemo, *key;
        mnemo = PyUnicode_FromString(errcode->mnemonic);
        key = Py_BuildValue("ii", errcode->library, errcode->reason);
        if (mnemo == NULL || key == NULL)
            return;
        if (PyDict_SetItem(err_codes_to_names, key, mnemo))
            return;
        if (PyDict_SetItem(err_names_to_codes, mnemo, key))
            return;
        Py_DECREF(key);
        Py_DECREF(mnemo);
        errcode++;
    }
    if (PyModule_AddObject(m, "err_codes_to_names", err_codes_to_names))
        return;
    if (PyModule_AddObject(m, "err_names_to_codes", err_names_to_codes))
        return;

    lib_codes_to_names = PyDict_New();
    if (lib_codes_to_names == NULL)
        return;
    libcode = library_codes;
    while (libcode->library != NULL) {
        PyObject *mnemo, *key;
        key = PyLong_FromLong(libcode->code);
        mnemo = PyUnicode_FromString(libcode->library);
        if (key == NULL || mnemo == NULL)
            return;
        if (PyDict_SetItem(lib_codes_to_names, key, mnemo))
            return;
        Py_DECREF(key);
        Py_DECREF(mnemo);
        libcode++;
    }
    if (PyModule_AddObject(m, "lib_codes_to_names", lib_codes_to_names))
        return;

    /* OpenSSL version */
    /* SSLeay() gives us the version of the library linked against,
       which could be different from the headers version.
    */
    libver = SSLeay();
    r = PyLong_FromUnsignedLong(libver);
    if (r == NULL)
        return;
    if (PyModule_AddObject(m, "OPENSSL_VERSION_NUMBER", r))
        return;
    parse_openssl_version(libver, &major, &minor, &fix, &patch, &status);
    r = Py_BuildValue("IIIII", major, minor, fix, patch, status);
    if (r == NULL || PyModule_AddObject(m, "OPENSSL_VERSION_INFO", r))
        return;
    r = PyString_FromString(SSLeay_version(SSLEAY_VERSION));
    if (r == NULL || PyModule_AddObject(m, "OPENSSL_VERSION", r))
        return;

    libver = OPENSSL_VERSION_NUMBER;
    parse_openssl_version(libver, &major, &minor, &fix, &patch, &status);
    r = Py_BuildValue("IIIII", major, minor, fix, patch, status);
    if (r == NULL || PyModule_AddObject(m, "_OPENSSL_API_VERSION", r))
        return;
}
