/* Module that wraps all OpenSSL hash algorithms */

/*
 * Copyright (C) 2005-2010   Gregory P. Smith (greg@krypto.org)
 * Licensed to PSF under a Contributor Agreement.
 *
 * Derived from a skeleton of shamodule.c containing work performed by:
 *
 * Andrew Kuchling (amk@amk.ca)
 * Greg Stein (gstein@lyra.org)
 *
 */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "hashlib.h"
#include "pystrhex.h"


/* EVP is the preferred interface to hashing in OpenSSL */
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/crypto.h>
/* We use the object interface to discover what hashes OpenSSL supports. */
#include <openssl/objects.h>
#include "openssl/err.h"

#include <openssl/crypto.h>       // FIPS_mode()

#ifndef OPENSSL_THREADS
#  error "OPENSSL_THREADS is not defined, Python requires thread-safe OpenSSL"
#endif

#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(LIBRESSL_VERSION_NUMBER)
/* OpenSSL < 1.1.0 */
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy

HMAC_CTX *
HMAC_CTX_new(void)
{
    HMAC_CTX *ctx = OPENSSL_malloc(sizeof(HMAC_CTX));
    if (ctx != NULL) {
        memset(ctx, 0, sizeof(HMAC_CTX));
        HMAC_CTX_init(ctx);
    }
    return ctx;
}

void
HMAC_CTX_free(HMAC_CTX *ctx)
{
    if (ctx != NULL) {
        HMAC_CTX_cleanup(ctx);
        OPENSSL_free(ctx);
    }
}

const EVP_MD *
HMAC_CTX_get_md(const HMAC_CTX *ctx)
{
    return ctx->md;
}
#endif

#define MUNCH_SIZE INT_MAX

#ifdef NID_sha3_224
#define PY_OPENSSL_HAS_SHA3 1
#endif

#if defined(EVP_MD_FLAG_XOF) && defined(NID_shake128)
#define PY_OPENSSL_HAS_SHAKE 1
#endif

#if defined(NID_blake2b512) && !defined(OPENSSL_NO_BLAKE2)
#define PY_OPENSSL_HAS_BLAKE2 1
#endif

static PyModuleDef _hashlibmodule;

typedef struct {
    PyTypeObject *EVPtype;
    PyTypeObject *HMACtype;
#ifdef PY_OPENSSL_HAS_SHAKE
    PyTypeObject *EVPXOFtype;
#endif
} _hashlibstate;

static inline _hashlibstate*
get_hashlib_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_hashlibstate *)state;
}

typedef struct {
    PyObject_HEAD
    EVP_MD_CTX          *ctx;   /* OpenSSL message digest context */
    PyThread_type_lock   lock;  /* OpenSSL context lock */
} EVPobject;

typedef struct {
    PyObject_HEAD
    HMAC_CTX *ctx;            /* OpenSSL hmac context */
    PyThread_type_lock lock;  /* HMAC context lock */
} HMACobject;

#include "clinic/_hashopenssl.c.h"
/*[clinic input]
module _hashlib
class _hashlib.HASH "EVPobject *" "((_hashlibstate *)PyModule_GetState(module))->EVPtype"
class _hashlib.HASHXOF "EVPobject *" "((_hashlibstate *)PyModule_GetState(module))->EVPXOFtype"
class _hashlib.HMAC "HMACobject *" "((_hashlibstate *)PyModule_GetState(module))->HMACtype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7df1bcf6f75cb8ef]*/


/* LCOV_EXCL_START */
static PyObject *
_setException(PyObject *exc)
{
    unsigned long errcode;
    const char *lib, *func, *reason;

    errcode = ERR_peek_last_error();
    if (!errcode) {
        PyErr_SetString(exc, "unknown reasons");
        return NULL;
    }
    ERR_clear_error();

    lib = ERR_lib_error_string(errcode);
    func = ERR_func_error_string(errcode);
    reason = ERR_reason_error_string(errcode);

    if (lib && func) {
        PyErr_Format(exc, "[%s: %s] %s", lib, func, reason);
    }
    else if (lib) {
        PyErr_Format(exc, "[%s] %s", lib, reason);
    }
    else {
        PyErr_SetString(exc, reason);
    }
    return NULL;
}
/* LCOV_EXCL_STOP */

/* {Py_tp_new, NULL} doesn't block __new__ */
static PyObject *
_disabled_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyErr_Format(PyExc_TypeError,
        "cannot create '%.100s' instances", _PyType_Name(type));
    return NULL;
}

static PyObject*
py_digest_name(const EVP_MD *md)
{
    int nid = EVP_MD_nid(md);
    const char *name = NULL;

    /* Hard-coded names for well-known hashing algorithms.
     * OpenSSL uses slightly different names algorithms like SHA3.
     */
    switch (nid) {
    case NID_md5:
        name = "md5";
        break;
    case NID_sha1:
        name = "sha1";
        break;
    case NID_sha224:
        name ="sha224";
        break;
    case NID_sha256:
        name ="sha256";
        break;
    case NID_sha384:
        name ="sha384";
        break;
    case NID_sha512:
        name ="sha512";
        break;
#ifdef NID_sha512_224
    case NID_sha512_224:
        name ="sha512_224";
        break;
    case NID_sha512_256:
        name ="sha512_256";
        break;
#endif
#ifdef PY_OPENSSL_HAS_SHA3
    case NID_sha3_224:
        name ="sha3_224";
        break;
    case NID_sha3_256:
        name ="sha3_256";
        break;
    case NID_sha3_384:
        name ="sha3_384";
        break;
    case NID_sha3_512:
        name ="sha3_512";
        break;
#endif
#ifdef PY_OPENSSL_HAS_SHAKE
    case NID_shake128:
        name ="shake_128";
        break;
    case NID_shake256:
        name ="shake_256";
        break;
#endif
#ifdef PY_OPENSSL_HAS_BLAKE2
    case NID_blake2s256:
        name ="blake2s";
        break;
    case NID_blake2b512:
        name ="blake2b";
        break;
#endif
    default:
        /* Ignore aliased names and only use long, lowercase name. The aliases
         * pollute the list and OpenSSL appears to have its own definition of
         * alias as the resulting list still contains duplicate and alternate
         * names for several algorithms.
         */
        name = OBJ_nid2ln(nid);
        if (name == NULL)
            name = OBJ_nid2sn(nid);
        break;
    }

    return PyUnicode_FromString(name);
}

