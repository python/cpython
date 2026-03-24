/* SHA3 module
 *
 * This module provides an interface to the SHA3 algorithm
 *
 * See below for information about the original code this module was
 * based upon. Additional work performed by:
 *
 *  Andrew Kuchling (amk@amk.ca)
 *  Greg Stein (gstein@lyra.org)
 *  Trevor Perrin (trevp@trevp.net)
 *  Gregory P. Smith (greg@krypto.org)
 *  Bénédikt Tran (10796600+picnixz@users.noreply.github.com)
 *
 * Copyright (C) 2012-2022  Christian Heimes (christian@python.org)
 * Licensed to PSF under a Contributor Agreement.
 *
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_object.h"        // _PyObject_VisitType()
#include "pycore_strhex.h"        // _Py_strhex()
#include "pycore_typeobject.h"    // _PyType_GetModuleState()
#include "hashlib.h"

#include "_hacl/Hacl_Hash_SHA3.h"

/*
 * Assert that 'LEN' can be safely casted to uint32_t.
 *
 * The 'LEN' parameter should be convertible to Py_ssize_t.
 */
#if !defined(NDEBUG) && (PY_SSIZE_T_MAX > UINT32_MAX)
#define CHECK_HACL_UINT32_T_LENGTH(LEN) assert((LEN) < (Py_ssize_t)UINT32_MAX)
#else
#define CHECK_HACL_UINT32_T_LENGTH(LEN)
#endif

#define SHA3_MAX_DIGESTSIZE 64 /* 64 Bytes (512 Bits) for 224 to 512 */

// --- Module state -----------------------------------------------------------

typedef struct {
    PyTypeObject *sha3_224_type;
    PyTypeObject *sha3_256_type;
    PyTypeObject *sha3_384_type;
    PyTypeObject *sha3_512_type;
    PyTypeObject *shake_128_type;
    PyTypeObject *shake_256_type;
} SHA3State;

static inline SHA3State*
sha3_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (SHA3State *)state;
}

// --- Module objects ---------------------------------------------------------

/* The structure for storing SHA3 info */

typedef struct {
    HASHLIB_OBJECT_HEAD
    Hacl_Hash_SHA3_state_t *hash_state;
} SHA3object;

#define _SHA3object_CAST(op)    ((SHA3object *)(op))

// --- Module clinic configuration --------------------------------------------

/*[clinic input]
module _sha3
class _sha3.sha3_224 "SHA3object *" "&PyType_Type"
class _sha3.sha3_256 "SHA3object *" "&PyType_Type"
class _sha3.sha3_384 "SHA3object *" "&PyType_Type"
class _sha3.sha3_512 "SHA3object *" "&PyType_Type"
class _sha3.shake_128 "SHA3object *" "&PyType_Type"
class _sha3.shake_256 "SHA3object *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ccd22550c7fb99bf]*/

#include "clinic/sha3module.c.h"

// --- SHA-3 object interface -------------------------------------------------

static SHA3object *
newSHA3object(PyTypeObject *type)
{
    SHA3object *newobj = PyObject_GC_New(SHA3object, type);
    if (newobj == NULL) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(newobj);

    PyObject_GC_Track(newobj);
    return newobj;
}

static void
sha3_update(Hacl_Hash_SHA3_state_t *state, uint8_t *buf, Py_ssize_t len)
{
  /*
   * Note: we explicitly ignore the error code on the basis that it would
   * take more than 1 billion years to overflow the maximum admissible length
   * for SHA-3 (2^64 - 1).
   */
#if PY_SSIZE_T_MAX > UINT32_MAX
    while (len > UINT32_MAX) {
        (void)Hacl_Hash_SHA3_update(state, buf, UINT32_MAX);
        len -= UINT32_MAX;
        buf += UINT32_MAX;
    }
#endif
    /* cast to uint32_t is now safe */
    (void)Hacl_Hash_SHA3_update(state, buf, (uint32_t)len);
}

/*[clinic input]
@classmethod
_sha3.sha3_224.__new__ as py_sha3_new

    data as data_obj: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Return a new SHA3 hash object.
[clinic start generated code]*/

static PyObject *
py_sha3_new_impl(PyTypeObject *type, PyObject *data_obj, int usedforsecurity,
                 PyObject *string)
