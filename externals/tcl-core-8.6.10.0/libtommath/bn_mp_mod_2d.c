#include "tommath_private.h"
#ifdef BN_MP_MOD_2D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* calc a value mod 2**b */
mp_err mp_mod_2d(const mp_int *a, int b, mp_int *c)
{
   int x;
   mp_err err;

   /* if b is <= 0 then zero the int */
   if (b <= 0) {
      mp_zero(c);
      return MP_OKAY;
   }

   /* if the modulus is larger than the value than return */
   if (b >= (a->used * MP_DIGIT_BIT)) {
      return mp_copy(a, c);
   }

   /* copy */
   if ((err = mp_copy(a, c)) != MP_OKAY) {
      return err;
   }

   /* zero digits above the last digit of the modulus */
   x = (b / MP_DIGIT_BIT) + (((b % MP_DIGIT_BIT) == 0) ? 0 : 1);
   MP_ZERO_DIGITS(c->dp + x, c->used - x);

   /* clear the digit that is not completely outside/inside the modulus */
   c->dp[b / MP_DIGIT_BIT] &=
      ((mp_digit)1 << (mp_digit)(b % MP_DIGIT_BIT)) - (mp_digit)1;
   mp_clamp(c);
   return MP_OKAY;
}
#endif
