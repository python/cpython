#include "tommath_private.h"
#ifdef BN_MP_MOD_D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

mp_err mp_mod_d(const mp_int *a, mp_digit b, mp_digit *c)
{
   return mp_div_d(a, b, NULL, c);
}
#endif
