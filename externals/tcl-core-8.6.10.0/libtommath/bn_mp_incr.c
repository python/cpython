#include "tommath_private.h"
#ifdef BN_MP_INCR_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Increment "a" by one like "a++". Changes input! */
mp_err mp_incr(mp_int *a)
{
   if (MP_IS_ZERO(a)) {
      mp_set(a,1uL);
      return MP_OKAY;
   } else if (a->sign == MP_NEG) {
      mp_err err;
      a->sign = MP_ZPOS;
      if ((err = mp_decr(a)) != MP_OKAY) {
         return err;
      }
      /* There is no -0 in LTM */
      if (!MP_IS_ZERO(a)) {
         a->sign = MP_NEG;
      }
      return MP_OKAY;
   } else if (a->dp[0] < MP_DIGIT_MAX) {
      a->dp[0]++;
      return MP_OKAY;
   } else {
      return mp_add_d(a, 1uL,a);
   }
}
#endif
