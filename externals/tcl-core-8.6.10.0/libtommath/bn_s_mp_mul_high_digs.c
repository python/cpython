#include "tommath_private.h"
#ifdef BN_S_MP_MUL_HIGH_DIGS_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* multiplies |a| * |b| and does not compute the lower digs digits
 * [meant to get the higher part of the product]
 */
mp_err s_mp_mul_high_digs(const mp_int *a, const mp_int *b, mp_int *c, int digs)
{
   mp_int   t;
   int      pa, pb, ix, iy;
   mp_err   err;
   mp_digit u;
   mp_word  r;
   mp_digit tmpx, *tmpt, *tmpy;

   /* can we use the fast multiplier? */
   if (MP_HAS(S_MP_MUL_HIGH_DIGS_FAST)
       && ((a->used + b->used + 1) < MP_WARRAY)
       && (MP_MIN(a->used, b->used) < MP_MAXFAST)) {
      return s_mp_mul_high_digs_fast(a, b, c, digs);
   }

   if ((err = mp_init_size(&t, a->used + b->used + 1)) != MP_OKAY) {
      return err;
   }
   t.used = a->used + b->used + 1;

   pa = a->used;
   pb = b->used;
   for (ix = 0; ix < pa; ix++) {
      /* clear the carry */
      u = 0;

      /* left hand side of A[ix] * B[iy] */
      tmpx = a->dp[ix];

      /* alias to the address of where the digits will be stored */
      tmpt = &(t.dp[digs]);

      /* alias for where to read the right hand side from */
      tmpy = b->dp + (digs - ix);

      for (iy = digs - ix; iy < pb; iy++) {
         /* calculate the double precision result */
         r       = (mp_word)*tmpt +
                   ((mp_word)tmpx * (mp_word)*tmpy++) +
                   (mp_word)u;

         /* get the lower part */
         *tmpt++ = (mp_digit)(r & (mp_word)MP_MASK);

         /* carry the carry */
         u       = (mp_digit)(r >> (mp_word)MP_DIGIT_BIT);
      }
      *tmpt = u;
   }
   mp_clamp(&t);
   mp_exch(&t, c);
   mp_clear(&t);
   return MP_OKAY;
}
#endif
