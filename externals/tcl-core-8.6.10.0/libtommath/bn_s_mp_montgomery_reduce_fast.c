#include "tommath_private.h"
#ifdef BN_S_MP_MONTGOMERY_REDUCE_FAST_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* computes xR**-1 == x (mod N) via Montgomery Reduction
 *
 * This is an optimized implementation of montgomery_reduce
 * which uses the comba method to quickly calculate the columns of the
 * reduction.
 *
 * Based on Algorithm 14.32 on pp.601 of HAC.
*/
mp_err s_mp_montgomery_reduce_fast(mp_int *x, const mp_int *n, mp_digit rho)
{
   int     ix, olduse;
   mp_err  err;
   mp_word W[MP_WARRAY];

   if (x->used > MP_WARRAY) {
      return MP_VAL;
   }

   /* get old used count */
   olduse = x->used;

   /* grow a as required */
   if (x->alloc < (n->used + 1)) {
      if ((err = mp_grow(x, n->used + 1)) != MP_OKAY) {
         return err;
      }
   }

   /* first we have to get the digits of the input into
    * an array of double precision words W[...]
    */
   {
      mp_word *_W;
      mp_digit *tmpx;

      /* alias for the W[] array */
      _W   = W;

      /* alias for the digits of  x*/
      tmpx = x->dp;

      /* copy the digits of a into W[0..a->used-1] */
      for (ix = 0; ix < x->used; ix++) {
         *_W++ = *tmpx++;
      }

      /* zero the high words of W[a->used..m->used*2] */
      if (ix < ((n->used * 2) + 1)) {
         MP_ZERO_BUFFER(_W, sizeof(mp_word) * (size_t)(((n->used * 2) + 1) - ix));
      }
   }

   /* now we proceed to zero successive digits
    * from the least significant upwards
    */
   for (ix = 0; ix < n->used; ix++) {
      /* mu = ai * m' mod b
       *
       * We avoid a double precision multiplication (which isn't required)
       * by casting the value down to a mp_digit.  Note this requires
       * that W[ix-1] have  the carry cleared (see after the inner loop)
       */
      mp_digit mu;
      mu = ((W[ix] & MP_MASK) * rho) & MP_MASK;

      /* a = a + mu * m * b**i
       *
       * This is computed in place and on the fly.  The multiplication
       * by b**i is handled by offseting which columns the results
       * are added to.
       *
       * Note the comba method normally doesn't handle carries in the
       * inner loop In this case we fix the carry from the previous
       * column since the Montgomery reduction requires digits of the
       * result (so far) [see above] to work.  This is
       * handled by fixing up one carry after the inner loop.  The
       * carry fixups are done in order so after these loops the
       * first m->used words of W[] have the carries fixed
       */
      {
         int iy;
         mp_digit *tmpn;
         mp_word *_W;

         /* alias for the digits of the modulus */
         tmpn = n->dp;

         /* Alias for the columns set by an offset of ix */
         _W = W + ix;

         /* inner loop */
         for (iy = 0; iy < n->used; iy++) {
            *_W++ += (mp_word)mu * (mp_word)*tmpn++;
         }
      }

      /* now fix carry for next digit, W[ix+1] */
      W[ix + 1] += W[ix] >> (mp_word)MP_DIGIT_BIT;
   }

   /* now we have to propagate the carries and
    * shift the words downward [all those least
    * significant digits we zeroed].
    */
   {
      mp_digit *tmpx;
      mp_word *_W, *_W1;

      /* nox fix rest of carries */

      /* alias for current word */
      _W1 = W + ix;

      /* alias for next word, where the carry goes */
      _W = W + ++ix;

      for (; ix < ((n->used * 2) + 1); ix++) {
         *_W++ += *_W1++ >> (mp_word)MP_DIGIT_BIT;
      }

      /* copy out, A = A/b**n
       *
       * The result is A/b**n but instead of converting from an
       * array of mp_word to mp_digit than calling mp_rshd
       * we just copy them in the right order
       */

      /* alias for destination word */
      tmpx = x->dp;

      /* alias for shifted double precision result */
      _W = W + n->used;

      for (ix = 0; ix < (n->used + 1); ix++) {
         *tmpx++ = *_W++ & (mp_word)MP_MASK;
      }

      /* zero oldused digits, if the input a was larger than
       * m->used+1 we'll have to clear the digits
       */
      MP_ZERO_DIGITS(tmpx, olduse - ix);
   }

   /* set the max used and clamp */
   x->used = n->used + 1;
   mp_clamp(x);

   /* if A >= m then A = A - m */
   if (mp_cmp_mag(x, n) != MP_LT) {
      return s_mp_sub(x, n, x);
   }
   return MP_OKAY;
}
#endif
