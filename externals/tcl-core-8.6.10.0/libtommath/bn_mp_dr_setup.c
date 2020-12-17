#include "tommath_private.h"
#ifdef BN_MP_DR_SETUP_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* determines the setup value */
void mp_dr_setup(const mp_int *a, mp_digit *d)
{
   /* the casts are required if MP_DIGIT_BIT is one less than
    * the number of bits in a mp_digit [e.g. MP_DIGIT_BIT==31]
    */
   *d = (mp_digit)(((mp_word)1 << (mp_word)MP_DIGIT_BIT) - (mp_word)a->dp[0]);
}

#endif
