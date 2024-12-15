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

#include <stddef.h>
#include <stdint.h>


/* Signaling wrappers for the quiet functions in mpdecimal.c. */


char *
mpd_format(const mpd_t *dec, const char *fmt, mpd_context_t *ctx)
{
    char *ret;
    uint32_t status = 0;
    ret = mpd_qformat(dec, fmt, ctx, &status);
    mpd_addstatus_raise(ctx, status);
    return ret;
}

void
mpd_import_u16(mpd_t *result, const uint16_t *srcdata, size_t srclen,
               uint8_t srcsign, uint32_t base, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qimport_u16(result, srcdata, srclen, srcsign, base, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_import_u32(mpd_t *result, const uint32_t *srcdata, size_t srclen,
               uint8_t srcsign, uint32_t base, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qimport_u32(result, srcdata, srclen, srcsign, base, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

size_t
mpd_export_u16(uint16_t **rdata, size_t rlen, uint32_t base, const mpd_t *src,
               mpd_context_t *ctx)
{
    size_t n;
    uint32_t status = 0;
    n = mpd_qexport_u16(rdata, rlen, base, src, &status);
    mpd_addstatus_raise(ctx, status);
    return n;
}

size_t
mpd_export_u32(uint32_t **rdata, size_t rlen, uint32_t base, const mpd_t *src,
               mpd_context_t *ctx)
{
    size_t n;
    uint32_t status = 0;
    n = mpd_qexport_u32(rdata, rlen, base, src, &status);
    mpd_addstatus_raise(ctx, status);
    return n;
}

void
mpd_finalize(mpd_t *result, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qfinalize(result, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

int
mpd_check_nan(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    if (mpd_qcheck_nan(result, a, ctx, &status)) {
        mpd_addstatus_raise(ctx, status);
        return 1;
    }
    return 0;
}

int
mpd_check_nans(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    if (mpd_qcheck_nans(result, a, b, ctx, &status)) {
        mpd_addstatus_raise(ctx, status);
        return 1;
    }
    return 0;
}

void
mpd_set_string(mpd_t *result, const char *s, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qset_string(result, s, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_maxcoeff(mpd_t *result, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmaxcoeff(result, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

/* set static mpd from signed integer */
void
mpd_sset_ssize(mpd_t *result, mpd_ssize_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsset_ssize(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_sset_i32(mpd_t *result, int32_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsset_i32(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifdef CONFIG_64
void
mpd_sset_i64(mpd_t *result, int64_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsset_i64(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

/* set static mpd from unsigned integer */
void
mpd_sset_uint(mpd_t *result, mpd_uint_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsset_uint(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_sset_u32(mpd_t *result, uint32_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsset_u32(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifdef CONFIG_64
void
mpd_sset_u64(mpd_t *result, uint64_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsset_u64(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

/* set mpd from signed integer */
void
mpd_set_ssize(mpd_t *result, mpd_ssize_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qset_ssize(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_set_i32(mpd_t *result, int32_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qset_i32(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_set_i64(mpd_t *result, int64_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qset_i64(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

/* set mpd from unsigned integer */
void
mpd_set_uint(mpd_t *result, mpd_uint_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qset_uint(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_set_u32(mpd_t *result, uint32_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qset_u32(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_set_u64(mpd_t *result, uint64_t a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qset_u64(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

/* convert mpd to signed integer */
mpd_ssize_t
mpd_get_ssize(const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_ssize_t ret;

    ret = mpd_qget_ssize(a, &status);
    mpd_addstatus_raise(ctx, status);
    return ret;
}

int32_t
mpd_get_i32(const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    int32_t ret;

    ret = mpd_qget_i32(a, &status);
    mpd_addstatus_raise(ctx, status);
    return ret;
}

#ifndef LEGACY_COMPILER
int64_t
mpd_get_i64(const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    int64_t ret;

    ret = mpd_qget_i64(a, &status);
    mpd_addstatus_raise(ctx, status);
    return ret;
}
#endif

mpd_uint_t
mpd_get_uint(const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_uint_t ret;

    ret = mpd_qget_uint(a, &status);
    mpd_addstatus_raise(ctx, status);
    return ret;
}

mpd_uint_t
mpd_abs_uint(const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_uint_t ret;

    ret = mpd_qabs_uint(a, &status);
    mpd_addstatus_raise(ctx, status);
    return ret;
}

uint32_t
mpd_get_u32(const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    uint32_t ret;

    ret = mpd_qget_u32(a, &status);
    mpd_addstatus_raise(ctx, status);
    return ret;
}

#ifndef LEGACY_COMPILER
uint64_t
mpd_get_u64(const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    uint64_t ret;

    ret = mpd_qget_u64(a, &status);
    mpd_addstatus_raise(ctx, status);
    return ret;
}
#endif

void
mpd_and(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qand(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_copy(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    if (!mpd_qcopy(result, a, &status)) {
        mpd_addstatus_raise(ctx, status);
    }
}

void
mpd_canonical(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    mpd_copy(result, a, ctx);
}

void
mpd_copy_abs(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    if (!mpd_qcopy_abs(result, a, &status)) {
        mpd_addstatus_raise(ctx, status);
    }
}

void
mpd_copy_negate(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    if (!mpd_qcopy_negate(result, a, &status)) {
        mpd_addstatus_raise(ctx, status);
    }
}

void
mpd_copy_sign(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    if (!mpd_qcopy_sign(result, a, b, &status)) {
        mpd_addstatus_raise(ctx, status);
    }
}

void
mpd_invert(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qinvert(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_logb(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qlogb(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_or(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qor(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_rotate(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qrotate(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_scaleb(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qscaleb(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_shiftl(mpd_t *result, const mpd_t *a, mpd_ssize_t n, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qshiftl(result, a, n, &status);
    mpd_addstatus_raise(ctx, status);
}

mpd_uint_t
mpd_shiftr(mpd_t *result, const mpd_t *a, mpd_ssize_t n, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_uint_t rnd;

    rnd = mpd_qshiftr(result, a, n, &status);
    mpd_addstatus_raise(ctx, status);
    return rnd;
}

void
mpd_shiftn(mpd_t *result, const mpd_t *a, mpd_ssize_t n, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qshiftn(result, a, n, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_shift(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qshift(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_xor(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qxor(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_abs(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qabs(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

int
mpd_cmp(const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    int c;
    c = mpd_qcmp(a, b, &status);
    mpd_addstatus_raise(ctx, status);
    return c;
}

int
mpd_compare(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    int c;
    c = mpd_qcompare(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
    return c;
}

int
mpd_compare_signal(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    int c;
    c = mpd_qcompare_signal(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
    return c;
}

void
mpd_add(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qadd(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_sub(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsub(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_add_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qadd_ssize(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_add_i32(mpd_t *result, const mpd_t *a, int32_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qadd_i32(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_add_i64(mpd_t *result, const mpd_t *a, int64_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qadd_i64(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

void
mpd_add_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qadd_uint(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_add_u32(mpd_t *result, const mpd_t *a, uint32_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qadd_u32(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_add_u64(mpd_t *result, const mpd_t *a, uint64_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qadd_u64(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

void
mpd_sub_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsub_ssize(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_sub_i32(mpd_t *result, const mpd_t *a, int32_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsub_i32(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_sub_i64(mpd_t *result, const mpd_t *a, int64_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsub_i64(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

void
mpd_sub_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsub_uint(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_sub_u32(mpd_t *result, const mpd_t *a, uint32_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsub_u32(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_sub_u64(mpd_t *result, const mpd_t *a, uint64_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsub_u64(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

void
mpd_div(mpd_t *q, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdiv(q, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_div_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdiv_ssize(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_div_i32(mpd_t *result, const mpd_t *a, int32_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdiv_i32(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_div_i64(mpd_t *result, const mpd_t *a, int64_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdiv_i64(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

void
mpd_div_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdiv_uint(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_div_u32(mpd_t *result, const mpd_t *a, uint32_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdiv_u32(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_div_u64(mpd_t *result, const mpd_t *a, uint64_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdiv_u64(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

void
mpd_divmod(mpd_t *q, mpd_t *r, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdivmod(q, r, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_divint(mpd_t *q, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qdivint(q, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_exp(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qexp(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_fma(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_t *c,
        mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qfma(result, a, b, c, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_ln(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qln(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_log10(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qlog10(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_max(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmax(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_max_mag(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmax_mag(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_min(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmin(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_min_mag(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmin_mag(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_minus(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qminus(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_mul(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmul(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_mul_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmul_ssize(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_mul_i32(mpd_t *result, const mpd_t *a, int32_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmul_i32(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_mul_i64(mpd_t *result, const mpd_t *a, int64_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmul_i64(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

void
mpd_mul_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmul_uint(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_mul_u32(mpd_t *result, const mpd_t *a, uint32_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmul_u32(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

#ifndef LEGACY_COMPILER
void
mpd_mul_u64(mpd_t *result, const mpd_t *a, uint64_t b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qmul_u64(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
#endif

void
mpd_next_minus(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qnext_minus(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_next_plus(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qnext_plus(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_next_toward(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qnext_toward(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_plus(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qplus(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_pow(mpd_t *result, const mpd_t *base, const mpd_t *exp, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qpow(result, base, exp, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_powmod(mpd_t *result, const mpd_t *base, const mpd_t *exp, const mpd_t *mod,
           mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qpowmod(result, base, exp, mod, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_quantize(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qquantize(result, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_rescale(mpd_t *result, const mpd_t *a, mpd_ssize_t exp, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qrescale(result, a, exp, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_reduce(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qreduce(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_rem(mpd_t *r, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qrem(r, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_rem_near(mpd_t *r, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qrem_near(r, a, b, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_round_to_intx(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qround_to_intx(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_round_to_int(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qround_to_int(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_trunc(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qtrunc(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_floor(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qfloor(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_ceil(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qceil(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_sqrt(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qsqrt(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}

void
mpd_invroot(mpd_t *result, const mpd_t *a, mpd_context_t *ctx)
{
    uint32_t status = 0;
    mpd_qinvroot(result, a, ctx, &status);
    mpd_addstatus_raise(ctx, status);
}
