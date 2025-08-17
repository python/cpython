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

/* Don't warn about deprecated functions, */
#ifndef OPENSSL_API_COMPAT
  // 0x10101000L == 1.1.1, 30000 == 3.0.0
  #define OPENSSL_API_COMPAT 0x10101000L
#endif
#define OPENSSL_NO_DEPRECATED 1

#define PY_SSIZE_T_CLEAN

#include "Python.h"

/* Include symbols from _socket module */
#include "socketmodule.h"

#include "_ssl.h"

/* Redefined below for Windows debug builds after important #includes */
#define _PySSL_FIX_ERRNO

#define PySSL_BEGIN_ALLOW_THREADS_S(save) \
    do { (save) = PyEval_SaveThread(); } while(0)
#define PySSL_END_ALLOW_THREADS_S(save) \
    do { PyEval_RestoreThread(save); _PySSL_FIX_ERRNO; } while(0)
#define PySSL_BEGIN_ALLOW_THREADS { \
            PyThreadState *_save = NULL;  \
            PySSL_BEGIN_ALLOW_THREADS_S(_save);
#define PySSL_END_ALLOW_THREADS PySSL_END_ALLOW_THREADS_S(_save); }


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
#include "openssl/bio.h"
#include "openssl/dh.h"

#ifndef OPENSSL_THREADS
#  error "OPENSSL_THREADS is not defined, Python requires thread-safe OpenSSL"
#endif



struct py_ssl_error_code {
    const char *mnemonic;
    int library, reason;
};

struct py_ssl_library_code {
    const char *library;
    int code;
};

#if defined(MS_WINDOWS) && defined(Py_DEBUG)
/* Debug builds on Windows rely on getting errno directly from OpenSSL.
 * However, because it uses a different CRT, we need to transfer the
 * value of errno from OpenSSL into our debug CRT.
 *
 * Don't be fooled - this is horribly ugly code. The only reasonable
 * alternative is to do both debug and release builds of OpenSSL, which
 * requires much uglier code to transform their automatically generated
 * makefile. This is the lesser of all the evils.
 */

static void _PySSLFixErrno(void) {
    HMODULE ucrtbase = GetModuleHandleW(L"ucrtbase.dll");
    if (!ucrtbase) {
        /* If ucrtbase.dll is not loaded but the SSL DLLs are, we likely
         * have a catastrophic failure, but this function is not the
         * place to raise it. */
        return;
    }

    typedef int *(__stdcall *errno_func)(void);
    errno_func ssl_errno = (errno_func)GetProcAddress(ucrtbase, "_errno");
    if (ssl_errno) {
        errno = *ssl_errno();
        *ssl_errno() = 0;
    } else {
        errno = ENOTRECOVERABLE;
    }
}

#undef _PySSL_FIX_ERRNO
#define _PySSL_FIX_ERRNO _PySSLFixErrno()
#endif

/* Include generated data (error codes) */
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
#include "_ssl_data_300.h"
#elif (OPENSSL_VERSION_NUMBER >= 0x10101000L) && !defined(LIBRESSL_VERSION_NUMBER)
#include "_ssl_data_111.h"
#else
#include "_ssl_data.h"
#endif

/* OpenSSL API 1.1.0+ does not include version methods */
#ifndef OPENSSL_NO_SSL3_METHOD
extern const SSL_METHOD *SSLv3_method(void);
#endif
#ifndef OPENSSL_NO_TLS1_METHOD
extern const SSL_METHOD *TLSv1_method(void);
#endif
#ifndef OPENSSL_NO_TLS1_1_METHOD
extern const SSL_METHOD *TLSv1_1_method(void);
#endif
#ifndef OPENSSL_NO_TLS1_2_METHOD
extern const SSL_METHOD *TLSv1_2_method(void);
#endif

#ifndef INVALID_SOCKET /* MS defines this */
#define INVALID_SOCKET (-1)
#endif

/* OpenSSL 1.1 does not have SSL 2.0 */
#define OPENSSL_NO_SSL2

/* Default cipher suites */
#ifndef PY_SSL_DEFAULT_CIPHERS
#define PY_SSL_DEFAULT_CIPHERS 1
#endif

#if PY_SSL_DEFAULT_CIPHERS == 0
  #ifndef PY_SSL_DEFAULT_CIPHER_STRING
     #error "Py_SSL_DEFAULT_CIPHERS 0 needs Py_SSL_DEFAULT_CIPHER_STRING"
  #endif
  #ifndef PY_SSL_MIN_PROTOCOL
    #define PY_SSL_MIN_PROTOCOL TLS1_2_VERSION
  #endif
#elif PY_SSL_DEFAULT_CIPHERS == 1
/* Python custom selection of sensible cipher suites
 * @SECLEVEL=2: security level 2 with 112 bits minimum security (e.g. 2048 bits RSA key)
 * ECDH+*: enable ephemeral elliptic curve Diffie-Hellman
 * DHE+*: fallback to ephemeral finite field Diffie-Hellman
 * encryption order: AES AEAD (GCM), ChaCha AEAD, AES CBC
 * !aNULL:!eNULL: really no NULL ciphers
 * !aDSS: no authentication with discrete logarithm DSA algorithm
 * !SHA1: no weak SHA1 MAC
 * !AESCCM: no CCM mode, it's uncommon and slow
 *
 * Based on Hynek's excellent blog post (update 2021-02-11)
 * https://hynek.me/articles/hardening-your-web-servers-ssl-ciphers/
 */
  #define PY_SSL_DEFAULT_CIPHER_STRING "@SECLEVEL=2:ECDH+AESGCM:ECDH+CHACHA20:ECDH+AES:DHE+AES:!aNULL:!eNULL:!aDSS:!SHA1:!AESCCM"
  #ifndef PY_SSL_MIN_PROTOCOL
    #define PY_SSL_MIN_PROTOCOL TLS1_2_VERSION
  #endif
#elif PY_SSL_DEFAULT_CIPHERS == 2
/* Ignored in SSLContext constructor, only used to as _ssl.DEFAULT_CIPHER_STRING */
  #define PY_SSL_DEFAULT_CIPHER_STRING SSL_DEFAULT_CIPHER_LIST
#else
  #error "Unsupported PY_SSL_DEFAULT_CIPHERS"
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
    PY_SSL_VERSION_TLS, /* SSLv23 */
    PY_SSL_VERSION_TLS1,
    PY_SSL_VERSION_TLS1_1,
    PY_SSL_VERSION_TLS1_2,
    PY_SSL_VERSION_TLS_CLIENT=0x10,
    PY_SSL_VERSION_TLS_SERVER,
};

enum py_proto_version {
    PY_PROTO_MINIMUM_SUPPORTED = -2,
    PY_PROTO_SSLv3 = SSL3_VERSION,
    PY_PROTO_TLSv1 = TLS1_VERSION,
    PY_PROTO_TLSv1_1 = TLS1_1_VERSION,
    PY_PROTO_TLSv1_2 = TLS1_2_VERSION,
#ifdef TLS1_3_VERSION
    PY_PROTO_TLSv1_3 = TLS1_3_VERSION,
#else
    PY_PROTO_TLSv1_3 = 0x304,
#endif
    PY_PROTO_MAXIMUM_SUPPORTED = -1,

/* OpenSSL has no dedicated API to set the minimum version to the maximum
 * available version, and the other way around. We have to figure out the
 * minimum and maximum available version on our own and hope for the best.
 */
#if defined(SSL3_VERSION) && !defined(OPENSSL_NO_SSL3)
    PY_PROTO_MINIMUM_AVAILABLE = PY_PROTO_SSLv3,
#elif defined(TLS1_VERSION) && !defined(OPENSSL_NO_TLS1)
    PY_PROTO_MINIMUM_AVAILABLE = PY_PROTO_TLSv1,
#elif defined(TLS1_1_VERSION) && !defined(OPENSSL_NO_TLS1_1)
    PY_PROTO_MINIMUM_AVAILABLE = PY_PROTO_TLSv1_1,
#elif defined(TLS1_2_VERSION) && !defined(OPENSSL_NO_TLS1_2)
    PY_PROTO_MINIMUM_AVAILABLE = PY_PROTO_TLSv1_2,
#elif defined(TLS1_3_VERSION) && !defined(OPENSSL_NO_TLS1_3)
    PY_PROTO_MINIMUM_AVAILABLE = PY_PROTO_TLSv1_3,
#else
    #error "PY_PROTO_MINIMUM_AVAILABLE not found"
#endif

#if defined(TLS1_3_VERSION) && !defined(OPENSSL_NO_TLS1_3)
    PY_PROTO_MAXIMUM_AVAILABLE = PY_PROTO_TLSv1_3,
#elif defined(TLS1_2_VERSION) && !defined(OPENSSL_NO_TLS1_2)
    PY_PROTO_MAXIMUM_AVAILABLE = PY_PROTO_TLSv1_2,
#elif defined(TLS1_1_VERSION) && !defined(OPENSSL_NO_TLS1_1)
    PY_PROTO_MAXIMUM_AVAILABLE = PY_PROTO_TLSv1_1,
#elif defined(TLS1_VERSION) && !defined(OPENSSL_NO_TLS1)
    PY_PROTO_MAXIMUM_AVAILABLE = PY_PROTO_TLSv1,
#elif defined(SSL3_VERSION) && !defined(OPENSSL_NO_SSL3)
    PY_PROTO_MAXIMUM_AVAILABLE = PY_PROTO_SSLv3,
#else
    #error "PY_PROTO_MAXIMUM_AVAILABLE not found"
#endif
};

/* SSL socket object */

#define X509_NAME_MAXLEN 256


/* In case of 'tls-unique' it will be 12 bytes for TLS, 36 bytes for
 * older SSL, but let's be safe */
#define PySSL_CB_MAXLEN 128


typedef struct {
    PyObject_HEAD
    SSL_CTX *ctx;
    unsigned char *alpn_protocols;
    unsigned int alpn_protocols_len;
    PyObject *set_sni_cb;
    int check_hostname;
    /* OpenSSL has no API to get hostflags from X509_VERIFY_PARAM* struct.
     * We have to maintain our own copy. OpenSSL's hostflags default to 0.
     */
    unsigned int hostflags;
    int protocol;
#ifdef TLS1_3_VERSION
    int post_handshake_auth;
#endif
    PyObject *msg_cb;
    PyObject *keylog_filename;
    BIO *keylog_bio;
    /* Cached module state, also used in SSLSocket and SSLSession code. */
    _sslmodulestate *state;
} PySSLContext;

typedef struct {
    int ssl; /* last seen error from SSL */
    int c; /* last seen error from libc */
#ifdef MS_WINDOWS
    int ws; /* last seen error from winsock */
#endif
} _PySSLError;

typedef struct {
    PyObject_HEAD
    PyObject *Socket; /* weakref to socket on which we're layered */
    SSL *ssl;
    PySSLContext *ctx; /* weakref to SSL context */
    char shutdown_seen_zero;
    enum py_ssl_server_or_client socket_type;
    PyObject *owner; /* Python level "owner" passed to servername callback */
    PyObject *server_hostname;
    _PySSLError err; /* last seen error from various sources */
    /* Some SSL callbacks don't have error reporting. Callback wrappers
     * store exception information on the socket. The handshake, read, write,
     * and shutdown methods check for chained exceptions.
     */
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_tb;
} PySSLSocket;

typedef struct {
    PyObject_HEAD
    BIO *bio;
    int eof_written;
} PySSLMemoryBIO;

typedef struct {
    PyObject_HEAD
    SSL_SESSION *session;
    PySSLContext *ctx;
} PySSLSession;

static inline _PySSLError _PySSL_errno(int failed, const SSL *ssl, int retcode)
{
    _PySSLError err = { 0 };
    if (failed) {
#ifdef MS_WINDOWS
        err.ws = WSAGetLastError();
        _PySSL_FIX_ERRNO;
#endif
        err.c = errno;
        err.ssl = SSL_get_error(ssl, retcode);
    }
    return err;
}

/*[clinic input]
module _ssl
class _ssl._SSLContext "PySSLContext *" "get_state_type(type)->PySSLContext_Type"
class _ssl._SSLSocket "PySSLSocket *" "get_state_type(type)->PySSLSocket_Type"
class _ssl.MemoryBIO "PySSLMemoryBIO *" "get_state_type(type)->PySSLMemoryBIO_Type"
class _ssl.SSLSession "PySSLSession *" "get_state_type(type)->PySSLSession_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d293bed8bae240fd]*/

#include "clinic/_ssl.c.h"

static int PySSL_select(PySocketSockObject *s, int writing, _PyTime_t timeout);

static int PySSL_set_owner(PySSLSocket *, PyObject *, void *);
static int PySSL_set_session(PySSLSocket *, PyObject *, void *);

typedef enum {
    SOCKET_IS_NONBLOCKING,
    SOCKET_IS_BLOCKING,
    SOCKET_HAS_TIMED_OUT,
    SOCKET_HAS_BEEN_CLOSED,
    SOCKET_TOO_LARGE_FOR_SELECT,
    SOCKET_OPERATION_OK
} timeout_state;

/* Wrap error strings with filename and line # */
#define ERRSTR1(x,y,z) (x ":" y ": " z)
#define ERRSTR(x) ERRSTR1("_ssl.c", Py_STRINGIFY(__LINE__), x)

/* Get the socket from a PySSLSocket, if it has one */
#define GET_SOCKET(obj) ((obj)->Socket ? \
    (PySocketSockObject *) PyWeakref_GetObject((obj)->Socket) : NULL)

/* If sock is NULL, use a timeout of 0 second */
#define GET_SOCKET_TIMEOUT(sock) \
    ((sock != NULL) ? (sock)->sock_timeout : 0)

#include "_ssl/debughelpers.c"

/*
 * SSL errors.
 */

PyDoc_STRVAR(SSLError_doc,
"An error occurred in the SSL implementation.");

PyDoc_STRVAR(SSLCertVerificationError_doc,
"A certificate could not be verified.");

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
SSLError_str(PyOSErrorObject *self)
{
    if (self->strerror != NULL && PyUnicode_Check(self->strerror)) {
        Py_INCREF(self->strerror);
        return self->strerror;
    }
    else
        return PyObject_Str(self->args);
}

static PyType_Slot sslerror_type_slots[] = {
    {Py_tp_doc, (void*)SSLError_doc},
    {Py_tp_str, SSLError_str},
    {0, 0},
};

static PyType_Spec sslerror_type_spec = {
    .name = "ssl.SSLError",
    .basicsize = sizeof(PyOSErrorObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = sslerror_type_slots
};

static void
fill_and_set_sslerror(_sslmodulestate *state,
                      PySSLSocket *sslsock, PyObject *type, int ssl_errno,
                      const char *errstr, int lineno, unsigned long errcode)
{
    PyObject *err_value = NULL, *reason_obj = NULL, *lib_obj = NULL;
    PyObject *verify_obj = NULL, *verify_code_obj = NULL;
    PyObject *init_value, *msg, *key;

    if (errcode != 0) {
        int lib, reason;

        lib = ERR_GET_LIB(errcode);
        reason = ERR_GET_REASON(errcode);
        key = Py_BuildValue("ii", lib, reason);
        if (key == NULL)
            goto fail;
        reason_obj = PyDict_GetItemWithError(state->err_codes_to_names, key);
        Py_DECREF(key);
        if (reason_obj == NULL && PyErr_Occurred()) {
            goto fail;
        }
        key = PyLong_FromLong(lib);
        if (key == NULL)
            goto fail;
        lib_obj = PyDict_GetItemWithError(state->lib_codes_to_names, key);
        Py_DECREF(key);
        if (lib_obj == NULL && PyErr_Occurred()) {
            goto fail;
        }
        if (errstr == NULL)
            errstr = ERR_reason_error_string(errcode);
    }
    if (errstr == NULL)
        errstr = "unknown error";

    /* verify code for cert validation error */
    if ((sslsock != NULL) && (type == state->PySSLCertVerificationErrorObject)) {
        const char *verify_str = NULL;
        long verify_code;

        verify_code = SSL_get_verify_result(sslsock->ssl);
        verify_code_obj = PyLong_FromLong(verify_code);
        if (verify_code_obj == NULL) {
            goto fail;
        }

        switch (verify_code) {
        case X509_V_ERR_HOSTNAME_MISMATCH:
            verify_obj = PyUnicode_FromFormat(
                "Hostname mismatch, certificate is not valid for '%S'.",
                sslsock->server_hostname
            );
            break;
        case X509_V_ERR_IP_ADDRESS_MISMATCH:
            verify_obj = PyUnicode_FromFormat(
                "IP address mismatch, certificate is not valid for '%S'.",
                sslsock->server_hostname
            );
            break;
        default:
            verify_str = X509_verify_cert_error_string(verify_code);
            if (verify_str != NULL) {
                verify_obj = PyUnicode_FromString(verify_str);
            } else {
                verify_obj = Py_None;
                Py_INCREF(verify_obj);
            }
            break;
        }
        if (verify_obj == NULL) {
            goto fail;
        }
    }

    if (verify_obj && reason_obj && lib_obj)
        msg = PyUnicode_FromFormat("[%S: %S] %s: %S (_ssl.c:%d)",
                                   lib_obj, reason_obj, errstr, verify_obj,
                                   lineno);
    else if (reason_obj && lib_obj)
        msg = PyUnicode_FromFormat("[%S: %S] %s (_ssl.c:%d)",
                                   lib_obj, reason_obj, errstr, lineno);
    else if (lib_obj)
        msg = PyUnicode_FromFormat("[%S] %s (_ssl.c:%d)",
                                   lib_obj, errstr, lineno);
    else
        msg = PyUnicode_FromFormat("%s (_ssl.c:%d)", errstr, lineno);
    if (msg == NULL)
        goto fail;

    init_value = Py_BuildValue("iN", ERR_GET_REASON(ssl_errno), msg);
    if (init_value == NULL)
        goto fail;

    err_value = PyObject_CallObject(type, init_value);
    Py_DECREF(init_value);
    if (err_value == NULL)
        goto fail;

    if (reason_obj == NULL)
        reason_obj = Py_None;
    if (PyObject_SetAttr(err_value, state->str_reason, reason_obj))
        goto fail;

    if (lib_obj == NULL)
        lib_obj = Py_None;
    if (PyObject_SetAttr(err_value, state->str_library, lib_obj))
        goto fail;

    if ((sslsock != NULL) && (type == state->PySSLCertVerificationErrorObject)) {
        /* Only set verify code / message for SSLCertVerificationError */
        if (PyObject_SetAttr(err_value, state->str_verify_code,
                                verify_code_obj))
            goto fail;
        if (PyObject_SetAttr(err_value, state->str_verify_message, verify_obj))
            goto fail;
    }

    PyErr_SetObject(type, err_value);
fail:
    Py_XDECREF(err_value);
    Py_XDECREF(verify_code_obj);
    Py_XDECREF(verify_obj);
}

static int
PySSL_ChainExceptions(PySSLSocket *sslsock) {
    if (sslsock->exc_type == NULL)
        return 0;

    _PyErr_ChainExceptions(sslsock->exc_type, sslsock->exc_value, sslsock->exc_tb);
    sslsock->exc_type = NULL;
    sslsock->exc_value = NULL;
    sslsock->exc_tb = NULL;
    return -1;
}

static PyObject *
PySSL_SetError(PySSLSocket *sslsock, int ret, const char *filename, int lineno)
{
    PyObject *type;
    char *errstr = NULL;
    _PySSLError err;
    enum py_ssl_error p = PY_SSL_ERROR_NONE;
    unsigned long e = 0;

    assert(sslsock != NULL);

    _sslmodulestate *state = get_state_sock(sslsock);
    type = state->PySSLErrorObject;

    assert(ret <= 0);
    e = ERR_peek_last_error();

    if (sslsock->ssl != NULL) {
        err = sslsock->err;

        switch (err.ssl) {
        case SSL_ERROR_ZERO_RETURN:
            errstr = "TLS/SSL connection has been closed (EOF)";
            type = state->PySSLZeroReturnErrorObject;
            p = PY_SSL_ERROR_ZERO_RETURN;
            break;
        case SSL_ERROR_WANT_READ:
            errstr = "The operation did not complete (read)";
            type = state->PySSLWantReadErrorObject;
            p = PY_SSL_ERROR_WANT_READ;
            break;
        case SSL_ERROR_WANT_WRITE:
            p = PY_SSL_ERROR_WANT_WRITE;
            type = state->PySSLWantWriteErrorObject;
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
                PySocketSockObject *s = GET_SOCKET(sslsock);
                if (ret == 0 || (((PyObject *)s) == Py_None)) {
                    p = PY_SSL_ERROR_EOF;
                    type = state->PySSLEOFErrorObject;
                    errstr = "EOF occurred in violation of protocol";
                } else if (s && ret == -1) {
                    /* underlying BIO reported an I/O error */
                    ERR_clear_error();
#ifdef MS_WINDOWS
                    if (err.ws) {
                        return PyErr_SetFromWindowsErr(err.ws);
                    }
#endif
                    if (err.c) {
                        errno = err.c;
                        return PyErr_SetFromErrno(PyExc_OSError);
                    }
                    else {
                        p = PY_SSL_ERROR_EOF;
                        type = state->PySSLEOFErrorObject;
                        errstr = "EOF occurred in violation of protocol";
                    }
                } else { /* possible? */
                    p = PY_SSL_ERROR_SYSCALL;
                    type = state->PySSLSyscallErrorObject;
                    errstr = "Some I/O error occurred";
                }
            } else {
                if (ERR_GET_LIB(e) == ERR_LIB_SSL &&
                        ERR_GET_REASON(e) == SSL_R_CERTIFICATE_VERIFY_FAILED) {
                    type = state->PySSLCertVerificationErrorObject;
                }
                if (ERR_GET_LIB(e) == ERR_LIB_SYS) {
                    // A system error is being reported; reason is set to errno
                    errno = ERR_GET_REASON(e);
                    return PyErr_SetFromErrno(PyExc_OSError);
                }
                p = PY_SSL_ERROR_SYSCALL;
            }
            break;
        }
        case SSL_ERROR_SSL:
        {
            p = PY_SSL_ERROR_SSL;
            if (e == 0) {
                /* possible? */
                errstr = "A failure in the SSL library occurred";
            }
            if (ERR_GET_LIB(e) == ERR_LIB_SSL &&
                    ERR_GET_REASON(e) == SSL_R_CERTIFICATE_VERIFY_FAILED) {
                type = state->PySSLCertVerificationErrorObject;
            }
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
            /* OpenSSL 3.0 changed transport EOF from SSL_ERROR_SYSCALL with
             * zero return value to SSL_ERROR_SSL with a special error code. */
            if (ERR_GET_LIB(e) == ERR_LIB_SSL &&
                    ERR_GET_REASON(e) == SSL_R_UNEXPECTED_EOF_WHILE_READING) {
                p = PY_SSL_ERROR_EOF;
                type = state->PySSLEOFErrorObject;
                errstr = "EOF occurred in violation of protocol";
            }
#endif
            if (ERR_GET_LIB(e) == ERR_LIB_SYS) {
                // A system error is being reported; reason is set to errno
                errno = ERR_GET_REASON(e);
                return PyErr_SetFromErrno(PyExc_OSError);
            }
            break;
        }
        default:
            p = PY_SSL_ERROR_INVALID_ERROR_CODE;
            errstr = "Invalid error code";
        }
    }
    fill_and_set_sslerror(state, sslsock, type, p, errstr, lineno, e);
    ERR_clear_error();
    PySSL_ChainExceptions(sslsock);
    return NULL;
}

