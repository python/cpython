#include "tommath_private.h"
#ifdef BN_MP_EXPT_U32_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* calculate c = a**b  using a square-multiply algorithm */
mp_err mp_expt_u32(const mp_int *a, unsigned int b, mp_int *c)
{
   mp_err err;

   mp_int  g;

   if ((err = mp_init_copy(&g, a)) != MP_OKAY) {
      return err;
   }

   /* set initial result */
   mp_set(c, 1uL);

   while (b > 0u) {
      /* if the bit is set multiply */
      if ((b & 1u) != 0u) {
         if ((err = mp_mul(c, &g, c)) != MP_OKAY) {
            goto LBL_ERR;
         }
      }

      /* square */
      if (b > 1u) {
         if ((err = mp_sqr(&g, &g)) != MP_OKAY) {
            goto LBL_ERR;
         }
      }

      /* shift to next bit */
      b >>= 1;
   }

   err = MP_OKAY;

LBL_ERR:
   mp_clear(&g);
   return err;
}

#endif
