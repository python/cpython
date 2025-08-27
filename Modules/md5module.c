/* MD5 module */

/* This module provides an interface to the MD5 algorithm */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)
   Trevor Perrin (trevp@trevp.net)
   Bénédikt Tran (10796600+picnixz@users.noreply.github.com)

   Copyright (C) 2005-2007   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

/* MD5 objects */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_strhex.h" // _Py_strhex()

#include "hashlib.h"

#include "_hacl/Hacl_Hash_MD5.h"

/* The MD5 block size and message digest sizes, in bytes */

#define MD5_BLOCKSIZE    64
#define MD5_DIGESTSIZE   16

// --- Module objects ---------------------------------------------------------

typedef struct {
    HASHLIB_OBJECT_HEAD
    Hacl_Hash_MD5_state_t *hash_state;
} MD5object;

#define _MD5object_CAST(op)     ((MD5object *)(op))

// --- Module state -----------------------------------------------------------

typedef struct {
    PyTypeObject* md5_type;
} MD5State;

static inline MD5State*
md5_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (MD5State *)state;
}

// --- Module clinic configuration --------------------------------------------

/*[clinic input]
module _md5
class MD5Type "MD5object *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6e5261719957a912]*/

#include "clinic/md5module.c.h"

// --- MD5 object interface ---------------------------------------------------

static MD5object *
newMD5object(MD5State * st)
{
    MD5object *md5 = PyObject_GC_New(MD5object, st->md5_type);
    if (!md5) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(md5);

    PyObject_GC_Track(md5);
    return md5;
}

/* Internal methods for a hash object */
static int
MD5_traverse(PyObject *ptr, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(ptr));
    return 0;
}

static void
MD5_dealloc(PyObject *op)
{
    MD5object *ptr = _MD5object_CAST(op);
    Hacl_Hash_MD5_free(ptr->hash_state);
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(ptr);
    PyObject_GC_Del(ptr);
    Py_DECREF(tp);
}


/* External methods for a hash object */

/*[clinic input]
MD5Type.copy

    cls: defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
MD5Type_copy_impl(MD5object *self, PyTypeObject *cls)
/*[clinic end generated code: output=bf055e08244bf5ee input=d89087dcfb2a8620]*/
{
    MD5State *st = PyType_GetModuleState(cls);

    MD5object *newobj;
    if ((newobj = newMD5object(st)) == NULL) {
        return NULL;
    }

    HASHLIB_ACQUIRE_LOCK(self);
    newobj->hash_state = Hacl_Hash_MD5_copy(self->hash_state);
    HASHLIB_RELEASE_LOCK(self);
    if (newobj->hash_state == NULL) {
        Py_DECREF(newobj);
        return PyErr_NoMemory();
    }
    return (PyObject *)newobj;
}

