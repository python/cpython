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
#include "structmember.h"
#include "hashlib.h"


/* EVP is the preferred interface to hashing in OpenSSL */
#include <openssl/evp.h>
#include <openssl/hmac.h>
/* We use the object interface to discover what hashes OpenSSL supports. */
#include <openssl/objects.h>
#include "openssl/err.h"

#define MUNCH_SIZE INT_MAX

#ifndef HASH_OBJ_CONSTRUCTOR
#define HASH_OBJ_CONSTRUCTOR 0
#endif

/* Minimum OpenSSL version needed to support sha224 and higher. */
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x00908000)
#define _OPENSSL_SUPPORTS_SHA2
#endif

typedef struct {
    PyObject_HEAD
    PyObject            *name;  /* name of this hash algorithm */
    EVP_MD_CTX           ctx;   /* OpenSSL message digest context */
#ifdef WITH_THREAD
    PyThread_type_lock   lock;  /* OpenSSL context lock */
#endif
} EVPobject;


static PyTypeObject EVPtype;


#define DEFINE_CONSTS_FOR_NEW(Name)  \
    static PyObject *CONST_ ## Name ## _name_obj = NULL; \
    static EVP_MD_CTX CONST_new_ ## Name ## _ctx; \
    static EVP_MD_CTX *CONST_new_ ## Name ## _ctx_p = NULL;

DEFINE_CONSTS_FOR_NEW(md5)
DEFINE_CONSTS_FOR_NEW(sha1)
#ifdef _OPENSSL_SUPPORTS_SHA2
DEFINE_CONSTS_FOR_NEW(sha224)
DEFINE_CONSTS_FOR_NEW(sha256)
DEFINE_CONSTS_FOR_NEW(sha384)
DEFINE_CONSTS_FOR_NEW(sha512)
#endif


static EVPobject *
newEVPobject(PyObject *name)
{
    EVPobject *retval = (EVPobject *)PyObject_New(EVPobject, &EVPtype);

    /* save the name for .name to return */
    if (retval != NULL) {
        Py_INCREF(name);
        retval->name = name;
#ifdef WITH_THREAD
        retval->lock = NULL;
#endif
    }

    return retval;
}

static void
EVP_hash(EVPobject *self, const void *vp, Py_ssize_t len)
{
    unsigned int process;
    const unsigned char *cp = (const unsigned char *)vp;
    while (0 < len) {
        if (len > (Py_ssize_t)MUNCH_SIZE)
            process = MUNCH_SIZE;
        else
            process = Py_SAFE_DOWNCAST(len, Py_ssize_t, unsigned int);
        EVP_DigestUpdate(&self->ctx, (const void*)cp, process);
        len -= process;
        cp += process;
    }
}

/* Internal methods for a hash object */

static void
EVP_dealloc(EVPobject *self)
{
#ifdef WITH_THREAD
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
#endif
    EVP_MD_CTX_cleanup(&self->ctx);
    Py_XDECREF(self->name);
    PyObject_Del(self);
}

static void locked_EVP_MD_CTX_copy(EVP_MD_CTX *new_ctx_p, EVPobject *self)
{
    ENTER_HASHLIB(self);
    EVP_MD_CTX_copy(new_ctx_p, &self->ctx);
    LEAVE_HASHLIB(self);
}

/* External methods for a hash object */

PyDoc_STRVAR(EVP_copy__doc__, "Return a copy of the hash object.");


static PyObject *
EVP_copy(EVPobject *self, PyObject *unused)
{
    EVPobject *newobj;

    if ( (newobj = newEVPobject(self->name))==NULL)
        return NULL;

    locked_EVP_MD_CTX_copy(&newobj->ctx, self);
    return (PyObject *)newobj;
}

PyDoc_STRVAR(EVP_digest__doc__,
"Return the digest value as a string of binary data.");

static PyObject *
EVP_digest(EVPobject *self, PyObject *unused)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    EVP_MD_CTX temp_ctx;
    PyObject *retval;
    unsigned int digest_size;

    locked_EVP_MD_CTX_copy(&temp_ctx, self);
    digest_size = EVP_MD_CTX_size(&temp_ctx);
    EVP_DigestFinal(&temp_ctx, digest, NULL);

    retval = PyBytes_FromStringAndSize((const char *)digest, digest_size);
    EVP_MD_CTX_cleanup(&temp_ctx);
    return retval;
}

