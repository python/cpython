#include "tommath_private.h"
#ifdef BN_MP_DIV_3_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* divide by three (based on routine from MPI and the GMP manual) */
mp_err mp_div_3(const mp_int *a, mp_int *c, mp_digit *d)
{
   mp_int   q;
   mp_word  w, t;
   mp_digit b;
   mp_err   err;
   int      ix;

   /* b = 2**MP_DIGIT_BIT / 3 */
   b = ((mp_word)1 << (mp_word)MP_DIGIT_BIT) / (mp_word)3;

   if ((err = mp_init_size(&q, a->used)) != MP_OKAY) {
      return err;
   }

   q.used = a->used;
   q.sign = a->sign;
   w = 0;
   for (ix = a->used - 1; ix >= 0; ix--) {
      w = (w << (mp_word)MP_DIGIT_BIT) | (mp_word)a->dp[ix];

      if (w >= 3u) {
         /* multiply w by [1/3] */
         t = (w * (mp_word)b) >> (mp_word)MP_DIGIT_BIT;

         /* now subtract 3 * [w/3] from w, to get the remainder */
         w -= t+t+t;

         /* fixup the remainder as required since
          * the optimization is not exact.
          */
         while (w >= 3u) {
            t += 1u;
            w -= 3u;
         }
      } else {
         t = 0;
      }
      q.dp[ix] = (mp_digit)t;
   }

   /* [optional] store the remainder */
   if (d != NULL) {
      *d = (mp_digit)w;
   }

   /* [optional] store the quotient */
   if (c != NULL) {
      mp_clamp(&q);
      mp_exch(&q, c);
   }
   mp_clear(&q);

   return err;
}

#endif
