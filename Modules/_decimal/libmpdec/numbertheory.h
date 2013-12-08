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


#ifndef NUMBER_THEORY_H
#define NUMBER_THEORY_H


#include "constants.h"
#include "mpdecimal.h"


/* Internal header file: all symbols have local scope in the DSO */
MPD_PRAGMA(MPD_HIDE_SYMBOLS_START)


/* transform parameters */
struct fnt_params {
    int modnum;
    mpd_uint_t modulus;
    mpd_uint_t kernel;
    mpd_uint_t wtable[];
};


mpd_uint_t _mpd_getkernel(mpd_uint_t n, int sign, int modnum);
struct fnt_params *_mpd_init_fnt_params(mpd_size_t n, int sign, int modnum);
void _mpd_init_w3table(mpd_uint_t w3table[3], int sign, int modnum);


#ifdef PPRO
static inline void
ppro_setmodulus(int modnum, mpd_uint_t *umod, double *dmod, uint32_t dinvmod[3])
{
    *dmod = *umod =  mpd_moduli[modnum];
    dinvmod[0] = mpd_invmoduli[modnum][0];
    dinvmod[1] = mpd_invmoduli[modnum][1];
    dinvmod[2] = mpd_invmoduli[modnum][2];
}
#else
static inline void
std_setmodulus(int modnum, mpd_uint_t *umod)
{
    *umod =  mpd_moduli[modnum];
}
#endif


MPD_PRAGMA(MPD_HIDE_SYMBOLS_END) /* restore previous scope rules */


#endif