/*[clinic input]
MD5Type.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
MD5Type_digest_impl(MD5object *self)
/*[clinic end generated code: output=eb691dc4190a07ec input=bc0c4397c2994be6]*/
{
    uint8_t digest[MD5_DIGESTSIZE];
    HASHLIB_ACQUIRE_LOCK(self);
    Hacl_Hash_MD5_digest(self->hash_state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return PyBytes_FromStringAndSize((const char *)digest, MD5_DIGESTSIZE);
}

/*[clinic input]
MD5Type.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
MD5Type_hexdigest_impl(MD5object *self)
/*[clinic end generated code: output=17badced1f3ac932 input=b60b19de644798dd]*/
{
    uint8_t digest[MD5_DIGESTSIZE];
    HASHLIB_ACQUIRE_LOCK(self);
    Hacl_Hash_MD5_digest(self->hash_state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return _Py_strhex((const char *)digest, MD5_DIGESTSIZE);
}

static void
update(Hacl_Hash_MD5_state_t *state, uint8_t *buf, Py_ssize_t len)
{
    /*
    * Note: we explicitly ignore the error code on the basis that it would
    * take more than 1 billion years to overflow the maximum admissible length
    * for MD5 (2^61 - 1).
    */
    assert(len >= 0);
#if PY_SSIZE_T_MAX > UINT32_MAX
    while (len > UINT32_MAX) {
        (void)Hacl_Hash_MD5_update(state, buf, UINT32_MAX);
        len -= UINT32_MAX;
        buf += UINT32_MAX;
    }
#endif
    /* cast to uint32_t is now safe */
    (void)Hacl_Hash_MD5_update(state, buf, (uint32_t)len);
}

/*[clinic input]
MD5Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
MD5Type_update_impl(MD5object *self, PyObject *obj)
/*[clinic end generated code: output=b0fed9a7ce7ad253 input=6e1efcd9ecf17032]*/
{
    Py_buffer buf;
    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, buf.len,
        update(self->hash_state, buf.buf, buf.len)
    );
    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static PyMethodDef MD5_methods[] = {
    MD5TYPE_COPY_METHODDEF
    MD5TYPE_DIGEST_METHODDEF
    MD5TYPE_HEXDIGEST_METHODDEF
    MD5TYPE_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

static PyObject *
MD5_get_block_size(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyLong_FromLong(MD5_BLOCKSIZE);
}

static PyObject *
MD5_get_name(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyUnicode_FromStringAndSize("md5", 3);
}

static PyObject *
md5_get_digest_size(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyLong_FromLong(MD5_DIGESTSIZE);
}

static PyGetSetDef MD5_getseters[] = {
    {"block_size", MD5_get_block_size, NULL, NULL, NULL},
    {"name", MD5_get_name, NULL, NULL, NULL},
    {"digest_size", md5_get_digest_size, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

static PyType_Slot md5_type_slots[] = {
    {Py_tp_dealloc, MD5_dealloc},
    {Py_tp_methods, MD5_methods},
    {Py_tp_getset, MD5_getseters},
    {Py_tp_traverse, MD5_traverse},
    {0,0}
};

static PyType_Spec md5_type_spec = {
    .name = "_md5.md5",
    .basicsize =  sizeof(MD5object),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = md5_type_slots
};

/* The single module-level function: new() */

/*[clinic input]
_md5.md5

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string as string_obj: object(c_default="NULL") = None

Return a new MD5 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_md5_md5_impl(PyObject *module, PyObject *data, int usedforsecurity,
              PyObject *string_obj)
/*[clinic end generated code: output=d45e187d3d16f3a8 input=7ea5c5366dbb44bf]*/
{
    PyObject *string;
    if (_Py_hashlib_data_argument(&string, data, string_obj) < 0) {
        return NULL;
    }

    MD5object *new;
    Py_buffer buf;

    if (string) {
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);
    }

    MD5State *st = md5_get_state(module);
    if ((new = newMD5object(st)) == NULL) {
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }

    new->hash_state = Hacl_Hash_MD5_malloc();
    if (new->hash_state == NULL) {
        Py_DECREF(new);
        if (string) {
            PyBuffer_Release(&buf);
        }
        return PyErr_NoMemory();
    }

    if (string) {
        /* Do not use self->mutex here as this is the constructor
         * where it is not yet possible to have concurrent access. */
        HASHLIB_EXTERNAL_INSTRUCTIONS_UNLOCKED(
            buf.len,
            update(new->hash_state, buf.buf, buf.len)
        );
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef MD5_functions[] = {
    _MD5_MD5_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};

static int
_md5_traverse(PyObject *module, visitproc visit, void *arg)
{
    MD5State *state = md5_get_state(module);
    Py_VISIT(state->md5_type);
    return 0;
}

static int
_md5_clear(PyObject *module)
{
    MD5State *state = md5_get_state(module);
    Py_CLEAR(state->md5_type);
    return 0;
}

static void
_md5_free(void *module)
{
    _md5_clear((PyObject *)module);
}

/* Initialize this module. */
static int
md5_exec(PyObject *m)
{
    MD5State *st = md5_get_state(m);

    st->md5_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &md5_type_spec, NULL);

    if (PyModule_AddObjectRef(m, "MD5Type", (PyObject *)st->md5_type) < 0) {
        return -1;
    }
    if (PyModule_AddIntConstant(m, "_GIL_MINSIZE", HASHLIB_GIL_MINSIZE) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot _md5_slots[] = {
    {Py_mod_exec, md5_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};


static struct PyModuleDef _md5module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_md5",
    .m_size = sizeof(MD5State),
    .m_methods = MD5_functions,
    .m_slots = _md5_slots,
    .m_traverse = _md5_traverse,
    .m_clear = _md5_clear,
    .m_free = _md5_free,
};

PyMODINIT_FUNC
PyInit__md5(void)
{
    return PyModuleDef_Init(&_md5module);
}
