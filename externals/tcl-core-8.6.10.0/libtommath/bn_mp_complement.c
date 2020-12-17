#include "tommath_private.h"
#ifdef BN_MP_COMPLEMENT_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* b = ~a */
mp_err mp_complement(const mp_int *a, mp_int *b)
{
   mp_err err = mp_neg(a, b);
   return (err == MP_OKAY) ? mp_sub_d(b, 1uL, b) : err;
}
#endif
