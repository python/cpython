#include "tommath_private.h"
#ifdef BN_MP_2EXPT_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* computes a = 2**b
 *
 * Simple algorithm which zeroes the int, grows it then just sets one bit
 * as required.
 */
mp_err mp_2expt(mp_int *a, int b)
{
   mp_err    err;

   /* zero a as per default */
   mp_zero(a);

   /* grow a to accomodate the single bit */
   if ((err = mp_grow(a, (b / MP_DIGIT_BIT) + 1)) != MP_OKAY) {
      return err;
   }

   /* set the used count of where the bit will go */
   a->used = (b / MP_DIGIT_BIT) + 1;

   /* put the single bit in its place */
   a->dp[b / MP_DIGIT_BIT] = (mp_digit)1 << (mp_digit)(b % MP_DIGIT_BIT);

   return MP_OKAY;
}
#endif