static const EVP_MD*
py_digest_by_name(const char *name)
{
    const EVP_MD *digest = EVP_get_digestbyname(name);

    /* OpenSSL uses dash instead of underscore in names of some algorithms
     * like SHA3 and SHAKE. Detect different spellings. */
    if (digest == NULL) {
        if (0) {}
#ifdef NID_sha512_224
        else if (!strcmp(name, "sha512_224") || !strcmp(name, "SHA512_224")) {
            digest = EVP_sha512_224();
        }
        else if (!strcmp(name, "sha512_256") || !strcmp(name, "SHA512_256")) {
            digest = EVP_sha512_256();
        }
#endif
#ifdef PY_OPENSSL_HAS_SHA3
        /* could be sha3_ or shake_, Python never defined upper case */
        else if (!strcmp(name, "sha3_224")) {
            digest = EVP_sha3_224();
        }
        else if (!strcmp(name, "sha3_256")) {
            digest = EVP_sha3_256();
        }
        else if (!strcmp(name, "sha3_384")) {
            digest = EVP_sha3_384();
        }
        else if (!strcmp(name, "sha3_512")) {
            digest = EVP_sha3_512();
        }
#endif
#ifdef PY_OPENSSL_HAS_SHAKE
        else if (!strcmp(name, "shake_128")) {
            digest = EVP_shake128();
        }
        else if (!strcmp(name, "shake_256")) {
            digest = EVP_shake256();
        }
#endif
#ifdef PY_OPENSSL_HAS_BLAKE2
        else if (!strcmp(name, "blake2s256")) {
            digest = EVP_blake2s256();
        }
        else if (!strcmp(name, "blake2b512")) {
            digest = EVP_blake2b512();
        }
#endif
    }

    return digest;
}

static EVPobject *
newEVPobject(PyTypeObject *type)
{
    EVPobject *retval = (EVPobject *)PyObject_New(EVPobject, type);
    if (retval == NULL) {
        return NULL;
    }

    retval->lock = NULL;

    retval->ctx = EVP_MD_CTX_new();
    if (retval->ctx == NULL) {
        Py_DECREF(retval);
        PyErr_NoMemory();
        return NULL;
    }

    return retval;
}

static int
EVP_hash(EVPobject *self, const void *vp, Py_ssize_t len)
{
    unsigned int process;
    const unsigned char *cp = (const unsigned char *)vp;
    while (0 < len) {
        if (len > (Py_ssize_t)MUNCH_SIZE)
            process = MUNCH_SIZE;
        else
            process = Py_SAFE_DOWNCAST(len, Py_ssize_t, unsigned int);
        if (!EVP_DigestUpdate(self->ctx, (const void*)cp, process)) {
            _setException(PyExc_ValueError);
            return -1;
        }
        len -= process;
        cp += process;
    }
    return 0;
}

/* Internal methods for a hash object */

static void
EVP_dealloc(EVPobject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
    EVP_MD_CTX_free(self->ctx);
    PyObject_Del(self);
    Py_DECREF(tp);
}

static int
locked_EVP_MD_CTX_copy(EVP_MD_CTX *new_ctx_p, EVPobject *self)
{
    int result;
    ENTER_HASHLIB(self);
    result = EVP_MD_CTX_copy(new_ctx_p, self->ctx);
    LEAVE_HASHLIB(self);
    return result;
}

/* External methods for a hash object */

/*[clinic input]
_hashlib.HASH.copy as EVP_copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
EVP_copy_impl(EVPobject *self)
/*[clinic end generated code: output=b370c21cdb8ca0b4 input=31455b6a3e638069]*/
{
    EVPobject *newobj;

    if ((newobj = newEVPobject(Py_TYPE(self))) == NULL)
        return NULL;

    if (!locked_EVP_MD_CTX_copy(newobj->ctx, self)) {
        Py_DECREF(newobj);
        return _setException(PyExc_ValueError);
    }
    return (PyObject *)newobj;
}

/*[clinic input]
_hashlib.HASH.digest as EVP_digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
EVP_digest_impl(EVPobject *self)
/*[clinic end generated code: output=0f6a3a0da46dc12d input=03561809a419bf00]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    EVP_MD_CTX *temp_ctx;
    PyObject *retval;
    unsigned int digest_size;

    temp_ctx = EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    if (!locked_EVP_MD_CTX_copy(temp_ctx, self)) {
        return _setException(PyExc_ValueError);
    }
    digest_size = EVP_MD_CTX_size(temp_ctx);
    if (!EVP_DigestFinal(temp_ctx, digest, NULL)) {
        _setException(PyExc_ValueError);
        return NULL;
    }

    retval = PyBytes_FromStringAndSize((const char *)digest, digest_size);
    EVP_MD_CTX_free(temp_ctx);
    return retval;
}

/*[clinic input]
_hashlib.HASH.hexdigest as EVP_hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
EVP_hexdigest_impl(EVPobject *self)
/*[clinic end generated code: output=18e6decbaf197296 input=aff9cf0e4c741a9a]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    EVP_MD_CTX *temp_ctx;
    unsigned int digest_size;

    temp_ctx = EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    /* Get the raw (binary) digest value */
    if (!locked_EVP_MD_CTX_copy(temp_ctx, self)) {
        return _setException(PyExc_ValueError);
    }
    digest_size = EVP_MD_CTX_size(temp_ctx);
    if (!EVP_DigestFinal(temp_ctx, digest, NULL)) {
        _setException(PyExc_ValueError);
        return NULL;
    }

    EVP_MD_CTX_free(temp_ctx);

    return _Py_strhex((const char *)digest, (Py_ssize_t)digest_size);
}

/*[clinic input]
_hashlib.HASH.update as EVP_update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
EVP_update(EVPobject *self, PyObject *obj)
/*[clinic end generated code: output=ec1d55ed2432e966 input=9b30ec848f015501]*/
{
    int result;
    Py_buffer view;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &view);

    if (self->lock == NULL && view.len >= HASHLIB_GIL_MINSIZE) {
        self->lock = PyThread_allocate_lock();
        /* fail? lock = NULL and we fail over to non-threaded code. */
    }

    if (self->lock != NULL) {
        Py_BEGIN_ALLOW_THREADS
        PyThread_acquire_lock(self->lock, 1);
        result = EVP_hash(self, view.buf, view.len);
        PyThread_release_lock(self->lock);
        Py_END_ALLOW_THREADS
    } else {
        result = EVP_hash(self, view.buf, view.len);
    }

    PyBuffer_Release(&view);

    if (result == -1)
        return NULL;
    Py_RETURN_NONE;
}

static PyMethodDef EVP_methods[] = {
    EVP_UPDATE_METHODDEF
    EVP_DIGEST_METHODDEF
    EVP_HEXDIGEST_METHODDEF
    EVP_COPY_METHODDEF
    {NULL, NULL}  /* sentinel */
};

static PyObject *
EVP_get_block_size(EVPobject *self, void *closure)
{
    long block_size;
    block_size = EVP_MD_CTX_block_size(self->ctx);
    return PyLong_FromLong(block_size);
}

static PyObject *
EVP_get_digest_size(EVPobject *self, void *closure)
{
    long size;
    size = EVP_MD_CTX_size(self->ctx);
    return PyLong_FromLong(size);
}