static PyObject *
_setSSLError (_sslmodulestate *state, const char *errstr, int errcode, const char *filename, int lineno)
{
    if (errstr == NULL)
        errcode = ERR_peek_last_error();
    else
        errcode = 0;
    fill_and_set_sslerror(state, NULL, state->PySSLErrorObject, errcode, errstr, lineno, errcode);
    ERR_clear_error();
    return NULL;
}

static int
_ssl_deprecated(const char* msg, int stacklevel) {
    return PyErr_WarnEx(
        PyExc_DeprecationWarning, msg, stacklevel
    );
}

#define PY_SSL_DEPRECATED(name, stacklevel, ret) \
    if (_ssl_deprecated((name), (stacklevel)) == -1) return (ret)

/*
 * SSL objects
 */

static int
_ssl_configure_hostname(PySSLSocket *self, const char* server_hostname)
{
    int retval = -1;
    ASN1_OCTET_STRING *ip;
    PyObject *hostname;
    size_t len;

    assert(server_hostname);

    /* Disable OpenSSL's special mode with leading dot in hostname:
     * When name starts with a dot (e.g ".example.com"), it will be
     * matched by a certificate valid for any sub-domain of name.
     */
    len = strlen(server_hostname);
    if (len == 0 || *server_hostname == '.') {
        PyErr_SetString(
            PyExc_ValueError,
            "server_hostname cannot be an empty string or start with a "
            "leading dot.");
        return retval;
    }

    /* inet_pton is not available on all platforms. */
    ip = a2i_IPADDRESS(server_hostname);
    if (ip == NULL) {
        ERR_clear_error();
    }

    hostname = PyUnicode_Decode(server_hostname, len, "ascii", "strict");
    if (hostname == NULL) {
        goto error;
    }
    self->server_hostname = hostname;

    /* Only send SNI extension for non-IP hostnames */
    if (ip == NULL) {
        if (!SSL_set_tlsext_host_name(self->ssl, server_hostname)) {
            _setSSLError(get_state_sock(self), NULL, 0, __FILE__, __LINE__);
            goto error;
        }
    }
    if (self->ctx->check_hostname) {
        X509_VERIFY_PARAM *param = SSL_get0_param(self->ssl);
        if (ip == NULL) {
            if (!X509_VERIFY_PARAM_set1_host(param, server_hostname,
                                             strlen(server_hostname))) {
                _setSSLError(get_state_sock(self), NULL, 0, __FILE__, __LINE__);
                goto error;
            }
        } else {
            if (!X509_VERIFY_PARAM_set1_ip(param, ASN1_STRING_get0_data(ip),
                                           ASN1_STRING_length(ip))) {
                _setSSLError(get_state_sock(self), NULL, 0, __FILE__, __LINE__);
                goto error;
            }
        }
    }
    retval = 0;
  error:
    if (ip != NULL) {
        ASN1_OCTET_STRING_free(ip);
    }
    return retval;
}

static PySSLSocket *
newPySSLSocket(PySSLContext *sslctx, PySocketSockObject *sock,
               enum py_ssl_server_or_client socket_type,
               char *server_hostname,
               PyObject *owner, PyObject *session,
               PySSLMemoryBIO *inbio, PySSLMemoryBIO *outbio)
{
    PySSLSocket *self;
    SSL_CTX *ctx = sslctx->ctx;
    _PySSLError err = { 0 };

    if ((socket_type == PY_SSL_SERVER) &&
        (sslctx->protocol == PY_SSL_VERSION_TLS_CLIENT)) {
        _setSSLError(get_state_ctx(sslctx),
                     "Cannot create a server socket with a "
                     "PROTOCOL_TLS_CLIENT context", 0, __FILE__, __LINE__);
        return NULL;
    }
    if ((socket_type == PY_SSL_CLIENT) &&
        (sslctx->protocol == PY_SSL_VERSION_TLS_SERVER)) {
        _setSSLError(get_state_ctx(sslctx),
                     "Cannot create a client socket with a "
                     "PROTOCOL_TLS_SERVER context", 0, __FILE__, __LINE__);
        return NULL;
    }

    self = PyObject_GC_New(PySSLSocket,
                           get_state_ctx(sslctx)->PySSLSocket_Type);
    if (self == NULL)
        return NULL;

    self->ssl = NULL;
    self->Socket = NULL;
    self->ctx = sslctx;
    Py_INCREF(sslctx);
    self->shutdown_seen_zero = 0;
    self->owner = NULL;
    self->server_hostname = NULL;
    self->err = err;
    self->exc_type = NULL;
    self->exc_value = NULL;
    self->exc_tb = NULL;

    /* Make sure the SSL error state is initialized */
    ERR_clear_error();

    PySSL_BEGIN_ALLOW_THREADS
    self->ssl = SSL_new(ctx);
    PySSL_END_ALLOW_THREADS
    if (self->ssl == NULL) {
        Py_DECREF(self);
        _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    /* bpo43522 and OpenSSL < 1.1.1l: copy hostflags manually */
#if !defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION < 0x101010cf
    X509_VERIFY_PARAM *ssl_params = SSL_get0_param(self->ssl);
    X509_VERIFY_PARAM_set_hostflags(ssl_params, sslctx->hostflags);
#endif
    SSL_set_app_data(self->ssl, self);
    if (sock) {
        SSL_set_fd(self->ssl, Py_SAFE_DOWNCAST(sock->sock_fd, SOCKET_T, int));
    } else {
        /* BIOs are reference counted and SSL_set_bio borrows our reference.
         * To prevent a double free in memory_bio_dealloc() we need to take an
         * extra reference here. */
        BIO_up_ref(inbio->bio);
        BIO_up_ref(outbio->bio);
        SSL_set_bio(self->ssl, inbio->bio, outbio->bio);
    }
    SSL_set_mode(self->ssl,
                 SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER | SSL_MODE_AUTO_RETRY);

#ifdef TLS1_3_VERSION
    if (sslctx->post_handshake_auth == 1) {
        if (socket_type == PY_SSL_SERVER) {
            /* bpo-37428: OpenSSL does not ignore SSL_VERIFY_POST_HANDSHAKE.
             * Set SSL_VERIFY_POST_HANDSHAKE flag only for server sockets and
             * only in combination with SSL_VERIFY_PEER flag. */
            int mode = SSL_get_verify_mode(self->ssl);
            if (mode & SSL_VERIFY_PEER) {
                int (*verify_cb)(int, X509_STORE_CTX *) = NULL;
                verify_cb = SSL_get_verify_callback(self->ssl);
                mode |= SSL_VERIFY_POST_HANDSHAKE;
                SSL_set_verify(self->ssl, mode, verify_cb);
            }
        } else {
            /* client socket */
            SSL_set_post_handshake_auth(self->ssl, 1);
        }
    }
#endif

    if (server_hostname != NULL) {
        if (_ssl_configure_hostname(self, server_hostname) < 0) {
            Py_DECREF(self);
            return NULL;
        }
    }
    /* If the socket is in non-blocking mode or timeout mode, set the BIO
     * to non-blocking mode (blocking is the default)
     */
    if (sock && sock->sock_timeout >= 0) {
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
    if (sock != NULL) {
        self->Socket = PyWeakref_NewRef((PyObject *) sock, NULL);
        if (self->Socket == NULL) {
            Py_DECREF(self);
            return NULL;
        }
    }
    if (owner && owner != Py_None) {
        if (PySSL_set_owner(self, owner, NULL) == -1) {
            Py_DECREF(self);
            return NULL;
        }
    }
    if (session && session != Py_None) {
        if (PySSL_set_session(self, session, NULL) == -1) {
            Py_DECREF(self);
            return NULL;
        }
    }

    PyObject_GC_Track(self);
    return self;
}

/* SSL object methods */

/*[clinic input]
_ssl._SSLSocket.do_handshake
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_do_handshake_impl(PySSLSocket *self)
/*[clinic end generated code: output=6c0898a8936548f6 input=d2d737de3df018c8]*/
{
    int ret;
    _PySSLError err;
    int sockstate, nonblocking;
    PySocketSockObject *sock = GET_SOCKET(self);
    _PyTime_t timeout, deadline = 0;
    int has_timeout;

    if (sock) {
        if (((PyObject*)sock) == Py_None) {
            _setSSLError(get_state_sock(self),
                         "Underlying socket connection gone",
                         PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
            return NULL;
        }
        Py_INCREF(sock);

        /* just in case the blocking state of the socket has been changed */
        nonblocking = (sock->sock_timeout >= 0);
        BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
        BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);
    }

    timeout = GET_SOCKET_TIMEOUT(sock);
    has_timeout = (timeout > 0);
    if (has_timeout) {
        deadline = _PyDeadline_Init(timeout);
    }

    /* Actually negotiate SSL connection */
    /* XXX If SSL_do_handshake() returns 0, it's also a failure. */
    do {
        PySSL_BEGIN_ALLOW_THREADS
        ret = SSL_do_handshake(self->ssl);
        err = _PySSL_errno(ret < 1, self->ssl, ret);
        PySSL_END_ALLOW_THREADS
        self->err = err;

        if (PyErr_CheckSignals())
            goto error;

        if (has_timeout)
            timeout = _PyDeadline_Get(deadline);

        if (err.ssl == SSL_ERROR_WANT_READ) {
            sockstate = PySSL_select(sock, 0, timeout);
        } else if (err.ssl == SSL_ERROR_WANT_WRITE) {
            sockstate = PySSL_select(sock, 1, timeout);
        } else {
            sockstate = SOCKET_OPERATION_OK;
        }

        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PyExc_TimeoutError,
                            ERRSTR("The handshake operation timed out"));
            goto error;
        } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
            PyErr_SetString(get_state_sock(self)->PySSLErrorObject,
                            ERRSTR("Underlying socket has been closed."));
            goto error;
        } else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
            PyErr_SetString(get_state_sock(self)->PySSLErrorObject,
                            ERRSTR("Underlying socket too large for select()."));
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err.ssl == SSL_ERROR_WANT_READ ||
             err.ssl == SSL_ERROR_WANT_WRITE);
    Py_XDECREF(sock);
    if (ret < 1)
        return PySSL_SetError(self, ret, __FILE__, __LINE__);
    if (PySSL_ChainExceptions(self) < 0)
        return NULL;
    Py_RETURN_NONE;
error:
    Py_XDECREF(sock);
    PySSL_ChainExceptions(self);
    return NULL;
}

static PyObject *
_asn1obj2py(_sslmodulestate *state, const ASN1_OBJECT *name, int no_name)
{
    char buf[X509_NAME_MAXLEN];
    char *namebuf = buf;
    int buflen;
    PyObject *name_obj = NULL;

    buflen = OBJ_obj2txt(namebuf, X509_NAME_MAXLEN, name, no_name);
    if (buflen < 0) {
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    /* initial buffer is too small for oid + terminating null byte */
    if (buflen > X509_NAME_MAXLEN - 1) {
        /* make OBJ_obj2txt() calculate the required buflen */
        buflen = OBJ_obj2txt(NULL, 0, name, no_name);
        /* allocate len + 1 for terminating NULL byte */
        namebuf = PyMem_Malloc(buflen + 1);
        if (namebuf == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        buflen = OBJ_obj2txt(namebuf, buflen + 1, name, no_name);
        if (buflen < 0) {
            _setSSLError(state, NULL, 0, __FILE__, __LINE__);
            goto done;
        }
    }
    if (!buflen && no_name) {
        Py_INCREF(Py_None);
        name_obj = Py_None;
    }
    else {
        name_obj = PyUnicode_FromStringAndSize(namebuf, buflen);
    }

  done:
    if (buf != namebuf) {
        PyMem_Free(namebuf);
    }
    return name_obj;
}

static PyObject *
_create_tuple_for_attribute(_sslmodulestate *state,
                            ASN1_OBJECT *name, ASN1_STRING *value)
{
    Py_ssize_t buflen;
    PyObject *pyattr;
    PyObject *pyname = _asn1obj2py(state, name, 0);

    if (pyname == NULL) {
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);
        return NULL;
    }

    if (ASN1_STRING_type(value) == V_ASN1_BIT_STRING) {
        buflen = ASN1_STRING_length(value);
        pyattr = Py_BuildValue("Ny#", pyname, ASN1_STRING_get0_data(value), buflen);
    } else {
        unsigned char *valuebuf = NULL;
        buflen = ASN1_STRING_to_UTF8(&valuebuf, value);
        if (buflen < 0) {
            _setSSLError(state, NULL, 0, __FILE__, __LINE__);
            Py_DECREF(pyname);
            return NULL;
        }
        pyattr = Py_BuildValue("Ns#", pyname, valuebuf, buflen);
        OPENSSL_free(valuebuf);
    }
    return pyattr;
}

static PyObject *
_create_tuple_for_X509_NAME (_sslmodulestate *state, X509_NAME *xname)
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
            if (rdn_level != X509_NAME_ENTRY_set(entry)) {
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
        rdn_level = X509_NAME_ENTRY_set(entry);

        /* now add this attribute to the current RDN */
        name = X509_NAME_ENTRY_get_object(entry);
        value = X509_NAME_ENTRY_get_data(entry);
        attr = _create_tuple_for_attribute(state, name, value);
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
_get_peer_alt_names (_sslmodulestate *state, X509 *certificate) {

    /* this code follows the procedure outlined in
       OpenSSL's crypto/x509v3/v3_prn.c:X509v3_EXT_print()
       function to extract the STACK_OF(GENERAL_NAME),
       then iterates through the stack to add the
       names. */

    int j;
    PyObject *peer_alt_names = Py_None;
    PyObject *v = NULL, *t;
    GENERAL_NAMES *names = NULL;
    GENERAL_NAME *name;
    BIO *biobuf = NULL;
    char buf[2048];
    char *vptr;
    int len;

    if (certificate == NULL)
        return peer_alt_names;

    /* get a memory buffer */
    biobuf = BIO_new(BIO_s_mem());
    if (biobuf == NULL) {
        PyErr_SetString(state->PySSLErrorObject, "failed to allocate BIO");
        return NULL;
    }

    names = (GENERAL_NAMES *)X509_get_ext_d2i(
        certificate, NID_subject_alt_name, NULL, NULL);
    if (names != NULL) {
        if (peer_alt_names == Py_None) {
            peer_alt_names = PyList_New(0);
            if (peer_alt_names == NULL)
                goto fail;
        }

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

                v = PyUnicode_FromString("DirName");
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);

                v = _create_tuple_for_X509_NAME(state, name->d.dirn);
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
                    v = PyUnicode_FromString("email");
                    as = name->d.rfc822Name;
                    break;
                case GEN_DNS:
                    v = PyUnicode_FromString("DNS");
                    as = name->d.dNSName;
                    break;
                case GEN_URI:
                    v = PyUnicode_FromString("URI");
                    as = name->d.uniformResourceIdentifier;
                    break;
                }
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);
                v = PyUnicode_FromStringAndSize((char *)ASN1_STRING_get0_data(as),
                                                ASN1_STRING_length(as));
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 1, v);
                break;

            case GEN_RID:
                t = PyTuple_New(2);
                if (t == NULL)
                    goto fail;

                v = PyUnicode_FromString("Registered ID");
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);

                len = i2t_ASN1_OBJECT(buf, sizeof(buf)-1, name->d.rid);
                if (len < 0) {
                    Py_DECREF(t);
                    _setSSLError(state, NULL, 0, __FILE__, __LINE__);
                    goto fail;
                } else if (len >= (int)sizeof(buf)) {
                    v = PyUnicode_FromString("<INVALID>");
                } else {
                    v = PyUnicode_FromStringAndSize(buf, len);
                }
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 1, v);
                break;

            case GEN_IPADD:
                /* OpenSSL < 3.0.0 adds a trailing \n to IPv6. 3.0.0 removed
                 * the trailing newline. Remove it in all versions
                 */
                t = PyTuple_New(2);
                if (t == NULL)
                    goto fail;

                v = PyUnicode_FromString("IP Address");
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);

                if (name->d.ip->length == 4) {
                    unsigned char *p = name->d.ip->data;
                    v = PyUnicode_FromFormat(
                        "%d.%d.%d.%d",
                        p[0], p[1], p[2], p[3]
                    );
                } else if (name->d.ip->length == 16) {
                    /* PyUnicode_FromFormat() does not support %X */
                    unsigned char *p = name->d.ip->data;
                    len = sprintf(
                        buf,
                        "%X:%X:%X:%X:%X:%X:%X:%X",
                        p[0] << 8 | p[1],
                        p[2] << 8 | p[3],
                        p[4] << 8 | p[5],
                        p[6] << 8 | p[7],
                        p[8] << 8 | p[9],
                        p[10] << 8 | p[11],
                        p[12] << 8 | p[13],
                        p[14] << 8 | p[15]
                    );
                    v = PyUnicode_FromStringAndSize(buf, len);
                } else {
                    v = PyUnicode_FromString("<invalid>");
                }

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
                    case GEN_RID:
                        break;
                    default:
                        if (PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
                                             "Unknown general name type %d",
                                             gntype) == -1) {
                            goto fail;
                        }
                        break;
                }
                (void) BIO_reset(biobuf);
                GENERAL_NAME_print(biobuf, name);
                len = BIO_gets(biobuf, buf, sizeof(buf)-1);
                if (len < 0) {
                    _setSSLError(state, NULL, 0, __FILE__, __LINE__);
                    goto fail;
                }
                vptr = strchr(buf, ':');
                if (vptr == NULL) {
                    PyErr_Format(PyExc_ValueError,
                                 "Invalid value %.200s",
                                 buf);
                    goto fail;
                }
                t = PyTuple_New(2);
                if (t == NULL)
                    goto fail;
                v = PyUnicode_FromStringAndSize(buf, (vptr - buf));
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);
                v = PyUnicode_FromStringAndSize((vptr + 1),
                                                (len - (vptr - buf + 1)));
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

    dps = X509_get_ext_d2i(certificate, NID_crl_distribution_points, NULL, NULL);

    if (dps == NULL)
        return Py_None;

    lst = PyList_New(0);
    if (lst == NULL)
        goto done;

    for (i=0; i < sk_DIST_POINT_num(dps); i++) {
        DIST_POINT *dp;
        STACK_OF(GENERAL_NAME) *gns;

        dp = sk_DIST_POINT_value(dps, i);
        if (dp->distpoint == NULL) {
            /* Ignore empty DP value, CVE-2019-5010 */
            continue;
        }
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
    CRL_DIST_POINTS_free(dps);
    return res;
}

