#include "tommath_private.h"
#ifdef BN_MP_RADIX_SIZE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* returns size of ASCII representation */
mp_err mp_radix_size(const mp_int *a, int radix, int *size)
{
   mp_err  err;
   int digs;
   mp_int   t;
   mp_digit d;

   *size = 0;

   /* make sure the radix is in range */
   if ((radix < 2) || (radix > 64)) {
      return MP_VAL;
   }

   if (MP_IS_ZERO(a)) {
      *size = 2;
      return MP_OKAY;
   }

   /* special case for binary */
   if (radix == 2) {
      *size = (mp_count_bits(a) + ((a->sign == MP_NEG) ? 1 : 0) + 1);
      return MP_OKAY;
   }

   /* digs is the digit count */
   digs = 0;

   /* if it's negative add one for the sign */
   if (a->sign == MP_NEG) {
      ++digs;
   }

   /* init a copy of the input */
   if ((err = mp_init_copy(&t, a)) != MP_OKAY) {
      return err;
   }

   /* force temp to positive */
   t.sign = MP_ZPOS;

   /* fetch out all of the digits */
   while (!MP_IS_ZERO(&t)) {
      if ((err = mp_div_d(&t, (mp_digit)radix, &t, &d)) != MP_OKAY) {
         goto LBL_ERR;
      }
      ++digs;
   }

   /* return digs + 1, the 1 is for the NULL byte that would be required. */
   *size = digs + 1;
   err = MP_OKAY;

LBL_ERR:
   mp_clear(&t);
   return err;
}

#endif
