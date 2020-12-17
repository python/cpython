#include "tommath_private.h"
#ifdef BN_MP_ABS_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* b = |a|
 *
 * Simple function copies the input and fixes the sign to positive
 */
mp_err mp_abs(const mp_int *a, mp_int *b)
{
   mp_err     err;

   /* copy a to b */
   if (a != b) {
      if ((err = mp_copy(a, b)) != MP_OKAY) {
         return err;
      }
   }

   /* force the sign of b to positive */
   b->sign = MP_ZPOS;

   return MP_OKAY;
}
#endif
