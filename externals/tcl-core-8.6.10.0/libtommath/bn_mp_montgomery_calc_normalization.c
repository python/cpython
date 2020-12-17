#include "tommath_private.h"
#ifdef BN_MP_MONTGOMERY_CALC_NORMALIZATION_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/*
 * shifts with subtractions when the result is greater than b.
 *
 * The method is slightly modified to shift B unconditionally upto just under
 * the leading bit of b.  This saves alot of multiple precision shifting.
 */
mp_err mp_montgomery_calc_normalization(mp_int *a, const mp_int *b)
{
   int    x, bits;
   mp_err err;

   /* how many bits of last digit does b use */
   bits = mp_count_bits(b) % MP_DIGIT_BIT;

   if (b->used > 1) {
      if ((err = mp_2expt(a, ((b->used - 1) * MP_DIGIT_BIT) + bits - 1)) != MP_OKAY) {
         return err;
      }
   } else {
      mp_set(a, 1uL);
      bits = 1;
   }


   /* now compute C = A * B mod b */
   for (x = bits - 1; x < (int)MP_DIGIT_BIT; x++) {
      if ((err = mp_mul_2(a, a)) != MP_OKAY) {
         return err;
      }
      if (mp_cmp_mag(a, b) != MP_LT) {
         if ((err = s_mp_sub(a, b, a)) != MP_OKAY) {
            return err;
         }
      }
   }

   return MP_OKAY;
}
#endif
