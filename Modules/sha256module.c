/* SHA256 module */

/* This module provides an interface to NIST's SHA-256 and SHA-224 Algorithms */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)
   Trevor Perrin (trevp@trevp.net)

   Copyright (C) 2005-2007   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

/* SHA objects */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_bitutils.h"      // _Py_bswap32()
#include "pycore_strhex.h"        // _Py_strhex()
#include "structmember.h"         // PyMemberDef
#include "hashlib.h"

/*[clinic input]
module _sha256
class SHA256Type "SHAobject *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=71a39174d4f0a744]*/


/* The SHA block size and maximum message digest sizes, in bytes */

#define SHA_BLOCKSIZE    64
#define SHA_DIGESTSIZE   32

/* The SHA2-224 and SHA2-256 implementations defer to the HACL* verified
 * library. */

#include "_hacl/Hacl_Streaming_SHA2.h"

typedef struct {
  PyObject_HEAD
  // Even though one could conceivably perform run-type checks to tell apart a
  // sha224_type from a sha256_type (and thus deduce the digest size), we must
  // keep this field because it's exposed as a member field on the underlying
  // python object.
  // TODO: could we transform this into a getter and get rid of the redundant
  // field?
  int digestsize;
  Hacl_Streaming_SHA2_state_sha2_256 *state;
} SHAobject;

#include "clinic/sha256module.c.h"

/* We shall use run-time type information in the remainder of this module to
 * tell apart SHA2-224 and SHA2-256 */
typedef struct {
    PyTypeObject* sha224_type;
    PyTypeObject* sha256_type;
} _sha256_state;

static inline _sha256_state*
_sha256_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_sha256_state *)state;
}

static void SHAcopy(SHAobject *src, SHAobject *dest)
{
    dest->digestsize = src->digestsize;
    dest->state = Hacl_Streaming_SHA2_copy_256(src->state);
}

static SHAobject *
newSHA224object(_sha256_state *state)
{
    SHAobject *sha = (SHAobject *)PyObject_GC_New(SHAobject,
                                                  state->sha224_type);
    PyObject_GC_Track(sha);
    return sha;
}

