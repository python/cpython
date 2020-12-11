#include "tommath_private.h"
#ifdef BN_S_MP_SQR_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* low level squaring, b = a*a, HAC pp.596-597, Algorithm 14.16 */
mp_err s_mp_sqr(const mp_int *a, mp_int *b)
{
   mp_int   t;
   int      ix, iy, pa;
   mp_err   err;
   mp_word  r;
   mp_digit u, tmpx, *tmpt;

   pa = a->used;
   if ((err = mp_init_size(&t, (2 * pa) + 1)) != MP_OKAY) {
      return err;
   }

   /* default used is maximum possible size */
   t.used = (2 * pa) + 1;

   for (ix = 0; ix < pa; ix++) {
      /* first calculate the digit at 2*ix */
      /* calculate double precision result */
      r = (mp_word)t.dp[2*ix] +
          ((mp_word)a->dp[ix] * (mp_word)a->dp[ix]);

      /* store lower part in result */
      t.dp[ix+ix] = (mp_digit)(r & (mp_word)MP_MASK);

      /* get the carry */
      u           = (mp_digit)(r >> (mp_word)MP_DIGIT_BIT);

      /* left hand side of A[ix] * A[iy] */
      tmpx        = a->dp[ix];

      /* alias for where to store the results */
      tmpt        = t.dp + ((2 * ix) + 1);

      for (iy = ix + 1; iy < pa; iy++) {
         /* first calculate the product */
         r       = (mp_word)tmpx * (mp_word)a->dp[iy];

         /* now calculate the double precision result, note we use
          * addition instead of *2 since it's easier to optimize
          */
         r       = (mp_word)*tmpt + r + r + (mp_word)u;

         /* store lower part */
         *tmpt++ = (mp_digit)(r & (mp_word)MP_MASK);

         /* get carry */
         u       = (mp_digit)(r >> (mp_word)MP_DIGIT_BIT);
      }
      /* propagate upwards */
      while (u != 0uL) {
         r       = (mp_word)*tmpt + (mp_word)u;
         *tmpt++ = (mp_digit)(r & (mp_word)MP_MASK);
         u       = (mp_digit)(r >> (mp_word)MP_DIGIT_BIT);
      }
   }

   mp_clamp(&t);
   mp_exch(&t, b);
   mp_clear(&t);
   return MP_OKAY;
}
#endif
