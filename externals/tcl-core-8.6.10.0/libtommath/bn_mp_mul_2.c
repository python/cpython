#include "tommath_private.h"
#ifdef BN_MP_MUL_2_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* b = a*2 */
mp_err mp_mul_2(const mp_int *a, mp_int *b)
{
   int     x, oldused;
   mp_err err;

   /* grow to accomodate result */
   if (b->alloc < (a->used + 1)) {
      if ((err = mp_grow(b, a->used + 1)) != MP_OKAY) {
         return err;
      }
   }

   oldused = b->used;
   b->used = a->used;

   {
      mp_digit r, rr, *tmpa, *tmpb;

      /* alias for source */
      tmpa = a->dp;

      /* alias for dest */
      tmpb = b->dp;

      /* carry */
      r = 0;
      for (x = 0; x < a->used; x++) {

         /* get what will be the *next* carry bit from the
          * MSB of the current digit
          */
         rr = *tmpa >> (mp_digit)(MP_DIGIT_BIT - 1);

         /* now shift up this digit, add in the carry [from the previous] */
         *tmpb++ = ((*tmpa++ << 1uL) | r) & MP_MASK;

         /* copy the carry that would be from the source
          * digit into the next iteration
          */
         r = rr;
      }

      /* new leading digit? */
      if (r != 0u) {
         /* add a MSB which is always 1 at this point */
         *tmpb = 1;
         ++(b->used);
      }

      /* now zero any excess digits on the destination
       * that we didn't write to
       */
      MP_ZERO_DIGITS(b->dp + b->used, oldused - b->used);
   }
   b->sign = a->sign;
   return MP_OKAY;
}
#endif
