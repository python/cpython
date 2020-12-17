#include "tommath_private.h"
#ifdef BN_MP_DIV_2_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* b = a/2 */
mp_err mp_div_2(const mp_int *a, mp_int *b)
{
   int     x, oldused;
   mp_digit r, rr, *tmpa, *tmpb;
   mp_err err;

   /* copy */
   if (b->alloc < a->used) {
      if ((err = mp_grow(b, a->used)) != MP_OKAY) {
         return err;
      }
   }

   oldused = b->used;
   b->used = a->used;

   /* source alias */
   tmpa = a->dp + b->used - 1;

   /* dest alias */
   tmpb = b->dp + b->used - 1;

   /* carry */
   r = 0;
   for (x = b->used - 1; x >= 0; x--) {
      /* get the carry for the next iteration */
      rr = *tmpa & 1u;

      /* shift the current digit, add in carry and store */
      *tmpb-- = (*tmpa-- >> 1) | (r << (MP_DIGIT_BIT - 1));

      /* forward carry to next iteration */
      r = rr;
   }

   /* zero excess digits */
   MP_ZERO_DIGITS(b->dp + b->used, oldused - b->used);

   b->sign = a->sign;
   mp_clamp(b);
   return MP_OKAY;
}
#endif
