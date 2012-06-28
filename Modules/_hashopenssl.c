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

#ifdef WITH_THREAD
#include "pythread.h"
    #define ENTER_HASHLIB(obj) \
        if ((obj)->lock) { \
            if (!PyThread_acquire_lock((obj)->lock, 0)) { \
                Py_BEGIN_ALLOW_THREADS \
                PyThread_acquire_lock((obj)->lock, 1); \
                Py_END_ALLOW_THREADS \
            } \
        }
    #define LEAVE_HASHLIB(obj) \
        if ((obj)->lock) { \
            PyThread_release_lock((obj)->lock); \
        }
#else
    #define ENTER_HASHLIB(obj)
    #define LEAVE_HASHLIB(obj)
#endif

/* EVP is the preferred interface to hashing in OpenSSL */
#include <openssl/evp.h>

#define MUNCH_SIZE INT_MAX

/* TODO(gps): We should probably make this a module or EVPobject attribute
 * to allow the user to optimize based on the platform they're using. */
#define HASHLIB_GIL_MINSIZE 2048

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
    EVP_MD_CTX          ctx;    /* OpenSSL message digest context */
#ifdef WITH_THREAD
    PyThread_type_lock  lock;   /* OpenSSL context lock */
#endif
} EVPobject;


static PyTypeObject EVPtype;


#define DEFINE_CONSTS_FOR_NEW(Name)  \
    static PyObject *CONST_ ## Name ## _name_obj; \
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
    while (0 < len)
    {
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

    retval = PyString_FromStringAndSize((const char *)digest, digest_size);
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

    /* Create a new string */
    /* NOTE: not thread safe! modifying an already created string object */
    /* (not a problem because we hold the GIL by default) */
    retval = PyString_FromStringAndSize(NULL, digest_size * 2);
    if (!retval)
            return NULL;
    hex_digest = PyString_AsString(retval);
    if (!hex_digest) {
            Py_DECREF(retval);
            return NULL;
    }

    /* Make hex version of the digest */
    for(i=j=0; i<digest_size; i++) {
        char c;
        c = (digest[i] >> 4) & 0xf;
        c = (c>9) ? c+'a'-10 : c + '0';
        hex_digest[j++] = c;
        c = (digest[i] & 0xf);
        c = (c>9) ? c+'a'-10 : c + '0';
        hex_digest[j++] = c;
    }
    return retval;
}

PyDoc_STRVAR(EVP_update__doc__,
"Update this hash object's state with the provided string.");

static PyObject *
EVP_update(EVPobject *self, PyObject *args)
{
    Py_buffer view;

    if (!PyArg_ParseTuple(args, "s*:update", &view))
        return NULL;

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
    }
    else
#endif
    {
        EVP_hash(self, view.buf, view.len);
    }

    PyBuffer_Release(&view);

    Py_RETURN_NONE;
}

