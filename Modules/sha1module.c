/* SHA1 module */

/* This module provides an interface to the SHA1 algorithm */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)
   Trevor Perrin (trevp@trevp.net)

   Copyright (C) 2005-2007   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

/* SHA1 objects */

#include "Python.h"
#include "hashlib.h"
#include "pystrhex.h"

/*[clinic input]
module _sha1
class SHA1Type "SHA1object *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3dc9a20d1becb759]*/

/* Some useful types */

#if SIZEOF_INT == 4
typedef unsigned int SHA1_INT32;        /* 32-bit integer */
typedef long long SHA1_INT64;        /* 64-bit integer */
#else
/* not defined. compilation will die. */
#endif

/* The SHA1 block size and message digest sizes, in bytes */

#define SHA1_BLOCKSIZE    64
#define SHA1_DIGESTSIZE   20

/* The structure for storing SHA1 info */

struct sha1_state {
    SHA1_INT64 length;
    SHA1_INT32 state[5], curlen;
    unsigned char buf[SHA1_BLOCKSIZE];
};

typedef struct {
    PyObject_HEAD

    struct sha1_state hash_state;
} SHA1object;

#include "clinic/sha1module.c.h"

/* ------------------------------------------------------------------------
 *
 * This code for the SHA1 algorithm was noted as public domain. The
 * original headers are pasted below.
 *
 * Several changes have been made to make it more compatible with the
 * Python environment and desired interface.
 *
 */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* rotate the hard way (platform optimizations could be done) */
#define ROL(x, y) ( (((unsigned long)(x)<<(unsigned long)((y)&31)) | (((unsigned long)(x)&0xFFFFFFFFUL)>>(unsigned long)(32-((y)&31)))) & 0xFFFFFFFFUL)
#define ROLc(x, y) ( (((unsigned long)(x)<<(unsigned long)((y)&31)) | (((unsigned long)(x)&0xFFFFFFFFUL)>>(unsigned long)(32-((y)&31)))) & 0xFFFFFFFFUL)

/* Endian Neutral macros that work on all platforms */

#define STORE32H(x, y)                                                                     \
     { (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned char)(((x)>>16)&255);   \
       (y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned char)((x)&255); }

#define LOAD32H(x, y)                            \
     { x = ((unsigned long)((y)[0] & 255)<<24) | \
           ((unsigned long)((y)[1] & 255)<<16) | \
           ((unsigned long)((y)[2] & 255)<<8)  | \
           ((unsigned long)((y)[3] & 255)); }

#define STORE64H(x, y)                                                                     \
   { (y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned char)(((x)>>48)&255);     \
     (y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned char)(((x)>>32)&255);     \
     (y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned char)(((x)>>16)&255);     \
     (y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); }


/* SHA1 macros */

#define F0(x,y,z)  (z ^ (x & (y ^ z)))
#define F1(x,y,z)  (x ^ y ^ z)
#define F2(x,y,z)  ((x & y) | (z & (x | y)))
#define F3(x,y,z)  (x ^ y ^ z)

static void sha1_compress(struct sha1_state *sha1, unsigned char *buf)
{
    SHA1_INT32 a,b,c,d,e,W[80],i;

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32H(W[i], buf + (4*i));
    }

    /* copy state */
    a = sha1->state[0];
    b = sha1->state[1];
    c = sha1->state[2];
    d = sha1->state[3];
    e = sha1->state[4];

    /* expand it */
    for (i = 16; i < 80; i++) {
        W[i] = ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }

    /* compress */
    /* round one */
    #define FF_0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
    #define FF_1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
    #define FF_2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
    #define FF_3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);

    for (i = 0; i < 20; ) {
       FF_0(a,b,c,d,e,i++);
       FF_0(e,a,b,c,d,i++);
       FF_0(d,e,a,b,c,i++);
       FF_0(c,d,e,a,b,i++);
       FF_0(b,c,d,e,a,i++);
    }

    /* round two */
    for (; i < 40; )  {
       FF_1(a,b,c,d,e,i++);
       FF_1(e,a,b,c,d,i++);
       FF_1(d,e,a,b,c,i++);
       FF_1(c,d,e,a,b,i++);
       FF_1(b,c,d,e,a,i++);
    }

    /* round three */
    for (; i < 60; )  {
       FF_2(a,b,c,d,e,i++);
       FF_2(e,a,b,c,d,i++);
       FF_2(d,e,a,b,c,i++);
       FF_2(c,d,e,a,b,i++);
       FF_2(b,c,d,e,a,i++);
    }

    /* round four */
    for (; i < 80; )  {
       FF_3(a,b,c,d,e,i++);
       FF_3(e,a,b,c,d,i++);
       FF_3(d,e,a,b,c,i++);
       FF_3(c,d,e,a,b,i++);
       FF_3(b,c,d,e,a,i++);
    }

    #undef FF_0
    #undef FF_1
    #undef FF_2
    #undef FF_3

    /* store */
    sha1->state[0] = sha1->state[0] + a;
    sha1->state[1] = sha1->state[1] + b;
    sha1->state[2] = sha1->state[2] + c;
    sha1->state[3] = sha1->state[3] + d;
    sha1->state[4] = sha1->state[4] + e;
}

