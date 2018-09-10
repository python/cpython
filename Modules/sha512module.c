/* SHA512 module */

/* This module provides an interface to NIST's SHA-512 and SHA-384 Algorithms */

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
#include "structmember.h"
#include "hashlib.h"
#include "pystrhex.h"

/*[clinic input]
module _sha512
class SHA512Type "SHAobject *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=81a3ccde92bcfe8d]*/

/* Some useful types */

typedef unsigned char SHA_BYTE;

#if SIZEOF_INT == 4
typedef unsigned int SHA_INT32; /* 32-bit integer */
typedef unsigned long long SHA_INT64;        /* 64-bit integer */
#else
/* not defined. compilation will die. */
#endif

/* The SHA block size and message digest sizes, in bytes */

#define SHA_BLOCKSIZE   128
#define SHA_DIGESTSIZE  64

/* The structure for storing SHA info */

typedef struct {
    PyObject_HEAD
    SHA_INT64 digest[8];                /* Message digest */
    SHA_INT32 count_lo, count_hi;       /* 64-bit bit count */
    SHA_BYTE data[SHA_BLOCKSIZE];       /* SHA data buffer */
    int local;                          /* unprocessed amount in data */
    int digestsize;
} SHAobject;

#include "clinic/sha512module.c.h"

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords. */

