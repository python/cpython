/*
 * Copyright (c) 2008-2012 Stefan Krah. All rights reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "constants.h"
#include "memory.h"
#include "typearith.h"
#include "basearith.h"


/*********************************************************************/
/*                   Calculations in base MPD_RADIX                  */
/*********************************************************************/


/*
 * Knuth, TAOCP, Volume 2, 4.3.1:
 *    w := sum of u (len m) and v (len n)
 *    n > 0 and m >= n
 * The calling function has to handle a possible final carry.
 */
mpd_uint_t
_mpd_baseadd(mpd_uint_t *w, const mpd_uint_t *u, const mpd_uint_t *v,
             mpd_size_t m, mpd_size_t n)
{
    mpd_uint_t s;
    mpd_uint_t carry = 0;
    mpd_size_t i;

    assert(n > 0 && m >= n);

    /* add n members of u and v */
    for (i = 0; i < n; i++) {
        s = u[i] + (v[i] + carry);
        carry = (s < u[i]) | (s >= MPD_RADIX);
        w[i] = carry ? s-MPD_RADIX : s;
    }
    /* if there is a carry, propagate it */
    for (; carry && i < m; i++) {
        s = u[i] + carry;
        carry = (s == MPD_RADIX);
        w[i] = carry ? 0 : s;
    }
    /* copy the rest of u */
    for (; i < m; i++) {
        w[i] = u[i];
    }

    return carry;
}

/*
 * Add the contents of u to w. Carries are propagated further. The caller
 * has to make sure that w is big enough.
 */
void
_mpd_baseaddto(mpd_uint_t *w, const mpd_uint_t *u, mpd_size_t n)
{
    mpd_uint_t s;
    mpd_uint_t carry = 0;
    mpd_size_t i;

    if (n == 0) return;

    /* add n members of u to w */
    for (i = 0; i < n; i++) {
        s = w[i] + (u[i] + carry);
        carry = (s < w[i]) | (s >= MPD_RADIX);
        w[i] = carry ? s-MPD_RADIX : s;
    }
    /* if there is a carry, propagate it */
    for (; carry; i++) {
        s = w[i] + carry;
        carry = (s == MPD_RADIX);
        w[i] = carry ? 0 : s;
    }
}

/*
 * Add v to w (len m). The calling function has to handle a possible
 * final carry. Assumption: m > 0.
 */
mpd_uint_t
_mpd_shortadd(mpd_uint_t *w, mpd_size_t m, mpd_uint_t v)
{
    mpd_uint_t s;
    mpd_uint_t carry;
    mpd_size_t i;

    assert(m > 0);

    /* add v to w */
    s = w[0] + v;
    carry = (s < v) | (s >= MPD_RADIX);
    w[0] = carry ? s-MPD_RADIX : s;

    /* if there is a carry, propagate it */
    for (i = 1; carry && i < m; i++) {
        s = w[i] + carry;
        carry = (s == MPD_RADIX);
        w[i] = carry ? 0 : s;
    }

    return carry;
}

/* Increment u. The calling function has to handle a possible carry. */
mpd_uint_t
_mpd_baseincr(mpd_uint_t *u, mpd_size_t n)
{
    mpd_uint_t s;
    mpd_uint_t carry = 1;
    mpd_size_t i;

    assert(n > 0);

    /* if there is a carry, propagate it */
    for (i = 0; carry && i < n; i++) {
        s = u[i] + carry;
        carry = (s == MPD_RADIX);
        u[i] = carry ? 0 : s;
    }

    return carry;
}

/*
 * Knuth, TAOCP, Volume 2, 4.3.1:
 *     w := difference of u (len m) and v (len n).
 *     number in u >= number in v;
 */
