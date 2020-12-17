#include "tommath_private.h"
#ifdef BN_MP_INVMOD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* hac 14.61, pp608 */
mp_err mp_invmod(const mp_int *a, const mp_int *b, mp_int *c)
{
   /* b cannot be negative and has to be >1 */
   if ((b->sign == MP_NEG) || (mp_cmp_d(b, 1uL) != MP_GT)) {
      return MP_VAL;
   }

   /* if the modulus is odd we can use a faster routine instead */
   if (MP_HAS(S_MP_INVMOD_FAST) && MP_IS_ODD(b)) {
      return s_mp_invmod_fast(a, b, c);
   }

   return MP_HAS(S_MP_INVMOD_SLOW)
          ? s_mp_invmod_slow(a, b, c)
          : MP_VAL;
}
#endif