#if PY_LITTLE_ENDIAN
static void longReverse(SHA_INT64 *buffer, int byteCount)
{
    SHA_INT64 value;

    byteCount /= sizeof(*buffer);
    while (byteCount--) {
        value = *buffer;

                ((unsigned char*)buffer)[0] = (unsigned char)(value >> 56) & 0xff;
                ((unsigned char*)buffer)[1] = (unsigned char)(value >> 48) & 0xff;
                ((unsigned char*)buffer)[2] = (unsigned char)(value >> 40) & 0xff;
                ((unsigned char*)buffer)[3] = (unsigned char)(value >> 32) & 0xff;
                ((unsigned char*)buffer)[4] = (unsigned char)(value >> 24) & 0xff;
                ((unsigned char*)buffer)[5] = (unsigned char)(value >> 16) & 0xff;
                ((unsigned char*)buffer)[6] = (unsigned char)(value >>  8) & 0xff;
                ((unsigned char*)buffer)[7] = (unsigned char)(value      ) & 0xff;

                buffer++;
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
 * This code for the SHA-512 algorithm was noted as public domain. The
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
 * Tom St Denis, tomstdenis@iahu.ca, http://libtom.org
 */


/* SHA512 by Tom St Denis */

/* Various logical functions */
#define ROR64(x, y) \
    ( ((((x) & 0xFFFFFFFFFFFFFFFFULL)>>((unsigned long long)(y) & 63)) | \
      ((x)<<((unsigned long long)(64-((y) & 63))))) & 0xFFFFFFFFFFFFFFFFULL)
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         ROR64((x),(n))
#define R(x, n)         (((x) & 0xFFFFFFFFFFFFFFFFULL) >> ((unsigned long long)n))
#define Sigma0(x)       (S(x, 28) ^ S(x, 34) ^ S(x, 39))
#define Sigma1(x)       (S(x, 14) ^ S(x, 18) ^ S(x, 41))
#define Gamma0(x)       (S(x, 1) ^ S(x, 8) ^ R(x, 7))
#define Gamma1(x)       (S(x, 19) ^ S(x, 61) ^ R(x, 6))


static void
sha512_transform(SHAobject *sha_info)
{
    int i;
    SHA_INT64 S[8], W[80], t0, t1;

    memcpy(W, sha_info->data, sizeof(sha_info->data));
#if PY_LITTLE_ENDIAN
    longReverse(W, (int)sizeof(sha_info->data));
#endif

    for (i = 16; i < 80; ++i) {
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

    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],0,0x428a2f98d728ae22ULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],1,0x7137449123ef65cdULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],2,0xb5c0fbcfec4d3b2fULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],3,0xe9b5dba58189dbbcULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],4,0x3956c25bf348b538ULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],5,0x59f111f1b605d019ULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],6,0x923f82a4af194f9bULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],7,0xab1c5ed5da6d8118ULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],8,0xd807aa98a3030242ULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],9,0x12835b0145706fbeULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],10,0x243185be4ee4b28cULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],11,0x550c7dc3d5ffb4e2ULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],12,0x72be5d74f27b896fULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],13,0x80deb1fe3b1696b1ULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],14,0x9bdc06a725c71235ULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],15,0xc19bf174cf692694ULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],16,0xe49b69c19ef14ad2ULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],17,0xefbe4786384f25e3ULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],18,0x0fc19dc68b8cd5b5ULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],19,0x240ca1cc77ac9c65ULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],20,0x2de92c6f592b0275ULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],21,0x4a7484aa6ea6e483ULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],22,0x5cb0a9dcbd41fbd4ULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],23,0x76f988da831153b5ULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],24,0x983e5152ee66dfabULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],25,0xa831c66d2db43210ULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],26,0xb00327c898fb213fULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],27,0xbf597fc7beef0ee4ULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],28,0xc6e00bf33da88fc2ULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],29,0xd5a79147930aa725ULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],30,0x06ca6351e003826fULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],31,0x142929670a0e6e70ULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],32,0x27b70a8546d22ffcULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],33,0x2e1b21385c26c926ULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],34,0x4d2c6dfc5ac42aedULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],35,0x53380d139d95b3dfULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],36,0x650a73548baf63deULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],37,0x766a0abb3c77b2a8ULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],38,0x81c2c92e47edaee6ULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],39,0x92722c851482353bULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],40,0xa2bfe8a14cf10364ULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],41,0xa81a664bbc423001ULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],42,0xc24b8b70d0f89791ULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],43,0xc76c51a30654be30ULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],44,0xd192e819d6ef5218ULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],45,0xd69906245565a910ULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],46,0xf40e35855771202aULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],47,0x106aa07032bbd1b8ULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],48,0x19a4c116b8d2d0c8ULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],49,0x1e376c085141ab53ULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],50,0x2748774cdf8eeb99ULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],51,0x34b0bcb5e19b48a8ULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],52,0x391c0cb3c5c95a63ULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],53,0x4ed8aa4ae3418acbULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],54,0x5b9cca4f7763e373ULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],55,0x682e6ff3d6b2b8a3ULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],56,0x748f82ee5defb2fcULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],57,0x78a5636f43172f60ULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],58,0x84c87814a1f0ab72ULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],59,0x8cc702081a6439ecULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],60,0x90befffa23631e28ULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],61,0xa4506cebde82bde9ULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],62,0xbef9a3f7b2c67915ULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],63,0xc67178f2e372532bULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],64,0xca273eceea26619cULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],65,0xd186b8c721c0c207ULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],66,0xeada7dd6cde0eb1eULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],67,0xf57d4f7fee6ed178ULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],68,0x06f067aa72176fbaULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],69,0x0a637dc5a2c898a6ULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],70,0x113f9804bef90daeULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],71,0x1b710b35131c471bULL);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],72,0x28db77f523047d84ULL);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],73,0x32caab7b40c72493ULL);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],74,0x3c9ebe0a15c9bebcULL);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],75,0x431d67c49c100d4cULL);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],76,0x4cc5d4becb3e42b6ULL);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],77,0x597f299cfc657e2aULL);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],78,0x5fcb6fab3ad6faecULL);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],79,0x6c44198c4a475817ULL);

#undef RND

    /* feedback */
    for (i = 0; i < 8; i++) {
        sha_info->digest[i] = sha_info->digest[i] + S[i];
    }

}



/* initialize the SHA digest */

static void
sha512_init(SHAobject *sha_info)
{
    sha_info->digest[0] = Py_ULL(0x6a09e667f3bcc908);
    sha_info->digest[1] = Py_ULL(0xbb67ae8584caa73b);
    sha_info->digest[2] = Py_ULL(0x3c6ef372fe94f82b);
    sha_info->digest[3] = Py_ULL(0xa54ff53a5f1d36f1);
    sha_info->digest[4] = Py_ULL(0x510e527fade682d1);
    sha_info->digest[5] = Py_ULL(0x9b05688c2b3e6c1f);
    sha_info->digest[6] = Py_ULL(0x1f83d9abfb41bd6b);
    sha_info->digest[7] = Py_ULL(0x5be0cd19137e2179);
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
    sha_info->digestsize = 64;
}

