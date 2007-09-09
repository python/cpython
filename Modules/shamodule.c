/* SHA module */

/* This module provides an interface to NIST's Secure Hash Algorithm */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)

   Copyright (C) 2005   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

/* SHA objects */

#include "Python.h"
#include "structmember.h"


/* Endianness testing and definitions */
#define TestEndianness(variable) {int i=1; variable=PCT_BIG_ENDIAN;\
	if (*((char*)&i)==1) variable=PCT_LITTLE_ENDIAN;}

#define PCT_LITTLE_ENDIAN 1
#define PCT_BIG_ENDIAN 0

/* Some useful types */

typedef unsigned char SHA_BYTE;

#if SIZEOF_INT == 4
typedef unsigned int SHA_INT32;	/* 32-bit integer */
#else
/* not defined. compilation will die. */
#endif

/* The SHA block size and message digest sizes, in bytes */

#define SHA_BLOCKSIZE    64
#define SHA_DIGESTSIZE  20

/* The structure for storing SHS info */

typedef struct {
    PyObject_HEAD
    SHA_INT32 digest[5];		/* Message digest */
    SHA_INT32 count_lo, count_hi;	/* 64-bit bit count */
    SHA_BYTE data[SHA_BLOCKSIZE];	/* SHA data buffer */
    int Endianness;
    int local;				/* unprocessed amount in data */
} SHAobject;

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords. */

static void longReverse(SHA_INT32 *buffer, int byteCount, int Endianness)
{
    SHA_INT32 value;

    if ( Endianness == PCT_BIG_ENDIAN )
	return;

    byteCount /= sizeof(*buffer);
    while (byteCount--) {
        value = *buffer;
        value = ( ( value & 0xFF00FF00L ) >> 8  ) | \
                ( ( value & 0x00FF00FFL ) << 8 );
        *buffer++ = ( value << 16 ) | ( value >> 16 );
    }
}

static void SHAcopy(SHAobject *src, SHAobject *dest)
{
    dest->Endianness = src->Endianness;
    dest->local = src->local;
    dest->count_lo = src->count_lo;
    dest->count_hi = src->count_hi;
    memcpy(dest->digest, src->digest, sizeof(src->digest));
    memcpy(dest->data, src->data, sizeof(src->data));
}


/* ------------------------------------------------------------------------
 *
 * This code for the SHA algorithm was noted as public domain. The original
 * headers are pasted below.
 *
 * Several changes have been made to make it more compatible with the
 * Python environment and desired interface.
 *
 */

/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach <uh@alumni.caltech edu> */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */
/* Further modifications to include the "UNRAVEL" stuff, below */

/* This code is in the public domain */

/* UNRAVEL should be fastest & biggest */
/* UNROLL_LOOPS should be just as big, but slightly slower */
/* both undefined should be smallest and slowest */

#define UNRAVEL
/* #define UNROLL_LOOPS */

/* The SHA f()-functions.  The f1 and f3 functions can be optimized to
   save one boolean operation each - thanks to Rich Schroeppel,
   rcs@cs.arizona.edu for discovering this */

/*#define f1(x,y,z)	((x & y) | (~x & z))		// Rounds  0-19 */
#define f1(x,y,z)	(z ^ (x & (y ^ z)))		/* Rounds  0-19 */
#define f2(x,y,z)	(x ^ y ^ z)			/* Rounds 20-39 */
/*#define f3(x,y,z)	((x & y) | (x & z) | (y & z))	// Rounds 40-59 */
#define f3(x,y,z)	((x & y) | (z & (x | y)))	/* Rounds 40-59 */
#define f4(x,y,z)	(x ^ y ^ z)			/* Rounds 60-79 */

/* SHA constants */

#define CONST1		0x5a827999L			/* Rounds  0-19 */
#define CONST2		0x6ed9eba1L			/* Rounds 20-39 */
#define CONST3		0x8f1bbcdcL			/* Rounds 40-59 */
#define CONST4		0xca62c1d6L			/* Rounds 60-79 */

/* 32-bit rotate */

#define R32(x,n)	((x << n) | (x >> (32 - n)))

/* the generic case, for when the overall rotation is not unraveled */

#define FG(n)	\
    T = R32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n;	\
    E = D; D = C; C = R32(B,30); B = A; A = T

/* specific cases, for when the overall rotation is unraveled */

#define FA(n)	\
    T = R32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n; B = R32(B,30)

#define FB(n)	\
    E = R32(T,5) + f##n(A,B,C) + D + *WP++ + CONST##n; A = R32(A,30)

