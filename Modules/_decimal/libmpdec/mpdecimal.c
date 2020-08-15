/*
 * Copyright (c) 2008-2020 Stefan Krah. All rights reserved.
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


#include "mpdecimal.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basearith.h"
#include "bits.h"
#include "constants.h"
#include "convolute.h"
#include "crt.h"
#include "mpalloc.h"
#include "typearith.h"

#ifdef PPRO
  #if defined(_MSC_VER)
    #include <float.h>
    #pragma float_control(precise, on)
    #pragma fenv_access(on)
  #elif !defined(__OpenBSD__) && !defined(__NetBSD__)
    /* C99 */
    #include <fenv.h>
    #pragma STDC FENV_ACCESS ON
  #endif
#endif


/* Disable warning that is part of -Wextra since gcc 7.0. */
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && __GNUC__ >= 7
  #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif


#if defined(_MSC_VER)
  #define ALWAYS_INLINE __forceinline
#elif defined(__IBMC__) || defined(LEGACY_COMPILER)
  #define ALWAYS_INLINE
  #undef inline
  #define inline
#else
  #ifdef TEST_COVERAGE
    #define ALWAYS_INLINE
  #else
    #define ALWAYS_INLINE inline __attribute__ ((always_inline))
  #endif
#endif


#define MPD_NEWTONDIV_CUTOFF 1024L

