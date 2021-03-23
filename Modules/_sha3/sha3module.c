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
 *
 * Copyright (C) 2012-2016  Christian Heimes (christian@python.org)
 * Licensed to PSF under a Contributor Agreement.
 *
 */

#include "Python.h"
#include "pystrhex.h"
#include "../hashlib.h"

/* **************************************************************************
 *                          SHA-3 (Keccak) and SHAKE
 *
 * The code is based on KeccakCodePackage from 2016-04-23
 * commit 647f93079afc4ada3d23737477a6e52511ca41fd
 *
 * The reference implementation is altered in this points:
 *  - C++ comments are converted to ANSI C comments.
 *  - all function names are mangled
 *  - typedef for UINT64 is commented out.
 *  - brg_endian.h is removed
 *
 * *************************************************************************/

#ifdef __sparc
  /* opt64 uses un-aligned memory access that causes a BUS error with msg
   * 'invalid address alignment' on SPARC. */
  #define KeccakOpt 32
#elif PY_BIG_ENDIAN
  /* opt64 is not yet supported on big endian platforms */
  #define KeccakOpt 32
#elif SIZEOF_VOID_P == 8
  /* opt64 works only on little-endian 64bit platforms with unsigned int64 */
  #define KeccakOpt 64
#else
  /* opt32 is used for the remaining 32 and 64bit platforms */
  #define KeccakOpt 32
#endif

#if KeccakOpt == 64
  /* 64bit platforms with unsigned int64 */
  typedef uint64_t UINT64;
  typedef unsigned char UINT8;
#endif

/* replacement for brg_endian.h */
#define IS_LITTLE_ENDIAN 1234
#define IS_BIG_ENDIAN 4321
#if PY_LITTLE_ENDIAN
#define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif
#if PY_BIG_ENDIAN
#define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#endif

/* mangle names */
#define KeccakF1600_FastLoop_Absorb _PySHA3_KeccakF1600_FastLoop_Absorb
#define Keccak_HashFinal _PySHA3_Keccak_HashFinal
#define Keccak_HashInitialize _PySHA3_Keccak_HashInitialize
#define Keccak_HashSqueeze _PySHA3_Keccak_HashSqueeze
#define Keccak_HashUpdate _PySHA3_Keccak_HashUpdate
#define KeccakP1600_AddBytes _PySHA3_KeccakP1600_AddBytes
#define KeccakP1600_AddBytesInLane _PySHA3_KeccakP1600_AddBytesInLane
#define KeccakP1600_AddLanes _PySHA3_KeccakP1600_AddLanes
#define KeccakP1600_ExtractAndAddBytes _PySHA3_KeccakP1600_ExtractAndAddBytes
#define KeccakP1600_ExtractAndAddBytesInLane _PySHA3_KeccakP1600_ExtractAndAddBytesInLane
#define KeccakP1600_ExtractAndAddLanes _PySHA3_KeccakP1600_ExtractAndAddLanes
#define KeccakP1600_ExtractBytes _PySHA3_KeccakP1600_ExtractBytes
#define KeccakP1600_ExtractBytesInLane _PySHA3_KeccakP1600_ExtractBytesInLane
#define KeccakP1600_ExtractLanes _PySHA3_KeccakP1600_ExtractLanes
#define KeccakP1600_Initialize _PySHA3_KeccakP1600_Initialize
#define KeccakP1600_OverwriteBytes _PySHA3_KeccakP1600_OverwriteBytes
#define KeccakP1600_OverwriteBytesInLane _PySHA3_KeccakP1600_OverwriteBytesInLane
#define KeccakP1600_OverwriteLanes _PySHA3_KeccakP1600_OverwriteLanes
#define KeccakP1600_OverwriteWithZeroes _PySHA3_KeccakP1600_OverwriteWithZeroes
#define KeccakP1600_Permute_12rounds _PySHA3_KeccakP1600_Permute_12rounds
#define KeccakP1600_Permute_24rounds _PySHA3_KeccakP1600_Permute_24rounds
#define KeccakWidth1600_Sponge _PySHA3_KeccakWidth1600_Sponge
#define KeccakWidth1600_SpongeAbsorb _PySHA3_KeccakWidth1600_SpongeAbsorb
#define KeccakWidth1600_SpongeAbsorbLastFewBits _PySHA3_KeccakWidth1600_SpongeAbsorbLastFewBits
#define KeccakWidth1600_SpongeInitialize _PySHA3_KeccakWidth1600_SpongeInitialize
#define KeccakWidth1600_SpongeSqueeze _PySHA3_KeccakWidth1600_SpongeSqueeze
#if KeccakOpt == 32
#define KeccakP1600_AddByte _PySHA3_KeccakP1600_AddByte
#define KeccakP1600_Permute_Nrounds _PySHA3_KeccakP1600_Permute_Nrounds
#define KeccakP1600_SetBytesInLaneToZero _PySHA3_KeccakP1600_SetBytesInLaneToZero
#endif

