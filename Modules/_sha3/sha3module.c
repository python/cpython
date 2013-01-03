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
 *  Copyright (C) 2012   Christian Heimes (christian@python.org)
 *  Licensed to PSF under a Contributor Agreement.
 *
 */

#include "Python.h"
#include "../hashlib.h"

/* **************************************************************************
 *                             SHA-3 (Keccak)
 *
 * The code is based on KeccakReferenceAndOptimized-3.2.zip from 29 May 2012.
 *
 * The reference implementation is altered in this points:
 *  - C++ comments are converted to ANSI C comments.
 *  - All functions and globals are declared static.
 *  - The typedef for UINT64 is commented out.
 *  - KeccakF-1600-opt[32|64]-settings.h are commented out
 *  - Some unused functions are commented out to silence compiler warnings.
 *
 * In order to avoid name clashes with other software I have to declare all
 * Keccak functions and global data as static. The C code is directly
 * included into this file in order to access the static functions.
 *
 * Keccak can be tuned with several paramenters. I try to explain all options
 * as far as I understand them. The reference implementation also contains
 * assembler code for ARM platforms (NEON instructions).
 *
 * Common
 * ======
 *
 * Options:
 *   UseBebigokimisa, Unrolling
 *
 * - Unrolling: loop unrolling (24, 12, 8, 6, 4, 3, 2, 1)
 * - UseBebigokimisa: lane complementing
 *
 * 64bit platforms
 * ===============
 *
 * Additional options:
 *   UseSSE, UseOnlySIMD64, UseMMX, UseXOP, UseSHLD
 *
 * Optimized instructions (disabled by default):
 *   - UseSSE: use Stream SIMD extensions
 *     o UseOnlySIMD64: limit to 64bit instructions, otherwise 128bit
 *     o w/o UseOnlySIMD64: requires compiler agument -mssse3 or -mtune
 *   - UseMMX: use 64bit MMX instructions
 *   - UseXOP: use AMD's eXtended Operations (128bit SSE extension)
 *
 * Other:
 *   - Unrolling: default 24
 *   - UseBebigokimisa: default 1
 *
 * When neither UseSSE, UseMMX nor UseXOP is configured, ROL64 (rotate left
 * 64) is implemented as:
 *   - Windows: _rotl64()
 *   - UseSHLD: use shld (shift left) asm optimization
 *   - otherwise: shift and xor
 *
 * UseBebigokimisa can't be used in combination with UseSSE, UseMMX or
 * UseXOP. UseOnlySIMD64 has no effect unless UseSSE is specified.
 *
 * Tests have shown that UseSSE + UseOnlySIMD64 is about three to four
 * times SLOWER than UseBebigokimisa. UseSSE and UseMMX are about two times
 * slower. (tested by CH and AP)
 *
 * 32bit platforms
 * ===============
 *
 * Additional options:
 *   UseInterleaveTables, UseSchedule
 *
 *   - Unrolling: default 2
 *   - UseBebigokimisa: default n/a
 *   - UseSchedule: ???, (1, 2, 3; default 3)
 *   - UseInterleaveTables: use two 64k lookup tables for (de)interleaving
 *     default: n/a
 *
 * schedules:
 *   - 3: no UseBebigokimisa, Unrolling must be 2
 *   - 2 + 1: ???
 *
 * *************************************************************************/

#ifdef __sparc
  /* opt64 uses un-aligned memory access that causes a BUS error with msg
   * 'invalid address alignment' on SPARC. */
  #define KeccakOpt 32
#elif SIZEOF_VOID_P == 8 && defined(PY_UINT64_T)
  /* opt64 works only for 64bit platforms with unsigned int64 */
  #define KeccakOpt 64
#else
  /* opt32 is used for the remaining 32 and 64bit platforms */
  #define KeccakOpt 32
#endif

#if KeccakOpt == 64 && defined(PY_UINT64_T)
  /* 64bit platforms with unsigned int64 */
  #define Unrolling 24
  #define UseBebigokimisa
  typedef PY_UINT64_T UINT64;
#elif KeccakOpt == 32 && defined(PY_UINT64_T)
  /* 32bit platforms with unsigned int64 */
  #define Unrolling 2
  #define UseSchedule 3
  typedef PY_UINT64_T UINT64;
#else
  /* 32 or 64bit platforms without unsigned int64 */
  #define Unrolling 2
  #define UseSchedule 3
  #define UseInterleaveTables
