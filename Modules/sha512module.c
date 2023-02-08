/* SHA512 module */

/* This module provides an interface to NIST's SHA-512 and SHA-384 Algorithms */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)
   Trevor Perrin (trevp@trevp.net)
   Jonathan Protzenko (jonathan@protzenko.fr)

   Copyright (C) 2005-2007   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

/* SHA objects */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_bitutils.h"      // _Py_bswap64()
#include "pycore_strhex.h"        // _Py_strhex()
#include "structmember.h"         // PyMemberDef
#include "hashlib.h"

/*[clinic input]
module _sha512
class SHA512Type "SHAobject *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=81a3ccde92bcfe8d]*/


/* The SHA block size and message digest sizes, in bytes */

#define SHA_BLOCKSIZE   128
#define SHA_DIGESTSIZE  64

/* The SHA2-384 and SHA2-512 implementations defer to the HACL* verified
 * library. */

#include "_hacl/Hacl_Streaming_SHA2.h"

typedef struct {
    PyObject_HEAD
    int digestsize;
    Hacl_Streaming_SHA2_state_sha2_512 *state;
} SHAobject;

#include "clinic/sha512module.c.h"


static void SHAcopy(SHAobject *src, SHAobject *dest)
{
    dest->digestsize = src->digestsize;
    dest->state = Hacl_Streaming_SHA2_copy_512(src->state);
}

typedef struct {
    PyTypeObject* sha384_type;
    PyTypeObject* sha512_type;
} SHA512State;

static inline SHA512State*
sha512_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (SHA512State *)state;
}

static SHAobject *
newSHA384object(SHA512State *st)
{
    SHAobject *sha = (SHAobject *)PyObject_GC_New(SHAobject, st->sha384_type);
    PyObject_GC_Track(sha);
    return sha;
}

static SHAobject *
newSHA512object(SHA512State *st)
{
    SHAobject *sha = (SHAobject *)PyObject_GC_New(SHAobject, st->sha512_type);
    PyObject_GC_Track(sha);
    return sha;
}

/* Internal methods for a hash object */
static int
SHA_traverse(PyObject *ptr, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(ptr));
    return 0;
}

static void
SHA512_dealloc(SHAobject *ptr)
{
    Hacl_Streaming_SHA2_free_512(ptr->state);
    PyTypeObject *tp = Py_TYPE(ptr);
    PyObject_GC_UnTrack(ptr);
    PyObject_GC_Del(ptr);
    Py_DECREF(tp);
}

/* HACL* takes a uint32_t for the length of its parameter, but Py_ssize_t can be
 * 64 bits. */
static void update_512(Hacl_Streaming_SHA2_state_sha2_512 *state, uint8_t *buf, Py_ssize_t len) {
  /* Note: we explicitly ignore the error code on the basis that it would take >
   * 1 billion years to overflow the maximum admissible length for this API
   * (namely, 2^64-1 bytes). */
  while (len > UINT32_MAX) {
    Hacl_Streaming_SHA2_update_512(state, buf, UINT32_MAX);
    len -= UINT32_MAX;
    buf += UINT32_MAX;
  }
  /* Cast to uint32_t is safe: upon exiting the loop, len <= UINT32_MAX, and
   * therefore fits in a uint32_t */
  Hacl_Streaming_SHA2_update_512(state, buf, (uint32_t) len);
}


/* External methods for a hash object */

/*[clinic input]
SHA512Type.copy

    cls: defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
SHA512Type_copy_impl(SHAobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=85ea5b47837a08e6 input=f673a18f66527c90]*/
{
    SHAobject *newobj;
    SHA512State *st = PyType_GetModuleState(cls);

    if (Py_IS_TYPE((PyObject*)self, st->sha512_type)) {
        if ( (newobj = newSHA512object(st))==NULL) {
            return NULL;
        }
    }
    else {
        if ( (newobj = newSHA384object(st))==NULL) {
            return NULL;
        }
    }

    SHAcopy(self, newobj);
    return (PyObject *)newobj;
}

