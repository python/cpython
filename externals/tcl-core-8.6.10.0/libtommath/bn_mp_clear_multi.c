#include "tommath_private.h"
#ifdef BN_MP_CLEAR_MULTI_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#include <stdarg.h>

void mp_clear_multi(mp_int *mp, ...)
{
   mp_int *next_mp = mp;
   va_list args;
   va_start(args, mp);
   while (next_mp != NULL) {
      mp_clear(next_mp);
      next_mp = va_arg(args, mp_int *);
   }
   va_end(args);
}
#endif
