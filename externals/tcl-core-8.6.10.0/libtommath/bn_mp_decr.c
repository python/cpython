#include "tommath_private.h"
#ifdef BN_MP_DECR_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Decrement "a" by one like "a--". Changes input! */
mp_err mp_decr(mp_int *a)
{
   if (MP_IS_ZERO(a)) {
      mp_set(a,1uL);
      a->sign = MP_NEG;
      return MP_OKAY;
   } else if (a->sign == MP_NEG) {
      mp_err err;
      a->sign = MP_ZPOS;
      if ((err = mp_incr(a)) != MP_OKAY) {
         return err;
      }
      /* There is no -0 in LTM */
      if (!MP_IS_ZERO(a)) {
         a->sign = MP_NEG;
      }
      return MP_OKAY;
   } else if (a->dp[0] > 1uL) {
      a->dp[0]--;
      if (a->dp[0] == 0u) {
         mp_zero(a);
      }
      return MP_OKAY;
   } else {
      return mp_sub_d(a, 1uL,a);
   }
}
#endif
