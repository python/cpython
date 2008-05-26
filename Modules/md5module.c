/* MD5 module */

/* This module provides an interface to the MD5 algorithm */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)
   Trevor Perrin (trevp@trevp.net)

   Copyright (C) 2005-2007   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

/* MD5 objects */

#include "Python.h"


/* Some useful types */

#if SIZEOF_INT == 4
typedef unsigned int MD5_INT32;	/* 32-bit integer */
typedef PY_LONG_LONG MD5_INT64;	/* 64-bit integer */
#else
/* not defined. compilation will die. */
#endif

/* The MD5 block size and message digest sizes, in bytes */

#define MD5_BLOCKSIZE    64
#define MD5_DIGESTSIZE   16

/* The structure for storing MD5 info */

struct md5_state {
    MD5_INT64 length;
    MD5_INT32 state[4], curlen;
    unsigned char buf[MD5_BLOCKSIZE];
};

typedef struct {
    PyObject_HEAD

    struct md5_state hash_state;
} MD5object;


/* ------------------------------------------------------------------------
 *
 * This code for the MD5 algorithm was noted as public domain. The
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
#define ROLc(x, y) ( (((unsigned long)(x)<<(unsigned long)((y)&31)) | (((unsigned long)(x)&0xFFFFFFFFUL)>>(unsigned long)(32-((y)&31)))) & 0xFFFFFFFFUL)

/* Endian Neutral macros that work on all platforms */

#define STORE32L(x, y)                                                                     \
     { (y)[3] = (unsigned char)(((x)>>24)&255); (y)[2] = (unsigned char)(((x)>>16)&255);   \
       (y)[1] = (unsigned char)(((x)>>8)&255); (y)[0] = (unsigned char)((x)&255); }

#define LOAD32L(x, y)                            \
     { x = ((unsigned long)((y)[3] & 255)<<24) | \
           ((unsigned long)((y)[2] & 255)<<16) | \
           ((unsigned long)((y)[1] & 255)<<8)  | \
           ((unsigned long)((y)[0] & 255)); }

#define STORE64L(x, y)                                                                     \
     { (y)[7] = (unsigned char)(((x)>>56)&255); (y)[6] = (unsigned char)(((x)>>48)&255);   \
       (y)[5] = (unsigned char)(((x)>>40)&255); (y)[4] = (unsigned char)(((x)>>32)&255);   \
       (y)[3] = (unsigned char)(((x)>>24)&255); (y)[2] = (unsigned char)(((x)>>16)&255);   \
       (y)[1] = (unsigned char)(((x)>>8)&255); (y)[0] = (unsigned char)((x)&255); }

#ifndef MIN
   #define MIN(x, y) ( ((x)<(y))?(x):(y) )
#endif


/* MD5 macros */

#define F(x,y,z)  (z ^ (x & (y ^ z)))
#define G(x,y,z)  (y ^ (z & (y ^ x)))
#define H(x,y,z)  (x^y^z)
#define I(x,y,z)  (y^(x|(~z)))

#define FF(a,b,c,d,M,s,t) \
    a = (a + F(b,c,d) + M + t); a = ROLc(a, s) + b;

#define GG(a,b,c,d,M,s,t) \
    a = (a + G(b,c,d) + M + t); a = ROLc(a, s) + b;

#define HH(a,b,c,d,M,s,t) \
    a = (a + H(b,c,d) + M + t); a = ROLc(a, s) + b;

#define II(a,b,c,d,M,s,t) \
    a = (a + I(b,c,d) + M + t); a = ROLc(a, s) + b;


