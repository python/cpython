#include "tommath_private.h"
#ifdef BN_MP_CMP_MAG_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* compare maginitude of two ints (unsigned) */
mp_ord mp_cmp_mag(const mp_int *a, const mp_int *b)
{
   int     n;
   const mp_digit *tmpa, *tmpb;

   /* compare based on # of non-zero digits */
   if (a->used > b->used) {
      return MP_GT;
   }

   if (a->used < b->used) {
      return MP_LT;
   }

   /* alias for a */
   tmpa = a->dp + (a->used - 1);

   /* alias for b */
   tmpb = b->dp + (a->used - 1);

   /* compare based on digits  */
   for (n = 0; n < a->used; ++n, --tmpa, --tmpb) {
      if (*tmpa > *tmpb) {
         return MP_GT;
      }

      if (*tmpa < *tmpb) {
         return MP_LT;
      }
   }
   return MP_EQ;
}
#endif
