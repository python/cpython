#ifndef Py_SSL_H
#define Py_SSL_H

/* OpenSSL header files */
#include "openssl/evp.h"
#include "openssl/x509.h"

/*
 * ssl module state
 */
typedef struct {
    /* Types */
    PyTypeObject *PySSLContext_Type;
    PyTypeObject *PySSLSocket_Type;
    PyTypeObject *PySSLMemoryBIO_Type;
    PyTypeObject *PySSLSession_Type;
    PyTypeObject *PySSLPrivateKey_Type;
    PyTypeObject *PySSLCertificate_Type;
    /* SSL error object */
    PyObject *PySSLErrorObject;
    PyObject *PySSLCertVerificationErrorObject;
    PyObject *PySSLZeroReturnErrorObject;
    PyObject *PySSLWantReadErrorObject;
    PyObject *PySSLWantWriteErrorObject;
    PyObject *PySSLSyscallErrorObject;
    PyObject *PySSLEOFErrorObject;
    /* Error mappings */
    PyObject *err_codes_to_names;
    PyObject *err_names_to_codes;
    PyObject *lib_codes_to_names;
    /* socket type from module CAPI */
    PyTypeObject *Sock_Type;
} _sslmodulestate;

static struct PyModuleDef _sslmodule_def;

Py_LOCAL_INLINE(_sslmodulestate*)
get_ssl_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_sslmodulestate *)state;
}

#define get_state_type(type) \
    (get_ssl_state(_PyType_GetModuleByDef(type, &_sslmodule_def)))
#define get_state_ctx(c) (((PySSLContext *)(c))->state)
#define get_state_sock(s) (((PySSLSocket *)(s))->ctx->state)
#define get_state_obj(o) ((_sslmodulestate *)PyType_GetModuleState(Py_TYPE(o)))
#define get_state_mbio(b) get_state_obj(b)
#define get_state_cert(c) get_state_obj(c)
#define get_state_pkey(p) get_state_obj(p)

enum py_ssl_filetype {
    PY_SSL_FILETYPE_PEM=X509_FILETYPE_PEM,
    PY_SSL_FILETYPE_ASN1=X509_FILETYPE_ASN1,
    PY_SSL_FILETYPE_PEM_AUX=X509_FILETYPE_PEM + 0x100,
};

typedef struct {
    PyObject_HEAD
    EVP_PKEY *pkey;
} PySSLPrivateKey;

typedef struct {
    PyObject_HEAD
    X509 *cert;
    Py_hash_t hash;
} PySSLCertificate;

typedef struct {
    PyObject_HEAD
    X509_STORE *store;
    /* OpenSSL 1.1.1 has no X509_STORE_dup() and X509_LOOKUP_dup.
     * Keep a list of hash directories so we can copy them over. */
    PyObject *hash_dirs;
} PySSLTrustStore;

/* ************************************************************************
 * helpers and utils
 */
<<<<<<< HEAD
static BIO *_PySSL_filebio(_sslmodulestate *state, PyObject *path);
static BIO *_PySSL_bufferbio(_sslmodulestate *state, Py_buffer *b);
static PyObject *_PySSL_BytesFromBIO(_sslmodulestate *state, BIO *bio);
#if 0
static PyObject *_PySSL_UnicodeFromBIO(_sslmodulestate *state, BIO *bio, const char *error);
#endif
||||||| parent of 27501a5ce86 (Work on trust store)
static BIO *_PySSL_filebio(PyObject *path);
static BIO *_PySSL_bufferbio(Py_buffer *b);
static PyObject *_PySSL_BytesFromBIO(BIO *bio);
#if 0
static PyObject *_PySSL_UnicodeFromBIO(BIO *bio, const char *error);
#endif
=======
static BIO *_PySSL_filebio(PyObject *path);
static BIO *_PySSL_bufferbio(Py_buffer *b);
static PyObject *_PySSL_BytesFromBIO(BIO *bio);
static PyObject *_PySSL_UnicodeFromBIO(BIO *bio, const char *error);
>>>>>>> 27501a5ce86 (Work on trust store)

/* ************************************************************************
 * password callback
 */

typedef struct {
    PyThreadState *thread_state;
    PyObject *callable;
    char *password;
    int size;
    int error;
} PySSLPasswordInfo;

#define PYSSL_PWINFO_INIT(pw_info, password, err)             \
    if ((password) && (password) != Py_None) {                \
        if (PyCallable_Check(password)) {                     \
            (pw_info)->callable = (password);                 \
        } else if (!PySSL_pwinfo_set((pw_info), (password),   \
                                "password should be a string or callable")) { \
            return (err);                                     \
        }                                                     \
    }

#define PYSSL_PWINFO_ERROR(state, pw_info)                     \
    if ((pw_info)->error) {                                    \
        /* the password callback has already set the error information */ \
        ERR_clear_error();                                     \
    }                                                          \
    else if (errno != 0) {                                     \
        ERR_clear_error();                                     \
        PyErr_SetFromErrno(PyExc_OSError);                     \
    }                                                          \
    else {                                                     \
        _setSSLError(state, NULL, 0, __FILE__, __LINE__);      \
    }

static int
PySSL_pwinfo_set(PySSLPasswordInfo *pw_info, PyObject* password,
                 const char *bad_type_error);
static int
PySSL_password_cb(char *buf, int size, int rwflag, void *userdata);

#endif /* Py_SSL_H */