static PyObject *
EVP_get_name(EVPobject *self, void *closure)
{
    return py_digest_name(EVP_MD_CTX_md(self->ctx));
}

static PyGetSetDef EVP_getseters[] = {
    {"digest_size",
     (getter)EVP_get_digest_size, NULL,
     NULL,
     NULL},
    {"block_size",
     (getter)EVP_get_block_size, NULL,
     NULL,
     NULL},
    {"name",
     (getter)EVP_get_name, NULL,
     NULL,
     PyDoc_STR("algorithm name.")},
    {NULL}  /* Sentinel */
};


static PyObject *
EVP_repr(EVPobject *self)
{
    PyObject *name_obj, *repr;
    name_obj = py_digest_name(EVP_MD_CTX_md(self->ctx));
    if (!name_obj) {
        return NULL;
    }
    repr = PyUnicode_FromFormat("<%U %s object @ %p>",
                                name_obj, Py_TYPE(self)->tp_name, self);
    Py_DECREF(name_obj);
    return repr;
}

PyDoc_STRVAR(hashtype_doc,
"HASH(name, string=b\'\')\n"
"--\n"
"\n"
"A hash is an object used to calculate a checksum of a string of information.\n"
"\n"
"Methods:\n"
"\n"
"update() -- updates the current digest with an additional string\n"
"digest() -- return the current digest value\n"
"hexdigest() -- return the current digest as a string of hexadecimal digits\n"
"copy() -- return a copy of the current hash object\n"
"\n"
"Attributes:\n"
"\n"
"name -- the hash algorithm being used by this object\n"
"digest_size -- number of bytes in this hashes output");

static PyType_Slot EVPtype_slots[] = {
    {Py_tp_dealloc, EVP_dealloc},
    {Py_tp_repr, EVP_repr},
    {Py_tp_doc, (char *)hashtype_doc},
    {Py_tp_methods, EVP_methods},
    {Py_tp_getset, EVP_getseters},
    {Py_tp_new, _disabled_new},
    {0, 0},
};

static PyType_Spec EVPtype_spec = {
    "_hashlib.HASH",    /*tp_name*/
    sizeof(EVPobject),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    EVPtype_slots
};

#ifdef PY_OPENSSL_HAS_SHAKE

/*[clinic input]
_hashlib.HASHXOF.digest as EVPXOF_digest

  length: Py_ssize_t

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
EVPXOF_digest_impl(EVPobject *self, Py_ssize_t length)
/*[clinic end generated code: output=ef9320c23280efad input=816a6537cea3d1db]*/
{
    EVP_MD_CTX *temp_ctx;
    PyObject *retval = PyBytes_FromStringAndSize(NULL, length);

    if (retval == NULL) {
        return NULL;
    }

    temp_ctx = EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        Py_DECREF(retval);
        PyErr_NoMemory();
        return NULL;
    }

    if (!locked_EVP_MD_CTX_copy(temp_ctx, self)) {
        Py_DECREF(retval);
        EVP_MD_CTX_free(temp_ctx);
        return _setException(PyExc_ValueError);
    }
    if (!EVP_DigestFinalXOF(temp_ctx,
                            (unsigned char*)PyBytes_AS_STRING(retval),
                            length)) {
        Py_DECREF(retval);
        EVP_MD_CTX_free(temp_ctx);
        _setException(PyExc_ValueError);
        return NULL;
    }

    EVP_MD_CTX_free(temp_ctx);
    return retval;
}

/*[clinic input]
_hashlib.HASHXOF.hexdigest as EVPXOF_hexdigest

    length: Py_ssize_t

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
EVPXOF_hexdigest_impl(EVPobject *self, Py_ssize_t length)
/*[clinic end generated code: output=eb3e6ee7788bf5b2 input=5f9d6a8f269e34df]*/
{
    unsigned char *digest;
    EVP_MD_CTX *temp_ctx;
    PyObject *retval;

    digest = (unsigned char*)PyMem_Malloc(length);
    if (digest == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    temp_ctx = EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        PyMem_Free(digest);
        PyErr_NoMemory();
        return NULL;
    }

    /* Get the raw (binary) digest value */
    if (!locked_EVP_MD_CTX_copy(temp_ctx, self)) {
        PyMem_Free(digest);
        EVP_MD_CTX_free(temp_ctx);
        return _setException(PyExc_ValueError);
    }
    if (!EVP_DigestFinalXOF(temp_ctx, digest, length)) {
        PyMem_Free(digest);
        EVP_MD_CTX_free(temp_ctx);
        _setException(PyExc_ValueError);
        return NULL;
    }

    EVP_MD_CTX_free(temp_ctx);

    retval = _Py_strhex((const char *)digest, length);
    PyMem_Free(digest);
    return retval;
}

static PyMethodDef EVPXOF_methods[] = {
    EVPXOF_DIGEST_METHODDEF
    EVPXOF_HEXDIGEST_METHODDEF
    {NULL, NULL}  /* sentinel */
};


static PyObject *
EVPXOF_get_digest_size(EVPobject *self, void *closure)
{
    return PyLong_FromLong(0);
}