void
_mpd_basesub(mpd_uint_t *w, const mpd_uint_t *u, const mpd_uint_t *v,
             mpd_size_t m, mpd_size_t n)
{
    mpd_uint_t d;
    mpd_uint_t borrow = 0;
    mpd_size_t i;

    assert(m > 0 && n > 0);

    /* subtract n members of v from u */
    for (i = 0; i < n; i++) {
        d = u[i] - (v[i] + borrow);
        borrow = (u[i] < d);
        w[i] = borrow ? d + MPD_RADIX : d;
    }
    /* if there is a borrow, propagate it */
    for (; borrow && i < m; i++) {
        d = u[i] - borrow;
        borrow = (u[i] == 0);
        w[i] = borrow ? MPD_RADIX-1 : d;
    }
    /* copy the rest of u */
    for (; i < m; i++) {
        w[i] = u[i];
    }
}

/*
 * Subtract the contents of u from w. w is larger than u. Borrows are
 * propagated further, but eventually w can absorb the final borrow.
 */
void
_mpd_basesubfrom(mpd_uint_t *w, const mpd_uint_t *u, mpd_size_t n)
{
    mpd_uint_t d;
    mpd_uint_t borrow = 0;
    mpd_size_t i;

    if (n == 0) return;

    /* subtract n members of u from w */
    for (i = 0; i < n; i++) {
        d = w[i] - (u[i] + borrow);
        borrow = (w[i] < d);
        w[i] = borrow ? d + MPD_RADIX : d;
    }
    /* if there is a borrow, propagate it */
    for (; borrow; i++) {
        d = w[i] - borrow;
        borrow = (w[i] == 0);
        w[i] = borrow ? MPD_RADIX-1 : d;
    }
}

/* w := product of u (len n) and v (single word) */
void
_mpd_shortmul(mpd_uint_t *w, const mpd_uint_t *u, mpd_size_t n, mpd_uint_t v)
{
    mpd_uint_t hi, lo;
    mpd_uint_t carry = 0;
    mpd_size_t i;

    assert(n > 0);

    for (i=0; i < n; i++) {

        _mpd_mul_words(&hi, &lo, u[i], v);
        lo = carry + lo;
        if (lo < carry) hi++;

        _mpd_div_words_r(&carry, &w[i], hi, lo);
    }
    w[i] = carry;
}

/*
 * Knuth, TAOCP, Volume 2, 4.3.1:
 *     w := product of u (len m) and v (len n)
 *     w must be initialized to zero
 */
void
_mpd_basemul(mpd_uint_t *w, const mpd_uint_t *u, const mpd_uint_t *v,
             mpd_size_t m, mpd_size_t n)
{
    mpd_uint_t hi, lo;
    mpd_uint_t carry;
    mpd_size_t i, j;

    assert(m > 0 && n > 0);

    for (j=0; j < n; j++) {
        carry = 0;
        for (i=0; i < m; i++) {

            _mpd_mul_words(&hi, &lo, u[i], v[j]);
            lo = w[i+j] + lo;
            if (lo < w[i+j]) hi++;
            lo = carry + lo;
            if (lo < carry) hi++;

            _mpd_div_words_r(&carry, &w[i+j], hi, lo);
        }
        w[j+m] = carry;
    }
}

/*
 * Knuth, TAOCP Volume 2, 4.3.1, exercise 16:
 *     w := quotient of u (len n) divided by a single word v
 */
mpd_uint_t
_mpd_shortdiv(mpd_uint_t *w, const mpd_uint_t *u, mpd_size_t n, mpd_uint_t v)
{
    mpd_uint_t hi, lo;
    mpd_uint_t rem = 0;
    mpd_size_t i;

    assert(n > 0);

    for (i=n-1; i != MPD_SIZE_MAX; i--) {

        _mpd_mul_words(&hi, &lo, rem, MPD_RADIX);
        lo = u[i] + lo;
        if (lo < u[i]) hi++;

        _mpd_div_words(&w[i], &rem, hi, lo, v);
    }

    return rem;
}

