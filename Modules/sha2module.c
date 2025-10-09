/* SHA2 module */

/* This provides an interface to NIST's SHA2 224, 256, 384, & 512 Algorithms */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)
   Trevor Perrin (trevp@trevp.net)
   Jonathan Protzenko (jonathan@protzenko.fr)
   Bénédikt Tran (10796600+picnixz@users.noreply.github.com)

   Copyright (C) 2005-2007   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_object.h"        // _PyObject_VisitType()
#include "pycore_strhex.h"        // _Py_strhex()
#include "pycore_typeobject.h"    // _PyType_GetModuleState()

#include "hashlib.h"

#include "_hacl/Hacl_Hash_SHA2.h"

/* The SHA block sizes and maximum message digest sizes, in bytes */

#define SHA256_BLOCKSIZE   64
#define SHA256_DIGESTSIZE  32
#define SHA512_BLOCKSIZE   128
#define SHA512_DIGESTSIZE  64

// --- Module objects ---------------------------------------------------------

// TODO: Get rid of int digestsize in favor of Hacl state info?

typedef struct {
    HASHLIB_OBJECT_HEAD
    int digestsize;
    Hacl_Hash_SHA2_state_t_256 *state;
} SHA256object;

typedef struct {
    HASHLIB_OBJECT_HEAD
    int digestsize;
    Hacl_Hash_SHA2_state_t_512 *state;
} SHA512object;

#define _SHA256object_CAST(op)  ((SHA256object *)(op))
#define _SHA512object_CAST(op)  ((SHA512object *)(op))

// --- Module state -----------------------------------------------------------

/* We shall use run-time type information in the remainder of this module to
 * tell apart SHA2-224 and SHA2-256 */
typedef struct {
    PyTypeObject* sha224_type;
    PyTypeObject* sha256_type;
    PyTypeObject* sha384_type;
    PyTypeObject* sha512_type;
} sha2_state;

static inline sha2_state*
sha2_get_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (sha2_state *)state;
}

// --- Module clinic configuration --------------------------------------------

/*[clinic input]
module _sha2
class SHA256Type "SHA256object *" "&PyType_Type"
class SHA512Type "SHA512object *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b5315a7b611c9afc]*/

#include "clinic/sha2module.c.h"

// --- SHA-2 object interface -------------------------------------------------

