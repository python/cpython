#include "tommath_private.h"
#ifdef BN_MP_MUL_D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* multiply by a digit */
mp_err mp_mul_d(const mp_int *a, mp_digit b, mp_int *c)
{
   mp_digit u, *tmpa, *tmpc;
   mp_word  r;
   mp_err   err;
   int      ix, olduse;

   /* make sure c is big enough to hold a*b */
   if (c->alloc < (a->used + 1)) {
      if ((err = mp_grow(c, a->used + 1)) != MP_OKAY) {
         return err;
      }
   }

   /* get the original destinations used count */
   olduse = c->used;

   /* set the sign */
   c->sign = a->sign;

   /* alias for a->dp [source] */
   tmpa = a->dp;

   /* alias for c->dp [dest] */
   tmpc = c->dp;

   /* zero carry */
   u = 0;

   /* compute columns */
   for (ix = 0; ix < a->used; ix++) {
      /* compute product and carry sum for this term */
      r       = (mp_word)u + ((mp_word)*tmpa++ * (mp_word)b);

      /* mask off higher bits to get a single digit */
      *tmpc++ = (mp_digit)(r & (mp_word)MP_MASK);

      /* send carry into next iteration */
      u       = (mp_digit)(r >> (mp_word)MP_DIGIT_BIT);
   }

   /* store final carry [if any] and increment ix offset  */
   *tmpc++ = u;
   ++ix;

   /* now zero digits above the top */
   MP_ZERO_DIGITS(tmpc, olduse - ix);

   /* set used count */
   c->used = a->used + 1;
   mp_clamp(c);

   return MP_OKAY;
}
#endif
