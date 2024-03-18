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
#include "bits.h"
#include "constants.h"
#include "convolute.h"
#include "fnt.h"
#include "fourstep.h"
#include "numbertheory.h"
#include "sixstep.h"
#include "umodarith.h"


/* Bignum: Fast convolution using the Number Theoretic Transform. Used for
   the multiplication of very large coefficients. */


/* Convolute the data in c1 and c2. Result is in c1. */
int
fnt_convolute(mpd_uint_t *c1, mpd_uint_t *c2, mpd_size_t n, int modnum)
{
    int (*fnt)(mpd_uint_t *, mpd_size_t, int);
    int (*inv_fnt)(mpd_uint_t *, mpd_size_t, int);
#ifdef PPRO
    double dmod;
    uint32_t dinvmod[3];
#endif
    mpd_uint_t n_inv, umod;
    mpd_size_t i;


    SETMODULUS(modnum);
    n_inv = POWMOD(n, (umod-2));

    if (ispower2(n)) {
        if (n > SIX_STEP_THRESHOLD) {
            fnt = six_step_fnt;
            inv_fnt = inv_six_step_fnt;
        }
        else {
            fnt = std_fnt;
            inv_fnt = std_inv_fnt;
        }
    }
    else {
        fnt = four_step_fnt;
        inv_fnt = inv_four_step_fnt;
    }

    if (!fnt(c1, n, modnum)) {
        return 0;
    }
    if (!fnt(c2, n, modnum)) {
        return 0;
    }
    for (i = 0; i < n-1; i += 2) {
        mpd_uint_t x0 = c1[i];
        mpd_uint_t y0 = c2[i];
        mpd_uint_t x1 = c1[i+1];
        mpd_uint_t y1 = c2[i+1];
        MULMOD2(&x0, y0, &x1, y1);
        c1[i] = x0;
        c1[i+1] = x1;
    }

    if (!inv_fnt(c1, n, modnum)) {
        return 0;
    }
    for (i = 0; i < n-3; i += 4) {
        mpd_uint_t x0 = c1[i];
        mpd_uint_t x1 = c1[i+1];
        mpd_uint_t x2 = c1[i+2];
        mpd_uint_t x3 = c1[i+3];
        MULMOD2C(&x0, &x1, n_inv);
        MULMOD2C(&x2, &x3, n_inv);
        c1[i] = x0;
        c1[i+1] = x1;
        c1[i+2] = x2;
        c1[i+3] = x3;
    }

    return 1;
}

/* Autoconvolute the data in c1. Result is in c1. */
int
fnt_autoconvolute(mpd_uint_t *c1, mpd_size_t n, int modnum)
{
    int (*fnt)(mpd_uint_t *, mpd_size_t, int);
    int (*inv_fnt)(mpd_uint_t *, mpd_size_t, int);
#ifdef PPRO
    double dmod;
    uint32_t dinvmod[3];
#endif
    mpd_uint_t n_inv, umod;
    mpd_size_t i;


    SETMODULUS(modnum);
    n_inv = POWMOD(n, (umod-2));

    if (ispower2(n)) {
        if (n > SIX_STEP_THRESHOLD) {
            fnt = six_step_fnt;
            inv_fnt = inv_six_step_fnt;
        }
        else {
            fnt = std_fnt;
            inv_fnt = std_inv_fnt;
        }
    }
    else {
        fnt = four_step_fnt;
        inv_fnt = inv_four_step_fnt;
    }

    if (!fnt(c1, n, modnum)) {
        return 0;
    }
    for (i = 0; i < n-1; i += 2) {
        mpd_uint_t x0 = c1[i];
        mpd_uint_t x1 = c1[i+1];
        MULMOD2(&x0, x0, &x1, x1);
        c1[i] = x0;
        c1[i+1] = x1;
    }

    if (!inv_fnt(c1, n, modnum)) {
        return 0;
    }
    for (i = 0; i < n-3; i += 4) {
        mpd_uint_t x0 = c1[i];
        mpd_uint_t x1 = c1[i+1];
        mpd_uint_t x2 = c1[i+2];
        mpd_uint_t x3 = c1[i+3];
        MULMOD2C(&x0, &x1, n_inv);
        MULMOD2C(&x2, &x3, n_inv);
        c1[i] = x0;
        c1[i+1] = x1;
        c1[i+2] = x2;
        c1[i+3] = x3;
    }

    return 1;
}