/* we are only interested in KeccakP1600 */
#define KeccakP200_excluded 1
#define KeccakP400_excluded 1
#define KeccakP800_excluded 1

/* inline all Keccak dependencies */
#include "kcp/KeccakHash.h"
#include "kcp/KeccakSponge.h"
#include "kcp/KeccakHash.c"
#include "kcp/KeccakSponge.c"
#if KeccakOpt == 64
  #include "kcp/KeccakP-1600-opt64.c"
#elif KeccakOpt == 32
  #include "kcp/KeccakP-1600-inplace32BI.c"
#endif

#define SHA3_MAX_DIGESTSIZE 64 /* 64 Bytes (512 Bits) for 224 to 512 */
#define SHA3_LANESIZE (20 * 8) /* ExtractLane needs max uint64_t[20] extra. */
#define SHA3_state Keccak_HashInstance
#define SHA3_init Keccak_HashInitialize
#define SHA3_process Keccak_HashUpdate
#define SHA3_done Keccak_HashFinal
#define SHA3_squeeze Keccak_HashSqueeze
#define SHA3_copystate(dest, src) memcpy(&(dest), &(src), sizeof(SHA3_state))

typedef struct {
    PyTypeObject *sha3_224_type;
    PyTypeObject *sha3_256_type;
    PyTypeObject *sha3_384_type;
    PyTypeObject *sha3_512_type;
#ifdef PY_WITH_KECCAK
    PyTypeObject *keccak_224_type;
    PyTypeObject *keccak_256_type;
    PyTypeObject *keccak_384_type;
    PyTypeObject *keccak_512_type;
#endif
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

/*[clinic input]
module _sha3
class _sha3.sha3_224 "SHA3object *" "&SHA3_224typ"
class _sha3.sha3_256 "SHA3object *" "&SHA3_256typ"
class _sha3.sha3_384 "SHA3object *" "&SHA3_384typ"
class _sha3.sha3_512 "SHA3object *" "&SHA3_512typ"
class _sha3.shake_128 "SHA3object *" "&SHAKE128type"
class _sha3.shake_256 "SHA3object *" "&SHAKE256type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b8a53680f370285a]*/

/* The structure for storing SHA3 info */

typedef struct {
    PyObject_HEAD
    SHA3_state hash_state;
    PyThread_type_lock lock;
} SHA3object;

#include "clinic/sha3module.c.h"

static SHA3object *
newSHA3object(PyTypeObject *type)
{
    SHA3object *newobj;
    newobj = (SHA3object *)PyObject_New(SHA3object, type);
    if (newobj == NULL) {
        return NULL;
    }
    newobj->lock = NULL;
    return newobj;
}

/*[clinic input]
@classmethod
_sha3.sha3_224.__new__ as py_sha3_new
    data: object(c_default="NULL") = b''
    /
    *
    usedforsecurity: bool = True

Return a new BLAKE2b hash object.
[clinic start generated code]*/

static PyObject *
py_sha3_new_impl(PyTypeObject *type, PyObject *data, int usedforsecurity)
/*[clinic end generated code: output=90409addc5d5e8b0 input=bcfcdf2e4368347a]*/
{
    SHA3object *self = newSHA3object(type);
    if (self == NULL) {
        goto error;
    }

    SHA3State *state = PyType_GetModuleState(type);
    assert(state != NULL);

    HashReturn res;
    if (type == state->sha3_224_type) {
        res = Keccak_HashInitialize_SHA3_224(&self->hash_state);
    } else if (type == state->sha3_256_type) {
        res = Keccak_HashInitialize_SHA3_256(&self->hash_state);
    } else if (type == state->sha3_384_type) {
        res = Keccak_HashInitialize_SHA3_384(&self->hash_state);
    } else if (type == state->sha3_512_type) {
        res = Keccak_HashInitialize_SHA3_512(&self->hash_state);
#ifdef PY_WITH_KECCAK
    } else if (type == state->keccak_224_type) {
        res = Keccak_HashInitialize(&self->hash_state, 1152, 448, 224, 0x01);
    } else if (type == state->keccak_256_type) {
        res = Keccak_HashInitialize(&self->hash_state, 1088, 512, 256, 0x01);
    } else if (type == state->keccak_384_type) {
        res = Keccak_HashInitialize(&self->hash_state, 832, 768, 384, 0x01);
    } else if (type == state->keccak_512_type) {
        res = Keccak_HashInitialize(&self->hash_state, 576, 1024, 512, 0x01);
#endif
    } else if (type == state->shake_128_type) {
        res = Keccak_HashInitialize_SHAKE128(&self->hash_state);
    } else if (type == state->shake_256_type) {
        res = Keccak_HashInitialize_SHAKE256(&self->hash_state);
    } else {
        PyErr_BadInternalCall();
        goto error;
    }

    Py_buffer buf = {NULL, NULL};
    if (data) {
        GET_BUFFER_VIEW_OR_ERROR(data, &buf, goto error);
        if (buf.len >= HASHLIB_GIL_MINSIZE) {
            /* invariant: New objects can't be accessed by other code yet,
             * thus it's safe to release the GIL without locking the object.
             */
            Py_BEGIN_ALLOW_THREADS
            res = SHA3_process(&self->hash_state, buf.buf, buf.len * 8);
            Py_END_ALLOW_THREADS
        }
        else {
            res = SHA3_process(&self->hash_state, buf.buf, buf.len * 8);
        }
        if (res != SUCCESS) {
            PyErr_SetString(PyExc_RuntimeError,
                            "internal error in SHA3 Update()");
            goto error;
        }
        PyBuffer_Release(&buf);
    }

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

static void
SHA3_dealloc(SHA3object *self)
{
    if (self->lock) {
        PyThread_free_lock(self->lock);
    }

    PyTypeObject *tp = Py_TYPE(self);
    PyObject_Free(self);
    Py_DECREF(tp);
}


/* External methods for a hash object */


/*[clinic input]
_sha3.sha3_224.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_sha3_sha3_224_copy_impl(SHA3object *self)
/*[clinic end generated code: output=6c537411ecdcda4c input=93a44aaebea51ba8]*/
{
    SHA3object *newobj;

    if ((newobj = newSHA3object(Py_TYPE(self))) == NULL) {
        return NULL;
    }
    ENTER_HASHLIB(self);
    SHA3_copystate(newobj->hash_state, self->hash_state);
    LEAVE_HASHLIB(self);
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
    unsigned char digest[SHA3_MAX_DIGESTSIZE + SHA3_LANESIZE];
    SHA3_state temp;
    HashReturn res;

    ENTER_HASHLIB(self);
    SHA3_copystate(temp, self->hash_state);
    LEAVE_HASHLIB(self);
    res = SHA3_done(&temp, digest);
    if (res != SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "internal error in SHA3 Final()");
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char *)digest,
                                      self->hash_state.fixedOutputLength / 8);
}


