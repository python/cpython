#include "tommath_private.h"
#ifdef BN_MP_TO_UBIN_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* store in unsigned [big endian] format */
mp_err mp_to_ubin(const mp_int *a, unsigned char *buf, size_t maxlen, size_t *written)
{
   size_t  x, count;
   mp_err  err;
   mp_int  t;

   size_t size = (size_t)mp_count_bits(a);
   count = (size / 8u) + (((size & 7u) != 0u) ? 1u : 0u);
   if (count > maxlen) {
      return MP_BUF;
   }

   if ((err = mp_init_copy(&t, a)) != MP_OKAY) {
      return err;
   }

   for (x = count; x --> 0u;) {
#ifndef MP_8BIT
      buf[x] = (unsigned char)(t.dp[0] & 255u);
#else
      buf[x] = (unsigned char)(t.dp[0] | ((t.dp[1] & 1u) << 7));
#endif
      if ((err = mp_div_2d(&t, 8, &t, NULL)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   if (written != NULL) {
      *written = count;
   }

LBL_ERR:
   mp_clear(&t);
   return err;
}
#endif