static PyGetSetDef EVPXOF_getseters[] = {
    {"digest_size",
     (getter)EVPXOF_get_digest_size, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};

PyDoc_STRVAR(hashxoftype_doc,
"HASHXOF(name, string=b\'\')\n"
"--\n"
"\n"
"A hash is an object used to calculate a checksum of a string of information.\n"
"\n"
"Methods:\n"
"\n"
"update() -- updates the current digest with an additional string\n"
"digest(length) -- return the current digest value\n"
"hexdigest(length) -- return the current digest as a string of hexadecimal digits\n"
"copy() -- return a copy of the current hash object\n"
"\n"
"Attributes:\n"
"\n"
"name -- the hash algorithm being used by this object\n"
"digest_size -- number of bytes in this hashes output");

static PyType_Slot EVPXOFtype_slots[] = {
    {Py_tp_doc, (char *)hashxoftype_doc},
    {Py_tp_methods, EVPXOF_methods},
    {Py_tp_getset, EVPXOF_getseters},
    {Py_tp_new, _disabled_new},
    {0, 0},
};

static PyType_Spec EVPXOFtype_spec = {
    "_hashlib.HASHXOF",    /*tp_name*/
    sizeof(EVPobject),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    EVPXOFtype_slots
};


#endif

static PyObject *
EVPnew(PyObject *module, const EVP_MD *digest,
       const unsigned char *cp, Py_ssize_t len, int usedforsecurity)
{
    int result = 0;
    EVPobject *self;
    PyTypeObject *type = get_hashlib_state(module)->EVPtype;

    if (!digest) {
        PyErr_SetString(PyExc_ValueError, "unsupported hash type");
        return NULL;
    }

#ifdef PY_OPENSSL_HAS_SHAKE
    if ((EVP_MD_flags(digest) & EVP_MD_FLAG_XOF) == EVP_MD_FLAG_XOF) {
        type = get_hashlib_state(module)->EVPXOFtype;
    }
#endif

    if ((self = newEVPobject(type)) == NULL)
        return NULL;

    if (!usedforsecurity) {
#ifdef EVP_MD_CTX_FLAG_NON_FIPS_ALLOW
        EVP_MD_CTX_set_flags(self->ctx, EVP_MD_CTX_FLAG_NON_FIPS_ALLOW);
#endif
    }


    if (!EVP_DigestInit_ex(self->ctx, digest, NULL)) {
        _setException(PyExc_ValueError);
        Py_DECREF(self);
        return NULL;
    }

    if (cp && len) {
        if (len >= HASHLIB_GIL_MINSIZE) {
            Py_BEGIN_ALLOW_THREADS
            result = EVP_hash(self, cp, len);
            Py_END_ALLOW_THREADS
        } else {
            result = EVP_hash(self, cp, len);
        }
        if (result == -1) {
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject *)self;
}


/* The module-level function: new() */

/*[clinic input]
_hashlib.new as EVP_new

    name as name_obj: object
    string as data_obj: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True

Return a new hash object using the named algorithm.

An optional string argument may be provided and will be
automatically hashed.

The MD5 and SHA1 algorithms are always supported.
[clinic start generated code]*/

static PyObject *
EVP_new_impl(PyObject *module, PyObject *name_obj, PyObject *data_obj,
             int usedforsecurity)
/*[clinic end generated code: output=ddd5053f92dffe90 input=c24554d0337be1b0]*/
{
    Py_buffer view = { 0 };
    PyObject *ret_obj;
    char *name;
    const EVP_MD *digest = NULL;

    if (!PyArg_Parse(name_obj, "s", &name)) {
        PyErr_SetString(PyExc_TypeError, "name must be a string");
        return NULL;
    }

    if (data_obj)
        GET_BUFFER_VIEW_OR_ERROUT(data_obj, &view);

    digest = py_digest_by_name(name);

    ret_obj = EVPnew(module, digest,
                     (unsigned char*)view.buf, view.len,
                     usedforsecurity);

    if (data_obj)
        PyBuffer_Release(&view);
    return ret_obj;
}

static PyObject*
EVP_fast_new(PyObject *module, PyObject *data_obj, const EVP_MD *digest,
             int usedforsecurity)
{
    Py_buffer view = { 0 };
    PyObject *ret_obj;

    if (data_obj)
        GET_BUFFER_VIEW_OR_ERROUT(data_obj, &view);

    ret_obj = EVPnew(module, digest,
                     (unsigned char*)view.buf, view.len,
                     usedforsecurity);

    if (data_obj)
        PyBuffer_Release(&view);

    return ret_obj;
}

/*[clinic input]
_hashlib.openssl_md5

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a md5 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_md5_impl(PyObject *module, PyObject *data_obj,
                          int usedforsecurity)
/*[clinic end generated code: output=87b0186440a44f8c input=990e36d5e689b16e]*/
{
    return EVP_fast_new(module, data_obj, EVP_md5(), usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha1

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha1 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha1_impl(PyObject *module, PyObject *data_obj,
                           int usedforsecurity)
/*[clinic end generated code: output=6813024cf690670d input=948f2f4b6deabc10]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha1(), usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha224

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha224 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha224_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity)
/*[clinic end generated code: output=a2dfe7cc4eb14ebb input=f9272821fadca505]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha224(), usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha256

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha256 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha256_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity)
/*[clinic end generated code: output=1f874a34870f0a68 input=549fad9d2930d4c5]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha256(), usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha384

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha384 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha384_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity)
/*[clinic end generated code: output=58529eff9ca457b2 input=48601a6e3bf14ad7]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha384(), usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha512

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha512 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha512_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity)
/*[clinic end generated code: output=2c744c9e4a40d5f6 input=c5c46a2a817aa98f]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha512(), usedforsecurity);
}


#ifdef PY_OPENSSL_HAS_SHA3

/*[clinic input]
_hashlib.openssl_sha3_224

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha3-224 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_224_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity)
/*[clinic end generated code: output=144641c1d144b974 input=e3a01b2888916157]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha3_224(), usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_256

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha3-256 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_256_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity)
/*[clinic end generated code: output=c61f1ab772d06668 input=e2908126c1b6deed]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha3_256(), usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_384

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha3-384 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_384_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity)
/*[clinic end generated code: output=f68e4846858cf0ee input=ec0edf5c792f8252]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha3_384(), usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_512

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha3-512 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_512_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity)
/*[clinic end generated code: output=2eede478c159354a input=64e2cc0c094d56f4]*/
{
    return EVP_fast_new(module, data_obj, EVP_sha3_512(), usedforsecurity);
}
#endif /* PY_OPENSSL_HAS_SHA3 */

#ifdef PY_OPENSSL_HAS_SHAKE
/*[clinic input]
_hashlib.openssl_shake_128

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a shake-128 variable hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_shake_128_impl(PyObject *module, PyObject *data_obj,
                                int usedforsecurity)
/*[clinic end generated code: output=bc49cdd8ada1fa97 input=6c9d67440eb33ec8]*/
{
    return EVP_fast_new(module, data_obj, EVP_shake128(), usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_shake_256

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a shake-256 variable hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_shake_256_impl(PyObject *module, PyObject *data_obj,
                                int usedforsecurity)
/*[clinic end generated code: output=358d213be8852df7 input=479cbe9fefd4a9f8]*/
{
    return EVP_fast_new(module, data_obj, EVP_shake256(), usedforsecurity);
}
#endif /* PY_OPENSSL_HAS_SHAKE */

/*[clinic input]
_hashlib.pbkdf2_hmac as pbkdf2_hmac

    hash_name: str
    password: Py_buffer
    salt: Py_buffer
    iterations: long
    dklen as dklen_obj: object = None

Password based key derivation function 2 (PKCS #5 v2.0) with HMAC as pseudorandom function.
[clinic start generated code]*/

static PyObject *
pbkdf2_hmac_impl(PyObject *module, const char *hash_name,
                 Py_buffer *password, Py_buffer *salt, long iterations,
                 PyObject *dklen_obj)
/*[clinic end generated code: output=144b76005416599b input=ed3ab0d2d28b5d5c]*/
{
    PyObject *key_obj = NULL;
    char *key;
    long dklen;
    int retval;
    const EVP_MD *digest;

    digest = py_digest_by_name(hash_name);
    if (digest == NULL) {
        PyErr_SetString(PyExc_ValueError, "unsupported hash type");
        goto end;
    }

    if (password->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "password is too long.");
        goto end;
    }

    if (salt->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "salt is too long.");
        goto end;
    }

    if (iterations < 1) {
        PyErr_SetString(PyExc_ValueError,
                        "iteration value must be greater than 0.");
        goto end;
    }
    if (iterations > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "iteration value is too great.");
        goto end;
    }

    if (dklen_obj == Py_None) {
        dklen = EVP_MD_size(digest);
    } else {
        dklen = PyLong_AsLong(dklen_obj);
        if ((dklen == -1) && PyErr_Occurred()) {
            goto end;
        }
    }
    if (dklen < 1) {
        PyErr_SetString(PyExc_ValueError,
                        "key length must be greater than 0.");
        goto end;
    }
    if (dklen > INT_MAX) {
        /* INT_MAX is always smaller than dkLen max (2^32 - 1) * hLen */
        PyErr_SetString(PyExc_OverflowError,
                        "key length is too great.");
        goto end;
    }

    key_obj = PyBytes_FromStringAndSize(NULL, dklen);
    if (key_obj == NULL) {
        goto end;
    }
    key = PyBytes_AS_STRING(key_obj);

    Py_BEGIN_ALLOW_THREADS
    retval = PKCS5_PBKDF2_HMAC((char*)password->buf, (int)password->len,
                               (unsigned char *)salt->buf, (int)salt->len,
                               iterations, digest, dklen,
                               (unsigned char *)key);
    Py_END_ALLOW_THREADS

    if (!retval) {
        Py_CLEAR(key_obj);
        _setException(PyExc_ValueError);
        goto end;
    }

  end:
    return key_obj;
}

#if OPENSSL_VERSION_NUMBER > 0x10100000L && !defined(OPENSSL_NO_SCRYPT) && !defined(LIBRESSL_VERSION_NUMBER)
#define PY_SCRYPT 1

/* XXX: Parameters salt, n, r and p should be required keyword-only parameters.
   They are optional in the Argument Clinic declaration only due to a
   limitation of PyArg_ParseTupleAndKeywords. */

/*[clinic input]
_hashlib.scrypt

    password: Py_buffer
    *
    salt: Py_buffer = None
    n as n_obj: object(subclass_of='&PyLong_Type') = None
    r as r_obj: object(subclass_of='&PyLong_Type') = None
    p as p_obj: object(subclass_of='&PyLong_Type') = None
    maxmem: long = 0
    dklen: long = 64


scrypt password-based key derivation function.
[clinic start generated code]*/

static PyObject *
_hashlib_scrypt_impl(PyObject *module, Py_buffer *password, Py_buffer *salt,
                     PyObject *n_obj, PyObject *r_obj, PyObject *p_obj,
                     long maxmem, long dklen)
/*[clinic end generated code: output=14849e2aa2b7b46c input=48a7d63bf3f75c42]*/
{
    PyObject *key_obj = NULL;
    char *key;
    int retval;
    unsigned long n, r, p;

    if (password->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "password is too long.");
        return NULL;
    }

    if (salt->buf == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "salt is required");
        return NULL;
    }
    if (salt->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "salt is too long.");
        return NULL;
    }

    n = PyLong_AsUnsignedLong(n_obj);
    if (n == (unsigned long) -1 && PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError,
                        "n is required and must be an unsigned int");
        return NULL;
    }
    if (n < 2 || n & (n - 1)) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a power of 2.");
        return NULL;
    }

    r = PyLong_AsUnsignedLong(r_obj);
    if (r == (unsigned long) -1 && PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError,
                         "r is required and must be an unsigned int");
        return NULL;
    }

    p = PyLong_AsUnsignedLong(p_obj);
    if (p == (unsigned long) -1 && PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError,
                         "p is required and must be an unsigned int");
        return NULL;
    }

    if (maxmem < 0 || maxmem > INT_MAX) {
        /* OpenSSL 1.1.0 restricts maxmem to 32 MiB. It may change in the
           future. The maxmem constant is private to OpenSSL. */
        PyErr_Format(PyExc_ValueError,
                     "maxmem must be positive and smaller than %d",
                      INT_MAX);
        return NULL;
    }

    if (dklen < 1 || dklen > INT_MAX) {
        PyErr_Format(PyExc_ValueError,
                    "dklen must be greater than 0 and smaller than %d",
                    INT_MAX);
        return NULL;
    }

    /* let OpenSSL validate the rest */
    retval = EVP_PBE_scrypt(NULL, 0, NULL, 0, n, r, p, maxmem, NULL, 0);
    if (!retval) {
        /* sorry, can't do much better */
        PyErr_SetString(PyExc_ValueError,
                        "Invalid parameter combination for n, r, p, maxmem.");
        return NULL;
   }

    key_obj = PyBytes_FromStringAndSize(NULL, dklen);
    if (key_obj == NULL) {
        return NULL;
    }
    key = PyBytes_AS_STRING(key_obj);

    Py_BEGIN_ALLOW_THREADS
    retval = EVP_PBE_scrypt(
        (const char*)password->buf, (size_t)password->len,
        (const unsigned char *)salt->buf, (size_t)salt->len,
        n, r, p, maxmem,
        (unsigned char *)key, (size_t)dklen
    );
    Py_END_ALLOW_THREADS

    if (!retval) {
        Py_CLEAR(key_obj);
        _setException(PyExc_ValueError);
        return NULL;
    }
    return key_obj;
}
#endif

