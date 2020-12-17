#include "tommath_private.h"
#ifdef BN_MP_PACK_COUNT_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

size_t mp_pack_count(const mp_int *a, size_t nails, size_t size)
{
   size_t bits = (size_t)mp_count_bits(a);
   return ((bits / ((size * 8u) - nails)) + (((bits % ((size * 8u) - nails)) != 0u) ? 1u : 0u));
}

#endif