static void md5_compress(struct md5_state *md5, unsigned char *buf)
{
    MD5_INT32 i, W[16], a, b, c, d;

    assert(md5 != NULL);
    assert(buf != NULL);

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32L(W[i], buf + (4*i));
    }
 
    /* copy state */
    a = md5->state[0];
    b = md5->state[1];
    c = md5->state[2];
    d = md5->state[3];

    FF(a,b,c,d,W[0],7,0xd76aa478UL)
    FF(d,a,b,c,W[1],12,0xe8c7b756UL)
    FF(c,d,a,b,W[2],17,0x242070dbUL)
    FF(b,c,d,a,W[3],22,0xc1bdceeeUL)
    FF(a,b,c,d,W[4],7,0xf57c0fafUL)
    FF(d,a,b,c,W[5],12,0x4787c62aUL)
    FF(c,d,a,b,W[6],17,0xa8304613UL)
    FF(b,c,d,a,W[7],22,0xfd469501UL)
    FF(a,b,c,d,W[8],7,0x698098d8UL)
    FF(d,a,b,c,W[9],12,0x8b44f7afUL)
    FF(c,d,a,b,W[10],17,0xffff5bb1UL)
    FF(b,c,d,a,W[11],22,0x895cd7beUL)
    FF(a,b,c,d,W[12],7,0x6b901122UL)
    FF(d,a,b,c,W[13],12,0xfd987193UL)
    FF(c,d,a,b,W[14],17,0xa679438eUL)
    FF(b,c,d,a,W[15],22,0x49b40821UL)
    GG(a,b,c,d,W[1],5,0xf61e2562UL)
    GG(d,a,b,c,W[6],9,0xc040b340UL)
    GG(c,d,a,b,W[11],14,0x265e5a51UL)
    GG(b,c,d,a,W[0],20,0xe9b6c7aaUL)
    GG(a,b,c,d,W[5],5,0xd62f105dUL)
    GG(d,a,b,c,W[10],9,0x02441453UL)
    GG(c,d,a,b,W[15],14,0xd8a1e681UL)
    GG(b,c,d,a,W[4],20,0xe7d3fbc8UL)
    GG(a,b,c,d,W[9],5,0x21e1cde6UL)
    GG(d,a,b,c,W[14],9,0xc33707d6UL)
    GG(c,d,a,b,W[3],14,0xf4d50d87UL)
    GG(b,c,d,a,W[8],20,0x455a14edUL)
    GG(a,b,c,d,W[13],5,0xa9e3e905UL)
    GG(d,a,b,c,W[2],9,0xfcefa3f8UL)
    GG(c,d,a,b,W[7],14,0x676f02d9UL)
    GG(b,c,d,a,W[12],20,0x8d2a4c8aUL)
    HH(a,b,c,d,W[5],4,0xfffa3942UL)
    HH(d,a,b,c,W[8],11,0x8771f681UL)
    HH(c,d,a,b,W[11],16,0x6d9d6122UL)
    HH(b,c,d,a,W[14],23,0xfde5380cUL)
    HH(a,b,c,d,W[1],4,0xa4beea44UL)
    HH(d,a,b,c,W[4],11,0x4bdecfa9UL)
    HH(c,d,a,b,W[7],16,0xf6bb4b60UL)
    HH(b,c,d,a,W[10],23,0xbebfbc70UL)
    HH(a,b,c,d,W[13],4,0x289b7ec6UL)
    HH(d,a,b,c,W[0],11,0xeaa127faUL)
    HH(c,d,a,b,W[3],16,0xd4ef3085UL)
    HH(b,c,d,a,W[6],23,0x04881d05UL)
    HH(a,b,c,d,W[9],4,0xd9d4d039UL)
    HH(d,a,b,c,W[12],11,0xe6db99e5UL)
    HH(c,d,a,b,W[15],16,0x1fa27cf8UL)
    HH(b,c,d,a,W[2],23,0xc4ac5665UL)
    II(a,b,c,d,W[0],6,0xf4292244UL)
    II(d,a,b,c,W[7],10,0x432aff97UL)
    II(c,d,a,b,W[14],15,0xab9423a7UL)
    II(b,c,d,a,W[5],21,0xfc93a039UL)
    II(a,b,c,d,W[12],6,0x655b59c3UL)
    II(d,a,b,c,W[3],10,0x8f0ccc92UL)
    II(c,d,a,b,W[10],15,0xffeff47dUL)
    II(b,c,d,a,W[1],21,0x85845dd1UL)
    II(a,b,c,d,W[8],6,0x6fa87e4fUL)
    II(d,a,b,c,W[15],10,0xfe2ce6e0UL)
    II(c,d,a,b,W[6],15,0xa3014314UL)
    II(b,c,d,a,W[13],21,0x4e0811a1UL)
    II(a,b,c,d,W[4],6,0xf7537e82UL)
    II(d,a,b,c,W[11],10,0xbd3af235UL)
    II(c,d,a,b,W[2],15,0x2ad7d2bbUL)
    II(b,c,d,a,W[9],21,0xeb86d391UL)

    md5->state[0] = md5->state[0] + a;
    md5->state[1] = md5->state[1] + b;
    md5->state[2] = md5->state[2] + c;
    md5->state[3] = md5->state[3] + d;
}


