/* Set of hash utility functions to help maintaining the invariant that
    if a==b then hash(a)==hash(b)

   All the utility functions (_Py_Hash*()) return "-1" to signify an error.
*/
#include "Python.h"
#include "pycore_pyhash.h"        // _Py_HashSecret_t

#ifdef __APPLE__
#  include <libkern/OSByteOrder.h>
#elif defined(HAVE_LE64TOH) && defined(HAVE_ENDIAN_H)
#  include <endian.h>
#elif defined(HAVE_LE64TOH) && defined(HAVE_SYS_ENDIAN_H)
#  include <sys/endian.h>
#endif

_Py_HashSecret_t _Py_HashSecret = {{0}};

#if Py_HASH_ALGORITHM == Py_HASH_EXTERNAL
extern PyHash_FuncDef PyHash_Func;
#else
static PyHash_FuncDef PyHash_Func;
#endif

/* Count Py_HashBuffer() calls */
#ifdef Py_HASH_STATS
#define Py_HASH_STATS_MAX 32
static Py_ssize_t hashstats[Py_HASH_STATS_MAX + 1] = {0};
#endif

/* For numeric types, the hash of a number x is based on the reduction
   of x modulo the prime P = 2**PyHASH_BITS - 1.  It's designed so that
   hash(x) == hash(y) whenever x and y are numerically equal, even if
   x and y have different types.

   A quick summary of the hashing strategy:

   (1) First define the 'reduction of x modulo P' for any rational
   number x; this is a standard extension of the usual notion of
   reduction modulo P for integers.  If x == p/q (written in lowest
   terms), the reduction is interpreted as the reduction of p times
   the inverse of the reduction of q, all modulo P; if q is exactly
   divisible by P then define the reduction to be infinity.  So we've
   got a well-defined map

      reduce : { rational numbers } -> { 0, 1, 2, ..., P-1, infinity }.

   (2) Now for a rational number x, define hash(x) by:

      reduce(x)   if x >= 0
      -reduce(-x) if x < 0

   If the result of the reduction is infinity (this is impossible for
   integers, floats and Decimals) then use the predefined hash value
   PyHASH_INF for x >= 0, or -PyHASH_INF for x < 0, instead.
   PyHASH_INF and -PyHASH_INF are also used for the
   hashes of float and Decimal infinities.

   NaNs hash with a pointer hash.  Having distinct hash values prevents
   catastrophic pileups from distinct NaN instances which used to always
   have the same hash value but would compare unequal.

   A selling point for the above strategy is that it makes it possible
   to compute hashes of decimal and binary floating-point numbers
   efficiently, even if the exponent of the binary or decimal number
   is large.  The key point is that

      reduce(x * y) == reduce(x) * reduce(y) (modulo PyHASH_MODULUS)

   provided that {reduce(x), reduce(y)} != {0, infinity}.  The reduction of a
   binary or decimal float is never infinity, since the denominator is a power
   of 2 (for binary) or a divisor of a power of 10 (for decimal).  So we have,
   for nonnegative x,

      reduce(x * 2**e) == reduce(x) * reduce(2**e) % PyHASH_MODULUS

      reduce(x * 10**e) == reduce(x) * reduce(10**e) % PyHASH_MODULUS

   and reduce(10**e) can be computed efficiently by the usual modular
   exponentiation algorithm.  For reduce(2**e) it's even better: since
   P is of the form 2**n-1, reduce(2**e) is 2**(e mod n), and multiplication
   by 2**(e mod n) modulo 2**n-1 just amounts to a rotation of bits.

   */

Py_hash_t
_Py_HashDouble(PyObject *inst, double v)
{
    int e, sign;
    double m;
    Py_uhash_t x, y;

    if (!isfinite(v)) {
        if (isinf(v))
            return v > 0 ? PyHASH_INF : -PyHASH_INF;
        else
            return PyObject_GenericHash(inst);
    }

    m = frexp(v, &e);

    sign = 1;
    if (m < 0) {
        sign = -1;
        m = -m;
    }

    /* process 28 bits at a time;  this should work well both for binary
       and hexadecimal floating point. */
    x = 0;
    while (m) {
        x = ((x << 28) & PyHASH_MODULUS) | x >> (PyHASH_BITS - 28);
        m *= 268435456.0;  /* 2**28 */
        e -= 28;
        y = (Py_uhash_t)m;  /* pull out integer part */
        m -= y;
        x += y;
        if (x >= PyHASH_MODULUS)
            x -= PyHASH_MODULUS;
    }

    /* adjust for the exponent;  first reduce it modulo PyHASH_BITS */
    e = e >= 0 ? e % PyHASH_BITS : PyHASH_BITS-1-((-1-e) % PyHASH_BITS);
    x = ((x << e) & PyHASH_MODULUS) | x >> (PyHASH_BITS - e);

    x = x * sign;
    if (x == (Py_uhash_t)-1)
        x = (Py_uhash_t)-2;
    return (Py_hash_t)x;
}

