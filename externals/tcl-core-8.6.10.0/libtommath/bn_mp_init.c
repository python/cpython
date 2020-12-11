#include "tommath_private.h"
#ifdef BN_MP_INIT_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* init a new mp_int */
mp_err mp_init(mp_int *a)
{
   /* allocate memory required and clear it */
   a->dp = (mp_digit *) MP_CALLOC((size_t)MP_PREC, sizeof(mp_digit));
   if (a->dp == NULL) {
      return MP_MEM;
   }

   /* set the used to zero, allocated digits to the default precision
    * and sign to positive */
   a->used  = 0;
   a->alloc = MP_PREC;
   a->sign  = MP_ZPOS;

   return MP_OKAY;
}
#endif