#define MPD_NEW_STATIC(name, flags, exp, digits, len) \
        mpd_uint_t name##_data[MPD_MINALLOC_MAX];                    \
        mpd_t name = {flags|MPD_STATIC|MPD_STATIC_DATA, exp, digits, \
                      len, MPD_MINALLOC_MAX, name##_data}

#define MPD_NEW_CONST(name, flags, exp, digits, len, alloc, initval) \
        mpd_uint_t name##_data[alloc] = {initval};                   \
        mpd_t name = {flags|MPD_STATIC|MPD_CONST_DATA, exp, digits,  \
                      len, alloc, name##_data}

#define MPD_NEW_SHARED(name, a) \
        mpd_t name = {(a->flags&~MPD_DATAFLAGS)|MPD_STATIC|MPD_SHARED_DATA, \
                      a->exp, a->digits, a->len, a->alloc, a->data}


static mpd_uint_t data_one[1] = {1};
static mpd_uint_t data_zero[1] = {0};
static const mpd_t one = {MPD_STATIC|MPD_CONST_DATA, 0, 1, 1, 1, data_one};
static const mpd_t minus_one = {MPD_NEG|MPD_STATIC|MPD_CONST_DATA, 0, 1, 1, 1,
                                data_one};
static const mpd_t zero = {MPD_STATIC|MPD_CONST_DATA, 0, 1, 1, 1, data_zero};

static inline void _mpd_check_exp(mpd_t *dec, const mpd_context_t *ctx,
                                  uint32_t *status);
static void _settriple(mpd_t *result, uint8_t sign, mpd_uint_t a,
                       mpd_ssize_t exp);
static inline mpd_ssize_t _mpd_real_size(mpd_uint_t *data, mpd_ssize_t size);

static int _mpd_cmp_abs(const mpd_t *a, const mpd_t *b);

static void _mpd_qadd(mpd_t *result, const mpd_t *a, const mpd_t *b,
                      const mpd_context_t *ctx, uint32_t *status);
static inline void _mpd_qmul(mpd_t *result, const mpd_t *a, const mpd_t *b,
                             const mpd_context_t *ctx, uint32_t *status);
static void _mpd_base_ndivmod(mpd_t *q, mpd_t *r, const mpd_t *a,
                              const mpd_t *b, uint32_t *status);
static inline void _mpd_qpow_uint(mpd_t *result, const mpd_t *base,
                                  mpd_uint_t exp, uint8_t resultsign,
                                  const mpd_context_t *ctx, uint32_t *status);

static mpd_uint_t mpd_qsshiftr(mpd_t *result, const mpd_t *a, mpd_ssize_t n);


/******************************************************************************/
/*                                  Version                                   */
/******************************************************************************/

const char *
mpd_version(void)
{
    return MPD_VERSION;
}


/******************************************************************************/
/*                  Performance critical inline functions                     */
/******************************************************************************/

#ifdef CONFIG_64
/* Digits in a word, primarily useful for the most significant word. */
ALWAYS_INLINE int
mpd_word_digits(mpd_uint_t word)
{
    if (word < mpd_pow10[9]) {
        if (word < mpd_pow10[4]) {
            if (word < mpd_pow10[2]) {
                return (word < mpd_pow10[1]) ? 1 : 2;
            }
            return (word < mpd_pow10[3]) ? 3 : 4;
        }
        if (word < mpd_pow10[6]) {
            return (word < mpd_pow10[5]) ? 5 : 6;
        }
        if (word < mpd_pow10[8]) {
            return (word < mpd_pow10[7]) ? 7 : 8;
        }
        return 9;
    }
    if (word < mpd_pow10[14]) {
        if (word < mpd_pow10[11]) {
            return (word < mpd_pow10[10]) ? 10 : 11;
        }
        if (word < mpd_pow10[13]) {
            return (word < mpd_pow10[12]) ? 12 : 13;
        }
        return 14;
    }
    if (word < mpd_pow10[18]) {
        if (word < mpd_pow10[16]) {
            return (word < mpd_pow10[15]) ? 15 : 16;
        }
        return (word < mpd_pow10[17]) ? 17 : 18;
    }

    return (word < mpd_pow10[19]) ? 19 : 20;
}
#else
ALWAYS_INLINE int
mpd_word_digits(mpd_uint_t word)
{
    if (word < mpd_pow10[4]) {
        if (word < mpd_pow10[2]) {
            return (word < mpd_pow10[1]) ? 1 : 2;
        }
        return (word < mpd_pow10[3]) ? 3 : 4;
    }
    if (word < mpd_pow10[6]) {
        return (word < mpd_pow10[5]) ? 5 : 6;
    }
    if (word < mpd_pow10[8]) {
        return (word < mpd_pow10[7]) ? 7 : 8;
    }

    return (word < mpd_pow10[9]) ? 9 : 10;
}
#endif


/* Adjusted exponent */
ALWAYS_INLINE mpd_ssize_t
mpd_adjexp(const mpd_t *dec)
{
    return (dec->exp + dec->digits) - 1;
}

/* Etiny */
ALWAYS_INLINE mpd_ssize_t
mpd_etiny(const mpd_context_t *ctx)
{
    return ctx->emin - (ctx->prec - 1);
}

/* Etop: used for folding down in IEEE clamping */
ALWAYS_INLINE mpd_ssize_t
mpd_etop(const mpd_context_t *ctx)
{
    return ctx->emax - (ctx->prec - 1);
}

/* Most significant word */
ALWAYS_INLINE mpd_uint_t
mpd_msword(const mpd_t *dec)
{
    assert(dec->len > 0);
    return dec->data[dec->len-1];
}

/* Most significant digit of a word */
inline mpd_uint_t
mpd_msd(mpd_uint_t word)
{
    int n;

    n = mpd_word_digits(word);
    return word / mpd_pow10[n-1];
}

/* Least significant digit of a word */
ALWAYS_INLINE mpd_uint_t
mpd_lsd(mpd_uint_t word)
{
    return word % 10;
}

/* Coefficient size needed to store 'digits' */
mpd_ssize_t
mpd_digits_to_size(mpd_ssize_t digits)
{
    mpd_ssize_t q, r;

    _mpd_idiv_word(&q, &r, digits, MPD_RDIGITS);
    return (r == 0) ? q : q+1;
}

/* Number of digits in the exponent. Not defined for MPD_SSIZE_MIN. */
inline int
mpd_exp_digits(mpd_ssize_t exp)
{
    exp = (exp < 0) ? -exp : exp;
    return mpd_word_digits(exp);
}

/* Canonical */
ALWAYS_INLINE int
mpd_iscanonical(const mpd_t *dec)
{
    (void)dec;
    return 1;
}

/* Finite */
ALWAYS_INLINE int
mpd_isfinite(const mpd_t *dec)
{
    return !(dec->flags & MPD_SPECIAL);
}

/* Infinite */
ALWAYS_INLINE int
mpd_isinfinite(const mpd_t *dec)
{
    return dec->flags & MPD_INF;
}

/* NaN */
ALWAYS_INLINE int
mpd_isnan(const mpd_t *dec)
{
    return dec->flags & (MPD_NAN|MPD_SNAN);
}

/* Negative */
ALWAYS_INLINE int
mpd_isnegative(const mpd_t *dec)
{
    return dec->flags & MPD_NEG;
}

/* Positive */
ALWAYS_INLINE int
mpd_ispositive(const mpd_t *dec)
{
    return !(dec->flags & MPD_NEG);
}

/* qNaN */
ALWAYS_INLINE int
mpd_isqnan(const mpd_t *dec)
{
    return dec->flags & MPD_NAN;
}

/* Signed */
ALWAYS_INLINE int
mpd_issigned(const mpd_t *dec)
{
    return dec->flags & MPD_NEG;
}

/* sNaN */
ALWAYS_INLINE int
mpd_issnan(const mpd_t *dec)
{
    return dec->flags & MPD_SNAN;
}

/* Special */
ALWAYS_INLINE int
mpd_isspecial(const mpd_t *dec)
{
    return dec->flags & MPD_SPECIAL;
}

/* Zero */
ALWAYS_INLINE int
mpd_iszero(const mpd_t *dec)
{
    return !mpd_isspecial(dec) && mpd_msword(dec) == 0;
}

/* Test for zero when specials have been ruled out already */
ALWAYS_INLINE int
mpd_iszerocoeff(const mpd_t *dec)
{
    return mpd_msword(dec) == 0;
}

/* Normal */
inline int
mpd_isnormal(const mpd_t *dec, const mpd_context_t *ctx)
{
    if (mpd_isspecial(dec)) return 0;
    if (mpd_iszerocoeff(dec)) return 0;

    return mpd_adjexp(dec) >= ctx->emin;
}

/* Subnormal */
inline int
mpd_issubnormal(const mpd_t *dec, const mpd_context_t *ctx)
{
    if (mpd_isspecial(dec)) return 0;
    if (mpd_iszerocoeff(dec)) return 0;

    return mpd_adjexp(dec) < ctx->emin;
}

/* Odd word */
ALWAYS_INLINE int
mpd_isoddword(mpd_uint_t word)
{
    return word & 1;
}

/* Odd coefficient */
ALWAYS_INLINE int
mpd_isoddcoeff(const mpd_t *dec)
{
    return mpd_isoddword(dec->data[0]);
}

/* 0 if dec is positive, 1 if dec is negative */
ALWAYS_INLINE uint8_t
mpd_sign(const mpd_t *dec)
{
    return dec->flags & MPD_NEG;
}

/* 1 if dec is positive, -1 if dec is negative */
ALWAYS_INLINE int
mpd_arith_sign(const mpd_t *dec)
{
    return 1 - 2 * mpd_isnegative(dec);
}

/* Radix */
ALWAYS_INLINE long
mpd_radix(void)
{
    return 10;
}

/* Dynamic decimal */
ALWAYS_INLINE int
mpd_isdynamic(const mpd_t *dec)
{
    return !(dec->flags & MPD_STATIC);
}

/* Static decimal */
ALWAYS_INLINE int
mpd_isstatic(const mpd_t *dec)
{
    return dec->flags & MPD_STATIC;
}

/* Data of decimal is dynamic */
ALWAYS_INLINE int
mpd_isdynamic_data(const mpd_t *dec)
{
    return !(dec->flags & MPD_DATAFLAGS);
}

/* Data of decimal is static */
ALWAYS_INLINE int
mpd_isstatic_data(const mpd_t *dec)
{
    return dec->flags & MPD_STATIC_DATA;
}

/* Data of decimal is shared */
ALWAYS_INLINE int
mpd_isshared_data(const mpd_t *dec)
{
    return dec->flags & MPD_SHARED_DATA;
}

/* Data of decimal is const */
ALWAYS_INLINE int
mpd_isconst_data(const mpd_t *dec)
{
    return dec->flags & MPD_CONST_DATA;
}


/******************************************************************************/
/*                         Inline memory handling                             */
/******************************************************************************/

/* Fill destination with zeros */
ALWAYS_INLINE void
mpd_uint_zero(mpd_uint_t *dest, mpd_size_t len)
{
    mpd_size_t i;

    for (i = 0; i < len; i++) {
        dest[i] = 0;
    }
}

/* Free a decimal */
ALWAYS_INLINE void
mpd_del(mpd_t *dec)
{
    if (mpd_isdynamic_data(dec)) {
        mpd_free(dec->data);
    }
    if (mpd_isdynamic(dec)) {
        mpd_free(dec);
    }
}

/*
 * Resize the coefficient. Existing data up to 'nwords' is left untouched.
 * Return 1 on success, 0 otherwise.
 *
 * Input invariant: MPD_MINALLOC <= result->alloc.
 *
 * Case nwords == result->alloc:
 *     'result' is unchanged. Return 1.
 *
 * Case nwords > result->alloc:
 *   Case realloc success:
 *     The value of 'result' does not change. Return 1.
 *   Case realloc failure:
 *     'result' is NaN, status is updated with MPD_Malloc_error. Return 0.
 *
 * Case nwords < result->alloc:
 *   Case is_static_data or realloc failure [1]:
 *     'result' is unchanged. Return 1.
 *   Case realloc success:
 *     The value of result is undefined (expected). Return 1.
 *
 *
 * [1] In that case the old (now oversized) area is still valid.
 */
ALWAYS_INLINE int
mpd_qresize(mpd_t *result, mpd_ssize_t nwords, uint32_t *status)
{
    assert(!mpd_isconst_data(result)); /* illegal operation for a const */
    assert(!mpd_isshared_data(result)); /* illegal operation for a shared */
    assert(MPD_MINALLOC <= result->alloc);

    nwords = (nwords <= MPD_MINALLOC) ? MPD_MINALLOC : nwords;
    if (nwords == result->alloc) {
        return 1;
    }
    if (mpd_isstatic_data(result)) {
        if (nwords > result->alloc) {
            return mpd_switch_to_dyn(result, nwords, status);
        }
        return 1;
    }

    return mpd_realloc_dyn(result, nwords, status);
}

/* Same as mpd_qresize, but do not set the result no NaN on failure. */
static ALWAYS_INLINE int
mpd_qresize_cxx(mpd_t *result, mpd_ssize_t nwords)
{
    assert(!mpd_isconst_data(result)); /* illegal operation for a const */
    assert(!mpd_isshared_data(result)); /* illegal operation for a shared */
    assert(MPD_MINALLOC <= result->alloc);

    nwords = (nwords <= MPD_MINALLOC) ? MPD_MINALLOC : nwords;
    if (nwords == result->alloc) {
        return 1;
    }
    if (mpd_isstatic_data(result)) {
        if (nwords > result->alloc) {
            return mpd_switch_to_dyn_cxx(result, nwords);
        }
        return 1;
    }

    return mpd_realloc_dyn_cxx(result, nwords);
}

/* Same as mpd_qresize, but the complete coefficient (including the old
 * memory area!) is initialized to zero. */
ALWAYS_INLINE int
mpd_qresize_zero(mpd_t *result, mpd_ssize_t nwords, uint32_t *status)
{
    assert(!mpd_isconst_data(result)); /* illegal operation for a const */
    assert(!mpd_isshared_data(result)); /* illegal operation for a shared */
    assert(MPD_MINALLOC <= result->alloc);

    nwords = (nwords <= MPD_MINALLOC) ? MPD_MINALLOC : nwords;
    if (nwords != result->alloc) {
        if (mpd_isstatic_data(result)) {
            if (nwords > result->alloc) {
                return mpd_switch_to_dyn_zero(result, nwords, status);
            }
        }
        else if (!mpd_realloc_dyn(result, nwords, status)) {
            return 0;
        }
    }

    mpd_uint_zero(result->data, nwords);
    return 1;
}

/*
 * Reduce memory size for the coefficient to MPD_MINALLOC. In theory,
 * realloc may fail even when reducing the memory size. But in that case
 * the old memory area is always big enough, so checking for MPD_Malloc_error
 * is not imperative.
 */
ALWAYS_INLINE void
mpd_minalloc(mpd_t *result)
{
    assert(!mpd_isconst_data(result)); /* illegal operation for a const */
    assert(!mpd_isshared_data(result)); /* illegal operation for a shared */

    if (!mpd_isstatic_data(result) && result->alloc > MPD_MINALLOC) {
        uint8_t err = 0;
        result->data = mpd_realloc(result->data, MPD_MINALLOC,
                                   sizeof *result->data, &err);
        if (!err) {
            result->alloc = MPD_MINALLOC;
        }
    }
}

int
mpd_resize(mpd_t *result, mpd_ssize_t nwords, mpd_context_t *ctx)
{
    uint32_t status = 0;
    if (!mpd_qresize(result, nwords, &status)) {
        mpd_addstatus_raise(ctx, status);
        return 0;
    }
    return 1;
}

int
mpd_resize_zero(mpd_t *result, mpd_ssize_t nwords, mpd_context_t *ctx)
{
    uint32_t status = 0;
    if (!mpd_qresize_zero(result, nwords, &status)) {
        mpd_addstatus_raise(ctx, status);
        return 0;
    }
    return 1;
}


/******************************************************************************/
/*                       Set attributes of a decimal                          */
/******************************************************************************/

/* Set digits. Assumption: result->len is initialized and > 0. */
inline void
mpd_setdigits(mpd_t *result)
{
    mpd_ssize_t wdigits = mpd_word_digits(mpd_msword(result));
    result->digits = wdigits + (result->len-1) * MPD_RDIGITS;
}

/* Set sign */
ALWAYS_INLINE void
mpd_set_sign(mpd_t *result, uint8_t sign)
{
    result->flags &= ~MPD_NEG;
    result->flags |= sign;
}

/* Copy sign from another decimal */
ALWAYS_INLINE void
mpd_signcpy(mpd_t *result, const mpd_t *a)
{
    uint8_t sign = a->flags&MPD_NEG;

    result->flags &= ~MPD_NEG;
    result->flags |= sign;
}

/* Set infinity */
ALWAYS_INLINE void
mpd_set_infinity(mpd_t *result)
{
    result->flags &= ~MPD_SPECIAL;
    result->flags |= MPD_INF;
}

/* Set qNaN */
ALWAYS_INLINE void
mpd_set_qnan(mpd_t *result)
{
    result->flags &= ~MPD_SPECIAL;
    result->flags |= MPD_NAN;
}

/* Set sNaN */
ALWAYS_INLINE void
mpd_set_snan(mpd_t *result)
{
    result->flags &= ~MPD_SPECIAL;
    result->flags |= MPD_SNAN;
}

/* Set to negative */
ALWAYS_INLINE void
mpd_set_negative(mpd_t *result)
{
    result->flags |= MPD_NEG;
}

/* Set to positive */
ALWAYS_INLINE void
mpd_set_positive(mpd_t *result)
{
    result->flags &= ~MPD_NEG;
}

/* Set to dynamic */
ALWAYS_INLINE void
mpd_set_dynamic(mpd_t *result)
{
    result->flags &= ~MPD_STATIC;
}

/* Set to static */
ALWAYS_INLINE void
mpd_set_static(mpd_t *result)
{
    result->flags |= MPD_STATIC;
}

/* Set data to dynamic */
ALWAYS_INLINE void
mpd_set_dynamic_data(mpd_t *result)
{
    result->flags &= ~MPD_DATAFLAGS;
}

/* Set data to static */
ALWAYS_INLINE void
mpd_set_static_data(mpd_t *result)
{
    result->flags &= ~MPD_DATAFLAGS;
    result->flags |= MPD_STATIC_DATA;
}

/* Set data to shared */
ALWAYS_INLINE void
mpd_set_shared_data(mpd_t *result)
{
    result->flags &= ~MPD_DATAFLAGS;
    result->flags |= MPD_SHARED_DATA;
}

/* Set data to const */
ALWAYS_INLINE void
mpd_set_const_data(mpd_t *result)
{
    result->flags &= ~MPD_DATAFLAGS;
    result->flags |= MPD_CONST_DATA;
}

/* Clear flags, preserving memory attributes. */
ALWAYS_INLINE void
mpd_clear_flags(mpd_t *result)
{
    result->flags &= (MPD_STATIC|MPD_DATAFLAGS);
}

/* Set flags, preserving memory attributes. */
ALWAYS_INLINE void
mpd_set_flags(mpd_t *result, uint8_t flags)
{
    result->flags &= (MPD_STATIC|MPD_DATAFLAGS);
    result->flags |= flags;
}

/* Copy flags, preserving memory attributes of result. */
ALWAYS_INLINE void
mpd_copy_flags(mpd_t *result, const mpd_t *a)
{
    uint8_t aflags = a->flags;
    result->flags &= (MPD_STATIC|MPD_DATAFLAGS);
    result->flags |= (aflags & ~(MPD_STATIC|MPD_DATAFLAGS));
}

/* Initialize a workcontext from ctx. Set traps, flags and newtrap to 0. */
static inline void
mpd_workcontext(mpd_context_t *workctx, const mpd_context_t *ctx)
{
    workctx->prec = ctx->prec;
    workctx->emax = ctx->emax;
    workctx->emin = ctx->emin;
    workctx->round = ctx->round;
    workctx->traps = 0;
    workctx->status = 0;
    workctx->newtrap = 0;
    workctx->clamp = ctx->clamp;
    workctx->allcr = ctx->allcr;
}


/******************************************************************************/
/*                  Getting and setting parts of decimals                     */
/******************************************************************************/

/* Flip the sign of a decimal */
static inline void
_mpd_negate(mpd_t *dec)
{
    dec->flags ^= MPD_NEG;
}

/* Set coefficient to zero */
void
mpd_zerocoeff(mpd_t *result)
{
    mpd_minalloc(result);
    result->digits = 1;
    result->len = 1;
    result->data[0] = 0;
}

/* Set the coefficient to all nines. */
void
mpd_qmaxcoeff(mpd_t *result, const mpd_context_t *ctx, uint32_t *status)
{
    mpd_ssize_t len, r;

    _mpd_idiv_word(&len, &r, ctx->prec, MPD_RDIGITS);
    len = (r == 0) ? len : len+1;

    if (!mpd_qresize(result, len, status)) {
        return;
    }

    result->len = len;
    result->digits = ctx->prec;

    --len;
    if (r > 0) {
        result->data[len--] = mpd_pow10[r]-1;
    }
    for (; len >= 0; --len) {
        result->data[len] = MPD_RADIX-1;
    }
}

/*
 * Cut off the most significant digits so that the rest fits in ctx->prec.
 * Cannot fail.
 */
static void
_mpd_cap(mpd_t *result, const mpd_context_t *ctx)
{
    uint32_t dummy;
    mpd_ssize_t len, r;

    if (result->len > 0 && result->digits > ctx->prec) {
        _mpd_idiv_word(&len, &r, ctx->prec, MPD_RDIGITS);
        len = (r == 0) ? len : len+1;

        if (r != 0) {
            result->data[len-1] %= mpd_pow10[r];
        }

        len = _mpd_real_size(result->data, len);
        /* resize to fewer words cannot fail */
        mpd_qresize(result, len, &dummy);
        result->len = len;
        mpd_setdigits(result);
    }
    if (mpd_iszero(result)) {
        _settriple(result, mpd_sign(result), 0, result->exp);
    }
}

/*
 * Cut off the most significant digits of a NaN payload so that the rest
 * fits in ctx->prec - ctx->clamp. Cannot fail.
 */
static void
_mpd_fix_nan(mpd_t *result, const mpd_context_t *ctx)
{
    uint32_t dummy;
    mpd_ssize_t prec;
    mpd_ssize_t len, r;

    prec = ctx->prec - ctx->clamp;
    if (result->len > 0 && result->digits > prec) {
        if (prec == 0) {
            mpd_minalloc(result);
            result->len = result->digits = 0;
        }
        else {
            _mpd_idiv_word(&len, &r, prec, MPD_RDIGITS);
            len = (r == 0) ? len : len+1;

            if (r != 0) {
                 result->data[len-1] %= mpd_pow10[r];
            }

            len = _mpd_real_size(result->data, len);
            /* resize to fewer words cannot fail */
            mpd_qresize(result, len, &dummy);
            result->len = len;
            mpd_setdigits(result);
            if (mpd_iszerocoeff(result)) {
                /* NaN0 is not a valid representation */
                result->len = result->digits = 0;
            }
        }
    }
}

/*
 * Get n most significant digits from a decimal, where 0 < n <= MPD_UINT_DIGITS.
 * Assumes MPD_UINT_DIGITS == MPD_RDIGITS+1, which is true for 32 and 64 bit
 * machines.
 *
 * The result of the operation will be in lo. If the operation is impossible,
 * hi will be nonzero. This is used to indicate an error.
 */
static inline void
_mpd_get_msdigits(mpd_uint_t *hi, mpd_uint_t *lo, const mpd_t *dec,
                  unsigned int n)
{
    mpd_uint_t r, tmp;

    assert(0 < n && n <= MPD_RDIGITS+1);

    _mpd_div_word(&tmp, &r, dec->digits, MPD_RDIGITS);
    r = (r == 0) ? MPD_RDIGITS : r; /* digits in the most significant word */

    *hi = 0;
    *lo = dec->data[dec->len-1];
    if (n <= r) {
        *lo /= mpd_pow10[r-n];
    }
    else if (dec->len > 1) {
        /* at this point 1 <= r < n <= MPD_RDIGITS+1 */
        _mpd_mul_words(hi, lo, *lo, mpd_pow10[n-r]);
        tmp = dec->data[dec->len-2] / mpd_pow10[MPD_RDIGITS-(n-r)];
        *lo = *lo + tmp;
        if (*lo < tmp) (*hi)++;
    }
}


/******************************************************************************/
/*                   Gathering information about a decimal                    */
/******************************************************************************/

/* The real size of the coefficient without leading zero words. */
static inline mpd_ssize_t
_mpd_real_size(mpd_uint_t *data, mpd_ssize_t size)
{
    while (size > 1 && data[size-1] == 0) {
        size--;
    }

    return size;
}

/* Return number of trailing zeros. No errors are possible. */
mpd_ssize_t
mpd_trail_zeros(const mpd_t *dec)
{
    mpd_uint_t word;
    mpd_ssize_t i, tz = 0;

    for (i=0; i < dec->len; ++i) {
        if (dec->data[i] != 0) {
            word = dec->data[i];
            tz = i * MPD_RDIGITS;
            while (word % 10 == 0) {
                word /= 10;
                tz++;
            }
            break;
        }
    }

    return tz;
}

/* Integer: Undefined for specials */
static int
_mpd_isint(const mpd_t *dec)
{
    mpd_ssize_t tz;

    if (mpd_iszerocoeff(dec)) {
        return 1;
    }

    tz = mpd_trail_zeros(dec);
    return (dec->exp + tz >= 0);
}

/* Integer */
int
mpd_isinteger(const mpd_t *dec)
{
    if (mpd_isspecial(dec)) {
        return 0;
    }
    return _mpd_isint(dec);
}

/* Word is a power of 10 */
static int
mpd_word_ispow10(mpd_uint_t word)
{
    int n;

    n = mpd_word_digits(word);
    if (word == mpd_pow10[n-1]) {
        return 1;
    }

    return 0;
}

/* Coefficient is a power of 10 */
static int
mpd_coeff_ispow10(const mpd_t *dec)
{
    if (mpd_word_ispow10(mpd_msword(dec))) {
        if (_mpd_isallzero(dec->data, dec->len-1)) {
            return 1;
        }
    }

    return 0;
}

/* All digits of a word are nines */
static int
mpd_word_isallnine(mpd_uint_t word)
{
    int n;

    n = mpd_word_digits(word);
    if (word == mpd_pow10[n]-1) {
        return 1;
    }

    return 0;
}

/* All digits of the coefficient are nines */
static int
mpd_coeff_isallnine(const mpd_t *dec)
{
    if (mpd_word_isallnine(mpd_msword(dec))) {
        if (_mpd_isallnine(dec->data, dec->len-1)) {
            return 1;
        }
    }

    return 0;
}

/* Odd decimal: Undefined for non-integers! */
int
mpd_isodd(const mpd_t *dec)
{
    mpd_uint_t q, r;
    assert(mpd_isinteger(dec));
    if (mpd_iszerocoeff(dec)) return 0;
    if (dec->exp < 0) {
        _mpd_div_word(&q, &r, -dec->exp, MPD_RDIGITS);
        q = dec->data[q] / mpd_pow10[r];
        return mpd_isoddword(q);
    }
    return dec->exp == 0 && mpd_isoddword(dec->data[0]);
}

/* Even: Undefined for non-integers! */
int
mpd_iseven(const mpd_t *dec)
{
    return !mpd_isodd(dec);
}

/******************************************************************************/
/*                      Getting and setting decimals                          */
/******************************************************************************/

/* Internal function: Set a static decimal from a triple, no error checking. */
static void
_ssettriple(mpd_t *result, uint8_t sign, mpd_uint_t a, mpd_ssize_t exp)
{
    mpd_set_flags(result, sign);
    result->exp = exp;
    _mpd_div_word(&result->data[1], &result->data[0], a, MPD_RADIX);
    result->len = (result->data[1] == 0) ? 1 : 2;
    mpd_setdigits(result);
}

/* Internal function: Set a decimal from a triple, no error checking. */
static void
_settriple(mpd_t *result, uint8_t sign, mpd_uint_t a, mpd_ssize_t exp)
{
    mpd_minalloc(result);
    mpd_set_flags(result, sign);
    result->exp = exp;
    _mpd_div_word(&result->data[1], &result->data[0], a, MPD_RADIX);
    result->len = (result->data[1] == 0) ? 1 : 2;
    mpd_setdigits(result);
}

/* Set a special number from a triple */
void
mpd_setspecial(mpd_t *result, uint8_t sign, uint8_t type)
{
    mpd_minalloc(result);
    result->flags &= ~(MPD_NEG|MPD_SPECIAL);
    result->flags |= (sign|type);
    result->exp = result->digits = result->len = 0;
}

/* Set result of NaN with an error status */
void
mpd_seterror(mpd_t *result, uint32_t flags, uint32_t *status)
{
    mpd_minalloc(result);
    mpd_set_qnan(result);
    mpd_set_positive(result);
    result->exp = result->digits = result->len = 0;
    *status |= flags;
}

/* quietly set a static decimal from an mpd_ssize_t */
void
mpd_qsset_ssize(mpd_t *result, mpd_ssize_t a, const mpd_context_t *ctx,
                uint32_t *status)
{
    mpd_uint_t u;
    uint8_t sign = MPD_POS;

    if (a < 0) {
        if (a == MPD_SSIZE_MIN) {
            u = (mpd_uint_t)MPD_SSIZE_MAX +
                (-(MPD_SSIZE_MIN+MPD_SSIZE_MAX));
        }
        else {
            u = -a;
        }
        sign = MPD_NEG;
    }
    else {
        u = a;
    }
    _ssettriple(result, sign, u, 0);
    mpd_qfinalize(result, ctx, status);
}

/* quietly set a static decimal from an mpd_uint_t */
void
mpd_qsset_uint(mpd_t *result, mpd_uint_t a, const mpd_context_t *ctx,
               uint32_t *status)
{
    _ssettriple(result, MPD_POS, a, 0);
    mpd_qfinalize(result, ctx, status);
}

/* quietly set a static decimal from an int32_t */
void
mpd_qsset_i32(mpd_t *result, int32_t a, const mpd_context_t *ctx,
              uint32_t *status)
{
    mpd_qsset_ssize(result, a, ctx, status);
}

/* quietly set a static decimal from a uint32_t */
void
mpd_qsset_u32(mpd_t *result, uint32_t a, const mpd_context_t *ctx,
              uint32_t *status)
{
    mpd_qsset_uint(result, a, ctx, status);
}

#ifdef CONFIG_64
/* quietly set a static decimal from an int64_t */
void
mpd_qsset_i64(mpd_t *result, int64_t a, const mpd_context_t *ctx,
              uint32_t *status)
{
    mpd_qsset_ssize(result, a, ctx, status);
}

/* quietly set a static decimal from a uint64_t */
void
mpd_qsset_u64(mpd_t *result, uint64_t a, const mpd_context_t *ctx,
              uint32_t *status)
{
    mpd_qsset_uint(result, a, ctx, status);
}
#endif

/* quietly set a decimal from an mpd_ssize_t */
void
mpd_qset_ssize(mpd_t *result, mpd_ssize_t a, const mpd_context_t *ctx,
               uint32_t *status)
{
    mpd_minalloc(result);
    mpd_qsset_ssize(result, a, ctx, status);
}

/* quietly set a decimal from an mpd_uint_t */
void
mpd_qset_uint(mpd_t *result, mpd_uint_t a, const mpd_context_t *ctx,
              uint32_t *status)
{
    _settriple(result, MPD_POS, a, 0);
    mpd_qfinalize(result, ctx, status);
}

/* quietly set a decimal from an int32_t */
void
mpd_qset_i32(mpd_t *result, int32_t a, const mpd_context_t *ctx,
             uint32_t *status)
{
    mpd_qset_ssize(result, a, ctx, status);
}

/* quietly set a decimal from a uint32_t */
void
mpd_qset_u32(mpd_t *result, uint32_t a, const mpd_context_t *ctx,
             uint32_t *status)
{
    mpd_qset_uint(result, a, ctx, status);
}

#if defined(CONFIG_32) && !defined(LEGACY_COMPILER)
/* set a decimal from a uint64_t */
static void
_c32setu64(mpd_t *result, uint64_t u, uint8_t sign, uint32_t *status)
{
    mpd_uint_t w[3];
    uint64_t q;
    int i, len;

    len = 0;
    do {
        q = u / MPD_RADIX;
        w[len] = (mpd_uint_t)(u - q * MPD_RADIX);
        u = q; len++;
    } while (u != 0);

    if (!mpd_qresize(result, len, status)) {
        return;
    }
    for (i = 0; i < len; i++) {
        result->data[i] = w[i];
    }

    mpd_set_flags(result, sign);
    result->exp = 0;
    result->len = len;
    mpd_setdigits(result);
}

static void
_c32_qset_u64(mpd_t *result, uint64_t a, const mpd_context_t *ctx,
              uint32_t *status)
{
    _c32setu64(result, a, MPD_POS, status);
    mpd_qfinalize(result, ctx, status);
}

/* set a decimal from an int64_t */
static void
_c32_qset_i64(mpd_t *result, int64_t a, const mpd_context_t *ctx,
              uint32_t *status)
{
    uint64_t u;
    uint8_t sign = MPD_POS;

    if (a < 0) {
        if (a == INT64_MIN) {
            u = (uint64_t)INT64_MAX + (-(INT64_MIN+INT64_MAX));
        }
        else {
            u = -a;
        }
        sign = MPD_NEG;
    }
    else {
        u = a;
    }
    _c32setu64(result, u, sign, status);
    mpd_qfinalize(result, ctx, status);
}
#endif /* CONFIG_32 && !LEGACY_COMPILER */

#ifndef LEGACY_COMPILER
/* quietly set a decimal from an int64_t */
void
mpd_qset_i64(mpd_t *result, int64_t a, const mpd_context_t *ctx,
             uint32_t *status)
{
#ifdef CONFIG_64
    mpd_qset_ssize(result, a, ctx, status);
#else
    _c32_qset_i64(result, a, ctx, status);
#endif
}

/* quietly set a decimal from an int64_t, use a maxcontext for conversion */
void
mpd_qset_i64_exact(mpd_t *result, int64_t a, uint32_t *status)
{
    mpd_context_t maxcontext;

    mpd_maxcontext(&maxcontext);
#ifdef CONFIG_64
    mpd_qset_ssize(result, a, &maxcontext, status);
#else
    _c32_qset_i64(result, a, &maxcontext, status);
#endif

    if (*status & (MPD_Inexact|MPD_Rounded|MPD_Clamped)) {
        /* we want exact results */
        mpd_seterror(result, MPD_Invalid_operation, status);
    }
    *status &= MPD_Errors;
}

/* quietly set a decimal from a uint64_t */
void
mpd_qset_u64(mpd_t *result, uint64_t a, const mpd_context_t *ctx,
             uint32_t *status)
{
#ifdef CONFIG_64
    mpd_qset_uint(result, a, ctx, status);
#else
    _c32_qset_u64(result, a, ctx, status);
#endif
}

/* quietly set a decimal from a uint64_t, use a maxcontext for conversion */
void
mpd_qset_u64_exact(mpd_t *result, uint64_t a, uint32_t *status)
{
    mpd_context_t maxcontext;

    mpd_maxcontext(&maxcontext);
#ifdef CONFIG_64
    mpd_qset_uint(result, a, &maxcontext, status);
#else
    _c32_qset_u64(result, a, &maxcontext, status);
#endif

    if (*status & (MPD_Inexact|MPD_Rounded|MPD_Clamped)) {
        /* we want exact results */
        mpd_seterror(result, MPD_Invalid_operation, status);
    }
    *status &= MPD_Errors;
}
#endif /* !LEGACY_COMPILER */

/*
 * Quietly get an mpd_uint_t from a decimal. Assumes
 * MPD_UINT_DIGITS == MPD_RDIGITS+1, which is true for
 * 32 and 64 bit machines.
 *
 * If the operation is impossible, MPD_Invalid_operation is set.
 */
static mpd_uint_t
_mpd_qget_uint(int use_sign, const mpd_t *a, uint32_t *status)
{
    mpd_t tmp;
    mpd_uint_t tmp_data[2];
    mpd_uint_t lo, hi;

    if (mpd_isspecial(a)) {
        *status |= MPD_Invalid_operation;
        return MPD_UINT_MAX;
    }
    if (mpd_iszero(a)) {
        return 0;
    }
    if (use_sign && mpd_isnegative(a)) {
        *status |= MPD_Invalid_operation;
        return MPD_UINT_MAX;
    }

    if (a->digits+a->exp > MPD_RDIGITS+1) {
        *status |= MPD_Invalid_operation;
        return MPD_UINT_MAX;
    }

    if (a->exp < 0) {
        if (!_mpd_isint(a)) {
            *status |= MPD_Invalid_operation;
            return MPD_UINT_MAX;
        }
        /* At this point a->digits+a->exp <= MPD_RDIGITS+1,
         * so the shift fits. */
        tmp.data = tmp_data;
        tmp.flags = MPD_STATIC|MPD_STATIC_DATA;
        tmp.alloc = 2;
        mpd_qsshiftr(&tmp, a, -a->exp);
        tmp.exp = 0;
        a = &tmp;
    }

    _mpd_get_msdigits(&hi, &lo, a, MPD_RDIGITS+1);
    if (hi) {
        *status |= MPD_Invalid_operation;
        return MPD_UINT_MAX;
    }

    if (a->exp > 0) {
        _mpd_mul_words(&hi, &lo, lo, mpd_pow10[a->exp]);
        if (hi) {
            *status |= MPD_Invalid_operation;
            return MPD_UINT_MAX;
        }
    }

    return lo;
}

/*
 * Sets Invalid_operation for:
 *   - specials
 *   - negative numbers (except negative zero)
 *   - non-integers
 *   - overflow
 */
mpd_uint_t
mpd_qget_uint(const mpd_t *a, uint32_t *status)
{
    return _mpd_qget_uint(1, a, status);
}

/* Same as above, but gets the absolute value, i.e. the sign is ignored. */
mpd_uint_t
mpd_qabs_uint(const mpd_t *a, uint32_t *status)
{
    return _mpd_qget_uint(0, a, status);
}

/* quietly get an mpd_ssize_t from a decimal */
mpd_ssize_t
mpd_qget_ssize(const mpd_t *a, uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_uint_t u;
    int isneg;

    u = mpd_qabs_uint(a, &workstatus);
    if (workstatus&MPD_Invalid_operation) {
        *status |= workstatus;
        return MPD_SSIZE_MAX;
    }

    isneg = mpd_isnegative(a);
    if (u <= MPD_SSIZE_MAX) {
        return isneg ? -((mpd_ssize_t)u) : (mpd_ssize_t)u;
    }
    else if (isneg && u+(MPD_SSIZE_MIN+MPD_SSIZE_MAX) == MPD_SSIZE_MAX) {
        return MPD_SSIZE_MIN;
    }

    *status |= MPD_Invalid_operation;
    return MPD_SSIZE_MAX;
}

#if defined(CONFIG_32) && !defined(LEGACY_COMPILER)
/*
 * Quietly get a uint64_t from a decimal. If the operation is impossible,
 * MPD_Invalid_operation is set.
 */
static uint64_t
_c32_qget_u64(int use_sign, const mpd_t *a, uint32_t *status)
{
    MPD_NEW_STATIC(tmp,0,0,20,3);
    mpd_context_t maxcontext;
    uint64_t ret;

    tmp_data[0] = 709551615;
    tmp_data[1] = 446744073;
    tmp_data[2] = 18;

    if (mpd_isspecial(a)) {
        *status |= MPD_Invalid_operation;
        return UINT64_MAX;
    }
    if (mpd_iszero(a)) {
        return 0;
    }
    if (use_sign && mpd_isnegative(a)) {
        *status |= MPD_Invalid_operation;
        return UINT64_MAX;
    }
    if (!_mpd_isint(a)) {
        *status |= MPD_Invalid_operation;
        return UINT64_MAX;
    }

    if (_mpd_cmp_abs(a, &tmp) > 0) {
        *status |= MPD_Invalid_operation;
        return UINT64_MAX;
    }

    mpd_maxcontext(&maxcontext);
    mpd_qrescale(&tmp, a, 0, &maxcontext, &maxcontext.status);
    maxcontext.status &= ~MPD_Rounded;
    if (maxcontext.status != 0) {
        *status |= (maxcontext.status|MPD_Invalid_operation); /* GCOV_NOT_REACHED */
        return UINT64_MAX; /* GCOV_NOT_REACHED */
    }

    ret = 0;
    switch (tmp.len) {
    case 3:
        ret += (uint64_t)tmp_data[2] * 1000000000000000000ULL;
    case 2:
        ret += (uint64_t)tmp_data[1] * 1000000000ULL;
    case 1:
        ret += tmp_data[0];
        break;
    default:
        abort(); /* GCOV_NOT_REACHED */
    }

    return ret;
}

static int64_t
_c32_qget_i64(const mpd_t *a, uint32_t *status)
{
    uint64_t u;
    int isneg;

    u = _c32_qget_u64(0, a, status);
    if (*status&MPD_Invalid_operation) {
        return INT64_MAX;
    }

    isneg = mpd_isnegative(a);
    if (u <= INT64_MAX) {
        return isneg ? -((int64_t)u) : (int64_t)u;
    }
    else if (isneg && u+(INT64_MIN+INT64_MAX) == INT64_MAX) {
        return INT64_MIN;
    }

    *status |= MPD_Invalid_operation;
    return INT64_MAX;
}
#endif /* CONFIG_32 && !LEGACY_COMPILER */

#ifdef CONFIG_64
/* quietly get a uint64_t from a decimal */
uint64_t
mpd_qget_u64(const mpd_t *a, uint32_t *status)
{
    return mpd_qget_uint(a, status);
}

/* quietly get an int64_t from a decimal */
int64_t
mpd_qget_i64(const mpd_t *a, uint32_t *status)
{
    return mpd_qget_ssize(a, status);
}

/* quietly get a uint32_t from a decimal */
uint32_t
mpd_qget_u32(const mpd_t *a, uint32_t *status)
{
    uint32_t workstatus = 0;
    uint64_t x = mpd_qget_uint(a, &workstatus);

    if (workstatus&MPD_Invalid_operation) {
        *status |= workstatus;
        return UINT32_MAX;
    }
    if (x > UINT32_MAX) {
        *status |= MPD_Invalid_operation;
        return UINT32_MAX;
    }

    return (uint32_t)x;
}

/* quietly get an int32_t from a decimal */
int32_t
mpd_qget_i32(const mpd_t *a, uint32_t *status)
{
    uint32_t workstatus = 0;
    int64_t x = mpd_qget_ssize(a, &workstatus);

    if (workstatus&MPD_Invalid_operation) {
        *status |= workstatus;
        return INT32_MAX;
    }
    if (x < INT32_MIN || x > INT32_MAX) {
        *status |= MPD_Invalid_operation;
        return INT32_MAX;
    }

    return (int32_t)x;
}
#else
#ifndef LEGACY_COMPILER
/* quietly get a uint64_t from a decimal */
uint64_t
mpd_qget_u64(const mpd_t *a, uint32_t *status)
{
    uint32_t workstatus = 0;
    uint64_t x = _c32_qget_u64(1, a, &workstatus);
    *status |= workstatus;
    return x;
}

/* quietly get an int64_t from a decimal */
int64_t
mpd_qget_i64(const mpd_t *a, uint32_t *status)
{
    uint32_t workstatus = 0;
    int64_t x = _c32_qget_i64(a, &workstatus);
    *status |= workstatus;
    return x;
}
#endif

/* quietly get a uint32_t from a decimal */
uint32_t
mpd_qget_u32(const mpd_t *a, uint32_t *status)
{
    return mpd_qget_uint(a, status);
}

/* quietly get an int32_t from a decimal */
int32_t
mpd_qget_i32(const mpd_t *a, uint32_t *status)
{
    return mpd_qget_ssize(a, status);
}
#endif


/******************************************************************************/
/*         Filtering input of functions, finalizing output of functions       */
/******************************************************************************/

/*
 * Check if the operand is NaN, copy to result and return 1 if this is
 * the case. Copying can fail since NaNs are allowed to have a payload that
 * does not fit in MPD_MINALLOC.
 */
int
mpd_qcheck_nan(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
               uint32_t *status)
{
    if (mpd_isnan(a)) {
        *status |= mpd_issnan(a) ? MPD_Invalid_operation : 0;
        mpd_qcopy(result, a, status);
        mpd_set_qnan(result);
        _mpd_fix_nan(result, ctx);
        return 1;
    }
    return 0;
}

/*
 * Check if either operand is NaN, copy to result and return 1 if this
 * is the case. Copying can fail since NaNs are allowed to have a payload
 * that does not fit in MPD_MINALLOC.
 */
int
mpd_qcheck_nans(mpd_t *result, const mpd_t *a, const mpd_t *b,
                const mpd_context_t *ctx, uint32_t *status)
{
    if ((a->flags|b->flags)&(MPD_NAN|MPD_SNAN)) {
        const mpd_t *choice = b;
        if (mpd_issnan(a)) {
            choice = a;
            *status |= MPD_Invalid_operation;
        }
        else if (mpd_issnan(b)) {
            *status |= MPD_Invalid_operation;
        }
        else if (mpd_isqnan(a)) {
            choice = a;
        }
        mpd_qcopy(result, choice, status);
        mpd_set_qnan(result);
        _mpd_fix_nan(result, ctx);
        return 1;
    }
    return 0;
}

/*
 * Check if one of the operands is NaN, copy to result and return 1 if this
 * is the case. Copying can fail since NaNs are allowed to have a payload
 * that does not fit in MPD_MINALLOC.
 */
static int
mpd_qcheck_3nans(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_t *c,
                 const mpd_context_t *ctx, uint32_t *status)
{
    if ((a->flags|b->flags|c->flags)&(MPD_NAN|MPD_SNAN)) {
        const mpd_t *choice = c;
        if (mpd_issnan(a)) {
            choice = a;
            *status |= MPD_Invalid_operation;
        }
        else if (mpd_issnan(b)) {
            choice = b;
            *status |= MPD_Invalid_operation;
        }
        else if (mpd_issnan(c)) {
            *status |= MPD_Invalid_operation;
        }
        else if (mpd_isqnan(a)) {
            choice = a;
        }
        else if (mpd_isqnan(b)) {
            choice = b;
        }
        mpd_qcopy(result, choice, status);
        mpd_set_qnan(result);
        _mpd_fix_nan(result, ctx);
        return 1;
    }
    return 0;
}

/* Check if rounding digit 'rnd' leads to an increment. */
static inline int
_mpd_rnd_incr(const mpd_t *dec, mpd_uint_t rnd, const mpd_context_t *ctx)
{
    int ld;

    switch (ctx->round) {
    case MPD_ROUND_DOWN: case MPD_ROUND_TRUNC:
        return 0;
    case MPD_ROUND_HALF_UP:
        return (rnd >= 5);
    case MPD_ROUND_HALF_EVEN:
        return (rnd > 5) || ((rnd == 5) && mpd_isoddcoeff(dec));
    case MPD_ROUND_CEILING:
        return !(rnd == 0 || mpd_isnegative(dec));
    case MPD_ROUND_FLOOR:
        return !(rnd == 0 || mpd_ispositive(dec));
    case MPD_ROUND_HALF_DOWN:
        return (rnd > 5);
    case MPD_ROUND_UP:
        return !(rnd == 0);
    case MPD_ROUND_05UP:
        ld = (int)mpd_lsd(dec->data[0]);
        return (!(rnd == 0) && (ld == 0 || ld == 5));
    default:
        /* Without a valid context, further results will be undefined. */
        return 0; /* GCOV_NOT_REACHED */
    }
}

/*
 * Apply rounding to a decimal that has been right-shifted into a full
 * precision decimal. If an increment leads to an overflow of the precision,
 * adjust the coefficient and the exponent and check the new exponent for
 * overflow.
 */
static inline void
_mpd_apply_round(mpd_t *dec, mpd_uint_t rnd, const mpd_context_t *ctx,
                 uint32_t *status)
{
    if (_mpd_rnd_incr(dec, rnd, ctx)) {
        /* We have a number with exactly ctx->prec digits. The increment
         * can only lead to an overflow if the decimal is all nines. In
         * that case, the result is a power of ten with prec+1 digits.
         *
         * If the precision is a multiple of MPD_RDIGITS, this situation is
         * detected by _mpd_baseincr returning a carry.
         * If the precision is not a multiple of MPD_RDIGITS, we have to
         * check if the result has one digit too many.
         */
        mpd_uint_t carry = _mpd_baseincr(dec->data, dec->len);
        if (carry) {
            dec->data[dec->len-1] = mpd_pow10[MPD_RDIGITS-1];
            dec->exp += 1;
            _mpd_check_exp(dec, ctx, status);
            return;
        }
        mpd_setdigits(dec);
        if (dec->digits > ctx->prec) {
            mpd_qshiftr_inplace(dec, 1);
            dec->exp += 1;
            dec->digits = ctx->prec;
            _mpd_check_exp(dec, ctx, status);
        }
    }
}

/*
 * Apply rounding to a decimal. Allow overflow of the precision.
 */
static inline void
_mpd_apply_round_excess(mpd_t *dec, mpd_uint_t rnd, const mpd_context_t *ctx,
                        uint32_t *status)
{
    if (_mpd_rnd_incr(dec, rnd, ctx)) {
        mpd_uint_t carry = _mpd_baseincr(dec->data, dec->len);
        if (carry) {
            if (!mpd_qresize(dec, dec->len+1, status)) {
                return;
            }
            dec->data[dec->len] = 1;
            dec->len += 1;
        }
        mpd_setdigits(dec);
    }
}

/*
 * Apply rounding to a decimal that has been right-shifted into a decimal
 * with full precision or less. Return failure if an increment would
 * overflow the precision.
 */
static inline int
_mpd_apply_round_fit(mpd_t *dec, mpd_uint_t rnd, const mpd_context_t *ctx,
                     uint32_t *status)
{
    if (_mpd_rnd_incr(dec, rnd, ctx)) {
        mpd_uint_t carry = _mpd_baseincr(dec->data, dec->len);
        if (carry) {
            if (!mpd_qresize(dec, dec->len+1, status)) {
                return 0;
            }
            dec->data[dec->len] = 1;
            dec->len += 1;
        }
        mpd_setdigits(dec);
        if (dec->digits > ctx->prec) {
            mpd_seterror(dec, MPD_Invalid_operation, status);
            return 0;
        }
    }
    return 1;
}

/* Check a normal number for overflow, underflow, clamping. If the operand
   is modified, it will be zero, special or (sub)normal with a coefficient
   that fits into the current context precision. */
static inline void
_mpd_check_exp(mpd_t *dec, const mpd_context_t *ctx, uint32_t *status)
{
    mpd_ssize_t adjexp, etiny, shift;
    int rnd;

    adjexp = mpd_adjexp(dec);
    if (adjexp > ctx->emax) {

        if (mpd_iszerocoeff(dec)) {
            dec->exp = ctx->emax;
            if (ctx->clamp) {
                dec->exp -= (ctx->prec-1);
            }
            mpd_zerocoeff(dec);
            *status |= MPD_Clamped;
            return;
        }

        switch (ctx->round) {
        case MPD_ROUND_HALF_UP: case MPD_ROUND_HALF_EVEN:
        case MPD_ROUND_HALF_DOWN: case MPD_ROUND_UP:
        case MPD_ROUND_TRUNC:
            mpd_setspecial(dec, mpd_sign(dec), MPD_INF);
            break;
        case MPD_ROUND_DOWN: case MPD_ROUND_05UP:
            mpd_qmaxcoeff(dec, ctx, status);
            dec->exp = ctx->emax - ctx->prec + 1;
            break;
        case MPD_ROUND_CEILING:
            if (mpd_isnegative(dec)) {
                mpd_qmaxcoeff(dec, ctx, status);
                dec->exp = ctx->emax - ctx->prec + 1;
            }
            else {
                mpd_setspecial(dec, MPD_POS, MPD_INF);
            }
            break;
        case MPD_ROUND_FLOOR:
            if (mpd_ispositive(dec)) {
                mpd_qmaxcoeff(dec, ctx, status);
                dec->exp = ctx->emax - ctx->prec + 1;
            }
            else {
                mpd_setspecial(dec, MPD_NEG, MPD_INF);
            }
            break;
        default: /* debug */
            abort(); /* GCOV_NOT_REACHED */
        }

        *status |= MPD_Overflow|MPD_Inexact|MPD_Rounded;

    } /* fold down */
    else if (ctx->clamp && dec->exp > mpd_etop(ctx)) {
        /* At this point adjexp=exp+digits-1 <= emax and exp > etop=emax-prec+1:
         *   (1) shift = exp -emax+prec-1 > 0
         *   (2) digits+shift = exp+digits-1 - emax + prec <= prec */
        shift = dec->exp - mpd_etop(ctx);
        if (!mpd_qshiftl(dec, dec, shift, status)) {
            return;
        }
        dec->exp -= shift;
        *status |= MPD_Clamped;
        if (!mpd_iszerocoeff(dec) && adjexp < ctx->emin) {
            /* Underflow is impossible, since exp < etiny=emin-prec+1
             * and exp > etop=emax-prec+1 would imply emax < emin. */
            *status |= MPD_Subnormal;
        }
    }
    else if (adjexp < ctx->emin) {

        etiny = mpd_etiny(ctx);

        if (mpd_iszerocoeff(dec)) {
            if (dec->exp < etiny) {
                dec->exp = etiny;
                mpd_zerocoeff(dec);
                *status |= MPD_Clamped;
            }
            return;
        }

        *status |= MPD_Subnormal;
        if (dec->exp < etiny) {
            /* At this point adjexp=exp+digits-1 < emin and exp < etiny=emin-prec+1:
             *   (1) shift = emin-prec+1 - exp > 0
             *   (2) digits-shift = exp+digits-1 - emin + prec < prec */
            shift = etiny - dec->exp;
            rnd = (int)mpd_qshiftr_inplace(dec, shift);
            dec->exp = etiny;
            /* We always have a spare digit in case of an increment. */
            _mpd_apply_round_excess(dec, rnd, ctx, status);
            *status |= MPD_Rounded;
            if (rnd) {
                *status |= (MPD_Inexact|MPD_Underflow);
                if (mpd_iszerocoeff(dec)) {
                    mpd_zerocoeff(dec);
                    *status |= MPD_Clamped;
                }
            }
        }
        /* Case exp >= etiny=emin-prec+1:
         *   (1) adjexp=exp+digits-1 < emin
         *   (2) digits < emin-exp+1 <= prec */
    }
}

/* Transcendental functions do not always set Underflow reliably,
 * since they only use as much precision as is necessary for correct
 * rounding. If a result like 1.0000000000e-101 is finalized, there
 * is no rounding digit that would trigger Underflow. But we can
 * assume Inexact, so a short check suffices. */
static inline void
mpd_check_underflow(mpd_t *dec, const mpd_context_t *ctx, uint32_t *status)
{
    if (mpd_adjexp(dec) < ctx->emin && !mpd_iszero(dec) &&
        dec->exp < mpd_etiny(ctx)) {
        *status |= MPD_Underflow;
    }
}

/* Check if a normal number must be rounded after the exponent has been checked. */
static inline void
_mpd_check_round(mpd_t *dec, const mpd_context_t *ctx, uint32_t *status)
{
    mpd_uint_t rnd;
    mpd_ssize_t shift;

    /* must handle specials: _mpd_check_exp() can produce infinities or NaNs */
    if (mpd_isspecial(dec)) {
        return;
    }

    if (dec->digits > ctx->prec) {
        shift = dec->digits - ctx->prec;
        rnd = mpd_qshiftr_inplace(dec, shift);
        dec->exp += shift;
        _mpd_apply_round(dec, rnd, ctx, status);
        *status |= MPD_Rounded;
        if (rnd) {
            *status |= MPD_Inexact;
        }
    }
}

/* Finalize all operations. */
void
mpd_qfinalize(mpd_t *result, const mpd_context_t *ctx, uint32_t *status)
{
    if (mpd_isspecial(result)) {
        if (mpd_isnan(result)) {
            _mpd_fix_nan(result, ctx);
        }
        return;
    }

    _mpd_check_exp(result, ctx, status);
    _mpd_check_round(result, ctx, status);
}


/******************************************************************************/
/*                                 Copying                                    */
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

/*
 * Copy a decimal. In case of an error, status is set to MPD_Malloc_error.
 */
int
mpd_qcopy(mpd_t *result, const mpd_t *a, uint32_t *status)
{
    if (result == a) return 1;

    if (!mpd_qresize(result, a->len, status)) {
        return 0;
    }

    mpd_copy_flags(result, a);
    result->exp = a->exp;
    result->digits = a->digits;
    result->len = a->len;
    memcpy(result->data, a->data, a->len * (sizeof *result->data));

    return 1;
}

/* Same as mpd_qcopy, but do not set the result to NaN on failure. */
int
mpd_qcopy_cxx(mpd_t *result, const mpd_t *a)
{
    if (result == a) return 1;

    if (!mpd_qresize_cxx(result, a->len)) {
        return 0;
    }

    mpd_copy_flags(result, a);
    result->exp = a->exp;
    result->digits = a->digits;
    result->len = a->len;
    memcpy(result->data, a->data, a->len * (sizeof *result->data));

    return 1;
}

/*
 * Copy to a decimal with a static buffer. The caller has to make sure that
 * the buffer is big enough. Cannot fail.
 */
static void
mpd_qcopy_static(mpd_t *result, const mpd_t *a)
{
    if (result == a) return;

    memcpy(result->data, a->data, a->len * (sizeof *result->data));

    mpd_copy_flags(result, a);
    result->exp = a->exp;
    result->digits = a->digits;
    result->len = a->len;
}

/*
 * Return a newly allocated copy of the operand. In case of an error,
 * status is set to MPD_Malloc_error and the return value is NULL.
 */
mpd_t *
mpd_qncopy(const mpd_t *a)
{
    mpd_t *result;

    if ((result = mpd_qnew_size(a->len)) == NULL) {
        return NULL;
    }
    memcpy(result->data, a->data, a->len * (sizeof *result->data));
    mpd_copy_flags(result, a);
    result->exp = a->exp;
    result->digits = a->digits;
    result->len = a->len;

    return result;
}

/*
 * Copy a decimal and set the sign to positive. In case of an error, the
 * status is set to MPD_Malloc_error.
 */
int
mpd_qcopy_abs(mpd_t *result, const mpd_t *a, uint32_t *status)
{
    if (!mpd_qcopy(result, a, status)) {
        return 0;
    }
    mpd_set_positive(result);
    return 1;
}

/*
 * Copy a decimal and negate the sign. In case of an error, the
 * status is set to MPD_Malloc_error.
 */
int
mpd_qcopy_negate(mpd_t *result, const mpd_t *a, uint32_t *status)
{
    if (!mpd_qcopy(result, a, status)) {
        return 0;
    }
    _mpd_negate(result);
    return 1;
}

/*
 * Copy a decimal, setting the sign of the first operand to the sign of the
 * second operand. In case of an error, the status is set to MPD_Malloc_error.
 */
int
mpd_qcopy_sign(mpd_t *result, const mpd_t *a, const mpd_t *b, uint32_t *status)
{
    uint8_t sign_b = mpd_sign(b); /* result may equal b! */

    if (!mpd_qcopy(result, a, status)) {
        return 0;
    }
    mpd_set_sign(result, sign_b);
    return 1;
}


/******************************************************************************/
/*                                Comparisons                                 */
/******************************************************************************/

/*
 * For all functions that compare two operands and return an int the usual
 * convention applies to the return value:
 *
 * -1 if op1 < op2
 *  0 if op1 == op2
 *  1 if op1 > op2
 *
 *  INT_MAX for error
 */


/* Convenience macro. If a and b are not equal, return from the calling
 * function with the correct comparison value. */
#define CMP_EQUAL_OR_RETURN(a, b)  \
        if (a != b) {              \
                if (a < b) {       \
                        return -1; \
                }                  \
                return 1;          \
        }

/*
 * Compare the data of big and small. This function does the equivalent
 * of first shifting small to the left and then comparing the data of
 * big and small, except that no allocation for the left shift is needed.
 */
static int
_mpd_basecmp(mpd_uint_t *big, mpd_uint_t *small, mpd_size_t n, mpd_size_t m,
             mpd_size_t shift)
{
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
    /* spurious uninitialized warnings */
    mpd_uint_t l=l, lprev=lprev, h=h;
#else
    mpd_uint_t l, lprev, h;
#endif
    mpd_uint_t q, r;
    mpd_uint_t ph, x;

    assert(m > 0 && n >= m && shift > 0);

    _mpd_div_word(&q, &r, (mpd_uint_t)shift, MPD_RDIGITS);

    if (r != 0) {

        ph = mpd_pow10[r];

        --m; --n;
        _mpd_divmod_pow10(&h, &lprev, small[m--], MPD_RDIGITS-r);
        if (h != 0) {
            CMP_EQUAL_OR_RETURN(big[n], h)
            --n;
        }
        for (; m != MPD_SIZE_MAX; m--,n--) {
            _mpd_divmod_pow10(&h, &l, small[m], MPD_RDIGITS-r);
            x = ph * lprev + h;
            CMP_EQUAL_OR_RETURN(big[n], x)
            lprev = l;
        }
        x = ph * lprev;
        CMP_EQUAL_OR_RETURN(big[q], x)
    }
    else {
        while (--m != MPD_SIZE_MAX) {
            CMP_EQUAL_OR_RETURN(big[m+q], small[m])
        }
    }

    return !_mpd_isallzero(big, q);
}

/* Compare two decimals with the same adjusted exponent. */
static int
_mpd_cmp_same_adjexp(const mpd_t *a, const mpd_t *b)
{
    mpd_ssize_t shift, i;

    if (a->exp != b->exp) {
        /* Cannot wrap: a->exp + a->digits = b->exp + b->digits, so
         * a->exp - b->exp = b->digits - a->digits. */
        shift = a->exp - b->exp;
        if (shift > 0) {
            return -1 * _mpd_basecmp(b->data, a->data, b->len, a->len, shift);
        }
        else {
            return _mpd_basecmp(a->data, b->data, a->len, b->len, -shift);
        }
    }

    /*
     * At this point adjexp(a) == adjexp(b) and a->exp == b->exp,
     * so a->digits == b->digits, therefore a->len == b->len.
     */
    for (i = a->len-1; i >= 0; --i) {
        CMP_EQUAL_OR_RETURN(a->data[i], b->data[i])
    }

    return 0;
}

/* Compare two numerical values. */
static int
_mpd_cmp(const mpd_t *a, const mpd_t *b)
{
    mpd_ssize_t adjexp_a, adjexp_b;

    /* equal pointers */
    if (a == b) {
        return 0;
    }

    /* infinities */
    if (mpd_isinfinite(a)) {
        if (mpd_isinfinite(b)) {
            return mpd_isnegative(b) - mpd_isnegative(a);
        }
        return mpd_arith_sign(a);
    }
    if (mpd_isinfinite(b)) {
        return -mpd_arith_sign(b);
    }

    /* zeros */
    if (mpd_iszerocoeff(a)) {
        if (mpd_iszerocoeff(b)) {
            return 0;
        }
        return -mpd_arith_sign(b);
    }
    if (mpd_iszerocoeff(b)) {
        return mpd_arith_sign(a);
    }

    /* different signs */
    if (mpd_sign(a) != mpd_sign(b)) {
        return mpd_sign(b) - mpd_sign(a);
    }

    /* different adjusted exponents */
    adjexp_a = mpd_adjexp(a);
    adjexp_b = mpd_adjexp(b);
    if (adjexp_a != adjexp_b) {
        if (adjexp_a < adjexp_b) {
            return -1 * mpd_arith_sign(a);
        }
        return mpd_arith_sign(a);
    }

    /* same adjusted exponents */
    return _mpd_cmp_same_adjexp(a, b) * mpd_arith_sign(a);
}

/* Compare the absolutes of two numerical values. */
static int
_mpd_cmp_abs(const mpd_t *a, const mpd_t *b)
{
    mpd_ssize_t adjexp_a, adjexp_b;

    /* equal pointers */
    if (a == b) {
        return 0;
    }

    /* infinities */
    if (mpd_isinfinite(a)) {
        if (mpd_isinfinite(b)) {
            return 0;
        }
        return 1;
    }
    if (mpd_isinfinite(b)) {
        return -1;
    }

    /* zeros */
    if (mpd_iszerocoeff(a)) {
        if (mpd_iszerocoeff(b)) {
            return 0;
        }
        return -1;
    }
    if (mpd_iszerocoeff(b)) {
        return 1;
    }

    /* different adjusted exponents */
    adjexp_a = mpd_adjexp(a);
    adjexp_b = mpd_adjexp(b);
    if (adjexp_a != adjexp_b) {
        if (adjexp_a < adjexp_b) {
            return -1;
        }
        return 1;
    }

    /* same adjusted exponents */
    return _mpd_cmp_same_adjexp(a, b);
}

/* Compare two values and return an integer result. */
int
mpd_qcmp(const mpd_t *a, const mpd_t *b, uint32_t *status)
{
    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_isnan(a) || mpd_isnan(b)) {
            *status |= MPD_Invalid_operation;
            return INT_MAX;
        }
    }

    return _mpd_cmp(a, b);
}