#endif

/* replacement for brg_endian.h */
#define IS_BIG_ENDIAN 4321
#define IS_LITTLE_ENDIAN 1234
#if PY_BIG_ENDIAN
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#else
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

/* inline all Keccak dependencies */
#include "keccak/KeccakNISTInterface.h"
#include "keccak/KeccakNISTInterface.c"
#include "keccak/KeccakSponge.c"
#if KeccakOpt == 64
  #include "keccak/KeccakF-1600-opt64.c"
#elif KeccakOpt == 32
  #include "keccak/KeccakF-1600-opt32.c"
#endif

/* #define SHA3_BLOCKSIZE 200 // 1600 bits  */
#define SHA3_MAX_DIGESTSIZE 64 /* 512 bits */
#define SHA3_state hashState
#define SHA3_init Init
#define SHA3_process Update
#define SHA3_done Final
#define SHA3_copystate(dest, src) memcpy(&(dest), &(src), sizeof(SHA3_state))
#define SHA3_clearstate(state) memset(&(state), 0, sizeof(SHA3_state))

/* The structure for storing SHA3 info */

typedef struct {
    PyObject_HEAD
    int hashbitlen;
    SHA3_state hash_state;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif

} SHA3object;

static PyTypeObject SHA3type;


static SHA3object *
newSHA3object(int hashbitlen)
{
    SHA3object *newobj;

    /* check hashbitlen */
    switch(hashbitlen) {
        /* supported hash length */
        case 224:
            break;
        case 256:
            break;
        case 384:
            break;
        case 512:
            break;
        case 0:
            /*  arbitrarily-long output isn't supported by this module */
        default:
            /* everything else is an error */
            PyErr_SetString(PyExc_ValueError,
                    "hashbitlen must be one of 224, 256, 384 or 512.");
            return NULL;
    }
    newobj = (SHA3object *)PyObject_New(SHA3object, &SHA3type);
    if (newobj == NULL) {
        return NULL;
    }
    newobj->hashbitlen = hashbitlen;
#ifdef WITH_THREAD
    newobj->lock = NULL;
#endif
    return newobj;
}


/* Internal methods for a hash object */

static void
SHA3_dealloc(SHA3object *self)
{
    SHA3_clearstate(self->hash_state);
#ifdef WITH_THREAD
    if (self->lock) {
        PyThread_free_lock(self->lock);
    }
#endif
    PyObject_Del(self);
}


/* External methods for a hash object */

PyDoc_STRVAR(SHA3_copy__doc__, "Return a copy of the hash object.");

static PyObject *
SHA3_copy(SHA3object *self, PyObject *unused)
{
    SHA3object *newobj;

    if ((newobj = newSHA3object(self->hashbitlen)) == NULL) {
        return NULL;
    }
    ENTER_HASHLIB(self);
    SHA3_copystate(newobj->hash_state, self->hash_state);
    LEAVE_HASHLIB(self);
    return (PyObject *)newobj;
}


PyDoc_STRVAR(SHA3_digest__doc__,
"Return the digest value as a string of binary data.");

static PyObject *
SHA3_digest(SHA3object *self, PyObject *unused)
{
    unsigned char digest[SHA3_MAX_DIGESTSIZE];
    SHA3_state temp;
    HashReturn res;

    ENTER_HASHLIB(self);
    SHA3_copystate(temp, self->hash_state);
    LEAVE_HASHLIB(self);
    res = SHA3_done(&temp, digest);
    SHA3_clearstate(temp);
    if (res != SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "internal error in SHA3 Final()");
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char *)digest,
                                      self->hashbitlen / 8);
}


PyDoc_STRVAR(SHA3_hexdigest__doc__,
"Return the digest value as a string of hexadecimal digits.");

