#include "tommath_private.h"
#ifdef BN_MP_DR_REDUCE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* reduce "x" in place modulo "n" using the Diminished Radix algorithm.
 *
 * Based on algorithm from the paper
 *
 * "Generating Efficient Primes for Discrete Log Cryptosystems"
 *                 Chae Hoon Lim, Pil Joong Lee,
 *          POSTECH Information Research Laboratories
 *
 * The modulus must be of a special format [see manual]
 *
 * Has been modified to use algorithm 7.10 from the LTM book instead
 *
 * Input x must be in the range 0 <= x <= (n-1)**2
 */
mp_err mp_dr_reduce(mp_int *x, const mp_int *n, mp_digit k)
{
   mp_err      err;
   int i, m;
   mp_word  r;
   mp_digit mu, *tmpx1, *tmpx2;

   /* m = digits in modulus */
   m = n->used;

   /* ensure that "x" has at least 2m digits */
   if (x->alloc < (m + m)) {
      if ((err = mp_grow(x, m + m)) != MP_OKAY) {
         return err;
      }
   }

   /* top of loop, this is where the code resumes if
    * another reduction pass is required.
    */
top:
   /* aliases for digits */
   /* alias for lower half of x */
   tmpx1 = x->dp;

   /* alias for upper half of x, or x/B**m */
   tmpx2 = x->dp + m;

   /* set carry to zero */
   mu = 0;

   /* compute (x mod B**m) + k * [x/B**m] inline and inplace */
   for (i = 0; i < m; i++) {
      r         = ((mp_word)*tmpx2++ * (mp_word)k) + *tmpx1 + mu;
      *tmpx1++  = (mp_digit)(r & MP_MASK);
      mu        = (mp_digit)(r >> ((mp_word)MP_DIGIT_BIT));
   }

   /* set final carry */
   *tmpx1++ = mu;

   /* zero words above m */
   MP_ZERO_DIGITS(tmpx1, (x->used - m) - 1);

   /* clamp, sub and return */
   mp_clamp(x);

   /* if x >= n then subtract and reduce again
    * Each successive "recursion" makes the input smaller and smaller.
    */
   if (mp_cmp_mag(x, n) != MP_LT) {
      if ((err = s_mp_sub(x, n, x)) != MP_OKAY) {
         return err;
      }
      goto top;
   }
   return MP_OKAY;
}
#endif
