/*
 * Copyright (c) 2008-2016 Stefan Krah. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef TYPEARITH_H
#define TYPEARITH_H


#include "mpdecimal.h"


/*****************************************************************************/
/*                 Low level native arithmetic on basic types                */
/*****************************************************************************/


/** ------------------------------------------------------------
 **           Double width multiplication and division
 ** ------------------------------------------------------------
 */

#if defined(CONFIG_64)
#if defined(ANSI)
#if defined(HAVE_UINT128_T)
static inline void
_mpd_mul_words(mpd_uint_t *hi, mpd_uint_t *lo, mpd_uint_t a, mpd_uint_t b)
{
    __uint128_t hl;

    hl = (__uint128_t)a * b;

    *hi = hl >> 64;
    *lo = (mpd_uint_t)hl;
}

static inline void
_mpd_div_words(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t hi, mpd_uint_t lo,
               mpd_uint_t d)
{
    __uint128_t hl;

    hl = ((__uint128_t)hi<<64) + lo;
    *q = (mpd_uint_t)(hl / d); /* quotient is known to fit */
    *r = (mpd_uint_t)(hl - (__uint128_t)(*q) * d);
}
#else
static inline void
_mpd_mul_words(mpd_uint_t *hi, mpd_uint_t *lo, mpd_uint_t a, mpd_uint_t b)
{
    uint32_t w[4], carry;
    uint32_t ah, al, bh, bl;
    uint64_t hl;

    ah = (uint32_t)(a>>32); al = (uint32_t)a;
    bh = (uint32_t)(b>>32); bl = (uint32_t)b;

    hl = (uint64_t)al * bl;
    w[0] = (uint32_t)hl;
    carry = (uint32_t)(hl>>32);

    hl = (uint64_t)ah * bl + carry;
    w[1] = (uint32_t)hl;
    w[2] = (uint32_t)(hl>>32);

    hl = (uint64_t)al * bh + w[1];
    w[1] = (uint32_t)hl;
    carry = (uint32_t)(hl>>32);

    hl = ((uint64_t)ah * bh + w[2]) + carry;
    w[2] = (uint32_t)hl;
    w[3] = (uint32_t)(hl>>32);

    *hi = ((uint64_t)w[3]<<32) + w[2];
    *lo = ((uint64_t)w[1]<<32) + w[0];
}

/*
 * By Henry S. Warren: http://www.hackersdelight.org/HDcode/divlu.c.txt
 * http://www.hackersdelight.org/permissions.htm:
 * "You are free to use, copy, and distribute any of the code on this web
 *  site, whether modified by you or not. You need not give attribution."
 *
 * Slightly modified, comments are mine.
 */
static inline int
nlz(uint64_t x)
{
    int n;

    if (x == 0) return(64);

    n = 0;
    if (x <= 0x00000000FFFFFFFF) {n = n +32; x = x <<32;}
    if (x <= 0x0000FFFFFFFFFFFF) {n = n +16; x = x <<16;}
    if (x <= 0x00FFFFFFFFFFFFFF) {n = n + 8; x = x << 8;}
    if (x <= 0x0FFFFFFFFFFFFFFF) {n = n + 4; x = x << 4;}
    if (x <= 0x3FFFFFFFFFFFFFFF) {n = n + 2; x = x << 2;}
    if (x <= 0x7FFFFFFFFFFFFFFF) {n = n + 1;}

    return n;
}