#define FC(n)	\
    D = R32(E,5) + f##n(T,A,B) + C + *WP++ + CONST##n; T = R32(T,30)

#define FD(n)	\
    C = R32(D,5) + f##n(E,T,A) + B + *WP++ + CONST##n; E = R32(E,30)

#define FE(n)	\
    B = R32(C,5) + f##n(D,E,T) + A + *WP++ + CONST##n; D = R32(D,30)

#define FT(n)	\
    A = R32(B,5) + f##n(C,D,E) + T + *WP++ + CONST##n; C = R32(C,30)

/* do SHA transformation */

static void
sha_transform(SHAobject *sha_info)
{
    int i;
    SHA_INT32 T, A, B, C, D, E, W[80], *WP;

    memcpy(W, sha_info->data, sizeof(sha_info->data));
    longReverse(W, (int)sizeof(sha_info->data), sha_info->Endianness);

    for (i = 16; i < 80; ++i) {
	W[i] = W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16];

	/* extra rotation fix */
	W[i] = R32(W[i], 1);
    }
    A = sha_info->digest[0];
    B = sha_info->digest[1];
    C = sha_info->digest[2];
    D = sha_info->digest[3];
    E = sha_info->digest[4];
    WP = W;
#ifdef UNRAVEL
    FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1); FC(1); FD(1);
    FE(1); FT(1); FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1);
    FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2); FE(2); FT(2);
    FA(2); FB(2); FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2);
    FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3); FA(3); FB(3);
    FC(3); FD(3); FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3);
    FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4); FC(4); FD(4);
    FE(4); FT(4); FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4);
    sha_info->digest[0] += E;
    sha_info->digest[1] += T;
    sha_info->digest[2] += A;
    sha_info->digest[3] += B;
    sha_info->digest[4] += C;
#else /* !UNRAVEL */
#ifdef UNROLL_LOOPS
    FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
    FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
    FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
    FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
    FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
    FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
    FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
    FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
#else /* !UNROLL_LOOPS */
    for (i =  0; i < 20; ++i) { FG(1); }
    for (i = 20; i < 40; ++i) { FG(2); }
    for (i = 40; i < 60; ++i) { FG(3); }
    for (i = 60; i < 80; ++i) { FG(4); }
#endif /* !UNROLL_LOOPS */
    sha_info->digest[0] += A;
    sha_info->digest[1] += B;
    sha_info->digest[2] += C;
    sha_info->digest[3] += D;
    sha_info->digest[4] += E;
#endif /* !UNRAVEL */
}

/* initialize the SHA digest */

static void
sha_init(SHAobject *sha_info)
{
    TestEndianness(sha_info->Endianness)

    sha_info->digest[0] = 0x67452301L;
    sha_info->digest[1] = 0xefcdab89L;
    sha_info->digest[2] = 0x98badcfeL;
    sha_info->digest[3] = 0x10325476L;
    sha_info->digest[4] = 0xc3d2e1f0L;
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
}

/* update the SHA digest */

static void
sha_update(SHAobject *sha_info, SHA_BYTE *buffer, int count)
{
    int i;
    SHA_INT32 clo;

    clo = sha_info->count_lo + ((SHA_INT32) count << 3);
    if (clo < sha_info->count_lo) {
        ++sha_info->count_hi;
    }
    sha_info->count_lo = clo;
    sha_info->count_hi += (SHA_INT32) count >> 29;
    if (sha_info->local) {
        i = SHA_BLOCKSIZE - sha_info->local;
        if (i > count) {
            i = count;
        }
        memcpy(((SHA_BYTE *) sha_info->data) + sha_info->local, buffer, i);
        count -= i;
        buffer += i;
        sha_info->local += i;
        if (sha_info->local == SHA_BLOCKSIZE) {
            sha_transform(sha_info);
        }
        else {
            return;
        }
    }
    while (count >= SHA_BLOCKSIZE) {
        memcpy(sha_info->data, buffer, SHA_BLOCKSIZE);
        buffer += SHA_BLOCKSIZE;
        count -= SHA_BLOCKSIZE;
        sha_transform(sha_info);
    }
    memcpy(sha_info->data, buffer, count);
    sha_info->local = count;
}

/* finish computing the SHA digest */