static void
sha384_init(SHAobject *sha_info)
{
    sha_info->digest[0] = Py_ULL(0xcbbb9d5dc1059ed8);
    sha_info->digest[1] = Py_ULL(0x629a292a367cd507);
    sha_info->digest[2] = Py_ULL(0x9159015a3070dd17);
    sha_info->digest[3] = Py_ULL(0x152fecd8f70e5939);
    sha_info->digest[4] = Py_ULL(0x67332667ffc00b31);
    sha_info->digest[5] = Py_ULL(0x8eb44a8768581511);
    sha_info->digest[6] = Py_ULL(0xdb0c2e0d64f98fa7);
    sha_info->digest[7] = Py_ULL(0x47b5481dbefa4fa4);
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
    sha_info->digestsize = 48;
}


/* update the SHA digest */

static void
sha512_update(SHAobject *sha_info, SHA_BYTE *buffer, Py_ssize_t count)
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
            sha512_transform(sha_info);
        }
        else {
            return;
        }
    }
    while (count >= SHA_BLOCKSIZE) {
        memcpy(sha_info->data, buffer, SHA_BLOCKSIZE);
        buffer += SHA_BLOCKSIZE;
        count -= SHA_BLOCKSIZE;
        sha512_transform(sha_info);
    }
    memcpy(sha_info->data, buffer, count);
    sha_info->local = (int)count;
}

/* finish computing the SHA digest */

