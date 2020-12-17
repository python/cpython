#include "tommath_private.h"
#ifdef BN_MP_REDUCE_2K_SETUP_L_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* determines the setup value */
mp_err mp_reduce_2k_setup_l(const mp_int *a, mp_int *d)
{
   mp_err err;
   mp_int tmp;

   if ((err = mp_init(&tmp)) != MP_OKAY) {
      return err;
   }

   if ((err = mp_2expt(&tmp, mp_count_bits(a))) != MP_OKAY) {
      goto LBL_ERR;
   }

   if ((err = s_mp_sub(&tmp, a, d)) != MP_OKAY) {
      goto LBL_ERR;
   }

LBL_ERR:
   mp_clear(&tmp);
   return err;
}
#endif