PyDoc_STRVAR(EVP_hexdigest__doc__,
"Return the digest value as a string of hexadecimal digits.");

static PyObject *
EVP_hexdigest(EVPobject *self, PyObject *unused)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    EVP_MD_CTX temp_ctx;
    PyObject *retval;
    char *hex_digest;
    unsigned int i, j, digest_size;

    /* Get the raw (binary) digest value */
    locked_EVP_MD_CTX_copy(&temp_ctx, self);
    digest_size = EVP_MD_CTX_size(&temp_ctx);
    EVP_DigestFinal(&temp_ctx, digest, NULL);

    EVP_MD_CTX_cleanup(&temp_ctx);

    /* Allocate a new buffer */
    hex_digest = PyMem_Malloc(digest_size * 2 + 1);
    if (!hex_digest)
        return PyErr_NoMemory();

    /* Make hex version of the digest */
    for(i=j=0; i<digest_size; i++) {
        unsigned char c;
        c = (digest[i] >> 4) & 0xf;
        hex_digest[j++] = Py_hexdigits[c];
        c = (digest[i] & 0xf);
        hex_digest[j++] = Py_hexdigits[c];
    }
    retval = PyUnicode_FromStringAndSize(hex_digest, digest_size * 2);
    PyMem_Free(hex_digest);
    return retval;
}

PyDoc_STRVAR(EVP_update__doc__,
"Update this hash object's state with the provided string.");

static PyObject *
EVP_update(EVPobject *self, PyObject *args)
{
    PyObject *obj;
    Py_buffer view;

    if (!PyArg_ParseTuple(args, "O:update", &obj))
        return NULL;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &view);

#ifdef WITH_THREAD
    if (self->lock == NULL && view.len >= HASHLIB_GIL_MINSIZE) {
        self->lock = PyThread_allocate_lock();
        /* fail? lock = NULL and we fail over to non-threaded code. */
    }

    if (self->lock != NULL) {
        Py_BEGIN_ALLOW_THREADS
        PyThread_acquire_lock(self->lock, 1);
        EVP_hash(self, view.buf, view.len);
        PyThread_release_lock(self->lock);
        Py_END_ALLOW_THREADS
    } else {
        EVP_hash(self, view.buf, view.len);
    }
#else
    EVP_hash(self, view.buf, view.len);
#endif

    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

static PyMethodDef EVP_methods[] = {
    {"update",    (PyCFunction)EVP_update,    METH_VARARGS, EVP_update__doc__},
    {"digest",    (PyCFunction)EVP_digest,    METH_NOARGS,  EVP_digest__doc__},
    {"hexdigest", (PyCFunction)EVP_hexdigest, METH_NOARGS,  EVP_hexdigest__doc__},
    {"copy",      (PyCFunction)EVP_copy,      METH_NOARGS,  EVP_copy__doc__},
    {NULL, NULL}  /* sentinel */
};

static PyObject *
EVP_get_block_size(EVPobject *self, void *closure)
{
    long block_size;
    block_size = EVP_MD_CTX_block_size(&self->ctx);
    return PyLong_FromLong(block_size);
}

static PyObject *
EVP_get_digest_size(EVPobject *self, void *closure)
{
    long size;
    size = EVP_MD_CTX_size(&self->ctx);
    return PyLong_FromLong(size);
}

static PyMemberDef EVP_members[] = {
    {"name", T_OBJECT, offsetof(EVPobject, name), READONLY, PyDoc_STR("algorithm name.")},
    {NULL}  /* Sentinel */
};

