#include "tommath_private.h"
#ifdef BN_MP_SQRT_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#ifndef NO_FLOATING_POINT
#include <math.h>
#if (MP_DIGIT_BIT != 28) || (FLT_RADIX != 2) || (DBL_MANT_DIG != 53) || (DBL_MAX_EXP != 1024)
#define NO_FLOATING_POINT
#endif
#endif

/* this function is less generic than mp_n_root, simpler and faster */
mp_err mp_sqrt(const mp_int *arg, mp_int *ret)
{
   mp_err err;
   mp_int t1, t2;
#ifndef NO_FLOATING_POINT
   int i, j, k;
   volatile double d;
   mp_digit dig;
#endif

   /* must be positive */
   if (arg->sign == MP_NEG) {
      return MP_VAL;
   }

   /* easy out */
   if (MP_IS_ZERO(arg)) {
      mp_zero(ret);
      return MP_OKAY;
   }

#ifndef NO_FLOATING_POINT

   i = (arg->used / 2) - 1;
   j = 2 * i;
   if ((err = mp_init_size(&t1, i+2)) != MP_OKAY) {
      return err;
   }

   if ((err = mp_init(&t2)) != MP_OKAY) {
      goto E2;
   }

   for (k = 0; k < i; ++k) {
      t1.dp[k] = (mp_digit) 0;
   }

   /* Estimate the square root using the hardware floating point unit. */

   d = 0.0;
   for (k = arg->used-1; k >= j; --k) {
      d = ldexp(d, MP_DIGIT_BIT) + (double)(arg->dp[k]);
   }

   /*
    * At this point, d is the nearest floating point number to the most
    * significant 1 or 2 mp_digits of arg. Extract its square root.
    */

   d = sqrt(d);

   /* dig is the most significant mp_digit of the square root */

   dig = (mp_digit) ldexp(d, -MP_DIGIT_BIT);

   /*
    * If the most significant digit is nonzero, find the next digit down
    * by subtracting MP_DIGIT_BIT times thie most significant digit.
    * Subtract one from the result so that our initial estimate is always
    * low.
    */

   if (dig) {
      t1.used = i+2;
      d -= ldexp((double) dig, MP_DIGIT_BIT);
      if (d >= 1.0) {
         t1.dp[i+1] = dig;
         t1.dp[i] = ((mp_digit) d) - 1;
      } else {
         t1.dp[i+1] = dig-1;
         t1.dp[i] = MP_DIGIT_MAX;
      }
   } else {
      t1.used = i+1;
      t1.dp[i] = ((mp_digit) d) - 1;
   }

#else

   if ((err = mp_init_copy(&t1, arg)) != MP_OKAY) {
      return err;
   }

   if ((err = mp_init(&t2)) != MP_OKAY) {
      goto E2;
   }

   /* First approx. (not very bad for large arg) */
   mp_rshd(&t1, t1.used/2);

#endif

   /* t1 > 0  */
   if ((err = mp_div(arg, &t1, &t2, NULL)) != MP_OKAY) {
      goto E1;
   }
   if ((err = mp_add(&t1, &t2, &t1)) != MP_OKAY) {
      goto E1;
   }
   if ((err = mp_div_2(&t1, &t1)) != MP_OKAY) {
      goto E1;
   }
   /* And now t1 > sqrt(arg) */
   do {
      if ((err = mp_div(arg, &t1, &t2, NULL)) != MP_OKAY) {
         goto E1;
      }
      if ((err = mp_add(&t1, &t2, &t1)) != MP_OKAY) {
         goto E1;
      }
      if ((err = mp_div_2(&t1, &t1)) != MP_OKAY) {
         goto E1;
      }
      /* t1 >= sqrt(arg) >= t2 at this point */
   } while (mp_cmp_mag(&t1, &t2) == MP_GT);

   mp_exch(&t1, ret);

E1:
   mp_clear(&t2);
E2:
   mp_clear(&t1);
   return err;
}

#endif