/**
   Initialize the hash state
   @param sha1   The hash state you wish to initialize
*/
static void
sha1_init(struct sha1_state *sha1)
{
   assert(sha1 != NULL);
   sha1->state[0] = 0x67452301UL;
   sha1->state[1] = 0xefcdab89UL;
   sha1->state[2] = 0x98badcfeUL;
   sha1->state[3] = 0x10325476UL;
   sha1->state[4] = 0xc3d2e1f0UL;
   sha1->curlen = 0;
   sha1->length = 0;
}

/**
   Process a block of memory though the hash
   @param sha1   The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
*/
static void
sha1_process(struct sha1_state *sha1,
                  const unsigned char *in, Py_ssize_t inlen)
{
    Py_ssize_t n;

    assert(sha1 != NULL);
    assert(in != NULL);
    assert(sha1->curlen <= sizeof(sha1->buf));

    while (inlen > 0) {
        if (sha1->curlen == 0 && inlen >= SHA1_BLOCKSIZE) {
           sha1_compress(sha1, (unsigned char *)in);
           sha1->length   += SHA1_BLOCKSIZE * 8;
           in             += SHA1_BLOCKSIZE;
           inlen          -= SHA1_BLOCKSIZE;
        } else {
           n = Py_MIN(inlen, (Py_ssize_t)(SHA1_BLOCKSIZE - sha1->curlen));
           memcpy(sha1->buf + sha1->curlen, in, (size_t)n);
           sha1->curlen   += (SHA1_INT32)n;
           in             += n;
           inlen          -= n;
           if (sha1->curlen == SHA1_BLOCKSIZE) {
              sha1_compress(sha1, sha1->buf);
              sha1->length += 8*SHA1_BLOCKSIZE;
              sha1->curlen = 0;
           }
       }
    }
}

