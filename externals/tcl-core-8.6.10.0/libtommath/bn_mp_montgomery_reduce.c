#include "tommath_private.h"
#ifdef BN_MP_MONTGOMERY_REDUCE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* computes xR**-1 == x (mod N) via Montgomery Reduction */
mp_err mp_montgomery_reduce(mp_int *x, const mp_int *n, mp_digit rho)
{
   int      ix, digs;
   mp_err   err;
   mp_digit mu;

   /* can the fast reduction [comba] method be used?
    *
    * Note that unlike in mul you're safely allowed *less*
    * than the available columns [255 per default] since carries
    * are fixed up in the inner loop.
    */
   digs = (n->used * 2) + 1;
   if ((digs < MP_WARRAY) &&
       (x->used <= MP_WARRAY) &&
       (n->used < MP_MAXFAST)) {
      return s_mp_montgomery_reduce_fast(x, n, rho);
   }

   /* grow the input as required */
   if (x->alloc < digs) {
      if ((err = mp_grow(x, digs)) != MP_OKAY) {
         return err;
      }
   }
   x->used = digs;

   for (ix = 0; ix < n->used; ix++) {
      /* mu = ai * rho mod b
       *
       * The value of rho must be precalculated via
       * montgomery_setup() such that
       * it equals -1/n0 mod b this allows the
       * following inner loop to reduce the
       * input one digit at a time
       */
      mu = (mp_digit)(((mp_word)x->dp[ix] * (mp_word)rho) & MP_MASK);

      /* a = a + mu * m * b**i */
      {
         int iy;
         mp_digit *tmpn, *tmpx, u;
         mp_word r;

         /* alias for digits of the modulus */
         tmpn = n->dp;

         /* alias for the digits of x [the input] */
         tmpx = x->dp + ix;

         /* set the carry to zero */
         u = 0;

         /* Multiply and add in place */
         for (iy = 0; iy < n->used; iy++) {
            /* compute product and sum */
            r       = ((mp_word)mu * (mp_word)*tmpn++) +
                      (mp_word)u + (mp_word)*tmpx;

            /* get carry */
            u       = (mp_digit)(r >> (mp_word)MP_DIGIT_BIT);

            /* fix digit */
            *tmpx++ = (mp_digit)(r & (mp_word)MP_MASK);
         }
         /* At this point the ix'th digit of x should be zero */


         /* propagate carries upwards as required*/
         while (u != 0u) {
            *tmpx   += u;
            u        = *tmpx >> MP_DIGIT_BIT;
            *tmpx++ &= MP_MASK;
         }
      }
   }

   /* at this point the n.used'th least
    * significant digits of x are all zero
    * which means we can shift x to the
    * right by n.used digits and the
    * residue is unchanged.
    */

   /* x = x/b**n.used */
   mp_clamp(x);
   mp_rshd(x, n->used);

   /* if x >= n then x = x - n */
   if (mp_cmp_mag(x, n) != MP_LT) {
      return s_mp_sub(x, n, x);
   }

   return MP_OKAY;
}
#endif