static PyObject *
SHA3_hexdigest(SHA3object *self, PyObject *unused)
{
    unsigned char digest[SHA3_MAX_DIGESTSIZE];
    SHA3_state temp;
    HashReturn res;
    PyObject *retval;
    Py_UCS1 *hex_digest;
    int digestlen, i, j;

    /* Get the raw (binary) digest value */
    ENTER_HASHLIB(self);
    SHA3_copystate(temp, self->hash_state);
    LEAVE_HASHLIB(self);
    res = SHA3_done(&temp, digest);
    SHA3_clearstate(temp);
    if (res != SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "internal error in SHA3 Final()");
        return NULL;
    }

    /* Create a new string */
    digestlen = self->hashbitlen / 8;
    retval = PyUnicode_New(digestlen * 2, 127);
    if (!retval)
            return NULL;
    hex_digest = PyUnicode_1BYTE_DATA(retval);

    /* Make hex version of the digest */
    for(i=j=0; i < digestlen; i++) {
        unsigned char c;
        c = (digest[i] >> 4) & 0xf;
        hex_digest[j++] = Py_hexdigits[c];
        c = (digest[i] & 0xf);
        hex_digest[j++] = Py_hexdigits[c];
    }
#ifdef Py_DEBUG
    assert(_PyUnicode_CheckConsistency(retval, 1));
#endif
    return retval;
}

PyDoc_STRVAR(SHA3_update__doc__,
"Update this hash object's state with the provided string.");

static PyObject *
SHA3_update(SHA3object *self, PyObject *args)
{
    PyObject *obj;
    Py_buffer buf;
    HashReturn res;

    if (!PyArg_ParseTuple(args, "O:update", &obj))
        return NULL;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);

    /* add new data, the function takes the length in bits not bytes */
#ifdef WITH_THREADS
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
#else
    res = SHA3_process(&self->hash_state, buf.buf, buf.len * 8);
#endif
    LEAVE_HASHLIB(self);

    if (res != SUCCESS) {
        PyBuffer_Release(&buf);
        PyErr_SetString(PyExc_RuntimeError,
                        "internal error in SHA3 Update()");
        return NULL;
    }

    PyBuffer_Release(&buf);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef SHA3_methods[] = {
    {"copy",      (PyCFunction)SHA3_copy,      METH_NOARGS,
         SHA3_copy__doc__},
    {"digest",    (PyCFunction)SHA3_digest,    METH_NOARGS,
         SHA3_digest__doc__},
    {"hexdigest", (PyCFunction)SHA3_hexdigest, METH_NOARGS,
         SHA3_hexdigest__doc__},
    {"update",    (PyCFunction)SHA3_update,    METH_VARARGS,
         SHA3_update__doc__},
    {NULL,        NULL}         /* sentinel */
};

static PyObject *
SHA3_get_block_size(SHA3object *self, void *closure)
{
    /* HMAC-SHA3 hasn't been specified yet and no official test vectors are
     * available. Thus block_size returns NotImplemented to prevent people
     * from using SHA3 with the hmac module.
     */
    Py_RETURN_NOTIMPLEMENTED;
}

static PyObject *
SHA3_get_name(SHA3object *self, void *closure)
{
    return PyUnicode_FromFormat("sha3_%i", self->hashbitlen);
}

static PyObject *
SHA3_get_digest_size(SHA3object *self, void *closure)
{
    return PyLong_FromLong(self->hashbitlen / 8);
}


