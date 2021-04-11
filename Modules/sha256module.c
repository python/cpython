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

#include "Python.h"
#include "pycore_bitutils.h"      // _Py_bswap32()
#include "structmember.h"         // PyMemberDef
#include "hashlib.h"
#include "pystrhex.h"

/*[clinic input]
module _sha256
class SHA256Type "SHAobject *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=71a39174d4f0a744]*/

/* Some useful types */

typedef unsigned char SHA_BYTE;
typedef uint32_t SHA_INT32;  /* 32-bit integer */

/* The SHA block size and message digest sizes, in bytes */

#define SHA_BLOCKSIZE    64
#define SHA_DIGESTSIZE  32

/* The structure for storing SHA info */

typedef struct {
    PyObject_HEAD
    SHA_INT32 digest[8];                /* Message digest */
    SHA_INT32 count_lo, count_hi;       /* 64-bit bit count */
    SHA_BYTE data[SHA_BLOCKSIZE];       /* SHA data buffer */
    int local;                          /* unprocessed amount in data */
    int digestsize;
} SHAobject;

#include "clinic/sha256module.c.h"

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

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords. */

#if PY_LITTLE_ENDIAN
static void longReverse(SHA_INT32 *buffer, int byteCount)
{
    byteCount /= sizeof(*buffer);
    for (; byteCount--; buffer++) {
        *buffer = _Py_bswap32(*buffer);
    }
}
#endif

static void SHAcopy(SHAobject *src, SHAobject *dest)
{
    dest->local = src->local;
    dest->digestsize = src->digestsize;
    dest->count_lo = src->count_lo;
    dest->count_hi = src->count_hi;
    memcpy(dest->digest, src->digest, sizeof(src->digest));
    memcpy(dest->data, src->data, sizeof(src->data));
}


/* ------------------------------------------------------------------------
 *
 * This code for the SHA-256 algorithm was noted as public domain. The
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
 * Tom St Denis, tomstdenis@iahu.ca, https://www.libtom.net
 */


/* SHA256 by Tom St Denis */

/* Various logical functions */
#define ROR(x, y)\
( ((((unsigned long)(x)&0xFFFFFFFFUL)>>(unsigned long)((y)&31)) | \
((unsigned long)(x)<<(unsigned long)(32-((y)&31)))) & 0xFFFFFFFFUL)
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         ROR((x),(n))
#define R(x, n)         (((x)&0xFFFFFFFFUL)>>(n))
#define Sigma0(x)       (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)       (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)       (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)       (S(x, 17) ^ S(x, 19) ^ R(x, 10))


static void
sha_transform(SHAobject *sha_info)
{
    int i;
        SHA_INT32 S[8], W[64], t0, t1;

    memcpy(W, sha_info->data, sizeof(sha_info->data));
#if PY_LITTLE_ENDIAN
    longReverse(W, (int)sizeof(sha_info->data));
#endif

    for (i = 16; i < 64; ++i) {
                W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }
    for (i = 0; i < 8; ++i) {
        S[i] = sha_info->digest[i];
    }

    /* Compress */
#define RND(a,b,c,d,e,f,g,h,i,ki)                    \
     t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i];   \
     t1 = Sigma0(a) + Maj(a, b, c);                  \
     d += t0;                                        \
     h  = t0 + t1;

    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],0,0x428a2f98);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],1,0x71374491);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],2,0xb5c0fbcf);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],3,0xe9b5dba5);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],4,0x3956c25b);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],5,0x59f111f1);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],6,0x923f82a4);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],7,0xab1c5ed5);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],8,0xd807aa98);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],9,0x12835b01);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],10,0x243185be);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],11,0x550c7dc3);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],12,0x72be5d74);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],13,0x80deb1fe);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],14,0x9bdc06a7);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],15,0xc19bf174);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],16,0xe49b69c1);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],17,0xefbe4786);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],18,0x0fc19dc6);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],19,0x240ca1cc);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],20,0x2de92c6f);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],21,0x4a7484aa);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],22,0x5cb0a9dc);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],23,0x76f988da);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],24,0x983e5152);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],25,0xa831c66d);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],26,0xb00327c8);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],27,0xbf597fc7);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],28,0xc6e00bf3);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],29,0xd5a79147);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],30,0x06ca6351);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],31,0x14292967);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],32,0x27b70a85);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],33,0x2e1b2138);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],34,0x4d2c6dfc);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],35,0x53380d13);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],36,0x650a7354);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],37,0x766a0abb);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],38,0x81c2c92e);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],39,0x92722c85);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],40,0xa2bfe8a1);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],41,0xa81a664b);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],42,0xc24b8b70);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],43,0xc76c51a3);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],44,0xd192e819);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],45,0xd6990624);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],46,0xf40e3585);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],47,0x106aa070);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],48,0x19a4c116);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],49,0x1e376c08);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],50,0x2748774c);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],51,0x34b0bcb5);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],52,0x391c0cb3);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],53,0x4ed8aa4a);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],54,0x5b9cca4f);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],55,0x682e6ff3);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],56,0x748f82ee);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],57,0x78a5636f);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],58,0x84c87814);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],59,0x8cc70208);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],60,0x90befffa);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],61,0xa4506ceb);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],62,0xbef9a3f7);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],63,0xc67178f2);