static PyMethodDef EVP_methods[] = {
    {"update",    (PyCFunction)EVP_update,    METH_VARARGS, EVP_update__doc__},
    {"digest",    (PyCFunction)EVP_digest,    METH_NOARGS,  EVP_digest__doc__},
    {"hexdigest", (PyCFunction)EVP_hexdigest, METH_NOARGS,  EVP_hexdigest__doc__},
    {"copy",      (PyCFunction)EVP_copy,      METH_NOARGS,  EVP_copy__doc__},
    {NULL,        NULL}         /* sentinel */
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
    /* the old md5 and sha modules support 'digest_size' as in PEP 247.
     * the old sha module also supported 'digestsize'.  ugh. */
    {"digestsize",
     (getter)EVP_get_digest_size, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};


static PyObject *
EVP_repr(PyObject *self)
{
    char buf[100];
    PyOS_snprintf(buf, sizeof(buf), "<%s HASH object @ %p>",
            PyString_AsString(((EVPobject *)self)->name), self);
    return PyString_FromString(buf);
}

#if HASH_OBJ_CONSTRUCTOR
static int
EVP_tp_init(EVPobject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", "string", NULL};
    PyObject *name_obj = NULL;
    Py_buffer view = { 0 };
    char *nameStr;
    const EVP_MD *digest;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|s*:HASH", kwlist,
                                     &name_obj, &view)) {
        return -1;
    }

    if (!PyArg_Parse(name_obj, "s", &nameStr)) {
        PyErr_SetString(PyExc_TypeError, "name must be a string");
        PyBuffer_Release(&view);
        return -1;
    }

    digest = EVP_get_digestbyname(nameStr);
    if (!digest) {
        PyErr_SetString(PyExc_ValueError, "unknown hash function");
        PyBuffer_Release(&view);
        return -1;
    }
    EVP_DigestInit(&self->ctx, digest);

    self->name = name_obj;
    Py_INCREF(self->name);

    if (view.obj) {
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
    (destructor)EVP_dealloc,    /*tp_dealloc*/
    0,                  /*tp_print*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_compare*/
    EVP_repr,           /*tp_repr*/
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
    Py_buffer view = { 0 };
    PyObject *ret_obj;
    char *name;
    const EVP_MD *digest;

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "O|s*:new", kwlist,
                                     &name_obj, &view)) {
        return NULL;
    }

    if (!PyArg_Parse(name_obj, "s", &name)) {
        PyBuffer_Release(&view);
        PyErr_SetString(PyExc_TypeError, "name must be a string");
        return NULL;
    }

    digest = EVP_get_digestbyname(name);

    ret_obj = EVPnew(name_obj, digest, NULL, (unsigned char*)view.buf,
                     view.len);
    PyBuffer_Release(&view);

    return ret_obj;
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
        Py_buffer view = { 0 }; \
        PyObject *ret_obj; \
     \
        if (!PyArg_ParseTuple(args, "|s*:" #NAME , &view)) { \
            return NULL; \
        } \
     \
        ret_obj = EVPnew( \
                    CONST_ ## NAME ## _name_obj, \
                    NULL, \
                    CONST_new_ ## NAME ## _ctx_p, \
                    (unsigned char*)view.buf, view.len); \
        PyBuffer_Release(&view); \
        return ret_obj; \
    }

/* a PyMethodDef structure for the constructor */
#define CONSTRUCTOR_METH_DEF(NAME)  \
    {"openssl_" #NAME, (PyCFunction)EVP_new_ ## NAME, METH_VARARGS, \
        PyDoc_STR("Returns a " #NAME \
                  " hash object; optionally initialized with a string") \
    }

/* used in the init function to setup a constructor */
#define INIT_CONSTRUCTOR_CONSTANTS(NAME)  do { \
    CONST_ ## NAME ## _name_obj = PyString_FromString(#NAME); \
    if (EVP_get_digestbyname(#NAME)) { \
        CONST_new_ ## NAME ## _ctx_p = &CONST_new_ ## NAME ## _ctx; \
        EVP_DigestInit(CONST_new_ ## NAME ## _ctx_p, EVP_get_digestbyname(#NAME)); \
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

PyMODINIT_FUNC
init_hashlib(void)
{
    PyObject *m;

    OpenSSL_add_all_digests();

    /* TODO build EVP_functions openssl_* entries dynamically based
     * on what hashes are supported rather than listing many
     * but having some be unsupported.  Only init appropriate
     * constants. */

    Py_TYPE(&EVPtype) = &PyType_Type;
    if (PyType_Ready(&EVPtype) < 0)
        return;

    m = Py_InitModule("_hashlib", EVP_functions);
    if (m == NULL)
        return;

#if HASH_OBJ_CONSTRUCTOR
    Py_INCREF(&EVPtype);
    PyModule_AddObject(m, "HASH", (PyObject *)&EVPtype);
#endif

    /* these constants are used by the convenience constructors */
    INIT_CONSTRUCTOR_CONSTANTS(md5);
    INIT_CONSTRUCTOR_CONSTANTS(sha1);
#ifdef _OPENSSL_SUPPORTS_SHA2
    INIT_CONSTRUCTOR_CONSTANTS(sha224);
    INIT_CONSTRUCTOR_CONSTANTS(sha256);
    INIT_CONSTRUCTOR_CONSTANTS(sha384);
    INIT_CONSTRUCTOR_CONSTANTS(sha512);
#endif
}
