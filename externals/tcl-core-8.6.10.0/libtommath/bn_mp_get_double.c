#include "tommath_private.h"
#ifdef BN_MP_GET_DOUBLE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

double mp_get_double(const mp_int *a)
{
   int i;
   double d = 0.0, fac = 1.0;
   for (i = 0; i < MP_DIGIT_BIT; ++i) {
      fac *= 2.0;
   }
   for (i = a->used; i --> 0;) {
      d = (d * fac) + (double)a->dp[i];
   }
   return (a->sign == MP_NEG) ? -d : d;
}
#endif