static PyGetSetDef EVP_getseters[] = {
    {"digest_size",
     (getter)EVP_get_digest_size, NULL,
     NULL,
     NULL},
    {"block_size",
     (getter)EVP_get_block_size, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};


static PyObject *
EVP_repr(EVPobject *self)
{
    return PyUnicode_FromFormat("<%U HASH object @ %p>", self->name, self);
}

#if HASH_OBJ_CONSTRUCTOR
static int
EVP_tp_init(EVPobject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", "string", NULL};
    PyObject *name_obj = NULL;
    PyObject *data_obj = NULL;
    Py_buffer view;
    char *nameStr;
    const EVP_MD *digest;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O:HASH", kwlist,
                                     &name_obj, &data_obj)) {
        return -1;
    }

    if (data_obj)
        GET_BUFFER_VIEW_OR_ERROUT(data_obj, &view);

    if (!PyArg_Parse(name_obj, "s", &nameStr)) {
        PyErr_SetString(PyExc_TypeError, "name must be a string");
        if (data_obj)
            PyBuffer_Release(&view);
        return -1;
    }

    digest = EVP_get_digestbyname(nameStr);
    if (!digest) {
        PyErr_SetString(PyExc_ValueError, "unknown hash function");
        if (data_obj)
            PyBuffer_Release(&view);
        return -1;
    }
    EVP_DigestInit(&self->ctx, digest);

    self->name = name_obj;
    Py_INCREF(self->name);

    if (data_obj) {
        if (view.len >= HASHLIB_GIL_MINSIZE) {
            Py_BEGIN_ALLOW_THREADS
            EVP_hash(self, view.buf, view.len);
            Py_END_ALLOW_THREADS
        } else {
            EVP_hash(self, view.buf, view.len);
        }
        PyBuffer_Release(&view);
    }

    return 0;
}
#endif


PyDoc_STRVAR(hashtype_doc,
"A hash represents the object used to calculate a checksum of a\n\
string of information.\n\
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
name -- the hash algorithm being used by this object\n\
digest_size -- number of bytes in this hashes output\n");

static PyTypeObject EVPtype = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hashlib.HASH",    /*tp_name*/
    sizeof(EVPobject),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    /* methods */
    (destructor)EVP_dealloc, /*tp_dealloc*/
    0,                  /*tp_print*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_reserved*/
    (reprfunc)EVP_repr, /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash*/
    0,                  /*tp_call*/
    0,                  /*tp_str*/
    0,                  /*tp_getattro*/
    0,                  /*tp_setattro*/
    0,                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    hashtype_doc,       /*tp_doc*/
    0,                  /*tp_traverse*/
    0,                  /*tp_clear*/
    0,                  /*tp_richcompare*/
    0,                  /*tp_weaklistoffset*/
    0,                  /*tp_iter*/
    0,                  /*tp_iternext*/
    EVP_methods,        /* tp_methods */
    EVP_members,        /* tp_members */
    EVP_getseters,      /* tp_getset */
#if 1
    0,                  /* tp_base */
    0,                  /* tp_dict */
    0,                  /* tp_descr_get */
    0,                  /* tp_descr_set */
    0,                  /* tp_dictoffset */
#endif
#if HASH_OBJ_CONSTRUCTOR
    (initproc)EVP_tp_init, /* tp_init */
#endif
};

static PyObject *
EVPnew(PyObject *name_obj,
       const EVP_MD *digest, const EVP_MD_CTX *initial_ctx,
       const unsigned char *cp, Py_ssize_t len)
{
    EVPobject *self;

    if (!digest && !initial_ctx) {
        PyErr_SetString(PyExc_ValueError, "unsupported hash type");
        return NULL;
    }

    if ((self = newEVPobject(name_obj)) == NULL)
        return NULL;

    if (initial_ctx) {
        EVP_MD_CTX_copy(&self->ctx, initial_ctx);
    } else {
        EVP_DigestInit(&self->ctx, digest);
    }

    if (cp && len) {
        if (len >= HASHLIB_GIL_MINSIZE) {
            Py_BEGIN_ALLOW_THREADS
            EVP_hash(self, cp, len);
            Py_END_ALLOW_THREADS
        } else {
            EVP_hash(self, cp, len);
        }
    }

    return (PyObject *)self;
}


/* The module-level function: new() */

PyDoc_STRVAR(EVP_new__doc__,
"Return a new hash object using the named algorithm.\n\
An optional string argument may be provided and will be\n\
automatically hashed.\n\
\n\
The MD5 and SHA1 algorithms are always supported.\n");