static inline void
_mpd_div_words(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t u1, mpd_uint_t u0,
               mpd_uint_t v)
{
    const mpd_uint_t b = 4294967296;
    mpd_uint_t un1, un0,
               vn1, vn0,
               q1, q0,
               un32, un21, un10,
               rhat, t;
    int s;

    assert(u1 < v);

    s = nlz(v);
    v = v << s;
    vn1 = v >> 32;
    vn0 = v & 0xFFFFFFFF;

    t = (s == 0) ? 0 : u0 >> (64 - s);
    un32 = (u1 << s) | t;
    un10 = u0 << s;

    un1 = un10 >> 32;
    un0 = un10 & 0xFFFFFFFF;

    q1 = un32 / vn1;
    rhat = un32 - q1*vn1;
again1:
    if (q1 >= b || q1*vn0 > b*rhat + un1) {
        q1 = q1 - 1;
        rhat = rhat + vn1;
        if (rhat < b) goto again1;
    }

    /*
     *  Before again1 we had:
     *      (1) q1*vn1   + rhat         = un32
     *      (2) q1*vn1*b + rhat*b + un1 = un32*b + un1
     *
     *  The statements inside the if-clause do not change the value
     *  of the left-hand side of (2), and the loop is only exited
     *  if q1*vn0 <= rhat*b + un1, so:
     *
     *      (3) q1*vn1*b + q1*vn0 <= un32*b + un1
     *      (4)              q1*v <= un32*b + un1
     *      (5)                 0 <= un32*b + un1 - q1*v
     *
     *  By (5) we are certain that the possible add-back step from
     *  Knuth's algorithm D is never required.
     *
     *  Since the final quotient is less than 2**64, the following
     *  must be true:
     *
     *      (6) un32*b + un1 - q1*v <= UINT64_MAX
     *
     *  This means that in the following line, the high words
     *  of un32*b and q1*v can be discarded without any effect
     *  on the result.
     */
    un21 = un32*b + un1 - q1*v;

    q0 = un21 / vn1;
    rhat = un21 - q0*vn1;
again2:
    if (q0 >= b || q0*vn0 > b*rhat + un0) {
        q0 = q0 - 1;
        rhat = rhat + vn1;
        if (rhat < b) goto again2;
    }

    *q = q1*b + q0;
    *r = (un21*b + un0 - q0*v) >> s;
}
#endif

/* END ANSI */
#elif defined(ASM)
static inline void
_mpd_mul_words(mpd_uint_t *hi, mpd_uint_t *lo, mpd_uint_t a, mpd_uint_t b)
{
    mpd_uint_t h, l;

    __asm__ ( "mulq %3\n\t"
              : "=d" (h), "=a" (l)
              : "%a" (a), "rm" (b)
              : "cc"
    );

    *hi = h;
    *lo = l;
}

static inline void
_mpd_div_words(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t hi, mpd_uint_t lo,
               mpd_uint_t d)
{
    mpd_uint_t qq, rr;

    __asm__ ( "divq %4\n\t"
              : "=a" (qq), "=d" (rr)
              : "a" (lo), "d" (hi), "rm" (d)
              : "cc"
    );

    *q = qq;
    *r = rr;
}
/* END GCC ASM */
#elif defined(MASM)
#include <intrin.h>
#pragma intrinsic(_umul128)

static inline void
_mpd_mul_words(mpd_uint_t *hi, mpd_uint_t *lo, mpd_uint_t a, mpd_uint_t b)
{
    *lo = _umul128(a, b, hi);
}

void _mpd_div_words(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t hi, mpd_uint_t lo,
                    mpd_uint_t d);

/* END MASM (_MSC_VER) */
#else
  #error "need platform specific 128 bit multiplication and division"
#endif

#define DIVMOD(q, r, v, d) *q = v / d; *r = v - *q * d
static inline void
_mpd_divmod_pow10(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t v, mpd_uint_t exp)
{
    assert(exp <= 19);

    if (exp <= 9) {
        if (exp <= 4) {
            switch (exp) {
            case 0: *q = v; *r = 0; break;
            case 1: DIVMOD(q, r, v, 10UL); break;
            case 2: DIVMOD(q, r, v, 100UL); break;
            case 3: DIVMOD(q, r, v, 1000UL); break;
            case 4: DIVMOD(q, r, v, 10000UL); break;
            }
        }
        else {
            switch (exp) {
            case 5: DIVMOD(q, r, v, 100000UL); break;
            case 6: DIVMOD(q, r, v, 1000000UL); break;
            case 7: DIVMOD(q, r, v, 10000000UL); break;
            case 8: DIVMOD(q, r, v, 100000000UL); break;
            case 9: DIVMOD(q, r, v, 1000000000UL); break;
            }
        }
    }
    else {
        if (exp <= 14) {
            switch (exp) {
            case 10: DIVMOD(q, r, v, 10000000000ULL); break;
            case 11: DIVMOD(q, r, v, 100000000000ULL); break;
            case 12: DIVMOD(q, r, v, 1000000000000ULL); break;
            case 13: DIVMOD(q, r, v, 10000000000000ULL); break;
            case 14: DIVMOD(q, r, v, 100000000000000ULL); break;
            }
        }
        else {
            switch (exp) {
            case 15: DIVMOD(q, r, v, 1000000000000000ULL); break;
            case 16: DIVMOD(q, r, v, 10000000000000000ULL); break;
            case 17: DIVMOD(q, r, v, 100000000000000000ULL); break;
            case 18: DIVMOD(q, r, v, 1000000000000000000ULL); break;
            case 19: DIVMOD(q, r, v, 10000000000000000000ULL); break; /* GCOV_NOT_REACHED */
            }
        }
    }
}