/*
 * Knuth, TAOCP Volume 2, 4.3.1:
 *     q, r := quotient and remainder of uconst (len nplusm)
 *             divided by vconst (len n)
 *     nplusm >= n
 *
 * If r is not NULL, r will contain the remainder. If r is NULL, the
 * return value indicates if there is a remainder: 1 for true, 0 for
 * false.  A return value of -1 indicates an error.
 */
int
_mpd_basedivmod(mpd_uint_t *q, mpd_uint_t *r,
                const mpd_uint_t *uconst, const mpd_uint_t *vconst,
                mpd_size_t nplusm, mpd_size_t n)
{
    mpd_uint_t ustatic[MPD_MINALLOC_MAX];
    mpd_uint_t vstatic[MPD_MINALLOC_MAX];
    mpd_uint_t *u = ustatic;
    mpd_uint_t *v = vstatic;
    mpd_uint_t d, qhat, rhat, w2[2];
    mpd_uint_t hi, lo, x;
    mpd_uint_t carry;
    mpd_size_t i, j, m;
    int retval = 0;

    assert(n > 1 && nplusm >= n);
    m = sub_size_t(nplusm, n);

    /* D1: normalize */
    d = MPD_RADIX / (vconst[n-1] + 1);

    if (nplusm >= MPD_MINALLOC_MAX) {
        if ((u = mpd_alloc(nplusm+1, sizeof *u)) == NULL) {
            return -1;
        }
    }
    if (n >= MPD_MINALLOC_MAX) {
        if ((v = mpd_alloc(n+1, sizeof *v)) == NULL) {
            mpd_free(u);
            return -1;
        }
    }

    _mpd_shortmul(u, uconst, nplusm, d);
    _mpd_shortmul(v, vconst, n, d);

    /* D2: loop */
    for (j=m; j != MPD_SIZE_MAX; j--) {

        /* D3: calculate qhat and rhat */
        rhat = _mpd_shortdiv(w2, u+j+n-1, 2, v[n-1]);
        qhat = w2[1] * MPD_RADIX + w2[0];

        while (1) {
            if (qhat < MPD_RADIX) {
                _mpd_singlemul(w2, qhat, v[n-2]);
                if (w2[1] <= rhat) {
                    if (w2[1] != rhat || w2[0] <= u[j+n-2]) {
                        break;
                    }
                }
            }
            qhat -= 1;
            rhat += v[n-1];
            if (rhat < v[n-1] || rhat >= MPD_RADIX) {
                break;
            }
        }
        /* D4: multiply and subtract */
        carry = 0;
        for (i=0; i <= n; i++) {

            _mpd_mul_words(&hi, &lo, qhat, v[i]);

            lo = carry + lo;
            if (lo < carry) hi++;

            _mpd_div_words_r(&hi, &lo, hi, lo);

            x = u[i+j] - lo;
            carry = (u[i+j] < x);
            u[i+j] = carry ? x+MPD_RADIX : x;
            carry += hi;
        }
        q[j] = qhat;
        /* D5: test remainder */
        if (carry) {
            q[j] -= 1;
            /* D6: add back */
            (void)_mpd_baseadd(u+j, u+j, v, n+1, n);
        }
    }

    /* D8: unnormalize */
    if (r != NULL) {
        _mpd_shortdiv(r, u, n, d);
        /* we are not interested in the return value here */
        retval = 0;
    }
    else {
        retval = !_mpd_isallzero(u, n);
    }


if (u != ustatic) mpd_free(u);
if (v != vstatic) mpd_free(v);
return retval;
}

/*
 * Left shift of src by 'shift' digits; src may equal dest.
 *
 *  dest := area of n mpd_uint_t with space for srcdigits+shift digits.
 *  src  := coefficient with length m.
 *
 * The case splits in the function are non-obvious. The following
 * equations might help:
 *
 *  Let msdigits denote the number of digits in the most significant
 *  word of src. Then 1 <= msdigits <= rdigits.
 *
 *   1) shift = q * rdigits + r
 *   2) srcdigits = qsrc * rdigits + msdigits
 *   3) destdigits = shift + srcdigits
 *                 = q * rdigits + r + qsrc * rdigits + msdigits
 *                 = q * rdigits + (qsrc * rdigits + (r + msdigits))
 *
 *  The result has q zero words, followed by the coefficient that
 *  is left-shifted by r. The case r == 0 is trivial. For r > 0, it
 *  is important to keep in mind that we always read m source words,
 *  but write m+1 destination words if r + msdigits > rdigits, m words
 *  otherwise.
 */