static void
sha512_final(unsigned char digest[SHA_DIGESTSIZE], SHAobject *sha_info)
{
    int count;
    SHA_INT32 lo_bit_count, hi_bit_count;

    lo_bit_count = sha_info->count_lo;
    hi_bit_count = sha_info->count_hi;
    count = (int) ((lo_bit_count >> 3) & 0x7f);
    ((SHA_BYTE *) sha_info->data)[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 16) {
        memset(((SHA_BYTE *) sha_info->data) + count, 0,
               SHA_BLOCKSIZE - count);
        sha512_transform(sha_info);
        memset((SHA_BYTE *) sha_info->data, 0, SHA_BLOCKSIZE - 16);
    }
    else {
        memset(((SHA_BYTE *) sha_info->data) + count, 0,
               SHA_BLOCKSIZE - 16 - count);
    }

    /* GJS: note that we add the hi/lo in big-endian. sha512_transform will
       swap these values into host-order. */
    sha_info->data[112] = 0;
    sha_info->data[113] = 0;
    sha_info->data[114] = 0;
    sha_info->data[115] = 0;
    sha_info->data[116] = 0;
    sha_info->data[117] = 0;
    sha_info->data[118] = 0;
    sha_info->data[119] = 0;
    sha_info->data[120] = (hi_bit_count >> 24) & 0xff;
    sha_info->data[121] = (hi_bit_count >> 16) & 0xff;
    sha_info->data[122] = (hi_bit_count >>  8) & 0xff;
    sha_info->data[123] = (hi_bit_count >>  0) & 0xff;
    sha_info->data[124] = (lo_bit_count >> 24) & 0xff;
    sha_info->data[125] = (lo_bit_count >> 16) & 0xff;
    sha_info->data[126] = (lo_bit_count >>  8) & 0xff;
    sha_info->data[127] = (lo_bit_count >>  0) & 0xff;
    sha512_transform(sha_info);
    digest[ 0] = (unsigned char) ((sha_info->digest[0] >> 56) & 0xff);
    digest[ 1] = (unsigned char) ((sha_info->digest[0] >> 48) & 0xff);
    digest[ 2] = (unsigned char) ((sha_info->digest[0] >> 40) & 0xff);
    digest[ 3] = (unsigned char) ((sha_info->digest[0] >> 32) & 0xff);
    digest[ 4] = (unsigned char) ((sha_info->digest[0] >> 24) & 0xff);
    digest[ 5] = (unsigned char) ((sha_info->digest[0] >> 16) & 0xff);
    digest[ 6] = (unsigned char) ((sha_info->digest[0] >>  8) & 0xff);
    digest[ 7] = (unsigned char) ((sha_info->digest[0]      ) & 0xff);
    digest[ 8] = (unsigned char) ((sha_info->digest[1] >> 56) & 0xff);
    digest[ 9] = (unsigned char) ((sha_info->digest[1] >> 48) & 0xff);
    digest[10] = (unsigned char) ((sha_info->digest[1] >> 40) & 0xff);
    digest[11] = (unsigned char) ((sha_info->digest[1] >> 32) & 0xff);
    digest[12] = (unsigned char) ((sha_info->digest[1] >> 24) & 0xff);
    digest[13] = (unsigned char) ((sha_info->digest[1] >> 16) & 0xff);
    digest[14] = (unsigned char) ((sha_info->digest[1] >>  8) & 0xff);
    digest[15] = (unsigned char) ((sha_info->digest[1]      ) & 0xff);
    digest[16] = (unsigned char) ((sha_info->digest[2] >> 56) & 0xff);
    digest[17] = (unsigned char) ((sha_info->digest[2] >> 48) & 0xff);
    digest[18] = (unsigned char) ((sha_info->digest[2] >> 40) & 0xff);
    digest[19] = (unsigned char) ((sha_info->digest[2] >> 32) & 0xff);
    digest[20] = (unsigned char) ((sha_info->digest[2] >> 24) & 0xff);
    digest[21] = (unsigned char) ((sha_info->digest[2] >> 16) & 0xff);
    digest[22] = (unsigned char) ((sha_info->digest[2] >>  8) & 0xff);
    digest[23] = (unsigned char) ((sha_info->digest[2]      ) & 0xff);
    digest[24] = (unsigned char) ((sha_info->digest[3] >> 56) & 0xff);
    digest[25] = (unsigned char) ((sha_info->digest[3] >> 48) & 0xff);
    digest[26] = (unsigned char) ((sha_info->digest[3] >> 40) & 0xff);
    digest[27] = (unsigned char) ((sha_info->digest[3] >> 32) & 0xff);
    digest[28] = (unsigned char) ((sha_info->digest[3] >> 24) & 0xff);
    digest[29] = (unsigned char) ((sha_info->digest[3] >> 16) & 0xff);
    digest[30] = (unsigned char) ((sha_info->digest[3] >>  8) & 0xff);
    digest[31] = (unsigned char) ((sha_info->digest[3]      ) & 0xff);
    digest[32] = (unsigned char) ((sha_info->digest[4] >> 56) & 0xff);
    digest[33] = (unsigned char) ((sha_info->digest[4] >> 48) & 0xff);
    digest[34] = (unsigned char) ((sha_info->digest[4] >> 40) & 0xff);
    digest[35] = (unsigned char) ((sha_info->digest[4] >> 32) & 0xff);
    digest[36] = (unsigned char) ((sha_info->digest[4] >> 24) & 0xff);
    digest[37] = (unsigned char) ((sha_info->digest[4] >> 16) & 0xff);
    digest[38] = (unsigned char) ((sha_info->digest[4] >>  8) & 0xff);
    digest[39] = (unsigned char) ((sha_info->digest[4]      ) & 0xff);
    digest[40] = (unsigned char) ((sha_info->digest[5] >> 56) & 0xff);
    digest[41] = (unsigned char) ((sha_info->digest[5] >> 48) & 0xff);
    digest[42] = (unsigned char) ((sha_info->digest[5] >> 40) & 0xff);
    digest[43] = (unsigned char) ((sha_info->digest[5] >> 32) & 0xff);
    digest[44] = (unsigned char) ((sha_info->digest[5] >> 24) & 0xff);
    digest[45] = (unsigned char) ((sha_info->digest[5] >> 16) & 0xff);
    digest[46] = (unsigned char) ((sha_info->digest[5] >>  8) & 0xff);
    digest[47] = (unsigned char) ((sha_info->digest[5]      ) & 0xff);
    digest[48] = (unsigned char) ((sha_info->digest[6] >> 56) & 0xff);
    digest[49] = (unsigned char) ((sha_info->digest[6] >> 48) & 0xff);
    digest[50] = (unsigned char) ((sha_info->digest[6] >> 40) & 0xff);
    digest[51] = (unsigned char) ((sha_info->digest[6] >> 32) & 0xff);
    digest[52] = (unsigned char) ((sha_info->digest[6] >> 24) & 0xff);
    digest[53] = (unsigned char) ((sha_info->digest[6] >> 16) & 0xff);
    digest[54] = (unsigned char) ((sha_info->digest[6] >>  8) & 0xff);
    digest[55] = (unsigned char) ((sha_info->digest[6]      ) & 0xff);
    digest[56] = (unsigned char) ((sha_info->digest[7] >> 56) & 0xff);
    digest[57] = (unsigned char) ((sha_info->digest[7] >> 48) & 0xff);
    digest[58] = (unsigned char) ((sha_info->digest[7] >> 40) & 0xff);
    digest[59] = (unsigned char) ((sha_info->digest[7] >> 32) & 0xff);
    digest[60] = (unsigned char) ((sha_info->digest[7] >> 24) & 0xff);
    digest[61] = (unsigned char) ((sha_info->digest[7] >> 16) & 0xff);
    digest[62] = (unsigned char) ((sha_info->digest[7] >>  8) & 0xff);
    digest[63] = (unsigned char) ((sha_info->digest[7]      ) & 0xff);
}