static PyObject *
_decode_certificate(_sslmodulestate *state, X509 *certificate) {

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
    const ASN1_TIME *notBefore, *notAfter;
    PyObject *pnotBefore, *pnotAfter;

    retval = PyDict_New();
    if (retval == NULL)
        return NULL;

    peer = _create_tuple_for_X509_NAME(
        state,
        X509_get_subject_name(certificate));
    if (peer == NULL)
        goto fail0;
    if (PyDict_SetItemString(retval, (const char *) "subject", peer) < 0) {
        Py_DECREF(peer);
        goto fail0;
    }
    Py_DECREF(peer);

    issuer = _create_tuple_for_X509_NAME(
        state,
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
    if (biobuf == NULL) {
        PyErr_SetString(state->PySSLErrorObject, "failed to allocate BIO");
        goto fail0;
    }

    (void) BIO_reset(biobuf);
    serialNumber = X509_get_serialNumber(certificate);
    /* should not exceed 20 octets, 160 bits, so buf is big enough */
    i2a_ASN1_INTEGER(biobuf, serialNumber);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);
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
    notBefore = X509_get0_notBefore(certificate);
    ASN1_TIME_print(biobuf, notBefore);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);
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
    notAfter = X509_get0_notAfter(certificate);
    ASN1_TIME_print(biobuf, notAfter);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);
        goto fail1;
    }
    pnotAfter = PyUnicode_FromStringAndSize(buf, len);
    if (pnotAfter == NULL)
        goto fail1;
    if (PyDict_SetItemString(retval, "notAfter", pnotAfter) < 0) {
        Py_DECREF(pnotAfter);
        goto fail1;
    }
    Py_DECREF(pnotAfter);

    /* Now look for subjectAltName */

    peer_alt_names = _get_peer_alt_names(state, certificate);
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
_certificate_to_der(_sslmodulestate *state, X509 *certificate)
{
    unsigned char *bytes_buf = NULL;
    int len;
    PyObject *retval;

    bytes_buf = NULL;
    len = i2d_X509(certificate, &bytes_buf);
    if (len < 0) {
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    /* this is actually an immutable bytes sequence */
    retval = PyBytes_FromStringAndSize((const char *) bytes_buf, len);
    OPENSSL_free(bytes_buf);
    return retval;
}

#include "_ssl/misc.c"
#include "_ssl/cert.c"

/*[clinic input]
_ssl._test_decode_cert
    path: object(converter="PyUnicode_FSConverter")
    /

[clinic start generated code]*/

static PyObject *
_ssl__test_decode_cert_impl(PyObject *module, PyObject *path)
/*[clinic end generated code: output=96becb9abb23c091 input=cdeaaf02d4346628]*/
{
    PyObject *retval = NULL;
    X509 *x=NULL;
    BIO *cert;
    _sslmodulestate *state = get_ssl_state(module);

    if ((cert=BIO_new(BIO_s_file())) == NULL) {
        PyErr_SetString(state->PySSLErrorObject,
                        "Can't malloc memory to read file");
        goto fail0;
    }

    if (BIO_read_filename(cert, PyBytes_AsString(path)) <= 0) {
        PyErr_SetString(state->PySSLErrorObject,
                        "Can't open file");
        goto fail0;
    }

    x = PEM_read_bio_X509(cert, NULL, NULL, NULL);
    if (x == NULL) {
        PyErr_SetString(state->PySSLErrorObject,
                        "Error decoding PEM-encoded file");
        goto fail0;
    }

    retval = _decode_certificate(state, x);
    X509_free(x);

  fail0:
    Py_DECREF(path);
    if (cert != NULL) BIO_free(cert);
    return retval;
}


/*[clinic input]
_ssl._SSLSocket.getpeercert
    der as binary_mode: bool = False
    /

Returns the certificate for the peer.

If no certificate was provided, returns None.  If a certificate was
provided, but not validated, returns an empty dictionary.  Otherwise
returns a dict containing information about the peer certificate.

If the optional argument is True, returns a DER-encoded copy of the
peer certificate, or None if no certificate was provided.  This will
return the certificate even if it wasn't validated.
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_getpeercert_impl(PySSLSocket *self, int binary_mode)
/*[clinic end generated code: output=1f0ab66dfb693c88 input=c0fbe802e57629b7]*/
{
    int verification;
    X509 *peer_cert;
    PyObject *result;

    if (!SSL_is_init_finished(self->ssl)) {
        PyErr_SetString(PyExc_ValueError,
                        "handshake not done yet");
        return NULL;
    }
    peer_cert = SSL_get_peer_certificate(self->ssl);
    if (peer_cert == NULL)
        Py_RETURN_NONE;

    if (binary_mode) {
        /* return cert in DER-encoded format */
        result = _certificate_to_der(get_state_sock(self), peer_cert);
    } else {
        verification = SSL_CTX_get_verify_mode(SSL_get_SSL_CTX(self->ssl));
        if ((verification & SSL_VERIFY_PEER) == 0)
            result = PyDict_New();
        else
            result = _decode_certificate(get_state_sock(self), peer_cert);
    }
    X509_free(peer_cert);
    return result;
}

/*[clinic input]
_ssl._SSLSocket.get_verified_chain

[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_get_verified_chain_impl(PySSLSocket *self)
/*[clinic end generated code: output=802421163cdc3110 input=5fb0714f77e2bd51]*/
{
    /* borrowed reference */
    STACK_OF(X509) *chain = SSL_get0_verified_chain(self->ssl);
    if (chain == NULL) {
        Py_RETURN_NONE;
    }
    return _PySSL_CertificateFromX509Stack(self->ctx->state, chain, 1);
}

/*[clinic input]
_ssl._SSLSocket.get_unverified_chain

[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_get_unverified_chain_impl(PySSLSocket *self)
/*[clinic end generated code: output=5acdae414e13f913 input=78c33c360c635cb5]*/
{
    PyObject *retval;
    /* borrowed reference */
    /* TODO: include SSL_get_peer_certificate() for server-side sockets */
    STACK_OF(X509) *chain = SSL_get_peer_cert_chain(self->ssl);
    if (chain == NULL) {
        Py_RETURN_NONE;
    }
    retval = _PySSL_CertificateFromX509Stack(self->ctx->state, chain, 1);
    if (retval == NULL) {
        return NULL;
    }
    /* OpenSSL does not include peer cert for server side connections */
    if (self->socket_type == PY_SSL_SERVER) {
        PyObject *peerobj = NULL;
        X509 *peer = SSL_get_peer_certificate(self->ssl);

        if (peer == NULL) {
            peerobj = Py_None;
            Py_INCREF(peerobj);
        } else {
            /* consume X509 reference on success */
            peerobj = _PySSL_CertificateFromX509(self->ctx->state, peer, 0);
            if (peerobj == NULL) {
                X509_free(peer);
                Py_DECREF(retval);
                return NULL;
            }
        }
        int res = PyList_Insert(retval, 0, peerobj);
        Py_DECREF(peerobj);
        if (res < 0) {
            Py_DECREF(retval);
            return NULL;
        }
    }
    return retval;
}

static PyObject *
cipher_to_tuple(const SSL_CIPHER *cipher)
{
    const char *cipher_name, *cipher_protocol;
    PyObject *v, *retval = PyTuple_New(3);
    if (retval == NULL)
        return NULL;

    cipher_name = SSL_CIPHER_get_name(cipher);
    if (cipher_name == NULL) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(retval, 0, Py_None);
    } else {
        v = PyUnicode_FromString(cipher_name);
        if (v == NULL)
            goto fail;
        PyTuple_SET_ITEM(retval, 0, v);
    }

    cipher_protocol = SSL_CIPHER_get_version(cipher);
    if (cipher_protocol == NULL) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(retval, 1, Py_None);
    } else {
        v = PyUnicode_FromString(cipher_protocol);
        if (v == NULL)
            goto fail;
        PyTuple_SET_ITEM(retval, 1, v);
    }

    v = PyLong_FromLong(SSL_CIPHER_get_bits(cipher, NULL));
    if (v == NULL)
        goto fail;
    PyTuple_SET_ITEM(retval, 2, v);

    return retval;

  fail:
    Py_DECREF(retval);
    return NULL;
}

static PyObject *
cipher_to_dict(const SSL_CIPHER *cipher)
{
    const char *cipher_name, *cipher_protocol;

    unsigned long cipher_id;
    int alg_bits, strength_bits, len;
    char buf[512] = {0};
    int aead, nid;
    const char *skcipher = NULL, *digest = NULL, *kx = NULL, *auth = NULL;

    /* can be NULL */
    cipher_name = SSL_CIPHER_get_name(cipher);
    cipher_protocol = SSL_CIPHER_get_version(cipher);
    cipher_id = SSL_CIPHER_get_id(cipher);
    SSL_CIPHER_description(cipher, buf, sizeof(buf) - 1);
    /* Downcast to avoid a warning. Safe since buf is always 512 bytes */
    len = (int)strlen(buf);
    if (len > 1 && buf[len-1] == '\n')
        buf[len-1] = '\0';
    strength_bits = SSL_CIPHER_get_bits(cipher, &alg_bits);

    aead = SSL_CIPHER_is_aead(cipher);
    nid = SSL_CIPHER_get_cipher_nid(cipher);
    skcipher = nid != NID_undef ? OBJ_nid2ln(nid) : NULL;
    nid = SSL_CIPHER_get_digest_nid(cipher);
    digest = nid != NID_undef ? OBJ_nid2ln(nid) : NULL;
    nid = SSL_CIPHER_get_kx_nid(cipher);
    kx = nid != NID_undef ? OBJ_nid2ln(nid) : NULL;
    nid = SSL_CIPHER_get_auth_nid(cipher);
    auth = nid != NID_undef ? OBJ_nid2ln(nid) : NULL;

    return Py_BuildValue(
        "{sksssssssisi"
        "sOssssssss"
        "}",
        "id", cipher_id,
        "name", cipher_name,
        "protocol", cipher_protocol,
        "description", buf,
        "strength_bits", strength_bits,
        "alg_bits", alg_bits
        ,"aead", aead ? Py_True : Py_False,
        "symmetric", skcipher,
        "digest", digest,
        "kea", kx,
        "auth", auth
       );
}

/*[clinic input]
_ssl._SSLSocket.shared_ciphers
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_shared_ciphers_impl(PySSLSocket *self)
/*[clinic end generated code: output=3d174ead2e42c4fd input=0bfe149da8fe6306]*/
{
    STACK_OF(SSL_CIPHER) *server_ciphers;
    STACK_OF(SSL_CIPHER) *client_ciphers;
    int i, len;
    PyObject *res;
    const SSL_CIPHER* cipher;

    /* Rather than use SSL_get_shared_ciphers, we use an equivalent algorithm because:

       1) It returns a colon seperated list of strings, in an undefined
          order, that we would have to post process back into tuples.
       2) It will return a truncated string with no indication that it has
          done so, if the buffer is too small.
     */

    server_ciphers = SSL_get_ciphers(self->ssl);
    if (!server_ciphers)
        Py_RETURN_NONE;
    client_ciphers = SSL_get_client_ciphers(self->ssl);
    if (!client_ciphers)
        Py_RETURN_NONE;

    res = PyList_New(sk_SSL_CIPHER_num(server_ciphers));
    if (!res)
        return NULL;
    len = 0;
    for (i = 0; i < sk_SSL_CIPHER_num(server_ciphers); i++) {
        cipher = sk_SSL_CIPHER_value(server_ciphers, i);
        if (sk_SSL_CIPHER_find(client_ciphers, cipher) < 0)
            continue;

        PyObject *tup = cipher_to_tuple(cipher);
        if (!tup) {
            Py_DECREF(res);
            return NULL;
        }
        PyList_SET_ITEM(res, len++, tup);
    }
    Py_SET_SIZE(res, len);
    return res;
}

/*[clinic input]
_ssl._SSLSocket.cipher
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_cipher_impl(PySSLSocket *self)
/*[clinic end generated code: output=376417c16d0e5815 input=548fb0e27243796d]*/
{
    const SSL_CIPHER *current;

    if (self->ssl == NULL)
        Py_RETURN_NONE;
    current = SSL_get_current_cipher(self->ssl);
    if (current == NULL)
        Py_RETURN_NONE;
    return cipher_to_tuple(current);
}

/*[clinic input]
_ssl._SSLSocket.version
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_version_impl(PySSLSocket *self)
/*[clinic end generated code: output=178aed33193b2cdb input=900186a503436fd6]*/
{
    const char *version;

    if (self->ssl == NULL)
        Py_RETURN_NONE;
    if (!SSL_is_init_finished(self->ssl)) {
        /* handshake not finished */
        Py_RETURN_NONE;
    }
    version = SSL_get_version(self->ssl);
    if (!strcmp(version, "unknown"))
        Py_RETURN_NONE;
    return PyUnicode_FromString(version);
}

/*[clinic input]
_ssl._SSLSocket.selected_alpn_protocol
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_selected_alpn_protocol_impl(PySSLSocket *self)
/*[clinic end generated code: output=ec33688b303d250f input=442de30e35bc2913]*/
{
    const unsigned char *out;
    unsigned int outlen;

    SSL_get0_alpn_selected(self->ssl, &out, &outlen);

    if (out == NULL)
        Py_RETURN_NONE;
    return PyUnicode_FromStringAndSize((char *)out, outlen);
}

/*[clinic input]
_ssl._SSLSocket.compression
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_compression_impl(PySSLSocket *self)
/*[clinic end generated code: output=bd16cb1bb4646ae7 input=5d059d0a2bbc32c8]*/
{
#ifdef OPENSSL_NO_COMP
    Py_RETURN_NONE;
#else
    const COMP_METHOD *comp_method;
    const char *short_name;

    if (self->ssl == NULL)
        Py_RETURN_NONE;
    comp_method = SSL_get_current_compression(self->ssl);
    if (comp_method == NULL || COMP_get_type(comp_method) == NID_undef)
        Py_RETURN_NONE;
    short_name = OBJ_nid2sn(COMP_get_type(comp_method));
    if (short_name == NULL)
        Py_RETURN_NONE;
    return PyUnicode_DecodeFSDefault(short_name);
#endif
}

static PySSLContext *PySSL_get_context(PySSLSocket *self, void *closure) {
    Py_INCREF(self->ctx);
    return self->ctx;
}

static int PySSL_set_context(PySSLSocket *self, PyObject *value,
                                   void *closure) {

    if (PyObject_TypeCheck(value, self->ctx->state->PySSLContext_Type)) {
        Py_INCREF(value);
        Py_SETREF(self->ctx, (PySSLContext *)value);
        SSL_set_SSL_CTX(self->ssl, self->ctx->ctx);
        /* Set SSL* internal msg_callback to state of new context's state */
        SSL_set_msg_callback(
            self->ssl,
            self->ctx->msg_cb ? _PySSL_msg_callback : NULL
        );
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
used from within a callback function set by the sni_callback\n\
on the SSLContext to change the certificate information associated with the\n\
SSLSocket before the cryptographic exchange handshake messages\n");


static PyObject *
PySSL_get_server_side(PySSLSocket *self, void *c)
{
    return PyBool_FromLong(self->socket_type == PY_SSL_SERVER);
}

PyDoc_STRVAR(PySSL_get_server_side_doc,
"Whether this is a server-side socket.");

static PyObject *
PySSL_get_server_hostname(PySSLSocket *self, void *c)
{
    if (self->server_hostname == NULL)
        Py_RETURN_NONE;
    Py_INCREF(self->server_hostname);
    return self->server_hostname;
}

PyDoc_STRVAR(PySSL_get_server_hostname_doc,
"The currently set server hostname (for SNI).");

static PyObject *
PySSL_get_owner(PySSLSocket *self, void *c)
{
    PyObject *owner;

    if (self->owner == NULL)
        Py_RETURN_NONE;

    owner = PyWeakref_GetObject(self->owner);
    Py_INCREF(owner);
    return owner;
}

static int
PySSL_set_owner(PySSLSocket *self, PyObject *value, void *c)
{
    Py_XSETREF(self->owner, PyWeakref_NewRef(value, NULL));
    if (self->owner == NULL)
        return -1;
    return 0;
}

PyDoc_STRVAR(PySSL_get_owner_doc,
"The Python-level owner of this object.\
Passed as \"self\" in servername callback.");

static int
PySSL_traverse(PySSLSocket *self, visitproc visit, void *arg)
{
    Py_VISIT(self->exc_type);
    Py_VISIT(self->exc_value);
    Py_VISIT(self->exc_tb);
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static int
PySSL_clear(PySSLSocket *self)
{
    Py_CLEAR(self->exc_type);
    Py_CLEAR(self->exc_value);
    Py_CLEAR(self->exc_tb);
    return 0;
}

static void
PySSL_dealloc(PySSLSocket *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    if (self->ssl) {
        SSL_free(self->ssl);
    }
    Py_XDECREF(self->Socket);
    Py_XDECREF(self->ctx);
    Py_XDECREF(self->server_hostname);
    Py_XDECREF(self->owner);
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

/* If the socket has a timeout, do a select()/poll() on the socket.
   The argument writing indicates the direction.
   Returns one of the possibilities in the timeout_state enum (above).
 */

static int
PySSL_select(PySocketSockObject *s, int writing, _PyTime_t timeout)
{
    int rc;
#ifdef HAVE_POLL
    struct pollfd pollfd;
    _PyTime_t ms;
#else
    int nfds;
    fd_set fds;
    struct timeval tv;
#endif

    /* Nothing to do unless we're in timeout mode (not non-blocking) */
    if ((s == NULL) || (timeout == 0))
        return SOCKET_IS_NONBLOCKING;
    else if (timeout < 0) {
        if (s->sock_timeout > 0)
            return SOCKET_HAS_TIMED_OUT;
        else
            return SOCKET_IS_BLOCKING;
    }

    /* Guard against closed socket */
    if (s->sock_fd == INVALID_SOCKET)
        return SOCKET_HAS_BEEN_CLOSED;

    /* Prefer poll, if available, since you can poll() any fd
     * which can't be done with select(). */
#ifdef HAVE_POLL
    pollfd.fd = s->sock_fd;
    pollfd.events = writing ? POLLOUT : POLLIN;

    /* timeout is in seconds, poll() uses milliseconds */
    ms = (int)_PyTime_AsMilliseconds(timeout, _PyTime_ROUND_CEILING);
    assert(ms <= INT_MAX);

    PySSL_BEGIN_ALLOW_THREADS
    rc = poll(&pollfd, 1, (int)ms);
    PySSL_END_ALLOW_THREADS
#else
    /* Guard against socket too large for select*/
    if (!_PyIsSelectable_fd(s->sock_fd))
        return SOCKET_TOO_LARGE_FOR_SELECT;

    _PyTime_AsTimeval_clamp(timeout, &tv, _PyTime_ROUND_CEILING);

    FD_ZERO(&fds);
    FD_SET(s->sock_fd, &fds);

    /* Wait until the socket becomes ready */
    PySSL_BEGIN_ALLOW_THREADS
    nfds = Py_SAFE_DOWNCAST(s->sock_fd+1, SOCKET_T, int);
    if (writing)
        rc = select(nfds, NULL, &fds, NULL, &tv);
    else
        rc = select(nfds, &fds, NULL, NULL, &tv);
    PySSL_END_ALLOW_THREADS
#endif

    /* Return SOCKET_TIMED_OUT on timeout, SOCKET_OPERATION_OK otherwise
       (when we are able to write or when there's something to read) */
    return rc == 0 ? SOCKET_HAS_TIMED_OUT : SOCKET_OPERATION_OK;
}

/*[clinic input]
_ssl._SSLSocket.write
    b: Py_buffer
    /

Writes the bytes-like object b into the SSL object.

Returns the number of bytes written.
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_write_impl(PySSLSocket *self, Py_buffer *b)
/*[clinic end generated code: output=aa7a6be5527358d8 input=77262d994fe5100a]*/
{
    size_t count = 0;
    int retval;
    int sockstate;
    _PySSLError err;
    int nonblocking;
    PySocketSockObject *sock = GET_SOCKET(self);
    _PyTime_t timeout, deadline = 0;
    int has_timeout;

    if (sock != NULL) {
        if (((PyObject*)sock) == Py_None) {
            _setSSLError(get_state_sock(self),
                         "Underlying socket connection gone",
                         PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
            return NULL;
        }
        Py_INCREF(sock);
    }

    if (sock != NULL) {
        /* just in case the blocking state of the socket has been changed */
        nonblocking = (sock->sock_timeout >= 0);
        BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
        BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);
    }

    timeout = GET_SOCKET_TIMEOUT(sock);
    has_timeout = (timeout > 0);
    if (has_timeout) {
        deadline = _PyDeadline_Init(timeout);
    }

    sockstate = PySSL_select(sock, 1, timeout);
    if (sockstate == SOCKET_HAS_TIMED_OUT) {
        PyErr_SetString(PyExc_TimeoutError,
                        "The write operation timed out");
        goto error;
    } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
        PyErr_SetString(get_state_sock(self)->PySSLErrorObject,
                        "Underlying socket has been closed.");
        goto error;
    } else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
        PyErr_SetString(get_state_sock(self)->PySSLErrorObject,
                        "Underlying socket too large for select().");
        goto error;
    }

    do {
        PySSL_BEGIN_ALLOW_THREADS
        retval = SSL_write_ex(self->ssl, b->buf, (size_t)b->len, &count);
        err = _PySSL_errno(retval == 0, self->ssl, retval);
        PySSL_END_ALLOW_THREADS
        self->err = err;

        if (PyErr_CheckSignals())
            goto error;

        if (has_timeout) {
            timeout = _PyDeadline_Get(deadline);
        }

        if (err.ssl == SSL_ERROR_WANT_READ) {
            sockstate = PySSL_select(sock, 0, timeout);
        } else if (err.ssl == SSL_ERROR_WANT_WRITE) {
            sockstate = PySSL_select(sock, 1, timeout);
        } else {
            sockstate = SOCKET_OPERATION_OK;
        }

        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PyExc_TimeoutError,
                            "The write operation timed out");
            goto error;
        } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
            PyErr_SetString(get_state_sock(self)->PySSLErrorObject,
                            "Underlying socket has been closed.");
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err.ssl == SSL_ERROR_WANT_READ ||
             err.ssl == SSL_ERROR_WANT_WRITE);

    Py_XDECREF(sock);
    if (retval == 0)
        return PySSL_SetError(self, retval, __FILE__, __LINE__);
    if (PySSL_ChainExceptions(self) < 0)
        return NULL;
    return PyLong_FromSize_t(count);
error:
    Py_XDECREF(sock);
    PySSL_ChainExceptions(self);
    return NULL;
}

/*[clinic input]
_ssl._SSLSocket.pending

Returns the number of already decrypted bytes available for read, pending on the connection.
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_pending_impl(PySSLSocket *self)
/*[clinic end generated code: output=983d9fecdc308a83 input=2b77487d6dfd597f]*/
{
    int count = 0;
    _PySSLError err;

    PySSL_BEGIN_ALLOW_THREADS
    count = SSL_pending(self->ssl);
    err = _PySSL_errno(count < 0, self->ssl, count);
    PySSL_END_ALLOW_THREADS
    self->err = err;

    if (count < 0)
        return PySSL_SetError(self, count, __FILE__, __LINE__);
    else
        return PyLong_FromLong(count);
}