#undef RND

    /* feedback */
    for (i = 0; i < 8; i++) {
        sha_info->digest[i] = sha_info->digest[i] + S[i];
    }

}



/* initialize the SHA digest */

static void
sha_init(SHAobject *sha_info)
{
    sha_info->digest[0] = 0x6A09E667L;
    sha_info->digest[1] = 0xBB67AE85L;
    sha_info->digest[2] = 0x3C6EF372L;
    sha_info->digest[3] = 0xA54FF53AL;
    sha_info->digest[4] = 0x510E527FL;
    sha_info->digest[5] = 0x9B05688CL;
    sha_info->digest[6] = 0x1F83D9ABL;
    sha_info->digest[7] = 0x5BE0CD19L;
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
    sha_info->digestsize = 32;
}

static void
sha224_init(SHAobject *sha_info)
{
    sha_info->digest[0] = 0xc1059ed8L;
    sha_info->digest[1] = 0x367cd507L;
    sha_info->digest[2] = 0x3070dd17L;
    sha_info->digest[3] = 0xf70e5939L;
    sha_info->digest[4] = 0xffc00b31L;
    sha_info->digest[5] = 0x68581511L;
    sha_info->digest[6] = 0x64f98fa7L;
    sha_info->digest[7] = 0xbefa4fa4L;
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
    sha_info->digestsize = 28;
}


/* update the SHA digest */

static void
sha_update(SHAobject *sha_info, SHA_BYTE *buffer, Py_ssize_t count)
{
    Py_ssize_t i;
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
        sha_info->local += (int)i;
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
    sha_info->local = (int)count;
}

/* finish computing the SHA digest */

static void
sha_final(unsigned char digest[SHA_DIGESTSIZE], SHAobject *sha_info)
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
    digest[20] = (unsigned char) ((sha_info->digest[5] >> 24) & 0xff);
    digest[21] = (unsigned char) ((sha_info->digest[5] >> 16) & 0xff);
    digest[22] = (unsigned char) ((sha_info->digest[5] >>  8) & 0xff);
    digest[23] = (unsigned char) ((sha_info->digest[5]      ) & 0xff);
    digest[24] = (unsigned char) ((sha_info->digest[6] >> 24) & 0xff);
    digest[25] = (unsigned char) ((sha_info->digest[6] >> 16) & 0xff);
    digest[26] = (unsigned char) ((sha_info->digest[6] >>  8) & 0xff);
    digest[27] = (unsigned char) ((sha_info->digest[6]      ) & 0xff);
    digest[28] = (unsigned char) ((sha_info->digest[7] >> 24) & 0xff);
    digest[29] = (unsigned char) ((sha_info->digest[7] >> 16) & 0xff);
    digest[30] = (unsigned char) ((sha_info->digest[7] >>  8) & 0xff);
    digest[31] = (unsigned char) ((sha_info->digest[7]      ) & 0xff);
}

/*
 * End of copied SHA code.
 *
 * ------------------------------------------------------------------------
 */


static SHAobject *
newSHA224object(_sha256_state *state)
{
    return (SHAobject *)PyObject_New(SHAobject, state->sha224_type);
}

static SHAobject *
newSHA256object(_sha256_state *state)
{
    return (SHAobject *)PyObject_New(SHAobject, state->sha256_type);
}

/* Internal methods for a hash object */

static void
SHA_dealloc(PyObject *ptr)
{
    PyTypeObject *tp = Py_TYPE(ptr);
    PyObject_Free(ptr);
    Py_DECREF(tp);
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
    unsigned char digest[SHA_DIGESTSIZE];
    SHAobject temp;

    SHAcopy(self, &temp);
    sha_final(digest, &temp);
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
    unsigned char digest[SHA_DIGESTSIZE];
    SHAobject temp;

    /* Get the raw (binary) digest value */
    SHAcopy(self, &temp);
    sha_final(digest, &temp);

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

    sha_update(self, buf.buf, buf.len);

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
SHA256_get_name(PyObject *self, void *closure)
{
    if (((SHAobject *)self)->digestsize == 32)
        return PyUnicode_FromStringAndSize("sha256", 6);
    else
        return PyUnicode_FromStringAndSize("sha224", 6);
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
    {0,0}
};

static PyType_Spec sha224_type_spec = {
    .name = "_sha256.sha224",
    .basicsize = sizeof(SHAobject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = sha256_types_slots
};

static PyType_Spec sha256_type_spec = {
    .name = "_sha256.sha256",
    .basicsize = sizeof(SHAobject),
    .flags = Py_TPFLAGS_DEFAULT,
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

    sha_init(new);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }
    if (string) {
        sha_update(new, buf.buf, buf.len);
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

    sha224_init(new);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string) {
            PyBuffer_Release(&buf);
        }
        return NULL;
    }
    if (string) {
        sha_update(new, buf.buf, buf.len);
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
