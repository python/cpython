#include "tommath_private.h"
#ifdef BN_MP_LSHD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* shift left a certain amount of digits */
mp_err mp_lshd(mp_int *a, int b)
{
   int x;
   mp_err err;
   mp_digit *top, *bottom;

   /* if its less than zero return */
   if (b <= 0) {
      return MP_OKAY;
   }
   /* no need to shift 0 around */
   if (MP_IS_ZERO(a)) {
      return MP_OKAY;
   }

   /* grow to fit the new digits */
   if (a->alloc < (a->used + b)) {
      if ((err = mp_grow(a, a->used + b)) != MP_OKAY) {
         return err;
      }
   }

   /* increment the used by the shift amount then copy upwards */
   a->used += b;

   /* top */
   top = a->dp + a->used - 1;

   /* base */
   bottom = (a->dp + a->used - 1) - b;

   /* much like mp_rshd this is implemented using a sliding window
    * except the window goes the otherway around.  Copying from
    * the bottom to the top.  see bn_mp_rshd.c for more info.
    */
   for (x = a->used - 1; x >= b; x--) {
      *top-- = *bottom--;
   }

   /* zero the lower digits */
   MP_ZERO_DIGITS(a->dp, b);

   return MP_OKAY;
}
#endif