Py_hash_t
Py_HashPointer(const void *ptr)
{
    Py_hash_t hash = _Py_HashPointerRaw(ptr);
    if (hash == -1) {
        hash = -2;
    }
    return hash;
}

Py_hash_t
PyObject_GenericHash(PyObject *obj)
{
    return Py_HashPointer(obj);
}

Py_hash_t
Py_HashBuffer(const void *ptr, Py_ssize_t len)
{
    /*
      We make the hash of the empty string be 0, rather than using
      (prefix ^ suffix), since this slightly obfuscates the hash secret
    */
    if (len == 0) {
        return 0;
    }

#ifdef Py_HASH_STATS
    hashstats[(len <= Py_HASH_STATS_MAX) ? len : 0]++;
#endif

    Py_hash_t x;
#if Py_HASH_CUTOFF > 0
    if (len < Py_HASH_CUTOFF) {
        /* Optimize hashing of very small strings with inline DJBX33A. */
        Py_uhash_t hash;
        const unsigned char *p = ptr;
        hash = 5381; /* DJBX33A starts with 5381 */

        switch(len) {
            /* ((hash << 5) + hash) + *p == hash * 33 + *p */
            case 7: hash = ((hash << 5) + hash) + *p++; _Py_FALLTHROUGH;
            case 6: hash = ((hash << 5) + hash) + *p++; _Py_FALLTHROUGH;
            case 5: hash = ((hash << 5) + hash) + *p++; _Py_FALLTHROUGH;
            case 4: hash = ((hash << 5) + hash) + *p++; _Py_FALLTHROUGH;
            case 3: hash = ((hash << 5) + hash) + *p++; _Py_FALLTHROUGH;
            case 2: hash = ((hash << 5) + hash) + *p++; _Py_FALLTHROUGH;
            case 1: hash = ((hash << 5) + hash) + *p++; break;
            default:
                Py_UNREACHABLE();
        }
        hash ^= len;
        hash ^= (Py_uhash_t) _Py_HashSecret.djbx33a.suffix;
        x = (Py_hash_t)hash;
    }
    else
#endif /* Py_HASH_CUTOFF */
    {
        x = PyHash_Func.hash(ptr, len);
    }

    if (x == -1) {
        return -2;
    }
    return x;
}

void
_PyHash_Fini(void)
{
#ifdef Py_HASH_STATS
    fprintf(stderr, "len   calls    total\n");
    Py_ssize_t total = 0;
    for (int i = 1; i <= Py_HASH_STATS_MAX; i++) {
        total += hashstats[i];
        fprintf(stderr, "%2i %8zd %8zd\n", i, hashstats[i], total);
    }
    total += hashstats[0];
    fprintf(stderr, ">  %8zd %8zd\n", hashstats[0], total);
#endif
}

PyHash_FuncDef *
PyHash_GetFuncDef(void)
{
    return &PyHash_Func;
}

/* Optimized memcpy() for Windows */
#ifdef _MSC_VER
#  if SIZEOF_PY_UHASH_T == 4
#    define PY_UHASH_CPY(dst, src) do {                                    \
       dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3]; \
       } while(0)
#  elif SIZEOF_PY_UHASH_T == 8
#    define PY_UHASH_CPY(dst, src) do {                                    \
       dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3]; \
       dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7]; \
       } while(0)
#  else
#    error SIZEOF_PY_UHASH_T must be 4 or 8
#  endif /* SIZEOF_PY_UHASH_T */
#else /* not Windows */
#  define PY_UHASH_CPY(dst, src) memcpy(dst, src, SIZEOF_PY_UHASH_T)
#endif /* _MSC_VER */


#if Py_HASH_ALGORITHM == Py_HASH_FNV
/* **************************************************************************
 * Modified Fowler-Noll-Vo (FNV) hash function
 */