void
_mpd_baseshiftl(mpd_uint_t *dest, mpd_uint_t *src, mpd_size_t n, mpd_size_t m,
                mpd_size_t shift)
{
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
    /* spurious uninitialized warnings */
    mpd_uint_t l=l, lprev=lprev, h=h;
#else
    mpd_uint_t l, lprev, h;
#endif
    mpd_uint_t q, r;
    mpd_uint_t ph;

    assert(m > 0 && n >= m);

    _mpd_div_word(&q, &r, (mpd_uint_t)shift, MPD_RDIGITS);

    if (r != 0) {

        ph = mpd_pow10[r];

        --m; --n;
        _mpd_divmod_pow10(&h, &lprev, src[m--], MPD_RDIGITS-r);
        if (h != 0) { /* r + msdigits > rdigits <==> h != 0 */
            dest[n--] = h;
        }
        /* write m-1 shifted words */
        for (; m != MPD_SIZE_MAX; m--,n--) {
            _mpd_divmod_pow10(&h, &l, src[m], MPD_RDIGITS-r);
            dest[n] = ph * lprev + h;
            lprev = l;
        }
        /* write least significant word */
        dest[q] = ph * lprev;
    }
    else {
        while (--m != MPD_SIZE_MAX) {
            dest[m+q] = src[m];
        }
    }

    mpd_uint_zero(dest, q);
}

/*
 * Right shift of src by 'shift' digits; src may equal dest.
 * Assumption: srcdigits-shift > 0.
 *
 *  dest := area with space for srcdigits-shift digits.
 *  src  := coefficient with length 'slen'.
 *
 * The case splits in the function rely on the following equations:
 *
 *  Let msdigits denote the number of digits in the most significant
 *  word of src. Then 1 <= msdigits <= rdigits.
 *
 *  1) shift = q * rdigits + r
 *  2) srcdigits = qsrc * rdigits + msdigits
 *  3) destdigits = srcdigits - shift
 *                = qsrc * rdigits + msdigits - (q * rdigits + r)
 *                = (qsrc - q) * rdigits + msdigits - r
 *
 * Since destdigits > 0 and 1 <= msdigits <= rdigits:
 *
 *  4) qsrc >= q
 *  5) qsrc == q  ==>  msdigits > r
 *
 * The result has slen-q words if msdigits > r, slen-q-1 words otherwise.
 */
mpd_uint_t
_mpd_baseshiftr(mpd_uint_t *dest, mpd_uint_t *src, mpd_size_t slen,
                mpd_size_t shift)
{
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
    /* spurious uninitialized warnings */
    mpd_uint_t l=l, h=h, hprev=hprev; /* low, high, previous high */
#else
    mpd_uint_t l, h, hprev; /* low, high, previous high */
#endif
    mpd_uint_t rnd, rest;   /* rounding digit, rest */
    mpd_uint_t q, r;
    mpd_size_t i, j;
    mpd_uint_t ph;

    assert(slen > 0);

    _mpd_div_word(&q, &r, (mpd_uint_t)shift, MPD_RDIGITS);

    rnd = rest = 0;
    if (r != 0) {

        ph = mpd_pow10[MPD_RDIGITS-r];

        _mpd_divmod_pow10(&hprev, &rest, src[q], r);
        _mpd_divmod_pow10(&rnd, &rest, rest, r-1);

        if (rest == 0 && q > 0) {
            rest = !_mpd_isallzero(src, q);
        }
        /* write slen-q-1 words */
        for (j=0,i=q+1; i<slen; i++,j++) {
            _mpd_divmod_pow10(&h, &l, src[i], r);
            dest[j] = ph * l + hprev;
            hprev = h;
        }
        /* write most significant word */
        if (hprev != 0) { /* always the case if slen==q-1 */
            dest[j] = hprev;
        }
    }
    else {
        if (q > 0) {
            _mpd_divmod_pow10(&rnd, &rest, src[q-1], MPD_RDIGITS-1);
            /* is there any non-zero digit below rnd? */
            if (rest == 0) rest = !_mpd_isallzero(src, q-1);
        }
        for (j = 0; j < slen-q; j++) {
            dest[j] = src[q+j];
        }
    }

    /* 0-4  ==> rnd+rest < 0.5   */
    /* 5    ==> rnd+rest == 0.5  */
    /* 6-9  ==> rnd+rest > 0.5   */
    return (rnd == 0 || rnd == 5) ? rnd + !!rest : rnd;
}


