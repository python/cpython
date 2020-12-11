#include "tommath_private.h"
#ifdef BN_MP_GROW_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* grow as required */
mp_err mp_grow(mp_int *a, int size)
{
   int     i;
   mp_digit *tmp;

   /* if the alloc size is smaller alloc more ram */
   if (a->alloc < size) {
      /* reallocate the array a->dp
       *
       * We store the return in a temporary variable
       * in case the operation failed we don't want
       * to overwrite the dp member of a.
       */
      tmp = (mp_digit *) MP_REALLOC(a->dp,
                                    (size_t)a->alloc * sizeof(mp_digit),
                                    (size_t)size * sizeof(mp_digit));
      if (tmp == NULL) {
         /* reallocation failed but "a" is still valid [can be freed] */
         return MP_MEM;
      }

      /* reallocation succeeded so set a->dp */
      a->dp = tmp;

      /* zero excess digits */
      i        = a->alloc;
      a->alloc = size;
      MP_ZERO_DIGITS(a->dp + i, a->alloc - i);
   }
   return MP_OKAY;
}
#endif