static Py_hash_t
fnv(const void *src, Py_ssize_t len)
{
    const unsigned char *p = src;
    Py_uhash_t x;
    Py_ssize_t remainder, blocks;
    union {
        Py_uhash_t value;
        unsigned char bytes[SIZEOF_PY_UHASH_T];
    } block;

#ifdef Py_DEBUG
    assert(_Py_HashSecret_Initialized);
#endif
    remainder = len % SIZEOF_PY_UHASH_T;
    if (remainder == 0) {
        /* Process at least one block byte by byte to reduce hash collisions
         * for strings with common prefixes. */
        remainder = SIZEOF_PY_UHASH_T;
    }
    blocks = (len - remainder) / SIZEOF_PY_UHASH_T;

    x = (Py_uhash_t) _Py_HashSecret.fnv.prefix;
    x ^= (Py_uhash_t) *p << 7;
    while (blocks--) {
        PY_UHASH_CPY(block.bytes, p);
        x = (PyHASH_MULTIPLIER * x) ^ block.value;
        p += SIZEOF_PY_UHASH_T;
    }
    /* add remainder */
    for (; remainder > 0; remainder--)
        x = (PyHASH_MULTIPLIER * x) ^ (Py_uhash_t) *p++;
    x ^= (Py_uhash_t) len;
    x ^= (Py_uhash_t) _Py_HashSecret.fnv.suffix;
    if (x == (Py_uhash_t) -1) {
        x = (Py_uhash_t) -2;
    }
    return x;
}

static PyHash_FuncDef PyHash_Func = {fnv, "fnv", 8 * SIZEOF_PY_HASH_T,
                                     16 * SIZEOF_PY_HASH_T};

#endif /* Py_HASH_ALGORITHM == Py_HASH_FNV */


/* **************************************************************************
 <MIT License>
 Copyright (c) 2013  Marek Majkowski <marek@popcount.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 </MIT License>

 Original location:
    https://github.com/majek/csiphash/

 Solution inspired by code from:
    Samuel Neves (supercop/crypto_auth/siphash24/little)
    djb (supercop/crypto_auth/siphash24/little2)
    Jean-Philippe Aumasson (https://131002.net/siphash/siphash24.c)

 Modified for Python by Christian Heimes:
    - C89 / MSVC compatibility
    - _rotl64() on Windows
    - letoh64() fallback
*/

/* byte swap little endian to host endian
 * Endian conversion not only ensures that the hash function returns the same
 * value on all platforms. It is also required to for a good dispersion of
 * the hash values' least significant bits.
 */
#if PY_LITTLE_ENDIAN
#  define _le64toh(x) ((uint64_t)(x))
#elif defined(__APPLE__)
#  define _le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(HAVE_LETOH64)
#  define _le64toh(x) le64toh(x)
#else
#  define _le64toh(x) (((uint64_t)(x) << 56) | \
                      (((uint64_t)(x) << 40) & 0xff000000000000ULL) | \
                      (((uint64_t)(x) << 24) & 0xff0000000000ULL) | \
                      (((uint64_t)(x) << 8)  & 0xff00000000ULL) | \
                      (((uint64_t)(x) >> 8)  & 0xff000000ULL) | \
                      (((uint64_t)(x) >> 24) & 0xff0000ULL) | \
                      (((uint64_t)(x) >> 40) & 0xff00ULL) | \
                      ((uint64_t)(x)  >> 56))
#endif


#ifdef _MSC_VER
#  define ROTATE(x, b)  _rotl64(x, b)
#else
#  define ROTATE(x, b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )
#endif

#define HALF_ROUND(a,b,c,d,s,t)     \
    a += b; c += d;                 \
    b = ROTATE(b, s) ^ a;           \
    d = ROTATE(d, t) ^ c;           \
    a = ROTATE(a, 32);

#define SINGLE_ROUND(v0,v1,v2,v3)   \
    HALF_ROUND(v0,v1,v2,v3,13,16);  \
    HALF_ROUND(v2,v1,v0,v3,17,21);

#define DOUBLE_ROUND(v0,v1,v2,v3)   \
    SINGLE_ROUND(v0,v1,v2,v3);      \
    SINGLE_ROUND(v0,v1,v2,v3);