static void
sha_final(unsigned char digest[20], SHAobject *sha_info)
{
    int count;
    SHA_INT32 lo_bit_count, hi_bit_count;

    lo_bit_count = sha_info->count_lo;
    hi_bit_count = sha_info->count_hi;
    count = (int) ((lo_bit_count >> 3) & 0x3f);
    ((SHA_BYTE *) sha_info->data)[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 8) {
	memset(((SHA_BYTE *) sha_info->data) + count, 0,
	       SHA_BLOCKSIZE - count);
	sha_transform(sha_info);
	memset((SHA_BYTE *) sha_info->data, 0, SHA_BLOCKSIZE - 8);
    }
    else {
	memset(((SHA_BYTE *) sha_info->data) + count, 0,
	       SHA_BLOCKSIZE - 8 - count);
    }

    /* GJS: note that we add the hi/lo in big-endian. sha_transform will
       swap these values into host-order. */
    sha_info->data[56] = (hi_bit_count >> 24) & 0xff;
    sha_info->data[57] = (hi_bit_count >> 16) & 0xff;
    sha_info->data[58] = (hi_bit_count >>  8) & 0xff;
    sha_info->data[59] = (hi_bit_count >>  0) & 0xff;
    sha_info->data[60] = (lo_bit_count >> 24) & 0xff;
    sha_info->data[61] = (lo_bit_count >> 16) & 0xff;
    sha_info->data[62] = (lo_bit_count >>  8) & 0xff;
    sha_info->data[63] = (lo_bit_count >>  0) & 0xff;
    sha_transform(sha_info);
    digest[ 0] = (unsigned char) ((sha_info->digest[0] >> 24) & 0xff);
    digest[ 1] = (unsigned char) ((sha_info->digest[0] >> 16) & 0xff);
    digest[ 2] = (unsigned char) ((sha_info->digest[0] >>  8) & 0xff);
    digest[ 3] = (unsigned char) ((sha_info->digest[0]      ) & 0xff);
    digest[ 4] = (unsigned char) ((sha_info->digest[1] >> 24) & 0xff);
    digest[ 5] = (unsigned char) ((sha_info->digest[1] >> 16) & 0xff);
    digest[ 6] = (unsigned char) ((sha_info->digest[1] >>  8) & 0xff);
    digest[ 7] = (unsigned char) ((sha_info->digest[1]      ) & 0xff);
    digest[ 8] = (unsigned char) ((sha_info->digest[2] >> 24) & 0xff);
    digest[ 9] = (unsigned char) ((sha_info->digest[2] >> 16) & 0xff);
    digest[10] = (unsigned char) ((sha_info->digest[2] >>  8) & 0xff);
    digest[11] = (unsigned char) ((sha_info->digest[2]      ) & 0xff);
    digest[12] = (unsigned char) ((sha_info->digest[3] >> 24) & 0xff);
    digest[13] = (unsigned char) ((sha_info->digest[3] >> 16) & 0xff);
    digest[14] = (unsigned char) ((sha_info->digest[3] >>  8) & 0xff);
    digest[15] = (unsigned char) ((sha_info->digest[3]      ) & 0xff);
    digest[16] = (unsigned char) ((sha_info->digest[4] >> 24) & 0xff);
    digest[17] = (unsigned char) ((sha_info->digest[4] >> 16) & 0xff);
    digest[18] = (unsigned char) ((sha_info->digest[4] >>  8) & 0xff);
    digest[19] = (unsigned char) ((sha_info->digest[4]      ) & 0xff);
}

/*
 * End of copied SHA code.
 *
 * ------------------------------------------------------------------------
 */

static PyTypeObject SHAtype;


static SHAobject *
newSHAobject(void)
{
    return (SHAobject *)PyObject_New(SHAobject, &SHAtype);
}

/* Internal methods for a hashing object */

static void
SHA_dealloc(PyObject *ptr)
{
    PyObject_Del(ptr);
}


/* External methods for a hashing object */

PyDoc_STRVAR(SHA_copy__doc__, "Return a copy of the hashing object.");

static PyObject *
SHA_copy(SHAobject *self, PyObject *unused)
{
    SHAobject *newobj;

    if ( (newobj = newSHAobject())==NULL)
        return NULL;

    SHAcopy(self, newobj);
    return (PyObject *)newobj;
}

PyDoc_STRVAR(SHA_digest__doc__,
"Return the digest value as a string of binary data.");

static PyObject *
SHA_digest(SHAobject *self, PyObject *unused)
{
    unsigned char digest[SHA_DIGESTSIZE];
    SHAobject temp;

    SHAcopy(self, &temp);
    sha_final(digest, &temp);
    return PyString_FromStringAndSize((const char *)digest, sizeof(digest));
}

PyDoc_STRVAR(SHA_hexdigest__doc__,
"Return the digest value as a string of hexadecimal digits.");