static PyObject *
EVP_new(PyObject *self, PyObject *args, PyObject *kwdict)
{
    static char *kwlist[] = {"name", "string", NULL};
    PyObject *name_obj = NULL;
    PyObject *data_obj = NULL;
    Py_buffer view = { 0 };
    PyObject *ret_obj;
    char *name;
    const EVP_MD *digest;

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "O|O:new", kwlist,
                                     &name_obj, &data_obj)) {
        return NULL;
    }

    if (!PyArg_Parse(name_obj, "s", &name)) {
        PyErr_SetString(PyExc_TypeError, "name must be a string");
        return NULL;
    }

    if (data_obj)
        GET_BUFFER_VIEW_OR_ERROUT(data_obj, &view);

    digest = EVP_get_digestbyname(name);

    ret_obj = EVPnew(name_obj, digest, NULL, (unsigned char*)view.buf, view.len);

    if (data_obj)
        PyBuffer_Release(&view);
    return ret_obj;
}



#if (OPENSSL_VERSION_NUMBER >= 0x10000000 && !defined(OPENSSL_NO_HMAC) \
     && !defined(OPENSSL_NO_SHA))

#define PY_PBKDF2_HMAC 1

/* Improved implementation of PKCS5_PBKDF2_HMAC()
 *
 * PKCS5_PBKDF2_HMAC_fast() hashes the password exactly one time instead of
 * `iter` times. Today (2013) the iteration count is typically 100,000 or
 * more. The improved algorithm is not subject to a Denial-of-Service
 * vulnerability with overly large passwords.
 *
 * Also OpenSSL < 1.0 don't provide PKCS5_PBKDF2_HMAC(), only
 * PKCS5_PBKDF2_SHA1.
 */
static int
PKCS5_PBKDF2_HMAC_fast(const char *pass, int passlen,
                       const unsigned char *salt, int saltlen,
                       int iter, const EVP_MD *digest,
                       int keylen, unsigned char *out)
{
    unsigned char digtmp[EVP_MAX_MD_SIZE], *p, itmp[4];
    int cplen, j, k, tkeylen, mdlen;
    unsigned long i = 1;
    HMAC_CTX hctx_tpl, hctx;

    mdlen = EVP_MD_size(digest);
    if (mdlen < 0)
        return 0;

    HMAC_CTX_init(&hctx_tpl);
    HMAC_CTX_init(&hctx);
    p = out;
    tkeylen = keylen;
    if (!HMAC_Init_ex(&hctx_tpl, pass, passlen, digest, NULL)) {
        HMAC_CTX_cleanup(&hctx_tpl);
        return 0;
    }
    while(tkeylen) {
        if(tkeylen > mdlen)
            cplen = mdlen;
        else
            cplen = tkeylen;
        /* We are unlikely to ever use more than 256 blocks (5120 bits!)
         * but just in case...
         */
        itmp[0] = (unsigned char)((i >> 24) & 0xff);
        itmp[1] = (unsigned char)((i >> 16) & 0xff);
        itmp[2] = (unsigned char)((i >> 8) & 0xff);
        itmp[3] = (unsigned char)(i & 0xff);
        if (!HMAC_CTX_copy(&hctx, &hctx_tpl)) {
            HMAC_CTX_cleanup(&hctx_tpl);
            return 0;
        }
        if (!HMAC_Update(&hctx, salt, saltlen)
                || !HMAC_Update(&hctx, itmp, 4)
                || !HMAC_Final(&hctx, digtmp, NULL)) {
            HMAC_CTX_cleanup(&hctx_tpl);
            HMAC_CTX_cleanup(&hctx);
            return 0;
        }
        HMAC_CTX_cleanup(&hctx);
        memcpy(p, digtmp, cplen);
        for (j = 1; j < iter; j++) {
            if (!HMAC_CTX_copy(&hctx, &hctx_tpl)) {
                HMAC_CTX_cleanup(&hctx_tpl);
                return 0;
            }
            if (!HMAC_Update(&hctx, digtmp, mdlen)
                    || !HMAC_Final(&hctx, digtmp, NULL)) {
                HMAC_CTX_cleanup(&hctx_tpl);
                HMAC_CTX_cleanup(&hctx);
                return 0;
            }
            HMAC_CTX_cleanup(&hctx);
            for (k = 0; k < cplen; k++) {
                p[k] ^= digtmp[k];
            }
        }
        tkeylen-= cplen;
        i++;
        p+= cplen;
    }
    HMAC_CTX_cleanup(&hctx_tpl);
    return 1;
}

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

