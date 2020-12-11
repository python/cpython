#include "tommath_private.h"
#ifdef BN_MP_OR_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* two complement or */
mp_err mp_or(const mp_int *a, const mp_int *b, mp_int *c)
{
   int used = MP_MAX(a->used, b->used) + 1, i;
   mp_err err;
   mp_digit ac = 1, bc = 1, cc = 1;
   mp_sign csign = ((a->sign == MP_NEG) || (b->sign == MP_NEG)) ? MP_NEG : MP_ZPOS;

   if (c->alloc < used) {
      if ((err = mp_grow(c, used)) != MP_OKAY) {
         return err;
      }
   }

   for (i = 0; i < used; i++) {
      mp_digit x, y;

      /* convert to two complement if negative */
      if (a->sign == MP_NEG) {
         ac += (i >= a->used) ? MP_MASK : (~a->dp[i] & MP_MASK);
         x = ac & MP_MASK;
         ac >>= MP_DIGIT_BIT;
      } else {
         x = (i >= a->used) ? 0uL : a->dp[i];
      }

      /* convert to two complement if negative */
      if (b->sign == MP_NEG) {
         bc += (i >= b->used) ? MP_MASK : (~b->dp[i] & MP_MASK);
         y = bc & MP_MASK;
         bc >>= MP_DIGIT_BIT;
      } else {
         y = (i >= b->used) ? 0uL : b->dp[i];
      }

      c->dp[i] = x | y;

      /* convert to to sign-magnitude if negative */
      if (csign == MP_NEG) {
         cc += ~c->dp[i] & MP_MASK;
         c->dp[i] = cc & MP_MASK;
         cc >>= MP_DIGIT_BIT;
      }
   }

   c->used = used;
   c->sign = csign;
   mp_clamp(c);
   return MP_OKAY;
}
#endif