/* Fast HMAC for hmac.digest()
 */

/*[clinic input]
_hashlib.hmac_digest as _hashlib_hmac_singleshot

    key: Py_buffer
    msg: Py_buffer
    digest: str

Single-shot HMAC.
[clinic start generated code]*/

static PyObject *
_hashlib_hmac_singleshot_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *msg, const char *digest)
/*[clinic end generated code: output=15658ede5ab98185 input=019dffc571909a46]*/
{
    unsigned char md[EVP_MAX_MD_SIZE] = {0};
    unsigned int md_len = 0;
    unsigned char *result;
    const EVP_MD *evp;

    evp = py_digest_by_name(digest);
    if (evp == NULL) {
        PyErr_SetString(PyExc_ValueError, "unsupported hash type");
        return NULL;
    }
    if (key->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "key is too long.");
        return NULL;
    }
    if (msg->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "msg is too long.");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    result = HMAC(
        evp,
        (const void*)key->buf, (int)key->len,
        (const unsigned char*)msg->buf, (int)msg->len,
        md, &md_len
    );
    Py_END_ALLOW_THREADS

    if (result == NULL) {
        _setException(PyExc_ValueError);
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char*)md, md_len);
}

/* OpenSSL-based HMAC implementation
 */

static int _hmac_update(HMACobject*, PyObject*);

/*[clinic input]
_hashlib.hmac_new

    key: Py_buffer
    msg as msg_obj: object(c_default="NULL") = b''
    digestmod: str(c_default="NULL") = None

Return a new hmac object.
[clinic start generated code]*/

static PyObject *
_hashlib_hmac_new_impl(PyObject *module, Py_buffer *key, PyObject *msg_obj,
                       const char *digestmod)
