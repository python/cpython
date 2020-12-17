#include "tommath_private.h"
#ifdef BN_MP_UBIN_SIZE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* get the size for an unsigned equivalent */
size_t mp_ubin_size(const mp_int *a)
{
   size_t size = (size_t)mp_count_bits(a);
   return (size / 8u) + (((size & 7u) != 0u) ? 1u : 0u);
}
#endif