/* END CONFIG_64 */
#elif defined(CONFIG_32)
#if defined(ANSI)
#if !defined(LEGACY_COMPILER)
static inline void
_mpd_mul_words(mpd_uint_t *hi, mpd_uint_t *lo, mpd_uint_t a, mpd_uint_t b)
{
    mpd_uuint_t hl;

    hl = (mpd_uuint_t)a * b;

    *hi = hl >> 32;
    *lo = (mpd_uint_t)hl;
}

static inline void
_mpd_div_words(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t hi, mpd_uint_t lo,
               mpd_uint_t d)
{
    mpd_uuint_t hl;

    hl = ((mpd_uuint_t)hi<<32) + lo;
    *q = (mpd_uint_t)(hl / d); /* quotient is known to fit */
    *r = (mpd_uint_t)(hl - (mpd_uuint_t)(*q) * d);
}
/* END ANSI + uint64_t */
#else
static inline void
_mpd_mul_words(mpd_uint_t *hi, mpd_uint_t *lo, mpd_uint_t a, mpd_uint_t b)
{
    uint16_t w[4], carry;
    uint16_t ah, al, bh, bl;
    uint32_t hl;

    ah = (uint16_t)(a>>16); al = (uint16_t)a;
    bh = (uint16_t)(b>>16); bl = (uint16_t)b;

    hl = (uint32_t)al * bl;
    w[0] = (uint16_t)hl;
    carry = (uint16_t)(hl>>16);

    hl = (uint32_t)ah * bl + carry;
    w[1] = (uint16_t)hl;
    w[2] = (uint16_t)(hl>>16);

    hl = (uint32_t)al * bh + w[1];
    w[1] = (uint16_t)hl;
    carry = (uint16_t)(hl>>16);

    hl = ((uint32_t)ah * bh + w[2]) + carry;
    w[2] = (uint16_t)hl;
    w[3] = (uint16_t)(hl>>16);

    *hi = ((uint32_t)w[3]<<16) + w[2];
    *lo = ((uint32_t)w[1]<<16) + w[0];
}

/*
 * By Henry S. Warren: http://www.hackersdelight.org/HDcode/divlu.c.txt
 * http://www.hackersdelight.org/permissions.htm:
 * "You are free to use, copy, and distribute any of the code on this web
 *  site, whether modified by you or not. You need not give attribution."
 *
 * Slightly modified, comments are mine.
 */
static inline int
nlz(uint32_t x)
{
    int n;

    if (x == 0) return(32);

    n = 0;
    if (x <= 0x0000FFFF) {n = n +16; x = x <<16;}
    if (x <= 0x00FFFFFF) {n = n + 8; x = x << 8;}
    if (x <= 0x0FFFFFFF) {n = n + 4; x = x << 4;}
    if (x <= 0x3FFFFFFF) {n = n + 2; x = x << 2;}
    if (x <= 0x7FFFFFFF) {n = n + 1;}

    return n;
}