/*
 * Compare a and b, convert the usual integer result to a decimal and
 * store it in 'result'. For convenience, the integer result of the comparison
 * is returned. Comparisons involving NaNs return NaN/INT_MAX.
 */
int
mpd_qcompare(mpd_t *result, const mpd_t *a, const mpd_t *b,
             const mpd_context_t *ctx, uint32_t *status)
{
    int c;

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            return INT_MAX;
        }
    }

    c = _mpd_cmp(a, b);
    _settriple(result, (c < 0), (c != 0), 0);
    return c;
}

/* Same as mpd_compare(), but signal for all NaNs, i.e. also for quiet NaNs. */
int
mpd_qcompare_signal(mpd_t *result, const mpd_t *a, const mpd_t *b,
                    const mpd_context_t *ctx, uint32_t *status)
{
    int c;

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            *status |= MPD_Invalid_operation;
            return INT_MAX;
        }
    }

    c = _mpd_cmp(a, b);
    _settriple(result, (c < 0), (c != 0), 0);
    return c;
}

/* Compare the operands using a total order. */
int
mpd_cmp_total(const mpd_t *a, const mpd_t *b)
{
    mpd_t aa, bb;
    int nan_a, nan_b;
    int c;

    if (mpd_sign(a) != mpd_sign(b)) {
        return mpd_sign(b) - mpd_sign(a);
    }


    if (mpd_isnan(a)) {
        c = 1;
        if (mpd_isnan(b)) {
            nan_a = (mpd_isqnan(a)) ? 1 : 0;
            nan_b = (mpd_isqnan(b)) ? 1 : 0;
            if (nan_b == nan_a) {
                if (a->len > 0 && b->len > 0) {
                    _mpd_copy_shared(&aa, a);
                    _mpd_copy_shared(&bb, b);
                    aa.exp = bb.exp = 0;
                    /* compare payload */
                    c = _mpd_cmp_abs(&aa, &bb);
                }
                else {
                    c = (a->len > 0) - (b->len > 0);
                }
            }
            else {
                c = nan_a - nan_b;
            }
        }
    }
    else if (mpd_isnan(b)) {
        c = -1;
    }
    else {
        c = _mpd_cmp_abs(a, b);
        if (c == 0 && a->exp != b->exp) {
            c = (a->exp < b->exp) ? -1 : 1;
        }
    }

    return c * mpd_arith_sign(a);
}

/*
 * Compare a and b according to a total order, convert the usual integer result
 * to a decimal and store it in 'result'. For convenience, the integer result
 * of the comparison is returned.
 */
int
mpd_compare_total(mpd_t *result, const mpd_t *a, const mpd_t *b)
{
    int c;

    c = mpd_cmp_total(a, b);
    _settriple(result, (c < 0), (c != 0), 0);
    return c;
}

/* Compare the magnitude of the operands using a total order. */
int
mpd_cmp_total_mag(const mpd_t *a, const mpd_t *b)
{
    mpd_t aa, bb;

    _mpd_copy_shared(&aa, a);
    _mpd_copy_shared(&bb, b);

    mpd_set_positive(&aa);
    mpd_set_positive(&bb);

    return mpd_cmp_total(&aa, &bb);
}

/*
 * Compare the magnitude of a and b according to a total order, convert the
 * the usual integer result to a decimal and store it in 'result'.
 * For convenience, the integer result of the comparison is returned.
 */
int
mpd_compare_total_mag(mpd_t *result, const mpd_t *a, const mpd_t *b)
{
    int c;

    c = mpd_cmp_total_mag(a, b);
    _settriple(result, (c < 0), (c != 0), 0);
    return c;
}

/* Determine an ordering for operands that are numerically equal. */
static inline int
_mpd_cmp_numequal(const mpd_t *a, const mpd_t *b)
{
    int sign_a, sign_b;
    int c;

    sign_a = mpd_sign(a);
    sign_b = mpd_sign(b);
    if (sign_a != sign_b) {
        c = sign_b - sign_a;
    }
    else {
        c = (a->exp < b->exp) ? -1 : 1;
        c *= mpd_arith_sign(a);
    }

    return c;
}


/******************************************************************************/
/*                         Shifting the coefficient                           */
/******************************************************************************/

/*
 * Shift the coefficient of the operand to the left, no check for specials.
 * Both operands may be the same pointer. If the result length has to be
 * increased, mpd_qresize() might fail with MPD_Malloc_error.
 */
int
mpd_qshiftl(mpd_t *result, const mpd_t *a, mpd_ssize_t n, uint32_t *status)
{
    mpd_ssize_t size;

    assert(!mpd_isspecial(a));
    assert(n >= 0);

    if (mpd_iszerocoeff(a) || n == 0) {
        return mpd_qcopy(result, a, status);
    }

    size = mpd_digits_to_size(a->digits+n);
    if (!mpd_qresize(result, size, status)) {
        return 0; /* result is NaN */
    }

    _mpd_baseshiftl(result->data, a->data, size, a->len, n);

    mpd_copy_flags(result, a);
    result->exp = a->exp;
    result->digits = a->digits+n;
    result->len = size;

    return 1;
}

/* Determine the rounding indicator if all digits of the coefficient are shifted
 * out of the picture. */
static mpd_uint_t
_mpd_get_rnd(const mpd_uint_t *data, mpd_ssize_t len, int use_msd)
{
    mpd_uint_t rnd = 0, rest = 0, word;

    word = data[len-1];
    /* special treatment for the most significant digit if shift == digits */
    if (use_msd) {
        _mpd_divmod_pow10(&rnd, &rest, word, mpd_word_digits(word)-1);
        if (len > 1 && rest == 0) {
             rest = !_mpd_isallzero(data, len-1);
        }
    }
    else {
        rest = !_mpd_isallzero(data, len);
    }

    return (rnd == 0 || rnd == 5) ? rnd + !!rest : rnd;
}

/*
 * Same as mpd_qshiftr(), but 'result' is an mpd_t with a static coefficient.
 * It is the caller's responsibility to ensure that the coefficient is big
 * enough. The function cannot fail.
 */
static mpd_uint_t
mpd_qsshiftr(mpd_t *result, const mpd_t *a, mpd_ssize_t n)
{
    mpd_uint_t rnd;
    mpd_ssize_t size;

    assert(!mpd_isspecial(a));
    assert(n >= 0);

    if (mpd_iszerocoeff(a) || n == 0) {
        mpd_qcopy_static(result, a);
        return 0;
    }

    if (n >= a->digits) {
        rnd = _mpd_get_rnd(a->data, a->len, (n==a->digits));
        mpd_zerocoeff(result);
    }
    else {
        result->digits = a->digits-n;
        size = mpd_digits_to_size(result->digits);
        rnd = _mpd_baseshiftr(result->data, a->data, a->len, n);
        result->len = size;
    }

    mpd_copy_flags(result, a);
    result->exp = a->exp;

    return rnd;
}

/*
 * Inplace shift of the coefficient to the right, no check for specials.
 * Returns the rounding indicator for mpd_rnd_incr().
 * The function cannot fail.
 */
mpd_uint_t
mpd_qshiftr_inplace(mpd_t *result, mpd_ssize_t n)
{
    uint32_t dummy;
    mpd_uint_t rnd;
    mpd_ssize_t size;

    assert(!mpd_isspecial(result));
    assert(n >= 0);

    if (mpd_iszerocoeff(result) || n == 0) {
        return 0;
    }

    if (n >= result->digits) {
        rnd = _mpd_get_rnd(result->data, result->len, (n==result->digits));
        mpd_zerocoeff(result);
    }
    else {
        rnd = _mpd_baseshiftr(result->data, result->data, result->len, n);
        result->digits -= n;
        size = mpd_digits_to_size(result->digits);
        /* reducing the size cannot fail */
        mpd_qresize(result, size, &dummy);
        result->len = size;
    }

    return rnd;
}

/*
 * Shift the coefficient of the operand to the right, no check for specials.
 * Both operands may be the same pointer. Returns the rounding indicator to
 * be used by mpd_rnd_incr(). If the result length has to be increased,
 * mpd_qcopy() or mpd_qresize() might fail with MPD_Malloc_error. In those
 * cases, MPD_UINT_MAX is returned.
 */
mpd_uint_t
mpd_qshiftr(mpd_t *result, const mpd_t *a, mpd_ssize_t n, uint32_t *status)
{
    mpd_uint_t rnd;
    mpd_ssize_t size;

    assert(!mpd_isspecial(a));
    assert(n >= 0);

    if (mpd_iszerocoeff(a) || n == 0) {
        if (!mpd_qcopy(result, a, status)) {
            return MPD_UINT_MAX;
        }
        return 0;
    }

    if (n >= a->digits) {
        rnd = _mpd_get_rnd(a->data, a->len, (n==a->digits));
        mpd_zerocoeff(result);
    }
    else {
        result->digits = a->digits-n;
        size = mpd_digits_to_size(result->digits);
        if (result == a) {
            rnd = _mpd_baseshiftr(result->data, a->data, a->len, n);
            /* reducing the size cannot fail */
            mpd_qresize(result, size, status);
        }
        else {
            if (!mpd_qresize(result, size, status)) {
                return MPD_UINT_MAX;
            }
            rnd = _mpd_baseshiftr(result->data, a->data, a->len, n);
        }
        result->len = size;
    }

    mpd_copy_flags(result, a);
    result->exp = a->exp;

    return rnd;
}


/******************************************************************************/
/*                         Miscellaneous operations                           */
/******************************************************************************/

/* Logical And */
void
mpd_qand(mpd_t *result, const mpd_t *a, const mpd_t *b,
         const mpd_context_t *ctx, uint32_t *status)
{
    const mpd_t *big = a, *small = b;
    mpd_uint_t x, y, z, xbit, ybit;
    int k, mswdigits;
    mpd_ssize_t i;

    if (mpd_isspecial(a) || mpd_isspecial(b) ||
        mpd_isnegative(a) || mpd_isnegative(b) ||
        a->exp != 0 || b->exp != 0) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (b->digits > a->digits) {
        big = b;
        small = a;
    }
    if (!mpd_qresize(result, big->len, status)) {
        return;
    }


    /* full words */
    for (i = 0; i < small->len-1; i++) {
        x = small->data[i];
        y = big->data[i];
        z = 0;
        for (k = 0; k < MPD_RDIGITS; k++) {
            xbit = x % 10;
            x /= 10;
            ybit = y % 10;
            y /= 10;
            if (xbit > 1 || ybit > 1) {
                goto invalid_operation;
            }
            z += (xbit&ybit) ? mpd_pow10[k] : 0;
        }
        result->data[i] = z;
    }
    /* most significant word of small */
    x = small->data[i];
    y = big->data[i];
    z = 0;
    mswdigits = mpd_word_digits(x);
    for (k = 0; k < mswdigits; k++) {
        xbit = x % 10;
        x /= 10;
        ybit = y % 10;
        y /= 10;
        if (xbit > 1 || ybit > 1) {
            goto invalid_operation;
        }
        z += (xbit&ybit) ? mpd_pow10[k] : 0;
    }
    result->data[i++] = z;

    /* scan the rest of y for digits > 1 */
    for (; k < MPD_RDIGITS; k++) {
        ybit = y % 10;
        y /= 10;
        if (ybit > 1) {
            goto invalid_operation;
        }
    }
    /* scan the rest of big for digits > 1 */
    for (; i < big->len; i++) {
        y = big->data[i];
        for (k = 0; k < MPD_RDIGITS; k++) {
            ybit = y % 10;
            y /= 10;
            if (ybit > 1) {
                goto invalid_operation;
            }
        }
    }

    mpd_clear_flags(result);
    result->exp = 0;
    result->len = _mpd_real_size(result->data, small->len);
    mpd_qresize(result, result->len, status);
    mpd_setdigits(result);
    _mpd_cap(result, ctx);
    return;

invalid_operation:
    mpd_seterror(result, MPD_Invalid_operation, status);
}

/* Class of an operand. Returns a pointer to the constant name. */
const char *
mpd_class(const mpd_t *a, const mpd_context_t *ctx)
{
    if (mpd_isnan(a)) {
        if (mpd_isqnan(a))
            return "NaN";
        else
            return "sNaN";
    }
    else if (mpd_ispositive(a)) {
        if (mpd_isinfinite(a))
            return "+Infinity";
        else if (mpd_iszero(a))
            return "+Zero";
        else if (mpd_isnormal(a, ctx))
            return "+Normal";
        else
            return "+Subnormal";
    }
    else {
        if (mpd_isinfinite(a))
            return "-Infinity";
        else if (mpd_iszero(a))
            return "-Zero";
        else if (mpd_isnormal(a, ctx))
            return "-Normal";
        else
            return "-Subnormal";
    }
}

/* Logical Xor */
void
mpd_qinvert(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
            uint32_t *status)
{
    mpd_uint_t x, z, xbit;
    mpd_ssize_t i, digits, len;
    mpd_ssize_t q, r;
    int k;

    if (mpd_isspecial(a) || mpd_isnegative(a) || a->exp != 0) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    digits = (a->digits < ctx->prec) ? ctx->prec : a->digits;
    _mpd_idiv_word(&q, &r, digits, MPD_RDIGITS);
    len = (r == 0) ? q : q+1;
    if (!mpd_qresize(result, len, status)) {
        return;
    }

    for (i = 0; i < len; i++) {
        x = (i < a->len) ? a->data[i] : 0;
        z = 0;
        for (k = 0; k < MPD_RDIGITS; k++) {
            xbit = x % 10;
            x /= 10;
            if (xbit > 1) {
                goto invalid_operation;
            }
            z += !xbit ? mpd_pow10[k] : 0;
        }
        result->data[i] = z;
    }

    mpd_clear_flags(result);
    result->exp = 0;
    result->len = _mpd_real_size(result->data, len);
    mpd_qresize(result, result->len, status);
    mpd_setdigits(result);
    _mpd_cap(result, ctx);
    return;

invalid_operation:
    mpd_seterror(result, MPD_Invalid_operation, status);
}

/* Exponent of the magnitude of the most significant digit of the operand. */
void
mpd_qlogb(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
          uint32_t *status)
{
    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        mpd_setspecial(result, MPD_POS, MPD_INF);
    }
    else if (mpd_iszerocoeff(a)) {
        mpd_setspecial(result, MPD_NEG, MPD_INF);
        *status |= MPD_Division_by_zero;
    }
    else {
        mpd_qset_ssize(result, mpd_adjexp(a), ctx, status);
    }
}

/* Logical Or */
void
mpd_qor(mpd_t *result, const mpd_t *a, const mpd_t *b,
        const mpd_context_t *ctx, uint32_t *status)
{
    const mpd_t *big = a, *small = b;
    mpd_uint_t x, y, z, xbit, ybit;
    int k, mswdigits;
    mpd_ssize_t i;

    if (mpd_isspecial(a) || mpd_isspecial(b) ||
        mpd_isnegative(a) || mpd_isnegative(b) ||
        a->exp != 0 || b->exp != 0) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (b->digits > a->digits) {
        big = b;
        small = a;
    }
    if (!mpd_qresize(result, big->len, status)) {
        return;
    }


    /* full words */
    for (i = 0; i < small->len-1; i++) {
        x = small->data[i];
        y = big->data[i];
        z = 0;
        for (k = 0; k < MPD_RDIGITS; k++) {
            xbit = x % 10;
            x /= 10;
            ybit = y % 10;
            y /= 10;
            if (xbit > 1 || ybit > 1) {
                goto invalid_operation;
            }
            z += (xbit|ybit) ? mpd_pow10[k] : 0;
        }
        result->data[i] = z;
    }
    /* most significant word of small */
    x = small->data[i];
    y = big->data[i];
    z = 0;
    mswdigits = mpd_word_digits(x);
    for (k = 0; k < mswdigits; k++) {
        xbit = x % 10;
        x /= 10;
        ybit = y % 10;
        y /= 10;
        if (xbit > 1 || ybit > 1) {
            goto invalid_operation;
        }
        z += (xbit|ybit) ? mpd_pow10[k] : 0;
    }

    /* scan for digits > 1 and copy the rest of y */
    for (; k < MPD_RDIGITS; k++) {
        ybit = y % 10;
        y /= 10;
        if (ybit > 1) {
            goto invalid_operation;
        }
        z += ybit*mpd_pow10[k];
    }
    result->data[i++] = z;
    /* scan for digits > 1 and copy the rest of big */
    for (; i < big->len; i++) {
        y = big->data[i];
        for (k = 0; k < MPD_RDIGITS; k++) {
            ybit = y % 10;
            y /= 10;
            if (ybit > 1) {
                goto invalid_operation;
            }
        }
        result->data[i] = big->data[i];
    }

    mpd_clear_flags(result);
    result->exp = 0;
    result->len = _mpd_real_size(result->data, big->len);
    mpd_qresize(result, result->len, status);
    mpd_setdigits(result);
    _mpd_cap(result, ctx);
    return;

invalid_operation:
    mpd_seterror(result, MPD_Invalid_operation, status);
}

/*
 * Rotate the coefficient of 'a' by 'b' digits. 'b' must be an integer with
 * exponent 0.
 */
void
mpd_qrotate(mpd_t *result, const mpd_t *a, const mpd_t *b,
            const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;
    MPD_NEW_STATIC(tmp,0,0,0,0);
    MPD_NEW_STATIC(big,0,0,0,0);
    MPD_NEW_STATIC(small,0,0,0,0);
    mpd_ssize_t n, lshift, rshift;

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            return;
        }
    }
    if (b->exp != 0 || mpd_isinfinite(b)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    n = mpd_qget_ssize(b, &workstatus);
    if (workstatus&MPD_Invalid_operation) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (n > ctx->prec || n < -ctx->prec) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (mpd_isinfinite(a)) {
        mpd_qcopy(result, a, status);
        return;
    }

    if (n >= 0) {
        lshift = n;
        rshift = ctx->prec-n;
    }
    else {
        lshift = ctx->prec+n;
        rshift = -n;
    }

    if (a->digits > ctx->prec) {
        if (!mpd_qcopy(&tmp, a, status)) {
            mpd_seterror(result, MPD_Malloc_error, status);
            goto finish;
        }
        _mpd_cap(&tmp, ctx);
        a = &tmp;
    }

    if (!mpd_qshiftl(&big, a, lshift, status)) {
        mpd_seterror(result, MPD_Malloc_error, status);
        goto finish;
    }
    _mpd_cap(&big, ctx);

    if (mpd_qshiftr(&small, a, rshift, status) == MPD_UINT_MAX) {
        mpd_seterror(result, MPD_Malloc_error, status);
        goto finish;
    }
    _mpd_qadd(result, &big, &small, ctx, status);


finish:
    mpd_del(&tmp);
    mpd_del(&big);
    mpd_del(&small);
}

/*
 * b must be an integer with exponent 0 and in the range +-2*(emax + prec).
 * XXX: In my opinion +-(2*emax + prec) would be more sensible.
 * The result is a with the value of b added to its exponent.
 */
void
mpd_qscaleb(mpd_t *result, const mpd_t *a, const mpd_t *b,
            const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_uint_t n, maxjump;
#ifndef LEGACY_COMPILER
    int64_t exp;
#else
    mpd_uint_t x;
    int x_sign, n_sign;
    mpd_ssize_t exp;
#endif

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            return;
        }
    }
    if (b->exp != 0 || mpd_isinfinite(b)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    n = mpd_qabs_uint(b, &workstatus);
    /* the spec demands this */
    maxjump = 2 * (mpd_uint_t)(ctx->emax + ctx->prec);

    if (n > maxjump || workstatus&MPD_Invalid_operation) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (mpd_isinfinite(a)) {
        mpd_qcopy(result, a, status);
        return;
    }

#ifndef LEGACY_COMPILER
    exp = a->exp + (int64_t)n * mpd_arith_sign(b);
    exp = (exp > MPD_EXP_INF) ? MPD_EXP_INF : exp;
    exp = (exp < MPD_EXP_CLAMP) ? MPD_EXP_CLAMP : exp;
#else
    x = (a->exp < 0) ? -a->exp : a->exp;
    x_sign = (a->exp < 0) ? 1 : 0;
    n_sign = mpd_isnegative(b) ? 1 : 0;

    if (x_sign == n_sign) {
        x = x + n;
        if (x < n) x = MPD_UINT_MAX;
    }
    else {
        x_sign = (x >= n) ? x_sign : n_sign;
        x = (x >= n) ? x - n : n - x;
    }
    if (!x_sign && x > MPD_EXP_INF) x = MPD_EXP_INF;
    if (x_sign && x > -MPD_EXP_CLAMP) x = -MPD_EXP_CLAMP;
    exp = x_sign ? -((mpd_ssize_t)x) : (mpd_ssize_t)x;
#endif

    mpd_qcopy(result, a, status);
    result->exp = (mpd_ssize_t)exp;

    mpd_qfinalize(result, ctx, status);
}

/*
 * Shift the coefficient by n digits, positive n is a left shift. In the case
 * of a left shift, the result is decapitated to fit the context precision. If
 * you don't want that, use mpd_shiftl().
 */
void
mpd_qshiftn(mpd_t *result, const mpd_t *a, mpd_ssize_t n, const mpd_context_t *ctx,
            uint32_t *status)
{
    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        mpd_qcopy(result, a, status);
        return;
    }

    if (n >= 0 && n <= ctx->prec) {
        mpd_qshiftl(result, a, n, status);
        _mpd_cap(result, ctx);
    }
    else if (n < 0 && n >= -ctx->prec) {
        if (!mpd_qcopy(result, a, status)) {
            return;
        }
        _mpd_cap(result, ctx);
        mpd_qshiftr_inplace(result, -n);
    }
    else {
        mpd_seterror(result, MPD_Invalid_operation, status);
    }
}

/*
 * Same as mpd_shiftn(), but the shift is specified by the decimal b, which
 * must be an integer with a zero exponent. Infinities remain infinities.
 */
void
mpd_qshift(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx,
           uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_ssize_t n;

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            return;
        }
    }
    if (b->exp != 0 || mpd_isinfinite(b)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    n = mpd_qget_ssize(b, &workstatus);
    if (workstatus&MPD_Invalid_operation) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (n > ctx->prec || n < -ctx->prec) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (mpd_isinfinite(a)) {
        mpd_qcopy(result, a, status);
        return;
    }

    if (n >= 0) {
        mpd_qshiftl(result, a, n, status);
        _mpd_cap(result, ctx);
    }
    else {
        if (!mpd_qcopy(result, a, status)) {
            return;
        }
        _mpd_cap(result, ctx);
        mpd_qshiftr_inplace(result, -n);
    }
}

/* Logical Xor */
void
mpd_qxor(mpd_t *result, const mpd_t *a, const mpd_t *b,
        const mpd_context_t *ctx, uint32_t *status)
{
    const mpd_t *big = a, *small = b;
    mpd_uint_t x, y, z, xbit, ybit;
    int k, mswdigits;
    mpd_ssize_t i;

    if (mpd_isspecial(a) || mpd_isspecial(b) ||
        mpd_isnegative(a) || mpd_isnegative(b) ||
        a->exp != 0 || b->exp != 0) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (b->digits > a->digits) {
        big = b;
        small = a;
    }
    if (!mpd_qresize(result, big->len, status)) {
        return;
    }


    /* full words */
    for (i = 0; i < small->len-1; i++) {
        x = small->data[i];
        y = big->data[i];
        z = 0;
        for (k = 0; k < MPD_RDIGITS; k++) {
            xbit = x % 10;
            x /= 10;
            ybit = y % 10;
            y /= 10;
            if (xbit > 1 || ybit > 1) {
                goto invalid_operation;
            }
            z += (xbit^ybit) ? mpd_pow10[k] : 0;
        }
        result->data[i] = z;
    }
    /* most significant word of small */
    x = small->data[i];
    y = big->data[i];
    z = 0;
    mswdigits = mpd_word_digits(x);
    for (k = 0; k < mswdigits; k++) {
        xbit = x % 10;
        x /= 10;
        ybit = y % 10;
        y /= 10;
        if (xbit > 1 || ybit > 1) {
            goto invalid_operation;
        }
        z += (xbit^ybit) ? mpd_pow10[k] : 0;
    }

    /* scan for digits > 1 and copy the rest of y */
    for (; k < MPD_RDIGITS; k++) {
        ybit = y % 10;
        y /= 10;
        if (ybit > 1) {
            goto invalid_operation;
        }
        z += ybit*mpd_pow10[k];
    }
    result->data[i++] = z;
    /* scan for digits > 1 and copy the rest of big */
    for (; i < big->len; i++) {
        y = big->data[i];
        for (k = 0; k < MPD_RDIGITS; k++) {
            ybit = y % 10;
            y /= 10;
            if (ybit > 1) {
                goto invalid_operation;
            }
        }
        result->data[i] = big->data[i];
    }

    mpd_clear_flags(result);
    result->exp = 0;
    result->len = _mpd_real_size(result->data, big->len);
    mpd_qresize(result, result->len, status);
    mpd_setdigits(result);
    _mpd_cap(result, ctx);
    return;

invalid_operation:
    mpd_seterror(result, MPD_Invalid_operation, status);
}


/******************************************************************************/
/*                         Arithmetic operations                              */
/******************************************************************************/

/*
 * The absolute value of a. If a is negative, the result is the same
 * as the result of the minus operation. Otherwise, the result is the
 * result of the plus operation.
 */
void
mpd_qabs(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
         uint32_t *status)
{
    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
    }

    if (mpd_isnegative(a)) {
        mpd_qminus(result, a, ctx, status);
    }
    else {
        mpd_qplus(result, a, ctx, status);
    }
}

static inline void
_mpd_ptrswap(const mpd_t **a, const mpd_t **b)
{
    const mpd_t *t = *a;
    *a = *b;
    *b = t;
}

/* Add or subtract infinities. */
static void
_mpd_qaddsub_inf(mpd_t *result, const mpd_t *a, const mpd_t *b, uint8_t sign_b,
                 uint32_t *status)
{
    if (mpd_isinfinite(a)) {
        if (mpd_sign(a) != sign_b && mpd_isinfinite(b)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
        }
        else {
            mpd_setspecial(result, mpd_sign(a), MPD_INF);
        }
        return;
    }
    assert(mpd_isinfinite(b));
    mpd_setspecial(result, sign_b, MPD_INF);
}

/* Add or subtract non-special numbers. */
static void
_mpd_qaddsub(mpd_t *result, const mpd_t *a, const mpd_t *b, uint8_t sign_b,
             const mpd_context_t *ctx, uint32_t *status)
{
    const mpd_t *big, *small;
    MPD_NEW_STATIC(big_aligned,0,0,0,0);
    MPD_NEW_CONST(tiny,0,0,1,1,1,1);
    mpd_uint_t carry;
    mpd_ssize_t newsize, shift;
    mpd_ssize_t exp, i;
    int swap = 0;


    /* compare exponents */
    big = a; small = b;
    if (big->exp != small->exp) {
        if (small->exp > big->exp) {
            _mpd_ptrswap(&big, &small);
            swap++;
        }
        /* align the coefficients */
        if (!mpd_iszerocoeff(big)) {
            exp = big->exp - 1;
            exp += (big->digits > ctx->prec) ? 0 : big->digits-ctx->prec-1;
            if (mpd_adjexp(small) < exp) {
                /*
                 * Avoid huge shifts by substituting a value for small that is
                 * guaranteed to produce the same results.
                 *
                 * adjexp(small) < exp if and only if:
                 *
                 *   bdigits <= prec AND
                 *   bdigits+shift >= prec+2+sdigits AND
                 *   exp = bexp+bdigits-prec-2
                 *
                 *     1234567000000000  ->  bdigits + shift
                 *     ----------XX1234  ->  sdigits
                 *     ----------X1      ->  tiny-digits
                 *     |- prec -|
                 *
                 *      OR
                 *
                 *   bdigits > prec AND
                 *   shift > sdigits AND
                 *   exp = bexp-1
                 *
                 *     1234567892100000  ->  bdigits + shift
                 *     ----------XX1234  ->  sdigits
                 *     ----------X1      ->  tiny-digits
                 *     |- prec -|
                 *
                 * If tiny is zero, adding or subtracting is a no-op.
                 * Otherwise, adding tiny generates a non-zero digit either
                 * below the rounding digit or the least significant digit
                 * of big. When subtracting, tiny is in the same position as
                 * the carry that would be generated by subtracting sdigits.
                 */
                mpd_copy_flags(&tiny, small);
                tiny.exp = exp;
                tiny.digits = 1;
                tiny.len = 1;
                tiny.data[0] = mpd_iszerocoeff(small) ? 0 : 1;
                small = &tiny;
            }
            /* This cannot wrap: the difference is positive and <= maxprec */
            shift = big->exp - small->exp;
            if (!mpd_qshiftl(&big_aligned, big, shift, status)) {
                mpd_seterror(result, MPD_Malloc_error, status);
                goto finish;
            }
            big = &big_aligned;
        }
    }
    result->exp = small->exp;


    /* compare length of coefficients */
    if (big->len < small->len) {
        _mpd_ptrswap(&big, &small);
        swap++;
    }

    newsize = big->len;
    if (!mpd_qresize(result, newsize, status)) {
        goto finish;
    }

    if (mpd_sign(a) == sign_b) {

        carry = _mpd_baseadd(result->data, big->data, small->data,
                             big->len, small->len);

        if (carry) {
            newsize = big->len + 1;
            if (!mpd_qresize(result, newsize, status)) {
                goto finish;
            }
            result->data[newsize-1] = carry;
        }

        result->len = newsize;
        mpd_set_flags(result, sign_b);
    }
    else {
        if (big->len == small->len) {
            for (i=big->len-1; i >= 0; --i) {
                if (big->data[i] != small->data[i]) {
                    if (big->data[i] < small->data[i]) {
                        _mpd_ptrswap(&big, &small);
                        swap++;
                    }
                    break;
                }
            }
        }

        _mpd_basesub(result->data, big->data, small->data,
                     big->len, small->len);
        newsize = _mpd_real_size(result->data, big->len);
        /* resize to smaller cannot fail */
        (void)mpd_qresize(result, newsize, status);

        result->len = newsize;
        sign_b = (swap & 1) ? sign_b : mpd_sign(a);
        mpd_set_flags(result, sign_b);

        if (mpd_iszerocoeff(result)) {
            mpd_set_positive(result);
            if (ctx->round == MPD_ROUND_FLOOR) {
                mpd_set_negative(result);
            }
        }
    }

    mpd_setdigits(result);

finish:
    mpd_del(&big_aligned);
}

/* Add a and b. No specials, no finalizing. */
static void
_mpd_qadd(mpd_t *result, const mpd_t *a, const mpd_t *b,
          const mpd_context_t *ctx, uint32_t *status)
{
    _mpd_qaddsub(result, a, b, mpd_sign(b), ctx, status);
}

/* Subtract b from a. No specials, no finalizing. */
static void
_mpd_qsub(mpd_t *result, const mpd_t *a, const mpd_t *b,
          const mpd_context_t *ctx, uint32_t *status)
{
     _mpd_qaddsub(result, a, b, !mpd_sign(b), ctx, status);
}

/* Add a and b. */
void
mpd_qadd(mpd_t *result, const mpd_t *a, const mpd_t *b,
         const mpd_context_t *ctx, uint32_t *status)
{
    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            return;
        }
        _mpd_qaddsub_inf(result, a, b, mpd_sign(b), status);
        return;
    }

    _mpd_qaddsub(result, a, b, mpd_sign(b), ctx, status);
    mpd_qfinalize(result, ctx, status);
}

/* Add a and b. Set NaN/Invalid_operation if the result is inexact. */
static void
_mpd_qadd_exact(mpd_t *result, const mpd_t *a, const mpd_t *b,
                const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;

    mpd_qadd(result, a, b, ctx, &workstatus);
    *status |= workstatus;
    if (workstatus & (MPD_Inexact|MPD_Rounded|MPD_Clamped)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
    }
}

/* Subtract b from a. */
void
mpd_qsub(mpd_t *result, const mpd_t *a, const mpd_t *b,
         const mpd_context_t *ctx, uint32_t *status)
{
    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            return;
        }
        _mpd_qaddsub_inf(result, a, b, !mpd_sign(b), status);
        return;
    }

    _mpd_qaddsub(result, a, b, !mpd_sign(b), ctx, status);
    mpd_qfinalize(result, ctx, status);
}

