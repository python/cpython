#include "tommath_private.h"
#ifdef BN_MP_EXCH_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* swap the elements of two integers, for cases where you can't simply swap the
 * mp_int pointers around
 */
void mp_exch(mp_int *a, mp_int *b)
{
   mp_int  t;

   t  = *a;
   *a = *b;
   *b = t;
}
#endif