static PyObject *
SHA_hexdigest(SHAobject *self, PyObject *unused)
{
    unsigned char digest[SHA_DIGESTSIZE];
    SHAobject temp;
    PyObject *retval;
    char *hex_digest;
    int i, j;

    /* Get the raw (binary) digest value */
    SHAcopy(self, &temp);
    sha_final(digest, &temp);

    /* Create a new string */
    retval = PyString_FromStringAndSize(NULL, sizeof(digest) * 2);
    if (!retval)
	    return NULL;
    hex_digest = PyString_AsString(retval);
    if (!hex_digest) {
	    Py_DECREF(retval);
	    return NULL;
    }

    /* Make hex version of the digest */
    for(i=j=0; i<sizeof(digest); i++) {
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

PyDoc_STRVAR(SHA_update__doc__,
"Update this hashing object's state with the provided string.");

static PyObject *
SHA_update(SHAobject *self, PyObject *args)
{
    unsigned char *cp;
    int len;

    if (!PyArg_ParseTuple(args, "s#:update", &cp, &len))
        return NULL;

    sha_update(self, cp, len);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef SHA_methods[] = {
    {"copy",	  (PyCFunction)SHA_copy,      METH_NOARGS,  SHA_copy__doc__},
    {"digest",	  (PyCFunction)SHA_digest,    METH_NOARGS,  SHA_digest__doc__},
    {"hexdigest", (PyCFunction)SHA_hexdigest, METH_NOARGS,  SHA_hexdigest__doc__},
    {"update",	  (PyCFunction)SHA_update,    METH_VARARGS, SHA_update__doc__},
    {NULL,	  NULL}		/* sentinel */
};

static PyObject *
SHA_get_block_size(PyObject *self, void *closure)
{
    return PyInt_FromLong(SHA_BLOCKSIZE);
}

static PyObject *
SHA_get_digest_size(PyObject *self, void *closure)
{
    return PyInt_FromLong(SHA_DIGESTSIZE);
}

static PyObject *
SHA_get_name(PyObject *self, void *closure)
{
    return PyString_FromStringAndSize("SHA1", 4);
}

static PyGetSetDef SHA_getseters[] = {
    {"digest_size",
     (getter)SHA_get_digest_size, NULL,
     NULL,
     NULL},
    {"block_size",
     (getter)SHA_get_block_size, NULL,
     NULL,
     NULL},
    {"name",
     (getter)SHA_get_name, NULL,
     NULL,
     NULL},
    /* the old md5 and sha modules support 'digest_size' as in PEP 247.
     * the old sha module also supported 'digestsize'.  ugh. */
    {"digestsize",
     (getter)SHA_get_digest_size, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};

static PyTypeObject SHAtype = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_sha.sha",		/*tp_name*/
    sizeof(SHAobject),	/*tp_size*/
    0,			/*tp_itemsize*/
    /* methods */
    SHA_dealloc,	/*tp_dealloc*/
    0,			/*tp_print*/
    0,                  /*tp_getattr*/
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
    SHA_methods,	/* tp_methods */
    0,                  /* tp_members */
    SHA_getseters,      /* tp_getset */
};


/* The single module-level function: new() */

PyDoc_STRVAR(SHA_new__doc__,
"Return a new SHA hashing object.  An optional string argument\n\
may be provided; if present, this string will be automatically\n\
hashed.");

static PyObject *
SHA_new(PyObject *self, PyObject *args, PyObject *kwdict)
{
    static char *kwlist[] = {"string", NULL};
    SHAobject *new;
    unsigned char *cp = NULL;
    int len;

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "|s#:new", kwlist,
                                     &cp, &len)) {
        return NULL;
    }

    if ((new = newSHAobject()) == NULL)
        return NULL;

    sha_init(new);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        return NULL;
    }
    if (cp)
        sha_update(new, cp, len);

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef SHA_functions[] = {
    {"new", (PyCFunction)SHA_new, METH_VARARGS|METH_KEYWORDS, SHA_new__doc__},
    {NULL,	NULL}		 /* Sentinel */
};


/* Initialize this module. */

#define insint(n,v) { PyModule_AddIntConstant(m,n,v); }

PyMODINIT_FUNC
init_sha(void)
{
    PyObject *m;

    Py_Type(&SHAtype) = &PyType_Type;
    if (PyType_Ready(&SHAtype) < 0)
        return;
    m = Py_InitModule("_sha", SHA_functions);
    if (m == NULL)
	return;

    /* Add some symbolic constants to the module */
    insint("blocksize", 1);  /* For future use, in case some hash
                                functions require an integral number of
                                blocks */ 
    insint("digestsize", 20);
    insint("digest_size", 20);
}
