#include "tommath_private.h"
#ifdef BN_MP_SET_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* set to a digit */
void mp_set(mp_int *a, mp_digit b)
{
   a->dp[0] = b & MP_MASK;
   a->sign  = MP_ZPOS;
   a->used  = (a->dp[0] != 0u) ? 1 : 0;
   MP_ZERO_DIGITS(a->dp + a->used, a->alloc - a->used);
}
#endif