static PyGetSetDef SHA3_getseters[] = {
    {"block_size", (getter)SHA3_get_block_size, NULL, NULL, NULL},
    {"name", (getter)SHA3_get_name, NULL, NULL, NULL},
    {"digest_size", (getter)SHA3_get_digest_size, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

static PyTypeObject SHA3type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_sha3.SHA3",       /* tp_name */
    sizeof(SHA3object), /* tp_size */
    0,                  /* tp_itemsize */
    /*  methods  */
    (destructor)SHA3_dealloc, /* tp_dealloc */
    0,                  /* tp_print */
    0,                  /* tp_getattr */
    0,                  /* tp_setattr */
    0,                  /* tp_reserved */
    0,                  /* tp_repr */
    0,                  /* tp_as_number */
    0,                  /* tp_as_sequence */
    0,                  /* tp_as_mapping */
    0,                  /* tp_hash */
    0,                  /* tp_call */
    0,                  /* tp_str */
    0,                  /* tp_getattro */
    0,                  /* tp_setattro */
    0,                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    0,                  /* tp_doc */
    0,                  /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    0,                  /* tp_weaklistoffset */
    0,                  /* tp_iter */
    0,                  /* tp_iternext */
    SHA3_methods,       /* tp_methods */
    NULL,               /* tp_members */
    SHA3_getseters,     /* tp_getset */
};


/* constructor helper */
static PyObject *
SHA3_factory(PyObject *args, PyObject *kwdict, const char *fmt,
             int hashbitlen)
{
    SHA3object *newobj = NULL;
    static char *kwlist[] = {"string", NULL};
    PyObject *data_obj = NULL;
    Py_buffer buf;
    HashReturn res;

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, fmt, kwlist,
                                     &data_obj)) {
        return NULL;
    }

    if (data_obj)
        GET_BUFFER_VIEW_OR_ERROUT(data_obj, &buf);

    if ((newobj = newSHA3object(hashbitlen)) == NULL) {
        goto error;
    }

    if (SHA3_init(&newobj->hash_state, hashbitlen) != SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError,
                        "internal error in SHA3 Update()");
        goto error;
    }

    if (data_obj) {
#ifdef WITH_THREADS
        if (buf.len >= HASHLIB_GIL_MINSIZE) {
            /* invariant: New objects can't be accessed by other code yet,
             * thus it's safe to release the GIL without locking the object.
             */
            Py_BEGIN_ALLOW_THREADS
            res = SHA3_process(&newobj->hash_state, buf.buf, buf.len * 8);
            Py_END_ALLOW_THREADS
        }
        else {
            res = SHA3_process(&newobj->hash_state, buf.buf, buf.len * 8);
        }
#else
        res = SHA3_process(&newobj->hash_state, buf.buf, buf.len * 8);
#endif
        if (res != SUCCESS) {
            PyErr_SetString(PyExc_RuntimeError,
                            "internal error in SHA3 Update()");
            goto error;
        }
        PyBuffer_Release(&buf);
    }

    return (PyObject *)newobj;

  error:
    if (newobj) {
        SHA3_dealloc(newobj);
    }
    if (data_obj) {
        PyBuffer_Release(&buf);
    }
    return NULL;

}

PyDoc_STRVAR(sha3_224__doc__,
"sha3_224([string]) -> SHA3 object\n\
\n\
Return a new SHA3 hash object with a hashbit length of 28 bytes.");

static PyObject *
sha3_224(PyObject *self, PyObject *args, PyObject *kwdict)
{
    return SHA3_factory(args, kwdict, "|O:sha3_224", 224);
}


PyDoc_STRVAR(sha3_256__doc__,
"sha3_256([string]) -> SHA3 object\n\
\n\
Return a new SHA3 hash object with a hashbit length of 32 bytes.");

static PyObject *
sha3_256(PyObject *self, PyObject *args, PyObject *kwdict)
{
    return SHA3_factory(args, kwdict, "|O:sha3_256", 256);
}

PyDoc_STRVAR(sha3_384__doc__,
"sha3_384([string]) -> SHA3 object\n\
\n\
Return a new SHA3 hash object with a hashbit length of 48 bytes.");

static PyObject *
sha3_384(PyObject *self, PyObject *args, PyObject *kwdict)
{
    return SHA3_factory(args, kwdict, "|O:sha3_384", 384);
}

PyDoc_STRVAR(sha3_512__doc__,
"sha3_512([string]) -> SHA3 object\n\
\n\
Return a new SHA3 hash object with a hashbit length of 64 bytes.");

static PyObject *
sha3_512(PyObject *self, PyObject *args, PyObject *kwdict)
{
    return SHA3_factory(args, kwdict, "|O:sha3_512", 512);
}


/* List of functions exported by this module */
static struct PyMethodDef SHA3_functions[] = {
    {"sha3_224", (PyCFunction)sha3_224, METH_VARARGS|METH_KEYWORDS,
         sha3_224__doc__},
    {"sha3_256", (PyCFunction)sha3_256, METH_VARARGS|METH_KEYWORDS,
         sha3_256__doc__},
    {"sha3_384", (PyCFunction)sha3_384, METH_VARARGS|METH_KEYWORDS,
         sha3_384__doc__},
    {"sha3_512", (PyCFunction)sha3_512, METH_VARARGS|METH_KEYWORDS,
         sha3_512__doc__},
    {NULL,      NULL}            /* Sentinel */
};


/* Initialize this module. */
static struct PyModuleDef _SHA3module = {
        PyModuleDef_HEAD_INIT,
        "_sha3",
        NULL,
        -1,
        SHA3_functions,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__sha3(void)
{
    Py_TYPE(&SHA3type) = &PyType_Type;
    if (PyType_Ready(&SHA3type) < 0) {
        return NULL;
    }

    return PyModule_Create(&_SHA3module);
}