PyDoc_STRVAR(pbkdf2_hmac__doc__,
"pbkdf2_hmac(hash_name, password, salt, iterations, dklen=None) -> key\n\
\n\
Password based key derivation function 2 (PKCS #5 v2.0) with HMAC as\n\
pseudorandom function.");

static PyObject *
pbkdf2_hmac(PyObject *self, PyObject *args, PyObject *kwdict)
{
    static char *kwlist[] = {"hash_name", "password", "salt", "iterations",
                             "dklen", NULL};
    PyObject *key_obj = NULL, *dklen_obj = Py_None;
    char *name, *key;
    Py_buffer password, salt;
    long iterations, dklen;
    int retval;
    const EVP_MD *digest;

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "sy*y*l|O:pbkdf2_hmac",
                                     kwlist, &name, &password, &salt,
                                     &iterations, &dklen_obj)) {
        return NULL;
    }

    digest = EVP_get_digestbyname(name);
    if (digest == NULL) {
        PyErr_SetString(PyExc_ValueError, "unsupported hash type");
        goto end;
    }

    if (password.len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "password is too long.");
        goto end;
    }

    if (salt.len > INT_MAX) {
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
    retval = PKCS5_PBKDF2_HMAC_fast((char*)password.buf, (int)password.len,
                                    (unsigned char *)salt.buf, (int)salt.len,
                                    iterations, digest, dklen,
                                    (unsigned char *)key);
    Py_END_ALLOW_THREADS

    if (!retval) {
        Py_CLEAR(key_obj);
        _setException(PyExc_ValueError);
        goto end;
    }

  end:
    PyBuffer_Release(&password);
    PyBuffer_Release(&salt);
    return key_obj;
}

#endif

/* State for our callback function so that it can accumulate a result. */
typedef struct _internal_name_mapper_state {
    PyObject *set;
    int error;
} _InternalNameMapperState;