/**
   Terminate the hash to get the digest
   @param sha1  The hash state
   @param out [out] The destination of the hash (20 bytes)
*/
static void
sha1_done(struct sha1_state *sha1, unsigned char *out)
{
    int i;

    assert(sha1 != NULL);
    assert(out != NULL);
    assert(sha1->curlen < sizeof(sha1->buf));

    /* increase the length of the message */
    sha1->length += sha1->curlen * 8;

    /* append the '1' bit */
    sha1->buf[sha1->curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (sha1->curlen > 56) {
        while (sha1->curlen < 64) {
            sha1->buf[sha1->curlen++] = (unsigned char)0;
        }
        sha1_compress(sha1, sha1->buf);
        sha1->curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (sha1->curlen < 56) {
        sha1->buf[sha1->curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(sha1->length, sha1->buf+56);
    sha1_compress(sha1, sha1->buf);

    /* copy output */
    for (i = 0; i < 5; i++) {
        STORE32H(sha1->state[i], out+(4*i));
    }
}


/* .Source: /cvs/libtom/libtomcrypt/src/hashes/sha1.c,v $ */
/* .Revision: 1.10 $ */
/* .Date: 2007/05/12 14:25:28 $ */

/*
 * End of copied SHA1 code.
 *
 * ------------------------------------------------------------------------
 */

typedef struct {
    PyTypeObject* sha1_type;
} SHA1State;

static inline SHA1State*
sha1_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (SHA1State *)state;
}

static SHA1object *
newSHA1object(SHA1State *st)
{
    return (SHA1object *)PyObject_New(SHA1object, st->sha1_type);
}


/* Internal methods for a hash object */

static void
SHA1_dealloc(PyObject *ptr)
{
    PyTypeObject *tp = Py_TYPE(ptr);
    PyObject_Del(ptr);
    Py_DECREF(tp);
}


/* External methods for a hash object */

/*[clinic input]
SHA1Type.copy

    cls: defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
SHA1Type_copy_impl(SHA1object *self, PyTypeObject *cls)
/*[clinic end generated code: output=b32d4461ce8bc7a7 input=6c22e66fcc34c58e]*/
{
    SHA1State *st = PyType_GetModuleState(cls);

    SHA1object *newobj;
    if ((newobj = newSHA1object(st)) == NULL)
        return NULL;

    newobj->hash_state = self->hash_state;
    return (PyObject *)newobj;
}

/*[clinic input]
SHA1Type.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
SHA1Type_digest_impl(SHA1object *self)
/*[clinic end generated code: output=2f05302a7aa2b5cb input=13824b35407444bd]*/
{
    unsigned char digest[SHA1_DIGESTSIZE];
    struct sha1_state temp;

    temp = self->hash_state;
    sha1_done(&temp, digest);
    return PyBytes_FromStringAndSize((const char *)digest, SHA1_DIGESTSIZE);
}

/*[clinic input]
SHA1Type.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
SHA1Type_hexdigest_impl(SHA1object *self)
/*[clinic end generated code: output=4161fd71e68c6659 input=97691055c0c74ab0]*/
{
    unsigned char digest[SHA1_DIGESTSIZE];
    struct sha1_state temp;

    /* Get the raw (binary) digest value */
    temp = self->hash_state;
    sha1_done(&temp, digest);

    return _Py_strhex((const char *)digest, SHA1_DIGESTSIZE);
}

/*[clinic input]
SHA1Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
SHA1Type_update(SHA1object *self, PyObject *obj)
/*[clinic end generated code: output=d9902f0e5015e9ae input=aad8e07812edbba3]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);

    sha1_process(&self->hash_state, buf.buf, buf.len);

    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static PyMethodDef SHA1_methods[] = {
    SHA1TYPE_COPY_METHODDEF
    SHA1TYPE_DIGEST_METHODDEF
    SHA1TYPE_HEXDIGEST_METHODDEF
    SHA1TYPE_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

static PyObject *
SHA1_get_block_size(PyObject *self, void *closure)
{
    return PyLong_FromLong(SHA1_BLOCKSIZE);
}

static PyObject *
SHA1_get_name(PyObject *self, void *closure)
{
    return PyUnicode_FromStringAndSize("sha1", 4);
}

static PyObject *
sha1_get_digest_size(PyObject *self, void *closure)
{
    return PyLong_FromLong(SHA1_DIGESTSIZE);
}

static PyGetSetDef SHA1_getseters[] = {
    {"block_size",
     (getter)SHA1_get_block_size, NULL,
     NULL,
     NULL},
    {"name",
     (getter)SHA1_get_name, NULL,
     NULL,
     NULL},
    {"digest_size",
     (getter)sha1_get_digest_size, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};

static PyType_Slot sha1_type_slots[] = {
    {Py_tp_dealloc, SHA1_dealloc},
    {Py_tp_methods, SHA1_methods},
    {Py_tp_getset, SHA1_getseters},
    {0,0}
};

static PyType_Spec sha1_type_spec = {
    .name = "_sha1.sha1",
    .basicsize =  sizeof(SHA1object),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = sha1_type_slots
};

/* The single module-level function: new() */

/*[clinic input]
_sha1.sha1

    string: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True

Return a new SHA1 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha1_sha1_impl(PyObject *module, PyObject *string, int usedforsecurity)
/*[clinic end generated code: output=6f8b3af05126e18e input=bd54b68e2bf36a8a]*/
{
    SHA1object *new;
    Py_buffer buf;

    if (string)
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);

    SHA1State *st = sha1_get_state(module);
    if ((new = newSHA1object(st)) == NULL) {
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }

    sha1_init(&new->hash_state);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }
    if (string) {
        sha1_process(&new->hash_state, buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef SHA1_functions[] = {
    _SHA1_SHA1_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};

static int
_sha1_traverse(PyObject *module, visitproc visit, void *arg)
{
    SHA1State *state = sha1_get_state(module);
    Py_VISIT(state->sha1_type);
    return 0;
}

static int
_sha1_clear(PyObject *module)
{
    SHA1State *state = sha1_get_state(module);
    Py_CLEAR(state->sha1_type);
    return 0;
}

static void
_sha1_free(void *module)
{
    _sha1_clear((PyObject *)module);
}

static int
_sha1_exec(PyObject *module)
{
    SHA1State* st = sha1_get_state(module);

    st->sha1_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &sha1_type_spec, NULL);

    if (st->sha1_type == NULL) {
        return -1;
    }

    Py_INCREF(st->sha1_type);
    if (PyModule_AddObject(module, 
                           "SHA1Type",
                           (PyObject *)st->sha1_type) < 0) {
        Py_DECREF(st->sha1_type);
        return -1;
    }

    return 0;
}


/* Initialize this module. */

static PyModuleDef_Slot _sha1_slots[] = {
    {Py_mod_exec, _sha1_exec},
    {0, NULL}
};

static struct PyModuleDef _sha1module = {
        PyModuleDef_HEAD_INIT,
        .m_name = "_sha1",
        .m_size = sizeof(SHA1State),
        .m_methods = SHA1_functions,
        .m_slots = _sha1_slots,
        .m_traverse = _sha1_traverse,
        .m_clear = _sha1_clear,
        .m_free = _sha1_free
};

PyMODINIT_FUNC
PyInit__sha1(void)
{
    return PyModuleDef_Init(&_sha1module);
}