static inline void
_mpd_div_words(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t u1, mpd_uint_t u0,
               mpd_uint_t v)
{
    const mpd_uint_t b = 65536;
    mpd_uint_t un1, un0,
               vn1, vn0,
               q1, q0,
               un32, un21, un10,
               rhat, t;
    int s;

    assert(u1 < v);

    s = nlz(v);
    v = v << s;
    vn1 = v >> 16;
    vn0 = v & 0xFFFF;

    t = (s == 0) ? 0 : u0 >> (32 - s);
    un32 = (u1 << s) | t;
    un10 = u0 << s;

    un1 = un10 >> 16;
    un0 = un10 & 0xFFFF;

    q1 = un32 / vn1;
    rhat = un32 - q1*vn1;
again1:
    if (q1 >= b || q1*vn0 > b*rhat + un1) {
        q1 = q1 - 1;
        rhat = rhat + vn1;
        if (rhat < b) goto again1;
    }

    /*
     *  Before again1 we had:
     *      (1) q1*vn1   + rhat         = un32
     *      (2) q1*vn1*b + rhat*b + un1 = un32*b + un1
     *
     *  The statements inside the if-clause do not change the value
     *  of the left-hand side of (2), and the loop is only exited
     *  if q1*vn0 <= rhat*b + un1, so:
     *
     *      (3) q1*vn1*b + q1*vn0 <= un32*b + un1
     *      (4)              q1*v <= un32*b + un1
     *      (5)                 0 <= un32*b + un1 - q1*v
     *
     *  By (5) we are certain that the possible add-back step from
     *  Knuth's algorithm D is never required.
     *
     *  Since the final quotient is less than 2**32, the following
     *  must be true:
     *
     *      (6) un32*b + un1 - q1*v <= UINT32_MAX
     *
     *  This means that in the following line, the high words
     *  of un32*b and q1*v can be discarded without any effect
     *  on the result.
     */
    un21 = un32*b + un1 - q1*v;

    q0 = un21 / vn1;
    rhat = un21 - q0*vn1;
again2:
    if (q0 >= b || q0*vn0 > b*rhat + un0) {
        q0 = q0 - 1;
        rhat = rhat + vn1;
        if (rhat < b) goto again2;
    }

    *q = q1*b + q0;
    *r = (un21*b + un0 - q0*v) >> s;
}
#endif /* END ANSI + LEGACY_COMPILER */

/* END ANSI */
#elif defined(ASM)
static inline void
_mpd_mul_words(mpd_uint_t *hi, mpd_uint_t *lo, mpd_uint_t a, mpd_uint_t b)
{
    mpd_uint_t h, l;

    __asm__ ( "mull %3\n\t"
              : "=d" (h), "=a" (l)
              : "%a" (a), "rm" (b)
              : "cc"
    );

    *hi = h;
    *lo = l;
}

static inline void
_mpd_div_words(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t hi, mpd_uint_t lo,
               mpd_uint_t d)
{
    mpd_uint_t qq, rr;

    __asm__ ( "divl %4\n\t"
              : "=a" (qq), "=d" (rr)
              : "a" (lo), "d" (hi), "rm" (d)
              : "cc"
    );

    *q = qq;
    *r = rr;
}
/* END GCC ASM */
#elif defined(MASM)
static inline void __cdecl
_mpd_mul_words(mpd_uint_t *hi, mpd_uint_t *lo, mpd_uint_t a, mpd_uint_t b)
{
    mpd_uint_t h, l;

    __asm {
        mov eax, a
        mul b
        mov h, edx
        mov l, eax
    }

    *hi = h;
    *lo = l;
}

static inline void __cdecl
_mpd_div_words(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t hi, mpd_uint_t lo,
               mpd_uint_t d)
{
    mpd_uint_t qq, rr;

    __asm {
        mov eax, lo
        mov edx, hi
        div d
        mov qq, eax
        mov rr, edx
    }

    *q = qq;
    *r = rr;
}
/* END MASM (_MSC_VER) */
#else
  #error "need platform specific 64 bit multiplication and division"
#endif

