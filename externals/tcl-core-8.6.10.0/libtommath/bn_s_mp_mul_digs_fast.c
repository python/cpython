#include "tommath_private.h"
#ifdef BN_S_MP_MUL_DIGS_FAST_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Fast (comba) multiplier
 *
 * This is the fast column-array [comba] multiplier.  It is
 * designed to compute the columns of the product first
 * then handle the carries afterwards.  This has the effect
 * of making the nested loops that compute the columns very
 * simple and schedulable on super-scalar processors.
 *
 * This has been modified to produce a variable number of
 * digits of output so if say only a half-product is required
 * you don't have to compute the upper half (a feature
 * required for fast Barrett reduction).
 *
 * Based on Algorithm 14.12 on pp.595 of HAC.
 *
 */
mp_err s_mp_mul_digs_fast(const mp_int *a, const mp_int *b, mp_int *c, int digs)
{
   int      olduse, pa, ix, iz;
   mp_err   err;
   mp_digit W[MP_WARRAY];
   mp_word  _W;

   /* grow the destination as required */
   if (c->alloc < digs) {
      if ((err = mp_grow(c, digs)) != MP_OKAY) {
         return err;
      }
   }

   /* number of output digits to produce */
   pa = MP_MIN(digs, a->used + b->used);

   /* clear the carry */
   _W = 0;
   for (ix = 0; ix < pa; ix++) {
      int      tx, ty;
      int      iy;
      mp_digit *tmpx, *tmpy;

      /* get offsets into the two bignums */
      ty = MP_MIN(b->used-1, ix);
      tx = ix - ty;

      /* setup temp aliases */
      tmpx = a->dp + tx;
      tmpy = b->dp + ty;

      /* this is the number of times the loop will iterrate, essentially
         while (tx++ < a->used && ty-- >= 0) { ... }
       */
      iy = MP_MIN(a->used-tx, ty+1);

      /* execute loop */
      for (iz = 0; iz < iy; ++iz) {
         _W += (mp_word)*tmpx++ * (mp_word)*tmpy--;

      }

      /* store term */
      W[ix] = (mp_digit)_W & MP_MASK;

      /* make next carry */
      _W = _W >> (mp_word)MP_DIGIT_BIT;
   }

   /* setup dest */
   olduse  = c->used;
   c->used = pa;

   {
      mp_digit *tmpc;
      tmpc = c->dp;
      for (ix = 0; ix < pa; ix++) {
         /* now extract the previous digit [below the carry] */
         *tmpc++ = W[ix];
      }

      /* clear unused digits [that existed in the old copy of c] */
      MP_ZERO_DIGITS(tmpc, olduse - ix);
   }
   mp_clamp(c);
   return MP_OKAY;
}
#endif