/**
   Initialize the hash state
   @param sha1   The hash state you wish to initialize
*/
void md5_init(struct md5_state *md5)
{
    assert(md5 != NULL);
    md5->state[0] = 0x67452301UL;
    md5->state[1] = 0xefcdab89UL;
    md5->state[2] = 0x98badcfeUL;
    md5->state[3] = 0x10325476UL;
    md5->curlen = 0;
    md5->length = 0;
}

/**
   Process a block of memory though the hash
   @param sha1   The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
*/
void md5_process(struct md5_state *md5,
                const unsigned char *in, unsigned long inlen)
{
    unsigned long n;

    assert(md5 != NULL);
    assert(in != NULL);
    assert(md5->curlen <= sizeof(md5->buf));

    while (inlen > 0) {
        if (md5->curlen == 0 && inlen >= MD5_BLOCKSIZE) {
           md5_compress(md5, (unsigned char *)in);
           md5->length    += MD5_BLOCKSIZE * 8;
           in             += MD5_BLOCKSIZE;
           inlen          -= MD5_BLOCKSIZE;
        } else {
           n = MIN(inlen, (MD5_BLOCKSIZE - md5->curlen));
           memcpy(md5->buf + md5->curlen, in, (size_t)n);
           md5->curlen    += n;
           in             += n;
           inlen          -= n;
           if (md5->curlen == MD5_BLOCKSIZE) {
              md5_compress(md5, md5->buf);
              md5->length += 8*MD5_BLOCKSIZE;
              md5->curlen = 0;
           }
       }
    }
}