/*[clinic end generated code: output=9a35673be0cbea1b input=a0878868eb190134]*/
{
    PyTypeObject *type = get_hashlib_state(module)->HMACtype;
    const EVP_MD *digest;
    HMAC_CTX *ctx = NULL;
    HMACobject *self = NULL;
    int r;

    if (key->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "key is too long.");
        return NULL;
    }

    if ((digestmod == NULL) || !strlen(digestmod)) {
        PyErr_SetString(
            PyExc_TypeError, "Missing required parameter 'digestmod'.");
        return NULL;
    }

    digest = py_digest_by_name(digestmod);
    if (!digest) {
        PyErr_SetString(PyExc_ValueError, "unknown hash function");
        return NULL;
    }

    ctx = HMAC_CTX_new();
    if (ctx == NULL) {
        _setException(PyExc_ValueError);
        goto error;
    }

    r = HMAC_Init_ex(
        ctx,
        (const char*)key->buf,
        (int)key->len,
        digest,
        NULL /*impl*/);
    if (r == 0) {
        _setException(PyExc_ValueError);
        goto error;
    }

    self = (HMACobject *)PyObject_New(HMACobject, type);
    if (self == NULL) {
        goto error;
    }

    self->ctx = ctx;
    self->lock = NULL;

    if ((msg_obj != NULL) && (msg_obj != Py_None)) {
        if (!_hmac_update(self, msg_obj))
            goto error;
    }

    return (PyObject*)self;

error:
    if (ctx) HMAC_CTX_free(ctx);
    if (self) PyObject_Del(self);
    return NULL;
}

/* helper functions */
static int
locked_HMAC_CTX_copy(HMAC_CTX *new_ctx_p, HMACobject *self)
{
    int result;
    ENTER_HASHLIB(self);
    result = HMAC_CTX_copy(new_ctx_p, self->ctx);
    LEAVE_HASHLIB(self);
    return result;
}

static unsigned int
_hmac_digest_size(HMACobject *self)
{
    unsigned int digest_size = EVP_MD_size(HMAC_CTX_get_md(self->ctx));
    assert(digest_size <= EVP_MAX_MD_SIZE);
    return digest_size;
}

static int
_hmac_update(HMACobject *self, PyObject *obj)
{
    int r;
    Py_buffer view = {0};

    GET_BUFFER_VIEW_OR_ERROR(obj, &view, return 0);

    if (self->lock == NULL && view.len >= HASHLIB_GIL_MINSIZE) {
        self->lock = PyThread_allocate_lock();
        /* fail? lock = NULL and we fail over to non-threaded code. */
    }

    if (self->lock != NULL) {
        ENTER_HASHLIB(self);
        r = HMAC_Update(self->ctx, (const unsigned char*)view.buf, view.len);
        LEAVE_HASHLIB(self);
    } else {
        r = HMAC_Update(self->ctx, (const unsigned char*)view.buf, view.len);
    }

    PyBuffer_Release(&view);

    if (r == 0) {
        _setException(PyExc_ValueError);
        return 0;
    }
    return 1;
}

/*[clinic input]
_hashlib.HMAC.copy

Return a copy ("clone") of the HMAC object.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_copy_impl(HMACobject *self)
/*[clinic end generated code: output=29aa28b452833127 input=e2fa6a05db61a4d6]*/
{
    HMACobject *retval;

    HMAC_CTX *ctx = HMAC_CTX_new();
    if (ctx == NULL) {
        return _setException(PyExc_ValueError);
    }
    if (!locked_HMAC_CTX_copy(ctx, self)) {
        HMAC_CTX_free(ctx);
        return _setException(PyExc_ValueError);
    }

    retval = (HMACobject *)PyObject_New(HMACobject, Py_TYPE(self));
    if (retval == NULL) {
        HMAC_CTX_free(ctx);
        return NULL;
    }
    retval->ctx = ctx;
    retval->lock = NULL;

    return (PyObject *)retval;
}

static void
_hmac_dealloc(HMACobject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    if (self->lock != NULL) {
        PyThread_free_lock(self->lock);
    }
    HMAC_CTX_free(self->ctx);
    PyObject_Del(self);
    Py_DECREF(tp);
}

static PyObject *
_hmac_repr(HMACobject *self)
{
    PyObject *digest_name = py_digest_name(HMAC_CTX_get_md(self->ctx));
    if (digest_name == NULL) {
        return NULL;
    }
    PyObject *repr = PyUnicode_FromFormat(
        "<%U HMAC object @ %p>", digest_name, self
    );
    Py_DECREF(digest_name);
    return repr;
}