/*
 * End of copied SHA code.
 *
 * ------------------------------------------------------------------------
 */

static PyTypeObject SHA384type;
static PyTypeObject SHA512type;


static SHAobject *
newSHA384object(void)
{
    return (SHAobject *)PyObject_New(SHAobject, &SHA384type);
}

static SHAobject *
newSHA512object(void)
{
    return (SHAobject *)PyObject_New(SHAobject, &SHA512type);
}

/* Internal methods for a hash object */

static void
SHA512_dealloc(PyObject *ptr)
{
    PyObject_Del(ptr);
}


/* External methods for a hash object */

/*[clinic input]
SHA512Type.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
SHA512Type_copy_impl(SHAobject *self)
/*[clinic end generated code: output=adea896ed3164821 input=9f5f31e6c457776a]*/
{
    SHAobject *newobj;

    if (((PyObject*)self)->ob_type == &SHA512type) {
        if ( (newobj = newSHA512object())==NULL)
            return NULL;
    } else {
        if ( (newobj = newSHA384object())==NULL)
            return NULL;
    }

    SHAcopy(self, newobj);
    return (PyObject *)newobj;
}

/*[clinic input]
SHA512Type.digest

Return the digest value as a string of binary data.
[clinic start generated code]*/

static PyObject *
SHA512Type_digest_impl(SHAobject *self)
/*[clinic end generated code: output=1080bbeeef7dde1b input=60c2cede9e023018]*/
{
    unsigned char digest[SHA_DIGESTSIZE];
    SHAobject temp;

    SHAcopy(self, &temp);
    sha512_final(digest, &temp);
    return PyBytes_FromStringAndSize((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA512Type.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
SHA512Type_hexdigest_impl(SHAobject *self)
/*[clinic end generated code: output=7373305b8601e18b input=498b877b25cbe0a2]*/
{
    unsigned char digest[SHA_DIGESTSIZE];
    SHAobject temp;

    /* Get the raw (binary) digest value */
    SHAcopy(self, &temp);
    sha512_final(digest, &temp);

    return _Py_strhex((const char *)digest, self->digestsize);
}

/*[clinic input]
SHA512Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
SHA512Type_update(SHAobject *self, PyObject *obj)
/*[clinic end generated code: output=1cf333e73995a79e input=ded2b46656566283]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);

    sha512_update(self, buf.buf, buf.len);

    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static PyMethodDef SHA_methods[] = {
    SHA512TYPE_COPY_METHODDEF
    SHA512TYPE_DIGEST_METHODDEF
    SHA512TYPE_HEXDIGEST_METHODDEF
    SHA512TYPE_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

static PyObject *
SHA512_get_block_size(PyObject *self, void *closure)
{
    return PyLong_FromLong(SHA_BLOCKSIZE);
}

static PyObject *
SHA512_get_name(PyObject *self, void *closure)
{
    if (((SHAobject *)self)->digestsize == 64)
        return PyUnicode_FromStringAndSize("sha512", 6);
    else
        return PyUnicode_FromStringAndSize("sha384", 6);
}

static PyGetSetDef SHA_getseters[] = {
    {"block_size",
     (getter)SHA512_get_block_size, NULL,
     NULL,
     NULL},
    {"name",
     (getter)SHA512_get_name, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};

static PyMemberDef SHA_members[] = {
    {"digest_size", T_INT, offsetof(SHAobject, digestsize), READONLY, NULL},
    {NULL}  /* Sentinel */
};

static PyTypeObject SHA384type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_sha512.sha384",   /*tp_name*/
    sizeof(SHAobject),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    /* methods */
    SHA512_dealloc,     /*tp_dealloc*/
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
    SHA_methods,        /* tp_methods */
    SHA_members,        /* tp_members */
    SHA_getseters,      /* tp_getset */
};

static PyTypeObject SHA512type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_sha512.sha512",   /*tp_name*/
    sizeof(SHAobject),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    /* methods */
    SHA512_dealloc,     /*tp_dealloc*/
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
    SHA_methods,        /* tp_methods */
    SHA_members,        /* tp_members */
    SHA_getseters,      /* tp_getset */
};