/*[clinic input]
_ssl._SSLSocket.read
    size as len: Py_ssize_t
    [
    buffer: Py_buffer(accept={rwbuffer})
    ]
    /

Read up to size bytes from the SSL socket.
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_read_impl(PySSLSocket *self, Py_ssize_t len,
                          int group_right_1, Py_buffer *buffer)
/*[clinic end generated code: output=49b16e6406023734 input=ec48bf622be1c4a1]*/
{
    PyObject *dest = NULL;
    char *mem;
    size_t count = 0;
    int retval;
    int sockstate;
    _PySSLError err;
    int nonblocking;
    PySocketSockObject *sock = GET_SOCKET(self);
    _PyTime_t timeout, deadline = 0;
    int has_timeout;

    if (!group_right_1 && len < 0) {
        PyErr_SetString(PyExc_ValueError, "size should not be negative");
        return NULL;
    }

    if (sock != NULL) {
        if (((PyObject*)sock) == Py_None) {
            _setSSLError(get_state_sock(self),
                         "Underlying socket connection gone",
                         PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
            return NULL;
        }
        Py_INCREF(sock);
    }

    if (!group_right_1) {
        dest = PyBytes_FromStringAndSize(NULL, len);
        if (dest == NULL)
            goto error;
        if (len == 0) {
            Py_XDECREF(sock);
            return dest;
        }
        mem = PyBytes_AS_STRING(dest);
    }
    else {
        mem = buffer->buf;
        if (len <= 0 || len > buffer->len) {
            len = (int) buffer->len;
            if (buffer->len != len) {
                PyErr_SetString(PyExc_OverflowError,
                                "maximum length can't fit in a C 'int'");
                goto error;
            }
            if (len == 0) {
                count = 0;
                goto done;
            }
        }
    }

    if (sock != NULL) {
        /* just in case the blocking state of the socket has been changed */
        nonblocking = (sock->sock_timeout >= 0);
        BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
        BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);
    }

    timeout = GET_SOCKET_TIMEOUT(sock);
    has_timeout = (timeout > 0);
    if (has_timeout)
        deadline = _PyDeadline_Init(timeout);

    do {
        PySSL_BEGIN_ALLOW_THREADS
        retval = SSL_read_ex(self->ssl, mem, (size_t)len, &count);
        err = _PySSL_errno(retval == 0, self->ssl, retval);
        PySSL_END_ALLOW_THREADS
        self->err = err;

        if (PyErr_CheckSignals())
            goto error;

        if (has_timeout) {
            timeout = _PyDeadline_Get(deadline);
        }

        if (err.ssl == SSL_ERROR_WANT_READ) {
            sockstate = PySSL_select(sock, 0, timeout);
        } else if (err.ssl == SSL_ERROR_WANT_WRITE) {
            sockstate = PySSL_select(sock, 1, timeout);
        } else if (err.ssl == SSL_ERROR_ZERO_RETURN &&
                   SSL_get_shutdown(self->ssl) == SSL_RECEIVED_SHUTDOWN)
        {
            count = 0;
            goto done;
        }
        else
            sockstate = SOCKET_OPERATION_OK;

        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PyExc_TimeoutError,
                            "The read operation timed out");
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err.ssl == SSL_ERROR_WANT_READ ||
             err.ssl == SSL_ERROR_WANT_WRITE);

    if (retval == 0) {
        PySSL_SetError(self, retval, __FILE__, __LINE__);
        goto error;
    }
    if (self->exc_type != NULL)
        goto error;

done:
    Py_XDECREF(sock);
    if (!group_right_1) {
        _PyBytes_Resize(&dest, count);
        return dest;
    }
    else {
        return PyLong_FromSize_t(count);
    }

error:
    PySSL_ChainExceptions(self);
    Py_XDECREF(sock);
    if (!group_right_1)
        Py_XDECREF(dest);
    return NULL;
}

/*[clinic input]
_ssl._SSLSocket.shutdown

Does the SSL shutdown handshake with the remote end.
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_shutdown_impl(PySSLSocket *self)
/*[clinic end generated code: output=ca1aa7ed9d25ca42 input=11d39e69b0a2bf4a]*/
{
    _PySSLError err;
    int sockstate, nonblocking, ret;
    int zeros = 0;
    PySocketSockObject *sock = GET_SOCKET(self);
    _PyTime_t timeout, deadline = 0;
    int has_timeout;

    if (sock != NULL) {
        /* Guard against closed socket */
        if ((((PyObject*)sock) == Py_None) || (sock->sock_fd == INVALID_SOCKET)) {
            _setSSLError(get_state_sock(self),
                         "Underlying socket connection gone",
                         PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
            return NULL;
        }
        Py_INCREF(sock);

        /* Just in case the blocking state of the socket has been changed */
        nonblocking = (sock->sock_timeout >= 0);
        BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
        BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);
    }

    timeout = GET_SOCKET_TIMEOUT(sock);
    has_timeout = (timeout > 0);
    if (has_timeout) {
        deadline = _PyDeadline_Init(timeout);
    }

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
        ret = SSL_shutdown(self->ssl);
        err = _PySSL_errno(ret < 0, self->ssl, ret);
        PySSL_END_ALLOW_THREADS
        self->err = err;

        /* If err == 1, a secure shutdown with SSL_shutdown() is complete */
        if (ret > 0)
            break;
        if (ret == 0) {
            /* Don't loop endlessly; instead preserve legacy
               behaviour of trying SSL_shutdown() only twice.
               This looks necessary for OpenSSL < 0.9.8m */
            if (++zeros > 1)
                break;
            /* Shutdown was sent, now try receiving */
            self->shutdown_seen_zero = 1;
            continue;
        }

        if (has_timeout) {
            timeout = _PyDeadline_Get(deadline);
        }

        /* Possibly retry shutdown until timeout or failure */
        if (err.ssl == SSL_ERROR_WANT_READ)
            sockstate = PySSL_select(sock, 0, timeout);
        else if (err.ssl == SSL_ERROR_WANT_WRITE)
            sockstate = PySSL_select(sock, 1, timeout);
        else
            break;

        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            if (err.ssl == SSL_ERROR_WANT_READ)
                PyErr_SetString(PyExc_TimeoutError,
                                "The read operation timed out");
            else
                PyErr_SetString(PyExc_TimeoutError,
                                "The write operation timed out");
            goto error;
        }
        else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
            PyErr_SetString(get_state_sock(self)->PySSLErrorObject,
                            "Underlying socket too large for select().");
            goto error;
        }
        else if (sockstate != SOCKET_OPERATION_OK)
            /* Retain the SSL error code */
            break;
    }
    if (ret < 0) {
        Py_XDECREF(sock);
        PySSL_SetError(self, ret, __FILE__, __LINE__);
        return NULL;
    }
    if (self->exc_type != NULL)
        goto error;
    if (sock)
        /* It's already INCREF'ed */
        return (PyObject *) sock;
    else
        Py_RETURN_NONE;

error:
    Py_XDECREF(sock);
    PySSL_ChainExceptions(self);
    return NULL;
}

/*[clinic input]
_ssl._SSLSocket.get_channel_binding
   cb_type: str = "tls-unique"

Get channel binding data for current connection.

Raise ValueError if the requested `cb_type` is not supported.  Return bytes
of the data or None if the data is not available (e.g. before the handshake).
Only 'tls-unique' channel binding data from RFC 5929 is supported.
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_get_channel_binding_impl(PySSLSocket *self,
                                         const char *cb_type)
/*[clinic end generated code: output=34bac9acb6a61d31 input=08b7e43b99c17d41]*/
{
    char buf[PySSL_CB_MAXLEN];
    size_t len;

    if (strcmp(cb_type, "tls-unique") == 0) {
        if (SSL_session_reused(self->ssl) ^ !self->socket_type) {
            /* if session is resumed XOR we are the client */
            len = SSL_get_finished(self->ssl, buf, PySSL_CB_MAXLEN);
        }
        else {
            /* if a new session XOR we are the server */
            len = SSL_get_peer_finished(self->ssl, buf, PySSL_CB_MAXLEN);
        }
    }
    else {
        PyErr_Format(
            PyExc_ValueError,
            "'%s' channel binding type not implemented",
            cb_type
        );
        return NULL;
    }

    /* It cannot be negative in current OpenSSL version as of July 2011 */
    if (len == 0)
        Py_RETURN_NONE;

    return PyBytes_FromStringAndSize(buf, len);
}

/*[clinic input]
_ssl._SSLSocket.verify_client_post_handshake

Initiate TLS 1.3 post-handshake authentication
[clinic start generated code]*/

static PyObject *
_ssl__SSLSocket_verify_client_post_handshake_impl(PySSLSocket *self)
/*[clinic end generated code: output=532147f3b1341425 input=6bfa874810a3d889]*/
{
#ifdef TLS1_3_VERSION
    int err = SSL_verify_client_post_handshake(self->ssl);
    if (err == 0)
        return _setSSLError(get_state_sock(self), NULL, 0, __FILE__, __LINE__);
    else
        Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "Post-handshake auth is not supported by your "
                    "OpenSSL version.");
    return NULL;
#endif
}

static SSL_SESSION*
_ssl_session_dup(SSL_SESSION *session) {
    SSL_SESSION *newsession = NULL;
    int slen;
    unsigned char *senc = NULL, *p;
    const unsigned char *const_p;

    if (session == NULL) {
        PyErr_SetString(PyExc_ValueError, "Invalid session");
        goto error;
    }

    /* get length */
    slen = i2d_SSL_SESSION(session, NULL);
    if (slen == 0 || slen > 0xFF00) {
        PyErr_SetString(PyExc_ValueError, "i2d() failed");
        goto error;
    }
    if ((senc = PyMem_Malloc(slen)) == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    p = senc;
    if (!i2d_SSL_SESSION(session, &p)) {
        PyErr_SetString(PyExc_ValueError, "i2d() failed");
        goto error;
    }
    const_p = senc;
    newsession = d2i_SSL_SESSION(NULL, &const_p, slen);
    if (newsession == NULL) {
        PyErr_SetString(PyExc_ValueError, "d2i() failed");
        goto error;
    }
    PyMem_Free(senc);
    return newsession;
  error:
    if (senc != NULL) {
        PyMem_Free(senc);
    }
    return NULL;
}

static PyObject *
PySSL_get_session(PySSLSocket *self, void *closure) {
    /* get_session can return sessions from a server-side connection,
     * it does not check for handshake done or client socket. */
    PySSLSession *pysess;
    SSL_SESSION *session;

    /* duplicate session as workaround for session bug in OpenSSL 1.1.0,
     * https://github.com/openssl/openssl/issues/1550 */
    session = SSL_get0_session(self->ssl);  /* borrowed reference */
    if (session == NULL) {
        Py_RETURN_NONE;
    }
    if ((session = _ssl_session_dup(session)) == NULL) {
        return NULL;
    }
    session = SSL_get1_session(self->ssl);
    if (session == NULL) {
        Py_RETURN_NONE;
    }
    pysess = PyObject_GC_New(PySSLSession, self->ctx->state->PySSLSession_Type);
    if (pysess == NULL) {
        SSL_SESSION_free(session);
        return NULL;
    }

    assert(self->ctx);
    pysess->ctx = self->ctx;
    Py_INCREF(pysess->ctx);
    pysess->session = session;
    PyObject_GC_Track(pysess);
    return (PyObject *)pysess;
}

static int PySSL_set_session(PySSLSocket *self, PyObject *value,
                             void *closure)
                              {
    PySSLSession *pysess;
    SSL_SESSION *session;
    int result;

    if (!Py_IS_TYPE(value, get_state_sock(self)->PySSLSession_Type)) {
        PyErr_SetString(PyExc_TypeError, "Value is not a SSLSession.");
        return -1;
    }
    pysess = (PySSLSession *)value;

    if (self->ctx->ctx != pysess->ctx->ctx) {
        PyErr_SetString(PyExc_ValueError,
                        "Session refers to a different SSLContext.");
        return -1;
    }
    if (self->socket_type != PY_SSL_CLIENT) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot set session for server-side SSLSocket.");
        return -1;
    }
    if (SSL_is_init_finished(self->ssl)) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot set session after handshake.");
        return -1;
    }
    /* duplicate session */
    if ((session = _ssl_session_dup(pysess->session)) == NULL) {
        return -1;
    }
    result = SSL_set_session(self->ssl, session);
    /* free duplicate, SSL_set_session() bumps ref count */
    SSL_SESSION_free(session);
    if (result == 0) {
        _setSSLError(get_state_sock(self), NULL, 0, __FILE__, __LINE__);
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(PySSL_set_session_doc,
"_setter_session(session)\n\
\
Get / set SSLSession.");

static PyObject *
PySSL_get_session_reused(PySSLSocket *self, void *closure) {
    if (SSL_session_reused(self->ssl)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyDoc_STRVAR(PySSL_get_session_reused_doc,
"Was the client session reused during handshake?");

static PyGetSetDef ssl_getsetlist[] = {
    {"context", (getter) PySSL_get_context,
                (setter) PySSL_set_context, PySSL_set_context_doc},
    {"server_side", (getter) PySSL_get_server_side, NULL,
                    PySSL_get_server_side_doc},
    {"server_hostname", (getter) PySSL_get_server_hostname, NULL,
                        PySSL_get_server_hostname_doc},
    {"owner", (getter) PySSL_get_owner, (setter) PySSL_set_owner,
              PySSL_get_owner_doc},
    {"session", (getter) PySSL_get_session,
                (setter) PySSL_set_session, PySSL_set_session_doc},
    {"session_reused", (getter) PySSL_get_session_reused, NULL,
              PySSL_get_session_reused_doc},
    {NULL},            /* sentinel */
};

static PyMethodDef PySSLMethods[] = {
    _SSL__SSLSOCKET_DO_HANDSHAKE_METHODDEF
    _SSL__SSLSOCKET_WRITE_METHODDEF
    _SSL__SSLSOCKET_READ_METHODDEF
    _SSL__SSLSOCKET_PENDING_METHODDEF
    _SSL__SSLSOCKET_GETPEERCERT_METHODDEF
    _SSL__SSLSOCKET_GET_CHANNEL_BINDING_METHODDEF
    _SSL__SSLSOCKET_CIPHER_METHODDEF
    _SSL__SSLSOCKET_SHARED_CIPHERS_METHODDEF
    _SSL__SSLSOCKET_VERSION_METHODDEF
    _SSL__SSLSOCKET_SELECTED_ALPN_PROTOCOL_METHODDEF
    _SSL__SSLSOCKET_COMPRESSION_METHODDEF
    _SSL__SSLSOCKET_SHUTDOWN_METHODDEF
    _SSL__SSLSOCKET_VERIFY_CLIENT_POST_HANDSHAKE_METHODDEF
    _SSL__SSLSOCKET_GET_UNVERIFIED_CHAIN_METHODDEF
    _SSL__SSLSOCKET_GET_VERIFIED_CHAIN_METHODDEF
    {NULL, NULL}
};

static PyType_Slot PySSLSocket_slots[] = {
    {Py_tp_methods, PySSLMethods},
    {Py_tp_getset, ssl_getsetlist},
    {Py_tp_dealloc, PySSL_dealloc},
    {Py_tp_traverse, PySSL_traverse},
    {Py_tp_clear, PySSL_clear},
    {0, 0},
};

static PyType_Spec PySSLSocket_spec = {
    .name = "_ssl._SSLSocket",
    .basicsize = sizeof(PySSLSocket),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_HAVE_GC),
    .slots = PySSLSocket_slots,
};

/*
 * _SSLContext objects
 */

static int
_set_verify_mode(PySSLContext *self, enum py_ssl_cert_requirements n)
{
    int mode;
    int (*verify_cb)(int, X509_STORE_CTX *) = NULL;

    switch(n) {
    case PY_SSL_CERT_NONE:
        mode = SSL_VERIFY_NONE;
        break;
    case PY_SSL_CERT_OPTIONAL:
        mode = SSL_VERIFY_PEER;
        break;
    case PY_SSL_CERT_REQUIRED:
        mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        break;
    default:
         PyErr_SetString(PyExc_ValueError,
                        "invalid value for verify_mode");
        return -1;
    }

    /* bpo-37428: newPySSLSocket() sets SSL_VERIFY_POST_HANDSHAKE flag for
     * server sockets and SSL_set_post_handshake_auth() for client. */

    /* keep current verify cb */
    verify_cb = SSL_CTX_get_verify_callback(self->ctx);
    SSL_CTX_set_verify(self->ctx, mode, verify_cb);
    return 0;
}

/*[clinic input]
@classmethod
_ssl._SSLContext.__new__
    protocol as proto_version: int
    /
[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_impl(PyTypeObject *type, int proto_version)
/*[clinic end generated code: output=2cf0d7a0741b6bd1 input=8d58a805b95fc534]*/
{
    PySSLContext *self;
    uint64_t options;
    const SSL_METHOD *method = NULL;
    SSL_CTX *ctx = NULL;
    X509_VERIFY_PARAM *params;
    int result;

   /* slower approach, walk MRO and get borrowed reference to module.
    * PyType_GetModuleByDef is required for SSLContext subclasses */
    PyObject *module = PyType_GetModuleByDef(type, &_sslmodule_def);
    if (module == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Cannot find internal module state");
        return NULL;
    }

    switch(proto_version) {
#if defined(SSL3_VERSION) && !defined(OPENSSL_NO_SSL3)
    case PY_SSL_VERSION_SSL3:
        PY_SSL_DEPRECATED("ssl.PROTOCOL_SSLv3 is deprecated", 2, NULL);
        method = SSLv3_method();
        break;
#endif
#if (defined(TLS1_VERSION) && \
        !defined(OPENSSL_NO_TLS1) && \
        !defined(OPENSSL_NO_TLS1_METHOD))
    case PY_SSL_VERSION_TLS1:
        PY_SSL_DEPRECATED("ssl.PROTOCOL_TLSv1 is deprecated", 2, NULL);
        method = TLSv1_method();
        break;
#endif
#if (defined(TLS1_1_VERSION) && \
        !defined(OPENSSL_NO_TLS1_1) && \
        !defined(OPENSSL_NO_TLS1_1_METHOD))
    case PY_SSL_VERSION_TLS1_1:
        PY_SSL_DEPRECATED("ssl.PROTOCOL_TLSv1_1 is deprecated", 2, NULL);
        method = TLSv1_1_method();
        break;
#endif
#if (defined(TLS1_2_VERSION) && \
        !defined(OPENSSL_NO_TLS1_2) && \
        !defined(OPENSSL_NO_TLS1_2_METHOD))
    case PY_SSL_VERSION_TLS1_2:
        PY_SSL_DEPRECATED("ssl.PROTOCOL_TLSv1_2 is deprecated", 2, NULL);
        method = TLSv1_2_method();
        break;
#endif
    case PY_SSL_VERSION_TLS:
        PY_SSL_DEPRECATED("ssl.PROTOCOL_TLS is deprecated", 2, NULL);
        method = TLS_method();
        break;
    case PY_SSL_VERSION_TLS_CLIENT:
        method = TLS_client_method();
        break;
    case PY_SSL_VERSION_TLS_SERVER:
        method = TLS_server_method();
        break;
    default:
        method = NULL;
    }

    if (method == NULL) {
        PyErr_Format(PyExc_ValueError,
                     "invalid or unsupported protocol version %i",
                     proto_version);
        return NULL;
    }

    PySSL_BEGIN_ALLOW_THREADS
    ctx = SSL_CTX_new(method);
    PySSL_END_ALLOW_THREADS

    if (ctx == NULL) {
        _setSSLError(get_ssl_state(module), NULL, 0, __FILE__, __LINE__);
        return NULL;
    }

    assert(type != NULL && type->tp_alloc != NULL);
    self = (PySSLContext *) type->tp_alloc(type, 0);
    if (self == NULL) {
        SSL_CTX_free(ctx);
        return NULL;
    }
    self->ctx = ctx;
    self->hostflags = X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS;
    self->protocol = proto_version;
    self->msg_cb = NULL;
    self->keylog_filename = NULL;
    self->keylog_bio = NULL;
    self->alpn_protocols = NULL;
    self->set_sni_cb = NULL;
    self->state = get_ssl_state(module);

    /* Don't check host name by default */
    if (proto_version == PY_SSL_VERSION_TLS_CLIENT) {
        self->check_hostname = 1;
        if (_set_verify_mode(self, PY_SSL_CERT_REQUIRED) == -1) {
            Py_DECREF(self);
            return NULL;
        }
    } else {
        self->check_hostname = 0;
        if (_set_verify_mode(self, PY_SSL_CERT_NONE) == -1) {
            Py_DECREF(self);
            return NULL;
        }
    }
    /* Defaults */
    options = SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    if (proto_version != PY_SSL_VERSION_SSL2)
        options |= SSL_OP_NO_SSLv2;
    if (proto_version != PY_SSL_VERSION_SSL3)
        options |= SSL_OP_NO_SSLv3;
    /* Minimal security flags for server and client side context.
     * Client sockets ignore server-side parameters. */
#ifdef SSL_OP_NO_COMPRESSION
    options |= SSL_OP_NO_COMPRESSION;
#endif
#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
    options |= SSL_OP_CIPHER_SERVER_PREFERENCE;
#endif
#ifdef SSL_OP_SINGLE_DH_USE
    options |= SSL_OP_SINGLE_DH_USE;
#endif
#ifdef SSL_OP_SINGLE_ECDH_USE
    options |= SSL_OP_SINGLE_ECDH_USE;