/**
   Terminate the hash to get the digest
   @param sha1  The hash state
   @param out [out] The destination of the hash (16 bytes)
*/
void md5_done(struct md5_state *md5, unsigned char *out)
{
    int i;

    assert(md5 != NULL);
    assert(out != NULL);
    assert(md5->curlen < sizeof(md5->buf));

    /* increase the length of the message */
    md5->length += md5->curlen * 8;

    /* append the '1' bit */
    md5->buf[md5->curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md5->curlen > 56) {
        while (md5->curlen < 64) {
            md5->buf[md5->curlen++] = (unsigned char)0;
        }
        md5_compress(md5, md5->buf);
        md5->curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md5->curlen < 56) {
        md5->buf[md5->curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64L(md5->length, md5->buf+56);
    md5_compress(md5, md5->buf);

    /* copy output */
    for (i = 0; i < 4; i++) {
        STORE32L(md5->state[i], out+(4*i));
    }
}

/* .Source: /cvs/libtom/libtomcrypt/src/hashes/md5.c,v $ */
/* .Revision: 1.10 $ */
/* .Date: 2007/05/12 14:25:28 $ */

/*
 * End of copied MD5 code.
 *
 * ------------------------------------------------------------------------
 */

static PyTypeObject MD5type;


static MD5object *
newMD5object(void)
{
    return (MD5object *)PyObject_New(MD5object, &MD5type);
}


/* Internal methods for a hash object */

static void
MD5_dealloc(PyObject *ptr)
{
    PyObject_Del(ptr);
}


/* External methods for a hash object */

PyDoc_STRVAR(MD5_copy__doc__, "Return a copy of the hash object.");

static PyObject *
MD5_copy(MD5object *self, PyObject *unused)
{
    MD5object *newobj;

    if (Py_TYPE(self) == &MD5type) {
        if ( (newobj = newMD5object())==NULL)
            return NULL;
    } else {
        if ( (newobj = newMD5object())==NULL)
            return NULL;
    }

    newobj->hash_state = self->hash_state;
    return (PyObject *)newobj;
}

PyDoc_STRVAR(MD5_digest__doc__,
"Return the digest value as a string of binary data.");

static PyObject *
MD5_digest(MD5object *self, PyObject *unused)
{
    unsigned char digest[MD5_DIGESTSIZE];
    struct md5_state temp;

    temp = self->hash_state;
    md5_done(&temp, digest);
    return PyBytes_FromStringAndSize((const char *)digest, MD5_DIGESTSIZE);
}

PyDoc_STRVAR(MD5_hexdigest__doc__,
"Return the digest value as a string of hexadecimal digits.");

static PyObject *
MD5_hexdigest(MD5object *self, PyObject *unused)
{
    unsigned char digest[MD5_DIGESTSIZE];
    struct md5_state temp;
    PyObject *retval;
    Py_UNICODE *hex_digest;
    int i, j;

    /* Get the raw (binary) digest value */
    temp = self->hash_state;
    md5_done(&temp, digest);

    /* Create a new string */
    retval = PyUnicode_FromStringAndSize(NULL, MD5_DIGESTSIZE * 2);
    if (!retval)
	    return NULL;
    hex_digest = PyUnicode_AS_UNICODE(retval);
    if (!hex_digest) {
	    Py_DECREF(retval);
	    return NULL;
    }

    /* Make hex version of the digest */
    for(i=j=0; i<MD5_DIGESTSIZE; i++) {
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

PyDoc_STRVAR(MD5_update__doc__,
"Update this hash object's state with the provided string.");

static PyObject *
MD5_update(MD5object *self, PyObject *args)
{
    unsigned char *cp;
    int len;

    if (!PyArg_ParseTuple(args, "s#:update", &cp, &len))
        return NULL;

    md5_process(&self->hash_state, cp, len);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef MD5_methods[] = {
    {"copy",	  (PyCFunction)MD5_copy,      METH_NOARGS,  MD5_copy__doc__},
    {"digest",	  (PyCFunction)MD5_digest,    METH_NOARGS,  MD5_digest__doc__},
    {"hexdigest", (PyCFunction)MD5_hexdigest, METH_NOARGS,  MD5_hexdigest__doc__},
    {"update",	  (PyCFunction)MD5_update,    METH_VARARGS, MD5_update__doc__},
    {NULL,	  NULL}		/* sentinel */
};

static PyObject *
MD5_get_block_size(PyObject *self, void *closure)
{
    return PyLong_FromLong(MD5_BLOCKSIZE);
}

static PyObject *
MD5_get_name(PyObject *self, void *closure)
{
    return PyUnicode_FromStringAndSize("MD5", 3);
}

static PyObject *
md5_get_digest_size(PyObject *self, void *closure)
{
    return PyLong_FromLong(MD5_DIGESTSIZE);
}


static PyGetSetDef MD5_getseters[] = {
    {"block_size",
     (getter)MD5_get_block_size, NULL,
     NULL,
     NULL},
    {"name",
     (getter)MD5_get_name, NULL,
     NULL,
     NULL},
    {"digest_size",
     (getter)md5_get_digest_size, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};

static PyTypeObject MD5type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_md5.md5",	        /*tp_name*/
    sizeof(MD5object),	/*tp_size*/
    0,			/*tp_itemsize*/
    /* methods */
    MD5_dealloc,	/*tp_dealloc*/
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
    MD5_methods,	/* tp_methods */
    NULL,	        /* tp_members */
    MD5_getseters,      /* tp_getset */
};


/* The single module-level function: new() */

PyDoc_STRVAR(MD5_new__doc__,
"Return a new MD5 hash object; optionally initialized with a string.");

static PyObject *
MD5_new(PyObject *self, PyObject *args, PyObject *kwdict)
{
    static char *kwlist[] = {"string", NULL};
    MD5object *new;
    unsigned char *cp = NULL;
    int len;

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "|s#:new", kwlist,
                                     &cp, &len)) {
        return NULL;
    }

    if ((new = newMD5object()) == NULL)
        return NULL;

    md5_init(&new->hash_state);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        return NULL;
    }
    if (cp)
        md5_process(&new->hash_state, cp, len);

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef MD5_functions[] = {
    {"md5", (PyCFunction)MD5_new, METH_VARARGS|METH_KEYWORDS, MD5_new__doc__},
    {NULL,	NULL}		 /* Sentinel */
};


/* Initialize this module. */

#define insint(n,v) { PyModule_AddIntConstant(m,n,v); }

PyMODINIT_FUNC
init_md5(void)
{
    PyObject *m;

    Py_TYPE(&MD5type) = &PyType_Type;
    if (PyType_Ready(&MD5type) < 0)
        return;
    m = Py_InitModule("_md5", MD5_functions);
    if (m == NULL)
	return;
}