/* Subtract b from a. Set NaN/Invalid_operation if the result is inexact. */
static void
_mpd_qsub_exact(mpd_t *result, const mpd_t *a, const mpd_t *b,
                const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;

    mpd_qsub(result, a, b, ctx, &workstatus);
    *status |= workstatus;
    if (workstatus & (MPD_Inexact|MPD_Rounded|MPD_Clamped)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
    }
}

/* Add decimal and mpd_ssize_t. */
void
mpd_qadd_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b,
               const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qsset_ssize(&bb, b, &maxcontext, status);
    mpd_qadd(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Add decimal and mpd_uint_t. */
void
mpd_qadd_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b,
              const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qsset_uint(&bb, b, &maxcontext, status);
    mpd_qadd(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Subtract mpd_ssize_t from decimal. */
void
mpd_qsub_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b,
               const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qsset_ssize(&bb, b, &maxcontext, status);
    mpd_qsub(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Subtract mpd_uint_t from decimal. */
void
mpd_qsub_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b,
              const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qsset_uint(&bb, b, &maxcontext, status);
    mpd_qsub(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Add decimal and int32_t. */
void
mpd_qadd_i32(mpd_t *result, const mpd_t *a, int32_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qadd_ssize(result, a, b, ctx, status);
}

/* Add decimal and uint32_t. */
void
mpd_qadd_u32(mpd_t *result, const mpd_t *a, uint32_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qadd_uint(result, a, b, ctx, status);
}

#ifdef CONFIG_64
/* Add decimal and int64_t. */
void
mpd_qadd_i64(mpd_t *result, const mpd_t *a, int64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qadd_ssize(result, a, b, ctx, status);
}

/* Add decimal and uint64_t. */
void
mpd_qadd_u64(mpd_t *result, const mpd_t *a, uint64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qadd_uint(result, a, b, ctx, status);
}
#elif !defined(LEGACY_COMPILER)
/* Add decimal and int64_t. */
void
mpd_qadd_i64(mpd_t *result, const mpd_t *a, int64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qset_i64(&bb, b, &maxcontext, status);
    mpd_qadd(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Add decimal and uint64_t. */
void
mpd_qadd_u64(mpd_t *result, const mpd_t *a, uint64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qset_u64(&bb, b, &maxcontext, status);
    mpd_qadd(result, a, &bb, ctx, status);
    mpd_del(&bb);
}
#endif

/* Subtract int32_t from decimal. */
void
mpd_qsub_i32(mpd_t *result, const mpd_t *a, int32_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qsub_ssize(result, a, b, ctx, status);
}

/* Subtract uint32_t from decimal. */
void
mpd_qsub_u32(mpd_t *result, const mpd_t *a, uint32_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qsub_uint(result, a, b, ctx, status);
}

#ifdef CONFIG_64
/* Subtract int64_t from decimal. */
void
mpd_qsub_i64(mpd_t *result, const mpd_t *a, int64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qsub_ssize(result, a, b, ctx, status);
}

/* Subtract uint64_t from decimal. */
void
mpd_qsub_u64(mpd_t *result, const mpd_t *a, uint64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qsub_uint(result, a, b, ctx, status);
}
#elif !defined(LEGACY_COMPILER)
/* Subtract int64_t from decimal. */
void
mpd_qsub_i64(mpd_t *result, const mpd_t *a, int64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qset_i64(&bb, b, &maxcontext, status);
    mpd_qsub(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Subtract uint64_t from decimal. */
void
mpd_qsub_u64(mpd_t *result, const mpd_t *a, uint64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qset_u64(&bb, b, &maxcontext, status);
    mpd_qsub(result, a, &bb, ctx, status);
    mpd_del(&bb);
}
#endif


/* Divide infinities. */
static void
_mpd_qdiv_inf(mpd_t *result, const mpd_t *a, const mpd_t *b,
              const mpd_context_t *ctx, uint32_t *status)
{
    if (mpd_isinfinite(a)) {
        if (mpd_isinfinite(b)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
            return;
        }
        mpd_setspecial(result, mpd_sign(a)^mpd_sign(b), MPD_INF);
        return;
    }
    assert(mpd_isinfinite(b));
    _settriple(result, mpd_sign(a)^mpd_sign(b), 0, mpd_etiny(ctx));
    *status |= MPD_Clamped;
}

enum {NO_IDEAL_EXP, SET_IDEAL_EXP};
/* Divide a by b. */
static void
_mpd_qdiv(int action, mpd_t *q, const mpd_t *a, const mpd_t *b,
          const mpd_context_t *ctx, uint32_t *status)
{
    MPD_NEW_STATIC(aligned,0,0,0,0);
    mpd_uint_t ld;
    mpd_ssize_t shift, exp, tz;
    mpd_ssize_t newsize;
    mpd_ssize_t ideal_exp;
    mpd_uint_t rem;
    uint8_t sign_a = mpd_sign(a);
    uint8_t sign_b = mpd_sign(b);


    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(q, a, b, ctx, status)) {
            return;
        }
        _mpd_qdiv_inf(q, a, b, ctx, status);
        return;
    }
    if (mpd_iszerocoeff(b)) {
        if (mpd_iszerocoeff(a)) {
            mpd_seterror(q, MPD_Division_undefined, status);
        }
        else {
            mpd_setspecial(q, sign_a^sign_b, MPD_INF);
            *status |= MPD_Division_by_zero;
        }
        return;
    }
    if (mpd_iszerocoeff(a)) {
        exp = a->exp - b->exp;
        _settriple(q, sign_a^sign_b, 0, exp);
        mpd_qfinalize(q, ctx, status);
        return;
    }

    shift = (b->digits - a->digits) + ctx->prec + 1;
    ideal_exp = a->exp - b->exp;
    exp = ideal_exp - shift;
    if (shift > 0) {
        if (!mpd_qshiftl(&aligned, a, shift, status)) {
            mpd_seterror(q, MPD_Malloc_error, status);
            goto finish;
        }
        a = &aligned;
    }
    else if (shift < 0) {
        shift = -shift;
        if (!mpd_qshiftl(&aligned, b, shift, status)) {
            mpd_seterror(q, MPD_Malloc_error, status);
            goto finish;
        }
        b = &aligned;
    }


    newsize = a->len - b->len + 1;
    if ((q != b && q != a) || (q == b && newsize > b->len)) {
        if (!mpd_qresize(q, newsize, status)) {
            mpd_seterror(q, MPD_Malloc_error, status);
            goto finish;
        }
    }


    if (b->len == 1) {
        rem = _mpd_shortdiv(q->data, a->data, a->len, b->data[0]);
    }
    else if (b->len <= MPD_NEWTONDIV_CUTOFF) {
        int ret = _mpd_basedivmod(q->data, NULL, a->data, b->data,
                                  a->len, b->len);
        if (ret < 0) {
            mpd_seterror(q, MPD_Malloc_error, status);
            goto finish;
        }
        rem = ret;
    }
    else {
        MPD_NEW_STATIC(r,0,0,0,0);
        _mpd_base_ndivmod(q, &r, a, b, status);
        if (mpd_isspecial(q) || mpd_isspecial(&r)) {
            mpd_setspecial(q, MPD_POS, MPD_NAN);
            mpd_del(&r);
            goto finish;
        }
        rem = !mpd_iszerocoeff(&r);
        mpd_del(&r);
        newsize = q->len;
    }

    newsize = _mpd_real_size(q->data, newsize);
    /* resize to smaller cannot fail */
    mpd_qresize(q, newsize, status);
    mpd_set_flags(q, sign_a^sign_b);
    q->len = newsize;
    mpd_setdigits(q);

    shift = ideal_exp - exp;
    if (rem) {
        ld = mpd_lsd(q->data[0]);
        if (ld == 0 || ld == 5) {
            q->data[0] += 1;
        }
    }
    else if (action == SET_IDEAL_EXP && shift > 0) {
        tz = mpd_trail_zeros(q);
        shift = (tz > shift) ? shift : tz;
        mpd_qshiftr_inplace(q, shift);
        exp += shift;
    }

    q->exp = exp;


finish:
    mpd_del(&aligned);
    mpd_qfinalize(q, ctx, status);
}

/* Divide a by b. */
void
mpd_qdiv(mpd_t *q, const mpd_t *a, const mpd_t *b,
         const mpd_context_t *ctx, uint32_t *status)
{
    MPD_NEW_STATIC(aa,0,0,0,0);
    MPD_NEW_STATIC(bb,0,0,0,0);
    uint32_t xstatus = 0;

    if (q == a) {
        if (!mpd_qcopy(&aa, a, status)) {
            mpd_seterror(q, MPD_Malloc_error, status);
            goto out;
        }
        a = &aa;
    }

    if (q == b) {
        if (!mpd_qcopy(&bb, b, status)) {
            mpd_seterror(q, MPD_Malloc_error, status);
            goto out;
        }
        b = &bb;
    }

    _mpd_qdiv(SET_IDEAL_EXP, q, a, b, ctx, &xstatus);

    if (xstatus & (MPD_Malloc_error|MPD_Division_impossible)) {
        /* Inexact quotients (the usual case) fill the entire context precision,
         * which can lead to the above errors for very high precisions. Retry
         * the operation with a lower precision in case the result is exact.
         *
         * We need an upper bound for the number of digits of a_coeff / b_coeff
         * when the result is exact.  If a_coeff' * 1 / b_coeff' is in lowest
         * terms, then maxdigits(a_coeff') + maxdigits(1 / b_coeff') is a suitable
         * bound.
         *
         * 1 / b_coeff' is exact iff b_coeff' exclusively has prime factors 2 or 5.
         * The largest amount of digits is generated if b_coeff' is a power of 2 or
         * a power of 5 and is less than or equal to log5(b_coeff') <= log2(b_coeff').
         *
         * We arrive at a total upper bound:
         *
         *   maxdigits(a_coeff') + maxdigits(1 / b_coeff') <=
         *   log10(a_coeff) + log2(b_coeff) =
         *   log10(a_coeff) + log10(b_coeff) / log10(2) <=
         *   a->digits + b->digits * 4;
         */
        mpd_context_t workctx = *ctx;
        uint32_t ystatus = 0;

        workctx.prec = a->digits + b->digits * 4;
        if (workctx.prec >= ctx->prec) {
            *status |= (xstatus&MPD_Errors);
            goto out;  /* No point in retrying, keep the original error. */
        }

        _mpd_qdiv(SET_IDEAL_EXP, q, a, b, &workctx, &ystatus);
        if (ystatus != 0) {
            ystatus = *status | ((ystatus|xstatus)&MPD_Errors);
            mpd_seterror(q, ystatus, status);
        }
    }
    else {
        *status |= xstatus;
    }


out:
    mpd_del(&aa);
    mpd_del(&bb);
}

/* Internal function. */
static void
_mpd_qdivmod(mpd_t *q, mpd_t *r, const mpd_t *a, const mpd_t *b,
             const mpd_context_t *ctx, uint32_t *status)
{
    MPD_NEW_STATIC(aligned,0,0,0,0);
    mpd_ssize_t qsize, rsize;
    mpd_ssize_t ideal_exp, expdiff, shift;
    uint8_t sign_a = mpd_sign(a);
    uint8_t sign_ab = mpd_sign(a)^mpd_sign(b);


    ideal_exp = (a->exp > b->exp) ?  b->exp : a->exp;
    if (mpd_iszerocoeff(a)) {
        if (!mpd_qcopy(r, a, status)) {
            goto nanresult; /* GCOV_NOT_REACHED */
        }
        r->exp = ideal_exp;
        _settriple(q, sign_ab, 0, 0);
        return;
    }

    expdiff = mpd_adjexp(a) - mpd_adjexp(b);
    if (expdiff < 0) {
        if (a->exp > b->exp) {
            /* positive and less than b->digits - a->digits */
            shift = a->exp - b->exp;
            if (!mpd_qshiftl(r, a, shift, status)) {
                goto nanresult;
            }
            r->exp = ideal_exp;
        }
        else {
            if (!mpd_qcopy(r, a, status)) {
                goto nanresult;
            }
        }
        _settriple(q, sign_ab, 0, 0);
        return;
    }
    if (expdiff > ctx->prec) {
        *status |= MPD_Division_impossible;
        goto nanresult;
    }


    /*
     * At this point we have:
     *   (1) 0 <= a->exp + a->digits - b->exp - b->digits <= prec
     *   (2) a->exp - b->exp >= b->digits - a->digits
     *   (3) a->exp - b->exp <= prec + b->digits - a->digits
     */
    if (a->exp != b->exp) {
        shift = a->exp - b->exp;
        if (shift > 0) {
            /* by (3), after the shift a->digits <= prec + b->digits */
            if (!mpd_qshiftl(&aligned, a, shift, status)) {
                goto nanresult;
            }
            a = &aligned;
        }
        else  {
            shift = -shift;
            /* by (2), after the shift b->digits <= a->digits */
            if (!mpd_qshiftl(&aligned, b, shift, status)) {
                goto nanresult;
            }
            b = &aligned;
        }
    }


    qsize = a->len - b->len + 1;
    if (!(q == a && qsize < a->len) && !(q == b && qsize < b->len)) {
        if (!mpd_qresize(q, qsize, status)) {
            goto nanresult;
        }
    }

    rsize = b->len;
    if (!(r == a && rsize < a->len)) {
        if (!mpd_qresize(r, rsize, status)) {
            goto nanresult;
        }
    }

    if (b->len == 1) {
        assert(b->data[0] != 0); /* annotation for scan-build */
        if (a->len == 1) {
            _mpd_div_word(&q->data[0], &r->data[0], a->data[0], b->data[0]);
        }
        else {
            r->data[0] = _mpd_shortdiv(q->data, a->data, a->len, b->data[0]);
        }
    }
    else if (b->len <= MPD_NEWTONDIV_CUTOFF) {
        int ret;
        ret = _mpd_basedivmod(q->data, r->data, a->data, b->data,
                              a->len, b->len);
        if (ret == -1) {
            *status |= MPD_Malloc_error;
            goto nanresult;
        }
    }
    else {
        _mpd_base_ndivmod(q, r, a, b, status);
        if (mpd_isspecial(q) || mpd_isspecial(r)) {
            goto nanresult;
        }
        qsize = q->len;
        rsize = r->len;
    }

    qsize = _mpd_real_size(q->data, qsize);
    /* resize to smaller cannot fail */
    mpd_qresize(q, qsize, status);
    q->len = qsize;
    mpd_setdigits(q);
    mpd_set_flags(q, sign_ab);
    q->exp = 0;
    if (q->digits > ctx->prec) {
        *status |= MPD_Division_impossible;
        goto nanresult;
    }

    rsize = _mpd_real_size(r->data, rsize);
    /* resize to smaller cannot fail */
    mpd_qresize(r, rsize, status);
    r->len = rsize;
    mpd_setdigits(r);
    mpd_set_flags(r, sign_a);
    r->exp = ideal_exp;

out:
    mpd_del(&aligned);
    return;

nanresult:
    mpd_setspecial(q, MPD_POS, MPD_NAN);
    mpd_setspecial(r, MPD_POS, MPD_NAN);
    goto out;
}

/* Integer division with remainder. */
void
mpd_qdivmod(mpd_t *q, mpd_t *r, const mpd_t *a, const mpd_t *b,
            const mpd_context_t *ctx, uint32_t *status)
{
    uint8_t sign = mpd_sign(a)^mpd_sign(b);

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(q, a, b, ctx, status)) {
            mpd_qcopy(r, q, status);
            return;
        }
        if (mpd_isinfinite(a)) {
            if (mpd_isinfinite(b)) {
                mpd_setspecial(q, MPD_POS, MPD_NAN);
            }
            else {
                mpd_setspecial(q, sign, MPD_INF);
            }
            mpd_setspecial(r, MPD_POS, MPD_NAN);
            *status |= MPD_Invalid_operation;
            return;
        }
        if (mpd_isinfinite(b)) {
            if (!mpd_qcopy(r, a, status)) {
                mpd_seterror(q, MPD_Malloc_error, status);
                return;
            }
            mpd_qfinalize(r, ctx, status);
            _settriple(q, sign, 0, 0);
            return;
        }
        /* debug */
        abort(); /* GCOV_NOT_REACHED */
    }
    if (mpd_iszerocoeff(b)) {
        if (mpd_iszerocoeff(a)) {
            mpd_setspecial(q, MPD_POS, MPD_NAN);
            mpd_setspecial(r, MPD_POS, MPD_NAN);
            *status |= MPD_Division_undefined;
        }
        else {
            mpd_setspecial(q, sign, MPD_INF);
            mpd_setspecial(r, MPD_POS, MPD_NAN);
            *status |= (MPD_Division_by_zero|MPD_Invalid_operation);
        }
        return;
    }

    _mpd_qdivmod(q, r, a, b, ctx, status);
    mpd_qfinalize(q, ctx, status);
    mpd_qfinalize(r, ctx, status);
}

void
mpd_qdivint(mpd_t *q, const mpd_t *a, const mpd_t *b,
            const mpd_context_t *ctx, uint32_t *status)
{
    MPD_NEW_STATIC(r,0,0,0,0);
    uint8_t sign = mpd_sign(a)^mpd_sign(b);

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(q, a, b, ctx, status)) {
            return;
        }
        if (mpd_isinfinite(a) && mpd_isinfinite(b)) {
            mpd_seterror(q, MPD_Invalid_operation, status);
            return;
        }
        if (mpd_isinfinite(a)) {
            mpd_setspecial(q, sign, MPD_INF);
            return;
        }
        if (mpd_isinfinite(b)) {
            _settriple(q, sign, 0, 0);
            return;
        }
        /* debug */
        abort(); /* GCOV_NOT_REACHED */
    }
    if (mpd_iszerocoeff(b)) {
        if (mpd_iszerocoeff(a)) {
            mpd_seterror(q, MPD_Division_undefined, status);
        }
        else {
            mpd_setspecial(q, sign, MPD_INF);
            *status |= MPD_Division_by_zero;
        }
        return;
    }


    _mpd_qdivmod(q, &r, a, b, ctx, status);
    mpd_del(&r);
    mpd_qfinalize(q, ctx, status);
}

/* Divide decimal by mpd_ssize_t. */
void
mpd_qdiv_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b,
               const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qsset_ssize(&bb, b, &maxcontext, status);
    mpd_qdiv(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Divide decimal by mpd_uint_t. */
void
mpd_qdiv_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b,
              const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qsset_uint(&bb, b, &maxcontext, status);
    mpd_qdiv(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Divide decimal by int32_t. */
void
mpd_qdiv_i32(mpd_t *result, const mpd_t *a, int32_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qdiv_ssize(result, a, b, ctx, status);
}

/* Divide decimal by uint32_t. */
void
mpd_qdiv_u32(mpd_t *result, const mpd_t *a, uint32_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qdiv_uint(result, a, b, ctx, status);
}

#ifdef CONFIG_64
/* Divide decimal by int64_t. */
void
mpd_qdiv_i64(mpd_t *result, const mpd_t *a, int64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qdiv_ssize(result, a, b, ctx, status);
}

/* Divide decimal by uint64_t. */
void
mpd_qdiv_u64(mpd_t *result, const mpd_t *a, uint64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qdiv_uint(result, a, b, ctx, status);
}
#elif !defined(LEGACY_COMPILER)
/* Divide decimal by int64_t. */
void
mpd_qdiv_i64(mpd_t *result, const mpd_t *a, int64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qset_i64(&bb, b, &maxcontext, status);
    mpd_qdiv(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Divide decimal by uint64_t. */
void
mpd_qdiv_u64(mpd_t *result, const mpd_t *a, uint64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qset_u64(&bb, b, &maxcontext, status);
    mpd_qdiv(result, a, &bb, ctx, status);
    mpd_del(&bb);
}
#endif

/* Pad the result with trailing zeros if it has fewer digits than prec. */
static void
_mpd_zeropad(mpd_t *result, const mpd_context_t *ctx, uint32_t *status)
{
    if (!mpd_isspecial(result) && !mpd_iszero(result) &&
        result->digits < ctx->prec) {
       mpd_ssize_t shift = ctx->prec - result->digits;
       mpd_qshiftl(result, result, shift, status);
       result->exp -= shift;
    }
}

/* Check if the result is guaranteed to be one. */
static int
_mpd_qexp_check_one(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
                    uint32_t *status)
{
    MPD_NEW_CONST(lim,0,-(ctx->prec+1),1,1,1,9);
    MPD_NEW_SHARED(aa, a);

    mpd_set_positive(&aa);

    /* abs(a) <= 9 * 10**(-prec-1) */
    if (_mpd_cmp(&aa, &lim) <= 0) {
        _settriple(result, 0, 1, 0);
        *status |= MPD_Rounded|MPD_Inexact;
        return 1;
    }

    return 0;
}

/*
 * Get the number of iterations for the Horner scheme in _mpd_qexp().
 */
static inline mpd_ssize_t
_mpd_get_exp_iterations(const mpd_t *r, mpd_ssize_t p)
{
    mpd_ssize_t log10pbyr; /* lower bound for log10(p / abs(r)) */
    mpd_ssize_t n;

    assert(p >= 10);
    assert(!mpd_iszero(r));
    assert(-p < mpd_adjexp(r) && mpd_adjexp(r) <= -1);

#ifdef CONFIG_64
    if (p > (mpd_ssize_t)(1ULL<<52)) {
        return MPD_SSIZE_MAX;
    }
#endif

    /*
     * Lower bound for log10(p / abs(r)): adjexp(p) - (adjexp(r) + 1)
     * At this point (for CONFIG_64, CONFIG_32 is not problematic):
     *    1) 10 <= p <= 2**52
     *    2) -p < adjexp(r) <= -1
     *    3) 1 <= log10pbyr <= 2**52 + 14
     */
    log10pbyr = (mpd_word_digits(p)-1) - (mpd_adjexp(r)+1);

    /*
     * The numerator in the paper is 1.435 * p - 1.182, calculated
     * exactly. We compensate for rounding errors by using 1.43503.
     * ACL2 proofs:
     *    1) exp-iter-approx-lower-bound: The term below evaluated
     *       in 53-bit floating point arithmetic is greater than or
     *       equal to the exact term used in the paper.
     *    2) exp-iter-approx-upper-bound: The term below is less than
     *       or equal to 3/2 * p <= 3/2 * 2**52.
     */
    n = (mpd_ssize_t)ceil((1.43503*(double)p - 1.182) / (double)log10pbyr);
    return n >= 3 ? n : 3;
}

/*
 * Internal function, specials have been dealt with. Apart from Overflow
 * and Underflow, two cases must be considered for the error of the result:
 *
 *   1) abs(a) <= 9 * 10**(-prec-1)  ==>  result == 1
 *
 *      Absolute error: abs(1 - e**x) < 10**(-prec)
 *      -------------------------------------------
 *
 *   2) abs(a) > 9 * 10**(-prec-1)
 *
 *      Relative error: abs(result - e**x) < 0.5 * 10**(-prec) * e**x
 *      -------------------------------------------------------------
 *
 * The algorithm is from Hull&Abrham, Variable Precision Exponential Function,
 * ACM Transactions on Mathematical Software, Vol. 12, No. 2, June 1986.
 *
 * Main differences:
 *
 *  - The number of iterations for the Horner scheme is calculated using
 *    53-bit floating point arithmetic.
 *
 *  - In the error analysis for ER (relative error accumulated in the
 *    evaluation of the truncated series) the reduced operand r may
 *    have any number of digits.
 *    ACL2 proof: exponent-relative-error
 *
 *  - The analysis for early abortion has been adapted for the mpd_t
 *    ranges.
 */
static void
_mpd_qexp(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
          uint32_t *status)
{
    mpd_context_t workctx;
    MPD_NEW_STATIC(tmp,0,0,0,0);
    MPD_NEW_STATIC(sum,0,0,0,0);
    MPD_NEW_CONST(word,0,0,1,1,1,1);
    mpd_ssize_t j, n, t;

    assert(!mpd_isspecial(a));

    if (mpd_iszerocoeff(a)) {
        _settriple(result, MPD_POS, 1, 0);
        return;
    }

    /*
     * We are calculating e^x = e^(r*10^t) = (e^r)^(10^t), where abs(r) < 1 and t >= 0.
     *
     * If t > 0, we have:
     *
     *   (1) 0.1 <= r < 1, so e^0.1 <= e^r. If t > MAX_T, overflow occurs:
     *
     *     MAX-EMAX+1 < log10(e^(0.1*10*t)) <= log10(e^(r*10^t)) < adjexp(e^(r*10^t))+1
     *
     *   (2) -1 < r <= -0.1, so e^r <= e^-0.1. If t > MAX_T, underflow occurs:
     *
     *     adjexp(e^(r*10^t)) <= log10(e^(r*10^t)) <= log10(e^(-0.1*10^t)) < MIN-ETINY
     */
#if defined(CONFIG_64)
    #define MPD_EXP_MAX_T 19
#elif defined(CONFIG_32)
    #define MPD_EXP_MAX_T 10
#endif
    t = a->digits + a->exp;
    t = (t > 0) ? t : 0;
    if (t > MPD_EXP_MAX_T) {
        if (mpd_ispositive(a)) {
            mpd_setspecial(result, MPD_POS, MPD_INF);
            *status |= MPD_Overflow|MPD_Inexact|MPD_Rounded;
        }
        else {
            _settriple(result, MPD_POS, 0, mpd_etiny(ctx));
            *status |= (MPD_Inexact|MPD_Rounded|MPD_Subnormal|
                        MPD_Underflow|MPD_Clamped);
        }
        return;
    }

    /* abs(a) <= 9 * 10**(-prec-1) */
    if (_mpd_qexp_check_one(result, a, ctx, status)) {
        return;
    }

    mpd_maxcontext(&workctx);
    workctx.prec = ctx->prec + t + 2;
    workctx.prec = (workctx.prec < 10) ? 10 : workctx.prec;
    workctx.round = MPD_ROUND_HALF_EVEN;

    if (!mpd_qcopy(result, a, status)) {
        return;
    }
    result->exp -= t;

    /*
     * At this point:
     *    1) 9 * 10**(-prec-1) < abs(a)
     *    2) 9 * 10**(-prec-t-1) < abs(r)
     *    3) log10(9) - prec - t - 1 < log10(abs(r)) < adjexp(abs(r)) + 1
     *    4) - prec - t - 2 < adjexp(abs(r)) <= -1
     */
    n = _mpd_get_exp_iterations(result, workctx.prec);
    if (n == MPD_SSIZE_MAX) {
        mpd_seterror(result, MPD_Invalid_operation, status); /* GCOV_UNLIKELY */
        return; /* GCOV_UNLIKELY */
    }

    _settriple(&sum, MPD_POS, 1, 0);

    for (j = n-1; j >= 1; j--) {
        word.data[0] = j;
        mpd_setdigits(&word);
        mpd_qdiv(&tmp, result, &word, &workctx, &workctx.status);
        mpd_qfma(&sum, &sum, &tmp, &one, &workctx, &workctx.status);
    }

#ifdef CONFIG_64
    _mpd_qpow_uint(result, &sum, mpd_pow10[t], MPD_POS, &workctx, status);
#else
    if (t <= MPD_MAX_POW10) {
        _mpd_qpow_uint(result, &sum, mpd_pow10[t], MPD_POS, &workctx, status);
    }
    else {
        t -= MPD_MAX_POW10;
        _mpd_qpow_uint(&tmp, &sum, mpd_pow10[MPD_MAX_POW10], MPD_POS,
                       &workctx, status);
        _mpd_qpow_uint(result, &tmp, mpd_pow10[t], MPD_POS, &workctx, status);
    }
#endif

    mpd_del(&tmp);
    mpd_del(&sum);
    *status |= (workctx.status&MPD_Errors);
    *status |= (MPD_Inexact|MPD_Rounded);
}

/* exp(a) */
void
mpd_qexp(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
         uint32_t *status)
{
    mpd_context_t workctx;

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        if (mpd_isnegative(a)) {
            _settriple(result, MPD_POS, 0, 0);
        }
        else {
            mpd_setspecial(result, MPD_POS, MPD_INF);
        }
        return;
    }
    if (mpd_iszerocoeff(a)) {
        _settriple(result, MPD_POS, 1, 0);
        return;
    }

    workctx = *ctx;
    workctx.round = MPD_ROUND_HALF_EVEN;

    if (ctx->allcr) {
        MPD_NEW_STATIC(t1, 0,0,0,0);
        MPD_NEW_STATIC(t2, 0,0,0,0);
        MPD_NEW_STATIC(ulp, 0,0,0,0);
        MPD_NEW_STATIC(aa, 0,0,0,0);
        mpd_ssize_t prec;
        mpd_ssize_t ulpexp;
        uint32_t workstatus;

        if (result == a) {
            if (!mpd_qcopy(&aa, a, status)) {
                mpd_seterror(result, MPD_Malloc_error, status);
                return;
            }
            a = &aa;
        }

        workctx.clamp = 0;
        prec = ctx->prec + 3;
        while (1) {
            workctx.prec = prec;
            workstatus = 0;

            _mpd_qexp(result, a, &workctx, &workstatus);
            *status |= workstatus;

            ulpexp = result->exp + result->digits - workctx.prec;
            if (workstatus & MPD_Underflow) {
                /* The effective work precision is result->digits. */
                ulpexp = result->exp;
            }
            _ssettriple(&ulp, MPD_POS, 1, ulpexp);

            /*
             * At this point [1]:
             *   1) abs(result - e**x) < 0.5 * 10**(-prec) * e**x
             *   2) result - ulp < e**x < result + ulp
             *   3) result - ulp < result < result + ulp
             *
             * If round(result-ulp)==round(result+ulp), then
             * round(result)==round(e**x). Therefore the result
             * is correctly rounded.
             *
             * [1] If abs(a) <= 9 * 10**(-prec-1), use the absolute
             *     error for a similar argument.
             */
            workctx.prec = ctx->prec;
            mpd_qadd(&t1, result, &ulp, &workctx, &workctx.status);
            mpd_qsub(&t2, result, &ulp, &workctx, &workctx.status);
            if (mpd_isspecial(result) || mpd_iszerocoeff(result) ||
                mpd_qcmp(&t1, &t2, status) == 0) {
                workctx.clamp = ctx->clamp;
                _mpd_zeropad(result, &workctx, status);
                mpd_check_underflow(result, &workctx, status);
                mpd_qfinalize(result, &workctx, status);
                break;
            }
            prec += MPD_RDIGITS;
        }
        mpd_del(&t1);
        mpd_del(&t2);
        mpd_del(&ulp);
        mpd_del(&aa);
    }
    else {
        _mpd_qexp(result, a, &workctx, status);
        _mpd_zeropad(result, &workctx, status);
        mpd_check_underflow(result, &workctx, status);
        mpd_qfinalize(result, &workctx, status);
    }
}

/* Fused multiply-add: (a * b) + c, with a single final rounding. */
void
mpd_qfma(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_t *c,
         const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_t *cc = NULL;

    if (result == c) {
        if ((cc = mpd_qncopy(c)) == NULL) {
            mpd_seterror(result, MPD_Malloc_error, status);
            return;
        }
        c = cc;
    }

    _mpd_qmul(result, a, b, ctx, &workstatus);
    if (!(workstatus&MPD_Invalid_operation)) {
        mpd_qadd(result, result, c, ctx, &workstatus);
    }

    if (cc) mpd_del(cc);
    *status |= workstatus;
}

/*
 * Schedule the optimal precision increase for the Newton iteration.
 *   v := input operand
 *   z_0 := initial approximation
 *   initprec := natural number such that abs(log(v) - z_0) < 10**-initprec
 *   maxprec := target precision
 *
 * For convenience the output klist contains the elements in reverse order:
 *   klist := [k_n-1, ..., k_0], where
 *     1) k_0 <= initprec and
 *     2) abs(log(v) - result) < 10**(-2*k_n-1 + 1) <= 10**-maxprec.
 */
static inline int
ln_schedule_prec(mpd_ssize_t klist[MPD_MAX_PREC_LOG2], mpd_ssize_t maxprec,
                 mpd_ssize_t initprec)
{
    mpd_ssize_t k;
    int i;

    assert(maxprec >= 2 && initprec >= 2);
    if (maxprec <= initprec) return -1;

    i = 0; k = maxprec;
    do {
        k = (k+2) / 2;
        klist[i++] = k;
    } while (k > initprec);

    return i-1;
}

/* The constants have been verified with both decimal.py and mpfr. */
#ifdef CONFIG_64
#if MPD_RDIGITS != 19
  #error "mpdecimal.c: MPD_RDIGITS must be 19."
#endif
static const mpd_uint_t mpd_ln10_data[MPD_MINALLOC_MAX] = {
  6983716328982174407ULL, 9089704281976336583ULL, 1515961135648465461ULL,
  4416816335727555703ULL, 2900988039194170265ULL, 2307925037472986509ULL,
   107598438319191292ULL, 3466624107184669231ULL, 4450099781311469159ULL,
  9807828059751193854ULL, 7713456862091670584ULL, 1492198849978748873ULL,
  6528728696511086257ULL, 2385392051446341972ULL, 8692180205189339507ULL,
  6518769751037497088ULL, 2375253577097505395ULL, 9095610299291824318ULL,
   982748238504564801ULL, 5438635917781170543ULL, 7547331541421808427ULL,
   752371033310119785ULL, 3171643095059950878ULL, 9785265383207606726ULL,
  2932258279850258550ULL, 5497347726624257094ULL, 2976979522110718264ULL,
  9221477656763693866ULL, 1979650047149510504ULL, 6674183485704422507ULL,
  9702766860595249671ULL, 9278096762712757753ULL, 9314848524948644871ULL,
  6826928280848118428ULL,  754403708474699401ULL,  230105703089634572ULL,
  1929203337658714166ULL, 7589402567763113569ULL, 4208241314695689016ULL,
  2922455440575892572ULL, 9356734206705811364ULL, 2684916746550586856ULL,
   644507064800027750ULL, 9476834636167921018ULL, 5659121373450747856ULL,
  2835522011480466371ULL, 6470806855677432162ULL, 7141748003688084012ULL,
  9619404400222105101ULL, 5504893431493939147ULL, 6674744042432743651ULL,
  2287698219886746543ULL, 7773262884616336622ULL, 1985283935053089653ULL,
  4680843799894826233ULL, 8168948290720832555ULL, 8067566662873690987ULL,
  6248633409525465082ULL, 9829834196778404228ULL, 3524802359972050895ULL,
  3327900967572609677ULL,  110148862877297603ULL,  179914546843642076ULL,
  2302585092994045684ULL
};
#else
#if MPD_RDIGITS != 9
  #error "mpdecimal.c: MPD_RDIGITS must be 9."
#endif
static const mpd_uint_t mpd_ln10_data[MPD_MINALLOC_MAX] = {
  401682692UL, 708474699UL, 720754403UL,  30896345UL, 602301057UL, 765871416UL,
  192920333UL, 763113569UL, 589402567UL, 956890167UL,  82413146UL, 589257242UL,
  245544057UL, 811364292UL, 734206705UL, 868569356UL, 167465505UL, 775026849UL,
  706480002UL,  18064450UL, 636167921UL, 569476834UL, 734507478UL, 156591213UL,
  148046637UL, 283552201UL, 677432162UL, 470806855UL, 880840126UL, 417480036UL,
  210510171UL, 940440022UL, 939147961UL, 893431493UL, 436515504UL, 440424327UL,
  654366747UL, 821988674UL, 622228769UL, 884616336UL, 537773262UL, 350530896UL,
  319852839UL, 989482623UL, 468084379UL, 720832555UL, 168948290UL, 736909878UL,
  675666628UL, 546508280UL, 863340952UL, 404228624UL, 834196778UL, 508959829UL,
   23599720UL, 967735248UL,  96757260UL, 603332790UL, 862877297UL, 760110148UL,
  468436420UL, 401799145UL, 299404568UL, 230258509UL
};
#endif
/* _mpd_ln10 is used directly for precisions smaller than MINALLOC_MAX*RDIGITS.
   Otherwise, it serves as the initial approximation for calculating ln(10). */
static const mpd_t _mpd_ln10 = {
  MPD_STATIC|MPD_CONST_DATA, -(MPD_MINALLOC_MAX*MPD_RDIGITS-1),
  MPD_MINALLOC_MAX*MPD_RDIGITS, MPD_MINALLOC_MAX, MPD_MINALLOC_MAX,
  (mpd_uint_t *)mpd_ln10_data
};

/*
 * Set 'result' to log(10).
 *   Ulp error: abs(result - log(10)) < ulp(log(10))
 *   Relative error: abs(result - log(10)) < 5 * 10**-prec * log(10)
 *
 * NOTE: The relative error is not derived from the ulp error, but
 * calculated separately using the fact that 23/10 < log(10) < 24/10.
 */
void
mpd_qln10(mpd_t *result, mpd_ssize_t prec, uint32_t *status)
{
    mpd_context_t varcontext, maxcontext;
    MPD_NEW_STATIC(tmp, 0,0,0,0);
    MPD_NEW_CONST(static10, 0,0,2,1,1,10);
    mpd_ssize_t klist[MPD_MAX_PREC_LOG2];
    mpd_uint_t rnd;
    mpd_ssize_t shift;
    int i;

    assert(prec >= 1);

    shift = MPD_MINALLOC_MAX*MPD_RDIGITS-prec;
    shift = shift < 0 ? 0 : shift;

    rnd = mpd_qshiftr(result, &_mpd_ln10, shift, status);
    if (rnd == MPD_UINT_MAX) {
        mpd_seterror(result, MPD_Malloc_error, status);
        return;
    }
    result->exp = -(result->digits-1);

    mpd_maxcontext(&maxcontext);
    if (prec < MPD_MINALLOC_MAX*MPD_RDIGITS) {
        maxcontext.prec = prec;
        _mpd_apply_round_excess(result, rnd, &maxcontext, status);
        *status |= (MPD_Inexact|MPD_Rounded);
        return;
    }

    mpd_maxcontext(&varcontext);
    varcontext.round = MPD_ROUND_TRUNC;

    i = ln_schedule_prec(klist, prec+2, -result->exp);
    for (; i >= 0; i--) {
        varcontext.prec = 2*klist[i]+3;
        result->flags ^= MPD_NEG;
        _mpd_qexp(&tmp, result, &varcontext, status);
        result->flags ^= MPD_NEG;
        mpd_qmul(&tmp, &static10, &tmp, &varcontext, status);
        mpd_qsub(&tmp, &tmp, &one, &maxcontext, status);
        mpd_qadd(result, result, &tmp, &maxcontext, status);
        if (mpd_isspecial(result)) {
            break;
        }
    }

    mpd_del(&tmp);
    maxcontext.prec = prec;
    mpd_qfinalize(result, &maxcontext, status);
}

/*
 * Initial approximations for the ln() iteration. The values have the
 * following properties (established with both decimal.py and mpfr):
 *
 * Index 0 - 400, logarithms of x in [1.00, 5.00]:
 *   abs(lnapprox[i] * 10**-3 - log((i+100)/100)) < 10**-2
 *   abs(lnapprox[i] * 10**-3 - log((i+1+100)/100)) < 10**-2
 *
 * Index 401 - 899, logarithms of x in (0.500, 0.999]:
 *   abs(-lnapprox[i] * 10**-3 - log((i+100)/1000)) < 10**-2
 *   abs(-lnapprox[i] * 10**-3 - log((i+1+100)/1000)) < 10**-2
 */
static const uint16_t lnapprox[900] = {
  /* index 0 - 400: log((i+100)/100) * 1000 */
  0, 10, 20, 30, 39, 49, 58, 68, 77, 86, 95, 104, 113, 122, 131, 140, 148, 157,
  166, 174, 182, 191, 199, 207, 215, 223, 231, 239, 247, 255, 262, 270, 278,
  285, 293, 300, 308, 315, 322, 329, 336, 344, 351, 358, 365, 372, 378, 385,
  392, 399, 406, 412, 419, 425, 432, 438, 445, 451, 457, 464, 470, 476, 482,
  489, 495, 501, 507, 513, 519, 525, 531, 536, 542, 548, 554, 560, 565, 571,
  577, 582, 588, 593, 599, 604, 610, 615, 621, 626, 631, 637, 642, 647, 652,
  658, 663, 668, 673, 678, 683, 688, 693, 698, 703, 708, 713, 718, 723, 728,
  732, 737, 742, 747, 751, 756, 761, 766, 770, 775, 779, 784, 788, 793, 798,
  802, 806, 811, 815, 820, 824, 829, 833, 837, 842, 846, 850, 854, 859, 863,
  867, 871, 876, 880, 884, 888, 892, 896, 900, 904, 908, 912, 916, 920, 924,
  928, 932, 936, 940, 944, 948, 952, 956, 959, 963, 967, 971, 975, 978, 982,
  986, 990, 993, 997, 1001, 1004, 1008, 1012, 1015, 1019, 1022, 1026, 1030,
  1033, 1037, 1040, 1044, 1047, 1051, 1054, 1058, 1061, 1065, 1068, 1072, 1075,
  1078, 1082, 1085, 1089, 1092, 1095, 1099, 1102, 1105, 1109, 1112, 1115, 1118,
  1122, 1125, 1128, 1131, 1135, 1138, 1141, 1144, 1147, 1151, 1154, 1157, 1160,
  1163, 1166, 1169, 1172, 1176, 1179, 1182, 1185, 1188, 1191, 1194, 1197, 1200,
  1203, 1206, 1209, 1212, 1215, 1218, 1221, 1224, 1227, 1230, 1233, 1235, 1238,
  1241, 1244, 1247, 1250, 1253, 1256, 1258, 1261, 1264, 1267, 1270, 1273, 1275,
  1278, 1281, 1284, 1286, 1289, 1292, 1295, 1297, 1300, 1303, 1306, 1308, 1311,
  1314, 1316, 1319, 1322, 1324, 1327, 1330, 1332, 1335, 1338, 1340, 1343, 1345,
  1348, 1351, 1353, 1356, 1358, 1361, 1364, 1366, 1369, 1371, 1374, 1376, 1379,
  1381, 1384, 1386, 1389, 1391, 1394, 1396, 1399, 1401, 1404, 1406, 1409, 1411,
  1413, 1416, 1418, 1421, 1423, 1426, 1428, 1430, 1433, 1435, 1437, 1440, 1442,
  1445, 1447, 1449, 1452, 1454, 1456, 1459, 1461, 1463, 1466, 1468, 1470, 1472,
  1475, 1477, 1479, 1482, 1484, 1486, 1488, 1491, 1493, 1495, 1497, 1500, 1502,
  1504, 1506, 1509, 1511, 1513, 1515, 1517, 1520, 1522, 1524, 1526, 1528, 1530,
  1533, 1535, 1537, 1539, 1541, 1543, 1545, 1548, 1550, 1552, 1554, 1556, 1558,
  1560, 1562, 1564, 1567, 1569, 1571, 1573, 1575, 1577, 1579, 1581, 1583, 1585,
  1587, 1589, 1591, 1593, 1595, 1597, 1599, 1601, 1603, 1605, 1607, 1609,
  /* index 401 - 899: -log((i+100)/1000) * 1000 */
  691, 689, 687, 685, 683, 681, 679, 677, 675, 673, 671, 669, 668, 666, 664,
  662, 660, 658, 656, 654, 652, 650, 648, 646, 644, 642, 641, 639, 637, 635,
  633, 631, 629, 627, 626, 624, 622, 620, 618, 616, 614, 612, 611, 609, 607,
  605, 603, 602, 600, 598, 596, 594, 592, 591, 589, 587, 585, 583, 582, 580,
  578, 576, 574, 573, 571, 569, 567, 566, 564, 562, 560, 559, 557, 555, 553,
  552, 550, 548, 546, 545, 543, 541, 540, 538, 536, 534, 533, 531, 529, 528,
  526, 524, 523, 521, 519, 518, 516, 514, 512, 511, 509, 508, 506, 504, 502,
  501, 499, 498, 496, 494, 493, 491, 489, 488, 486, 484, 483, 481, 480, 478,
  476, 475, 473, 472, 470, 468, 467, 465, 464, 462, 460, 459, 457, 456, 454,
  453, 451, 449, 448, 446, 445, 443, 442, 440, 438, 437, 435, 434, 432, 431,
  429, 428, 426, 425, 423, 422, 420, 419, 417, 416, 414, 412, 411, 410, 408,
  406, 405, 404, 402, 400, 399, 398, 396, 394, 393, 392, 390, 389, 387, 386,
  384, 383, 381, 380, 378, 377, 375, 374, 372, 371, 370, 368, 367, 365, 364,
  362, 361, 360, 358, 357, 355, 354, 352, 351, 350, 348, 347, 345, 344, 342,
  341, 340, 338, 337, 336, 334, 333, 331, 330, 328, 327, 326, 324, 323, 322,
  320, 319, 318, 316, 315, 313, 312, 311, 309, 308, 306, 305, 304, 302, 301,
  300, 298, 297, 296, 294, 293, 292, 290, 289, 288, 286, 285, 284, 282, 281,
  280, 278, 277, 276, 274, 273, 272, 270, 269, 268, 267, 265, 264, 263, 261,
  260, 259, 258, 256, 255, 254, 252, 251, 250, 248, 247, 246, 245, 243, 242,
  241, 240, 238, 237, 236, 234, 233, 232, 231, 229, 228, 227, 226, 224, 223,
  222, 221, 219, 218, 217, 216, 214, 213, 212, 211, 210, 208, 207, 206, 205,
  203, 202, 201, 200, 198, 197, 196, 195, 194, 192, 191, 190, 189, 188, 186,
  185, 184, 183, 182, 180, 179, 178, 177, 176, 174, 173, 172, 171, 170, 168,
  167, 166, 165, 164, 162, 161, 160, 159, 158, 157, 156, 154, 153, 152, 151,
  150, 148, 147, 146, 145, 144, 143, 142, 140, 139, 138, 137, 136, 135, 134,
  132, 131, 130, 129, 128, 127, 126, 124, 123, 122, 121, 120, 119, 118, 116,
  115, 114, 113, 112, 111, 110, 109, 108, 106, 105, 104, 103, 102, 101, 100,
  99, 98, 97, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 84, 83, 82, 81, 80, 79,
  78, 77, 76, 75, 74, 73, 72, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59,
  58, 57, 56, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39,
  38, 37, 36, 35, 34, 33, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19,
  18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
};

/*
 * Internal ln() function that does not check for specials, zero or one.
 * Relative error: abs(result - log(a)) < 0.1 * 10**-prec * abs(log(a))
 */
static void
_mpd_qln(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
         uint32_t *status)
{
    mpd_context_t varcontext, maxcontext;
    mpd_t *z = (mpd_t *) result;
    MPD_NEW_STATIC(v,0,0,0,0);
    MPD_NEW_STATIC(vtmp,0,0,0,0);
    MPD_NEW_STATIC(tmp,0,0,0,0);
    mpd_ssize_t klist[MPD_MAX_PREC_LOG2];
    mpd_ssize_t maxprec, shift, t;
    mpd_ssize_t a_digits, a_exp;
    mpd_uint_t dummy, x;
    int i;

    assert(!mpd_isspecial(a) && !mpd_iszerocoeff(a));

    /*
     * We are calculating ln(a) = ln(v * 10^t) = ln(v) + t*ln(10),
     * where 0.5 < v <= 5.
     */
    if (!mpd_qcopy(&v, a, status)) {
        mpd_seterror(result, MPD_Malloc_error, status);
        goto finish;
    }

    /* Initial approximation: we have at least one non-zero digit */
    _mpd_get_msdigits(&dummy, &x, &v, 3);
    if (x < 10) x *= 10;
    if (x < 100) x *= 10;
    x -= 100;

    /* a may equal z */
    a_digits = a->digits;
    a_exp = a->exp;

    mpd_minalloc(z);
    mpd_clear_flags(z);
    z->data[0] = lnapprox[x];
    z->len = 1;
    z->exp = -3;
    mpd_setdigits(z);

    if (x <= 400) {
        /* Reduce the input operand to 1.00 <= v <= 5.00. Let y = x + 100,
         * so 100 <= y <= 500. Since y contains the most significant digits
         * of v, y/100 <= v < (y+1)/100 and abs(z - log(v)) < 10**-2. */
        v.exp = -(a_digits - 1);
        t = a_exp + a_digits - 1;
    }
    else {
        /* Reduce the input operand to 0.500 < v <= 0.999. Let y = x + 100,
         * so 500 < y <= 999. Since y contains the most significant digits
         * of v, y/1000 <= v < (y+1)/1000 and abs(z - log(v)) < 10**-2. */
        v.exp = -a_digits;
        t = a_exp + a_digits;
        mpd_set_negative(z);
    }

    mpd_maxcontext(&maxcontext);
    mpd_maxcontext(&varcontext);
    varcontext.round = MPD_ROUND_TRUNC;

    maxprec = ctx->prec + 2;
    if (t == 0 && (x <= 15 || x >= 800)) {
        /* 0.900 <= v <= 1.15: Estimate the magnitude of the logarithm.
         * If ln(v) will underflow, skip the loop. Otherwise, adjust the
         * precision upwards in order to obtain a sufficient number of
         * significant digits.
         *
         *   Case v > 1:
         *      abs((v-1)/10) < abs((v-1)/v) < abs(ln(v)) < abs(v-1)
         *   Case v < 1:
         *      abs(v-1) < abs(ln(v)) < abs((v-1)/v) < abs((v-1)*10)
         */
        int cmp = _mpd_cmp(&v, &one);

        /* Upper bound (assume v > 1): abs(v-1), unrounded */
        _mpd_qsub(&tmp, &v, &one, &maxcontext, &maxcontext.status);
        if (maxcontext.status & MPD_Errors) {
            mpd_seterror(result, MPD_Malloc_error, status);
            goto finish;
        }

        if (cmp < 0) {
            /* v < 1: abs((v-1)*10) */
            tmp.exp += 1;
        }
        if (mpd_adjexp(&tmp) < mpd_etiny(ctx)) {
            /* The upper bound is less than etiny: Underflow to zero */
            _settriple(result, (cmp<0), 1, mpd_etiny(ctx)-1);
            goto finish;
        }
        /* Lower bound: abs((v-1)/10) or abs(v-1) */
        tmp.exp -= 1;
        if (mpd_adjexp(&tmp) < 0) {
            /* Absolute error of the loop: abs(z - log(v)) < 10**-p. If
             * p = ctx->prec+2-adjexp(lower), then the relative error of
             * the result is (using 10**adjexp(x) <= abs(x)):
             *
             *   abs(z - log(v)) / abs(log(v)) < 10**-p / abs(log(v))
             *                                 <= 10**(-ctx->prec-2)
             */
            maxprec = maxprec - mpd_adjexp(&tmp);
        }
    }

    i = ln_schedule_prec(klist, maxprec, 2);
    for (; i >= 0; i--) {
        varcontext.prec = 2*klist[i]+3;
        z->flags ^= MPD_NEG;
        _mpd_qexp(&tmp, z, &varcontext, status);
        z->flags ^= MPD_NEG;

        if (v.digits > varcontext.prec) {
            shift = v.digits - varcontext.prec;
            mpd_qshiftr(&vtmp, &v, shift, status);
            vtmp.exp += shift;
            mpd_qmul(&tmp, &vtmp, &tmp, &varcontext, status);
        }
        else {
            mpd_qmul(&tmp, &v, &tmp, &varcontext, status);
        }

        mpd_qsub(&tmp, &tmp, &one, &maxcontext, status);
        mpd_qadd(z, z, &tmp, &maxcontext, status);
        if (mpd_isspecial(z)) {
            break;
        }
    }

    /*
     * Case t == 0:
     *    t * log(10) == 0, the result does not change and the analysis
     *    above applies. If v < 0.900 or v > 1.15, the relative error is
     *    less than 10**(-ctx.prec-1).
     * Case t != 0:
     *      z := approx(log(v))
     *      y := approx(log(10))
     *      p := maxprec = ctx->prec + 2
     *   Absolute errors:
     *      1) abs(z - log(v)) < 10**-p
     *      2) abs(y - log(10)) < 10**-p
     *   The multiplication is exact, so:
     *      3) abs(t*y - t*log(10)) < t*10**-p
     *   The sum is exact, so:
     *      4) abs((z + t*y) - (log(v) + t*log(10))) < (abs(t) + 1) * 10**-p
     *   Bounds for log(v) and log(10):
     *      5) -7/10 < log(v) < 17/10
     *      6) 23/10 < log(10) < 24/10
     *   Using 4), 5), 6) and t != 0, the relative error is:
     *
     *      7) relerr < ((abs(t) + 1)*10**-p) / abs(log(v) + t*log(10))
     *                < 0.5 * 10**(-p + 1) = 0.5 * 10**(-ctx->prec-1)
     */
    mpd_qln10(&v, maxprec+1, status);
    mpd_qmul_ssize(&tmp, &v, t, &maxcontext, status);
    mpd_qadd(result, &tmp, z, &maxcontext, status);


finish:
    *status |= (MPD_Inexact|MPD_Rounded);
    mpd_del(&v);
    mpd_del(&vtmp);
    mpd_del(&tmp);
}

/* ln(a) */
void
mpd_qln(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
        uint32_t *status)
{
    mpd_context_t workctx;
    mpd_ssize_t adjexp, t;

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        if (mpd_isnegative(a)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
            return;
        }
        mpd_setspecial(result, MPD_POS, MPD_INF);
        return;
    }
    if (mpd_iszerocoeff(a)) {
        mpd_setspecial(result, MPD_NEG, MPD_INF);
        return;
    }
    if (mpd_isnegative(a)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (_mpd_cmp(a, &one) == 0) {
        _settriple(result, MPD_POS, 0, 0);
        return;
    }
    /*
     * Check if the result will overflow (0 < x, x != 1):
     *   1) log10(x) < 0 iff adjexp(x) < 0
     *   2) 0 < x /\ x <= y ==> adjexp(x) <= adjexp(y)
     *   3) 0 < x /\ x != 1 ==> 2 * abs(log10(x)) < abs(log(x))
     *   4) adjexp(x) <= log10(x) < adjexp(x) + 1
     *
     * Case adjexp(x) >= 0:
     *   5) 2 * adjexp(x) < abs(log(x))
     *   Case adjexp(x) > 0:
     *     6) adjexp(2 * adjexp(x)) <= adjexp(abs(log(x)))
     *   Case adjexp(x) == 0:
     *     mpd_exp_digits(t)-1 == 0 <= emax (the shortcut is not triggered)
     *
     * Case adjexp(x) < 0:
     *   7) 2 * (-adjexp(x) - 1) < abs(log(x))
     *   Case adjexp(x) < -1:
     *     8) adjexp(2 * (-adjexp(x) - 1)) <= adjexp(abs(log(x)))
     *   Case adjexp(x) == -1:
     *     mpd_exp_digits(t)-1 == 0 <= emax (the shortcut is not triggered)
     */
    adjexp = mpd_adjexp(a);
    t = (adjexp < 0) ? -adjexp-1 : adjexp;
    t *= 2;
    if (mpd_exp_digits(t)-1 > ctx->emax) {
        *status |= MPD_Overflow|MPD_Inexact|MPD_Rounded;
        mpd_setspecial(result, (adjexp<0), MPD_INF);
        return;
    }

    workctx = *ctx;
    workctx.round = MPD_ROUND_HALF_EVEN;

    if (ctx->allcr) {
        MPD_NEW_STATIC(t1, 0,0,0,0);
        MPD_NEW_STATIC(t2, 0,0,0,0);
        MPD_NEW_STATIC(ulp, 0,0,0,0);
        MPD_NEW_STATIC(aa, 0,0,0,0);
        mpd_ssize_t prec;

        if (result == a) {
            if (!mpd_qcopy(&aa, a, status)) {
                mpd_seterror(result, MPD_Malloc_error, status);
                return;
            }
            a = &aa;
        }

        workctx.clamp = 0;
        prec = ctx->prec + 3;
        while (1) {
            workctx.prec = prec;
            _mpd_qln(result, a, &workctx, status);
            _ssettriple(&ulp, MPD_POS, 1,
                        result->exp + result->digits-workctx.prec);

            workctx.prec = ctx->prec;
            mpd_qadd(&t1, result, &ulp, &workctx, &workctx.status);
            mpd_qsub(&t2, result, &ulp, &workctx, &workctx.status);
            if (mpd_isspecial(result) || mpd_iszerocoeff(result) ||
                mpd_qcmp(&t1, &t2, status) == 0) {
                workctx.clamp = ctx->clamp;
                mpd_check_underflow(result, &workctx, status);
                mpd_qfinalize(result, &workctx, status);
                break;
            }
            prec += MPD_RDIGITS;
        }
        mpd_del(&t1);
        mpd_del(&t2);
        mpd_del(&ulp);
        mpd_del(&aa);
    }
    else {
        _mpd_qln(result, a, &workctx, status);
        mpd_check_underflow(result, &workctx, status);
        mpd_qfinalize(result, &workctx, status);
    }
}

/*
 * Internal log10() function that does not check for specials, zero or one.
 * Case SKIP_FINALIZE:
 *   Relative error: abs(result - log10(a)) < 0.1 * 10**-prec * abs(log10(a))
 * Case DO_FINALIZE:
 *   Ulp error: abs(result - log10(a)) < ulp(log10(a))
 */
enum {SKIP_FINALIZE, DO_FINALIZE};
static void
_mpd_qlog10(int action, mpd_t *result, const mpd_t *a,
            const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t workctx;
    MPD_NEW_STATIC(ln10,0,0,0,0);

    mpd_maxcontext(&workctx);
    workctx.prec = ctx->prec + 3;
    /* relative error: 0.1 * 10**(-p-3). The specific underflow shortcut
     * in _mpd_qln() does not change the final result. */
    _mpd_qln(result, a, &workctx, status);
    /* relative error: 5 * 10**(-p-3) */
    mpd_qln10(&ln10, workctx.prec, status);

    if (action == DO_FINALIZE) {
        workctx = *ctx;
        workctx.round = MPD_ROUND_HALF_EVEN;
    }
    /* SKIP_FINALIZE: relative error: 5 * 10**(-p-3) */
    _mpd_qdiv(NO_IDEAL_EXP, result, result, &ln10, &workctx, status);

    mpd_del(&ln10);
}

/* log10(a) */
void
mpd_qlog10(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
           uint32_t *status)
{
    mpd_context_t workctx;
    mpd_ssize_t adjexp, t;

    workctx = *ctx;
    workctx.round = MPD_ROUND_HALF_EVEN;

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        if (mpd_isnegative(a)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
            return;
        }
        mpd_setspecial(result, MPD_POS, MPD_INF);
        return;
    }
    if (mpd_iszerocoeff(a)) {
        mpd_setspecial(result, MPD_NEG, MPD_INF);
        return;
    }
    if (mpd_isnegative(a)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (mpd_coeff_ispow10(a)) {
        uint8_t sign = 0;
        adjexp = mpd_adjexp(a);
        if (adjexp < 0) {
            sign = 1;
            adjexp = -adjexp;
        }
        _settriple(result, sign, adjexp, 0);
        mpd_qfinalize(result, &workctx, status);
        return;
    }
    /*
     * Check if the result will overflow (0 < x, x != 1):
     *   1) log10(x) < 0 iff adjexp(x) < 0
     *   2) 0 < x /\ x <= y ==> adjexp(x) <= adjexp(y)
     *   3) adjexp(x) <= log10(x) < adjexp(x) + 1
     *
     * Case adjexp(x) >= 0:
     *   4) adjexp(x) <= abs(log10(x))
     *   Case adjexp(x) > 0:
     *     5) adjexp(adjexp(x)) <= adjexp(abs(log10(x)))
     *   Case adjexp(x) == 0:
     *     mpd_exp_digits(t)-1 == 0 <= emax (the shortcut is not triggered)
     *
     * Case adjexp(x) < 0:
     *   6) -adjexp(x) - 1 < abs(log10(x))
     *   Case adjexp(x) < -1:
     *     7) adjexp(-adjexp(x) - 1) <= adjexp(abs(log(x)))
     *   Case adjexp(x) == -1:
     *     mpd_exp_digits(t)-1 == 0 <= emax (the shortcut is not triggered)
     */
    adjexp = mpd_adjexp(a);
    t = (adjexp < 0) ? -adjexp-1 : adjexp;
    if (mpd_exp_digits(t)-1 > ctx->emax) {
        *status |= MPD_Overflow|MPD_Inexact|MPD_Rounded;
        mpd_setspecial(result, (adjexp<0), MPD_INF);
        return;
    }

    if (ctx->allcr) {
        MPD_NEW_STATIC(t1, 0,0,0,0);
        MPD_NEW_STATIC(t2, 0,0,0,0);
        MPD_NEW_STATIC(ulp, 0,0,0,0);
        MPD_NEW_STATIC(aa, 0,0,0,0);
        mpd_ssize_t prec;

        if (result == a) {
            if (!mpd_qcopy(&aa, a, status)) {
                mpd_seterror(result, MPD_Malloc_error, status);
                return;
            }
            a = &aa;
        }

        workctx.clamp = 0;
        prec = ctx->prec + 3;
        while (1) {
            workctx.prec = prec;
            _mpd_qlog10(SKIP_FINALIZE, result, a, &workctx, status);
            _ssettriple(&ulp, MPD_POS, 1,
                        result->exp + result->digits-workctx.prec);

            workctx.prec = ctx->prec;
            mpd_qadd(&t1, result, &ulp, &workctx, &workctx.status);
            mpd_qsub(&t2, result, &ulp, &workctx, &workctx.status);
            if (mpd_isspecial(result) || mpd_iszerocoeff(result) ||
                mpd_qcmp(&t1, &t2, status) == 0) {
                workctx.clamp = ctx->clamp;
                mpd_check_underflow(result, &workctx, status);
                mpd_qfinalize(result, &workctx, status);
                break;
            }
            prec += MPD_RDIGITS;
        }
        mpd_del(&t1);
        mpd_del(&t2);
        mpd_del(&ulp);
        mpd_del(&aa);
    }
    else {
        _mpd_qlog10(DO_FINALIZE, result, a, &workctx, status);
        mpd_check_underflow(result, &workctx, status);
    }
}

/*
 * Maximum of the two operands. Attention: If one operand is a quiet NaN and the
 * other is numeric, the numeric operand is returned. This may not be what one
 * expects.
 */
void
mpd_qmax(mpd_t *result, const mpd_t *a, const mpd_t *b,
         const mpd_context_t *ctx, uint32_t *status)
{
    int c;

    if (mpd_isqnan(a) && !mpd_isnan(b)) {
        mpd_qcopy(result, b, status);
    }
    else if (mpd_isqnan(b) && !mpd_isnan(a)) {
        mpd_qcopy(result, a, status);
    }
    else if (mpd_qcheck_nans(result, a, b, ctx, status)) {
        return;
    }
    else {
        c = _mpd_cmp(a, b);
        if (c == 0) {
            c = _mpd_cmp_numequal(a, b);
        }

        if (c < 0) {
            mpd_qcopy(result, b, status);
        }
        else {
            mpd_qcopy(result, a, status);
        }
    }

    mpd_qfinalize(result, ctx, status);
}

/*
 * Maximum magnitude: Same as mpd_max(), but compares the operands with their
 * sign ignored.
 */
void
mpd_qmax_mag(mpd_t *result, const mpd_t *a, const mpd_t *b,
             const mpd_context_t *ctx, uint32_t *status)
{
    int c;

    if (mpd_isqnan(a) && !mpd_isnan(b)) {
        mpd_qcopy(result, b, status);
    }
    else if (mpd_isqnan(b) && !mpd_isnan(a)) {
        mpd_qcopy(result, a, status);
    }
    else if (mpd_qcheck_nans(result, a, b, ctx, status)) {
        return;
    }
    else {
        c = _mpd_cmp_abs(a, b);
        if (c == 0) {
            c = _mpd_cmp_numequal(a, b);
        }

        if (c < 0) {
            mpd_qcopy(result, b, status);
        }
        else {
            mpd_qcopy(result, a, status);
        }
    }

    mpd_qfinalize(result, ctx, status);
}

/*
 * Minimum of the two operands. Attention: If one operand is a quiet NaN and the
 * other is numeric, the numeric operand is returned. This may not be what one
 * expects.
 */
void
mpd_qmin(mpd_t *result, const mpd_t *a, const mpd_t *b,
         const mpd_context_t *ctx, uint32_t *status)
{
    int c;

    if (mpd_isqnan(a) && !mpd_isnan(b)) {
        mpd_qcopy(result, b, status);
    }
    else if (mpd_isqnan(b) && !mpd_isnan(a)) {
        mpd_qcopy(result, a, status);
    }
    else if (mpd_qcheck_nans(result, a, b, ctx, status)) {
        return;
    }
    else {
        c = _mpd_cmp(a, b);
        if (c == 0) {
            c = _mpd_cmp_numequal(a, b);
        }

        if (c < 0) {
            mpd_qcopy(result, a, status);
        }
        else {
            mpd_qcopy(result, b, status);
        }
    }

    mpd_qfinalize(result, ctx, status);
}

/*
 * Minimum magnitude: Same as mpd_min(), but compares the operands with their
 * sign ignored.
 */
void
mpd_qmin_mag(mpd_t *result, const mpd_t *a, const mpd_t *b,
             const mpd_context_t *ctx, uint32_t *status)
{
    int c;

    if (mpd_isqnan(a) && !mpd_isnan(b)) {
        mpd_qcopy(result, b, status);
    }
    else if (mpd_isqnan(b) && !mpd_isnan(a)) {
        mpd_qcopy(result, a, status);
    }
    else if (mpd_qcheck_nans(result, a, b, ctx, status)) {
        return;
    }
    else {
        c = _mpd_cmp_abs(a, b);
        if (c == 0) {
            c = _mpd_cmp_numequal(a, b);
        }

        if (c < 0) {
            mpd_qcopy(result, a, status);
        }
        else {
            mpd_qcopy(result, b, status);
        }
    }

    mpd_qfinalize(result, ctx, status);
}

/* Minimum space needed for the result array in _karatsuba_rec(). */
static inline mpd_size_t
_kmul_resultsize(mpd_size_t la, mpd_size_t lb)
{
    mpd_size_t n, m;

    n = add_size_t(la, lb);
    n = add_size_t(n, 1);

    m = (la+1)/2 + 1;
    m = mul_size_t(m, 3);

    return (m > n) ? m : n;
}

/* Work space needed in _karatsuba_rec(). lim >= 4 */
static inline mpd_size_t
_kmul_worksize(mpd_size_t n, mpd_size_t lim)
{
    mpd_size_t m;

    if (n <= lim) {
        return 0;
    }

    m = (n+1)/2 + 1;

    return add_size_t(mul_size_t(m, 2), _kmul_worksize(m, lim));
}


#define MPD_KARATSUBA_BASECASE 16  /* must be >= 4 */

/*
 * Add the product of a and b to c.
 * c must be _kmul_resultsize(la, lb) in size.
 * w is used as a work array and must be _kmul_worksize(a, lim) in size.
 * Roman E. Maeder, Storage Allocation for the Karatsuba Integer Multiplication
 * Algorithm. In "Design and implementation of symbolic computation systems",
 * Springer, 1993, ISBN 354057235X, 9783540572350.
 */
static void
_karatsuba_rec(mpd_uint_t *c, const mpd_uint_t *a, const mpd_uint_t *b,
               mpd_uint_t *w, mpd_size_t la, mpd_size_t lb)
{
    mpd_size_t m, lt;

    assert(la >= lb && lb > 0);
    assert(la <= MPD_KARATSUBA_BASECASE || w != NULL);

    if (la <= MPD_KARATSUBA_BASECASE) {
        _mpd_basemul(c, a, b, la, lb);
        return;
    }

    m = (la+1)/2;  /* ceil(la/2) */

    /* lb <= m < la */
    if (lb <= m) {

        /* lb can now be larger than la-m */
        if (lb > la-m) {
            lt = lb + lb + 1;       /* space needed for result array */
            mpd_uint_zero(w, lt);   /* clear result array */
            _karatsuba_rec(w, b, a+m, w+lt, lb, la-m); /* b*ah */
        }
        else {
            lt = (la-m) + (la-m) + 1;  /* space needed for result array */
            mpd_uint_zero(w, lt);      /* clear result array */
            _karatsuba_rec(w, a+m, b, w+lt, la-m, lb); /* ah*b */
        }
        _mpd_baseaddto(c+m, w, (la-m)+lb);      /* add ah*b*B**m */

        lt = m + m + 1;         /* space needed for the result array */
        mpd_uint_zero(w, lt);   /* clear result array */
        _karatsuba_rec(w, a, b, w+lt, m, lb);  /* al*b */
        _mpd_baseaddto(c, w, m+lb);    /* add al*b */

        return;
    }

    /* la >= lb > m */
    memcpy(w, a, m * sizeof *w);
    w[m] = 0;
    _mpd_baseaddto(w, a+m, la-m);

    memcpy(w+(m+1), b, m * sizeof *w);
    w[m+1+m] = 0;
    _mpd_baseaddto(w+(m+1), b+m, lb-m);

    _karatsuba_rec(c+m, w, w+(m+1), w+2*(m+1), m+1, m+1);

    lt = (la-m) + (la-m) + 1;
    mpd_uint_zero(w, lt);

    _karatsuba_rec(w, a+m, b+m, w+lt, la-m, lb-m);

    _mpd_baseaddto(c+2*m, w, (la-m) + (lb-m));
    _mpd_basesubfrom(c+m, w, (la-m) + (lb-m));

    lt = m + m + 1;
    mpd_uint_zero(w, lt);

    _karatsuba_rec(w, a, b, w+lt, m, m);
    _mpd_baseaddto(c, w, m+m);
    _mpd_basesubfrom(c+m, w, m+m);

    return;
}

/*
 * Multiply u and v, using Karatsuba multiplication. Returns a pointer
 * to the result or NULL in case of failure (malloc error).
 * Conditions: ulen >= vlen, ulen >= 4
 */
static mpd_uint_t *
_mpd_kmul(const mpd_uint_t *u, const mpd_uint_t *v,
          mpd_size_t ulen, mpd_size_t vlen,
          mpd_size_t *rsize)
{
    mpd_uint_t *result = NULL, *w = NULL;
    mpd_size_t m;

    assert(ulen >= 4);
    assert(ulen >= vlen);

    *rsize = _kmul_resultsize(ulen, vlen);
    if ((result = mpd_calloc(*rsize, sizeof *result)) == NULL) {
        return NULL;
    }

    m = _kmul_worksize(ulen, MPD_KARATSUBA_BASECASE);
    if (m && ((w = mpd_calloc(m, sizeof *w)) == NULL)) {
        mpd_free(result);
        return NULL;
    }

    _karatsuba_rec(result, u, v, w, ulen, vlen);


    if (w) mpd_free(w);
    return result;
}


/*
 * Determine the minimum length for the number theoretic transform. Valid
 * transform lengths are 2**n or 3*2**n, where 2**n <= MPD_MAXTRANSFORM_2N.
 * The function finds the shortest length m such that rsize <= m.
 */
static inline mpd_size_t
_mpd_get_transform_len(mpd_size_t rsize)
{
    mpd_size_t log2rsize;
    mpd_size_t x, step;

    assert(rsize >= 4);
    log2rsize = mpd_bsr(rsize);

    if (rsize <= 1024) {
        /* 2**n is faster in this range. */
        x = ((mpd_size_t)1)<<log2rsize;
        return (rsize == x) ? x : x<<1;
    }
    else if (rsize <= MPD_MAXTRANSFORM_2N) {
        x = ((mpd_size_t)1)<<log2rsize;
        if (rsize == x) return x;
        step = x>>1;
        x += step;
        return (rsize <= x) ? x : x + step;
    }
    else if (rsize <= MPD_MAXTRANSFORM_2N+MPD_MAXTRANSFORM_2N/2) {
        return MPD_MAXTRANSFORM_2N+MPD_MAXTRANSFORM_2N/2;
    }
    else if (rsize <= 3*MPD_MAXTRANSFORM_2N) {
        return 3*MPD_MAXTRANSFORM_2N;
    }
    else {
        return MPD_SIZE_MAX;
    }
}

#ifdef PPRO
#ifndef _MSC_VER
static inline unsigned short
_mpd_get_control87(void)
{
    unsigned short cw;

    __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));
    return cw;
}

static inline void
_mpd_set_control87(unsigned short cw)
{
    __asm__ __volatile__ ("fldcw %0" : : "m" (cw));
}
#endif

static unsigned int
mpd_set_fenv(void)
{
    unsigned int cw;
#ifdef _MSC_VER
    unsigned int flags =
        _EM_INVALID|_EM_DENORMAL|_EM_ZERODIVIDE|_EM_OVERFLOW|
        _EM_UNDERFLOW|_EM_INEXACT|_RC_CHOP|_PC_64;
    unsigned int mask = _MCW_EM|_MCW_RC|_MCW_PC;
    unsigned int dummy;

    __control87_2(0, 0, &cw, NULL);
    __control87_2(flags, mask, &dummy, NULL);
#else
    cw = _mpd_get_control87();
    _mpd_set_control87(cw|0xF3F);
#endif
    return cw;
}

static void
mpd_restore_fenv(unsigned int cw)
{
#ifdef _MSC_VER
    unsigned int mask = _MCW_EM|_MCW_RC|_MCW_PC;
    unsigned int dummy;

    __control87_2(cw, mask, &dummy, NULL);
#else
    _mpd_set_control87((unsigned short)cw);
#endif
}
#endif /* PPRO */

/*
 * Multiply u and v, using the fast number theoretic transform. Returns
 * a pointer to the result or NULL in case of failure (malloc error).
 */
static mpd_uint_t *
_mpd_fntmul(const mpd_uint_t *u, const mpd_uint_t *v,
            mpd_size_t ulen, mpd_size_t vlen,
            mpd_size_t *rsize)
{
    mpd_uint_t *c1 = NULL, *c2 = NULL, *c3 = NULL, *vtmp = NULL;
    mpd_size_t n;

#ifdef PPRO
    unsigned int cw;
    cw = mpd_set_fenv();
#endif

    *rsize = add_size_t(ulen, vlen);
    if ((n = _mpd_get_transform_len(*rsize)) == MPD_SIZE_MAX) {
        goto malloc_error;
    }

    if ((c1 = mpd_calloc(n, sizeof *c1)) == NULL) {
        goto malloc_error;
    }
    if ((c2 = mpd_calloc(n, sizeof *c2)) == NULL) {
        goto malloc_error;
    }
    if ((c3 = mpd_calloc(n, sizeof *c3)) == NULL) {
        goto malloc_error;
    }

    memcpy(c1, u, ulen * (sizeof *c1));
    memcpy(c2, u, ulen * (sizeof *c2));
    memcpy(c3, u, ulen * (sizeof *c3));

    if (u == v) {
        if (!fnt_autoconvolute(c1, n, P1) ||
            !fnt_autoconvolute(c2, n, P2) ||
            !fnt_autoconvolute(c3, n, P3)) {
            goto malloc_error;
        }
    }
    else {
        if ((vtmp = mpd_calloc(n, sizeof *vtmp)) == NULL) {
            goto malloc_error;
        }

        memcpy(vtmp, v, vlen * (sizeof *vtmp));
        if (!fnt_convolute(c1, vtmp, n, P1)) {
            mpd_free(vtmp);
            goto malloc_error;
        }

        memcpy(vtmp, v, vlen * (sizeof *vtmp));
        mpd_uint_zero(vtmp+vlen, n-vlen);
        if (!fnt_convolute(c2, vtmp, n, P2)) {
            mpd_free(vtmp);
            goto malloc_error;
        }

        memcpy(vtmp, v, vlen * (sizeof *vtmp));
        mpd_uint_zero(vtmp+vlen, n-vlen);
        if (!fnt_convolute(c3, vtmp, n, P3)) {
            mpd_free(vtmp);
            goto malloc_error;
        }

        mpd_free(vtmp);
    }

    crt3(c1, c2, c3, *rsize);

out:
#ifdef PPRO
    mpd_restore_fenv(cw);
#endif
    if (c2) mpd_free(c2);
    if (c3) mpd_free(c3);
    return c1;

malloc_error:
    if (c1) mpd_free(c1);
    c1 = NULL;
    goto out;
}


/*
 * Karatsuba multiplication with FNT/basemul as the base case.
 */
static int
_karatsuba_rec_fnt(mpd_uint_t *c, const mpd_uint_t *a, const mpd_uint_t *b,
                   mpd_uint_t *w, mpd_size_t la, mpd_size_t lb)
{
    mpd_size_t m, lt;

    assert(la >= lb && lb > 0);
    assert(la <= 3*(MPD_MAXTRANSFORM_2N/2) || w != NULL);

    if (la <= 3*(MPD_MAXTRANSFORM_2N/2)) {

        if (lb <= 192) {
            _mpd_basemul(c, b, a, lb, la);
        }
        else {
            mpd_uint_t *result;
            mpd_size_t dummy;

            if ((result = _mpd_fntmul(a, b, la, lb, &dummy)) == NULL) {
                return 0;
            }
            memcpy(c, result, (la+lb) * (sizeof *result));
            mpd_free(result);
        }
        return 1;
    }

    m = (la+1)/2;  /* ceil(la/2) */

    /* lb <= m < la */
    if (lb <= m) {

        /* lb can now be larger than la-m */
        if (lb > la-m) {
            lt = lb + lb + 1;       /* space needed for result array */
            mpd_uint_zero(w, lt);   /* clear result array */
            if (!_karatsuba_rec_fnt(w, b, a+m, w+lt, lb, la-m)) { /* b*ah */
                return 0; /* GCOV_UNLIKELY */
            }
        }
        else {
            lt = (la-m) + (la-m) + 1;  /* space needed for result array */
            mpd_uint_zero(w, lt);      /* clear result array */
            if (!_karatsuba_rec_fnt(w, a+m, b, w+lt, la-m, lb)) { /* ah*b */
                return 0; /* GCOV_UNLIKELY */
            }
        }
        _mpd_baseaddto(c+m, w, (la-m)+lb); /* add ah*b*B**m */

        lt = m + m + 1;         /* space needed for the result array */
        mpd_uint_zero(w, lt);   /* clear result array */
        if (!_karatsuba_rec_fnt(w, a, b, w+lt, m, lb)) {  /* al*b */
            return 0; /* GCOV_UNLIKELY */
        }
        _mpd_baseaddto(c, w, m+lb);       /* add al*b */

        return 1;
    }

    /* la >= lb > m */
    memcpy(w, a, m * sizeof *w);
    w[m] = 0;
    _mpd_baseaddto(w, a+m, la-m);

    memcpy(w+(m+1), b, m * sizeof *w);
    w[m+1+m] = 0;
    _mpd_baseaddto(w+(m+1), b+m, lb-m);

    if (!_karatsuba_rec_fnt(c+m, w, w+(m+1), w+2*(m+1), m+1, m+1)) {
        return 0; /* GCOV_UNLIKELY */
    }

    lt = (la-m) + (la-m) + 1;
    mpd_uint_zero(w, lt);

    if (!_karatsuba_rec_fnt(w, a+m, b+m, w+lt, la-m, lb-m)) {
        return 0; /* GCOV_UNLIKELY */
    }

    _mpd_baseaddto(c+2*m, w, (la-m) + (lb-m));
    _mpd_basesubfrom(c+m, w, (la-m) + (lb-m));

    lt = m + m + 1;
    mpd_uint_zero(w, lt);

    if (!_karatsuba_rec_fnt(w, a, b, w+lt, m, m)) {
        return 0; /* GCOV_UNLIKELY */
    }
    _mpd_baseaddto(c, w, m+m);
    _mpd_basesubfrom(c+m, w, m+m);

    return 1;
}

/*
 * Multiply u and v, using Karatsuba multiplication with the FNT as the
 * base case. Returns a pointer to the result or NULL in case of failure
 * (malloc error). Conditions: ulen >= vlen, ulen >= 4.
 */
static mpd_uint_t *
_mpd_kmul_fnt(const mpd_uint_t *u, const mpd_uint_t *v,
              mpd_size_t ulen, mpd_size_t vlen,
              mpd_size_t *rsize)
{
    mpd_uint_t *result = NULL, *w = NULL;
    mpd_size_t m;

    assert(ulen >= 4);
    assert(ulen >= vlen);

    *rsize = _kmul_resultsize(ulen, vlen);
    if ((result = mpd_calloc(*rsize, sizeof *result)) == NULL) {
        return NULL;
    }

    m = _kmul_worksize(ulen, 3*(MPD_MAXTRANSFORM_2N/2));
    if (m && ((w = mpd_calloc(m, sizeof *w)) == NULL)) {
        mpd_free(result); /* GCOV_UNLIKELY */
        return NULL; /* GCOV_UNLIKELY */
    }

    if (!_karatsuba_rec_fnt(result, u, v, w, ulen, vlen)) {
        mpd_free(result);
        result = NULL;
    }


    if (w) mpd_free(w);
    return result;
}


/* Deal with the special cases of multiplying infinities. */
static void
_mpd_qmul_inf(mpd_t *result, const mpd_t *a, const mpd_t *b, uint32_t *status)
{
    if (mpd_isinfinite(a)) {
        if (mpd_iszero(b)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
        }
        else {
            mpd_setspecial(result, mpd_sign(a)^mpd_sign(b), MPD_INF);
        }
        return;
    }
    assert(mpd_isinfinite(b));
    if (mpd_iszero(a)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
    }
    else {
        mpd_setspecial(result, mpd_sign(a)^mpd_sign(b), MPD_INF);
    }
}

/*
 * Internal function: Multiply a and b. _mpd_qmul deals with specials but
 * does NOT finalize the result. This is for use in mpd_fma().
 */
static inline void
_mpd_qmul(mpd_t *result, const mpd_t *a, const mpd_t *b,
          const mpd_context_t *ctx, uint32_t *status)
{
    const mpd_t *big = a, *small = b;
    mpd_uint_t *rdata = NULL;
    mpd_uint_t rbuf[MPD_MINALLOC_MAX];
    mpd_size_t rsize, i;


    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            return;
        }
        _mpd_qmul_inf(result, a, b, status);
        return;
    }

    if (small->len > big->len) {
        _mpd_ptrswap(&big, &small);
    }

    rsize = big->len + small->len;

    if (big->len == 1) {
        _mpd_singlemul(result->data, big->data[0], small->data[0]);
        goto finish;
    }
    if (rsize <= (mpd_size_t)MPD_MINALLOC_MAX) {
        if (big->len == 2) {
            _mpd_mul_2_le2(rbuf, big->data, small->data, small->len);
        }
        else {
            mpd_uint_zero(rbuf, rsize);
            if (small->len == 1) {
                _mpd_shortmul(rbuf, big->data, big->len, small->data[0]);
            }
            else {
                _mpd_basemul(rbuf, small->data, big->data, small->len, big->len);
            }
        }
        if (!mpd_qresize(result, rsize, status)) {
            return;
        }
        for(i = 0; i < rsize; i++) {
            result->data[i] = rbuf[i];
        }
        goto finish;
    }


    if (small->len <= 256) {
        rdata = mpd_calloc(rsize, sizeof *rdata);
        if (rdata != NULL) {
            if (small->len == 1) {
                _mpd_shortmul(rdata, big->data, big->len, small->data[0]);
            }
            else {
                _mpd_basemul(rdata, small->data, big->data, small->len, big->len);
            }
        }
    }
    else if (rsize <= 1024) {
        rdata = _mpd_kmul(big->data, small->data, big->len, small->len, &rsize);
    }
    else if (rsize <= 3*MPD_MAXTRANSFORM_2N) {
        rdata = _mpd_fntmul(big->data, small->data, big->len, small->len, &rsize);
    }
    else {
        rdata = _mpd_kmul_fnt(big->data, small->data, big->len, small->len, &rsize);
    }

    if (rdata == NULL) {
        mpd_seterror(result, MPD_Malloc_error, status);
        return;
    }

    if (mpd_isdynamic_data(result)) {
        mpd_free(result->data);
    }
    result->data = rdata;
    result->alloc = rsize;
    mpd_set_dynamic_data(result);


finish:
    mpd_set_flags(result, mpd_sign(a)^mpd_sign(b));
    result->exp = big->exp + small->exp;
    result->len = _mpd_real_size(result->data, rsize);
    /* resize to smaller cannot fail */
    mpd_qresize(result, result->len, status);
    mpd_setdigits(result);
}

/* Multiply a and b. */
void
mpd_qmul(mpd_t *result, const mpd_t *a, const mpd_t *b,
         const mpd_context_t *ctx, uint32_t *status)
{
    _mpd_qmul(result, a, b, ctx, status);
    mpd_qfinalize(result, ctx, status);
}

/* Multiply a and b. Set NaN/Invalid_operation if the result is inexact. */
static void
_mpd_qmul_exact(mpd_t *result, const mpd_t *a, const mpd_t *b,
                const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;

    mpd_qmul(result, a, b, ctx, &workstatus);
    *status |= workstatus;
    if (workstatus & (MPD_Inexact|MPD_Rounded|MPD_Clamped)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
    }
}

/* Multiply decimal and mpd_ssize_t. */
void
mpd_qmul_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b,
               const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qsset_ssize(&bb, b, &maxcontext, status);
    mpd_qmul(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Multiply decimal and mpd_uint_t. */
void
mpd_qmul_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b,
              const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qsset_uint(&bb, b, &maxcontext, status);
    mpd_qmul(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

void
mpd_qmul_i32(mpd_t *result, const mpd_t *a, int32_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qmul_ssize(result, a, b, ctx, status);
}

void
mpd_qmul_u32(mpd_t *result, const mpd_t *a, uint32_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qmul_uint(result, a, b, ctx, status);
}

#ifdef CONFIG_64
void
mpd_qmul_i64(mpd_t *result, const mpd_t *a, int64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qmul_ssize(result, a, b, ctx, status);
}

void
mpd_qmul_u64(mpd_t *result, const mpd_t *a, uint64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_qmul_uint(result, a, b, ctx, status);
}
#elif !defined(LEGACY_COMPILER)
/* Multiply decimal and int64_t. */
void
mpd_qmul_i64(mpd_t *result, const mpd_t *a, int64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qset_i64(&bb, b, &maxcontext, status);
    mpd_qmul(result, a, &bb, ctx, status);
    mpd_del(&bb);
}

/* Multiply decimal and uint64_t. */
void
mpd_qmul_u64(mpd_t *result, const mpd_t *a, uint64_t b,
             const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(bb,0,0,0,0);

    mpd_maxcontext(&maxcontext);
    mpd_qset_u64(&bb, b, &maxcontext, status);
    mpd_qmul(result, a, &bb, ctx, status);
    mpd_del(&bb);
}
#endif

/* Like the minus operator. */
void
mpd_qminus(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
           uint32_t *status)
{
    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
    }

    if (mpd_iszero(a) && ctx->round != MPD_ROUND_FLOOR) {
        mpd_qcopy_abs(result, a, status);
    }
    else {
        mpd_qcopy_negate(result, a, status);
    }

    mpd_qfinalize(result, ctx, status);
}

/* Like the plus operator. */
void
mpd_qplus(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
          uint32_t *status)
{
    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
    }

    if (mpd_iszero(a) && ctx->round != MPD_ROUND_FLOOR) {
        mpd_qcopy_abs(result, a, status);
    }
    else {
        mpd_qcopy(result, a, status);
    }

    mpd_qfinalize(result, ctx, status);
}

/* The largest representable number that is smaller than the operand. */
void
mpd_qnext_minus(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
                uint32_t *status)
{
    mpd_context_t workctx;
    MPD_NEW_CONST(tiny,MPD_POS,mpd_etiny(ctx)-1,1,1,1,1);

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }

        assert(mpd_isinfinite(a));
        if (mpd_isnegative(a)) {
            mpd_qcopy(result, a, status);
            return;
        }
        else {
            mpd_clear_flags(result);
            mpd_qmaxcoeff(result, ctx, status);
            if (mpd_isnan(result)) {
                return;
            }
            result->exp = mpd_etop(ctx);
            return;
        }
    }

    mpd_workcontext(&workctx, ctx);
    workctx.round = MPD_ROUND_FLOOR;

    if (!mpd_qcopy(result, a, status)) {
        return;
    }

    mpd_qfinalize(result, &workctx, &workctx.status);
    if (workctx.status&(MPD_Inexact|MPD_Errors)) {
        *status |= (workctx.status&MPD_Errors);
        return;
    }

    workctx.status = 0;
    mpd_qsub(result, a, &tiny, &workctx, &workctx.status);
    *status |= (workctx.status&MPD_Errors);
}