/* The single module-level function: new() */

/*[clinic input]
_sha512.sha512

    string: object(c_default="NULL") = b''

Return a new SHA-512 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha512_sha512_impl(PyObject *module, PyObject *string)
/*[clinic end generated code: output=8b865a2df73bd387 input=e69bad9ae9b6a308]*/
{
    SHAobject *new;
    Py_buffer buf;

    if (string)
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);

    if ((new = newSHA512object()) == NULL) {
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }

    sha512_init(new);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }
    if (string) {
        sha512_update(new, buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}

/*[clinic input]
_sha512.sha384

    string: object(c_default="NULL") = b''

Return a new SHA-384 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyObject *
_sha512_sha384_impl(PyObject *module, PyObject *string)
/*[clinic end generated code: output=ae4b2e26decf81e8 input=c9327788d4ea4545]*/
{
    SHAobject *new;
    Py_buffer buf;

    if (string)
        GET_BUFFER_VIEW_OR_ERROUT(string, &buf);

    if ((new = newSHA384object()) == NULL) {
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }

    sha384_init(new);

    if (PyErr_Occurred()) {
        Py_DECREF(new);
        if (string)
            PyBuffer_Release(&buf);
        return NULL;
    }
    if (string) {
        sha512_update(new, buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }

    return (PyObject *)new;
}


/* List of functions exported by this module */

static struct PyMethodDef SHA_functions[] = {
    _SHA512_SHA512_METHODDEF
    _SHA512_SHA384_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};


/* Initialize this module. */

#define insint(n,v) { PyModule_AddIntConstant(m,n,v); }


static struct PyModuleDef _sha512module = {
        PyModuleDef_HEAD_INIT,
        "_sha512",
        NULL,
        -1,
        SHA_functions,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__sha512(void)
{
    PyObject *m;

    Py_TYPE(&SHA384type) = &PyType_Type;
    if (PyType_Ready(&SHA384type) < 0)
        return NULL;
    Py_TYPE(&SHA512type) = &PyType_Type;
    if (PyType_Ready(&SHA512type) < 0)
        return NULL;

    m = PyModule_Create(&_sha512module);
    if (m == NULL)
        return NULL;

    Py_INCREF((PyObject *)&SHA384type);
    PyModule_AddObject(m, "SHA384Type", (PyObject *)&SHA384type);
    Py_INCREF((PyObject *)&SHA512type);
    PyModule_AddObject(m, "SHA512Type", (PyObject *)&SHA512type);
    return m;
}
