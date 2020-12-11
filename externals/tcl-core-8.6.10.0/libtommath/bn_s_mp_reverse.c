#include "tommath_private.h"
#ifdef BN_S_MP_REVERSE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* reverse an array, used for radix code */
void s_mp_reverse(unsigned char *s, size_t len)
{
   size_t   ix, iy;
   unsigned char t;

   ix = 0u;
   iy = len - 1u;
   while (ix < iy) {
      t     = s[ix];
      s[ix] = s[iy];
      s[iy] = t;
      ++ix;
      --iy;
   }
}
#endif
