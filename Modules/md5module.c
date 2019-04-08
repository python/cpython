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
#include "hashlib.h"
#include "pystrhex.h"

/*[clinic input]
module _md5
class MD5Type "MD5object *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6e5261719957a912]*/

/* Some useful types */

#if SIZEOF_INT == 4
typedef unsigned int MD5_INT32; /* 32-bit integer */
typedef long long MD5_INT64; /* 64-bit integer */
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

#include "clinic/md5module.c.h"

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
static void
md5_init(struct md5_state *md5)
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
static void
md5_process(struct md5_state *md5, const unsigned char *in, Py_ssize_t inlen)
{
    Py_ssize_t n;

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
           n = Py_MIN(inlen, (Py_ssize_t)(MD5_BLOCKSIZE - md5->curlen));
           memcpy(md5->buf + md5->curlen, in, (size_t)n);
           md5->curlen    += (MD5_INT32)n;
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
static void
md5_done(struct md5_state *md5, unsigned char *out)
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

    /* pad up to 56 bytes of zeroes */
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

/*[clinic input]
MD5Type.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
MD5Type_copy_impl(MD5object *self)
/*[clinic end generated code: output=596eb36852f02071 input=2c09e6d2493f3079]*/
{
    MD5object *newobj;

    if ((newobj = newMD5object())==NULL)
        return NULL;

    newobj->hash_state = self->hash_state;
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
    unsigned char digest[MD5_DIGESTSIZE];
    struct md5_state temp;

    temp = self->hash_state;
    md5_done(&temp, digest);
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
    unsigned char digest[MD5_DIGESTSIZE];
    struct md5_state temp;

    /* Get the raw (binary) digest value */
    temp = self->hash_state;
    md5_done(&temp, digest);

    return _Py_strhex((const char*)digest, MD5_DIGESTSIZE);
}

/*[clinic input]
MD5Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
MD5Type_update(MD5object *self, PyObject *obj)
/*[clinic end generated code: output=f6ad168416338423 input=6e1efcd9ecf17032]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);

    md5_process(&self->hash_state, buf.buf, buf.len);

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
MD5_get_block_size(PyObject *self, void *closure)
{
    return PyLong_FromLong(MD5_BLOCKSIZE);
}

static PyObject *
MD5_get_name(PyObject *self, void *closure)
{
    return PyUnicode_FromStringAndSize("md5", 3);
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
    "_md5.md5",         /*tp_name*/
    sizeof(MD5object),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    /* methods */
    MD5_dealloc,        /*tp_dealloc*/
    0,                  /*tp_print*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_reserved*/
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
    0,                  /*tp_clear*/
    0,                  /*tp_richcompare*/
    0,                  /*tp_weaklistoffset*/
    0,                  /*tp_iter*/
    0,                  /*tp_iternext*/
    MD5_methods,        /* tp_methods */
    NULL,               /* tp_members */
    MD5_getseters,      /* tp_getset */
};


/* The single module-level function: new() */

/*[clinic input]
_md5.md5

    string: object(c_default="NULL") = b''

Return a new MD5 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_md5_md5_impl(PyObject *module, PyObject *string)
/*[clinic end generated code: output=2cfd0f8c091b97e6 input=d12ef8f72d684f7b]*/
{
    MD5object *new;
    Py_buffer buf;

    if (string)
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);

    if ((new = newMD5object()) == NULL) {
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }

    md5_init(&new->hash_state);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }
    if (string) {
        md5_process(&new->hash_state, buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef MD5_functions[] = {
    _MD5_MD5_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};


/* Initialize this module. */

#define insint(n,v) { PyModule_AddIntConstant(m,n,v); }


static struct PyModuleDef _md5module = {
        PyModuleDef_HEAD_INIT,
        "_md5",
        NULL,
        -1,
        MD5_functions,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__md5(void)
{
    PyObject *m;

    Py_TYPE(&MD5type) = &PyType_Type;
    if (PyType_Ready(&MD5type) < 0)
        return NULL;

    m = PyModule_Create(&_md5module);
    if (m == NULL)
        return NULL;

    Py_INCREF((PyObject *)&MD5type);
    PyModule_AddObject(m, "MD5Type", (PyObject *)&MD5type);
    return m;
}
