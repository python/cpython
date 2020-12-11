#include "tommath_private.h"
#ifdef BN_S_MP_ADD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* low level addition, based on HAC pp.594, Algorithm 14.7 */
mp_err s_mp_add(const mp_int *a, const mp_int *b, mp_int *c)
{
   const mp_int *x;
   mp_err err;
   int     olduse, min, max;

   /* find sizes, we let |a| <= |b| which means we have to sort
    * them.  "x" will point to the input with the most digits
    */
   if (a->used > b->used) {
      min = b->used;
      max = a->used;
      x = a;
   } else {
      min = a->used;
      max = b->used;
      x = b;
   }

   /* init result */
   if (c->alloc < (max + 1)) {
      if ((err = mp_grow(c, max + 1)) != MP_OKAY) {
         return err;
      }
   }

   /* get old used digit count and set new one */
   olduse = c->used;
   c->used = max + 1;

   {
      mp_digit u, *tmpa, *tmpb, *tmpc;
      int i;

      /* alias for digit pointers */

      /* first input */
      tmpa = a->dp;

      /* second input */
      tmpb = b->dp;

      /* destination */
      tmpc = c->dp;

      /* zero the carry */
      u = 0;
      for (i = 0; i < min; i++) {
         /* Compute the sum at one digit, T[i] = A[i] + B[i] + U */
         *tmpc = *tmpa++ + *tmpb++ + u;

         /* U = carry bit of T[i] */
         u = *tmpc >> (mp_digit)MP_DIGIT_BIT;

         /* take away carry bit from T[i] */
         *tmpc++ &= MP_MASK;
      }

      /* now copy higher words if any, that is in A+B
       * if A or B has more digits add those in
       */
      if (min != max) {
         for (; i < max; i++) {
            /* T[i] = X[i] + U */
            *tmpc = x->dp[i] + u;

            /* U = carry bit of T[i] */
            u = *tmpc >> (mp_digit)MP_DIGIT_BIT;

            /* take away carry bit from T[i] */
            *tmpc++ &= MP_MASK;
         }
      }

      /* add carry */
      *tmpc++ = u;

      /* clear digits above oldused */
      MP_ZERO_DIGITS(tmpc, olduse - c->used);
   }

   mp_clamp(c);
   return MP_OKAY;
}
#endif