/* The smallest representable number that is larger than the operand. */
void
mpd_qnext_plus(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
               uint32_t *status)
{
    mpd_context_t workctx;
    MPD_NEW_CONST(tiny,MPD_POS,mpd_etiny(ctx)-1,1,1,1,1);

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }

        assert(mpd_isinfinite(a));
        if (mpd_ispositive(a)) {
            mpd_qcopy(result, a, status);
        }
        else {
            mpd_clear_flags(result);
            mpd_qmaxcoeff(result, ctx, status);
            if (mpd_isnan(result)) {
                return;
            }
            mpd_set_flags(result, MPD_NEG);
            result->exp = mpd_etop(ctx);
        }
        return;
    }

    mpd_workcontext(&workctx, ctx);
    workctx.round = MPD_ROUND_CEILING;

    if (!mpd_qcopy(result, a, status)) {
        return;
    }

    mpd_qfinalize(result, &workctx, &workctx.status);
    if (workctx.status & (MPD_Inexact|MPD_Errors)) {
        *status |= (workctx.status&MPD_Errors);
        return;
    }

    workctx.status = 0;
    mpd_qadd(result, a, &tiny, &workctx, &workctx.status);
    *status |= (workctx.status&MPD_Errors);
}

/*
 * The number closest to the first operand that is in the direction towards
 * the second operand.
 */
