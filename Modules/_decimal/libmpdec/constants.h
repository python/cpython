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


#ifndef CONSTANTS_H
#define CONSTANTS_H


#include "mpdecimal.h"


/* Internal header file: all symbols have local scope in the DSO */
MPD_PRAGMA(MPD_HIDE_SYMBOLS_START)


/* choice of optimized functions */
#if defined(CONFIG_64)
/* x64 */
  #define MULMOD(a, b) x64_mulmod(a, b, umod)
  #define MULMOD2C(a0, a1, w) x64_mulmod2c(a0, a1, w, umod)
  #define MULMOD2(a0, b0, a1, b1) x64_mulmod2(a0, b0, a1, b1, umod)
  #define POWMOD(base, exp) x64_powmod(base, exp, umod)
  #define SETMODULUS(modnum) std_setmodulus(modnum, &umod)
  #define SIZE3_NTT(x0, x1, x2, w3table) std_size3_ntt(x0, x1, x2, w3table, umod)
#elif defined(PPRO)
/* PentiumPro (or later) gcc inline asm */
  #define MULMOD(a, b) ppro_mulmod(a, b, &dmod, dinvmod)
  #define MULMOD2C(a0, a1, w) ppro_mulmod2c(a0, a1, w, &dmod, dinvmod)
  #define MULMOD2(a0, b0, a1, b1) ppro_mulmod2(a0, b0, a1, b1, &dmod, dinvmod)
  #define POWMOD(base, exp) ppro_powmod(base, exp, &dmod, dinvmod)
  #define SETMODULUS(modnum) ppro_setmodulus(modnum, &umod, &dmod, dinvmod)
  #define SIZE3_NTT(x0, x1, x2, w3table) ppro_size3_ntt(x0, x1, x2, w3table, umod, &dmod, dinvmod)
#else
  /* ANSI C99 */
  #define MULMOD(a, b) std_mulmod(a, b, umod)
  #define MULMOD2C(a0, a1, w) std_mulmod2c(a0, a1, w, umod)
  #define MULMOD2(a0, b0, a1, b1) std_mulmod2(a0, b0, a1, b1, umod)
  #define POWMOD(base, exp) std_powmod(base, exp, umod)
  #define SETMODULUS(modnum) std_setmodulus(modnum, &umod)
  #define SIZE3_NTT(x0, x1, x2, w3table) std_size3_ntt(x0, x1, x2, w3table, umod)
#endif

/* PentiumPro (or later) gcc inline asm */
extern const float MPD_TWO63;
extern const uint32_t mpd_invmoduli[3][3];

enum {P1, P2, P3};

extern const mpd_uint_t mpd_moduli[];
extern const mpd_uint_t mpd_roots[];
extern const mpd_size_t mpd_bits[];
extern const mpd_uint_t mpd_pow10[];

extern const mpd_uint_t INV_P1_MOD_P2;
extern const mpd_uint_t INV_P1P2_MOD_P3;
extern const mpd_uint_t LH_P1P2;
extern const mpd_uint_t UH_P1P2;


MPD_PRAGMA(MPD_HIDE_SYMBOLS_END) /* restore previous scope rules */


#endif /* CONSTANTS_H */



