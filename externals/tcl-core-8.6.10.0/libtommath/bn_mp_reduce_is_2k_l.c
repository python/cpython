#include "tommath_private.h"
#ifdef BN_MP_REDUCE_IS_2K_L_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* determines if reduce_2k_l can be used */
mp_bool mp_reduce_is_2k_l(const mp_int *a)
{
   int ix, iy;

   if (a->used == 0) {
      return MP_NO;
   } else if (a->used == 1) {
      return MP_YES;
   } else if (a->used > 1) {
      /* if more than half of the digits are -1 we're sold */
      for (iy = ix = 0; ix < a->used; ix++) {
         if (a->dp[ix] == MP_DIGIT_MAX) {
            ++iy;
         }
      }
      return (iy >= (a->used/2)) ? MP_YES : MP_NO;
   } else {
      return MP_NO;
   }
}

#endif
