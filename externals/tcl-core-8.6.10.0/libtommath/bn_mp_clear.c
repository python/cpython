#include "tommath_private.h"
#ifdef BN_MP_CLEAR_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* clear one (frees)  */
void mp_clear(mp_int *a)
{
   /* only do anything if a hasn't been freed previously */
   if (a->dp != NULL) {
      /* free ram */
      MP_FREE_DIGITS(a->dp, a->alloc);

      /* reset members to make debugging easier */
      a->dp    = NULL;
      a->alloc = a->used = 0;
      a->sign  = MP_ZPOS;
   }
}
#endif
