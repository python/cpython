#include "tommath_private.h"
#ifdef BN_MP_REDUCE_SETUP_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* pre-calculate the value required for Barrett reduction
 * For a given modulus "b" it calulates the value required in "a"
 */
mp_err mp_reduce_setup(mp_int *a, const mp_int *b)
{
   mp_err err;
   if ((err = mp_2expt(a, b->used * 2 * MP_DIGIT_BIT)) != MP_OKAY) {
      return err;
   }
   return mp_div(a, b, a, NULL);
}
#endif