/* A callback function to pass to OpenSSL's OBJ_NAME_do_all(...) */
static void
_openssl_hash_name_mapper(const OBJ_NAME *openssl_obj_name, void *arg)
{
    _InternalNameMapperState *state = (_InternalNameMapperState *)arg;
    PyObject *py_name;

    assert(state != NULL);
    if (openssl_obj_name == NULL)
        return;
    /* Ignore aliased names, they pollute the list and OpenSSL appears to
     * have a its own definition of alias as the resulting list still
     * contains duplicate and alternate names for several algorithms.     */
    if (openssl_obj_name->alias)
        return;

    py_name = PyUnicode_FromString(openssl_obj_name->name);
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
static PyObject*
generate_hash_name_list(void)
{
    _InternalNameMapperState state;
    state.set = PyFrozenSet_New(NULL);
    if (state.set == NULL)
        return NULL;
    state.error = 0;

    OBJ_NAME_do_all(OBJ_NAME_TYPE_MD_METH, &_openssl_hash_name_mapper, &state);

    if (state.error) {
        Py_DECREF(state.set);
        return NULL;
    }
    return state.set;
}


/*
 *  This macro generates constructor function definitions for specific
 *  hash algorithms.  These constructors are much faster than calling
 *  the generic one passing it a python string and are noticably
 *  faster than calling a python new() wrapper.  Thats important for
 *  code that wants to make hashes of a bunch of small strings.
 */
#define GEN_CONSTRUCTOR(NAME)  \
    static PyObject * \
    EVP_new_ ## NAME (PyObject *self, PyObject *args) \
    { \
        PyObject *data_obj = NULL; \
        Py_buffer view = { 0 }; \
        PyObject *ret_obj; \
     \
        if (!PyArg_ParseTuple(args, "|O:" #NAME , &data_obj)) { \
            return NULL; \
        } \
     \
        if (data_obj) \
            GET_BUFFER_VIEW_OR_ERROUT(data_obj, &view); \
     \
        ret_obj = EVPnew( \
                    CONST_ ## NAME ## _name_obj, \
                    NULL, \
                    CONST_new_ ## NAME ## _ctx_p, \
                    (unsigned char*)view.buf, \
                    view.len); \
     \
        if (data_obj) \
            PyBuffer_Release(&view); \
        return ret_obj; \
    }

/* a PyMethodDef structure for the constructor */
#define CONSTRUCTOR_METH_DEF(NAME)  \
    {"openssl_" #NAME, (PyCFunction)EVP_new_ ## NAME, METH_VARARGS, \
        PyDoc_STR("Returns a " #NAME \
                  " hash object; optionally initialized with a string") \
    }

/* used in the init function to setup a constructor: initialize OpenSSL
   constructor constants if they haven't been initialized already.  */
#define INIT_CONSTRUCTOR_CONSTANTS(NAME)  do { \
    if (CONST_ ## NAME ## _name_obj == NULL) { \
        CONST_ ## NAME ## _name_obj = PyUnicode_FromString(#NAME); \
        if (EVP_get_digestbyname(#NAME)) { \
            CONST_new_ ## NAME ## _ctx_p = &CONST_new_ ## NAME ## _ctx; \
            EVP_DigestInit(CONST_new_ ## NAME ## _ctx_p, EVP_get_digestbyname(#NAME)); \
        } \
    } \
} while (0);

GEN_CONSTRUCTOR(md5)
GEN_CONSTRUCTOR(sha1)
#ifdef _OPENSSL_SUPPORTS_SHA2
GEN_CONSTRUCTOR(sha224)
GEN_CONSTRUCTOR(sha256)
GEN_CONSTRUCTOR(sha384)
GEN_CONSTRUCTOR(sha512)
#endif

/* List of functions exported by this module */

static struct PyMethodDef EVP_functions[] = {
    {"new", (PyCFunction)EVP_new, METH_VARARGS|METH_KEYWORDS, EVP_new__doc__},
#ifdef PY_PBKDF2_HMAC
    {"pbkdf2_hmac", (PyCFunction)pbkdf2_hmac, METH_VARARGS|METH_KEYWORDS,
     pbkdf2_hmac__doc__},
#endif
    CONSTRUCTOR_METH_DEF(md5),
    CONSTRUCTOR_METH_DEF(sha1),
#ifdef _OPENSSL_SUPPORTS_SHA2
    CONSTRUCTOR_METH_DEF(sha224),
    CONSTRUCTOR_METH_DEF(sha256),
    CONSTRUCTOR_METH_DEF(sha384),
    CONSTRUCTOR_METH_DEF(sha512),
#endif
    {NULL,      NULL}            /* Sentinel */
};


/* Initialize this module. */


static struct PyModuleDef _hashlibmodule = {
    PyModuleDef_HEAD_INIT,
    "_hashlib",
    NULL,
    -1,
    EVP_functions,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__hashlib(void)
{
    PyObject *m, *openssl_md_meth_names;

    OpenSSL_add_all_digests();
    ERR_load_crypto_strings();

    /* TODO build EVP_functions openssl_* entries dynamically based
     * on what hashes are supported rather than listing many
     * but having some be unsupported.  Only init appropriate
     * constants. */

    Py_TYPE(&EVPtype) = &PyType_Type;
    if (PyType_Ready(&EVPtype) < 0)
        return NULL;

    m = PyModule_Create(&_hashlibmodule);
    if (m == NULL)
        return NULL;

    openssl_md_meth_names = generate_hash_name_list();
    if (openssl_md_meth_names == NULL) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddObject(m, "openssl_md_meth_names", openssl_md_meth_names)) {
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF((PyObject *)&EVPtype);
    PyModule_AddObject(m, "HASH", (PyObject *)&EVPtype);

    /* these constants are used by the convenience constructors */
    INIT_CONSTRUCTOR_CONSTANTS(md5);
    INIT_CONSTRUCTOR_CONSTANTS(sha1);
#ifdef _OPENSSL_SUPPORTS_SHA2
    INIT_CONSTRUCTOR_CONSTANTS(sha224);
    INIT_CONSTRUCTOR_CONSTANTS(sha256);
    INIT_CONSTRUCTOR_CONSTANTS(sha384);
    INIT_CONSTRUCTOR_CONSTANTS(sha512);
#endif
    return m;
}