static SHAobject *
newSHA256object(_sha256_state *state)
{
    SHAobject *sha = (SHAobject *)PyObject_GC_New(SHAobject,
                                                  state->sha256_type);
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
SHA_dealloc(SHAobject *ptr)
{
    Hacl_Streaming_SHA2_free_256(ptr->state);
    PyTypeObject *tp = Py_TYPE(ptr);
    PyObject_GC_UnTrack(ptr);
    PyObject_GC_Del(ptr);
    Py_DECREF(tp);
}

/* HACL* takes a uint32_t for the length of its parameter, but Py_ssize_t can be
 * 64 bits. */
static void update_256(Hacl_Streaming_SHA2_state_sha2_256 *state, uint8_t *buf, Py_ssize_t len) {
  /* Note: we explicitly ignore the error code on the basis that it would take >
   * 1 billion years to overflow the maximum admissible length for SHA2-256
   * (namely, 2^61-1 bytes). */
  while (len > UINT32_MAX) {
    Hacl_Streaming_SHA2_update_256(state, buf, UINT32_MAX);
    len -= UINT32_MAX;
    buf += UINT32_MAX;
  }
  /* Cast to uint32_t is safe: upon exiting the loop, len <= UINT32_MAX, and
   * therefore fits in a uint32_t */
  Hacl_Streaming_SHA2_update_256(state, buf, (uint32_t) len);
}


/* External methods for a hash object */

/*[clinic input]
SHA256Type.copy

    cls:defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
SHA256Type_copy_impl(SHAobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=9273f92c382be12f input=3137146fcb88e212]*/
{
    SHAobject *newobj;
    _sha256_state *state = PyType_GetModuleState(cls);
    if (Py_IS_TYPE(self, state->sha256_type)) {
        if ( (newobj = newSHA256object(state)) == NULL) {
            return NULL;
        }
    } else {
        if ( (newobj = newSHA224object(state))==NULL) {
            return NULL;
        }
    }

    SHAcopy(self, newobj);
    return (PyObject *)newobj;
}

/*[clinic input]
SHA256Type.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
SHA256Type_digest_impl(SHAobject *self)
/*[clinic end generated code: output=46616a5e909fbc3d input=f1f4cfea5cbde35c]*/
{
    uint8_t digest[SHA_DIGESTSIZE];
    // HACL performs copies under the hood so that self->state remains valid
    // after this call.
    Hacl_Streaming_SHA2_finish_256(self->state, digest);
    return PyBytes_FromStringAndSize((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA256Type.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
SHA256Type_hexdigest_impl(SHAobject *self)
/*[clinic end generated code: output=725f8a7041ae97f3 input=0cc4c714693010d1]*/
{
    uint8_t digest[SHA_DIGESTSIZE];
    Hacl_Streaming_SHA2_finish_256(self->state, digest);
    return _Py_strhex((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA256Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
SHA256Type_update(SHAobject *self, PyObject *obj)
/*[clinic end generated code: output=0967fb2860c66af7 input=b2d449d5b30f0f5a]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);

    update_256(self->state, buf.buf, buf.len);

    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static PyMethodDef SHA_methods[] = {
    SHA256TYPE_COPY_METHODDEF
    SHA256TYPE_DIGEST_METHODDEF
    SHA256TYPE_HEXDIGEST_METHODDEF
    SHA256TYPE_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

static PyObject *
SHA256_get_block_size(PyObject *self, void *closure)
{
    return PyLong_FromLong(SHA_BLOCKSIZE);
}

static PyObject *
SHA256_get_name(SHAobject *self, void *closure)
{
    if (self->digestsize == 28) {
        return PyUnicode_FromStringAndSize("sha224", 6);
    }
    return PyUnicode_FromStringAndSize("sha256", 6);
}

static PyGetSetDef SHA_getseters[] = {
    {"block_size",
     (getter)SHA256_get_block_size, NULL,
     NULL,
     NULL},
    {"name",
     (getter)SHA256_get_name, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};

static PyMemberDef SHA_members[] = {
    {"digest_size", T_INT, offsetof(SHAobject, digestsize), READONLY, NULL},
    {NULL}  /* Sentinel */
};

static PyType_Slot sha256_types_slots[] = {
    {Py_tp_dealloc, SHA_dealloc},
    {Py_tp_methods, SHA_methods},
    {Py_tp_members, SHA_members},
    {Py_tp_getset, SHA_getseters},
    {Py_tp_traverse, SHA_traverse},
    {0,0}
};

static PyType_Spec sha224_type_spec = {
    .name = "_sha256.sha224",
    .basicsize = sizeof(SHAobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha256_types_slots
};

static PyType_Spec sha256_type_spec = {
    .name = "_sha256.sha256",
    .basicsize = sizeof(SHAobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha256_types_slots
};

/* The single module-level function: new() */

/*[clinic input]
_sha256.sha256

    string: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True

Return a new SHA-256 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha256_sha256_impl(PyObject *module, PyObject *string, int usedforsecurity)
/*[clinic end generated code: output=a1de327e8e1185cf input=9be86301aeb14ea5]*/
{
    Py_buffer buf;

    if (string) {
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);
    }

    _sha256_state *state = PyModule_GetState(module);

    SHAobject *new;
    if ((new = newSHA256object(state)) == NULL) {
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }

    new->state = Hacl_Streaming_SHA2_create_in_256();
    new->digestsize = 32;

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }
    if (string) {
        update_256(new->state, buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}

/*[clinic input]
_sha256.sha224

    string: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True

Return a new SHA-224 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha256_sha224_impl(PyObject *module, PyObject *string, int usedforsecurity)
/*[clinic end generated code: output=08be6b36569bc69c input=9fcfb46e460860ac]*/
{
    Py_buffer buf;
    if (string) {
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);
    }

    _sha256_state *state = PyModule_GetState(module);
    SHAobject *new;
    if ((new = newSHA224object(state)) == NULL) {
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }

    new->state = Hacl_Streaming_SHA2_create_in_224();
    new->digestsize = 28;

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }
    if (string) {
        update_256(new->state, buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef SHA_functions[] = {
    _SHA256_SHA256_METHODDEF
    _SHA256_SHA224_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};

static int
_sha256_traverse(PyObject *module, visitproc visit, void *arg)
{
    _sha256_state *state = _sha256_get_state(module);
    Py_VISIT(state->sha224_type);
    Py_VISIT(state->sha256_type);
    return 0;
}

static int
_sha256_clear(PyObject *module)
{
    _sha256_state *state = _sha256_get_state(module);
    Py_CLEAR(state->sha224_type);
    Py_CLEAR(state->sha256_type);
    return 0;
}

static void
_sha256_free(void *module)
{
    _sha256_clear((PyObject *)module);
}

static int sha256_exec(PyObject *module)
{
    _sha256_state *state = _sha256_get_state(module);

    state->sha224_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &sha224_type_spec, NULL);

    if (state->sha224_type == NULL) {
        return -1;
    }

    state->sha256_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &sha256_type_spec, NULL);

    if (state->sha256_type == NULL) {
        return -1;
    }

    Py_INCREF((PyObject *)state->sha224_type);
    if (PyModule_AddObject(module, "SHA224Type", (PyObject *)state->sha224_type) < 0) {
        Py_DECREF((PyObject *)state->sha224_type);
        return -1;
    }
    Py_INCREF((PyObject *)state->sha256_type);
    if (PyModule_AddObject(module, "SHA256Type", (PyObject *)state->sha256_type) < 0) {
        Py_DECREF((PyObject *)state->sha256_type);
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot _sha256_slots[] = {
    {Py_mod_exec, sha256_exec},
    {0, NULL}
};

static struct PyModuleDef _sha256module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_sha256",
    .m_size = sizeof(_sha256_state),
    .m_methods = SHA_functions,
    .m_slots = _sha256_slots,
    .m_traverse = _sha256_traverse,
    .m_clear = _sha256_clear,
    .m_free = _sha256_free
};

/* Initialize this module. */
PyMODINIT_FUNC
PyInit__sha256(void)
{
    return PyModuleDef_Init(&_sha256module);
}