/*[clinic input]
SHA512Type.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
SHA512Type_digest_impl(SHAobject *self)
/*[clinic end generated code: output=1080bbeeef7dde1b input=f6470dd359071f4b]*/
{
    uint8_t digest[SHA_DIGESTSIZE];
    // HACL performs copies under the hood so that self->state remains valid
    // after this call.
    Hacl_Streaming_SHA2_finish_512(self->state, digest);
    return PyBytes_FromStringAndSize((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA512Type.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
SHA512Type_hexdigest_impl(SHAobject *self)
/*[clinic end generated code: output=7373305b8601e18b input=498b877b25cbe0a2]*/
{
    uint8_t digest[SHA_DIGESTSIZE];
    Hacl_Streaming_SHA2_finish_512(self->state, digest);
    return _Py_strhex((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA512Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
SHA512Type_update(SHAobject *self, PyObject *obj)
/*[clinic end generated code: output=1cf333e73995a79e input=ded2b46656566283]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);

    update_512(self->state, buf.buf, buf.len);

    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static PyMethodDef SHA_methods[] = {
    SHA512TYPE_COPY_METHODDEF
    SHA512TYPE_DIGEST_METHODDEF
    SHA512TYPE_HEXDIGEST_METHODDEF
    SHA512TYPE_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

static PyObject *
SHA512_get_block_size(PyObject *self, void *closure)
{
    return PyLong_FromLong(SHA_BLOCKSIZE);
}

static PyObject *
SHA512_get_name(PyObject *self, void *closure)
{
    if (((SHAobject *)self)->digestsize == 64)
        return PyUnicode_FromStringAndSize("sha512", 6);
    else
        return PyUnicode_FromStringAndSize("sha384", 6);
}

static PyGetSetDef SHA_getseters[] = {
    {"block_size",
     (getter)SHA512_get_block_size, NULL,
     NULL,
     NULL},
    {"name",
     (getter)SHA512_get_name, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};

static PyMemberDef SHA_members[] = {
    {"digest_size", T_INT, offsetof(SHAobject, digestsize), READONLY, NULL},
    {NULL}  /* Sentinel */
};

static PyType_Slot sha512_sha384_type_slots[] = {
    {Py_tp_dealloc, SHA512_dealloc},
    {Py_tp_methods, SHA_methods},
    {Py_tp_members, SHA_members},
    {Py_tp_getset, SHA_getseters},
    {Py_tp_traverse, SHA_traverse},
    {0,0}
};

static PyType_Spec sha512_sha384_type_spec = {
    .name = "_sha512.sha384",
    .basicsize =  sizeof(SHAobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha512_sha384_type_slots
};

// Using PyType_GetModuleState() on this type is safe since
// it cannot be subclassed: it does not have the Py_TPFLAGS_BASETYPE flag.
static PyType_Spec sha512_sha512_type_spec = {
    .name = "_sha512.sha512",
    .basicsize =  sizeof(SHAobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha512_sha384_type_slots
};

/* The single module-level function: new() */

/*[clinic input]
_sha512.sha512

    string: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True

Return a new SHA-512 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha512_sha512_impl(PyObject *module, PyObject *string, int usedforsecurity)
/*[clinic end generated code: output=a8d9e5f9e6a0831c input=23b4daebc2ebb9c9]*/
{
    SHAobject *new;
    Py_buffer buf;

    SHA512State *st = sha512_get_state(module);

    if (string)
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);

    if ((new = newSHA512object(st)) == NULL) {
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }

    new->state = Hacl_Streaming_SHA2_create_in_512();
    new->digestsize = 64;

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }
    if (string) {
        update_512(new->state, buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}

/*[clinic input]
_sha512.sha384

    string: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True

Return a new SHA-384 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha512_sha384_impl(PyObject *module, PyObject *string, int usedforsecurity)
/*[clinic end generated code: output=da7d594a08027ac3 input=59ef72f039a6b431]*/
{
    SHAobject *new;
    Py_buffer buf;

    SHA512State *st = sha512_get_state(module);

    if (string)
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);

    if ((new = newSHA384object(st)) == NULL) {
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }

    new->state = Hacl_Streaming_SHA2_create_in_384();
    new->digestsize = 48;

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }
    if (string) {
        update_512(new->state, buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef SHA_functions[] = {
    _SHA512_SHA512_METHODDEF
    _SHA512_SHA384_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};

static int
_sha512_traverse(PyObject *module, visitproc visit, void *arg)
{
    SHA512State *state = sha512_get_state(module);
    Py_VISIT(state->sha384_type);
    Py_VISIT(state->sha512_type);
    return 0;
}

static int
_sha512_clear(PyObject *module)
{
    SHA512State *state = sha512_get_state(module);
    Py_CLEAR(state->sha384_type);
    Py_CLEAR(state->sha512_type);
    return 0;
}

static void
_sha512_free(void *module)
{
    _sha512_clear((PyObject *)module);
}


/* Initialize this module. */
static int
_sha512_exec(PyObject *m)
{
    SHA512State* st = sha512_get_state(m);

    st->sha384_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &sha512_sha384_type_spec, NULL);

    st->sha512_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &sha512_sha512_type_spec, NULL);

    if (st->sha384_type == NULL || st->sha512_type == NULL) {
        return -1;
    }

    Py_INCREF(st->sha384_type);
    if (PyModule_AddObject(m, "SHA384Type", (PyObject *)st->sha384_type) < 0) {
        Py_DECREF(st->sha384_type);
        return -1;
    }

    Py_INCREF(st->sha512_type);
    if (PyModule_AddObject(m, "SHA384Type", (PyObject *)st->sha512_type) < 0) {
        Py_DECREF(st->sha512_type);
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot _sha512_slots[] = {
    {Py_mod_exec, _sha512_exec},
    {0, NULL}
};

static struct PyModuleDef _sha512module = {
        PyModuleDef_HEAD_INIT,
        .m_name = "_sha512",
        .m_size = sizeof(SHA512State),
        .m_methods = SHA_functions,
        .m_slots = _sha512_slots,
        .m_traverse = _sha512_traverse,
        .m_clear = _sha512_clear,
        .m_free = _sha512_free
};

PyMODINIT_FUNC
PyInit__sha512(void)
{
    return PyModuleDef_Init(&_sha512module);
}
