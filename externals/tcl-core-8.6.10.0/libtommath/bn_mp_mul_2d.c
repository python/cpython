#include "tommath_private.h"
#ifdef BN_MP_MUL_2D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* shift left by a certain bit count */
mp_err mp_mul_2d(const mp_int *a, int b, mp_int *c)
{
   mp_digit d;
   mp_err   err;

   /* copy */
   if (a != c) {
      if ((err = mp_copy(a, c)) != MP_OKAY) {
         return err;
      }
   }

   if (c->alloc < (c->used + (b / MP_DIGIT_BIT) + 1)) {
      if ((err = mp_grow(c, c->used + (b / MP_DIGIT_BIT) + 1)) != MP_OKAY) {
         return err;
      }
   }

   /* shift by as many digits in the bit count */
   if (b >= MP_DIGIT_BIT) {
      if ((err = mp_lshd(c, b / MP_DIGIT_BIT)) != MP_OKAY) {
         return err;
      }
   }

   /* shift any bit count < MP_DIGIT_BIT */
   d = (mp_digit)(b % MP_DIGIT_BIT);
   if (d != 0u) {
      mp_digit *tmpc, shift, mask, r, rr;
      int x;

      /* bitmask for carries */
      mask = ((mp_digit)1 << d) - (mp_digit)1;

      /* shift for msbs */
      shift = (mp_digit)MP_DIGIT_BIT - d;

      /* alias */
      tmpc = c->dp;

      /* carry */
      r    = 0;
      for (x = 0; x < c->used; x++) {
         /* get the higher bits of the current word */
         rr = (*tmpc >> shift) & mask;

         /* shift the current word and OR in the carry */
         *tmpc = ((*tmpc << d) | r) & MP_MASK;
         ++tmpc;

         /* set the carry to the carry bits of the current word */
         r = rr;
      }

      /* set final carry */
      if (r != 0u) {
         c->dp[(c->used)++] = r;
      }
   }
   mp_clamp(c);
   return MP_OKAY;
}
#endif
