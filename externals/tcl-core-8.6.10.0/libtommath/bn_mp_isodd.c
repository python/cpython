#include "tommath_private.h"
#ifdef BN_MP_ISODD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

mp_bool mp_isodd(const mp_int *a)
{
   return MP_IS_ODD(a) ? MP_YES : MP_NO;
}
#endif
