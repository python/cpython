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


#include "mpdecimal.h"
#include <stdio.h>
#include <stdlib.h>
#include "typearith.h"
#include "memory.h"


/* Guaranteed minimum allocation for a coefficient. May be changed once
   at program start using mpd_setminalloc(). */
mpd_ssize_t MPD_MINALLOC = MPD_MINALLOC_MIN;

/* Custom allocation and free functions */
void *(* mpd_mallocfunc)(size_t size) = malloc;
void *(* mpd_reallocfunc)(void *ptr, size_t size) = realloc;
void *(* mpd_callocfunc)(size_t nmemb, size_t size) = calloc;
void (* mpd_free)(void *ptr) = free;


/* emulate calloc if it is not available */
void *
mpd_callocfunc_em(size_t nmemb, size_t size)
{
    void *ptr;
    size_t req;
    mpd_size_t overflow;

#if MPD_SIZE_MAX < SIZE_MAX
    /* full_coverage test only */
    if (nmemb > MPD_SIZE_MAX || size > MPD_SIZE_MAX) {
        return NULL;
    }
#endif

    req = mul_size_t_overflow((mpd_size_t)nmemb, (mpd_size_t)size,
                              &overflow);
    if (overflow) {
        return NULL;
    }

    ptr = mpd_mallocfunc(req);
    if (ptr == NULL) {
        return NULL;
    }
    /* used on uint32_t or uint64_t */
    memset(ptr, 0, req);

    return ptr;
}


/* malloc with overflow checking */
void *
mpd_alloc(mpd_size_t nmemb, mpd_size_t size)
{
    mpd_size_t req, overflow;

    req = mul_size_t_overflow(nmemb, size, &overflow);
    if (overflow) {
        return NULL;
    }

    return mpd_mallocfunc(req);
}

/* calloc with overflow checking */
void *
mpd_calloc(mpd_size_t nmemb, mpd_size_t size)
{
    mpd_size_t overflow;

    (void)mul_size_t_overflow(nmemb, size, &overflow);
    if (overflow) {
        return NULL;
    }

    return mpd_callocfunc(nmemb, size);
}

/* realloc with overflow checking */
void *
mpd_realloc(void *ptr, mpd_size_t nmemb, mpd_size_t size, uint8_t *err)
{
    void *new;
    mpd_size_t req, overflow;

    req = mul_size_t_overflow(nmemb, size, &overflow);
    if (overflow) {
        *err = 1;
        return ptr;
    }

    new = mpd_reallocfunc(ptr, req);
    if (new == NULL) {
        *err = 1;
        return ptr;
    }

    return new;
}

/* struct hack malloc with overflow checking */
void *
mpd_sh_alloc(mpd_size_t struct_size, mpd_size_t nmemb, mpd_size_t size)
{
    mpd_size_t req, overflow;

    req = mul_size_t_overflow(nmemb, size, &overflow);
    if (overflow) {
        return NULL;
    }

    req = add_size_t_overflow(req, struct_size, &overflow);
    if (overflow) {
        return NULL;
    }

    return mpd_mallocfunc(req);
}


/* Allocate a new decimal with a coefficient of length 'nwords'. In case
   of an error the return value is NULL. */
mpd_t *
mpd_qnew_size(mpd_ssize_t nwords)
{
    mpd_t *result;

    nwords = (nwords < MPD_MINALLOC) ? MPD_MINALLOC : nwords;

    result = mpd_alloc(1, sizeof *result);
    if (result == NULL) {
        return NULL;
    }

    result->data = mpd_alloc(nwords, sizeof *result->data);
    if (result->data == NULL) {
        mpd_free(result);
        return NULL;
    }

    result->flags = 0;
    result->exp = 0;
    result->digits = 0;
    result->len = 0;
    result->alloc = nwords;

    return result;
}

/* Allocate a new decimal with a coefficient of length MPD_MINALLOC.
   In case of an error the return value is NULL. */
mpd_t *
mpd_qnew(void)
{
    return mpd_qnew_size(MPD_MINALLOC);
}

/* Allocate new decimal. Caller can check for NULL or MPD_Malloc_error.
   Raises on error. */
mpd_t *
mpd_new(mpd_context_t *ctx)
{
    mpd_t *result;

    result = mpd_qnew();
    if (result == NULL) {
        mpd_addstatus_raise(ctx, MPD_Malloc_error);
    }
    return result;
}

/*
 * Input: 'result' is a static mpd_t with a static coefficient.
 * Assumption: 'nwords' >= result->alloc.
 *
 * Resize the static coefficient to a larger dynamic one and copy the
 * existing data. If successful, the value of 'result' is unchanged.
 * Otherwise, set 'result' to NaN and update 'status' with MPD_Malloc_error.
 */
int
mpd_switch_to_dyn(mpd_t *result, mpd_ssize_t nwords, uint32_t *status)
{
    mpd_uint_t *p = result->data;

    assert(nwords >= result->alloc);

    result->data = mpd_alloc(nwords, sizeof *result->data);
    if (result->data == NULL) {
        result->data = p;
        mpd_set_qnan(result);
        mpd_set_positive(result);
        result->exp = result->digits = result->len = 0;
        *status |= MPD_Malloc_error;
        return 0;
    }

    memcpy(result->data, p, result->alloc * (sizeof *result->data));
    result->alloc = nwords;
    mpd_set_dynamic_data(result);
    return 1;
}

/*
 * Input: 'result' is a static mpd_t with a static coefficient.
 *
 * Convert the coefficient to a dynamic one that is initialized to zero. If
 * malloc fails, set 'result' to NaN and update 'status' with MPD_Malloc_error.
 */
int
mpd_switch_to_dyn_zero(mpd_t *result, mpd_ssize_t nwords, uint32_t *status)
{
    mpd_uint_t *p = result->data;

    result->data = mpd_calloc(nwords, sizeof *result->data);
    if (result->data == NULL) {
        result->data = p;
        mpd_set_qnan(result);
        mpd_set_positive(result);
        result->exp = result->digits = result->len = 0;
        *status |= MPD_Malloc_error;
        return 0;
    }

    result->alloc = nwords;
    mpd_set_dynamic_data(result);

    return 1;
}

/*
 * Input: 'result' is a static or a dynamic mpd_t with a dynamic coefficient.
 * Resize the coefficient to length 'nwords':
 *   Case nwords > result->alloc:
 *     If realloc is successful:
 *       'result' has a larger coefficient but the same value. Return 1.
 *     Otherwise:
 *       Set 'result' to NaN, update status with MPD_Malloc_error and return 0.
 *   Case nwords < result->alloc:
 *     If realloc is successful:
 *       'result' has a smaller coefficient. result->len is undefined. Return 1.
 *     Otherwise (unlikely):
 *       'result' is unchanged. Reuse the now oversized coefficient. Return 1.
 */
int
mpd_realloc_dyn(mpd_t *result, mpd_ssize_t nwords, uint32_t *status)
{
    uint8_t err = 0;

    result->data = mpd_realloc(result->data, nwords, sizeof *result->data, &err);
    if (!err) {
        result->alloc = nwords;
    }
    else if (nwords > result->alloc) {
        mpd_set_qnan(result);
        mpd_set_positive(result);
        result->exp = result->digits = result->len = 0;
        *status |= MPD_Malloc_error;
        return 0;
    }

    return 1;
}