void
mpd_qnext_toward(mpd_t *result, const mpd_t *a, const mpd_t *b,
                 const mpd_context_t *ctx, uint32_t *status)
{
    int c;

    if (mpd_qcheck_nans(result, a, b, ctx, status)) {
        return;
    }

    c = _mpd_cmp(a, b);
    if (c == 0) {
        mpd_qcopy_sign(result, a, b, status);
        return;
    }

    if (c < 0) {
        mpd_qnext_plus(result, a, ctx, status);
    }
    else {
        mpd_qnext_minus(result, a, ctx, status);
    }

    if (mpd_isinfinite(result)) {
        *status |= (MPD_Overflow|MPD_Rounded|MPD_Inexact);
    }
    else if (mpd_adjexp(result) < ctx->emin) {
        *status |= (MPD_Underflow|MPD_Subnormal|MPD_Rounded|MPD_Inexact);
        if (mpd_iszero(result)) {
            *status |= MPD_Clamped;
        }
    }
}

/*
 * Internal function: Integer power with mpd_uint_t exponent. The function
 * can fail with MPD_Malloc_error.
 *
 * The error is equal to the error incurred in k-1 multiplications. Assuming
 * the upper bound for the relative error in each operation:
 *
 *   abs(err) = 5 * 10**-prec
 *   result = x**k * (1 + err)**(k-1)
 */
static inline void
_mpd_qpow_uint(mpd_t *result, const mpd_t *base, mpd_uint_t exp,
               uint8_t resultsign, const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_uint_t n;

    if (exp == 0) {
        _settriple(result, resultsign, 1, 0); /* GCOV_NOT_REACHED */
        return; /* GCOV_NOT_REACHED */
    }

    if (!mpd_qcopy(result, base, status)) {
        return;
    }

    n = mpd_bits[mpd_bsr(exp)];
    while (n >>= 1) {
        mpd_qmul(result, result, result, ctx, &workstatus);
        if (exp & n) {
            mpd_qmul(result, result, base, ctx, &workstatus);
        }
        if (mpd_isspecial(result) ||
            (mpd_iszerocoeff(result) && (workstatus & MPD_Clamped))) {
            break;
        }
    }

    *status |= workstatus;
    mpd_set_sign(result, resultsign);
}

/*
 * Internal function: Integer power with mpd_t exponent, tbase and texp
 * are modified!! Function can fail with MPD_Malloc_error.
 *
 * The error is equal to the error incurred in k multiplications. Assuming
 * the upper bound for the relative error in each operation:
 *
 *   abs(err) = 5 * 10**-prec
 *   result = x**k * (1 + err)**k
 */
static inline void
_mpd_qpow_mpd(mpd_t *result, mpd_t *tbase, mpd_t *texp, uint8_t resultsign,
              const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_context_t maxctx;
    MPD_NEW_CONST(two,0,0,1,1,1,2);


    mpd_maxcontext(&maxctx);

    /* resize to smaller cannot fail */
    mpd_qcopy(result, &one, status);

    while (!mpd_iszero(texp)) {
        if (mpd_isodd(texp)) {
            mpd_qmul(result, result, tbase, ctx, &workstatus);
            *status |= workstatus;
            if (mpd_isspecial(result) ||
                (mpd_iszerocoeff(result) && (workstatus & MPD_Clamped))) {
                break;
            }
        }
        mpd_qmul(tbase, tbase, tbase, ctx, &workstatus);
        mpd_qdivint(texp, texp, &two, &maxctx, &workstatus);
        if (mpd_isnan(tbase) || mpd_isnan(texp)) {
            mpd_seterror(result, workstatus&MPD_Errors, status);
            return;
        }
    }
    mpd_set_sign(result, resultsign);
}

/*
 * The power function for integer exponents. Relative error _before_ the
 * final rounding to prec:
 *   abs(result - base**exp) < 0.1 * 10**-prec * abs(base**exp)
 */
static void
_mpd_qpow_int(mpd_t *result, const mpd_t *base, const mpd_t *exp,
              uint8_t resultsign,
              const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t workctx;
    MPD_NEW_STATIC(tbase,0,0,0,0);
    MPD_NEW_STATIC(texp,0,0,0,0);
    mpd_ssize_t n;


    mpd_workcontext(&workctx, ctx);
    workctx.prec += (exp->digits + exp->exp + 2);
    workctx.round = MPD_ROUND_HALF_EVEN;
    workctx.clamp = 0;
    if (mpd_isnegative(exp)) {
        uint32_t workstatus = 0;
        workctx.prec += 1;
        mpd_qdiv(&tbase, &one, base, &workctx, &workstatus);
        *status |= workstatus;
        if (workstatus&MPD_Errors) {
            mpd_setspecial(result, MPD_POS, MPD_NAN);
            goto finish;
        }
    }
    else {
        if (!mpd_qcopy(&tbase, base, status)) {
            mpd_setspecial(result, MPD_POS, MPD_NAN);
            goto finish;
        }
    }

    n = mpd_qabs_uint(exp, &workctx.status);
    if (workctx.status&MPD_Invalid_operation) {
        if (!mpd_qcopy(&texp, exp, status)) {
            mpd_setspecial(result, MPD_POS, MPD_NAN); /* GCOV_UNLIKELY */
            goto finish; /* GCOV_UNLIKELY */
        }
        _mpd_qpow_mpd(result, &tbase, &texp, resultsign, &workctx, status);
    }
    else {
        _mpd_qpow_uint(result, &tbase, n, resultsign, &workctx, status);
    }

    if (mpd_isinfinite(result)) {
        /* for ROUND_DOWN, ROUND_FLOOR, etc. */
        _settriple(result, resultsign, 1, MPD_EXP_INF);
    }

finish:
    mpd_del(&tbase);
    mpd_del(&texp);
    mpd_qfinalize(result, ctx, status);
}

/*
 * If the exponent is infinite and base equals one, the result is one
 * with a coefficient of length prec. Otherwise, result is undefined.
 * Return the value of the comparison against one.
 */
static int
_qcheck_pow_one_inf(mpd_t *result, const mpd_t *base, uint8_t resultsign,
                    const mpd_context_t *ctx, uint32_t *status)
{
    mpd_ssize_t shift;
    int cmp;

    if ((cmp = _mpd_cmp(base, &one)) == 0) {
        shift = ctx->prec-1;
        mpd_qshiftl(result, &one, shift, status);
        result->exp = -shift;
        mpd_set_flags(result, resultsign);
        *status |= (MPD_Inexact|MPD_Rounded);
    }

    return cmp;
}

/*
 * If abs(base) equals one, calculate the correct power of one result.
 * Otherwise, result is undefined. Return the value of the comparison
 * against 1.
 *
 * This is an internal function that does not check for specials.
 */
static int
_qcheck_pow_one(mpd_t *result, const mpd_t *base, const mpd_t *exp,
                uint8_t resultsign,
                const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_ssize_t shift;
    int cmp;

    if ((cmp = _mpd_cmp_abs(base, &one)) == 0) {
        if (_mpd_isint(exp)) {
            if (mpd_isnegative(exp)) {
                _settriple(result, resultsign, 1, 0);
                return 0;
            }
            /* 1.000**3 = 1.000000000 */
            mpd_qmul_ssize(result, exp, -base->exp, ctx, &workstatus);
            if (workstatus&MPD_Errors) {
                *status |= (workstatus&MPD_Errors);
                return 0;
            }
            /* digits-1 after exponentiation */
            shift = mpd_qget_ssize(result, &workstatus);
            /* shift is MPD_SSIZE_MAX if result is too large */
            if (shift > ctx->prec-1) {
                shift = ctx->prec-1;
                *status |= MPD_Rounded;
            }
        }
        else if (mpd_ispositive(base)) {
            shift = ctx->prec-1;
            *status |= (MPD_Inexact|MPD_Rounded);
        }
        else {
            return -2; /* GCOV_NOT_REACHED */
        }
        if (!mpd_qshiftl(result, &one, shift, status)) {
            return 0;
        }
        result->exp = -shift;
        mpd_set_flags(result, resultsign);
    }

    return cmp;
}

