#include "tommath_private.h"
#ifdef BN_MP_LCM_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* computes least common multiple as |a*b|/(a, b) */
mp_err mp_lcm(const mp_int *a, const mp_int *b, mp_int *c)
{
   mp_err  err;
   mp_int  t1, t2;


   if ((err = mp_init_multi(&t1, &t2, NULL)) != MP_OKAY) {
      return err;
   }

   /* t1 = get the GCD of the two inputs */
   if ((err = mp_gcd(a, b, &t1)) != MP_OKAY) {
      goto LBL_T;
   }

   /* divide the smallest by the GCD */
   if (mp_cmp_mag(a, b) == MP_LT) {
      /* store quotient in t2 such that t2 * b is the LCM */
      if ((err = mp_div(a, &t1, &t2, NULL)) != MP_OKAY) {
         goto LBL_T;
      }
      err = mp_mul(b, &t2, c);
   } else {
      /* store quotient in t2 such that t2 * a is the LCM */
      if ((err = mp_div(b, &t1, &t2, NULL)) != MP_OKAY) {
         goto LBL_T;
      }
      err = mp_mul(a, &t2, c);
   }

   /* fix the sign to positive */
   c->sign = MP_ZPOS;

LBL_T:
   mp_clear_multi(&t1, &t2, NULL);
   return err;
}
#endif
