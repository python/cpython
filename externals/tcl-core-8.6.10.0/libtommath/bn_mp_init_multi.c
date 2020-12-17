#include "tommath_private.h"
#ifdef BN_MP_INIT_MULTI_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#include <stdarg.h>

mp_err mp_init_multi(mp_int *mp, ...)
{
   mp_err err = MP_OKAY;      /* Assume ok until proven otherwise */
   int n = 0;                 /* Number of ok inits */
   mp_int *cur_arg = mp;
   va_list args;

   va_start(args, mp);        /* init args to next argument from caller */
   while (cur_arg != NULL) {
      if (mp_init(cur_arg) != MP_OKAY) {
         /* Oops - error! Back-track and mp_clear what we already
            succeeded in init-ing, then return error.
         */
         va_list clean_args;

         /* now start cleaning up */
         cur_arg = mp;
         va_start(clean_args, mp);
         while (n-- != 0) {
            mp_clear(cur_arg);
            cur_arg = va_arg(clean_args, mp_int *);
         }
         va_end(clean_args);
         err = MP_MEM;
         break;
      }
      n++;
      cur_arg = va_arg(args, mp_int *);
   }
   va_end(args);
   return err;                /* Assumed ok, if error flagged above. */
}

#endif