/*
 * Detect certain over/underflow of x**y.
 * ACL2 proof: pow-bounds.lisp.
 *
 *   Symbols:
 *
 *     e: EXP_INF or EXP_CLAMP
 *     x: base
 *     y: exponent
 *
 *     omega(e) = log10(abs(e))
 *     zeta(x)  = log10(abs(log10(x)))
 *     theta(y) = log10(abs(y))
 *
 *   Upper and lower bounds:
 *
 *     ub_omega(e) = ceil(log10(abs(e)))
 *     lb_theta(y) = floor(log10(abs(y)))
 *
 *                  | floor(log10(floor(abs(log10(x))))) if x < 1/10 or x >= 10
 *     lb_zeta(x) = | floor(log10(abs(x-1)/10)) if 1/10 <= x < 1
 *                  | floor(log10(abs((x-1)/100))) if 1 < x < 10
 *
 *   ub_omega(e) and lb_theta(y) are obviously upper and lower bounds
 *   for omega(e) and theta(y).
 *
 *   lb_zeta is a lower bound for zeta(x):
 *
 *     x < 1/10 or x >= 10:
 *
 *       abs(log10(x)) >= 1, so the outer log10 is well defined. Since log10
 *       is strictly increasing, the end result is a lower bound.
 *
 *     1/10 <= x < 1:
 *
 *       We use: log10(x) <= (x-1)/log(10)
 *               abs(log10(x)) >= abs(x-1)/log(10)
 *               abs(log10(x)) >= abs(x-1)/10
 *
 *     1 < x < 10:
 *
 *       We use: (x-1)/(x*log(10)) < log10(x)
 *               abs((x-1)/100) < abs(log10(x))
 *
 *       XXX: abs((x-1)/10) would work, need ACL2 proof.
 *
 *
 *   Let (0 < x < 1 and y < 0) or (x > 1 and y > 0).                  (H1)
 *   Let ub_omega(exp_inf) < lb_zeta(x) + lb_theta(y)                 (H2)
 *
 *   Then:
 *       log10(abs(exp_inf)) < log10(abs(log10(x))) + log10(abs(y)).   (1)
 *                   exp_inf < log10(x) * y                            (2)
 *               10**exp_inf < x**y                                    (3)
 *
 *   Let (0 < x < 1 and y > 0) or (x > 1 and y < 0).                  (H3)
 *   Let ub_omega(exp_clamp) < lb_zeta(x) + lb_theta(y)               (H4)
 *
 *   Then:
 *     log10(abs(exp_clamp)) < log10(abs(log10(x))) + log10(abs(y)).   (4)
 *              log10(x) * y < exp_clamp                               (5)
 *                      x**y < 10**exp_clamp                           (6)
 *
 */
static mpd_ssize_t
_lower_bound_zeta(const mpd_t *x, uint32_t *status)
{
    mpd_context_t maxctx;
    MPD_NEW_STATIC(scratch,0,0,0,0);
    mpd_ssize_t t, u;

    t = mpd_adjexp(x);
    if (t > 0) {
        /* x >= 10 -> floor(log10(floor(abs(log10(x))))) */
        return mpd_exp_digits(t) - 1;
    }
    else if (t < -1) {
        /* x < 1/10 -> floor(log10(floor(abs(log10(x))))) */
        return mpd_exp_digits(t+1) - 1;
    }
    else {
        mpd_maxcontext(&maxctx);
        mpd_qsub(&scratch, x, &one, &maxctx, status);
        if (mpd_isspecial(&scratch)) {
            mpd_del(&scratch);
            return MPD_SSIZE_MAX;
        }
        u = mpd_adjexp(&scratch);
        mpd_del(&scratch);

        /* t == -1, 1/10 <= x < 1 -> floor(log10(abs(x-1)/10))
         * t == 0,  1 < x < 10    -> floor(log10(abs(x-1)/100)) */
        return (t == 0) ? u-2 : u-1;
    }
}

/*
 * Detect cases of certain overflow/underflow in the power function.
 * Assumptions: x != 1, y != 0. The proof above is for positive x.
 * If x is negative and y is an odd integer, x**y == -(abs(x)**y),
 * so the analysis does not change.
 */
static int
_qcheck_pow_bounds(mpd_t *result, const mpd_t *x, const mpd_t *y,
                   uint8_t resultsign,
                   const mpd_context_t *ctx, uint32_t *status)
{
    MPD_NEW_SHARED(abs_x, x);
    mpd_ssize_t ub_omega, lb_zeta, lb_theta;
    uint8_t sign;

    mpd_set_positive(&abs_x);

    lb_theta = mpd_adjexp(y);
    lb_zeta = _lower_bound_zeta(&abs_x, status);
    if (lb_zeta == MPD_SSIZE_MAX) {
        mpd_seterror(result, MPD_Malloc_error, status);
        return 1;
    }

    sign = (mpd_adjexp(&abs_x) < 0) ^ mpd_sign(y);
    if (sign == 0) {
        /* (0 < |x| < 1 and y < 0) or (|x| > 1 and y > 0) */
        ub_omega = mpd_exp_digits(ctx->emax);
        if (ub_omega < lb_zeta + lb_theta) {
            _settriple(result, resultsign, 1, MPD_EXP_INF);
            mpd_qfinalize(result, ctx, status);
            return 1;
        }
    }
    else {
        /* (0 < |x| < 1 and y > 0) or (|x| > 1 and y < 0). */
        ub_omega = mpd_exp_digits(mpd_etiny(ctx));
        if (ub_omega < lb_zeta + lb_theta) {
            _settriple(result, resultsign, 1, mpd_etiny(ctx)-1);
            mpd_qfinalize(result, ctx, status);
            return 1;
        }
    }

    return 0;
}

/*
 * TODO: Implement algorithm for computing exact powers from decimal.py.
 * In order to prevent infinite loops, this has to be called before
 * using Ziv's strategy for correct rounding.
 */
/*
static int
_mpd_qpow_exact(mpd_t *result, const mpd_t *base, const mpd_t *exp,
                const mpd_context_t *ctx, uint32_t *status)
{
    return 0;
}
*/

/*
 * The power function for real exponents.
 *   Relative error: abs(result - e**y) < e**y * 1/5 * 10**(-prec - 1)
 */
static void
_mpd_qpow_real(mpd_t *result, const mpd_t *base, const mpd_t *exp,
               const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t workctx;
    MPD_NEW_STATIC(texp,0,0,0,0);

    if (!mpd_qcopy(&texp, exp, status)) {
        mpd_seterror(result, MPD_Malloc_error, status);
        return;
    }

    mpd_maxcontext(&workctx);
    workctx.prec = (base->digits > ctx->prec) ? base->digits : ctx->prec;
    workctx.prec += (4 + MPD_EXPDIGITS);
    workctx.round = MPD_ROUND_HALF_EVEN;
    workctx.allcr = ctx->allcr;

    /*
     * extra := MPD_EXPDIGITS = MPD_EXP_MAX_T
     * wp := prec + 4 + extra
     * abs(err) < 5 * 10**-wp
     * y := log(base) * exp
     * Calculate:
     *   1)   e**(y * (1 + err)**2) * (1 + err)
     *      = e**y * e**(y * (2*err + err**2)) * (1 + err)
     *        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
     * Relative error of the underlined term:
     *   2) abs(e**(y * (2*err + err**2)) - 1)
     * Case abs(y) >= 10**extra:
     *   3) adjexp(y)+1 > log10(abs(y)) >= extra
     *   This triggers the Overflow/Underflow shortcut in _mpd_qexp(),
     *   so no further analysis is necessary.
     * Case abs(y) < 10**extra:
     *   4) abs(y * (2*err + err**2)) < 1/5 * 10**(-prec - 2)
     *   Use (see _mpd_qexp):
     *     5) abs(x) <= 9/10 * 10**-p ==> abs(e**x - 1) < 10**-p
     *   With 2), 4) and 5):
     *     6) abs(e**(y * (2*err + err**2)) - 1) < 10**(-prec - 2)
     *   The complete relative error of 1) is:
     *     7) abs(result - e**y) < e**y * 1/5 * 10**(-prec - 1)
     */
    mpd_qln(result, base, &workctx, &workctx.status);
    mpd_qmul(result, result, &texp, &workctx, &workctx.status);
    mpd_qexp(result, result, &workctx, status);

    mpd_del(&texp);
    *status |= (workctx.status&MPD_Errors);
    *status |= (MPD_Inexact|MPD_Rounded);
}

/* The power function: base**exp */
void
mpd_qpow(mpd_t *result, const mpd_t *base, const mpd_t *exp,
         const mpd_context_t *ctx, uint32_t *status)
{
    uint8_t resultsign = 0;
    int intexp = 0;
    int cmp;

    if (mpd_isspecial(base) || mpd_isspecial(exp)) {
        if (mpd_qcheck_nans(result, base, exp, ctx, status)) {
            return;
        }
    }
    if (mpd_isinteger(exp)) {
        intexp = 1;
        resultsign = mpd_isnegative(base) && mpd_isodd(exp);
    }

    if (mpd_iszero(base)) {
        if (mpd_iszero(exp)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
        }
        else if (mpd_isnegative(exp)) {
            mpd_setspecial(result, resultsign, MPD_INF);
        }
        else {
            _settriple(result, resultsign, 0, 0);
        }
        return;
    }
    if (mpd_isnegative(base)) {
        if (!intexp || mpd_isinfinite(exp)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
            return;
        }
    }
    if (mpd_isinfinite(exp)) {
        /* power of one */
        cmp = _qcheck_pow_one_inf(result, base, resultsign, ctx, status);
        if (cmp == 0) {
            return;
        }
        else {
            cmp *= mpd_arith_sign(exp);
            if (cmp < 0) {
                _settriple(result, resultsign, 0, 0);
            }
            else {
                mpd_setspecial(result, resultsign, MPD_INF);
            }
        }
        return;
    }
    if (mpd_isinfinite(base)) {
        if (mpd_iszero(exp)) {
            _settriple(result, resultsign, 1, 0);
        }
        else if (mpd_isnegative(exp)) {
            _settriple(result, resultsign, 0, 0);
        }
        else {
            mpd_setspecial(result, resultsign, MPD_INF);
        }
        return;
    }
    if (mpd_iszero(exp)) {
        _settriple(result, resultsign, 1, 0);
        return;
    }
    if (_qcheck_pow_one(result, base, exp, resultsign, ctx, status) == 0) {
        return;
    }
    if (_qcheck_pow_bounds(result, base, exp, resultsign, ctx, status)) {
        return;
    }

    if (intexp) {
        _mpd_qpow_int(result, base, exp, resultsign, ctx, status);
    }
    else {
        _mpd_qpow_real(result, base, exp, ctx, status);
        if (!mpd_isspecial(result) && _mpd_cmp(result, &one) == 0) {
            mpd_ssize_t shift = ctx->prec-1;
            mpd_qshiftl(result, &one, shift, status);
            result->exp = -shift;
        }
        if (mpd_isinfinite(result)) {
            /* for ROUND_DOWN, ROUND_FLOOR, etc. */
            _settriple(result, MPD_POS, 1, MPD_EXP_INF);
        }
        mpd_qfinalize(result, ctx, status);
    }
}

/*
 * Internal function: Integer powmod with mpd_uint_t exponent, base is modified!
 * Function can fail with MPD_Malloc_error.
 */
static inline void
_mpd_qpowmod_uint(mpd_t *result, mpd_t *base, mpd_uint_t exp,
                  const mpd_t *mod, uint32_t *status)
{
    mpd_context_t maxcontext;

    mpd_maxcontext(&maxcontext);

    /* resize to smaller cannot fail */
    mpd_qcopy(result, &one, status);

    while (exp > 0) {
        if (exp & 1) {
            _mpd_qmul_exact(result, result, base, &maxcontext, status);
            mpd_qrem(result, result, mod, &maxcontext, status);
        }
        _mpd_qmul_exact(base, base, base, &maxcontext, status);
        mpd_qrem(base, base, mod, &maxcontext, status);
        exp >>= 1;
    }
}

/* The powmod function: (base**exp) % mod */
void
mpd_qpowmod(mpd_t *result, const mpd_t *base, const mpd_t *exp,
            const mpd_t *mod,
            const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(tbase,0,0,0,0);
    MPD_NEW_STATIC(texp,0,0,0,0);
    MPD_NEW_STATIC(tmod,0,0,0,0);
    MPD_NEW_STATIC(tmp,0,0,0,0);
    MPD_NEW_CONST(two,0,0,1,1,1,2);
    mpd_ssize_t tbase_exp, texp_exp;
    mpd_ssize_t i;
    mpd_t t;
    mpd_uint_t r;
    uint8_t sign;


    if (mpd_isspecial(base) || mpd_isspecial(exp) || mpd_isspecial(mod)) {
        if (mpd_qcheck_3nans(result, base, exp, mod, ctx, status)) {
            return;
        }
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }


    if (!_mpd_isint(base) || !_mpd_isint(exp) || !_mpd_isint(mod)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (mpd_iszerocoeff(mod)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (mod->digits+mod->exp > ctx->prec) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    sign = (mpd_isnegative(base)) && (mpd_isodd(exp));
    if (mpd_iszerocoeff(exp)) {
        if (mpd_iszerocoeff(base)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
            return;
        }
        r = (_mpd_cmp_abs(mod, &one)==0) ? 0 : 1;
        _settriple(result, sign, r, 0);
        return;
    }
    if (mpd_isnegative(exp)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }
    if (mpd_iszerocoeff(base)) {
        _settriple(result, sign, 0, 0);
        return;
    }

    mpd_maxcontext(&maxcontext);

    mpd_qrescale(&tmod, mod, 0, &maxcontext, &maxcontext.status);
    if (maxcontext.status&MPD_Errors) {
        mpd_seterror(result, maxcontext.status&MPD_Errors, status);
        goto out;
    }
    maxcontext.status = 0;
    mpd_set_positive(&tmod);

    mpd_qround_to_int(&tbase, base, &maxcontext, status);
    mpd_set_positive(&tbase);
    tbase_exp = tbase.exp;
    tbase.exp = 0;

    mpd_qround_to_int(&texp, exp, &maxcontext, status);
    texp_exp = texp.exp;
    texp.exp = 0;

    /* base = (base.int % modulo * pow(10, base.exp, modulo)) % modulo */
    mpd_qrem(&tbase, &tbase, &tmod, &maxcontext, status);
    mpd_qshiftl(result, &one, tbase_exp, status);
    mpd_qrem(result, result, &tmod, &maxcontext, status);
    _mpd_qmul_exact(&tbase, &tbase, result, &maxcontext, status);
    mpd_qrem(&tbase, &tbase, &tmod, &maxcontext, status);
    if (mpd_isspecial(&tbase) ||
        mpd_isspecial(&texp) ||
        mpd_isspecial(&tmod)) {
        goto mpd_errors;
    }

    for (i = 0; i < texp_exp; i++) {
        _mpd_qpowmod_uint(&tmp, &tbase, 10, &tmod, status);
        t = tmp;
        tmp = tbase;
        tbase = t;
    }
    if (mpd_isspecial(&tbase)) {
        goto mpd_errors; /* GCOV_UNLIKELY */
    }

    /* resize to smaller cannot fail */
    mpd_qcopy(result, &one, status);
    while (mpd_isfinite(&texp) && !mpd_iszero(&texp)) {
        if (mpd_isodd(&texp)) {
            _mpd_qmul_exact(result, result, &tbase, &maxcontext, status);
            mpd_qrem(result, result, &tmod, &maxcontext, status);
        }
        _mpd_qmul_exact(&tbase, &tbase, &tbase, &maxcontext, status);
        mpd_qrem(&tbase, &tbase, &tmod, &maxcontext, status);
        mpd_qdivint(&texp, &texp, &two, &maxcontext, status);
    }
    if (mpd_isspecial(&texp) || mpd_isspecial(&tbase) ||
        mpd_isspecial(&tmod) || mpd_isspecial(result)) {
        /* MPD_Malloc_error */
        goto mpd_errors;
    }
    else {
        mpd_set_sign(result, sign);
    }

out:
    mpd_del(&tbase);
    mpd_del(&texp);
    mpd_del(&tmod);
    mpd_del(&tmp);
    return;

mpd_errors:
    mpd_setspecial(result, MPD_POS, MPD_NAN);
    goto out;
}

void
mpd_qquantize(mpd_t *result, const mpd_t *a, const mpd_t *b,
              const mpd_context_t *ctx, uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_ssize_t b_exp = b->exp;
    mpd_ssize_t expdiff, shift;
    mpd_uint_t rnd;

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(result, a, b, ctx, status)) {
            return;
        }
        if (mpd_isinfinite(a) && mpd_isinfinite(b)) {
            mpd_qcopy(result, a, status);
            return;
        }
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    if (b->exp > ctx->emax || b->exp < mpd_etiny(ctx)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    if (mpd_iszero(a)) {
        _settriple(result, mpd_sign(a), 0, b->exp);
        mpd_qfinalize(result, ctx, status);
        return;
    }


    expdiff = a->exp - b->exp;
    if (a->digits + expdiff > ctx->prec) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    if (expdiff >= 0) {
        shift = expdiff;
        if (!mpd_qshiftl(result, a, shift, status)) {
            return;
        }
        result->exp = b_exp;
    }
    else {
        /* At this point expdiff < 0 and a->digits+expdiff <= prec,
         * so the shift before an increment will fit in prec. */
        shift = -expdiff;
        rnd = mpd_qshiftr(result, a, shift, status);
        if (rnd == MPD_UINT_MAX) {
            return;
        }
        result->exp = b_exp;
        if (!_mpd_apply_round_fit(result, rnd, ctx, status)) {
            return;
        }
        workstatus |= MPD_Rounded;
        if (rnd) {
            workstatus |= MPD_Inexact;
        }
    }

    if (mpd_adjexp(result) > ctx->emax ||
        mpd_adjexp(result) < mpd_etiny(ctx)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    *status |= workstatus;
    mpd_qfinalize(result, ctx, status);
}

void
mpd_qreduce(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
            uint32_t *status)
{
    mpd_ssize_t shift, maxexp, maxshift;
    uint8_t sign_a = mpd_sign(a);

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        mpd_qcopy(result, a, status);
        return;
    }

    if (!mpd_qcopy(result, a, status)) {
        return;
    }
    mpd_qfinalize(result, ctx, status);
    if (mpd_isspecial(result)) {
        return;
    }
    if (mpd_iszero(result)) {
        _settriple(result, sign_a, 0, 0);
        return;
    }

    shift = mpd_trail_zeros(result);
    maxexp = (ctx->clamp) ? mpd_etop(ctx) : ctx->emax;
    /* After the finalizing above result->exp <= maxexp. */
    maxshift = maxexp - result->exp;
    shift = (shift > maxshift) ? maxshift : shift;

    mpd_qshiftr_inplace(result, shift);
    result->exp += shift;
}

void
mpd_qrem(mpd_t *r, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx,
         uint32_t *status)
{
    MPD_NEW_STATIC(q,0,0,0,0);

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(r, a, b, ctx, status)) {
            return;
        }
        if (mpd_isinfinite(a)) {
            mpd_seterror(r, MPD_Invalid_operation, status);
            return;
        }
        if (mpd_isinfinite(b)) {
            mpd_qcopy(r, a, status);
            mpd_qfinalize(r, ctx, status);
            return;
        }
        /* debug */
        abort(); /* GCOV_NOT_REACHED */
    }
    if (mpd_iszerocoeff(b)) {
        if (mpd_iszerocoeff(a)) {
            mpd_seterror(r, MPD_Division_undefined, status);
        }
        else {
            mpd_seterror(r, MPD_Invalid_operation, status);
        }
        return;
    }

    _mpd_qdivmod(&q, r, a, b, ctx, status);
    mpd_del(&q);
    mpd_qfinalize(r, ctx, status);
}

void
mpd_qrem_near(mpd_t *r, const mpd_t *a, const mpd_t *b,
              const mpd_context_t *ctx, uint32_t *status)
{
    mpd_context_t workctx;
    MPD_NEW_STATIC(btmp,0,0,0,0);
    MPD_NEW_STATIC(q,0,0,0,0);
    mpd_ssize_t expdiff, qdigits;
    int cmp, isodd, allnine;

    assert(r != NULL); /* annotation for scan-build */

    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        if (mpd_qcheck_nans(r, a, b, ctx, status)) {
            return;
        }
        if (mpd_isinfinite(a)) {
            mpd_seterror(r, MPD_Invalid_operation, status);
            return;
        }
        if (mpd_isinfinite(b)) {
            mpd_qcopy(r, a, status);
            mpd_qfinalize(r, ctx, status);
            return;
        }
        /* debug */
        abort(); /* GCOV_NOT_REACHED */
    }
    if (mpd_iszerocoeff(b)) {
        if (mpd_iszerocoeff(a)) {
            mpd_seterror(r,  MPD_Division_undefined, status);
        }
        else {
            mpd_seterror(r,  MPD_Invalid_operation, status);
        }
        return;
    }

    if (r == b) {
        if (!mpd_qcopy(&btmp, b, status)) {
            mpd_seterror(r, MPD_Malloc_error, status);
            return;
        }
        b = &btmp;
    }

    _mpd_qdivmod(&q, r, a, b, ctx, status);
    if (mpd_isnan(&q) || mpd_isnan(r)) {
        goto finish;
    }
    if (mpd_iszerocoeff(r)) {
        goto finish;
    }

    expdiff = mpd_adjexp(b) - mpd_adjexp(r);
    if (-1 <= expdiff && expdiff <= 1) {

        allnine = mpd_coeff_isallnine(&q);
        qdigits = q.digits;
        isodd = mpd_isodd(&q);

        mpd_maxcontext(&workctx);
        if (mpd_sign(a) == mpd_sign(b)) {
            /* sign(r) == sign(b) */
            _mpd_qsub(&q, r, b, &workctx, &workctx.status);
        }
        else {
            /* sign(r) != sign(b) */
            _mpd_qadd(&q, r, b, &workctx, &workctx.status);
        }

        if (workctx.status&MPD_Errors) {
            mpd_seterror(r, workctx.status&MPD_Errors, status);
            goto finish;
        }

        cmp = _mpd_cmp_abs(&q, r);
        if (cmp < 0 || (cmp == 0 && isodd)) {
            /* abs(r) > abs(b)/2 or abs(r) == abs(b)/2 and isodd(quotient) */
            if (allnine && qdigits == ctx->prec) {
                /* abs(quotient) + 1 == 10**prec */
                mpd_seterror(r, MPD_Division_impossible, status);
                goto finish;
            }
            mpd_qcopy(r, &q, status);
        }
    }


finish:
    mpd_del(&btmp);
    mpd_del(&q);
    mpd_qfinalize(r, ctx, status);
}

static void
_mpd_qrescale(mpd_t *result, const mpd_t *a, mpd_ssize_t exp,
              const mpd_context_t *ctx, uint32_t *status)
{
    mpd_ssize_t expdiff, shift;
    mpd_uint_t rnd;

    if (mpd_isspecial(a)) {
        mpd_qcopy(result, a, status);
        return;
    }

    if (mpd_iszero(a)) {
        _settriple(result, mpd_sign(a), 0, exp);
        return;
    }

    expdiff = a->exp - exp;
    if (expdiff >= 0) {
        shift = expdiff;
        if (a->digits + shift > MPD_MAX_PREC+1) {
            mpd_seterror(result, MPD_Invalid_operation, status);
            return;
        }
        if (!mpd_qshiftl(result, a, shift, status)) {
            return;
        }
        result->exp = exp;
    }
    else {
        shift = -expdiff;
        rnd = mpd_qshiftr(result, a, shift, status);
        if (rnd == MPD_UINT_MAX) {
            return;
        }
        result->exp = exp;
        _mpd_apply_round_excess(result, rnd, ctx, status);
        *status |= MPD_Rounded;
        if (rnd) {
            *status |= MPD_Inexact;
        }
    }

    if (mpd_issubnormal(result, ctx)) {
        *status |= MPD_Subnormal;
    }
}

/*
 * Rescale a number so that it has exponent 'exp'. Does not regard context
 * precision, emax, emin, but uses the rounding mode. Special numbers are
 * quietly copied. Restrictions:
 *
 *     MPD_MIN_ETINY <= exp <= MPD_MAX_EMAX+1
 *     result->digits <= MPD_MAX_PREC+1
 */
void
mpd_qrescale(mpd_t *result, const mpd_t *a, mpd_ssize_t exp,
             const mpd_context_t *ctx, uint32_t *status)
{
    if (exp > MPD_MAX_EMAX+1 || exp < MPD_MIN_ETINY) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    _mpd_qrescale(result, a, exp, ctx, status);
}

/*
 * Same as mpd_qrescale, but with relaxed restrictions. The result of this
 * function should only be used for formatting a number and never as input
 * for other operations.
 *
 *     MPD_MIN_ETINY-MPD_MAX_PREC <= exp <= MPD_MAX_EMAX+1
 *     result->digits <= MPD_MAX_PREC+1
 */
void
mpd_qrescale_fmt(mpd_t *result, const mpd_t *a, mpd_ssize_t exp,
                 const mpd_context_t *ctx, uint32_t *status)
{
    if (exp > MPD_MAX_EMAX+1 || exp < MPD_MIN_ETINY-MPD_MAX_PREC) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    _mpd_qrescale(result, a, exp, ctx, status);
}

/* Round to an integer according to 'action' and ctx->round. */
enum {TO_INT_EXACT, TO_INT_SILENT, TO_INT_TRUNC};
static void
_mpd_qround_to_integral(int action, mpd_t *result, const mpd_t *a,
                        const mpd_context_t *ctx, uint32_t *status)
{
    mpd_uint_t rnd;

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        mpd_qcopy(result, a, status);
        return;
    }
    if (a->exp >= 0) {
        mpd_qcopy(result, a, status);
        return;
    }
    if (mpd_iszerocoeff(a)) {
        _settriple(result, mpd_sign(a), 0, 0);
        return;
    }

    rnd = mpd_qshiftr(result, a, -a->exp, status);
    if (rnd == MPD_UINT_MAX) {
        return;
    }
    result->exp = 0;

    if (action == TO_INT_EXACT || action == TO_INT_SILENT) {
        _mpd_apply_round_excess(result, rnd, ctx, status);
        if (action == TO_INT_EXACT) {
            *status |= MPD_Rounded;
            if (rnd) {
                *status |= MPD_Inexact;
            }
        }
    }
}

void
mpd_qround_to_intx(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
                   uint32_t *status)
{
    (void)_mpd_qround_to_integral(TO_INT_EXACT, result, a, ctx, status);
}

void
mpd_qround_to_int(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
                  uint32_t *status)
{
    (void)_mpd_qround_to_integral(TO_INT_SILENT, result, a, ctx, status);
}