#endif
    SSL_CTX_set_options(self->ctx, options);

    /* A bare minimum cipher list without completely broken cipher suites.
     * It's far from perfect but gives users a better head start. */
    if (proto_version != PY_SSL_VERSION_SSL2) {
#if PY_SSL_DEFAULT_CIPHERS == 2
        /* stick to OpenSSL's default settings */
        result = 1;
#else
        result = SSL_CTX_set_cipher_list(ctx, PY_SSL_DEFAULT_CIPHER_STRING);
#endif
    } else {
        /* SSLv2 needs MD5 */
        result = SSL_CTX_set_cipher_list(ctx, "HIGH:!aNULL:!eNULL");
    }
    if (result == 0) {
        ERR_clear_error();
        PyErr_SetString(get_state_ctx(self)->PySSLErrorObject,
                        "No cipher can be selected.");
        goto error;
    }
#ifdef PY_SSL_MIN_PROTOCOL
    switch(proto_version) {
    case PY_SSL_VERSION_TLS:
    case PY_SSL_VERSION_TLS_CLIENT:
    case PY_SSL_VERSION_TLS_SERVER:
        result = SSL_CTX_set_min_proto_version(ctx, PY_SSL_MIN_PROTOCOL);
        if (result == 0) {
            PyErr_Format(PyExc_ValueError,
                         "Failed to set minimum protocol 0x%x",
                          PY_SSL_MIN_PROTOCOL);
            goto error;
        }
        break;
    default:
        break;
    }
#endif

    /* Set SSL_MODE_RELEASE_BUFFERS. This potentially greatly reduces memory
       usage for no cost at all. */
    SSL_CTX_set_mode(self->ctx, SSL_MODE_RELEASE_BUFFERS);

#define SID_CTX "Python"
    SSL_CTX_set_session_id_context(self->ctx, (const unsigned char *) SID_CTX,
                                   sizeof(SID_CTX));
#undef SID_CTX

    params = SSL_CTX_get0_param(self->ctx);
    /* Improve trust chain building when cross-signed intermediate
       certificates are present. See https://bugs.python.org/issue23476. */
    X509_VERIFY_PARAM_set_flags(params, X509_V_FLAG_TRUSTED_FIRST);
    X509_VERIFY_PARAM_set_hostflags(params, self->hostflags);

#ifdef TLS1_3_VERSION
    self->post_handshake_auth = 0;
    SSL_CTX_set_post_handshake_auth(self->ctx, self->post_handshake_auth);
#endif

    return (PyObject *)self;
  error:
    Py_XDECREF(self);
    ERR_clear_error();
    return NULL;
}

