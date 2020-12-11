#include "tommath_private.h"
#ifdef BN_MP_ZERO_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* set to zero */
void mp_zero(mp_int *a)
{
   a->sign = MP_ZPOS;
   a->used = 0;
   MP_ZERO_DIGITS(a->dp, a->alloc);
}
#endif