/*[clinic input]
_hashlib.HMAC.update
    msg: object

Update the HMAC object with msg.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_update_impl(HMACobject *self, PyObject *msg)
/*[clinic end generated code: output=f31f0ace8c625b00 input=1829173bb3cfd4e6]*/
{
    if (!_hmac_update(self, msg)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
_hmac_digest(HMACobject *self, unsigned char *buf, unsigned int len)
{
    HMAC_CTX *temp_ctx = HMAC_CTX_new();
    if (temp_ctx == NULL) {
        PyErr_NoMemory();
        return 0;
    }
    if (!locked_HMAC_CTX_copy(temp_ctx, self)) {
        _setException(PyExc_ValueError);
        return 0;
    }
    int r = HMAC_Final(temp_ctx, buf, &len);
    HMAC_CTX_free(temp_ctx);
    if (r == 0) {
        _setException(PyExc_ValueError);
        return 0;
    }
    return 1;
}

/*[clinic input]
_hashlib.HMAC.digest
Return the digest of the bytes passed to the update() method so far.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_digest_impl(HMACobject *self)
/*[clinic end generated code: output=1b1424355af7a41e input=bff07f74da318fb4]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_size = _hmac_digest_size(self);
    if (digest_size == 0) {
        return _setException(PyExc_ValueError);
    }
    int r = _hmac_digest(self, digest, digest_size);
    if (r == 0) {
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char *)digest, digest_size);
}

/*[clinic input]
_hashlib.HMAC.hexdigest

Return hexadecimal digest of the bytes passed to the update() method so far.

This may be used to exchange the value safely in email or other non-binary
environments.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_hexdigest_impl(HMACobject *self)
/*[clinic end generated code: output=80d825be1eaae6a7 input=5abc42702874ddcf]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_size = _hmac_digest_size(self);
    if (digest_size == 0) {
        return _setException(PyExc_ValueError);
    }
    int r = _hmac_digest(self, digest, digest_size);
    if (r == 0) {
        return NULL;
    }
    return _Py_strhex((const char *)digest, digest_size);
}

static PyObject *
_hashlib_hmac_get_digest_size(HMACobject *self, void *closure)
{
    unsigned int digest_size = _hmac_digest_size(self);
    if (digest_size == 0) {
        return _setException(PyExc_ValueError);
    }
    return PyLong_FromLong(digest_size);
}

static PyObject *
_hashlib_hmac_get_block_size(HMACobject *self, void *closure)
{
    const EVP_MD *md = HMAC_CTX_get_md(self->ctx);
    if (md == NULL) {
        return _setException(PyExc_ValueError);
    }
    return PyLong_FromLong(EVP_MD_block_size(md));
}

static PyObject *
_hashlib_hmac_get_name(HMACobject *self, void *closure)
{
    PyObject *digest_name = py_digest_name(HMAC_CTX_get_md(self->ctx));
    if (digest_name == NULL) {
        return NULL;
    }
    PyObject *name = PyUnicode_FromFormat("hmac-%U", digest_name);
    Py_DECREF(digest_name);
    return name;
}

static PyMethodDef HMAC_methods[] = {
    _HASHLIB_HMAC_UPDATE_METHODDEF
    _HASHLIB_HMAC_DIGEST_METHODDEF
    _HASHLIB_HMAC_HEXDIGEST_METHODDEF
    _HASHLIB_HMAC_COPY_METHODDEF
    {NULL, NULL}  /* sentinel */
};

static PyGetSetDef HMAC_getset[] = {
    {"digest_size", (getter)_hashlib_hmac_get_digest_size, NULL, NULL, NULL},
    {"block_size", (getter)_hashlib_hmac_get_block_size, NULL, NULL, NULL},
    {"name", (getter)_hashlib_hmac_get_name, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};


PyDoc_STRVAR(hmactype_doc,
"The object used to calculate HMAC of a message.\n\
\n\
Methods:\n\
\n\
update() -- updates the current digest with an additional string\n\
digest() -- return the current digest value\n\
hexdigest() -- return the current digest as a string of hexadecimal digits\n\
copy() -- return a copy of the current hash object\n\
\n\
Attributes:\n\
\n\
name -- the name, including the hash algorithm used by this object\n\
digest_size -- number of bytes in digest() output\n");

static PyType_Slot HMACtype_slots[] = {
    {Py_tp_doc, (char *)hmactype_doc},
    {Py_tp_repr, (reprfunc)_hmac_repr},
    {Py_tp_dealloc,(destructor)_hmac_dealloc},
    {Py_tp_methods, HMAC_methods},
    {Py_tp_getset, HMAC_getset},
    {Py_tp_new, _disabled_new},
    {0, NULL}
};

PyType_Spec HMACtype_spec = {
    "_hashlib.HMAC",    /* name */
    sizeof(HMACobject),     /* basicsize */
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = HMACtype_slots,
};


/* State for our callback function so that it can accumulate a result. */
typedef struct _internal_name_mapper_state {
    PyObject *set;
    int error;
} _InternalNameMapperState;


/* A callback function to pass to OpenSSL's OBJ_NAME_do_all(...) */
static void
_openssl_hash_name_mapper(const EVP_MD *md, const char *from,
                          const char *to, void *arg)
{
    _InternalNameMapperState *state = (_InternalNameMapperState *)arg;
    PyObject *py_name;

    assert(state != NULL);
    if (md == NULL)
        return;

    py_name = py_digest_name(md);
    if (py_name == NULL) {
        state->error = 1;
    } else {
        if (PySet_Add(state->set, py_name) != 0) {
            state->error = 1;
        }
        Py_DECREF(py_name);
    }
}


/* Ask OpenSSL for a list of supported ciphers, filling in a Python set. */
static int
hashlib_md_meth_names(PyObject *module)
{
    _InternalNameMapperState state = {
        .set = PyFrozenSet_New(NULL),
        .error = 0
    };
    if (state.set == NULL) {
        return -1;
    }

    EVP_MD_do_all(&_openssl_hash_name_mapper, &state);

    if (state.error) {
        Py_DECREF(state.set);
        return -1;
    }

    if (PyModule_AddObject(module, "openssl_md_meth_names", state.set) < 0) {
        Py_DECREF(state.set);
        return -1;
    }

    return 0;
}

/* LibreSSL doesn't support FIPS:
   https://marc.info/?l=openbsd-misc&m=139819485423701&w=2

   Ted Unangst wrote: "I figured I should mention our current libressl policy
   wrt FIPS mode.  It's gone and it's not coming back." */
#ifndef LIBRESSL_VERSION_NUMBER
/*[clinic input]
_hashlib.get_fips_mode -> int

Determine the OpenSSL FIPS mode of operation.

For OpenSSL 3.0.0 and newer it returns the state of the default provider
in the default OSSL context. It's not quite the same as FIPS_mode() but good
enough for unittests.

Effectively any non-zero return value indicates FIPS mode;
values other than 1 may have additional significance.
[clinic start generated code]*/

static int
_hashlib_get_fips_mode_impl(PyObject *module)
/*[clinic end generated code: output=87eece1bab4d3fa9 input=2db61538c41c6fef]*/

{
    int result;
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    result = EVP_default_properties_is_fips_enabled(NULL);
#else
    ERR_clear_error();
    result = FIPS_mode();
    if (result == 0) {
        // "If the library was built without support of the FIPS Object Module,
        // then the function will return 0 with an error code of
        // CRYPTO_R_FIPS_MODE_NOT_SUPPORTED (0x0f06d065)."
        // But 0 is also a valid result value.
        unsigned long errcode = ERR_peek_last_error();
        if (errcode) {
            _setException(PyExc_ValueError);
            return -1;
        }
    }
    return result;
#endif
}
#endif  // !LIBRESSL_VERSION_NUMBER


static int
_tscmp(const unsigned char *a, const unsigned char *b,
        Py_ssize_t len_a, Py_ssize_t len_b)
{
    /* loop count depends on length of b. Might leak very little timing
     * information if sizes are different.
     */
    Py_ssize_t length = len_b;
    const void *left = a;
    const void *right = b;
    int result = 0;

    if (len_a != length) {
        left = b;
        result = 1;
    }

    result |= CRYPTO_memcmp(left, right, length);

    return (result == 0);
}

/* NOTE: Keep in sync with _operator.c implementation. */

/*[clinic input]
_hashlib.compare_digest

    a: object
    b: object
    /

Return 'a == b'.

This function uses an approach designed to prevent
timing analysis, making it appropriate for cryptography.

a and b must both be of the same type: either str (ASCII only),
or any bytes-like object.

Note: If a and b are of different lengths, or if an error occurs,
a timing attack could theoretically reveal information about the
types and lengths of a and b--but not their values.
[clinic start generated code]*/

static PyObject *
_hashlib_compare_digest_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=6f1c13927480aed9 input=9c40c6e566ca12f5]*/
{
    int rc;

    /* ASCII unicode string */
    if(PyUnicode_Check(a) && PyUnicode_Check(b)) {
        if (PyUnicode_READY(a) == -1 || PyUnicode_READY(b) == -1) {
            return NULL;
        }
        if (!PyUnicode_IS_ASCII(a) || !PyUnicode_IS_ASCII(b)) {
            PyErr_SetString(PyExc_TypeError,
                            "comparing strings with non-ASCII characters is "
                            "not supported");
            return NULL;
        }

        rc = _tscmp(PyUnicode_DATA(a),
                    PyUnicode_DATA(b),
                    PyUnicode_GET_LENGTH(a),
                    PyUnicode_GET_LENGTH(b));
    }
    /* fallback to buffer interface for bytes, bytesarray and other */
    else {
        Py_buffer view_a;
        Py_buffer view_b;

        if (PyObject_CheckBuffer(a) == 0 && PyObject_CheckBuffer(b) == 0) {
            PyErr_Format(PyExc_TypeError,
                         "unsupported operand types(s) or combination of types: "
                         "'%.100s' and '%.100s'",
                         Py_TYPE(a)->tp_name, Py_TYPE(b)->tp_name);
            return NULL;
        }

        if (PyObject_GetBuffer(a, &view_a, PyBUF_SIMPLE) == -1) {
            return NULL;
        }
        if (view_a.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            return NULL;
        }

        if (PyObject_GetBuffer(b, &view_b, PyBUF_SIMPLE) == -1) {
            PyBuffer_Release(&view_a);
            return NULL;
        }
        if (view_b.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            PyBuffer_Release(&view_b);
            return NULL;
        }

        rc = _tscmp((const unsigned char*)view_a.buf,
                    (const unsigned char*)view_b.buf,
                    view_a.len,
                    view_b.len);

        PyBuffer_Release(&view_a);
        PyBuffer_Release(&view_b);
    }

    return PyBool_FromLong(rc);
}

/* List of functions exported by this module */

static struct PyMethodDef EVP_functions[] = {
    EVP_NEW_METHODDEF
    PBKDF2_HMAC_METHODDEF
    _HASHLIB_SCRYPT_METHODDEF
    _HASHLIB_GET_FIPS_MODE_METHODDEF
    _HASHLIB_COMPARE_DIGEST_METHODDEF
    _HASHLIB_HMAC_SINGLESHOT_METHODDEF
    _HASHLIB_HMAC_NEW_METHODDEF
    _HASHLIB_OPENSSL_MD5_METHODDEF
    _HASHLIB_OPENSSL_SHA1_METHODDEF
    _HASHLIB_OPENSSL_SHA224_METHODDEF
    _HASHLIB_OPENSSL_SHA256_METHODDEF
    _HASHLIB_OPENSSL_SHA384_METHODDEF
    _HASHLIB_OPENSSL_SHA512_METHODDEF
    _HASHLIB_OPENSSL_SHA3_224_METHODDEF
    _HASHLIB_OPENSSL_SHA3_256_METHODDEF
    _HASHLIB_OPENSSL_SHA3_384_METHODDEF
    _HASHLIB_OPENSSL_SHA3_512_METHODDEF
    _HASHLIB_OPENSSL_SHAKE_128_METHODDEF
    _HASHLIB_OPENSSL_SHAKE_256_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};


/* Initialize this module. */

static int
hashlib_traverse(PyObject *m, visitproc visit, void *arg)
{
    _hashlibstate *state = get_hashlib_state(m);
    Py_VISIT(state->EVPtype);
    Py_VISIT(state->HMACtype);
#ifdef PY_OPENSSL_HAS_SHAKE
    Py_VISIT(state->EVPXOFtype);
#endif
    return 0;
}

static int
hashlib_clear(PyObject *m)
{
    _hashlibstate *state = get_hashlib_state(m);
    Py_CLEAR(state->EVPtype);
    Py_CLEAR(state->HMACtype);
#ifdef PY_OPENSSL_HAS_SHAKE
    Py_CLEAR(state->EVPXOFtype);
#endif
    return 0;
}

static void
hashlib_free(void *m)
{
    hashlib_clear((PyObject *)m);
}

/* Py_mod_exec functions */
static int
hashlib_openssl_legacy_init(PyObject *module)
{
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(LIBRESSL_VERSION_NUMBER)
    /* Load all digest algorithms and initialize cpuid */
    OPENSSL_add_all_algorithms_noconf();
    ERR_load_crypto_strings();
#endif
    return 0;
}

static int
hashlib_init_evptype(PyObject *module)
{
    _hashlibstate *state = get_hashlib_state(module);

    state->EVPtype = (PyTypeObject *)PyType_FromSpec(&EVPtype_spec);
    if (state->EVPtype == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->EVPtype) < 0) {
        return -1;
    }
    return 0;
}