/*[clinic input]
_sha3.sha3_224.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_sha3_sha3_224_hexdigest_impl(SHA3object *self)
/*[clinic end generated code: output=75ad03257906918d input=2d91bb6e0d114ee3]*/
{
    unsigned char digest[SHA3_MAX_DIGESTSIZE + SHA3_LANESIZE];
    SHA3_state temp;
    HashReturn res;

    /* Get the raw (binary) digest value */
    ENTER_HASHLIB(self);
    SHA3_copystate(temp, self->hash_state);
    LEAVE_HASHLIB(self);
    res = SHA3_done(&temp, digest);
    if (res != SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "internal error in SHA3 Final()");
        return NULL;
    }
    return _Py_strhex((const char *)digest,
                      self->hash_state.fixedOutputLength / 8);
}


/*[clinic input]
_sha3.sha3_224.update

    data: object
    /

Update this hash object's state with the provided bytes-like object.
[clinic start generated code]*/

static PyObject *
_sha3_sha3_224_update(SHA3object *self, PyObject *data)
/*[clinic end generated code: output=d3223352286ed357 input=a887f54dcc4ae227]*/
{
    Py_buffer buf;
    HashReturn res;

    GET_BUFFER_VIEW_OR_ERROUT(data, &buf);

    /* add new data, the function takes the length in bits not bytes */
    if (self->lock == NULL && buf.len >= HASHLIB_GIL_MINSIZE) {
        self->lock = PyThread_allocate_lock();
    }
    /* Once a lock exists all code paths must be synchronized. We have to
     * release the GIL even for small buffers as acquiring the lock may take
     * an unlimited amount of time when another thread updates this object
     * with lots of data. */
    if (self->lock) {
        Py_BEGIN_ALLOW_THREADS
        PyThread_acquire_lock(self->lock, 1);
        res = SHA3_process(&self->hash_state, buf.buf, buf.len * 8);
        PyThread_release_lock(self->lock);
        Py_END_ALLOW_THREADS
    }
    else {
        res = SHA3_process(&self->hash_state, buf.buf, buf.len * 8);
    }

    if (res != SUCCESS) {
        PyBuffer_Release(&buf);
        PyErr_SetString(PyExc_RuntimeError,
                        "internal error in SHA3 Update()");
        return NULL;
    }

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
SHA3_get_block_size(SHA3object *self, void *closure)
{
    int rate = self->hash_state.sponge.rate;
    return PyLong_FromLong(rate / 8);
}


static PyObject *
SHA3_get_name(SHA3object *self, void *closure)
{
    PyTypeObject *type = Py_TYPE(self);

    SHA3State *state = PyType_GetModuleState(type);
    assert(state != NULL);

    if (type == state->sha3_224_type) {
        return PyUnicode_FromString("sha3_224");
    } else if (type == state->sha3_256_type) {
        return PyUnicode_FromString("sha3_256");
    } else if (type == state->sha3_384_type) {
        return PyUnicode_FromString("sha3_384");
    } else if (type == state->sha3_512_type) {
        return PyUnicode_FromString("sha3_512");
#ifdef PY_WITH_KECCAK
    } else if (type == state->keccak_224_type) {
        return PyUnicode_FromString("keccak_224");
    } else if (type == state->keccak_256_type) {
        return PyUnicode_FromString("keccak_256");
    } else if (type == state->keccak_384_type) {
        return PyUnicode_FromString("keccak_384");
    } else if (type == state->keccak_512_type) {
        return PyUnicode_FromString("keccak_512");
#endif
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
SHA3_get_digest_size(SHA3object *self, void *closure)
{
    return PyLong_FromLong(self->hash_state.fixedOutputLength / 8);
}


static PyObject *
SHA3_get_capacity_bits(SHA3object *self, void *closure)
{
    int capacity = 1600 - self->hash_state.sponge.rate;
    return PyLong_FromLong(capacity);
}


static PyObject *
SHA3_get_rate_bits(SHA3object *self, void *closure)
{
    unsigned int rate = self->hash_state.sponge.rate;
    return PyLong_FromLong(rate);
}

static PyObject *
SHA3_get_suffix(SHA3object *self, void *closure)
{
    unsigned char suffix[2];
    suffix[0] = self->hash_state.delimitedSuffix;
    suffix[1] = 0;
    return PyBytes_FromStringAndSize((const char *)suffix, 1);
}

static PyGetSetDef SHA3_getseters[] = {
    {"block_size", (getter)SHA3_get_block_size, NULL, NULL, NULL},
    {"name", (getter)SHA3_get_name, NULL, NULL, NULL},
    {"digest_size", (getter)SHA3_get_digest_size, NULL, NULL, NULL},
    {"_capacity_bits", (getter)SHA3_get_capacity_bits, NULL, NULL, NULL},
    {"_rate_bits", (getter)SHA3_get_rate_bits, NULL, NULL, NULL},
    {"_suffix", (getter)SHA3_get_suffix, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

#define SHA3_TYPE_SLOTS(type_slots_obj, type_doc, type_methods) \
    static PyType_Slot type_slots_obj[] = { \
        {Py_tp_dealloc, SHA3_dealloc}, \
        {Py_tp_doc, (char*)type_doc}, \
        {Py_tp_methods, type_methods}, \
        {Py_tp_getset, SHA3_getseters}, \
        {Py_tp_new, py_sha3_new}, \
        {0,0} \
    }

// Using PyType_GetModuleState() on these types is safe since they
// cannot be subclassed: it does not have the Py_TPFLAGS_BASETYPE flag.
#define SHA3_TYPE_SPEC(type_spec_obj, type_name, type_slots) \
    static PyType_Spec type_spec_obj = { \
        .name = "_sha3." type_name, \
        .basicsize = sizeof(SHA3object), \
        .flags = Py_TPFLAGS_DEFAULT, \
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

#ifdef PY_WITH_KECCAK
PyDoc_STRVAR(keccak_224__doc__,
"keccak_224([data], *, usedforsecurity=True) -> Keccak object\n\
\n\
Return a new Keccak hash object with a hashbit length of 28 bytes.");

PyDoc_STRVAR(keccak_256__doc__,
"keccak_256([data], *, usedforsecurity=True) -> Keccak object\n\
\n\
Return a new Keccak hash object with a hashbit length of 32 bytes.");

PyDoc_STRVAR(keccak_384__doc__,
"keccak_384([data], *, usedforsecurity=True) -> Keccak object\n\
\n\
Return a new Keccak hash object with a hashbit length of 48 bytes.");

PyDoc_STRVAR(keccak_512__doc__,
"keccak_512([data], *, usedforsecurity=True) -> Keccak object\n\
\n\
Return a new Keccak hash object with a hashbit length of 64 bytes.");

#endif

SHA3_TYPE_SLOTS(sha3_224_slots, sha3_224__doc__, SHA3_methods);
SHA3_TYPE_SPEC(sha3_224_spec, "sha3_224", sha3_224_slots);

SHA3_TYPE_SLOTS(sha3_256_slots, sha3_256__doc__, SHA3_methods);
SHA3_TYPE_SPEC(sha3_256_spec, "sha3_256", sha3_256_slots);

SHA3_TYPE_SLOTS(sha3_384_slots, sha3_384__doc__, SHA3_methods);
SHA3_TYPE_SPEC(sha3_384_spec, "sha3_384", sha3_384_slots);

SHA3_TYPE_SLOTS(sha3_512_slots, sha3_512__doc__, SHA3_methods);
SHA3_TYPE_SPEC(sha3_512_spec, "sha3_512", sha3_512_slots);

#ifdef PY_WITH_KECCAK
SHA3_TYPE_SLOTS(Keccak_224_slots, keccak_224__doc__, SHA3_methods);
SHA3_TYPE_SPEC(Keccak_224_spec, "keccak_224", Keccak_224_slots);

SHA3_TYPE_SLOTS(Keccak_256_slots, keccak_256__doc__, SHA3_methods);
SHA3_TYPE_SPEC(Keccak_256_spec, "keccak_256", Keccak_256_slots);

SHA3_TYPE_SLOTS(Keccak_384_slots, keccak_384__doc__, SHA3_methods);
SHA3_TYPE_SPEC(Keccak_384_spec, "keccak_384", Keccak_384_slots);

SHA3_TYPE_SLOTS(Keccak_512_slots, keccak_512__doc__, SHA3_methods);
SHA3_TYPE_SPEC(Keccak_512_spec, "keccak_512", Keccak_512_slots);
#endif


static PyObject *
_SHAKE_digest(SHA3object *self, unsigned long digestlen, int hex)
{
    unsigned char *digest = NULL;
    SHA3_state temp;
    int res;
    PyObject *result = NULL;

    if (digestlen >= (1 << 29)) {
        PyErr_SetString(PyExc_ValueError, "length is too large");
        return NULL;
    }
    /* ExtractLane needs at least SHA3_MAX_DIGESTSIZE + SHA3_LANESIZE and
     * SHA3_LANESIZE extra space.
     */
    digest = (unsigned char*)PyMem_Malloc(digestlen + SHA3_LANESIZE);
    if (digest == NULL) {
        return PyErr_NoMemory();
    }

    /* Get the raw (binary) digest value */
    ENTER_HASHLIB(self);
    SHA3_copystate(temp, self->hash_state);
    LEAVE_HASHLIB(self);
    res = SHA3_done(&temp, NULL);
    if (res != SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "internal error in SHA3 done()");
        goto error;
    }
    res = SHA3_squeeze(&temp, digest, digestlen * 8);
    if (res != SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "internal error in SHA3 Squeeze()");
        return NULL;
    }
    if (hex) {
         result = _Py_strhex((const char *)digest, digestlen);
    } else {
        result = PyBytes_FromStringAndSize((const char *)digest,
                                           digestlen);
    }
  error:
    if (digest != NULL) {
        PyMem_Free(digest);
    }
    return result;
}


/*[clinic input]
_sha3.shake_128.digest

    length: unsigned_long
    /

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_sha3_shake_128_digest_impl(SHA3object *self, unsigned long length)
/*[clinic end generated code: output=2313605e2f87bb8f input=418ef6a36d2e6082]*/
{
    return _SHAKE_digest(self, length, 0);
}


/*[clinic input]
_sha3.shake_128.hexdigest

    length: unsigned_long
    /

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_sha3_shake_128_hexdigest_impl(SHA3object *self, unsigned long length)
/*[clinic end generated code: output=bf8e2f1e490944a8 input=69fb29b0926ae321]*/
{
    return _SHAKE_digest(self, length, 1);
}


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

SHA3_TYPE_SLOTS(SHAKE128slots, shake_128__doc__, SHAKE_methods);
SHA3_TYPE_SPEC(SHAKE128_spec, "shake_128", SHAKE128slots);

SHA3_TYPE_SLOTS(SHAKE256slots, shake_256__doc__, SHAKE_methods);
SHA3_TYPE_SPEC(SHAKE256_spec, "shake_256", SHAKE256slots);


static int
_sha3_traverse(PyObject *module, visitproc visit, void *arg)
{
    SHA3State *state = sha3_get_state(module);
    Py_VISIT(state->sha3_224_type);
    Py_VISIT(state->sha3_256_type);
    Py_VISIT(state->sha3_384_type);
    Py_VISIT(state->sha3_512_type);
#ifdef PY_WITH_KECCAK
    Py_VISIT(state->keccak_224_type);
    Py_VISIT(state->keccak_256_type);
    Py_VISIT(state->keccak_384_type);
    Py_VISIT(state->keccak_512_type);
#endif
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
#ifdef PY_WITH_KECCAK
    Py_CLEAR(state->keccak_224_type);
    Py_CLEAR(state->keccak_256_type);
    Py_CLEAR(state->keccak_384_type);
    Py_CLEAR(state->keccak_512_type);
#endif
    Py_CLEAR(state->shake_128_type);
    Py_CLEAR(state->shake_256_type);
    return 0;
}

static void
_sha3_free(void *module)
{
    _sha3_clear((PyObject *)module);
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
#ifdef PY_WITH_KECCAK
    init_sha3type(keccak_224_type, Keccak_224_spec);
    init_sha3type(keccak_256_type, Keccak_256_spec);
    init_sha3type(keccak_384_type, Keccak_384_spec);
    init_sha3type(keccak_512_type, Keccak_512_spec);
#endif
    init_sha3type(shake_128_type, SHAKE128_spec);
    init_sha3type(shake_256_type, SHAKE256_spec);
#undef init_sha3type

    if (PyModule_AddIntConstant(m, "keccakopt", KeccakOpt) < 0) {
        return -1;
    }
    if (PyModule_AddStringConstant(m, "implementation",
                                   KeccakP1600_implementation) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot _sha3_slots[] = {
    {Py_mod_exec, _sha3_exec},
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
