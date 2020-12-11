#include "tommath_private.h"
#ifdef BN_S_MP_GET_BIT_C

/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Get bit at position b and return MP_YES if the bit is 1, MP_NO if it is 0 */
mp_bool s_mp_get_bit(const mp_int *a, unsigned int b)
{
   mp_digit bit;
   int limb = (int)(b / MP_DIGIT_BIT);

   if (limb >= a->used) {
      return MP_NO;
   }

   bit = (mp_digit)1 << (b % MP_DIGIT_BIT);
   return ((a->dp[limb] & bit) != 0u) ? MP_YES : MP_NO;
}

#endif