static uint64_t
siphash13(uint64_t k0, uint64_t k1, const void *src, Py_ssize_t src_sz) {
    uint64_t b = (uint64_t)src_sz << 56;
    const uint8_t *in = (const uint8_t*)src;

    uint64_t v0 = k0 ^ 0x736f6d6570736575ULL;
    uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
    uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
    uint64_t v3 = k1 ^ 0x7465646279746573ULL;

    uint64_t t;
    uint8_t *pt;

    while (src_sz >= 8) {
        uint64_t mi;
        memcpy(&mi, in, sizeof(mi));
        mi = _le64toh(mi);
        in += sizeof(mi);
        src_sz -= sizeof(mi);
        v3 ^= mi;
        SINGLE_ROUND(v0,v1,v2,v3);
        v0 ^= mi;
    }

    t = 0;
    pt = (uint8_t *)&t;
    switch (src_sz) {
        case 7: pt[6] = in[6]; _Py_FALLTHROUGH;
        case 6: pt[5] = in[5]; _Py_FALLTHROUGH;
        case 5: pt[4] = in[4]; _Py_FALLTHROUGH;
        case 4: memcpy(pt, in, sizeof(uint32_t)); break;
        case 3: pt[2] = in[2]; _Py_FALLTHROUGH;
        case 2: pt[1] = in[1]; _Py_FALLTHROUGH;
        case 1: pt[0] = in[0]; break;
    }
    b |= _le64toh(t);

    v3 ^= b;
    SINGLE_ROUND(v0,v1,v2,v3);
    v0 ^= b;
    v2 ^= 0xff;
    SINGLE_ROUND(v0,v1,v2,v3);
    SINGLE_ROUND(v0,v1,v2,v3);
    SINGLE_ROUND(v0,v1,v2,v3);

    /* modified */
    t = (v0 ^ v1) ^ (v2 ^ v3);
    return t;
}

#if Py_HASH_ALGORITHM == Py_HASH_SIPHASH24
static uint64_t
siphash24(uint64_t k0, uint64_t k1, const void *src, Py_ssize_t src_sz) {
    uint64_t b = (uint64_t)src_sz << 56;
    const uint8_t *in = (const uint8_t*)src;

    uint64_t v0 = k0 ^ 0x736f6d6570736575ULL;
    uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
    uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
    uint64_t v3 = k1 ^ 0x7465646279746573ULL;

    uint64_t t;
    uint8_t *pt;

    while (src_sz >= 8) {
        uint64_t mi;
        memcpy(&mi, in, sizeof(mi));
        mi = _le64toh(mi);
        in += sizeof(mi);
        src_sz -= sizeof(mi);
        v3 ^= mi;
        DOUBLE_ROUND(v0,v1,v2,v3);
        v0 ^= mi;
    }

    t = 0;
    pt = (uint8_t *)&t;
    switch (src_sz) {
        case 7: pt[6] = in[6]; _Py_FALLTHROUGH;
        case 6: pt[5] = in[5]; _Py_FALLTHROUGH;
        case 5: pt[4] = in[4]; _Py_FALLTHROUGH;
        case 4: memcpy(pt, in, sizeof(uint32_t)); break;
        case 3: pt[2] = in[2]; _Py_FALLTHROUGH;
        case 2: pt[1] = in[1]; _Py_FALLTHROUGH;
        case 1: pt[0] = in[0]; break;
    }
    b |= _le64toh(t);

    v3 ^= b;
    DOUBLE_ROUND(v0,v1,v2,v3);
    v0 ^= b;
    v2 ^= 0xff;
    DOUBLE_ROUND(v0,v1,v2,v3);
    DOUBLE_ROUND(v0,v1,v2,v3);

    /* modified */
    t = (v0 ^ v1) ^ (v2 ^ v3);
    return t;
}
#endif

uint64_t
_Py_KeyedHash(uint64_t key, const void *src, Py_ssize_t src_sz)
{
    return siphash13(key, 0, src, src_sz);
}


#if Py_HASH_ALGORITHM == Py_HASH_SIPHASH13
static Py_hash_t
pysiphash(const void *src, Py_ssize_t src_sz) {
    return (Py_hash_t)siphash13(
        _le64toh(_Py_HashSecret.siphash.k0), _le64toh(_Py_HashSecret.siphash.k1),
        src, src_sz);
}

static PyHash_FuncDef PyHash_Func = {pysiphash, "siphash13", 64, 128};
#endif

#if Py_HASH_ALGORITHM == Py_HASH_SIPHASH24
static Py_hash_t
pysiphash(const void *src, Py_ssize_t src_sz) {
    return (Py_hash_t)siphash24(
        _le64toh(_Py_HashSecret.siphash.k0), _le64toh(_Py_HashSecret.siphash.k1),
        src, src_sz);
}

static PyHash_FuncDef PyHash_Func = {pysiphash, "siphash24", 64, 128};
#endif