/*[clinic end generated code: output=dcec1eca20395f2a input=c106e0b4e2d67d58]*/
{
    PyObject *data;
    if (_Py_hashlib_data_argument(&data, data_obj, string) < 0) {
        return NULL;
    }

    Py_buffer buf = {NULL, NULL};
    SHA3State *state = _PyType_GetModuleState(type);
    SHA3object *self = newSHA3object(type);
    if (self == NULL) {
        goto error;
    }

    assert(state != NULL);

    if (type == state->sha3_224_type) {
        self->hash_state = Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_SHA3_224);
    }
    else if (type == state->sha3_256_type) {
        self->hash_state = Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_SHA3_256);
    }
    else if (type == state->sha3_384_type) {
        self->hash_state = Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_SHA3_384);
    }
    else if (type == state->sha3_512_type) {
        self->hash_state = Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_SHA3_512);
    }
    else if (type == state->shake_128_type) {
        self->hash_state = Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_Shake128);
    }
    else if (type == state->shake_256_type) {
        self->hash_state = Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_Shake256);
    }
    else {
        PyErr_BadInternalCall();
        goto error;
    }

    if (self->hash_state == NULL) {
        (void)PyErr_NoMemory();
        goto error;
    }

    if (data) {
        GET_BUFFER_VIEW_OR_ERROR(data, &buf, goto error);
        /* Do not use self->mutex here as this is the constructor
         * where it is not yet possible to have concurrent access. */
        HASHLIB_EXTERNAL_INSTRUCTIONS_UNLOCKED(
            buf.len,
            sha3_update(self->hash_state, buf.buf, buf.len)
        );
    }

    PyBuffer_Release(&buf);

    return (PyObject *)self;

error:
    if (self) {
        Py_DECREF(self);
    }
    if (data && buf.obj) {
        PyBuffer_Release(&buf);
    }
    return NULL;
}


/* Internal methods for a hash object */

static int
SHA3_clear(PyObject *op)
{
    SHA3object *self = _SHA3object_CAST(op);
    if (self->hash_state != NULL) {
        Hacl_Hash_SHA3_free(self->hash_state);
        self->hash_state = NULL;
    }
    return 0;
}

static void
SHA3_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)SHA3_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

/* External methods for a hash object */


/*[clinic input]
_sha3.sha3_224.copy

    cls: defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_sha3_sha3_224_copy_impl(SHA3object *self, PyTypeObject *cls)
/*[clinic end generated code: output=13958b44c244013e input=7134b4dc0a2fbcac]*/
{
    SHA3object *newobj;
    if ((newobj = newSHA3object(cls)) == NULL) {
        return NULL;
    }
    HASHLIB_ACQUIRE_LOCK(self);
    newobj->hash_state = Hacl_Hash_SHA3_copy(self->hash_state);
    HASHLIB_RELEASE_LOCK(self);
    if (newobj->hash_state == NULL) {
        Py_DECREF(newobj);
        return PyErr_NoMemory();
    }
    return (PyObject *)newobj;
}