/*********************************************************************/
/*                      Calculations in base b                       */
/*********************************************************************/

/*
 * Add v to w (len m). The calling function has to handle a possible
 * final carry. Assumption: m > 0.
 */
mpd_uint_t
_mpd_shortadd_b(mpd_uint_t *w, mpd_size_t m, mpd_uint_t v, mpd_uint_t b)
{
    mpd_uint_t s;
    mpd_uint_t carry;
    mpd_size_t i;

    assert(m > 0);

    /* add v to w */
    s = w[0] + v;
    carry = (s < v) | (s >= b);
    w[0] = carry ? s-b : s;

    /* if there is a carry, propagate it */
    for (i = 1; carry && i < m; i++) {
        s = w[i] + carry;
        carry = (s == b);
        w[i] = carry ? 0 : s;
    }

    return carry;
}

/* w := product of u (len n) and v (single word). Return carry. */
mpd_uint_t
_mpd_shortmul_c(mpd_uint_t *w, const mpd_uint_t *u, mpd_size_t n, mpd_uint_t v)
{
    mpd_uint_t hi, lo;
    mpd_uint_t carry = 0;
    mpd_size_t i;

    assert(n > 0);

    for (i=0; i < n; i++) {

        _mpd_mul_words(&hi, &lo, u[i], v);
        lo = carry + lo;
        if (lo < carry) hi++;

        _mpd_div_words_r(&carry, &w[i], hi, lo);
    }

    return carry;
}

/* w := product of u (len n) and v (single word) */
mpd_uint_t
_mpd_shortmul_b(mpd_uint_t *w, const mpd_uint_t *u, mpd_size_t n,
                mpd_uint_t v, mpd_uint_t b)
{
    mpd_uint_t hi, lo;
    mpd_uint_t carry = 0;
    mpd_size_t i;

    assert(n > 0);

    for (i=0; i < n; i++) {

        _mpd_mul_words(&hi, &lo, u[i], v);
        lo = carry + lo;
        if (lo < carry) hi++;

        _mpd_div_words(&carry, &w[i], hi, lo, b);
    }

    return carry;
}

/*
 * Knuth, TAOCP Volume 2, 4.3.1, exercise 16:
 *     w := quotient of u (len n) divided by a single word v
 */
mpd_uint_t
_mpd_shortdiv_b(mpd_uint_t *w, const mpd_uint_t *u, mpd_size_t n,
                mpd_uint_t v, mpd_uint_t b)
{
    mpd_uint_t hi, lo;
    mpd_uint_t rem = 0;
    mpd_size_t i;

    assert(n > 0);

    for (i=n-1; i != MPD_SIZE_MAX; i--) {

        _mpd_mul_words(&hi, &lo, rem, b);
        lo = u[i] + lo;
        if (lo < u[i]) hi++;

        _mpd_div_words(&w[i], &rem, hi, lo, v);
    }

    return rem;
}