static int
hashlib_init_evpxoftype(PyObject *module)
{
#ifdef PY_OPENSSL_HAS_SHAKE
    _hashlibstate *state = get_hashlib_state(module);
    PyObject *bases;

    if (state->EVPtype == NULL) {
        return -1;
    }

    bases = PyTuple_Pack(1, state->EVPtype);
    if (bases == NULL) {
        return -1;
    }

    state->EVPXOFtype = (PyTypeObject *)PyType_FromSpecWithBases(
        &EVPXOFtype_spec, bases
    );
    Py_DECREF(bases);
    if (state->EVPXOFtype == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->EVPXOFtype) < 0) {
        return -1;
    }
#endif
    return 0;
}

static int
hashlib_init_hmactype(PyObject *module)
{
    _hashlibstate *state = get_hashlib_state(module);

    state->HMACtype = (PyTypeObject *)PyType_FromSpec(&HMACtype_spec);
    if (state->HMACtype == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->HMACtype) < 0) {
        return -1;
    }
    return 0;
}

#if 0
static PyModuleDef_Slot hashlib_slots[] = {
    /* OpenSSL 1.0.2 and LibreSSL */
    {Py_mod_exec, hashlib_openssl_legacy_init},
    {Py_mod_exec, hashlib_init_evptype},
    {Py_mod_exec, hashlib_init_evpxoftype},
    {Py_mod_exec, hashlib_init_hmactype},
    {Py_mod_exec, hashlib_md_meth_names},
    {0, NULL}
};
#endif

static struct PyModuleDef _hashlibmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_hashlib",
    .m_doc = "OpenSSL interface for hashlib module",
    .m_size = sizeof(_hashlibstate),
    .m_methods = EVP_functions,
    .m_slots = NULL,
    .m_traverse = hashlib_traverse,
    .m_clear = hashlib_clear,
    .m_free = hashlib_free
};

PyMODINIT_FUNC
PyInit__hashlib(void)
{
    PyObject *m = PyState_FindModule(&_hashlibmodule);
    if (m != NULL) {
        Py_INCREF(m);
        return m;
    }

    m = PyModule_Create(&_hashlibmodule);
    if (m == NULL) {
        return NULL;
    }

    if (hashlib_openssl_legacy_init(m) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    if (hashlib_init_evptype(m) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    if (hashlib_init_evpxoftype(m) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    if (hashlib_init_hmactype(m) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    if (hashlib_md_meth_names(m) == -1) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