static int
SHA256copy(SHA256object *src, SHA256object *dest)
{
    dest->digestsize = src->digestsize;
    dest->state = Hacl_Hash_SHA2_copy_256(src->state);
    if (dest->state == NULL) {
        (void)PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static int
SHA512copy(SHA512object *src, SHA512object *dest)
{
    dest->digestsize = src->digestsize;
    dest->state = Hacl_Hash_SHA2_copy_512(src->state);
    if (dest->state == NULL) {
        (void)PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static SHA256object *
newSHA224object(sha2_state *state)
{
    SHA256object *sha = PyObject_GC_New(SHA256object, state->sha224_type);
    if (!sha) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(sha);

    PyObject_GC_Track(sha);
    return sha;
}

static SHA256object *
newSHA256object(sha2_state *state)
{
    SHA256object *sha = PyObject_GC_New(SHA256object, state->sha256_type);
    if (!sha) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(sha);

    PyObject_GC_Track(sha);
    return sha;
}

static SHA512object *
newSHA384object(sha2_state *state)
{
    SHA512object *sha = PyObject_GC_New(SHA512object, state->sha384_type);
    if (!sha) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(sha);

    PyObject_GC_Track(sha);
    return sha;
}

static SHA512object *
newSHA512object(sha2_state *state)
{
    SHA512object *sha = PyObject_GC_New(SHA512object, state->sha512_type);
    if (!sha) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(sha);

    PyObject_GC_Track(sha);
    return sha;
}

/* Internal methods for our hash objects. */
static void
SHA256_dealloc(PyObject *op)
{
    SHA256object *ptr = _SHA256object_CAST(op);
    if (ptr->state != NULL) {
        Hacl_Hash_SHA2_free_256(ptr->state);
        ptr->state = NULL;
    }
    PyTypeObject *tp = Py_TYPE(ptr);
    PyObject_GC_UnTrack(ptr);
    PyObject_GC_Del(ptr);
    Py_DECREF(tp);
}

static void
SHA512_dealloc(PyObject *op)
{
    SHA512object *ptr = _SHA512object_CAST(op);
    if (ptr->state != NULL) {
        Hacl_Hash_SHA2_free_512(ptr->state);
        ptr->state = NULL;
    }
    PyTypeObject *tp = Py_TYPE(ptr);
    PyObject_GC_UnTrack(ptr);
    PyObject_GC_Del(ptr);
    Py_DECREF(tp);
}

/* HACL* takes a uint32_t for the length of its parameter, but Py_ssize_t can be
 * 64 bits so we loop in <4gig chunks when needed. */

static void
update_256(Hacl_Hash_SHA2_state_t_256 *state, uint8_t *buf, Py_ssize_t len)
{
    /*
     * Note: we explicitly ignore the error code on the basis that it would
     * take more than 1 billion years to overflow the maximum admissible length
     * for SHA-2-256 (2^61 - 1).
     */
#if PY_SSIZE_T_MAX > UINT32_MAX
    while (len > UINT32_MAX) {
        (void)Hacl_Hash_SHA2_update_256(state, buf, UINT32_MAX);
        len -= UINT32_MAX;
        buf += UINT32_MAX;
    }
#endif
    /* cast to uint32_t is now safe */
    (void)Hacl_Hash_SHA2_update_256(state, buf, (uint32_t)len);
}

static void
update_512(Hacl_Hash_SHA2_state_t_512 *state, uint8_t *buf, Py_ssize_t len)
{
    /*
     * Note: we explicitly ignore the error code on the basis that it would
     * take more than 1 billion years to overflow the maximum admissible length
     * for SHA-2-512 (2^64 - 1).
     */
#if PY_SSIZE_T_MAX > UINT32_MAX
    while (len > UINT32_MAX) {
        (void)Hacl_Hash_SHA2_update_512(state, buf, UINT32_MAX);
        len -= UINT32_MAX;
        buf += UINT32_MAX;
    }
#endif
    /* cast to uint32_t is now safe */
    (void)Hacl_Hash_SHA2_update_512(state, buf, (uint32_t)len);
}


/* External methods for our hash objects */

/*[clinic input]
SHA256Type.copy

    cls:defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
SHA256Type_copy_impl(SHA256object *self, PyTypeObject *cls)
/*[clinic end generated code: output=fabd515577805cd3 input=3137146fcb88e212]*/
{
    int rc;
    SHA256object *newobj;
    sha2_state *state = _PyType_GetModuleState(cls);
    if (Py_IS_TYPE(self, state->sha256_type)) {
        if ((newobj = newSHA256object(state)) == NULL) {
            return NULL;
        }
    }
    else {
        if ((newobj = newSHA224object(state)) == NULL) {
            return NULL;
        }
    }

    HASHLIB_ACQUIRE_LOCK(self);
    rc = SHA256copy(self, newobj);
    HASHLIB_RELEASE_LOCK(self);
    if (rc < 0) {
        Py_DECREF(newobj);
        return NULL;
    }
    return (PyObject *)newobj;
}

/*[clinic input]
SHA512Type.copy

    cls: defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
SHA512Type_copy_impl(SHA512object *self, PyTypeObject *cls)
/*[clinic end generated code: output=66d2a8ef20de8302 input=f673a18f66527c90]*/
{
    int rc;
    SHA512object *newobj;
    sha2_state *state = _PyType_GetModuleState(cls);

    if (Py_IS_TYPE((PyObject*)self, state->sha512_type)) {
        if ((newobj = newSHA512object(state)) == NULL) {
            return NULL;
        }
    }
    else {
        if ((newobj = newSHA384object(state)) == NULL) {
            return NULL;
        }
    }

    HASHLIB_ACQUIRE_LOCK(self);
    rc = SHA512copy(self, newobj);
    HASHLIB_RELEASE_LOCK(self);
    if (rc < 0) {
        Py_DECREF(newobj);
        return NULL;
    }
    return (PyObject *)newobj;
}

/*[clinic input]
SHA256Type.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
SHA256Type_digest_impl(SHA256object *self)
/*[clinic end generated code: output=3a2e3997a98ee792 input=f1f4cfea5cbde35c]*/
{
    uint8_t digest[SHA256_DIGESTSIZE];
    assert(self->digestsize <= SHA256_DIGESTSIZE);
    HASHLIB_ACQUIRE_LOCK(self);
    // HACL* performs copies under the hood so that self->state remains valid
    // after this call.
    Hacl_Hash_SHA2_digest_256(self->state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return PyBytes_FromStringAndSize((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA512Type.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
SHA512Type_digest_impl(SHA512object *self)
/*[clinic end generated code: output=dd8c6320070458e0 input=f6470dd359071f4b]*/
{
    uint8_t digest[SHA512_DIGESTSIZE];
    assert(self->digestsize <= SHA512_DIGESTSIZE);
    HASHLIB_ACQUIRE_LOCK(self);
    // HACL* performs copies under the hood so that self->state remains valid
    // after this call.
    Hacl_Hash_SHA2_digest_512(self->state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return PyBytes_FromStringAndSize((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA256Type.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
SHA256Type_hexdigest_impl(SHA256object *self)
/*[clinic end generated code: output=96cb68996a780ab3 input=0cc4c714693010d1]*/
{
    uint8_t digest[SHA256_DIGESTSIZE];
    assert(self->digestsize <= SHA256_DIGESTSIZE);
    HASHLIB_ACQUIRE_LOCK(self);
    Hacl_Hash_SHA2_digest_256(self->state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return _Py_strhex((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA512Type.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
SHA512Type_hexdigest_impl(SHA512object *self)
/*[clinic end generated code: output=cbd6f844aba1fe7c input=498b877b25cbe0a2]*/
{
    uint8_t digest[SHA512_DIGESTSIZE];
    assert(self->digestsize <= SHA512_DIGESTSIZE);
    HASHLIB_ACQUIRE_LOCK(self);
    Hacl_Hash_SHA2_digest_512(self->state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return _Py_strhex((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA256Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
SHA256Type_update_impl(SHA256object *self, PyObject *obj)
/*[clinic end generated code: output=dc58a580cf8905a5 input=b2d449d5b30f0f5a]*/
{
    Py_buffer buf;
    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, buf.len,
        update_256(self->state, buf.buf, buf.len)
    );
    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

/*[clinic input]
SHA512Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
SHA512Type_update_impl(SHA512object *self, PyObject *obj)
/*[clinic end generated code: output=9af211766c0b7365 input=ded2b46656566283]*/
{
    Py_buffer buf;
    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, buf.len,
        update_512(self->state, buf.buf, buf.len)
    );
    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static PyMethodDef SHA256_methods[] = {
    SHA256TYPE_COPY_METHODDEF
    SHA256TYPE_DIGEST_METHODDEF
    SHA256TYPE_HEXDIGEST_METHODDEF
    SHA256TYPE_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

static PyMethodDef SHA512_methods[] = {
    SHA512TYPE_COPY_METHODDEF
    SHA512TYPE_DIGEST_METHODDEF
    SHA512TYPE_HEXDIGEST_METHODDEF
    SHA512TYPE_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

static PyObject *
SHA256_get_block_size(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyLong_FromLong(SHA256_BLOCKSIZE);
}

static PyObject *
SHA512_get_block_size(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyLong_FromLong(SHA512_BLOCKSIZE);
}

static PyObject *
SHA256_get_digest_size(PyObject *op, void *Py_UNUSED(closure))
{
    SHA256object *self = _SHA256object_CAST(op);
    return PyLong_FromLong(self->digestsize);
}

static PyObject *
SHA512_get_digest_size(PyObject *op, void *Py_UNUSED(closure))
{
    SHA512object *self = _SHA512object_CAST(op);
    return PyLong_FromLong(self->digestsize);
}

static PyObject *
SHA256_get_name(PyObject *op, void *Py_UNUSED(closure))
{
    SHA256object *self = _SHA256object_CAST(op);
    if (self->digestsize == 28) {
        return PyUnicode_FromStringAndSize("sha224", 6);
    }
    return PyUnicode_FromStringAndSize("sha256", 6);
}

static PyObject *
SHA512_get_name(PyObject *op, void *Py_UNUSED(closure))
{
    SHA512object *self = _SHA512object_CAST(op);
    if (self->digestsize == 64) {
        return PyUnicode_FromStringAndSize("sha512", 6);
    }
    return PyUnicode_FromStringAndSize("sha384", 6);
}

static PyGetSetDef SHA256_getseters[] = {
    {"block_size", SHA256_get_block_size, NULL, NULL, NULL},
    {"name", SHA256_get_name, NULL, NULL, NULL},
    {"digest_size", SHA256_get_digest_size, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

static PyGetSetDef SHA512_getseters[] = {
    {"block_size", SHA512_get_block_size, NULL, NULL, NULL},
    {"name", SHA512_get_name, NULL, NULL, NULL},
    {"digest_size", SHA512_get_digest_size, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

static PyType_Slot sha256_types_slots[] = {
    {Py_tp_dealloc, SHA256_dealloc},
    {Py_tp_methods, SHA256_methods},
    {Py_tp_getset, SHA256_getseters},
    {Py_tp_traverse, _PyObject_VisitType},
    {0,0}
};

static PyType_Slot sha512_type_slots[] = {
    {Py_tp_dealloc, SHA512_dealloc},
    {Py_tp_methods, SHA512_methods},
    {Py_tp_getset, SHA512_getseters},
    {Py_tp_traverse, _PyObject_VisitType},
    {0,0}
};

// Using _PyType_GetModuleState() on these types is safe since they
// cannot be subclassed: they don't have the Py_TPFLAGS_BASETYPE flag.
static PyType_Spec sha224_type_spec = {
    .name = "_sha2.SHA224Type",
    .basicsize = sizeof(SHA256object),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha256_types_slots
};

static PyType_Spec sha256_type_spec = {
    .name = "_sha2.SHA256Type",
    .basicsize = sizeof(SHA256object),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha256_types_slots
};

static PyType_Spec sha384_type_spec = {
    .name = "_sha2.SHA384Type",
    .basicsize =  sizeof(SHA512object),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha512_type_slots
};

static PyType_Spec sha512_type_spec = {
    .name = "_sha2.SHA512Type",
    .basicsize =  sizeof(SHA512object),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha512_type_slots
};

/* The module-level constructors. */

/*[clinic input]
_sha2.sha256

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string as string_obj: object(c_default="NULL") = None

Return a new SHA-256 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha2_sha256_impl(PyObject *module, PyObject *data, int usedforsecurity,
                  PyObject *string_obj)
/*[clinic end generated code: output=49828a7bcd418f45 input=9ce1d70e669abc14]*/
{
    Py_buffer buf;
    PyObject *string;
    if (_Py_hashlib_data_argument(&string, data, string_obj) < 0) {
        return NULL;
    }

    if (string) {
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);
    }

    sha2_state *state = sha2_get_state(module);

    SHA256object *new;
    if ((new = newSHA256object(state)) == NULL) {
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }

    new->state = Hacl_Hash_SHA2_malloc_256();
    new->digestsize = 32;

    if (new->state == NULL) {
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
            update_256(new->state, buf.buf, buf.len)
        );
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}

/*[clinic input]
_sha2.sha224

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string as string_obj: object(c_default="NULL") = None

Return a new SHA-224 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha2_sha224_impl(PyObject *module, PyObject *data, int usedforsecurity,
                  PyObject *string_obj)
/*[clinic end generated code: output=2163cb03b6cf6157 input=612f7682a889bc2a]*/
{
    Py_buffer buf;
    PyObject *string;
    if (_Py_hashlib_data_argument(&string, data, string_obj) < 0) {
        return NULL;
    }

    if (string) {
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);
    }

    sha2_state *state = sha2_get_state(module);
    SHA256object *new;
    if ((new = newSHA224object(state)) == NULL) {
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }

    new->state = Hacl_Hash_SHA2_malloc_224();
    new->digestsize = 28;

    if (new->state == NULL) {
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
            update_256(new->state, buf.buf, buf.len)
        );
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}

/*[clinic input]
_sha2.sha512

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string as string_obj: object(c_default="NULL") = None

Return a new SHA-512 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha2_sha512_impl(PyObject *module, PyObject *data, int usedforsecurity,
                  PyObject *string_obj)
/*[clinic end generated code: output=cc3fcfce001a4538 input=19c9f2c06d59563a]*/
{
    SHA512object *new;
    Py_buffer buf;
    PyObject *string;
    if (_Py_hashlib_data_argument(&string, data, string_obj) < 0) {
        return NULL;
    }

    sha2_state *state = sha2_get_state(module);

    if (string) {
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);
    }

    if ((new = newSHA512object(state)) == NULL) {
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }

    new->state = Hacl_Hash_SHA2_malloc_512();
    new->digestsize = 64;

    if (new->state == NULL) {
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
            update_512(new->state, buf.buf, buf.len)
        );
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}

/*[clinic input]
_sha2.sha384

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string as string_obj: object(c_default="NULL") = None

Return a new SHA-384 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha2_sha384_impl(PyObject *module, PyObject *data, int usedforsecurity,
                  PyObject *string_obj)
/*[clinic end generated code: output=b6e3db593b5a0330 input=9fd50c942ad9e0bf]*/
{
    SHA512object *new;
    Py_buffer buf;
    PyObject *string;
    if (_Py_hashlib_data_argument(&string, data, string_obj) < 0) {
        return NULL;
    }

    sha2_state *state = sha2_get_state(module);

    if (string) {
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);
    }

    if ((new = newSHA384object(state)) == NULL) {
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }

    new->state = Hacl_Hash_SHA2_malloc_384();
    new->digestsize = 48;

    if (new->state == NULL) {
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
            update_512(new->state, buf.buf, buf.len)
        );
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}

/* List of functions exported by this module */

static struct PyMethodDef SHA2_functions[] = {
    _SHA2_SHA256_METHODDEF
    _SHA2_SHA224_METHODDEF
    _SHA2_SHA512_METHODDEF
    _SHA2_SHA384_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};

static int
_sha2_traverse(PyObject *module, visitproc visit, void *arg)
{
    sha2_state *state = sha2_get_state(module);
    Py_VISIT(state->sha224_type);
    Py_VISIT(state->sha256_type);
    Py_VISIT(state->sha384_type);
    Py_VISIT(state->sha512_type);
    return 0;
}

static int
_sha2_clear(PyObject *module)
{
    sha2_state *state = sha2_get_state(module);
    Py_CLEAR(state->sha224_type);
    Py_CLEAR(state->sha256_type);
    Py_CLEAR(state->sha384_type);
    Py_CLEAR(state->sha512_type);
    return 0;
}

static void
_sha2_free(void *module)
{
    (void)_sha2_clear((PyObject *)module);
}

/* Initialize this module. */
static int sha2_exec(PyObject *module)
{
    sha2_state *state = sha2_get_state(module);

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
    state->sha384_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &sha384_type_spec, NULL);
    if (state->sha384_type == NULL) {
        return -1;
    }
    state->sha512_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &sha512_type_spec, NULL);
    if (state->sha512_type == NULL) {
        return -1;
    }

    if (PyModule_AddType(module, state->sha224_type) < 0) {
        return -1;
    }
    if (PyModule_AddType(module, state->sha256_type) < 0) {
        return -1;
    }
    if (PyModule_AddType(module, state->sha384_type) < 0) {
        return -1;
    }
    if (PyModule_AddType(module, state->sha512_type) < 0) {
        return -1;
    }

    if (PyModule_AddIntConstant(module,
                                "_GIL_MINSIZE",
                                HASHLIB_GIL_MINSIZE) < 0)
    {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot _sha2_slots[] = {
    {Py_mod_exec, sha2_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _sha2module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_sha2",
    .m_size = sizeof(sha2_state),
    .m_methods = SHA2_functions,
    .m_slots = _sha2_slots,
    .m_traverse = _sha2_traverse,
    .m_clear = _sha2_clear,
    .m_free = _sha2_free
};

PyMODINIT_FUNC
PyInit__sha2(void)
{
    return PyModuleDef_Init(&_sha2module);
}