/*[clinic input]
_sha3.sha3_224.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_sha3_sha3_224_digest_impl(SHA3object *self)
/*[clinic end generated code: output=fd531842e20b2d5b input=5b2a659536bbd248]*/
{
    unsigned char digest[SHA3_MAX_DIGESTSIZE];
    // This function errors out if the algorithm is SHAKE. Here, we know this
    // not to be the case, and therefore do not perform error checking.
    HASHLIB_ACQUIRE_LOCK(self);
    (void)Hacl_Hash_SHA3_digest(self->hash_state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return PyBytes_FromStringAndSize((const char *)digest,
        Hacl_Hash_SHA3_hash_len(self->hash_state));
}


/*[clinic input]
_sha3.sha3_224.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_sha3_sha3_224_hexdigest_impl(SHA3object *self)
/*[clinic end generated code: output=75ad03257906918d input=2d91bb6e0d114ee3]*/
{
    unsigned char digest[SHA3_MAX_DIGESTSIZE];
    HASHLIB_ACQUIRE_LOCK(self);
    (void)Hacl_Hash_SHA3_digest(self->hash_state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return _Py_strhex((const char *)digest,
        Hacl_Hash_SHA3_hash_len(self->hash_state));
}


/*[clinic input]
_sha3.sha3_224.update

    data: object
    /

Update this hash object's state with the provided bytes-like object.
[clinic start generated code]*/

static PyObject *
_sha3_sha3_224_update_impl(SHA3object *self, PyObject *data)
/*[clinic end generated code: output=390b7abf7c9795a5 input=a887f54dcc4ae227]*/
{
    Py_buffer buf;
    GET_BUFFER_VIEW_OR_ERROUT(data, &buf);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, buf.len,
        sha3_update(self->hash_state, buf.buf, buf.len)
    );
    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}


static PyMethodDef SHA3_methods[] = {
    _SHA3_SHA3_224_COPY_METHODDEF
    _SHA3_SHA3_224_DIGEST_METHODDEF
    _SHA3_SHA3_224_HEXDIGEST_METHODDEF
    _SHA3_SHA3_224_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};


static PyObject *
SHA3_get_block_size(PyObject *op, void *Py_UNUSED(closure))
{
    SHA3object *self = _SHA3object_CAST(op);
    uint32_t rate = Hacl_Hash_SHA3_block_len(self->hash_state);
    return PyLong_FromLong(rate);
}


static PyObject *
SHA3_get_name(PyObject *self, void *Py_UNUSED(closure))
{
    PyTypeObject *type = Py_TYPE(self);

    SHA3State *state = _PyType_GetModuleState(type);
    assert(state != NULL);

    if (type == state->sha3_224_type) {
        return PyUnicode_FromString("sha3_224");
    } else if (type == state->sha3_256_type) {
        return PyUnicode_FromString("sha3_256");
    } else if (type == state->sha3_384_type) {
        return PyUnicode_FromString("sha3_384");
    } else if (type == state->sha3_512_type) {
        return PyUnicode_FromString("sha3_512");
    } else if (type == state->shake_128_type) {
        return PyUnicode_FromString("shake_128");
    } else if (type == state->shake_256_type) {
        return PyUnicode_FromString("shake_256");
    } else {
        PyErr_BadInternalCall();
        return NULL;
    }
}


static PyObject *
SHA3_get_digest_size(PyObject *op, void *Py_UNUSED(closure))
{
    // Preserving previous behavior: variable-length algorithms return 0
    SHA3object *self = _SHA3object_CAST(op);
    if (Hacl_Hash_SHA3_is_shake(self->hash_state))
      return PyLong_FromLong(0);
    else
      return PyLong_FromLong(Hacl_Hash_SHA3_hash_len(self->hash_state));
}


static PyObject *
SHA3_get_capacity_bits(PyObject *op, void *Py_UNUSED(closure))
{
    SHA3object *self = _SHA3object_CAST(op);
    uint32_t rate = Hacl_Hash_SHA3_block_len(self->hash_state) * 8;
    assert(rate <= 1600);
    int capacity = 1600 - rate;
    return PyLong_FromLong(capacity);
}


static PyObject *
SHA3_get_rate_bits(PyObject *op, void *Py_UNUSED(closure))
{
    SHA3object *self = _SHA3object_CAST(op);
    uint32_t rate = Hacl_Hash_SHA3_block_len(self->hash_state) * 8;
    return PyLong_FromLong(rate);
}

static PyObject *
SHA3_get_suffix(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    unsigned char suffix[2] = {0x06, 0};
    return PyBytes_FromStringAndSize((const char *)suffix, 1);
}

static PyGetSetDef SHA3_getseters[] = {
    {"block_size", SHA3_get_block_size, NULL, NULL, NULL},
    {"name", SHA3_get_name, NULL, NULL, NULL},
    {"digest_size", SHA3_get_digest_size, NULL, NULL, NULL},
    {"_capacity_bits", SHA3_get_capacity_bits, NULL, NULL, NULL},
    {"_rate_bits", SHA3_get_rate_bits, NULL, NULL, NULL},
    {"_suffix", SHA3_get_suffix, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

#define SHA3_TYPE_SLOTS(type_slots_obj, type_doc, type_methods, type_getseters) \
    static PyType_Slot type_slots_obj[] = { \
        {Py_tp_clear, SHA3_clear}, \
        {Py_tp_dealloc, SHA3_dealloc}, \
        {Py_tp_traverse, _PyObject_VisitType}, \
        {Py_tp_doc, (char*)type_doc}, \
        {Py_tp_methods, type_methods}, \
        {Py_tp_getset, type_getseters}, \
        {Py_tp_new, py_sha3_new}, \
        {0, NULL} \
    }

// Using _PyType_GetModuleState() on these types is safe since they
// cannot be subclassed: it does not have the Py_TPFLAGS_BASETYPE flag.
#define SHA3_TYPE_SPEC(type_spec_obj, type_name, type_slots) \
    static PyType_Spec type_spec_obj = { \
        .name = "_sha3." type_name, \
        .basicsize = sizeof(SHA3object), \
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE \
                 | Py_TPFLAGS_HAVE_GC, \
        .slots = type_slots \
    }

PyDoc_STRVAR(sha3_224__doc__,
"sha3_224([data], *, usedforsecurity=True) -> SHA3 object\n\
\n\
Return a new SHA3 hash object with a hashbit length of 28 bytes.");

PyDoc_STRVAR(sha3_256__doc__,
"sha3_256([data], *, usedforsecurity=True) -> SHA3 object\n\
\n\
Return a new SHA3 hash object with a hashbit length of 32 bytes.");

PyDoc_STRVAR(sha3_384__doc__,
"sha3_384([data], *, usedforsecurity=True) -> SHA3 object\n\
\n\
Return a new SHA3 hash object with a hashbit length of 48 bytes.");

PyDoc_STRVAR(sha3_512__doc__,
"sha3_512([data], *, usedforsecurity=True) -> SHA3 object\n\
\n\
Return a new SHA3 hash object with a hashbit length of 64 bytes.");

SHA3_TYPE_SLOTS(sha3_224_slots, sha3_224__doc__, SHA3_methods, SHA3_getseters);
SHA3_TYPE_SPEC(sha3_224_spec, "sha3_224", sha3_224_slots);

SHA3_TYPE_SLOTS(sha3_256_slots, sha3_256__doc__, SHA3_methods, SHA3_getseters);
SHA3_TYPE_SPEC(sha3_256_spec, "sha3_256", sha3_256_slots);

SHA3_TYPE_SLOTS(sha3_384_slots, sha3_384__doc__, SHA3_methods, SHA3_getseters);
SHA3_TYPE_SPEC(sha3_384_spec, "sha3_384", sha3_384_slots);

SHA3_TYPE_SLOTS(sha3_512_slots, sha3_512__doc__, SHA3_methods, SHA3_getseters);
SHA3_TYPE_SPEC(sha3_512_spec, "sha3_512", sha3_512_slots);

static int
sha3_shake_check_digest_length(Py_ssize_t length)
{
    assert(length >= 0);
    if ((size_t)length >= (1 << 29)) {
        /*
         * Raise OverflowError to match the semantics of OpenSSL SHAKE
         * when the digest length exceeds the range of a 'Py_ssize_t';
         * the exception message will however be different in this case.
         */
        PyErr_SetString(PyExc_OverflowError, "digest length is too large");
        return -1;
    }
    return 0;
}


/*[clinic input]
_sha3.shake_128.digest

    length: Py_ssize_t(allow_negative=False)

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_sha3_shake_128_digest_impl(SHA3object *self, Py_ssize_t length)
/*[clinic end generated code: output=6c53fb71a6cff0a0 input=1160c9f86ae0f867]*/
{
    if (sha3_shake_check_digest_length(length) < 0) {
        return NULL;
    }

    /*
     * Hacl_Hash_SHA3_squeeze() fails if the algorithm is not SHAKE,
     * or if the length is 0. In the latter case, we follow OpenSSL's
     * behavior and return an empty digest, without raising an error.
     */
    if (length == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    CHECK_HACL_UINT32_T_LENGTH(length);

    PyBytesWriter *writer = PyBytesWriter_Create(length);
    if (writer == NULL) {
        return NULL;
    }
    uint8_t *buffer = (uint8_t *)PyBytesWriter_GetData(writer);

    HASHLIB_ACQUIRE_LOCK(self);
    (void)Hacl_Hash_SHA3_squeeze(self->hash_state, buffer, (uint32_t)length);
    HASHLIB_RELEASE_LOCK(self);

    return PyBytesWriter_Finish(writer);
}


/*[clinic input]
_sha3.shake_128.hexdigest

    length: Py_ssize_t(allow_negative=False)

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_sha3_shake_128_hexdigest_impl(SHA3object *self, Py_ssize_t length)
/*[clinic end generated code: output=a27412d404f64512 input=ff06c9362949d2c8]*/
{
    if (sha3_shake_check_digest_length(length) < 0) {
        return NULL;
    }

    /* See _sha3_shake_128_digest_impl() for the fast path rationale. */
    if (length == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_STR);
    }
    CHECK_HACL_UINT32_T_LENGTH(length);

    uint8_t *buffer = PyMem_Malloc(length);
    if (buffer == NULL) {
        return PyErr_NoMemory();
    }

    HASHLIB_ACQUIRE_LOCK(self);
    (void)Hacl_Hash_SHA3_squeeze(self->hash_state, buffer, (uint32_t)length);
    HASHLIB_RELEASE_LOCK(self);

    PyObject *digest = _Py_strhex((const char *)buffer, length);
    PyMem_Free(buffer);
    return digest;
}

static PyObject *
SHAKE_get_digest_size(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyLong_FromLong(0);
}

static PyObject *
SHAKE_get_suffix(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    unsigned char suffix[2] = {0x1f, 0};
    return PyBytes_FromStringAndSize((const char *)suffix, 1);
}


static PyGetSetDef SHAKE_getseters[] = {
    {"block_size", SHA3_get_block_size, NULL, NULL, NULL},
    {"name", SHA3_get_name, NULL, NULL, NULL},
    {"digest_size", SHAKE_get_digest_size, NULL, NULL, NULL},
    {"_capacity_bits", SHA3_get_capacity_bits, NULL, NULL, NULL},
    {"_rate_bits", SHA3_get_rate_bits, NULL, NULL, NULL},
    {"_suffix", SHAKE_get_suffix, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};


static PyMethodDef SHAKE_methods[] = {
    _SHA3_SHA3_224_COPY_METHODDEF
    _SHA3_SHAKE_128_DIGEST_METHODDEF
    _SHA3_SHAKE_128_HEXDIGEST_METHODDEF
    _SHA3_SHA3_224_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

PyDoc_STRVAR(shake_128__doc__,
"shake_128([data], *, usedforsecurity=True) -> SHAKE object\n\
\n\
Return a new SHAKE hash object.");

PyDoc_STRVAR(shake_256__doc__,
"shake_256([data], *, usedforsecurity=True) -> SHAKE object\n\
\n\
Return a new SHAKE hash object.");

SHA3_TYPE_SLOTS(SHAKE128slots, shake_128__doc__, SHAKE_methods, SHAKE_getseters);
SHA3_TYPE_SPEC(SHAKE128_spec, "shake_128", SHAKE128slots);

SHA3_TYPE_SLOTS(SHAKE256slots, shake_256__doc__, SHAKE_methods, SHAKE_getseters);
SHA3_TYPE_SPEC(SHAKE256_spec, "shake_256", SHAKE256slots);


static int
_sha3_traverse(PyObject *module, visitproc visit, void *arg)
{
    SHA3State *state = sha3_get_state(module);
    Py_VISIT(state->sha3_224_type);
    Py_VISIT(state->sha3_256_type);
    Py_VISIT(state->sha3_384_type);
    Py_VISIT(state->sha3_512_type);
    Py_VISIT(state->shake_128_type);
    Py_VISIT(state->shake_256_type);
    return 0;
}

static int
_sha3_clear(PyObject *module)
{
    SHA3State *state = sha3_get_state(module);
    Py_CLEAR(state->sha3_224_type);
    Py_CLEAR(state->sha3_256_type);
    Py_CLEAR(state->sha3_384_type);
    Py_CLEAR(state->sha3_512_type);
    Py_CLEAR(state->shake_128_type);
    Py_CLEAR(state->shake_256_type);
    return 0;
}

static void
_sha3_free(void *module)
{
    (void)_sha3_clear((PyObject *)module);
}

static int
_sha3_exec(PyObject *m)
{
    SHA3State *st = sha3_get_state(m);

#define init_sha3type(type, typespec)                           \
    do {                                                        \
        st->type = (PyTypeObject *)PyType_FromModuleAndSpec(    \
            m, &typespec, NULL);                                \
        if (st->type == NULL) {                                 \
            return -1;                                          \
        }                                                       \
        if (PyModule_AddType(m, st->type) < 0) {                \
            return -1;                                          \
        }                                                       \
    } while(0)

    init_sha3type(sha3_224_type, sha3_224_spec);
    init_sha3type(sha3_256_type, sha3_256_spec);
    init_sha3type(sha3_384_type, sha3_384_spec);
    init_sha3type(sha3_512_type, sha3_512_spec);
    init_sha3type(shake_128_type, SHAKE128_spec);
    init_sha3type(shake_256_type, SHAKE256_spec);
#undef init_sha3type

    if (PyModule_AddStringConstant(m, "implementation", "HACL") < 0) {
        return -1;
    }
    if (PyModule_AddIntConstant(m, "_GIL_MINSIZE", HASHLIB_GIL_MINSIZE) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot _sha3_slots[] = {
    {Py_mod_exec, _sha3_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

/* Initialize this module. */
static struct PyModuleDef _sha3module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_sha3",
    .m_size = sizeof(SHA3State),
    .m_slots = _sha3_slots,
    .m_traverse = _sha3_traverse,
    .m_clear = _sha3_clear,
    .m_free = _sha3_free,
};


PyMODINIT_FUNC
PyInit__sha3(void)
{
    return PyModuleDef_Init(&_sha3module);
}