static int
context_traverse(PySSLContext *self, visitproc visit, void *arg)
{
    Py_VISIT(self->set_sni_cb);
    Py_VISIT(self->msg_cb);
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static int
context_clear(PySSLContext *self)
{
    Py_CLEAR(self->set_sni_cb);
    Py_CLEAR(self->msg_cb);
    Py_CLEAR(self->keylog_filename);
    if (self->keylog_bio != NULL) {
        PySSL_BEGIN_ALLOW_THREADS
        BIO_free_all(self->keylog_bio);
        PySSL_END_ALLOW_THREADS
        self->keylog_bio = NULL;
    }
    return 0;
}

static void
context_dealloc(PySSLContext *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(self);
    context_clear(self);
    SSL_CTX_free(self->ctx);
    PyMem_FREE(self->alpn_protocols);
    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

/*[clinic input]
_ssl._SSLContext.set_ciphers
    cipherlist: str
    /
[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_set_ciphers_impl(PySSLContext *self, const char *cipherlist)
/*[clinic end generated code: output=3a3162f3557c0f3f input=a7ac931b9f3ca7fc]*/
{
    int ret = SSL_CTX_set_cipher_list(self->ctx, cipherlist);
    if (ret == 0) {
        /* Clearing the error queue is necessary on some OpenSSL versions,
           otherwise the error will be reported again when another SSL call
           is done. */
        ERR_clear_error();
        PyErr_SetString(get_state_ctx(self)->PySSLErrorObject,
                        "No cipher can be selected.");
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_ssl._SSLContext.get_ciphers
[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_get_ciphers_impl(PySSLContext *self)
/*[clinic end generated code: output=a56e4d68a406dfc4 input=a2aadc9af89b79c5]*/
{
    SSL *ssl = NULL;
    STACK_OF(SSL_CIPHER) *sk = NULL;
    const SSL_CIPHER *cipher;
    int i=0;
    PyObject *result = NULL, *dct;

    ssl = SSL_new(self->ctx);
    if (ssl == NULL) {
        _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        goto exit;
    }
    sk = SSL_get_ciphers(ssl);

    result = PyList_New(sk_SSL_CIPHER_num(sk));
    if (result == NULL) {
        goto exit;
    }

    for (i = 0; i < sk_SSL_CIPHER_num(sk); i++) {
        cipher = sk_SSL_CIPHER_value(sk, i);
        dct = cipher_to_dict(cipher);
        if (dct == NULL) {
            Py_CLEAR(result);
            goto exit;
        }
        PyList_SET_ITEM(result, i, dct);
    }

  exit:
    if (ssl != NULL)
        SSL_free(ssl);
    return result;

}


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

/*[clinic input]
_ssl._SSLContext._set_alpn_protocols
    protos: Py_buffer
    /
[clinic start generated code]*/

static PyObject *
_ssl__SSLContext__set_alpn_protocols_impl(PySSLContext *self,
                                          Py_buffer *protos)
/*[clinic end generated code: output=87599a7f76651a9b input=9bba964595d519be]*/
{
    if ((size_t)protos->len > UINT_MAX) {
        PyErr_Format(PyExc_OverflowError,
            "protocols longer than %u bytes", UINT_MAX);
        return NULL;
    }

    PyMem_Free(self->alpn_protocols);
    self->alpn_protocols = PyMem_Malloc(protos->len);
    if (!self->alpn_protocols)
        return PyErr_NoMemory();
    memcpy(self->alpn_protocols, protos->buf, protos->len);
    self->alpn_protocols_len = (unsigned int)protos->len;

    if (SSL_CTX_set_alpn_protos(self->ctx, self->alpn_protocols, self->alpn_protocols_len))
        return PyErr_NoMemory();
    SSL_CTX_set_alpn_select_cb(self->ctx, _selectALPN_cb, self);

    Py_RETURN_NONE;
}

static PyObject *
get_verify_mode(PySSLContext *self, void *c)
{
    /* ignore SSL_VERIFY_CLIENT_ONCE and SSL_VERIFY_POST_HANDSHAKE */
    int mask = (SSL_VERIFY_NONE | SSL_VERIFY_PEER |
                SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
    switch (SSL_CTX_get_verify_mode(self->ctx) & mask) {
    case SSL_VERIFY_NONE:
        return PyLong_FromLong(PY_SSL_CERT_NONE);
    case SSL_VERIFY_PEER:
        return PyLong_FromLong(PY_SSL_CERT_OPTIONAL);
    case SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT:
        return PyLong_FromLong(PY_SSL_CERT_REQUIRED);
    }
    PyErr_SetString(get_state_ctx(self)->PySSLErrorObject,
                    "invalid return value from SSL_CTX_get_verify_mode");
    return NULL;
}

static int
set_verify_mode(PySSLContext *self, PyObject *arg, void *c)
{
    int n;
    if (!PyArg_Parse(arg, "i", &n))
        return -1;
    if (n == PY_SSL_CERT_NONE && self->check_hostname) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot set verify_mode to CERT_NONE when "
                        "check_hostname is enabled.");
        return -1;
    }
    return _set_verify_mode(self, n);
}

static PyObject *
get_verify_flags(PySSLContext *self, void *c)
{
    X509_VERIFY_PARAM *param;
    unsigned long flags;

    param = SSL_CTX_get0_param(self->ctx);
    flags = X509_VERIFY_PARAM_get_flags(param);
    return PyLong_FromUnsignedLong(flags);
}

static int
set_verify_flags(PySSLContext *self, PyObject *arg, void *c)
{
    X509_VERIFY_PARAM *param;
    unsigned long new_flags, flags, set, clear;

    if (!PyArg_Parse(arg, "k", &new_flags))
        return -1;
    param = SSL_CTX_get0_param(self->ctx);
    flags = X509_VERIFY_PARAM_get_flags(param);
    clear = flags & ~new_flags;
    set = ~flags & new_flags;
    if (clear) {
        if (!X509_VERIFY_PARAM_clear_flags(param, clear)) {
            _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
            return -1;
        }
    }
    if (set) {
        if (!X509_VERIFY_PARAM_set_flags(param, set)) {
            _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
            return -1;
        }
    }
    return 0;
}

/* Getter and setter for protocol version */
static int
set_min_max_proto_version(PySSLContext *self, PyObject *arg, int what)
{
    long v;
    int result;

    if (!PyArg_Parse(arg, "l", &v))
        return -1;
    if (v > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Option is too long");
        return -1;
    }

    switch(self->protocol) {
    case PY_SSL_VERSION_TLS_CLIENT:  /* fall through */
    case PY_SSL_VERSION_TLS_SERVER:  /* fall through */
    case PY_SSL_VERSION_TLS:
        break;
    default:
        PyErr_SetString(
            PyExc_ValueError,
            "The context's protocol doesn't support modification of "
            "highest and lowest version."
        );
        return -1;
    }

    /* check for deprecations and supported values */
    switch(v) {
        case PY_PROTO_SSLv3:
            PY_SSL_DEPRECATED("ssl.TLSVersion.SSLv3 is deprecated", 2, -1);
            break;
        case PY_PROTO_TLSv1:
            PY_SSL_DEPRECATED("ssl.TLSVersion.TLSv1 is deprecated", 2, -1);
            break;
        case PY_PROTO_TLSv1_1:
            PY_SSL_DEPRECATED("ssl.TLSVersion.TLSv1_1 is deprecated", 2, -1);
            break;
        case PY_PROTO_MINIMUM_SUPPORTED:
        case PY_PROTO_MAXIMUM_SUPPORTED:
        case PY_PROTO_TLSv1_2:
        case PY_PROTO_TLSv1_3:
            /* ok */
            break;
        default:
            PyErr_Format(PyExc_ValueError,
                     "Unsupported TLS/SSL version 0x%x", v);
            return -1;
    }

    if (what == 0) {
        switch(v) {
        case PY_PROTO_MINIMUM_SUPPORTED:
            v = 0;
            break;
        case PY_PROTO_MAXIMUM_SUPPORTED:
            /* Emulate max for set_min_proto_version */
            v = PY_PROTO_MAXIMUM_AVAILABLE;
            break;
        default:
            break;
        }
        result = SSL_CTX_set_min_proto_version(self->ctx, v);
    }
    else {
        switch(v) {
        case PY_PROTO_MAXIMUM_SUPPORTED:
            v = 0;
            break;
        case PY_PROTO_MINIMUM_SUPPORTED:
            /* Emulate max for set_min_proto_version */
            v = PY_PROTO_MINIMUM_AVAILABLE;
            break;
        default:
            break;
        }
        result = SSL_CTX_set_max_proto_version(self->ctx, v);
    }
    if (result == 0) {
        PyErr_Format(PyExc_ValueError,
                     "Unsupported protocol version 0x%x", v);
        return -1;
    }
    return 0;
}

static PyObject *
get_minimum_version(PySSLContext *self, void *c)
{
    int v = SSL_CTX_get_min_proto_version(self->ctx);
    if (v == 0) {
        v = PY_PROTO_MINIMUM_SUPPORTED;
    }
    return PyLong_FromLong(v);
}

static int
set_minimum_version(PySSLContext *self, PyObject *arg, void *c)
{
    return set_min_max_proto_version(self, arg, 0);
}

static PyObject *
get_maximum_version(PySSLContext *self, void *c)
{
    int v = SSL_CTX_get_max_proto_version(self->ctx);
    if (v == 0) {
        v = PY_PROTO_MAXIMUM_SUPPORTED;
    }
    return PyLong_FromLong(v);
}

static int
set_maximum_version(PySSLContext *self, PyObject *arg, void *c)
{
    return set_min_max_proto_version(self, arg, 1);
}

#ifdef TLS1_3_VERSION
static PyObject *
get_num_tickets(PySSLContext *self, void *c)
{
    return PyLong_FromSize_t(SSL_CTX_get_num_tickets(self->ctx));
}

static int
set_num_tickets(PySSLContext *self, PyObject *arg, void *c)
{
    long num;
    if (!PyArg_Parse(arg, "l", &num))
        return -1;
    if (num < 0) {
        PyErr_SetString(PyExc_ValueError, "value must be non-negative");
        return -1;
    }
    if (self->protocol != PY_SSL_VERSION_TLS_SERVER) {
        PyErr_SetString(PyExc_ValueError,
                        "SSLContext is not a server context.");
        return -1;
    }
    if (SSL_CTX_set_num_tickets(self->ctx, num) != 1) {
        PyErr_SetString(PyExc_ValueError, "failed to set num tickets.");
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(PySSLContext_num_tickets_doc,
"Control the number of TLSv1.3 session tickets");
#endif /* TLS1_3_VERSION */

static PyObject *
get_security_level(PySSLContext *self, void *c)
{
    return PyLong_FromLong(SSL_CTX_get_security_level(self->ctx));
}
PyDoc_STRVAR(PySSLContext_security_level_doc, "The current security level");

static PyObject *
get_options(PySSLContext *self, void *c)
{
    uint64_t options = SSL_CTX_get_options(self->ctx);
    Py_BUILD_ASSERT(sizeof(unsigned long long) >= sizeof(options));
    return PyLong_FromUnsignedLongLong(options);
}

static int
set_options(PySSLContext *self, PyObject *arg, void *c)
{
    PyObject *new_opts_obj;
    unsigned long long new_opts_arg;
    uint64_t new_opts, opts, clear, set;
    uint64_t opt_no = (
        SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 |
        SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3
    );

    if (!PyArg_Parse(arg, "O!", &PyLong_Type, &new_opts_obj)) {
        return -1;
    }
    new_opts_arg = PyLong_AsUnsignedLongLong(new_opts_obj);
    if (new_opts_arg == (unsigned long long)-1 && PyErr_Occurred()) {
        return -1;
    }
    Py_BUILD_ASSERT(sizeof(new_opts) >= sizeof(new_opts_arg));
    new_opts = (uint64_t)new_opts_arg;

    opts = SSL_CTX_get_options(self->ctx);
    clear = opts & ~new_opts;
    set = ~opts & new_opts;

    if ((set & opt_no) != 0) {
        if (_ssl_deprecated("ssl.OP_NO_SSL*/ssl.OP_NO_TLS* options are "
                            "deprecated", 2) < 0) {
            return -1;
        }
    }
    if (clear) {
        SSL_CTX_clear_options(self->ctx, clear);
    }
    if (set) {
        SSL_CTX_set_options(self->ctx, set);
    }
    return 0;
}

static PyObject *
get_host_flags(PySSLContext *self, void *c)
{
    return PyLong_FromUnsignedLong(self->hostflags);
}

static int
set_host_flags(PySSLContext *self, PyObject *arg, void *c)
{
    X509_VERIFY_PARAM *param;
    unsigned int new_flags = 0;

    if (!PyArg_Parse(arg, "I", &new_flags))
        return -1;

    param = SSL_CTX_get0_param(self->ctx);
    self->hostflags = new_flags;
    X509_VERIFY_PARAM_set_hostflags(param, new_flags);
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
    int check_hostname;
    if (!PyArg_Parse(arg, "p", &check_hostname))
        return -1;
    if (check_hostname &&
            SSL_CTX_get_verify_mode(self->ctx) == SSL_VERIFY_NONE) {
        /* check_hostname = True sets verify_mode = CERT_REQUIRED */
        if (_set_verify_mode(self, PY_SSL_CERT_REQUIRED) == -1) {
            return -1;
        }
    }
    self->check_hostname = check_hostname;
    return 0;
}

static PyObject *
get_post_handshake_auth(PySSLContext *self, void *c) {
#if TLS1_3_VERSION
    return PyBool_FromLong(self->post_handshake_auth);
#else
    Py_RETURN_NONE;
#endif
}

#if TLS1_3_VERSION
static int
set_post_handshake_auth(PySSLContext *self, PyObject *arg, void *c) {
    if (arg == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    int pha = PyObject_IsTrue(arg);

    if (pha == -1) {
        return -1;
    }
    self->post_handshake_auth = pha;

    /* bpo-37428: newPySSLSocket() sets SSL_VERIFY_POST_HANDSHAKE flag for
     * server sockets and SSL_set_post_handshake_auth() for client. */

    return 0;
}
#endif

static PyObject *
get_protocol(PySSLContext *self, void *c) {
    return PyLong_FromLong(self->protocol);
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
        password_bytes = PyUnicode_AsUTF8String(password);
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

    if (pw_info->error) {
        /* already failed previously. OpenSSL 3.0.0-alpha14 invokes the
         * callback multiple times which can lead to fatal Python error in
         * exception check. */
        goto error;
    }

    if (pw_info->callable) {
        fn_ret = PyObject_CallNoArgs(pw_info->callable);
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

/*[clinic input]
_ssl._SSLContext.load_cert_chain
    certfile: object
    keyfile: object = None
    password: object = None

[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_load_cert_chain_impl(PySSLContext *self, PyObject *certfile,
                                      PyObject *keyfile, PyObject *password)
/*[clinic end generated code: output=9480bc1c380e2095 input=30bc7e967ea01a58]*/
{
    PyObject *certfile_bytes = NULL, *keyfile_bytes = NULL;
    pem_password_cb *orig_passwd_cb = SSL_CTX_get_default_passwd_cb(self->ctx);
    void *orig_passwd_userdata = SSL_CTX_get_default_passwd_cb_userdata(self->ctx);
    _PySSLPasswordInfo pw_info = { NULL, NULL, NULL, 0, 0 };
    int r;

    errno = 0;
    ERR_clear_error();
    if (keyfile == Py_None)
        keyfile = NULL;
    if (!PyUnicode_FSConverter(certfile, &certfile_bytes)) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_SetString(PyExc_TypeError,
                            "certfile should be a valid filesystem path");
        }
        return NULL;
    }
    if (keyfile && !PyUnicode_FSConverter(keyfile, &keyfile_bytes)) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_SetString(PyExc_TypeError,
                            "keyfile should be a valid filesystem path");
        }
        goto error;
    }
    if (password != Py_None) {
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
    r = SSL_CTX_use_certificate_chain_file(self->ctx,
        PyBytes_AS_STRING(certfile_bytes));
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    if (r != 1) {
        if (pw_info.error) {
            ERR_clear_error();
            /* the password callback has already set the error information */
        }
        else if (errno != 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            ERR_clear_error();
        }
        else {
            _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        }
        goto error;
    }
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info.thread_state);
    r = SSL_CTX_use_PrivateKey_file(self->ctx,
        PyBytes_AS_STRING(keyfile ? keyfile_bytes : certfile_bytes),
        SSL_FILETYPE_PEM);
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    Py_CLEAR(keyfile_bytes);
    Py_CLEAR(certfile_bytes);
    if (r != 1) {
        if (pw_info.error) {
            ERR_clear_error();
            /* the password callback has already set the error information */
        }
        else if (errno != 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            ERR_clear_error();
        }
        else {
            _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        }
        goto error;
    }
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info.thread_state);
    r = SSL_CTX_check_private_key(self->ctx);
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    if (r != 1) {
        _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        goto error;
    }
    SSL_CTX_set_default_passwd_cb(self->ctx, orig_passwd_cb);
    SSL_CTX_set_default_passwd_cb_userdata(self->ctx, orig_passwd_userdata);
    PyMem_Free(pw_info.password);
    Py_RETURN_NONE;

error:
    SSL_CTX_set_default_passwd_cb(self->ctx, orig_passwd_cb);
    SSL_CTX_set_default_passwd_cb_userdata(self->ctx, orig_passwd_userdata);
    PyMem_Free(pw_info.password);
    Py_XDECREF(keyfile_bytes);
    Py_XDECREF(certfile_bytes);
    return NULL;
}

/* internal helper function, returns -1 on error
 */
static int
_add_ca_certs(PySSLContext *self, const void *data, Py_ssize_t len,
              int filetype)
{
    BIO *biobuf = NULL;
    X509_STORE *store;
    int retval = -1, err, loaded = 0;

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
        _setSSLError(get_state_ctx(self), "Can't allocate buffer", 0, __FILE__, __LINE__);
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
                                     SSL_CTX_get_default_passwd_cb(self->ctx),
                                     SSL_CTX_get_default_passwd_cb_userdata(self->ctx)
                                    );
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
    if (loaded == 0) {
        const char *msg = NULL;
        if (filetype == SSL_FILETYPE_PEM) {
            msg = "no start line: cadata does not contain a certificate";
        } else {
            msg = "not enough data: cadata does not contain a certificate";
        }
        _setSSLError(get_state_ctx(self), msg, 0, __FILE__, __LINE__);
        retval = -1;
    } else if ((filetype == SSL_FILETYPE_ASN1) &&
                    (ERR_GET_LIB(err) == ERR_LIB_ASN1) &&
                    (ERR_GET_REASON(err) == ASN1_R_HEADER_TOO_LONG)) {
        /* EOF ASN1 file, not an error */
        ERR_clear_error();
        retval = 0;
    } else if ((filetype == SSL_FILETYPE_PEM) &&
                   (ERR_GET_LIB(err) == ERR_LIB_PEM) &&
                   (ERR_GET_REASON(err) == PEM_R_NO_START_LINE)) {
        /* EOF PEM file, not an error */
        ERR_clear_error();
        retval = 0;
    } else if (err != 0) {
        _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        retval = -1;
    } else {
        retval = 0;
    }

    BIO_free(biobuf);
    return retval;
}


/*[clinic input]
_ssl._SSLContext.load_verify_locations
    cafile: object = None
    capath: object = None
    cadata: object = None

[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_load_verify_locations_impl(PySSLContext *self,
                                            PyObject *cafile,
                                            PyObject *capath,
                                            PyObject *cadata)
/*[clinic end generated code: output=454c7e41230ca551 input=42ecfe258233e194]*/
{
    PyObject *cafile_bytes = NULL, *capath_bytes = NULL;
    const char *cafile_buf = NULL, *capath_buf = NULL;
    int r = 0, ok = 1;

    errno = 0;
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
    if (cafile && !PyUnicode_FSConverter(cafile, &cafile_bytes)) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_SetString(PyExc_TypeError,
                            "cafile should be a valid filesystem path");
        }
        goto error;
    }
    if (capath && !PyUnicode_FSConverter(capath, &capath_bytes)) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_SetString(PyExc_TypeError,
                            "capath should be a valid filesystem path");
        }
        goto error;
    }

    /* validate cadata type and load cadata */
    if (cadata) {
        if (PyUnicode_Check(cadata)) {
            PyObject *cadata_ascii = PyUnicode_AsASCIIString(cadata);
            if (cadata_ascii == NULL) {
                if (PyErr_ExceptionMatches(PyExc_UnicodeEncodeError)) {
                    goto invalid_cadata;
                }
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
        else if (PyObject_CheckBuffer(cadata)) {
            Py_buffer buf;
            if (PyObject_GetBuffer(cadata, &buf, PyBUF_SIMPLE)) {
                goto error;
            }
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
        }
        else {
  invalid_cadata:
            PyErr_SetString(PyExc_TypeError,
                            "cadata should be an ASCII string or a "
                            "bytes-like object");
            goto error;
        }
    }

    /* load cafile or capath */
    if (cafile || capath) {
        if (cafile)
            cafile_buf = PyBytes_AS_STRING(cafile_bytes);
        if (capath)
            capath_buf = PyBytes_AS_STRING(capath_bytes);
        PySSL_BEGIN_ALLOW_THREADS
        r = SSL_CTX_load_verify_locations(self->ctx, cafile_buf, capath_buf);
        PySSL_END_ALLOW_THREADS
        if (r != 1) {
            if (errno != 0) {
                PyErr_SetFromErrno(PyExc_OSError);
                ERR_clear_error();
            }
            else {
                _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
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

/*[clinic input]
_ssl._SSLContext.load_dh_params
    path as filepath: object
    /

[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_load_dh_params(PySSLContext *self, PyObject *filepath)
/*[clinic end generated code: output=1c8e57a38e055af0 input=c8871f3c796ae1d6]*/
{
    FILE *f;
    DH *dh;

    f = _Py_fopen_obj(filepath, "rb");
    if (f == NULL)
        return NULL;

    errno = 0;
    PySSL_BEGIN_ALLOW_THREADS
    dh = PEM_read_DHparams(f, NULL, NULL, NULL);
    fclose(f);
    PySSL_END_ALLOW_THREADS
    if (dh == NULL) {
        if (errno != 0) {
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, filepath);
            ERR_clear_error();
        }
        else {
            _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        }
        return NULL;
    }
    if (!SSL_CTX_set_tmp_dh(self->ctx, dh)) {
        DH_free(dh);
        return _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
    }
    DH_free(dh);
    Py_RETURN_NONE;
}

/*[clinic input]
_ssl._SSLContext._wrap_socket
    sock: object(subclass_of="get_state_ctx(self)->Sock_Type")
    server_side: int
    server_hostname as hostname_obj: object = None
    *
    owner: object = None
    session: object = None

[clinic start generated code]*/

static PyObject *
_ssl__SSLContext__wrap_socket_impl(PySSLContext *self, PyObject *sock,
                                   int server_side, PyObject *hostname_obj,
                                   PyObject *owner, PyObject *session)
/*[clinic end generated code: output=f103f238633940b4 input=f5916eadbc6eae81]*/
{
    char *hostname = NULL;
    PyObject *res;

    /* server_hostname is either None (or absent), or to be encoded
       as IDN A-label (ASCII str) without NULL bytes. */
    if (hostname_obj != Py_None) {
        if (!PyArg_Parse(hostname_obj, "es", "ascii", &hostname))
            return NULL;
    }

    res = (PyObject *) newPySSLSocket(self, (PySocketSockObject *)sock,
                                      server_side, hostname,
                                      owner, session,
                                      NULL, NULL);
    if (hostname != NULL)
        PyMem_Free(hostname);
    return res;
}

/*[clinic input]
_ssl._SSLContext._wrap_bio
    incoming: object(subclass_of="get_state_ctx(self)->PySSLMemoryBIO_Type", type="PySSLMemoryBIO *")
    outgoing: object(subclass_of="get_state_ctx(self)->PySSLMemoryBIO_Type", type="PySSLMemoryBIO *")
    server_side: int
    server_hostname as hostname_obj: object = None
    *
    owner: object = None
    session: object = None

[clinic start generated code]*/

static PyObject *
_ssl__SSLContext__wrap_bio_impl(PySSLContext *self, PySSLMemoryBIO *incoming,
                                PySSLMemoryBIO *outgoing, int server_side,
                                PyObject *hostname_obj, PyObject *owner,
                                PyObject *session)
/*[clinic end generated code: output=5c5d6d9b41f99332 input=331edeec9c738382]*/
{
    char *hostname = NULL;
    PyObject *res;

    /* server_hostname is either None (or absent), or to be encoded
       as IDN A-label (ASCII str) without NULL bytes. */
    if (hostname_obj != Py_None) {
        if (!PyArg_Parse(hostname_obj, "es", "ascii", &hostname))
            return NULL;
    }

    res = (PyObject *) newPySSLSocket(self, NULL, server_side, hostname,
                                      owner, session,
                                      incoming, outgoing);

    PyMem_Free(hostname);
    return res;
}

/*[clinic input]
_ssl._SSLContext.session_stats
[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_session_stats_impl(PySSLContext *self)
/*[clinic end generated code: output=0d96411c42893bfb input=7e0a81fb11102c8b]*/
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

/*[clinic input]
_ssl._SSLContext.set_default_verify_paths
[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_set_default_verify_paths_impl(PySSLContext *self)
/*[clinic end generated code: output=0bee74e6e09deaaa input=35f3408021463d74]*/
{
    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = SSL_CTX_set_default_verify_paths(self->ctx);
    Py_END_ALLOW_THREADS
    if (!rc) {
        _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_ssl._SSLContext.set_ecdh_curve
    name: object
    /

[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_set_ecdh_curve(PySSLContext *self, PyObject *name)
/*[clinic end generated code: output=23022c196e40d7d2 input=c2bafb6f6e34726b]*/
{
    PyObject *name_bytes;
    int nid;
    if (!PyUnicode_FSConverter(name, &name_bytes))
        return NULL;
    assert(PyBytes_Check(name_bytes));
    nid = OBJ_sn2nid(PyBytes_AS_STRING(name_bytes));
    Py_DECREF(name_bytes);
    if (nid == 0) {
        PyErr_Format(PyExc_ValueError,
                     "unknown elliptic curve name %R", name);
        return NULL;
    }
#if OPENSSL_VERSION_MAJOR < 3
    EC_KEY *key = EC_KEY_new_by_curve_name(nid);
    if (key == NULL) {
        _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    SSL_CTX_set_tmp_ecdh(self->ctx, key);
    EC_KEY_free(key);
#else
    if (!SSL_CTX_set1_groups(self->ctx, &nid, 1)) {
        _setSSLError(get_state_ctx(self), NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
#endif
    Py_RETURN_NONE;
}

static int
_servername_callback(SSL *s, int *al, void *args)
{
    int ret;
    PySSLContext *sslctx = (PySSLContext *) args;
    PySSLSocket *ssl;
    PyObject *result;
    /* The high-level ssl.SSLSocket object */
    PyObject *ssl_socket;
    const char *servername = SSL_get_servername(s, TLSEXT_NAMETYPE_host_name);
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (sslctx->set_sni_cb == NULL) {
        /* remove race condition in this the call back while if removing the
         * callback is in progress */
        PyGILState_Release(gstate);
        return SSL_TLSEXT_ERR_OK;
    }

    ssl = SSL_get_app_data(s);
    assert(Py_IS_TYPE(ssl, get_state_ctx(sslctx)->PySSLSocket_Type));

    /* The servername callback expects an argument that represents the current
     * SSL connection and that has a .context attribute that can be changed to
     * identify the requested hostname. Since the official API is the Python
     * level API we want to pass the callback a Python level object rather than
     * a _ssl.SSLSocket instance. If there's an "owner" (typically an
     * SSLObject) that will be passed. Otherwise if there's a socket then that
     * will be passed. If both do not exist only then the C-level object is
     * passed. */
    if (ssl->owner)
        ssl_socket = PyWeakref_GetObject(ssl->owner);
    else if (ssl->Socket)
        ssl_socket = PyWeakref_GetObject(ssl->Socket);
    else
        ssl_socket = (PyObject *) ssl;

    Py_INCREF(ssl_socket);
    if (ssl_socket == Py_None)
        goto error;

    if (servername == NULL) {
        result = PyObject_CallFunctionObjArgs(sslctx->set_sni_cb, ssl_socket,
                                              Py_None, sslctx, NULL);
    }
    else {
        PyObject *servername_bytes;
        PyObject *servername_str;

        servername_bytes = PyBytes_FromString(servername);
        if (servername_bytes == NULL) {
            PyErr_WriteUnraisable((PyObject *) sslctx);
            goto error;
        }
        /* server_hostname was encoded to an A-label by our caller; put it
         * back into a str object, but still as an A-label (bpo-28414)
         */
        servername_str = PyUnicode_FromEncodedObject(servername_bytes, "ascii", NULL);
        if (servername_str == NULL) {
            PyErr_WriteUnraisable(servername_bytes);
            Py_DECREF(servername_bytes);
            goto error;
        }
        Py_DECREF(servername_bytes);
        result = PyObject_CallFunctionObjArgs(
            sslctx->set_sni_cb, ssl_socket, servername_str,
            sslctx, NULL);
        Py_DECREF(servername_str);
    }
    Py_DECREF(ssl_socket);

    if (result == NULL) {
        PyErr_WriteUnraisable(sslctx->set_sni_cb);
        *al = SSL_AD_HANDSHAKE_FAILURE;
        ret = SSL_TLSEXT_ERR_ALERT_FATAL;
    }
    else {
        /* Result may be None, a SSLContext or an integer
         * None and SSLContext are OK, integer or other values are an error.
         */
        if (result == Py_None) {
            ret = SSL_TLSEXT_ERR_OK;
        } else {
            *al = (int) PyLong_AsLong(result);
            if (PyErr_Occurred()) {
                PyErr_WriteUnraisable(result);
                *al = SSL_AD_INTERNAL_ERROR;
            }
            ret = SSL_TLSEXT_ERR_ALERT_FATAL;
        }
        Py_DECREF(result);
    }

    PyGILState_Release(gstate);
    return ret;

error:
    Py_DECREF(ssl_socket);
    *al = SSL_AD_INTERNAL_ERROR;
    ret = SSL_TLSEXT_ERR_ALERT_FATAL;
    PyGILState_Release(gstate);
    return ret;
}

static PyObject *
get_sni_callback(PySSLContext *self, void *c)
{
    PyObject *cb = self->set_sni_cb;
    if (cb == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(cb);
    return cb;
}

static int
set_sni_callback(PySSLContext *self, PyObject *arg, void *c)
{
    if (self->protocol == PY_SSL_VERSION_TLS_CLIENT) {
        PyErr_SetString(PyExc_ValueError,
                        "sni_callback cannot be set on TLS_CLIENT context");
        return -1;
    }
    Py_CLEAR(self->set_sni_cb);
    if (arg == Py_None) {
        SSL_CTX_set_tlsext_servername_callback(self->ctx, NULL);
    }
    else {
        if (!PyCallable_Check(arg)) {
            SSL_CTX_set_tlsext_servername_callback(self->ctx, NULL);
            PyErr_SetString(PyExc_TypeError,
                            "not a callable object");
            return -1;
        }
        Py_INCREF(arg);
        self->set_sni_cb = arg;
        SSL_CTX_set_tlsext_servername_callback(self->ctx, _servername_callback);
        SSL_CTX_set_tlsext_servername_arg(self->ctx, self);
    }
    return 0;
}

#if OPENSSL_VERSION_NUMBER < 0x30300000L
static X509_OBJECT *x509_object_dup(const X509_OBJECT *obj)
{
    int ok;
    X509_OBJECT *ret = X509_OBJECT_new();
    if (ret == NULL) {
        return NULL;
    }
    switch (X509_OBJECT_get_type(obj)) {
        case X509_LU_X509:
            ok = X509_OBJECT_set1_X509(ret, X509_OBJECT_get0_X509(obj));
            break;
        case X509_LU_CRL:
            /* X509_OBJECT_get0_X509_CRL was not const-correct prior to 3.0.*/
            ok = X509_OBJECT_set1_X509_CRL(
                ret, X509_OBJECT_get0_X509_CRL((X509_OBJECT *)obj));
            break;
        default:
            /* We cannot duplicate unrecognized types in a polyfill, but it is
             * safe to leave an empty object. The caller will ignore it. */
            ok = 1;
            break;
    }
    if (!ok) {
        X509_OBJECT_free(ret);
        return NULL;
    }
    return ret;
}

static STACK_OF(X509_OBJECT) *
X509_STORE_get1_objects(X509_STORE *store)
{
    STACK_OF(X509_OBJECT) *ret;
    if (!X509_STORE_lock(store)) {
        return NULL;
    }
    ret = sk_X509_OBJECT_deep_copy(X509_STORE_get0_objects(store),
                                   x509_object_dup, X509_OBJECT_free);
    X509_STORE_unlock(store);
    return ret;
}
#endif

PyDoc_STRVAR(PySSLContext_sni_callback_doc,
"Set a callback that will be called when a server name is provided by the SSL/TLS client in the SNI extension.\n\
\n\
If the argument is None then the callback is disabled. The method is called\n\
with the SSLSocket, the server name as a string, and the SSLContext object.\n\
See RFC 6066 for details of the SNI extension.");

/*[clinic input]
_ssl._SSLContext.cert_store_stats

Returns quantities of loaded X.509 certificates.

X.509 certificates with a CA extension and certificate revocation lists
inside the context's cert store.

NOTE: Certificates in a capath directory aren't loaded unless they have
been used at least once.
[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_cert_store_stats_impl(PySSLContext *self)
/*[clinic end generated code: output=5f356f4d9cca874d input=eb40dd0f6d0e40cf]*/
{
    X509_STORE *store;
    STACK_OF(X509_OBJECT) *objs;
    X509_OBJECT *obj;
    int x509 = 0, crl = 0, ca = 0, i;

    store = SSL_CTX_get_cert_store(self->ctx);
    objs = X509_STORE_get1_objects(store);
    if (objs == NULL) {
        PyErr_SetString(PyExc_MemoryError, "failed to query cert store");
        return NULL;
    }

    for (i = 0; i < sk_X509_OBJECT_num(objs); i++) {
        obj = sk_X509_OBJECT_value(objs, i);
        switch (X509_OBJECT_get_type(obj)) {
            case X509_LU_X509:
                x509++;
                if (X509_check_ca(X509_OBJECT_get0_X509(obj))) {
                    ca++;
                }
                break;
            case X509_LU_CRL:
                crl++;
                break;
            default:
                /* Ignore unrecognized types. */
                break;
        }
    }
    sk_X509_OBJECT_pop_free(objs, X509_OBJECT_free);
    return Py_BuildValue("{sisisi}", "x509", x509, "crl", crl,
        "x509_ca", ca);
}

/*[clinic input]
_ssl._SSLContext.get_ca_certs
    binary_form: bool = False

Returns a list of dicts with information of loaded CA certs.

If the optional argument is True, returns a DER-encoded copy of the CA
certificate.

NOTE: Certificates in a capath directory aren't loaded unless they have
been used at least once.
[clinic start generated code]*/

static PyObject *
_ssl__SSLContext_get_ca_certs_impl(PySSLContext *self, int binary_form)
/*[clinic end generated code: output=0d58f148f37e2938 input=6887b5a09b7f9076]*/
{
    X509_STORE *store;
    STACK_OF(X509_OBJECT) *objs;
    PyObject *ci = NULL, *rlist = NULL;
    int i;

    if ((rlist = PyList_New(0)) == NULL) {
        return NULL;
    }

    store = SSL_CTX_get_cert_store(self->ctx);
    objs = X509_STORE_get1_objects(store);
    if (objs == NULL) {
        PyErr_SetString(PyExc_MemoryError, "failed to query cert store");
        goto error;
    }

    for (i = 0; i < sk_X509_OBJECT_num(objs); i++) {
        X509_OBJECT *obj;
        X509 *cert;

        obj = sk_X509_OBJECT_value(objs, i);
        if (X509_OBJECT_get_type(obj) != X509_LU_X509) {
            /* not a x509 cert */
            continue;
        }
        /* CA for any purpose */
        cert = X509_OBJECT_get0_X509(obj);
        if (!X509_check_ca(cert)) {
            continue;
        }
        if (binary_form) {
            ci = _certificate_to_der(get_state_ctx(self), cert);
        } else {
            ci = _decode_certificate(get_state_ctx(self), cert);
        }
        if (ci == NULL) {
            goto error;
        }
        if (PyList_Append(rlist, ci) == -1) {
            goto error;
        }
        Py_CLEAR(ci);
    }
    sk_X509_OBJECT_pop_free(objs, X509_OBJECT_free);
    return rlist;

  error:
    sk_X509_OBJECT_pop_free(objs, X509_OBJECT_free);
    Py_XDECREF(ci);
    Py_XDECREF(rlist);
    return NULL;
}


static PyGetSetDef context_getsetlist[] = {
    {"check_hostname", (getter) get_check_hostname,
                       (setter) set_check_hostname, NULL},
    {"_host_flags", (getter) get_host_flags,
                    (setter) set_host_flags, NULL},
    {"minimum_version", (getter) get_minimum_version,
                        (setter) set_minimum_version, NULL},
    {"maximum_version", (getter) get_maximum_version,
                        (setter) set_maximum_version, NULL},
    {"keylog_filename", (getter) _PySSLContext_get_keylog_filename,
                        (setter) _PySSLContext_set_keylog_filename, NULL},
    {"_msg_callback", (getter) _PySSLContext_get_msg_callback,
                      (setter) _PySSLContext_set_msg_callback, NULL},
    {"sni_callback", (getter) get_sni_callback,
                     (setter) set_sni_callback, PySSLContext_sni_callback_doc},
#ifdef TLS1_3_VERSION
    {"num_tickets", (getter) get_num_tickets,
                    (setter) set_num_tickets, PySSLContext_num_tickets_doc},
#endif
    {"options", (getter) get_options,
                (setter) set_options, NULL},
    {"post_handshake_auth", (getter) get_post_handshake_auth,
#ifdef TLS1_3_VERSION
                            (setter) set_post_handshake_auth,
#else
                            NULL,
#endif
                            NULL},
    {"protocol", (getter) get_protocol,
                 NULL, NULL},
    {"verify_flags", (getter) get_verify_flags,
                     (setter) set_verify_flags, NULL},
    {"verify_mode", (getter) get_verify_mode,
                    (setter) set_verify_mode, NULL},
    {"security_level", (getter) get_security_level,
                       NULL, PySSLContext_security_level_doc},
    {NULL},            /* sentinel */
};

static struct PyMethodDef context_methods[] = {
    _SSL__SSLCONTEXT__WRAP_SOCKET_METHODDEF
    _SSL__SSLCONTEXT__WRAP_BIO_METHODDEF
    _SSL__SSLCONTEXT_SET_CIPHERS_METHODDEF
    _SSL__SSLCONTEXT__SET_ALPN_PROTOCOLS_METHODDEF
    _SSL__SSLCONTEXT_LOAD_CERT_CHAIN_METHODDEF
    _SSL__SSLCONTEXT_LOAD_DH_PARAMS_METHODDEF
    _SSL__SSLCONTEXT_LOAD_VERIFY_LOCATIONS_METHODDEF
    _SSL__SSLCONTEXT_SESSION_STATS_METHODDEF
    _SSL__SSLCONTEXT_SET_DEFAULT_VERIFY_PATHS_METHODDEF
    _SSL__SSLCONTEXT_SET_ECDH_CURVE_METHODDEF
    _SSL__SSLCONTEXT_CERT_STORE_STATS_METHODDEF
    _SSL__SSLCONTEXT_GET_CA_CERTS_METHODDEF
    _SSL__SSLCONTEXT_GET_CIPHERS_METHODDEF
    {NULL, NULL}        /* sentinel */
};

static PyType_Slot PySSLContext_slots[] = {
    {Py_tp_methods, context_methods},
    {Py_tp_getset, context_getsetlist},
    {Py_tp_new, _ssl__SSLContext},
    {Py_tp_dealloc, context_dealloc},
    {Py_tp_traverse, context_traverse},
    {Py_tp_clear, context_clear},
    {0, 0},
};

static PyType_Spec PySSLContext_spec = {
    .name = "_ssl._SSLContext",
    .basicsize = sizeof(PySSLContext),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = PySSLContext_slots,
};


/*
 * MemoryBIO objects
 */

/*[clinic input]
@classmethod
_ssl.MemoryBIO.__new__

[clinic start generated code]*/

static PyObject *
_ssl_MemoryBIO_impl(PyTypeObject *type)
/*[clinic end generated code: output=8820a58db78330ac input=26d22e4909ecb1b5]*/
{
    BIO *bio;
    PySSLMemoryBIO *self;

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        PyErr_SetString(PyExc_MemoryError, "failed to allocate BIO");
        return NULL;
    }
    /* Since our BIO is non-blocking an empty read() does not indicate EOF,
     * just that no data is currently available. The SSL routines should retry
     * the read, which we can achieve by calling BIO_set_retry_read(). */
    BIO_set_retry_read(bio);
    BIO_set_mem_eof_return(bio, -1);

    assert(type != NULL && type->tp_alloc != NULL);
    self = (PySSLMemoryBIO *) type->tp_alloc(type, 0);
    if (self == NULL) {
        BIO_free(bio);
        return NULL;
    }
    self->bio = bio;
    self->eof_written = 0;

    return (PyObject *) self;
}

static int
memory_bio_traverse(PySSLMemoryBIO *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static void
memory_bio_dealloc(PySSLMemoryBIO *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    BIO_free(self->bio);
    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
memory_bio_get_pending(PySSLMemoryBIO *self, void *c)
{
    return PyLong_FromSize_t(BIO_ctrl_pending(self->bio));
}

PyDoc_STRVAR(PySSL_memory_bio_pending_doc,
"The number of bytes pending in the memory BIO.");

static PyObject *
memory_bio_get_eof(PySSLMemoryBIO *self, void *c)
{
    return PyBool_FromLong((BIO_ctrl_pending(self->bio) == 0)
                           && self->eof_written);
}

PyDoc_STRVAR(PySSL_memory_bio_eof_doc,
"Whether the memory BIO is at EOF.");

/*[clinic input]
_ssl.MemoryBIO.read
    size as len: int = -1
    /

Read up to size bytes from the memory BIO.

If size is not specified, read the entire buffer.
If the return value is an empty bytes instance, this means either
EOF or that no data is available. Use the "eof" property to
distinguish between the two.
[clinic start generated code]*/

static PyObject *
_ssl_MemoryBIO_read_impl(PySSLMemoryBIO *self, int len)
/*[clinic end generated code: output=a657aa1e79cd01b3 input=574d7be06a902366]*/
{
    int avail, nbytes;
    PyObject *result;

    avail = (int)Py_MIN(BIO_ctrl_pending(self->bio), INT_MAX);
    if ((len < 0) || (len > avail))
        len = avail;

    result = PyBytes_FromStringAndSize(NULL, len);
    if ((result == NULL) || (len == 0))
        return result;

    nbytes = BIO_read(self->bio, PyBytes_AS_STRING(result), len);
    if (nbytes < 0) {
        _sslmodulestate *state = get_state_mbio(self);
        Py_DECREF(result);
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);
        return NULL;
    }

    /* There should never be any short reads but check anyway. */
    if (nbytes < len) {
        _PyBytes_Resize(&result, nbytes);
    }

    return result;
}

/*[clinic input]
_ssl.MemoryBIO.write
    b: Py_buffer
    /

Writes the bytes b into the memory BIO.

Returns the number of bytes written.
[clinic start generated code]*/

static PyObject *
_ssl_MemoryBIO_write_impl(PySSLMemoryBIO *self, Py_buffer *b)
/*[clinic end generated code: output=156ec59110d75935 input=e45757b3e17c4808]*/
{
    int nbytes;

    if (b->len > INT_MAX) {
        PyErr_Format(PyExc_OverflowError,
                     "string longer than %d bytes", INT_MAX);
        return NULL;
    }

    if (self->eof_written) {
        PyObject *module = PyType_GetModule(Py_TYPE(self));
        if (module == NULL)
            return NULL;
        PyErr_SetString(get_ssl_state(module)->PySSLErrorObject,
                        "cannot write() after write_eof()");
        return NULL;
    }

    nbytes = BIO_write(self->bio, b->buf, (int)b->len);
    if (nbytes < 0) {
        _sslmodulestate *state = get_state_mbio(self);
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);
        return NULL;
    }

    return PyLong_FromLong(nbytes);
}

/*[clinic input]
_ssl.MemoryBIO.write_eof

Write an EOF marker to the memory BIO.

When all data has been read, the "eof" property will be True.
[clinic start generated code]*/

static PyObject *
_ssl_MemoryBIO_write_eof_impl(PySSLMemoryBIO *self)
/*[clinic end generated code: output=d4106276ccd1ed34 input=56a945f1d29e8bd6]*/
{
    self->eof_written = 1;
    /* After an EOF is written, a zero return from read() should be a real EOF
     * i.e. it should not be retried. Clear the SHOULD_RETRY flag. */
    BIO_clear_retry_flags(self->bio);
    BIO_set_mem_eof_return(self->bio, 0);

    Py_RETURN_NONE;
}

static PyGetSetDef memory_bio_getsetlist[] = {
    {"pending", (getter) memory_bio_get_pending, NULL,
                PySSL_memory_bio_pending_doc},
    {"eof", (getter) memory_bio_get_eof, NULL,
            PySSL_memory_bio_eof_doc},
    {NULL},            /* sentinel */
};

static struct PyMethodDef memory_bio_methods[] = {
    _SSL_MEMORYBIO_READ_METHODDEF
    _SSL_MEMORYBIO_WRITE_METHODDEF
    _SSL_MEMORYBIO_WRITE_EOF_METHODDEF
    {NULL, NULL}        /* sentinel */
};

static PyType_Slot PySSLMemoryBIO_slots[] = {
    {Py_tp_methods, memory_bio_methods},
    {Py_tp_getset, memory_bio_getsetlist},
    {Py_tp_new, _ssl_MemoryBIO},
    {Py_tp_dealloc, memory_bio_dealloc},
    {Py_tp_traverse, memory_bio_traverse},
    {0, 0},
};

static PyType_Spec PySSLMemoryBIO_spec = {
    .name = "_ssl.MemoryBIO",
    .basicsize = sizeof(PySSLMemoryBIO),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_HAVE_GC),
    .slots = PySSLMemoryBIO_slots,
};

/*
 * SSL Session object
 */

static void
PySSLSession_dealloc(PySSLSession *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->ctx);
    if (self->session != NULL) {
        SSL_SESSION_free(self->session);
    }
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

static PyObject *
PySSLSession_richcompare(PyObject *left, PyObject *right, int op)
{
    int result;
    PyTypeObject *sesstype = ((PySSLSession*)left)->ctx->state->PySSLSession_Type;

    if (left == NULL || right == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!Py_IS_TYPE(left, sesstype) || !Py_IS_TYPE(right, sesstype)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (left == right) {
        result = 0;
    } else {
        const unsigned char *left_id, *right_id;
        unsigned int left_len, right_len;
        left_id = SSL_SESSION_get_id(((PySSLSession *)left)->session,
                                     &left_len);
        right_id = SSL_SESSION_get_id(((PySSLSession *)right)->session,
                                      &right_len);
        if (left_len == right_len) {
            result = memcmp(left_id, right_id, left_len);
        } else {
            result = 1;
        }
    }

    switch (op) {
      case Py_EQ:
        if (result == 0) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
        break;
      case Py_NE:
        if (result != 0) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
        break;
      case Py_LT:
      case Py_LE:
      case Py_GT:
      case Py_GE:
        Py_RETURN_NOTIMPLEMENTED;
        break;
      default:
        PyErr_BadArgument();
        return NULL;
    }
}

static int
PySSLSession_traverse(PySSLSession *self, visitproc visit, void *arg)
{
    Py_VISIT(self->ctx);
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static int
PySSLSession_clear(PySSLSession *self)
{
    Py_CLEAR(self->ctx);
    return 0;
}


static PyObject *
PySSLSession_get_time(PySSLSession *self, void *closure) {
    return PyLong_FromLong(SSL_SESSION_get_time(self->session));
}

PyDoc_STRVAR(PySSLSession_get_time_doc,
"Session creation time (seconds since epoch).");


static PyObject *
PySSLSession_get_timeout(PySSLSession *self, void *closure) {
    return PyLong_FromLong(SSL_SESSION_get_timeout(self->session));
}

PyDoc_STRVAR(PySSLSession_get_timeout_doc,
"Session timeout (delta in seconds).");


static PyObject *
PySSLSession_get_ticket_lifetime_hint(PySSLSession *self, void *closure) {
    unsigned long hint = SSL_SESSION_get_ticket_lifetime_hint(self->session);
    return PyLong_FromUnsignedLong(hint);
}

PyDoc_STRVAR(PySSLSession_get_ticket_lifetime_hint_doc,
"Ticket life time hint.");


static PyObject *
PySSLSession_get_session_id(PySSLSession *self, void *closure) {
    const unsigned char *id;
    unsigned int len;
    id = SSL_SESSION_get_id(self->session, &len);
    return PyBytes_FromStringAndSize((const char *)id, len);
}

PyDoc_STRVAR(PySSLSession_get_session_id_doc,
"Session id");


static PyObject *
PySSLSession_get_has_ticket(PySSLSession *self, void *closure) {
    if (SSL_SESSION_has_ticket(self->session)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyDoc_STRVAR(PySSLSession_get_has_ticket_doc,
"Does the session contain a ticket?");


static PyGetSetDef PySSLSession_getsetlist[] = {
    {"has_ticket", (getter) PySSLSession_get_has_ticket, NULL,
              PySSLSession_get_has_ticket_doc},
    {"id",   (getter) PySSLSession_get_session_id, NULL,
              PySSLSession_get_session_id_doc},
    {"ticket_lifetime_hint", (getter) PySSLSession_get_ticket_lifetime_hint,
              NULL, PySSLSession_get_ticket_lifetime_hint_doc},
    {"time", (getter) PySSLSession_get_time, NULL,
              PySSLSession_get_time_doc},
    {"timeout", (getter) PySSLSession_get_timeout, NULL,
              PySSLSession_get_timeout_doc},
    {NULL},            /* sentinel */
};

static PyType_Slot PySSLSession_slots[] = {
    {Py_tp_getset,PySSLSession_getsetlist},
    {Py_tp_richcompare, PySSLSession_richcompare},
    {Py_tp_dealloc, PySSLSession_dealloc},
    {Py_tp_traverse, PySSLSession_traverse},
    {Py_tp_clear, PySSLSession_clear},
    {0, 0},
};

static PyType_Spec PySSLSession_spec = {
    .name = "_ssl.SSLSession",
    .basicsize = sizeof(PySSLSession),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = PySSLSession_slots,
};


/* helper routines for seeding the SSL PRNG */
/*[clinic input]
_ssl.RAND_add
    string as view: Py_buffer(accept={str, buffer})
    entropy: double
    /

Mix string into the OpenSSL PRNG state.

entropy (a float) is a lower bound on the entropy contained in
string.  See RFC 4086.
[clinic start generated code]*/

static PyObject *
_ssl_RAND_add_impl(PyObject *module, Py_buffer *view, double entropy)
/*[clinic end generated code: output=e6dd48df9c9024e9 input=5c33017422828f5c]*/
{
    const char *buf;
    Py_ssize_t len, written;

    buf = (const char *)view->buf;
    len = view->len;
    do {
        written = Py_MIN(len, INT_MAX);
        RAND_add(buf, (int)written, entropy);
        buf += written;
        len -= written;
    } while (len);
    Py_RETURN_NONE;
}

static PyObject *
PySSL_RAND(PyObject *module, int len, int pseudo)
{
    int ok;
    PyObject *bytes;
    unsigned long err;
    const char *errstr;
    PyObject *v;

    if (len < 0) {
        PyErr_SetString(PyExc_ValueError, "num must be positive");
        return NULL;
    }

    bytes = PyBytes_FromStringAndSize(NULL, len);
    if (bytes == NULL)
        return NULL;
    if (pseudo) {
        ok = RAND_bytes((unsigned char*)PyBytes_AS_STRING(bytes), len);
        if (ok == 0 || ok == 1)
            return Py_BuildValue("NO", bytes, ok == 1 ? Py_True : Py_False);
    }
    else {
        ok = RAND_bytes((unsigned char*)PyBytes_AS_STRING(bytes), len);
        if (ok == 1)
            return bytes;
    }
    Py_DECREF(bytes);

    err = ERR_get_error();
    errstr = ERR_reason_error_string(err);
    v = Py_BuildValue("(ks)", err, errstr);
    if (v != NULL) {
        PyErr_SetObject(get_ssl_state(module)->PySSLErrorObject, v);
        Py_DECREF(v);
    }
    return NULL;
}

/*[clinic input]
_ssl.RAND_bytes
    n: int
    /

Generate n cryptographically strong pseudo-random bytes.
[clinic start generated code]*/

static PyObject *
_ssl_RAND_bytes_impl(PyObject *module, int n)
/*[clinic end generated code: output=977da635e4838bc7 input=678ddf2872dfebfc]*/
{
    return PySSL_RAND(module, n, 0);
}

/*[clinic input]
_ssl.RAND_pseudo_bytes
    n: int
    /

Generate n pseudo-random bytes.

Return a pair (bytes, is_cryptographic).  is_cryptographic is True
if the bytes generated are cryptographically strong.
[clinic start generated code]*/

static PyObject *
_ssl_RAND_pseudo_bytes_impl(PyObject *module, int n)
/*[clinic end generated code: output=b1509e937000e52d input=58312bd53f9bbdd0]*/
{
    PY_SSL_DEPRECATED("ssl.RAND_pseudo_bytes() is deprecated", 1, NULL);
    return PySSL_RAND(module, n, 1);
}

/*[clinic input]
_ssl.RAND_status

Returns True if the OpenSSL PRNG has been seeded with enough data and False if not.

It is necessary to seed the PRNG with RAND_add() on some platforms before
using the ssl() function.
[clinic start generated code]*/

static PyObject *
_ssl_RAND_status_impl(PyObject *module)
/*[clinic end generated code: output=7e0aaa2d39fdc1ad input=d5ae5aea52f36e01]*/
{
    return PyBool_FromLong(RAND_status());
}

/*[clinic input]
_ssl.get_default_verify_paths

Return search paths and environment vars that are used by SSLContext's set_default_verify_paths() to load default CAs.

The values are 'cert_file_env', 'cert_file', 'cert_dir_env', 'cert_dir'.
[clinic start generated code]*/

static PyObject *
_ssl_get_default_verify_paths_impl(PyObject *module)
/*[clinic end generated code: output=e5b62a466271928b input=5210c953d98c3eb5]*/
{
    PyObject *ofile_env = NULL;
    PyObject *ofile = NULL;
    PyObject *odir_env = NULL;
    PyObject *odir = NULL;

#define CONVERT(info, target) { \
        const char *tmp = (info); \
        target = NULL; \
        if (!tmp) { Py_INCREF(Py_None); target = Py_None; } \
        else if ((target = PyUnicode_DecodeFSDefault(tmp)) == NULL) { \
            target = PyBytes_FromString(tmp); } \
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
asn1obj2py(_sslmodulestate *state, ASN1_OBJECT *obj)
{
    int nid;
    const char *ln, *sn;

    nid = OBJ_obj2nid(obj);
    if (nid == NID_undef) {
        PyErr_Format(PyExc_ValueError, "Unknown object");
        return NULL;
    }
    sn = OBJ_nid2sn(nid);
    ln = OBJ_nid2ln(nid);
    return Py_BuildValue("issN", nid, sn, ln, _asn1obj2py(state, obj, 1));
}

/*[clinic input]
_ssl.txt2obj
    txt: str
    name: bool = False

Lookup NID, short name, long name and OID of an ASN1_OBJECT.

By default objects are looked up by OID. With name=True short and
long name are also matched.
[clinic start generated code]*/

static PyObject *
_ssl_txt2obj_impl(PyObject *module, const char *txt, int name)
/*[clinic end generated code: output=c38e3991347079c1 input=1c1e7d0aa7c48602]*/
{
    PyObject *result = NULL;
    ASN1_OBJECT *obj;

    obj = OBJ_txt2obj(txt, name ? 0 : 1);
    if (obj == NULL) {
        PyErr_Format(PyExc_ValueError, "unknown object '%.100s'", txt);
        return NULL;
    }
    result = asn1obj2py(get_ssl_state(module), obj);
    ASN1_OBJECT_free(obj);
    return result;
}

/*[clinic input]
_ssl.nid2obj
    nid: int
    /

Lookup NID, short name, long name and OID of an ASN1_OBJECT by NID.
[clinic start generated code]*/

static PyObject *
_ssl_nid2obj_impl(PyObject *module, int nid)
/*[clinic end generated code: output=4a98ab691cd4f84a input=51787a3bee7d8f98]*/
{
    PyObject *result = NULL;
    ASN1_OBJECT *obj;

    if (nid < NID_undef) {
        PyErr_SetString(PyExc_ValueError, "NID must be positive.");
        return NULL;
    }
    obj = OBJ_nid2obj(nid);
    if (obj == NULL) {
        PyErr_Format(PyExc_ValueError, "unknown NID %i", nid);
        return NULL;
    }
    result = asn1obj2py(get_ssl_state(module), obj);
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
        x509_asn = PyUnicode_InternFromString("x509_asn");
        if (x509_asn == NULL)
            return NULL;
    }
    if (pkcs_7_asn == NULL) {
        pkcs_7_asn = PyUnicode_InternFromString("pkcs_7_asn");
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
        return PyLong_FromLong(encodingType);
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
    retval = PyFrozenSet_New(NULL);
    if (retval == NULL) {
        goto error;
    }
    for (i = 0; i < usage->cUsageIdentifier; ++i) {
        if (usage->rgpszUsageIdentifier[i]) {
            PyObject *oid;
            int err;
            oid = PyUnicode_FromString(usage->rgpszUsageIdentifier[i]);
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

static HCERTSTORE
ssl_collect_certificates(const char *store_name)
{
/* this function collects the system certificate stores listed in
 * system_stores into a collection certificate store for being
 * enumerated. The store must be readable to be added to the
 * store collection.
 */

    HCERTSTORE hCollectionStore = NULL, hSystemStore = NULL;
    static DWORD system_stores[] = {
        CERT_SYSTEM_STORE_LOCAL_MACHINE,
        CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE,
        CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY,
        CERT_SYSTEM_STORE_CURRENT_USER,
        CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY,
        CERT_SYSTEM_STORE_SERVICES,
        CERT_SYSTEM_STORE_USERS};
    size_t i, storesAdded;
    BOOL result;

    hCollectionStore = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0,
                                     (HCRYPTPROV)NULL, 0, NULL);
    if (!hCollectionStore) {
        return NULL;
    }
    storesAdded = 0;
    for (i = 0; i < sizeof(system_stores) / sizeof(DWORD); i++) {
        hSystemStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0,
                                     (HCRYPTPROV)NULL,
                                     CERT_STORE_READONLY_FLAG |
                                     system_stores[i], store_name);
        if (hSystemStore) {
            result = CertAddStoreToCollection(hCollectionStore, hSystemStore,
                                     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
            if (result) {
                ++storesAdded;
            }
            CertCloseStore(hSystemStore, 0);  /* flag must be 0 */
        }
    }
    if (storesAdded == 0) {
        CertCloseStore(hCollectionStore, CERT_CLOSE_STORE_FORCE_FLAG);
        return NULL;
    }

    return hCollectionStore;
}

/*[clinic input]
_ssl.enum_certificates
    store_name: str

Retrieve certificates from Windows' cert store.

store_name may be one of 'CA', 'ROOT' or 'MY'.  The system may provide
more cert storages, too.  The function returns a list of (bytes,
encoding_type, trust) tuples.  The encoding_type flag can be interpreted
with X509_ASN_ENCODING or PKCS_7_ASN_ENCODING. The trust setting is either
a set of OIDs or the boolean True.
[clinic start generated code]*/

static PyObject *
_ssl_enum_certificates_impl(PyObject *module, const char *store_name)
/*[clinic end generated code: output=5134dc8bb3a3c893 input=915f60d70461ea4e]*/
{
    HCERTSTORE hCollectionStore = NULL;
    PCCERT_CONTEXT pCertCtx = NULL;
    PyObject *keyusage = NULL, *cert = NULL, *enc = NULL, *tup = NULL;
    PyObject *result = NULL;

    result = PySet_New(NULL);
    if (result == NULL) {
        return NULL;
    }
    hCollectionStore = ssl_collect_certificates(store_name);
    if (hCollectionStore == NULL) {
        Py_DECREF(result);
        return PyErr_SetFromWindowsErr(GetLastError());
    }

    while (pCertCtx = CertEnumCertificatesInStore(hCollectionStore, pCertCtx)) {
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
        if (PySet_Add(result, tup) == -1) {
            Py_CLEAR(result);
            Py_CLEAR(tup);
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

    /* CERT_CLOSE_STORE_FORCE_FLAG forces freeing of memory for all contexts
       associated with the store, in this case our collection store and the
       associated system stores. */
    if (!CertCloseStore(hCollectionStore, CERT_CLOSE_STORE_FORCE_FLAG)) {
        /* This error case might shadow another exception.*/
        Py_XDECREF(result);
        return PyErr_SetFromWindowsErr(GetLastError());
    }

    /* convert set to list */
    if (result == NULL) {
        return NULL;
    } else {
        PyObject *lst = PySequence_List(result);
        Py_DECREF(result);
        return lst;
    }
}

/*[clinic input]
_ssl.enum_crls
    store_name: str

Retrieve CRLs from Windows' cert store.

store_name may be one of 'CA', 'ROOT' or 'MY'.  The system may provide
more cert storages, too.  The function returns a list of (bytes,
encoding_type) tuples.  The encoding_type flag can be interpreted with
X509_ASN_ENCODING or PKCS_7_ASN_ENCODING.
[clinic start generated code]*/

static PyObject *
_ssl_enum_crls_impl(PyObject *module, const char *store_name)
/*[clinic end generated code: output=bce467f60ccd03b6 input=a1f1d7629f1c5d3d]*/
{
    HCERTSTORE hCollectionStore = NULL;
    PCCRL_CONTEXT pCrlCtx = NULL;
    PyObject *crl = NULL, *enc = NULL, *tup = NULL;
    PyObject *result = NULL;

    result = PySet_New(NULL);
    if (result == NULL) {
        return NULL;
    }
    hCollectionStore = ssl_collect_certificates(store_name);
    if (hCollectionStore == NULL) {
        Py_DECREF(result);
        return PyErr_SetFromWindowsErr(GetLastError());
    }

    while (pCrlCtx = CertEnumCRLsInStore(hCollectionStore, pCrlCtx)) {
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

        if (PySet_Add(result, tup) == -1) {
            Py_CLEAR(result);
            Py_CLEAR(tup);
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

    /* CERT_CLOSE_STORE_FORCE_FLAG forces freeing of memory for all contexts
       associated with the store, in this case our collection store and the
       associated system stores. */
    if (!CertCloseStore(hCollectionStore, CERT_CLOSE_STORE_FORCE_FLAG)) {
        /* This error case might shadow another exception.*/
        Py_XDECREF(result);
        return PyErr_SetFromWindowsErr(GetLastError());
    }
    /* convert set to list */
    if (result == NULL) {
        return NULL;
    } else {
        PyObject *lst = PySequence_List(result);
        Py_DECREF(result);
        return lst;
    }
}

#endif /* _MSC_VER */

/* List of functions exported by this module. */
static PyMethodDef PySSL_methods[] = {
    _SSL__TEST_DECODE_CERT_METHODDEF
    _SSL_RAND_ADD_METHODDEF
    _SSL_RAND_BYTES_METHODDEF
    _SSL_RAND_PSEUDO_BYTES_METHODDEF
    _SSL_RAND_STATUS_METHODDEF
    _SSL_GET_DEFAULT_VERIFY_PATHS_METHODDEF
    _SSL_ENUM_CERTIFICATES_METHODDEF
    _SSL_ENUM_CRLS_METHODDEF
    _SSL_TXT2OBJ_METHODDEF
    _SSL_NID2OBJ_METHODDEF
    {NULL,                  NULL}            /* Sentinel */
};


PyDoc_STRVAR(module_doc,
"Implementation module for SSL socket operations.  See the socket module\n\
for documentation.");

static int
sslmodule_init_exceptions(PyObject *module)
{
    _sslmodulestate *state = get_ssl_state(module);
    PyObject *bases = NULL;

#define add_exception(exc, name, doc, base)                                 \
do {                                                                        \
    (exc) = PyErr_NewExceptionWithDoc("ssl." name, (doc), (base), NULL);    \
    if ((state) == NULL) goto error;                                        \
    if (PyModule_AddObjectRef(module, name, exc) < 0) goto error;           \
} while(0)

    state->PySSLErrorObject = PyType_FromSpecWithBases(
        &sslerror_type_spec, PyExc_OSError);
    if (state->PySSLErrorObject == NULL) {
        goto error;
    }
    if (PyModule_AddObjectRef(module, "SSLError", state->PySSLErrorObject) < 0) {
        goto error;
    }

    /* ssl.CertificateError used to be a subclass of ValueError */
    bases = PyTuple_Pack(2, state->PySSLErrorObject, PyExc_ValueError);
    if (bases == NULL) {
        goto error;
    }
    add_exception(
        state->PySSLCertVerificationErrorObject,
        "SSLCertVerificationError",
        SSLCertVerificationError_doc,
        bases
    );
    Py_CLEAR(bases);

    add_exception(
        state->PySSLZeroReturnErrorObject,
        "SSLZeroReturnError",
        SSLZeroReturnError_doc,
        state->PySSLErrorObject
    );

    add_exception(
        state->PySSLWantWriteErrorObject,
        "SSLWantWriteError",
        SSLWantWriteError_doc,
        state->PySSLErrorObject
    );

    add_exception(
        state->PySSLWantReadErrorObject,
        "SSLWantReadError",
        SSLWantReadError_doc,
        state->PySSLErrorObject
    );

    add_exception(
        state->PySSLSyscallErrorObject,
        "SSLSyscallError",
        SSLSyscallError_doc,
        state->PySSLErrorObject
    );

    add_exception(
        state->PySSLEOFErrorObject,
        "SSLEOFError",
        SSLEOFError_doc,
        state->PySSLErrorObject
    );
#undef add_exception

    return 0;
  error:
    Py_XDECREF(bases);
    return -1;
}

static int
sslmodule_init_socketapi(PyObject *module)
{
    _sslmodulestate *state = get_ssl_state(module);
    PySocketModule_APIObject *sockmod = PySocketModule_ImportModuleAndAPI();

    if ((sockmod == NULL) || (sockmod->Sock_Type == NULL)) {
        return -1;
    }
    state->Sock_Type = sockmod->Sock_Type;
    Py_INCREF(state->Sock_Type);
    return 0;
}


static int
sslmodule_add_option(PyObject *m, const char *name, uint64_t value)
{
    Py_BUILD_ASSERT(sizeof(unsigned long long) >= sizeof(value));
    PyObject *obj = PyLong_FromUnsignedLongLong(value);
    if (obj == NULL) {
        return -1;
    }
    int res = PyModule_AddObjectRef(m, name, obj);
    Py_DECREF(obj);
    return res;
}


static int
sslmodule_init_constants(PyObject *m)
{
    PyModule_AddStringConstant(m, "_DEFAULT_CIPHERS",
                               PY_SSL_DEFAULT_CIPHER_STRING);

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
    PyModule_AddIntConstant(m, "VERIFY_ALLOW_PROXY_CERTS",
                            X509_V_FLAG_ALLOW_PROXY_CERTS);
    PyModule_AddIntConstant(m, "VERIFY_X509_TRUSTED_FIRST",
                            X509_V_FLAG_TRUSTED_FIRST);

#ifdef X509_V_FLAG_PARTIAL_CHAIN
    PyModule_AddIntConstant(m, "VERIFY_X509_PARTIAL_CHAIN",
                            X509_V_FLAG_PARTIAL_CHAIN);
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
                            PY_SSL_VERSION_TLS);
    PyModule_AddIntConstant(m, "PROTOCOL_TLS",
                            PY_SSL_VERSION_TLS);
    PyModule_AddIntConstant(m, "PROTOCOL_TLS_CLIENT",
                            PY_SSL_VERSION_TLS_CLIENT);
    PyModule_AddIntConstant(m, "PROTOCOL_TLS_SERVER",
                            PY_SSL_VERSION_TLS_SERVER);
    PyModule_AddIntConstant(m, "PROTOCOL_TLSv1",
                            PY_SSL_VERSION_TLS1);
    PyModule_AddIntConstant(m, "PROTOCOL_TLSv1_1",
                            PY_SSL_VERSION_TLS1_1);
    PyModule_AddIntConstant(m, "PROTOCOL_TLSv1_2",
                            PY_SSL_VERSION_TLS1_2);

#define ADD_OPTION(NAME, VALUE) if (sslmodule_add_option(m, NAME, (VALUE)) < 0) return -1

    /* protocol options */
    ADD_OPTION("OP_ALL", SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
    ADD_OPTION("OP_NO_SSLv2", SSL_OP_NO_SSLv2);
    ADD_OPTION("OP_NO_SSLv3", SSL_OP_NO_SSLv3);
    ADD_OPTION("OP_NO_TLSv1", SSL_OP_NO_TLSv1);
    ADD_OPTION("OP_NO_TLSv1_1", SSL_OP_NO_TLSv1_1);
    ADD_OPTION("OP_NO_TLSv1_2", SSL_OP_NO_TLSv1_2);
#ifdef SSL_OP_NO_TLSv1_3
    ADD_OPTION("OP_NO_TLSv1_3", SSL_OP_NO_TLSv1_3);
#else
    ADD_OPTION("OP_NO_TLSv1_3", 0);
#endif
    ADD_OPTION("OP_CIPHER_SERVER_PREFERENCE",
                            SSL_OP_CIPHER_SERVER_PREFERENCE);
    ADD_OPTION("OP_SINGLE_DH_USE", SSL_OP_SINGLE_DH_USE);
    ADD_OPTION("OP_NO_TICKET", SSL_OP_NO_TICKET);
#ifdef SSL_OP_SINGLE_ECDH_USE
    ADD_OPTION("OP_SINGLE_ECDH_USE", SSL_OP_SINGLE_ECDH_USE);
#endif
#ifdef SSL_OP_NO_COMPRESSION
    ADD_OPTION("OP_NO_COMPRESSION",
                            SSL_OP_NO_COMPRESSION);
#endif
#ifdef SSL_OP_ENABLE_MIDDLEBOX_COMPAT
    ADD_OPTION("OP_ENABLE_MIDDLEBOX_COMPAT",
                            SSL_OP_ENABLE_MIDDLEBOX_COMPAT);
#endif
#ifdef SSL_OP_NO_RENEGOTIATION
    ADD_OPTION("OP_NO_RENEGOTIATION",
                            SSL_OP_NO_RENEGOTIATION);
#endif
#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
    ADD_OPTION("OP_IGNORE_UNEXPECTED_EOF",
                            SSL_OP_IGNORE_UNEXPECTED_EOF);
#endif

#ifdef X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT
    PyModule_AddIntConstant(m, "HOSTFLAG_ALWAYS_CHECK_SUBJECT",
                            X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT);
#endif
#ifdef X509_CHECK_FLAG_NEVER_CHECK_SUBJECT
    PyModule_AddIntConstant(m, "HOSTFLAG_NEVER_CHECK_SUBJECT",
                            X509_CHECK_FLAG_NEVER_CHECK_SUBJECT);
#endif
#ifdef X509_CHECK_FLAG_NO_WILDCARDS
    PyModule_AddIntConstant(m, "HOSTFLAG_NO_WILDCARDS",
                            X509_CHECK_FLAG_NO_WILDCARDS);
#endif
#ifdef X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS
    PyModule_AddIntConstant(m, "HOSTFLAG_NO_PARTIAL_WILDCARDS",
                            X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
#endif
#ifdef X509_CHECK_FLAG_MULTI_LABEL_WILDCARDS
    PyModule_AddIntConstant(m, "HOSTFLAG_MULTI_LABEL_WILDCARDS",
                            X509_CHECK_FLAG_MULTI_LABEL_WILDCARDS);
#endif
#ifdef X509_CHECK_FLAG_SINGLE_LABEL_SUBDOMAINS
    PyModule_AddIntConstant(m, "HOSTFLAG_SINGLE_LABEL_SUBDOMAINS",
                            X509_CHECK_FLAG_SINGLE_LABEL_SUBDOMAINS);
#endif

    /* file types */
    PyModule_AddIntConstant(m, "ENCODING_PEM", PY_SSL_ENCODING_PEM);
    PyModule_AddIntConstant(m, "ENCODING_DER", PY_SSL_ENCODING_DER);

    /* protocol versions */
    PyModule_AddIntConstant(m, "PROTO_MINIMUM_SUPPORTED",
                            PY_PROTO_MINIMUM_SUPPORTED);
    PyModule_AddIntConstant(m, "PROTO_MAXIMUM_SUPPORTED",
                            PY_PROTO_MAXIMUM_SUPPORTED);
    PyModule_AddIntConstant(m, "PROTO_SSLv3", PY_PROTO_SSLv3);
    PyModule_AddIntConstant(m, "PROTO_TLSv1", PY_PROTO_TLSv1);
    PyModule_AddIntConstant(m, "PROTO_TLSv1_1", PY_PROTO_TLSv1_1);
    PyModule_AddIntConstant(m, "PROTO_TLSv1_2", PY_PROTO_TLSv1_2);
    PyModule_AddIntConstant(m, "PROTO_TLSv1_3", PY_PROTO_TLSv1_3);

#define addbool(m, key, value) \
    do { \
        PyObject *bool_obj = (value) ? Py_True : Py_False; \
        Py_INCREF(bool_obj); \
        PyModule_AddObject((m), (key), bool_obj); \
    } while (0)

    addbool(m, "HAS_SNI", 1);
    addbool(m, "HAS_TLS_UNIQUE", 1);
    addbool(m, "HAS_ECDH", 1);
    addbool(m, "HAS_NPN", 0);
    addbool(m, "HAS_ALPN", 1);

#if defined(SSL2_VERSION) && !defined(OPENSSL_NO_SSL2)
    addbool(m, "HAS_SSLv2", 1);
#else
    addbool(m, "HAS_SSLv2", 0);
#endif

#if defined(SSL3_VERSION) && !defined(OPENSSL_NO_SSL3)
    addbool(m, "HAS_SSLv3", 1);
#else
    addbool(m, "HAS_SSLv3", 0);
#endif

#if defined(TLS1_VERSION) && !defined(OPENSSL_NO_TLS1)
    addbool(m, "HAS_TLSv1", 1);
#else
    addbool(m, "HAS_TLSv1", 0);
#endif

#if defined(TLS1_1_VERSION) && !defined(OPENSSL_NO_TLS1_1)
    addbool(m, "HAS_TLSv1_1", 1);
#else
    addbool(m, "HAS_TLSv1_1", 0);
#endif

#if defined(TLS1_2_VERSION) && !defined(OPENSSL_NO_TLS1_2)
    addbool(m, "HAS_TLSv1_2", 1);
#else
    addbool(m, "HAS_TLSv1_2", 0);
#endif

#if defined(TLS1_3_VERSION) && !defined(OPENSSL_NO_TLS1_3)
    addbool(m, "HAS_TLSv1_3", 1);
#else
    addbool(m, "HAS_TLSv1_3", 0);
#endif

    return 0;
}

static int
sslmodule_init_errorcodes(PyObject *module)
{
    _sslmodulestate *state = get_ssl_state(module);

    struct py_ssl_error_code *errcode;
    struct py_ssl_library_code *libcode;

    /* Mappings for error codes */
    state->err_codes_to_names = PyDict_New();
    if (state->err_codes_to_names == NULL)
        return -1;
    state->err_names_to_codes = PyDict_New();
    if (state->err_names_to_codes == NULL)
        return -1;
    state->lib_codes_to_names = PyDict_New();
    if (state->lib_codes_to_names == NULL)
        return -1;

    errcode = error_codes;
    while (errcode->mnemonic != NULL) {
        PyObject *mnemo = PyUnicode_FromString(errcode->mnemonic);
        if (mnemo == NULL) {
            return -1;
        }
        PyObject *key = Py_BuildValue("ii", errcode->library, errcode->reason);
        if (key == NULL) {
            Py_DECREF(mnemo);
            return -1;
        }
        if (PyDict_SetItem(state->err_codes_to_names, key, mnemo) < 0) {
            Py_DECREF(key);
            Py_DECREF(mnemo);
            return -1;
        }
        int rc = PyDict_SetItem(state->err_names_to_codes, mnemo, key);
        Py_DECREF(key);
        Py_DECREF(mnemo);
        if (rc < 0) {
            return -1;
        }
        errcode++;
    }

    libcode = library_codes;
    while (libcode->library != NULL) {
        PyObject *mnemo, *key;
        key = PyLong_FromLong(libcode->code);
        mnemo = PyUnicode_FromString(libcode->library);
        if (key == NULL || mnemo == NULL)
            return -1;
        if (PyDict_SetItem(state->lib_codes_to_names, key, mnemo))
            return -1;
        Py_DECREF(key);
        Py_DECREF(mnemo);
        libcode++;
    }

    if (PyModule_AddObjectRef(module, "err_codes_to_names", state->err_codes_to_names))
        return -1;
    if (PyModule_AddObjectRef(module, "err_names_to_codes", state->err_names_to_codes))
        return -1;
    if (PyModule_AddObjectRef(module, "lib_codes_to_names", state->lib_codes_to_names))
        return -1;

    return 0;
}

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

static int
sslmodule_init_versioninfo(PyObject *m)
{
    PyObject *r;
    unsigned long libver;
    unsigned int major, minor, fix, patch, status;

    /* OpenSSL version */
    /* SSLeay() gives us the version of the library linked against,
       which could be different from the headers version.
    */
    libver = OpenSSL_version_num();
    r = PyLong_FromUnsignedLong(libver);
    if (_PyModule_Add(m, "OPENSSL_VERSION_NUMBER", r) < 0)
        return -1;

    parse_openssl_version(libver, &major, &minor, &fix, &patch, &status);
    r = Py_BuildValue("IIIII", major, minor, fix, patch, status);
    if (_PyModule_Add(m, "OPENSSL_VERSION_INFO", r) < 0)
        return -1;

    r = PyUnicode_FromString(OpenSSL_version(OPENSSL_VERSION));
    if (_PyModule_Add(m, "OPENSSL_VERSION", r) < 0)
        return -1;

    libver = OPENSSL_VERSION_NUMBER;
    parse_openssl_version(libver, &major, &minor, &fix, &patch, &status);
    r = Py_BuildValue("IIIII", major, minor, fix, patch, status);
    if (_PyModule_Add(m, "_OPENSSL_API_VERSION", r) < 0)
        return -1;

    return 0;
}

static int
sslmodule_init_types(PyObject *module)
{
    _sslmodulestate *state = get_ssl_state(module);

    state->PySSLContext_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PySSLContext_spec, NULL
    );
    if (state->PySSLContext_Type == NULL)
        return -1;

    state->PySSLSocket_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PySSLSocket_spec, NULL
    );
    if (state->PySSLSocket_Type == NULL)
        return -1;

    state->PySSLMemoryBIO_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PySSLMemoryBIO_spec, NULL
    );
    if (state->PySSLMemoryBIO_Type == NULL)
        return -1;

    state->PySSLSession_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PySSLSession_spec, NULL
    );
    if (state->PySSLSession_Type == NULL)
        return -1;

    state->PySSLCertificate_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PySSLCertificate_spec, NULL
    );
    if (state->PySSLCertificate_Type == NULL)
        return -1;

    if (PyModule_AddType(module, state->PySSLContext_Type))
        return -1;
    if (PyModule_AddType(module, state->PySSLSocket_Type))
        return -1;
    if (PyModule_AddType(module, state->PySSLMemoryBIO_Type))
        return -1;
    if (PyModule_AddType(module, state->PySSLSession_Type))
        return -1;
    if (PyModule_AddType(module, state->PySSLCertificate_Type))
        return -1;
    return 0;
}

static int
sslmodule_init_strings(PyObject *module)
{
    _sslmodulestate *state = get_ssl_state(module);
    state->str_library = PyUnicode_InternFromString("library");
    if (state->str_library == NULL) {
        return -1;
    }
    state->str_reason = PyUnicode_InternFromString("reason");
    if (state->str_reason == NULL) {
        return -1;
    }
    state->str_verify_message = PyUnicode_InternFromString("verify_message");
    if (state->str_verify_message == NULL) {
        return -1;
    }
    state->str_verify_code = PyUnicode_InternFromString("verify_code");
    if (state->str_verify_code == NULL) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot sslmodule_slots[] = {
    {Py_mod_exec, sslmodule_init_types},
    {Py_mod_exec, sslmodule_init_exceptions},
    {Py_mod_exec, sslmodule_init_socketapi},
    {Py_mod_exec, sslmodule_init_errorcodes},
    {Py_mod_exec, sslmodule_init_constants},
    {Py_mod_exec, sslmodule_init_versioninfo},
    {Py_mod_exec, sslmodule_init_strings},
    {0, NULL}
};

static int
sslmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    _sslmodulestate *state = get_ssl_state(m);

    Py_VISIT(state->PySSLContext_Type);
    Py_VISIT(state->PySSLSocket_Type);
    Py_VISIT(state->PySSLMemoryBIO_Type);
    Py_VISIT(state->PySSLSession_Type);
    Py_VISIT(state->PySSLCertificate_Type);
    Py_VISIT(state->PySSLErrorObject);
    Py_VISIT(state->PySSLCertVerificationErrorObject);
    Py_VISIT(state->PySSLZeroReturnErrorObject);
    Py_VISIT(state->PySSLWantReadErrorObject);
    Py_VISIT(state->PySSLWantWriteErrorObject);
    Py_VISIT(state->PySSLSyscallErrorObject);
    Py_VISIT(state->PySSLEOFErrorObject);
    Py_VISIT(state->err_codes_to_names);
    Py_VISIT(state->err_names_to_codes);
    Py_VISIT(state->lib_codes_to_names);
    Py_VISIT(state->Sock_Type);

    return 0;
}

static int
sslmodule_clear(PyObject *m)
{
    _sslmodulestate *state = get_ssl_state(m);

    Py_CLEAR(state->PySSLContext_Type);
    Py_CLEAR(state->PySSLSocket_Type);
    Py_CLEAR(state->PySSLMemoryBIO_Type);
    Py_CLEAR(state->PySSLSession_Type);
    Py_CLEAR(state->PySSLCertificate_Type);
    Py_CLEAR(state->PySSLErrorObject);
    Py_CLEAR(state->PySSLCertVerificationErrorObject);
    Py_CLEAR(state->PySSLZeroReturnErrorObject);
    Py_CLEAR(state->PySSLWantReadErrorObject);
    Py_CLEAR(state->PySSLWantWriteErrorObject);
    Py_CLEAR(state->PySSLSyscallErrorObject);
    Py_CLEAR(state->PySSLEOFErrorObject);
    Py_CLEAR(state->err_codes_to_names);
    Py_CLEAR(state->err_names_to_codes);
    Py_CLEAR(state->lib_codes_to_names);
    Py_CLEAR(state->Sock_Type);
    Py_CLEAR(state->str_library);
    Py_CLEAR(state->str_reason);
    Py_CLEAR(state->str_verify_code);
    Py_CLEAR(state->str_verify_message);
    return 0;
}

static void
sslmodule_free(void *m)
{
    sslmodule_clear((PyObject *)m);
}

static struct PyModuleDef _sslmodule_def = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_ssl",
    .m_doc = module_doc,
    .m_size = sizeof(_sslmodulestate),
    .m_methods = PySSL_methods,
    .m_slots = sslmodule_slots,
    .m_traverse = sslmodule_traverse,
    .m_clear = sslmodule_clear,
    .m_free = sslmodule_free
};

PyMODINIT_FUNC
PyInit__ssl(void)
{
    return PyModuleDef_Init(&_sslmodule_def);
}
