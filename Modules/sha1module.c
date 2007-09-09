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


/* Some useful types */

#if SIZEOF_INT == 4
typedef unsigned int SHA1_INT32;	/* 32-bit integer */
typedef PY_LONG_LONG SHA1_INT64;	/* 64-bit integer */
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

#ifndef MIN
   #define MIN(x, y) ( ((x)<(y))?(x):(y) )
#endif


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
    #define FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
    #define FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
    #define FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
    #define FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);
 
    for (i = 0; i < 20; ) {
       FF0(a,b,c,d,e,i++);
       FF0(e,a,b,c,d,i++);
       FF0(d,e,a,b,c,i++);
       FF0(c,d,e,a,b,i++);
       FF0(b,c,d,e,a,i++);
    }

    /* round two */
    for (; i < 40; )  { 
       FF1(a,b,c,d,e,i++);
       FF1(e,a,b,c,d,i++);
       FF1(d,e,a,b,c,i++);
       FF1(c,d,e,a,b,i++);
       FF1(b,c,d,e,a,i++);
    }

    /* round three */
    for (; i < 60; )  { 
       FF2(a,b,c,d,e,i++);
       FF2(e,a,b,c,d,i++);
       FF2(d,e,a,b,c,i++);
       FF2(c,d,e,a,b,i++);
       FF2(b,c,d,e,a,i++);
    }

    /* round four */
    for (; i < 80; )  { 
       FF3(a,b,c,d,e,i++);
       FF3(e,a,b,c,d,i++);
       FF3(d,e,a,b,c,i++);
       FF3(c,d,e,a,b,i++);
       FF3(b,c,d,e,a,i++);
    }

    #undef FF0
    #undef FF1
    #undef FF2
    #undef FF3

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
void sha1_init(struct sha1_state *sha1)
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
void sha1_process(struct sha1_state *sha1,
                  const unsigned char *in, unsigned long inlen)
{
    unsigned long n;

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
           n = MIN(inlen, (SHA1_BLOCKSIZE - sha1->curlen));
           memcpy(sha1->buf + sha1->curlen, in, (size_t)n);
           sha1->curlen   += n;
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
void sha1_done(struct sha1_state *sha1, unsigned char *out)
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

static PyTypeObject SHA1type;


static SHA1object *
newSHA1object(void)
{
    return (SHA1object *)PyObject_New(SHA1object, &SHA1type);
}


/* Internal methods for a hash object */

static void
SHA1_dealloc(PyObject *ptr)
{
    PyObject_Del(ptr);
}


/* External methods for a hash object */

PyDoc_STRVAR(SHA1_copy__doc__, "Return a copy of the hash object.");

static PyObject *
SHA1_copy(SHA1object *self, PyObject *unused)
{
    SHA1object *newobj;

    if (Py_Type(self) == &SHA1type) {
        if ( (newobj = newSHA1object())==NULL)
            return NULL;
    } else {
        if ( (newobj = newSHA1object())==NULL)
            return NULL;
    }

    newobj->hash_state = self->hash_state;
    return (PyObject *)newobj;
}

PyDoc_STRVAR(SHA1_digest__doc__,
"Return the digest value as a string of binary data.");

static PyObject *
SHA1_digest(SHA1object *self, PyObject *unused)
{
    unsigned char digest[SHA1_DIGESTSIZE];
    struct sha1_state temp;

    temp = self->hash_state;
    sha1_done(&temp, digest);
    return PyBytes_FromStringAndSize((const char *)digest, SHA1_DIGESTSIZE);
}

PyDoc_STRVAR(SHA1_hexdigest__doc__,
"Return the digest value as a string of hexadecimal digits.");

static PyObject *
SHA1_hexdigest(SHA1object *self, PyObject *unused)
{
    unsigned char digest[SHA1_DIGESTSIZE];
    struct sha1_state temp;
    PyObject *retval;
    Py_UNICODE *hex_digest;
    int i, j;

    /* Get the raw (binary) digest value */
    temp = self->hash_state;
    sha1_done(&temp, digest);

    /* Create a new string */
    retval = PyUnicode_FromStringAndSize(NULL, SHA1_DIGESTSIZE * 2);
    if (!retval)
	    return NULL;
    hex_digest = PyUnicode_AS_UNICODE(retval);
    if (!hex_digest) {
	    Py_DECREF(retval);
	    return NULL;
    }

    /* Make hex version of the digest */
    for(i=j=0; i<SHA1_DIGESTSIZE; i++) {
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

PyDoc_STRVAR(SHA1_update__doc__,
"Update this hash object's state with the provided string.");

static PyObject *
SHA1_update(SHA1object *self, PyObject *args)
{
    unsigned char *cp;
    int len;

    if (!PyArg_ParseTuple(args, "s#:update", &cp, &len))
        return NULL;

    sha1_process(&self->hash_state, cp, len);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef SHA1_methods[] = {
    {"copy",	  (PyCFunction)SHA1_copy,      METH_NOARGS,  SHA1_copy__doc__},
    {"digest",	  (PyCFunction)SHA1_digest,    METH_NOARGS,  SHA1_digest__doc__},
    {"hexdigest", (PyCFunction)SHA1_hexdigest, METH_NOARGS,  SHA1_hexdigest__doc__},
    {"update",	  (PyCFunction)SHA1_update,    METH_VARARGS, SHA1_update__doc__},
    {NULL,	  NULL}		/* sentinel */
};

static PyObject *
SHA1_get_block_size(PyObject *self, void *closure)
{
    return PyInt_FromLong(SHA1_BLOCKSIZE);
}

static PyObject *
SHA1_get_name(PyObject *self, void *closure)
{
    return PyUnicode_FromStringAndSize("SHA1", 3);
}

static PyObject *
sha1_get_digest_size(PyObject *self, void *closure)
{
    return PyInt_FromLong(SHA1_DIGESTSIZE);
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

static PyTypeObject SHA1type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_sha1.sha1",	        /*tp_name*/
    sizeof(SHA1object),	/*tp_size*/
    0,			/*tp_itemsize*/
    /* methods */
    SHA1_dealloc,	/*tp_dealloc*/
    0,			/*tp_print*/
    0,          	/*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_compare*/
    0,                  /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash*/
    0,                  /*tp_call*/
    0,                  /*tp_str*/
    0,                  /*tp_getattro*/
    0,                  /*tp_setattro*/
    0,                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT, /*tp_flags*/
    0,                  /*tp_doc*/
    0,                  /*tp_traverse*/
    0,			/*tp_clear*/
    0,			/*tp_richcompare*/
    0,			/*tp_weaklistoffset*/
    0,			/*tp_iter*/
    0,			/*tp_iternext*/
    SHA1_methods,	/* tp_methods */
    NULL,	        /* tp_members */
    SHA1_getseters,      /* tp_getset */
};


/* The single module-level function: new() */

PyDoc_STRVAR(SHA1_new__doc__,
"Return a new SHA1 hash object; optionally initialized with a string.");

static PyObject *
SHA1_new(PyObject *self, PyObject *args, PyObject *kwdict)
{
    static char *kwlist[] = {"string", NULL};
    SHA1object *new;
    unsigned char *cp = NULL;
    int len;

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "|s#:new", kwlist,
                                     &cp, &len)) {
        return NULL;
    }

    if ((new = newSHA1object()) == NULL)
        return NULL;

    sha1_init(&new->hash_state);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        return NULL;
    }
    if (cp)
        sha1_process(&new->hash_state, cp, len);

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef SHA1_functions[] = {
    {"sha1",(PyCFunction)SHA1_new, METH_VARARGS|METH_KEYWORDS,SHA1_new__doc__},
    {NULL,	NULL}		 /* Sentinel */
};


/* Initialize this module. */

#define insint(n,v) { PyModule_AddIntConstant(m,n,v); }

PyMODINIT_FUNC
init_sha1(void)
{
    PyObject *m;

    Py_Type(&SHA1type) = &PyType_Type;
    if (PyType_Ready(&SHA1type) < 0)
        return;
    m = Py_InitModule("_sha1", SHA1_functions);
    if (m == NULL)
	return;
}
