#include "tommath_private.h"
#ifdef BN_MP_SBIN_SIZE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* get the size for an signed equivalent */
size_t mp_sbin_size(const mp_int *a)
{
   return 1u + mp_ubin_size(a);
}
#endif
