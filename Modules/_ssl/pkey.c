#include "Python.h"
#include "../_ssl.h"

#include "openssl/bio.h"
#include "openssl/evp.h"
#include "openssl/pem.h"

/*[clinic input]
module _ssl
class _ssl.PrivateKey "PySSLPrivateKey *" "PySSLPrivateKey_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=46196dd92bd0a6d8]*/

#include "clinic/pkey.c.h"

static PyObject *
newPrivateKey(PyTypeObject *type, EVP_PKEY *pkey, int upref)
{
    PySSLPrivateKey *self;

    assert(type != NULL && type->tp_alloc != NULL);
    assert(pkey != NULL);

    self = (PySSLPrivateKey *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    if (upref == 1) {
       EVP_PKEY_up_ref(pkey);
    }
    self->pkey = pkey;

    return (PyObject *) self;
}

static EVP_PKEY *
read_pkey_bio(BIO *bio, int format, PySSLPasswordInfo *pw_info)
{
    EVP_PKEY *pkey = NULL;

    switch(format) {
    case PY_SSL_FILETYPE_PEM:
        PySSL_BEGIN_ALLOW_THREADS_S(pw_info->thread_state);
        pkey = PEM_read_bio_PrivateKey(bio, NULL, PySSL_password_cb, pw_info);
        PySSL_END_ALLOW_THREADS_S(pw_info->thread_state);
        break;
    case PY_SSL_FILETYPE_ASN1:
        PySSL_BEGIN_ALLOW_THREADS_S(pw_info->thread_state);
        pkey = d2i_PKCS8PrivateKey_bio(bio, NULL, PySSL_password_cb, pw_info);
        PySSL_END_ALLOW_THREADS_S(pw_info->thread_state);
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Invalid format");
        return NULL;
    }

    if (pkey == NULL) {
        PYSSL_PWINFO_ERROR(pw_info)
        return NULL;
    }
    return pkey;
}

/*[clinic input]
@classmethod
_ssl.PrivateKey.from_file
    path: object(converter="PyUnicode_FSConverter")
    *
    password: object = None
    format: int(c_default="PY_SSL_FILETYPE_PEM") = FILETYPE_PEM

[clinic start generated code]*/

static PyObject *
_ssl_PrivateKey_from_file_impl(PyTypeObject *type, PyObject *path,
                               PyObject *password, int format)
/*[clinic end generated code: output=5dc7bfeda73c8b4b input=1f0112f77dded55b]*/
{
    EVP_PKEY *pkey = NULL;
    BIO *bio;
    PySSLPasswordInfo pw_info = { NULL, NULL, NULL, 0, 0 };

    PYSSL_PWINFO_INIT(&pw_info, password, NULL)

    bio = _PySSL_filebio(path);
    if (bio == NULL) {
        return NULL;
    }
    pkey = read_pkey_bio(bio, format, &pw_info);
    BIO_free(bio);
    if (pkey == NULL) {
        return NULL;
    }
    return newPrivateKey(type, pkey, 0);
}

/*[clinic input]
@classmethod
_ssl.PrivateKey.from_buffer
    buffer: Py_buffer
    *
    password: object = None
    format: int(c_default="PY_SSL_FILETYPE_PEM") = FILETYPE_PEM
[clinic start generated code]*/

static PyObject *
_ssl_PrivateKey_from_buffer_impl(PyTypeObject *type, Py_buffer *buffer,
                                 PyObject *password, int format)
/*[clinic end generated code: output=e6acef288f8eff17 input=a76a6549e5381124]*/
{
    EVP_PKEY *pkey = NULL;
    BIO *bio;
    PySSLPasswordInfo pw_info = { NULL, NULL, NULL, 0, 0 };

    PYSSL_PWINFO_INIT(&pw_info, password, NULL)

    bio = _PySSL_bufferbio(buffer);
    if (bio == NULL) {
        return NULL;
    }
    pkey = read_pkey_bio(bio, format, &pw_info);
    BIO_free(bio);
    if (pkey == NULL) {
        return NULL;
    }
    return newPrivateKey(type, pkey, 0);
}

static void
pkey_dealloc(PySSLPrivateKey *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    EVP_PKEY_free(self->pkey);
    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static PyGetSetDef pkey_getsetlist[] = {
    {NULL}             /* sentinel */
};

static PyMethodDef pkey_methods[] = {
    _SSL_PRIVATEKEY_FROM_FILE_METHODDEF
    _SSL_PRIVATEKEY_FROM_BUFFER_METHODDEF
    {NULL, NULL}
};

static PyType_Slot PySSLPrivateKey_slots[] = {
    {Py_tp_dealloc, pkey_dealloc},
    {Py_tp_methods, pkey_methods},
    {Py_tp_getset, pkey_getsetlist},
    {0, 0},
};

static PyType_Spec PySSLPrivateKey_spec = {
    "_ssl.PrivateKey",
    sizeof(PySSLPrivateKey),
    0,
    Py_TPFLAGS_DEFAULT,
    PySSLPrivateKey_slots,
};