#define DIVMOD(q, r, v, d) *q = v / d; *r = v - *q * d
static inline void
_mpd_divmod_pow10(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t v, mpd_uint_t exp)
{
    assert(exp <= 9);

    if (exp <= 4) {
        switch (exp) {
        case 0: *q = v; *r = 0; break;
        case 1: DIVMOD(q, r, v, 10UL); break;
        case 2: DIVMOD(q, r, v, 100UL); break;
        case 3: DIVMOD(q, r, v, 1000UL); break;
        case 4: DIVMOD(q, r, v, 10000UL); break;
        }
    }
    else {
        switch (exp) {
        case 5: DIVMOD(q, r, v, 100000UL); break;
        case 6: DIVMOD(q, r, v, 1000000UL); break;
        case 7: DIVMOD(q, r, v, 10000000UL); break;
        case 8: DIVMOD(q, r, v, 100000000UL); break;
        case 9: DIVMOD(q, r, v, 1000000000UL); break; /* GCOV_NOT_REACHED */
        }
    }
}
/* END CONFIG_32 */

/* NO CONFIG */
#else
  #error "define CONFIG_64 or CONFIG_32"
#endif /* CONFIG */


static inline void
_mpd_div_word(mpd_uint_t *q, mpd_uint_t *r, mpd_uint_t v, mpd_uint_t d)
{
    *q = v / d;
    *r = v - *q * d;
}

static inline void
_mpd_idiv_word(mpd_ssize_t *q, mpd_ssize_t *r, mpd_ssize_t v, mpd_ssize_t d)
{
    *q = v / d;
    *r = v - *q * d;
}


/** ------------------------------------------------------------
 **              Arithmetic with overflow checking
 ** ------------------------------------------------------------
 */

/* The following macros do call exit() in case of an overflow.
   If the library is used correctly (i.e. with valid context
   parameters), such overflows cannot occur. The macros are used
   as sanity checks in a couple of strategic places and should
   be viewed as a handwritten version of gcc's -ftrapv option. */

static inline mpd_size_t
add_size_t(mpd_size_t a, mpd_size_t b)
{
    if (a > MPD_SIZE_MAX - b) {
        mpd_err_fatal("add_size_t(): overflow: check the context"); /* GCOV_NOT_REACHED */
    }
    return a + b;
}

static inline mpd_size_t
sub_size_t(mpd_size_t a, mpd_size_t b)
{
    if (b > a) {
        mpd_err_fatal("sub_size_t(): overflow: check the context"); /* GCOV_NOT_REACHED */
    }
    return a - b;
}

#if MPD_SIZE_MAX != MPD_UINT_MAX
  #error "adapt mul_size_t() and mulmod_size_t()"
#endif

static inline mpd_size_t
mul_size_t(mpd_size_t a, mpd_size_t b)
{
    mpd_uint_t hi, lo;

    _mpd_mul_words(&hi, &lo, (mpd_uint_t)a, (mpd_uint_t)b);
    if (hi) {
        mpd_err_fatal("mul_size_t(): overflow: check the context"); /* GCOV_NOT_REACHED */
    }
    return lo;
}

static inline mpd_size_t
add_size_t_overflow(mpd_size_t a, mpd_size_t b, mpd_size_t *overflow)
{
    mpd_size_t ret;

    *overflow = 0;
    ret = a + b;
    if (ret < a) *overflow = 1;
    return ret;
}

static inline mpd_size_t
mul_size_t_overflow(mpd_size_t a, mpd_size_t b, mpd_size_t *overflow)
{
    mpd_uint_t lo;

    _mpd_mul_words((mpd_uint_t *)overflow, &lo, (mpd_uint_t)a,
                   (mpd_uint_t)b);
    return lo;
}

static inline mpd_ssize_t
mod_mpd_ssize_t(mpd_ssize_t a, mpd_ssize_t m)
{
    mpd_ssize_t r = a % m;
    return (r < 0) ? r + m : r;
}

static inline mpd_size_t
mulmod_size_t(mpd_size_t a, mpd_size_t b, mpd_size_t m)
{
    mpd_uint_t hi, lo;
    mpd_uint_t q, r;

    _mpd_mul_words(&hi, &lo, (mpd_uint_t)a, (mpd_uint_t)b);
    _mpd_div_words(&q, &r, hi, lo, (mpd_uint_t)m);

    return r;
}


#endif /* TYPEARITH_H */



