/*
 * Copyright (c) 2020 Stefan Krah. All rights reserved.
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


/*
 * This file contains the mpd_triple API that will be part of mpdecimal
 * in a future release.  libmpdec is now strictly externally developed,
 * so the libmpdec copy in Python should remain unchanged.
 */


#ifndef LIBMPDEC_TRIPLE_H_
#define LIBMPDEC_TRIPLE_H_


#include "mpdecimal.h"
#include "basearith.h"


#ifdef __cplus_plus
#include <cassert>
#include <climits>
#include <cstdint>
extern "C" {
#else
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#endif


/* status cases for getting a triple */
enum mpd_triple_class {
  MPD_TRIPLE_NORMAL,
  MPD_TRIPLE_INF,
  MPD_TRIPLE_QNAN,
  MPD_TRIPLE_SNAN,
  MPD_TRIPLE_ERROR,
};

typedef struct {
  enum mpd_triple_class tag;
  uint8_t sign;
  uint64_t hi;
  uint64_t lo;
  int64_t exp;
} mpd_uint128_triple_t;


/******************************************************************************/
/*                                     Util                                   */
/******************************************************************************/

/* Internal function: Copy a decimal, share data with src: USE WITH CARE! */
static inline void
_mpd_copy_shared(mpd_t *dest, const mpd_t *src)
{
    dest->flags = src->flags;
    dest->exp = src->exp;
    dest->digits = src->digits;
    dest->len = src->len;
    dest->alloc = src->alloc;
    dest->data = src->data;

    mpd_set_shared_data(dest);
}

#if !defined(CONFIG_64) || !defined(__SIZEOF_INT128__)
static inline mpd_ssize_t
_mpd_real_size(mpd_uint_t *data, mpd_ssize_t size)
{
    while (size > 1 && data[size-1] == 0) {
        size--;
    }

    return size;
}

static inline size_t
_uint_from_u16(mpd_uint_t *w, mpd_ssize_t wlen, const uint16_t *u, size_t ulen)
{
    const mpd_uint_t ubase = 1U<<16;
    mpd_ssize_t n = 0;
    mpd_uint_t carry;

    assert(wlen > 0 && ulen > 0);

    w[n++] = u[--ulen];
    while (--ulen != SIZE_MAX) {
        carry = _mpd_shortmul_c(w, w, n, ubase);
        if (carry) {
            if (n >= wlen) {
                abort();  /* GCOV_NOT_REACHED */
            }
            w[n++] = carry;
        }
        carry = _mpd_shortadd(w, n, u[ulen]);
        if (carry) {
            if (n >= wlen) {
                abort();  /* GCOV_NOT_REACHED */
            }
            w[n++] = carry;
        }
    }

    return n;
}

static inline size_t
_uint_to_u16(uint16_t w[8], mpd_uint_t *u, mpd_ssize_t ulen)
{
    const mpd_uint_t wbase = 1U<<16;
    size_t n = 0;

    assert(ulen > 0);

    do {
        if (n >= 8) {
            abort();  /* GCOV_NOT_REACHED */
        }
        w[n++] = (uint16_t)_mpd_shortdiv(u, u, ulen, wbase);
        /* ulen is at least 1. u[ulen-1] can only be zero if ulen == 1. */
        ulen = _mpd_real_size(u, ulen);

    } while (u[ulen-1] != 0);

    return n;
}
#endif


/******************************************************************************/
/*                                From triple                                 */
/******************************************************************************/

#if defined(CONFIG_64) && defined(__SIZEOF_INT128__)
static inline mpd_ssize_t
_set_coeff(uint64_t data[3], uint64_t hi, uint64_t lo)
{
    __uint128_t d = ((__uint128_t)hi << 64) + lo;
    __uint128_t q, r;

    q = d / MPD_RADIX;
    r = d % MPD_RADIX;
    data[0] = (uint64_t)r;
    d = q;

    q = d / MPD_RADIX;
    r = d % MPD_RADIX;
    data[1] = (uint64_t)r;
    d = q;

    q = d / MPD_RADIX;
    r = d % MPD_RADIX;
    data[2] = (uint64_t)r;

    if (q != 0) {
        abort(); /* GCOV_NOT_REACHED */
    }

    return data[2] != 0 ? 3 : (data[1] != 0 ? 2 : 1);
}
#else
static inline mpd_ssize_t
_set_coeff(mpd_uint_t *data, mpd_ssize_t len, uint64_t hi, uint64_t lo)
{
    uint16_t u16[8] = {0};

    u16[7] = (uint16_t)((hi & 0xFFFF000000000000ULL) >> 48);
    u16[6] = (uint16_t)((hi & 0x0000FFFF00000000ULL) >> 32);
    u16[5] = (uint16_t)((hi & 0x00000000FFFF0000ULL) >> 16);
    u16[4] = (uint16_t) (hi & 0x000000000000FFFFULL);

    u16[3] = (uint16_t)((lo & 0xFFFF000000000000ULL) >> 48);
    u16[2] = (uint16_t)((lo & 0x0000FFFF00000000ULL) >> 32);
    u16[1] = (uint16_t)((lo & 0x00000000FFFF0000ULL) >> 16);
    u16[0] = (uint16_t) (lo & 0x000000000000FFFFULL);

    return (mpd_ssize_t)_uint_from_u16(data, len, u16, 8);
}
#endif

static inline int
_set_uint128_coeff_exp(mpd_t *result, uint64_t hi, uint64_t lo, mpd_ssize_t exp)
{
    mpd_uint_t data[5] = {0};
    uint32_t status = 0;
    mpd_ssize_t len;

#if defined(CONFIG_64) && defined(__SIZEOF_INT128__)
    len = _set_coeff(data, hi, lo);
#else
    len = _set_coeff(data, 5, hi, lo);
#endif

    if (!mpd_qresize(result, len, &status)) {
        return -1;
    }

    for (mpd_ssize_t i = 0; i < len; i++) {
        result->data[i] = data[i];
    }

    result->exp = exp;
    result->len = len;
    mpd_setdigits(result);

    return 0;
}

static inline int
mpd_from_uint128_triple(mpd_t *result, const mpd_uint128_triple_t *triple, uint32_t *status)
{
    static const mpd_context_t maxcontext = {
     .prec=MPD_MAX_PREC,
     .emax=MPD_MAX_EMAX,
     .emin=MPD_MIN_EMIN,
     .round=MPD_ROUND_HALF_EVEN,
     .traps=MPD_Traps,
     .status=0,
     .newtrap=0,
     .clamp=0,
     .allcr=1,
    };
    const enum mpd_triple_class tag = triple->tag;
    const uint8_t sign = triple->sign;
    const uint64_t hi = triple->hi;
    const uint64_t lo = triple->lo;
    mpd_ssize_t exp;

#ifdef CONFIG_32
    if (triple->exp < MPD_SSIZE_MIN || triple->exp > MPD_SSIZE_MAX) {
        goto conversion_error;
    }
#endif
    exp = (mpd_ssize_t)triple->exp;

    switch (tag) {
    case MPD_TRIPLE_QNAN: case MPD_TRIPLE_SNAN: {
        if (sign > 1 || exp != 0) {
            goto conversion_error;
        }

        const uint8_t flags = tag == MPD_TRIPLE_QNAN ? MPD_NAN : MPD_SNAN;
        mpd_setspecial(result, sign, flags);

        if (hi == 0 && lo == 0) {  /* no payload */
            return 0;
        }

        if (_set_uint128_coeff_exp(result, hi, lo, exp) < 0) {
            goto malloc_error;
        }

        return 0;
    }

    case MPD_TRIPLE_INF: {
        if (sign > 1 || hi != 0 || lo != 0 || exp != 0) {
            goto conversion_error;
        }

        mpd_setspecial(result, sign, MPD_INF);

        return 0;
    }

    case MPD_TRIPLE_NORMAL: {
        if (sign > 1) {
            goto conversion_error;
        }

        const uint8_t flags = sign ? MPD_NEG : MPD_POS;
        mpd_set_flags(result, flags);

        if (exp > MPD_EXP_INF) {
            exp = MPD_EXP_INF;
        }
        if (exp == MPD_SSIZE_MIN) {
            exp = MPD_SSIZE_MIN+1;
        }

        if (_set_uint128_coeff_exp(result, hi, lo, exp) < 0) {
            goto malloc_error;
        }

        uint32_t workstatus = 0;
        mpd_qfinalize(result, &maxcontext, &workstatus);
        if (workstatus & (MPD_Inexact|MPD_Rounded|MPD_Clamped)) {
            goto conversion_error;
        }

        return 0;
    }

    default:
        goto conversion_error;
    }

    return 0;

conversion_error:
    mpd_seterror(result, MPD_Conversion_syntax, status);
    return -1;

malloc_error:
    mpd_seterror(result, MPD_Malloc_error, status);
    return -1;
}


/******************************************************************************/
/*                                  As triple                                 */
/******************************************************************************/

#if defined(CONFIG_64) && defined(__SIZEOF_INT128__)
static inline void
_get_coeff(uint64_t *hi, uint64_t *lo, const mpd_t *a)
{
    __uint128_t u128 = 0;

    switch (a->len) {
    case 3:
        u128 = a->data[2]; /* fall through */
    case 2:
        u128 = u128 * MPD_RADIX + a->data[1]; /* fall through */
    case 1:
        u128 = u128 * MPD_RADIX + a->data[0];
        break;
    default:
        abort(); /* GCOV_NOT_REACHED */
    }

    *hi = u128 >> 64;
    *lo = (uint64_t)u128;
}
#else
static inline void
_get_coeff(uint64_t *hi, uint64_t *lo, const mpd_t *a)
{
    uint16_t u16[8] = {0};
    mpd_uint_t data[5] = {0};

    switch (a->len) {
    case 5:
        data[4] = a->data[4]; /* fall through */
    case 4:
        data[3] = a->data[3]; /* fall through */
    case 3:
        data[2] = a->data[2]; /* fall through */
    case 2:
        data[1] = a->data[1]; /* fall through */
    case 1:
        data[0] = a->data[0];
        break;
    default:
        abort();  /* GCOV_NOT_REACHED */
    }

    _uint_to_u16(u16, data, a->len);

    *hi = (uint64_t)u16[7] << 48;
    *hi |= (uint64_t)u16[6] << 32;
    *hi |= (uint64_t)u16[5] << 16;
    *hi |= (uint64_t)u16[4];

    *lo = (uint64_t)u16[3] << 48;
    *lo |= (uint64_t)u16[2] << 32;
    *lo |= (uint64_t)u16[1] << 16;
    *lo |= (uint64_t)u16[0];
}
#endif

static inline enum mpd_triple_class
_coeff_as_uint128(uint64_t *hi, uint64_t *lo, const mpd_t *a)
{
#ifdef CONFIG_64
    static mpd_uint_t uint128_max_data[3] = { 3374607431768211455ULL, 4028236692093846346ULL, 3ULL };
    static const mpd_t uint128_max = { MPD_STATIC|MPD_CONST_DATA, 0, 39, 3, 3, uint128_max_data };
#else
    static mpd_uint_t uint128_max_data[5] = { 768211455U, 374607431U, 938463463U, 282366920U, 340U };
    static const mpd_t uint128_max = { MPD_STATIC|MPD_CONST_DATA, 0, 39, 5, 5, uint128_max_data };
#endif
    enum mpd_triple_class ret = MPD_TRIPLE_NORMAL;
    uint32_t status = 0;
    mpd_t coeff;

    *hi = *lo = 0ULL;

    if (mpd_isspecial(a)) {
        if (mpd_isinfinite(a)) {
            return MPD_TRIPLE_INF;
        }

        ret = mpd_isqnan(a) ? MPD_TRIPLE_QNAN : MPD_TRIPLE_SNAN;
        if (a->len == 0) { /* no payload */
            return ret;
        }
    }
    else if (mpd_iszero(a)) {
        return ret;
    }

    _mpd_copy_shared(&coeff, a);
    mpd_set_flags(&coeff, 0);
    coeff.exp = 0;

    if (mpd_qcmp(&coeff, &uint128_max, &status) > 0) {
        return MPD_TRIPLE_ERROR;
    }

    _get_coeff(hi, lo, &coeff);
    return ret;
}

static inline mpd_uint128_triple_t
mpd_as_uint128_triple(const mpd_t *a)
{
    mpd_uint128_triple_t triple = { MPD_TRIPLE_ERROR, 0, 0, 0, 0 };

    triple.tag = _coeff_as_uint128(&triple.hi, &triple.lo, a);
    if (triple.tag == MPD_TRIPLE_ERROR) {
        return triple;
    }

    triple.sign = !!mpd_isnegative(a);
    if (triple.tag == MPD_TRIPLE_NORMAL) {
        triple.exp = a->exp;
    }

    return triple;
}


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* LIBMPDEC_TRIPLE_H_ */
