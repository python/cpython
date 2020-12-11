#include "tommath_private.h"
#ifdef BN_S_MP_MUL_DIGS_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* multiplies |a| * |b| and only computes upto digs digits of result
 * HAC pp. 595, Algorithm 14.12  Modified so you can control how
 * many digits of output are created.
 */
mp_err s_mp_mul_digs(const mp_int *a, const mp_int *b, mp_int *c, int digs)
{
   mp_int  t;
   mp_err  err;
   int     pa, pb, ix, iy;
   mp_digit u;
   mp_word r;
   mp_digit tmpx, *tmpt, *tmpy;

   /* can we use the fast multiplier? */
   if ((digs < MP_WARRAY) &&
       (MP_MIN(a->used, b->used) < MP_MAXFAST)) {
      return s_mp_mul_digs_fast(a, b, c, digs);
   }

   if ((err = mp_init_size(&t, digs)) != MP_OKAY) {
      return err;
   }
   t.used = digs;

   /* compute the digits of the product directly */
   pa = a->used;
   for (ix = 0; ix < pa; ix++) {
      /* set the carry to zero */
      u = 0;

      /* limit ourselves to making digs digits of output */
      pb = MP_MIN(b->used, digs - ix);

      /* setup some aliases */
      /* copy of the digit from a used within the nested loop */
      tmpx = a->dp[ix];

      /* an alias for the destination shifted ix places */
      tmpt = t.dp + ix;

      /* an alias for the digits of b */
      tmpy = b->dp;

      /* compute the columns of the output and propagate the carry */
      for (iy = 0; iy < pb; iy++) {
         /* compute the column as a mp_word */
         r       = (mp_word)*tmpt +
                   ((mp_word)tmpx * (mp_word)*tmpy++) +
                   (mp_word)u;

         /* the new column is the lower part of the result */
         *tmpt++ = (mp_digit)(r & (mp_word)MP_MASK);

         /* get the carry word from the result */
         u       = (mp_digit)(r >> (mp_word)MP_DIGIT_BIT);
      }
      /* set carry if it is placed below digs */
      if ((ix + iy) < digs) {
         *tmpt = u;
      }
   }

   mp_clamp(&t);
   mp_exch(&t, c);

   mp_clear(&t);
   return MP_OKAY;
}
#endif
