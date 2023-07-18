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
    PyObject *lib_codes_to_names;
    /* socket type from module CAPI */
    PyTypeObject *Sock_Type;
    /* Interned strings */
    PyObject *str_library;
    PyObject *str_reason;
    PyObject *str_verify_code;
    PyObject *str_verify_message;
    /* keylog lock */
    PyThread_type_lock keylog_lock;
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
    (get_ssl_state(PyType_GetModuleByDef(type, &_sslmodule_def)))
#define get_state_ctx(c) (((PySSLContext *)(c))->state)
#define get_state_sock(s) (((PySSLSocket *)(s))->ctx->state)
#define get_state_obj(o) ((_sslmodulestate *)PyType_GetModuleState(Py_TYPE(o)))
#define get_state_mbio(b) get_state_obj(b)
#define get_state_cert(c) get_state_obj(c)

/* ************************************************************************
 * certificate
 */

enum py_ssl_encoding {
    PY_SSL_ENCODING_PEM=X509_FILETYPE_PEM,
    PY_SSL_ENCODING_DER=X509_FILETYPE_ASN1,
    PY_SSL_ENCODING_PEM_AUX=X509_FILETYPE_PEM + 0x100,
};

typedef struct {
    PyObject_HEAD
    X509 *cert;
    Py_hash_t hash;
} PySSLCertificate;

/* ************************************************************************
 * helpers and utils
 */
static PyObject *_PySSL_BytesFromBIO(_sslmodulestate *state, BIO *bio);
static PyObject *_PySSL_UnicodeFromBIO(_sslmodulestate *state, BIO *bio, const char *error);

#endif /* Py_SSL_H */
