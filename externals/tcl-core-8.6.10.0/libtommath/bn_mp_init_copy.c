#include "tommath_private.h"
#ifdef BN_MP_INIT_COPY_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* creates "a" then copies b into it */
mp_err mp_init_copy(mp_int *a, const mp_int *b)
{
   mp_err     err;

   if ((err = mp_init_size(a, b->used)) != MP_OKAY) {
      return err;
   }

   if ((err = mp_copy(b, a)) != MP_OKAY) {
      mp_clear(a);
   }

   return err;
}
#endif