void
mpd_qtrunc(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
           uint32_t *status)
{
    if (mpd_isspecial(a)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    (void)_mpd_qround_to_integral(TO_INT_TRUNC, result, a, ctx, status);
}

void
mpd_qfloor(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
           uint32_t *status)
{
    mpd_context_t workctx = *ctx;

    if (mpd_isspecial(a)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    workctx.round = MPD_ROUND_FLOOR;
    (void)_mpd_qround_to_integral(TO_INT_SILENT, result, a,
                                  &workctx, status);
}

void
mpd_qceil(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
          uint32_t *status)
{
    mpd_context_t workctx = *ctx;

    if (mpd_isspecial(a)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    workctx.round = MPD_ROUND_CEILING;
    (void)_mpd_qround_to_integral(TO_INT_SILENT, result, a,
                                  &workctx, status);
}

int
mpd_same_quantum(const mpd_t *a, const mpd_t *b)
{
    if (mpd_isspecial(a) || mpd_isspecial(b)) {
        return ((mpd_isnan(a) && mpd_isnan(b)) ||
                (mpd_isinfinite(a) && mpd_isinfinite(b)));
    }

    return a->exp == b->exp;
}

/* Schedule the increase in precision for the Newton iteration. */
static inline int
recpr_schedule_prec(mpd_ssize_t klist[MPD_MAX_PREC_LOG2],
                    mpd_ssize_t maxprec, mpd_ssize_t initprec)
{
    mpd_ssize_t k;
    int i;

    assert(maxprec > 0 && initprec > 0);
    if (maxprec <= initprec) return -1;

    i = 0; k = maxprec;
    do {
        k = (k+1) / 2;
        klist[i++] = k;
    } while (k > initprec);

    return i-1;
}

/*
 * Initial approximation for the reciprocal:
 *    k_0 := MPD_RDIGITS-2
 *    z_0 := 10**(-k_0) * floor(10**(2*k_0 + 2) / floor(v * 10**(k_0 + 2)))
 * Absolute error:
 *    |1/v - z_0| < 10**(-k_0)
 * ACL2 proof: maxerror-inverse-approx
 */
static void
_mpd_qreciprocal_approx(mpd_t *z, const mpd_t *v, uint32_t *status)
{
    mpd_uint_t p10data[2] = {0, mpd_pow10[MPD_RDIGITS-2]};
    mpd_uint_t dummy, word;
    int n;

    assert(v->exp == -v->digits);

    _mpd_get_msdigits(&dummy, &word, v, MPD_RDIGITS);
    n = mpd_word_digits(word);
    word *= mpd_pow10[MPD_RDIGITS-n];

    mpd_qresize(z, 2, status);
    (void)_mpd_shortdiv(z->data, p10data, 2, word);

    mpd_clear_flags(z);
    z->exp = -(MPD_RDIGITS-2);
    z->len = (z->data[1] == 0) ? 1 : 2;
    mpd_setdigits(z);
}

/*
 * Reciprocal, calculated with Newton's Method. Assumption: result != a.
 * NOTE: The comments in the function show that certain operations are
 * exact. The proof for the maximum error is too long to fit in here.
 * ACL2 proof: maxerror-inverse-complete
 */
static void
_mpd_qreciprocal(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
                 uint32_t *status)
{
    mpd_context_t varcontext, maxcontext;
    mpd_t *z = result;         /* current approximation */
    mpd_t *v;                  /* a, normalized to a number between 0.1 and 1 */
    MPD_NEW_SHARED(vtmp, a);   /* v shares data with a */
    MPD_NEW_STATIC(s,0,0,0,0); /* temporary variable */
    MPD_NEW_STATIC(t,0,0,0,0); /* temporary variable */
    MPD_NEW_CONST(two,0,0,1,1,1,2); /* const 2 */
    mpd_ssize_t klist[MPD_MAX_PREC_LOG2];
    mpd_ssize_t adj, maxprec, initprec;
    uint8_t sign = mpd_sign(a);
    int i;

    assert(result != a);

    v = &vtmp;
    mpd_clear_flags(v);
    adj = v->digits + v->exp;
    v->exp = -v->digits;

    /* Initial approximation */
    _mpd_qreciprocal_approx(z, v, status);

    mpd_maxcontext(&varcontext);
    mpd_maxcontext(&maxcontext);
    varcontext.round = maxcontext.round = MPD_ROUND_TRUNC;
    varcontext.emax = maxcontext.emax = MPD_MAX_EMAX + 100;
    varcontext.emin = maxcontext.emin = MPD_MIN_EMIN - 100;
    maxcontext.prec = MPD_MAX_PREC + 100;

    maxprec = ctx->prec;
    maxprec += 2;
    initprec = MPD_RDIGITS-3;

    i = recpr_schedule_prec(klist, maxprec, initprec);
    for (; i >= 0; i--) {
         /* Loop invariant: z->digits <= klist[i]+7 */
         /* Let s := z**2, exact result */
        _mpd_qmul_exact(&s, z, z, &maxcontext, status);
        varcontext.prec = 2*klist[i] + 5;
        if (v->digits > varcontext.prec) {
            /* Let t := v, truncated to n >= 2*k+5 fraction digits */
            mpd_qshiftr(&t, v, v->digits-varcontext.prec, status);
            t.exp = -varcontext.prec;
            /* Let t := trunc(v)*s, truncated to n >= 2*k+1 fraction digits */
            mpd_qmul(&t, &t, &s, &varcontext, status);
        }
        else { /* v->digits <= 2*k+5 */
            /* Let t := v*s, truncated to n >= 2*k+1 fraction digits */
            mpd_qmul(&t, v, &s, &varcontext, status);
        }
        /* Let s := 2*z, exact result */
        _mpd_qmul_exact(&s, z, &two, &maxcontext, status);
        /* s.digits < t.digits <= 2*k+5, |adjexp(s)-adjexp(t)| <= 1,
         * so the subtraction generates at most 2*k+6 <= klist[i+1]+7
         * digits. The loop invariant is preserved. */
        _mpd_qsub_exact(z, &s, &t, &maxcontext, status);
    }

    if (!mpd_isspecial(z)) {
        z->exp -= adj;
        mpd_set_flags(z, sign);
    }

    mpd_del(&s);
    mpd_del(&t);
    mpd_qfinalize(z, ctx, status);
}

/*
 * Internal function for large numbers:
 *
 *     q, r = divmod(coeff(a), coeff(b))
 *
 * Strategy: Multiply the dividend by the reciprocal of the divisor. The
 * inexact result is fixed by a small loop, using at most one iteration.
 *
 * ACL2 proofs:
 * ------------
 *    1) q is a natural number.  (ndivmod-quotient-natp)
 *    2) r is a natural number.  (ndivmod-remainder-natp)
 *    3) a = q * b + r           (ndivmod-q*b+r==a)
 *    4) r < b                   (ndivmod-remainder-<-b)
 */
static void
_mpd_base_ndivmod(mpd_t *q, mpd_t *r, const mpd_t *a, const mpd_t *b,
                  uint32_t *status)
{
    mpd_context_t workctx;
    mpd_t *qq = q, *rr = r;
    mpd_t aa, bb;
    int k;

    _mpd_copy_shared(&aa, a);
    _mpd_copy_shared(&bb, b);

    mpd_set_positive(&aa);
    mpd_set_positive(&bb);
    aa.exp = 0;
    bb.exp = 0;

    if (q == a || q == b) {
        if ((qq = mpd_qnew()) == NULL) {
            *status |= MPD_Malloc_error;
            goto nanresult;
        }
    }
    if (r == a || r == b) {
        if ((rr = mpd_qnew()) == NULL) {
            *status |= MPD_Malloc_error;
            goto nanresult;
        }
    }

    mpd_maxcontext(&workctx);

    /* Let prec := adigits - bdigits + 4 */
    workctx.prec = a->digits - b->digits + 1 + 3;
    if (a->digits > MPD_MAX_PREC || workctx.prec > MPD_MAX_PREC) {
        *status |= MPD_Division_impossible;
        goto nanresult;
    }

    /* Let x := _mpd_qreciprocal(b, prec)
     * Then x is bounded by:
     *    1) 1/b - 10**(-prec - bdigits) < x < 1/b + 10**(-prec - bdigits)
     *    2) 1/b - 10**(-adigits - 4) < x < 1/b + 10**(-adigits - 4)
     */
    _mpd_qreciprocal(rr, &bb, &workctx, &workctx.status);

    /* Get an estimate for the quotient. Let q := a * x
     * Then q is bounded by:
     *    3) a/b - 10**-4 < q < a/b + 10**-4
     */
    _mpd_qmul(qq, &aa, rr, &workctx, &workctx.status);
    /* Truncate q to an integer:
     *    4) a/b - 2 < trunc(q) < a/b + 1
     */
    mpd_qtrunc(qq, qq, &workctx, &workctx.status);

    workctx.prec = aa.digits + 3;
    workctx.emax = MPD_MAX_EMAX + 3;
    workctx.emin = MPD_MIN_EMIN - 3;
    /* Multiply the estimate for q by b:
     *    5) a - 2 * b < trunc(q) * b < a + b
     */
    _mpd_qmul(rr, &bb, qq, &workctx, &workctx.status);
    /* Get the estimate for r such that a = q * b + r. */
    _mpd_qsub_exact(rr, &aa, rr, &workctx, &workctx.status);

    /* Fix the result. At this point -b < r < 2*b, so the correction loop
       takes at most one iteration. */
    for (k = 0;; k++) {
        if (mpd_isspecial(qq) || mpd_isspecial(rr)) {
            *status |= (workctx.status&MPD_Errors);
            goto nanresult;
        }
        if (k > 2) { /* Allow two iterations despite the proof. */
            mpd_err_warn("libmpdec: internal error in "       /* GCOV_NOT_REACHED */
                         "_mpd_base_ndivmod: please report"); /* GCOV_NOT_REACHED */
            *status |= MPD_Invalid_operation;                 /* GCOV_NOT_REACHED */
            goto nanresult;                                   /* GCOV_NOT_REACHED */
        }
        /* r < 0 */
        else if (_mpd_cmp(&zero, rr) == 1) {
            _mpd_qadd_exact(rr, rr, &bb, &workctx, &workctx.status);
            _mpd_qadd_exact(qq, qq, &minus_one, &workctx, &workctx.status);
        }
        /* 0 <= r < b */
        else if (_mpd_cmp(rr, &bb) == -1) {
            break;
        }
        /* r >= b */
        else {
            _mpd_qsub_exact(rr, rr, &bb, &workctx, &workctx.status);
            _mpd_qadd_exact(qq, qq, &one, &workctx, &workctx.status);
        }
    }

    if (qq != q) {
        if (!mpd_qcopy(q, qq, status)) {
            goto nanresult; /* GCOV_UNLIKELY */
        }
        mpd_del(qq);
    }
    if (rr != r) {
        if (!mpd_qcopy(r, rr, status)) {
            goto nanresult; /* GCOV_UNLIKELY */
        }
        mpd_del(rr);
    }

    *status |= (workctx.status&MPD_Errors);
    return;


nanresult:
    if (qq && qq != q) mpd_del(qq);
    if (rr && rr != r) mpd_del(rr);
    mpd_setspecial(q, MPD_POS, MPD_NAN);
    mpd_setspecial(r, MPD_POS, MPD_NAN);
}

/* LIBMPDEC_ONLY */
/*
 * Schedule the optimal precision increase for the Newton iteration.
 *   v := input operand
 *   z_0 := initial approximation
 *   initprec := natural number such that abs(sqrt(v) - z_0) < 10**-initprec
 *   maxprec := target precision
 *
 * For convenience the output klist contains the elements in reverse order:
 *   klist := [k_n-1, ..., k_0], where
 *     1) k_0 <= initprec and
 *     2) abs(sqrt(v) - result) < 10**(-2*k_n-1 + 2) <= 10**-maxprec.
 */
static inline int
invroot_schedule_prec(mpd_ssize_t klist[MPD_MAX_PREC_LOG2],
                      mpd_ssize_t maxprec, mpd_ssize_t initprec)
{
    mpd_ssize_t k;
    int i;

    assert(maxprec >= 3 && initprec >= 3);
    if (maxprec <= initprec) return -1;

    i = 0; k = maxprec;
    do {
        k = (k+3) / 2;
        klist[i++] = k;
    } while (k > initprec);

    return i-1;
}

/*
 * Initial approximation for the inverse square root function.
 *   Input:
 *     v := rational number, with 1 <= v < 100
 *     vhat := floor(v * 10**6)
 *   Output:
 *     z := approximation to 1/sqrt(v), such that abs(z - 1/sqrt(v)) < 10**-3.
 */
static inline void
_invroot_init_approx(mpd_t *z, mpd_uint_t vhat)
{
    mpd_uint_t lo = 1000;
    mpd_uint_t hi = 10000;
    mpd_uint_t a, sq;

    assert(lo*lo <= vhat && vhat < (hi+1)*(hi+1));

    for(;;) {
        a = (lo + hi) / 2;
        sq = a * a;
        if (vhat >= sq) {
            if (vhat < sq + 2*a + 1) {
                break;
            }
            lo = a + 1;
        }
        else {
            hi = a - 1;
        }
    }

    /*
     * After the binary search we have:
     *  1) a**2 <= floor(v * 10**6) < (a + 1)**2
     * This implies:
     *  2) a**2 <= v * 10**6 < (a + 1)**2
     *  3) a <= sqrt(v) * 10**3 < a + 1
     * Since 10**3 <= a:
     *  4) 0 <= 10**prec/a - 1/sqrt(v) < 10**-prec
     * We have:
     *  5) 10**3/a - 10**-3 < floor(10**9/a) * 10**-6 <= 10**3/a
     * Merging 4) and 5):
     *  6) abs(floor(10**9/a) * 10**-6 - 1/sqrt(v)) < 10**-3
     */
    mpd_minalloc(z);
    mpd_clear_flags(z);
    z->data[0] = 1000000000UL / a;
    z->len = 1;
    z->exp = -6;
    mpd_setdigits(z);
}

/*
 * Set 'result' to 1/sqrt(a).
 *   Relative error: abs(result - 1/sqrt(a)) < 10**-prec * 1/sqrt(a)
 */
static void
_mpd_qinvroot(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
              uint32_t *status)
{
    uint32_t workstatus = 0;
    mpd_context_t varcontext, maxcontext;
    mpd_t *z = result;         /* current approximation */
    mpd_t *v;                  /* a, normalized to a number between 1 and 100 */
    MPD_NEW_SHARED(vtmp, a);   /* by default v will share data with a */
    MPD_NEW_STATIC(s,0,0,0,0); /* temporary variable */
    MPD_NEW_STATIC(t,0,0,0,0); /* temporary variable */
    MPD_NEW_CONST(one_half,0,-1,1,1,1,5);
    MPD_NEW_CONST(three,0,0,1,1,1,3);
    mpd_ssize_t klist[MPD_MAX_PREC_LOG2];
    mpd_ssize_t ideal_exp, shift;
    mpd_ssize_t adj, tz;
    mpd_ssize_t maxprec, fracdigits;
    mpd_uint_t vhat, dummy;
    int i, n;


    ideal_exp = -(a->exp - (a->exp & 1)) / 2;

    v = &vtmp;
    if (result == a) {
        if ((v = mpd_qncopy(a)) == NULL) {
            mpd_seterror(result, MPD_Malloc_error, status);
            return;
        }
    }

    /* normalize a to 1 <= v < 100 */
    if ((v->digits+v->exp) & 1) {
        fracdigits = v->digits - 1;
        v->exp = -fracdigits;
        n = (v->digits > 7) ? 7 : (int)v->digits;
        /* Let vhat := floor(v * 10**(2*initprec)) */
        _mpd_get_msdigits(&dummy, &vhat, v, n);
        if (n < 7) {
            vhat *= mpd_pow10[7-n];
        }
    }
    else {
        fracdigits = v->digits - 2;
        v->exp = -fracdigits;
        n = (v->digits > 8) ? 8 : (int)v->digits;
        /* Let vhat := floor(v * 10**(2*initprec)) */
        _mpd_get_msdigits(&dummy, &vhat, v, n);
        if (n < 8) {
            vhat *= mpd_pow10[8-n];
        }
    }
    adj = (a->exp-v->exp) / 2;

    /* initial approximation */
    _invroot_init_approx(z, vhat);

    mpd_maxcontext(&maxcontext);
    mpd_maxcontext(&varcontext);
    varcontext.round = MPD_ROUND_TRUNC;
    maxprec = ctx->prec + 1;

    /* initprec == 3 */
    i = invroot_schedule_prec(klist, maxprec, 3);
    for (; i >= 0; i--) {
        varcontext.prec = 2*klist[i]+2;
        mpd_qmul(&s, z, z, &maxcontext, &workstatus);
        if (v->digits > varcontext.prec) {
            shift = v->digits - varcontext.prec;
            mpd_qshiftr(&t, v, shift, &workstatus);
            t.exp += shift;
            mpd_qmul(&t, &t, &s, &varcontext, &workstatus);
        }
        else {
            mpd_qmul(&t, v, &s, &varcontext, &workstatus);
        }
        mpd_qsub(&t, &three, &t, &maxcontext, &workstatus);
        mpd_qmul(z, z, &t, &varcontext, &workstatus);
        mpd_qmul(z, z, &one_half, &maxcontext, &workstatus);
    }

    z->exp -= adj;

    tz = mpd_trail_zeros(result);
    shift = ideal_exp - result->exp;
    shift = (tz > shift) ? shift : tz;
    if (shift > 0) {
        mpd_qshiftr_inplace(result, shift);
        result->exp += shift;
    }


    mpd_del(&s);
    mpd_del(&t);
    if (v != &vtmp) mpd_del(v);
    *status |= (workstatus&MPD_Errors);
    *status |= (MPD_Rounded|MPD_Inexact);
}

void
mpd_qinvroot(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
             uint32_t *status)
{
    mpd_context_t workctx;

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        if (mpd_isnegative(a)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
            return;
        }
        /* positive infinity */
        _settriple(result, MPD_POS, 0, mpd_etiny(ctx));
        *status |= MPD_Clamped;
        return;
    }
    if (mpd_iszero(a)) {
        mpd_setspecial(result, mpd_sign(a), MPD_INF);
        *status |= MPD_Division_by_zero;
        return;
    }
    if (mpd_isnegative(a)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    workctx = *ctx;
    workctx.prec += 2;
    workctx.round = MPD_ROUND_HALF_EVEN;
    _mpd_qinvroot(result, a, &workctx, status);
    mpd_qfinalize(result, ctx, status);
}
/* END LIBMPDEC_ONLY */

/* Algorithm from decimal.py */
static void
_mpd_qsqrt(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
           uint32_t *status)
{
    mpd_context_t maxcontext;
    MPD_NEW_STATIC(c,0,0,0,0);
    MPD_NEW_STATIC(q,0,0,0,0);
    MPD_NEW_STATIC(r,0,0,0,0);
    MPD_NEW_CONST(two,0,0,1,1,1,2);
    mpd_ssize_t prec, ideal_exp;
    mpd_ssize_t l, shift;
    int exact = 0;


    ideal_exp = (a->exp - (a->exp & 1)) / 2;

    if (mpd_isspecial(a)) {
        if (mpd_qcheck_nan(result, a, ctx, status)) {
            return;
        }
        if (mpd_isnegative(a)) {
            mpd_seterror(result, MPD_Invalid_operation, status);
            return;
        }
        mpd_setspecial(result, MPD_POS, MPD_INF);
        return;
    }
    if (mpd_iszero(a)) {
        _settriple(result, mpd_sign(a), 0, ideal_exp);
        mpd_qfinalize(result, ctx, status);
        return;
    }
    if (mpd_isnegative(a)) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    mpd_maxcontext(&maxcontext);
    prec = ctx->prec + 1;

    if (!mpd_qcopy(&c, a, status)) {
        goto malloc_error;
    }
    c.exp = 0;

    if (a->exp & 1) {
        if (!mpd_qshiftl(&c, &c, 1, status)) {
            goto malloc_error;
        }
        l = (a->digits >> 1) + 1;
    }
    else {
        l = (a->digits + 1) >> 1;
    }

    shift = prec - l;
    if (shift >= 0) {
        if (!mpd_qshiftl(&c, &c, 2*shift, status)) {
            goto malloc_error;
        }
        exact = 1;
    }
    else {
        exact = !mpd_qshiftr_inplace(&c, -2*shift);
    }

    ideal_exp -= shift;

    /* find result = floor(sqrt(c)) using Newton's method */
    if (!mpd_qshiftl(result, &one, prec, status)) {
        goto malloc_error;
    }

    while (1) {
        _mpd_qdivmod(&q, &r, &c, result, &maxcontext, &maxcontext.status);
        if (mpd_isspecial(result) || mpd_isspecial(&q)) {
            mpd_seterror(result, maxcontext.status&MPD_Errors, status);
            goto out;
        }
        if (_mpd_cmp(result, &q) <= 0) {
            break;
        }
        _mpd_qadd_exact(result, result, &q, &maxcontext, &maxcontext.status);
        if (mpd_isspecial(result)) {
            mpd_seterror(result, maxcontext.status&MPD_Errors, status);
            goto out;
        }
        _mpd_qdivmod(result, &r, result, &two, &maxcontext, &maxcontext.status);
    }

    if (exact) {
        _mpd_qmul_exact(&r, result, result, &maxcontext, &maxcontext.status);
        if (mpd_isspecial(&r)) {
            mpd_seterror(result, maxcontext.status&MPD_Errors, status);
            goto out;
        }
        exact = (_mpd_cmp(&r, &c) == 0);
    }

    if (exact) {
        if (shift >= 0) {
            mpd_qshiftr_inplace(result, shift);
        }
        else {
            if (!mpd_qshiftl(result, result, -shift, status)) {
                goto malloc_error;
            }
        }
        ideal_exp += shift;
    }
    else {
        int lsd = (int)mpd_lsd(result->data[0]);
        if (lsd == 0 || lsd == 5) {
            result->data[0] += 1;
        }
    }

    result->exp = ideal_exp;


out:
    mpd_del(&c);
    mpd_del(&q);
    mpd_del(&r);
    maxcontext = *ctx;
    maxcontext.round = MPD_ROUND_HALF_EVEN;
    mpd_qfinalize(result, &maxcontext, status);
    return;

malloc_error:
    mpd_seterror(result, MPD_Malloc_error, status);
    goto out;
}

void
mpd_qsqrt(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx,
          uint32_t *status)
{
    MPD_NEW_STATIC(aa,0,0,0,0);
    uint32_t xstatus = 0;

    if (result == a) {
        if (!mpd_qcopy(&aa, a, status)) {
            mpd_seterror(result, MPD_Malloc_error, status);
            goto out;
        }
        a = &aa;
    }

    _mpd_qsqrt(result, a, ctx, &xstatus);

    if (xstatus & (MPD_Malloc_error|MPD_Division_impossible)) {
        /* The above conditions can occur at very high context precisions
         * if intermediate values get too large. Retry the operation with
         * a lower context precision in case the result is exact.
         *
         * If the result is exact, an upper bound for the number of digits
         * is the number of digits in the input.
         *
         * NOTE: sqrt(40e9) = 2.0e+5 /\ digits(40e9) = digits(2.0e+5) = 2
         */
        uint32_t ystatus = 0;
        mpd_context_t workctx = *ctx;

        workctx.prec = a->digits;
        if (workctx.prec >= ctx->prec) {
            *status |= (xstatus|MPD_Errors);
            goto out; /* No point in repeating this, keep the original error. */
        }

        _mpd_qsqrt(result, a, &workctx, &ystatus);
        if (ystatus != 0) {
            ystatus = *status | ((xstatus|ystatus)&MPD_Errors);
            mpd_seterror(result, ystatus, status);
        }
    }
    else {
        *status |= xstatus;
    }

out:
    mpd_del(&aa);
}


/******************************************************************************/
/*                              Base conversions                              */
/******************************************************************************/

/* Space needed to represent an integer mpd_t in base 'base'. */
size_t
mpd_sizeinbase(const mpd_t *a, uint32_t base)
{
    double x;
    size_t digits;
    double upper_bound;

    assert(mpd_isinteger(a));
    assert(base >= 2);

    if (mpd_iszero(a)) {
        return 1;
    }

    digits = a->digits+a->exp;
    assert(digits > 0);

#ifdef CONFIG_64
    /* ceil(2711437152599294 / log10(2)) + 4 == 2**53 */
    if (digits > 2711437152599294ULL) {
        return SIZE_MAX;
    }

    upper_bound = (double)((1ULL<<53)-1);
#else
    upper_bound = (double)(SIZE_MAX-1);
#endif

    x = (double)digits / log10(base);
    return (x > upper_bound) ? SIZE_MAX : (size_t)x + 1;
}

/* Space needed to import a base 'base' integer of length 'srclen'. */
static mpd_ssize_t
_mpd_importsize(size_t srclen, uint32_t base)
{
    double x;
    double upper_bound;

    assert(srclen > 0);
    assert(base >= 2);

#if SIZE_MAX == UINT64_MAX
    if (srclen > (1ULL<<53)) {
        return MPD_SSIZE_MAX;
    }

    assert((1ULL<<53) <= MPD_MAXIMPORT);
    upper_bound = (double)((1ULL<<53)-1);
#else
    upper_bound = MPD_MAXIMPORT-1;
#endif

    x = (double)srclen * (log10(base)/MPD_RDIGITS);
    return (x > upper_bound) ? MPD_SSIZE_MAX : (mpd_ssize_t)x + 1;
}

static uint8_t
mpd_resize_u16(uint16_t **w, size_t nmemb)
{
    uint8_t err = 0;
    *w = mpd_realloc(*w, nmemb, sizeof **w, &err);
    return !err;
}

static uint8_t
mpd_resize_u32(uint32_t **w, size_t nmemb)
{
    uint8_t err = 0;
    *w = mpd_realloc(*w, nmemb, sizeof **w, &err);
    return !err;
}

static size_t
_baseconv_to_u16(uint16_t **w, size_t wlen, mpd_uint_t wbase,
                 mpd_uint_t *u, mpd_ssize_t ulen)
{
    size_t n = 0;

    assert(wlen > 0 && ulen > 0);
    assert(wbase <= (1U<<16));

    do {
        if (n >= wlen) {
            if (!mpd_resize_u16(w, n+1)) {
                return SIZE_MAX;
            }
            wlen = n+1;
        }
        (*w)[n++] = (uint16_t)_mpd_shortdiv(u, u, ulen, wbase);
        /* ulen is at least 1. u[ulen-1] can only be zero if ulen == 1. */
        ulen = _mpd_real_size(u, ulen);

    } while (u[ulen-1] != 0);

    return n;
}

static size_t
_coeff_from_u16(mpd_t *w, mpd_ssize_t wlen,
                const mpd_uint_t *u, size_t ulen, uint32_t ubase,
                uint32_t *status)
{
    mpd_ssize_t n = 0;
    mpd_uint_t carry;

    assert(wlen > 0 && ulen > 0);
    assert(ubase <= (1U<<16));

    w->data[n++] = u[--ulen];
    while (--ulen != SIZE_MAX) {
        carry = _mpd_shortmul_c(w->data, w->data, n, ubase);
        if (carry) {
            if (n >= wlen) {
                if (!mpd_qresize(w, n+1, status)) {
                    return SIZE_MAX;
                }
                wlen = n+1;
            }
            w->data[n++] = carry;
        }
        carry = _mpd_shortadd(w->data, n, u[ulen]);
        if (carry) {
            if (n >= wlen) {
                if (!mpd_qresize(w, n+1, status)) {
                    return SIZE_MAX;
                }
                wlen = n+1;
            }
            w->data[n++] = carry;
        }
    }

    return n;
}

/* target base wbase < source base ubase */
static size_t
_baseconv_to_smaller(uint32_t **w, size_t wlen, uint32_t wbase,
                     mpd_uint_t *u, mpd_ssize_t ulen, mpd_uint_t ubase)
{
    size_t n = 0;

    assert(wlen > 0 && ulen > 0);
    assert(wbase < ubase);

    do {
        if (n >= wlen) {
            if (!mpd_resize_u32(w, n+1)) {
                return SIZE_MAX;
            }
            wlen = n+1;
        }
        (*w)[n++] = (uint32_t)_mpd_shortdiv_b(u, u, ulen, wbase, ubase);
        /* ulen is at least 1. u[ulen-1] can only be zero if ulen == 1. */
        ulen = _mpd_real_size(u, ulen);

    } while (u[ulen-1] != 0);

    return n;
}

#ifdef CONFIG_32
/* target base 'wbase' == source base 'ubase' */
static size_t
_copy_equal_base(uint32_t **w, size_t wlen,
                 const uint32_t *u, size_t ulen)
{
    if (wlen < ulen) {
        if (!mpd_resize_u32(w, ulen)) {
            return SIZE_MAX;
        }
    }

    memcpy(*w, u, ulen * (sizeof **w));
    return ulen;
}

/* target base 'wbase' > source base 'ubase' */
static size_t
_baseconv_to_larger(uint32_t **w, size_t wlen, mpd_uint_t wbase,
                    const mpd_uint_t *u, size_t ulen, mpd_uint_t ubase)
{
    size_t n = 0;
    mpd_uint_t carry;

    assert(wlen > 0 && ulen > 0);
    assert(ubase < wbase);

    (*w)[n++] = u[--ulen];
    while (--ulen != SIZE_MAX) {
        carry = _mpd_shortmul_b(*w, *w, n, ubase, wbase);
        if (carry) {
            if (n >= wlen) {
                if (!mpd_resize_u32(w, n+1)) {
                    return SIZE_MAX;
                }
                wlen = n+1;
            }
            (*w)[n++] = carry;
        }
        carry = _mpd_shortadd_b(*w, n, u[ulen], wbase);
        if (carry) {
            if (n >= wlen) {
                if (!mpd_resize_u32(w, n+1)) {
                    return SIZE_MAX;
                }
                wlen = n+1;
            }
            (*w)[n++] = carry;
        }
    }

    return n;
}

/* target base wbase < source base ubase */
static size_t
_coeff_from_larger_base(mpd_t *w, size_t wlen, mpd_uint_t wbase,
                        mpd_uint_t *u, mpd_ssize_t ulen, mpd_uint_t ubase,
                        uint32_t *status)
{
    size_t n = 0;

    assert(wlen > 0 && ulen > 0);
    assert(wbase < ubase);

    do {
        if (n >= wlen) {
            if (!mpd_qresize(w, n+1, status)) {
                return SIZE_MAX;
            }
            wlen = n+1;
        }
        w->data[n++] = (uint32_t)_mpd_shortdiv_b(u, u, ulen, wbase, ubase);
        /* ulen is at least 1. u[ulen-1] can only be zero if ulen == 1. */
        ulen = _mpd_real_size(u, ulen);

    } while (u[ulen-1] != 0);

    return n;
}
#endif

/* target base 'wbase' > source base 'ubase' */
static size_t
_coeff_from_smaller_base(mpd_t *w, mpd_ssize_t wlen, mpd_uint_t wbase,
                         const uint32_t *u, size_t ulen, mpd_uint_t ubase,
                         uint32_t *status)
{
    mpd_ssize_t n = 0;
    mpd_uint_t carry;

    assert(wlen > 0 && ulen > 0);
    assert(wbase > ubase);

    w->data[n++] = u[--ulen];
    while (--ulen != SIZE_MAX) {
        carry = _mpd_shortmul_b(w->data, w->data, n, ubase, wbase);
        if (carry) {
            if (n >= wlen) {
                if (!mpd_qresize(w, n+1, status)) {
                    return SIZE_MAX;
                }
                wlen = n+1;
            }
            w->data[n++] = carry;
        }
        carry = _mpd_shortadd_b(w->data, n, u[ulen], wbase);
        if (carry) {
            if (n >= wlen) {
                if (!mpd_qresize(w, n+1, status)) {
                    return SIZE_MAX;
                }
                wlen = n+1;
            }
            w->data[n++] = carry;
        }
    }

    return n;
}

/*
 * Convert an integer mpd_t to a multiprecision integer with base <= 2**16.
 * The least significant word of the result is (*rdata)[0].
 *
 * If rdata is NULL, space is allocated by the function and rlen is irrelevant.
 * In case of an error any allocated storage is freed and rdata is set back to
 * NULL.
 *
 * If rdata is non-NULL, it MUST be allocated by one of libmpdec's allocation
 * functions and rlen MUST be correct. If necessary, the function will resize
 * rdata. In case of an error the caller must free rdata.
 *
 * Return value: In case of success, the exact length of rdata, SIZE_MAX
 * otherwise.
 */
size_t
mpd_qexport_u16(uint16_t **rdata, size_t rlen, uint32_t rbase,
                const mpd_t *src, uint32_t *status)
{
    MPD_NEW_STATIC(tsrc,0,0,0,0);
    int alloc = 0; /* rdata == NULL */
    size_t n;

    assert(rbase <= (1U<<16));

    if (mpd_isspecial(src) || !_mpd_isint(src)) {
        *status |= MPD_Invalid_operation;
        return SIZE_MAX;
    }

    if (*rdata == NULL) {
        rlen = mpd_sizeinbase(src, rbase);
        if (rlen == SIZE_MAX) {
            *status |= MPD_Invalid_operation;
            return SIZE_MAX;
        }
        *rdata = mpd_alloc(rlen, sizeof **rdata);
        if (*rdata == NULL) {
            goto malloc_error;
        }
        alloc = 1;
    }

    if (mpd_iszero(src)) {
        **rdata = 0;
        return 1;
    }

    if (src->exp >= 0) {
        if (!mpd_qshiftl(&tsrc, src, src->exp, status)) {
            goto malloc_error;
        }
    }
    else {
        if (mpd_qshiftr(&tsrc, src, -src->exp, status) == MPD_UINT_MAX) {
            goto malloc_error;
        }
    }

    n = _baseconv_to_u16(rdata, rlen, rbase, tsrc.data, tsrc.len);
    if (n == SIZE_MAX) {
        goto malloc_error;
    }


out:
    mpd_del(&tsrc);
    return n;

malloc_error:
    if (alloc) {
        mpd_free(*rdata);
        *rdata = NULL;
    }
    n = SIZE_MAX;
    *status |= MPD_Malloc_error;
    goto out;
}

/*
 * Convert an integer mpd_t to a multiprecision integer with base<=UINT32_MAX.
 * The least significant word of the result is (*rdata)[0].
 *
 * If rdata is NULL, space is allocated by the function and rlen is irrelevant.
 * In case of an error any allocated storage is freed and rdata is set back to
 * NULL.
 *
 * If rdata is non-NULL, it MUST be allocated by one of libmpdec's allocation
 * functions and rlen MUST be correct. If necessary, the function will resize
 * rdata. In case of an error the caller must free rdata.
 *
 * Return value: In case of success, the exact length of rdata, SIZE_MAX
 * otherwise.
 */
size_t
mpd_qexport_u32(uint32_t **rdata, size_t rlen, uint32_t rbase,
                const mpd_t *src, uint32_t *status)
{
    MPD_NEW_STATIC(tsrc,0,0,0,0);
    int alloc = 0; /* rdata == NULL */
    size_t n;

    if (mpd_isspecial(src) || !_mpd_isint(src)) {
        *status |= MPD_Invalid_operation;
        return SIZE_MAX;
    }

    if (*rdata == NULL) {
        rlen = mpd_sizeinbase(src, rbase);
        if (rlen == SIZE_MAX) {
            *status |= MPD_Invalid_operation;
            return SIZE_MAX;
        }
        *rdata = mpd_alloc(rlen, sizeof **rdata);
        if (*rdata == NULL) {
            goto malloc_error;
        }
        alloc = 1;
    }

    if (mpd_iszero(src)) {
        **rdata = 0;
        return 1;
    }

    if (src->exp >= 0) {
        if (!mpd_qshiftl(&tsrc, src, src->exp, status)) {
            goto malloc_error;
        }
    }
    else {
        if (mpd_qshiftr(&tsrc, src, -src->exp, status) == MPD_UINT_MAX) {
            goto malloc_error;
        }
    }

#ifdef CONFIG_64
    n = _baseconv_to_smaller(rdata, rlen, rbase,
                             tsrc.data, tsrc.len, MPD_RADIX);
#else
    if (rbase == MPD_RADIX) {
        n = _copy_equal_base(rdata, rlen, tsrc.data, tsrc.len);
    }
    else if (rbase < MPD_RADIX) {
        n = _baseconv_to_smaller(rdata, rlen, rbase,
                                 tsrc.data, tsrc.len, MPD_RADIX);
    }
    else {
        n = _baseconv_to_larger(rdata, rlen, rbase,
                                tsrc.data, tsrc.len, MPD_RADIX);
    }
#endif

    if (n == SIZE_MAX) {
        goto malloc_error;
    }


out:
    mpd_del(&tsrc);
    return n;

malloc_error:
    if (alloc) {
        mpd_free(*rdata);
        *rdata = NULL;
    }
    n = SIZE_MAX;
    *status |= MPD_Malloc_error;
    goto out;
}


/*
 * Converts a multiprecision integer with base <= UINT16_MAX+1 to an mpd_t.
 * The least significant word of the source is srcdata[0].
 */
void
mpd_qimport_u16(mpd_t *result,
                const uint16_t *srcdata, size_t srclen,
                uint8_t srcsign, uint32_t srcbase,
                const mpd_context_t *ctx, uint32_t *status)
{
    mpd_uint_t *usrc; /* uint16_t src copied to an mpd_uint_t array */
    mpd_ssize_t rlen; /* length of the result */
    size_t n;

    assert(srclen > 0);
    assert(srcbase <= (1U<<16));

    rlen = _mpd_importsize(srclen, srcbase);
    if (rlen == MPD_SSIZE_MAX) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    usrc = mpd_alloc((mpd_size_t)srclen, sizeof *usrc);
    if (usrc == NULL) {
        mpd_seterror(result, MPD_Malloc_error, status);
        return;
    }
    for (n = 0; n < srclen; n++) {
        usrc[n] = srcdata[n];
    }

    if (!mpd_qresize(result, rlen, status)) {
        goto finish;
    }

    n = _coeff_from_u16(result, rlen, usrc, srclen, srcbase, status);
    if (n == SIZE_MAX) {
        goto finish;
    }

    mpd_set_flags(result, srcsign);
    result->exp = 0;
    result->len = n;
    mpd_setdigits(result);

    mpd_qresize(result, result->len, status);
    mpd_qfinalize(result, ctx, status);


finish:
    mpd_free(usrc);
}

/*
 * Converts a multiprecision integer with base <= UINT32_MAX to an mpd_t.
 * The least significant word of the source is srcdata[0].
 */
void
mpd_qimport_u32(mpd_t *result,
                const uint32_t *srcdata, size_t srclen,
                uint8_t srcsign, uint32_t srcbase,
                const mpd_context_t *ctx, uint32_t *status)
{
    mpd_ssize_t rlen; /* length of the result */
    size_t n;

    assert(srclen > 0);

    rlen = _mpd_importsize(srclen, srcbase);
    if (rlen == MPD_SSIZE_MAX) {
        mpd_seterror(result, MPD_Invalid_operation, status);
        return;
    }

    if (!mpd_qresize(result, rlen, status)) {
        return;
    }

#ifdef CONFIG_64
    n = _coeff_from_smaller_base(result, rlen, MPD_RADIX,
                                 srcdata, srclen, srcbase,
                                 status);
#else
    if (srcbase == MPD_RADIX) {
        if (!mpd_qresize(result, srclen, status)) {
            return;
        }
        memcpy(result->data, srcdata, srclen * (sizeof *srcdata));
        n = srclen;
    }
    else if (srcbase < MPD_RADIX) {
        n = _coeff_from_smaller_base(result, rlen, MPD_RADIX,
                                     srcdata, srclen, srcbase,
                                     status);
    }
    else {
        mpd_uint_t *usrc = mpd_alloc((mpd_size_t)srclen, sizeof *usrc);
        if (usrc == NULL) {
            mpd_seterror(result, MPD_Malloc_error, status);
            return;
        }
        for (n = 0; n < srclen; n++) {
            usrc[n] = srcdata[n];
        }

        n = _coeff_from_larger_base(result, rlen, MPD_RADIX,
                                    usrc, (mpd_ssize_t)srclen, srcbase,
                                    status);
        mpd_free(usrc);
    }
#endif

    if (n == SIZE_MAX) {
        return;
    }

    mpd_set_flags(result, srcsign);
    result->exp = 0;
    result->len = n;
    mpd_setdigits(result);

    mpd_qresize(result, result->len, status);
    mpd_qfinalize(result, ctx, status);
}
