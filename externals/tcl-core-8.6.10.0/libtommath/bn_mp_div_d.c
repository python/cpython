#include "tommath_private.h"
#ifdef BN_MP_DIV_D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* single digit division (based on routine from MPI) */
mp_err mp_div_d(const mp_int *a, mp_digit b, mp_int *c, mp_digit *d)
{
   mp_int  q;
   mp_word w;
   mp_digit t;
   mp_err err;
   int ix;

   /* cannot divide by zero */
   if (b == 0u) {
      return MP_VAL;
   }

   /* quick outs */
   if ((b == 1u) || MP_IS_ZERO(a)) {
      if (d != NULL) {
         *d = 0;
      }
      if (c != NULL) {
         return mp_copy(a, c);
      }
      return MP_OKAY;
   }

   /* power of two ? */
   if ((b & (b - 1u)) == 0u) {
      ix = 1;
      while ((ix < MP_DIGIT_BIT) && (b != (((mp_digit)1)<<ix))) {
         ix++;
      }
      if (d != NULL) {
         *d = a->dp[0] & (((mp_digit)1<<(mp_digit)ix) - 1uL);
      }
      if (c != NULL) {
         return mp_div_2d(a, ix, c, NULL);
      }
      return MP_OKAY;
   }

   /* three? */
   if (MP_HAS(MP_DIV_3) && (b == 3u)) {
      return mp_div_3(a, c, d);
   }

   /* no easy answer [c'est la vie].  Just division */
   if ((err = mp_init_size(&q, a->used)) != MP_OKAY) {
      return err;
   }

   q.used = a->used;
   q.sign = a->sign;
   w = 0;
   for (ix = a->used - 1; ix >= 0; ix--) {
      w = (w << (mp_word)MP_DIGIT_BIT) | (mp_word)a->dp[ix];

      if (w >= b) {
         t = (mp_digit)(w / b);
         w -= (mp_word)t * (mp_word)b;
      } else {
         t = 0;
      }
      q.dp[ix] = t;
   }

   if (d != NULL) {
      *d = (mp_digit)w;
   }

   if (c != NULL) {
      mp_clamp(&q);
      mp_exch(&q, c);
   }
   mp_clear(&q);

   return err;
}

#endif
