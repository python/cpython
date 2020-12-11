#include "tommath_private.h"
#ifdef BN_MP_ADD_D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* single digit addition */
mp_err mp_add_d(const mp_int *a, mp_digit b, mp_int *c)
{
   mp_err     err;
   int ix, oldused;
   mp_digit *tmpa, *tmpc;

   /* grow c as required */
   if (c->alloc < (a->used + 1)) {
      if ((err = mp_grow(c, a->used + 1)) != MP_OKAY) {
         return err;
      }
   }

   /* if a is negative and |a| >= b, call c = |a| - b */
   if ((a->sign == MP_NEG) && ((a->used > 1) || (a->dp[0] >= b))) {
      mp_int a_ = *a;
      /* temporarily fix sign of a */
      a_.sign = MP_ZPOS;

      /* c = |a| - b */
      err = mp_sub_d(&a_, b, c);

      /* fix sign  */
      c->sign = MP_NEG;

      /* clamp */
      mp_clamp(c);

      return err;
   }

   /* old number of used digits in c */
   oldused = c->used;

   /* source alias */
   tmpa    = a->dp;

   /* destination alias */
   tmpc    = c->dp;

   /* if a is positive */
   if (a->sign == MP_ZPOS) {
      /* add digits, mu is carry */
      mp_digit mu = b;
      for (ix = 0; ix < a->used; ix++) {
         *tmpc   = *tmpa++ + mu;
         mu      = *tmpc >> MP_DIGIT_BIT;
         *tmpc++ &= MP_MASK;
      }
      /* set final carry */
      ix++;
      *tmpc++  = mu;

      /* setup size */
      c->used = a->used + 1;
   } else {
      /* a was negative and |a| < b */
      c->used  = 1;

      /* the result is a single digit */
      if (a->used == 1) {
         *tmpc++  =  b - a->dp[0];
      } else {
         *tmpc++  =  b;
      }

      /* setup count so the clearing of oldused
       * can fall through correctly
       */
      ix       = 1;
   }

   /* sign always positive */
   c->sign = MP_ZPOS;

   /* now zero to oldused */
   MP_ZERO_DIGITS(tmpc, oldused - ix);
   mp_clamp(c);

   return MP_OKAY;
}

#endif
