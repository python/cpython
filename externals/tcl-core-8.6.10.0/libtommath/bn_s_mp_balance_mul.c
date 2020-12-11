#include "tommath_private.h"
#ifdef BN_S_MP_BALANCE_MUL_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* single-digit multiplication with the smaller number as the single-digit */
mp_err s_mp_balance_mul(const mp_int *a, const mp_int *b, mp_int *c)
{
   int count, len_a, len_b, nblocks, i, j, bsize;
   mp_int a0, tmp, A, B, r;
   mp_err err;

   len_a = a->used;
   len_b = b->used;

   nblocks = MP_MAX(a->used, b->used) / MP_MIN(a->used, b->used);
   bsize = MP_MIN(a->used, b->used) ;

   if ((err = mp_init_size(&a0, bsize + 2)) != MP_OKAY) {
      return err;
   }
   if ((err = mp_init_multi(&tmp, &r, NULL)) != MP_OKAY) {
      mp_clear(&a0);
      return err;
   }

   /* Make sure that A is the larger one*/
   if (len_a < len_b) {
      B = *a;
      A = *b;
   } else {
      A = *a;
      B = *b;
   }

   for (i = 0, j=0; i < nblocks; i++) {
      /* Cut a slice off of a */
      a0.used = 0;
      for (count = 0; count < bsize; count++) {
         a0.dp[count] = A.dp[ j++ ];
         a0.used++;
      }
      mp_clamp(&a0);
      /* Multiply with b */
      if ((err = mp_mul(&a0, &B, &tmp)) != MP_OKAY) {
         goto LBL_ERR;
      }
      /* Shift tmp to the correct position */
      if ((err = mp_lshd(&tmp, bsize * i)) != MP_OKAY) {
         goto LBL_ERR;
      }
      /* Add to output. No carry needed */
      if ((err = mp_add(&r, &tmp, &r)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }
   /* The left-overs; there are always left-overs */
   if (j < A.used) {
      a0.used = 0;
      for (count = 0; j < A.used; count++) {
         a0.dp[count] = A.dp[ j++ ];
         a0.used++;
      }
      mp_clamp(&a0);
      if ((err = mp_mul(&a0, &B, &tmp)) != MP_OKAY) {
         goto LBL_ERR;
      }
      if ((err = mp_lshd(&tmp, bsize * i)) != MP_OKAY) {
         goto LBL_ERR;
      }
      if ((err = mp_add(&r, &tmp, &r)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   mp_exch(&r,c);
LBL_ERR:
   mp_clear_multi(&a0, &tmp, &r,NULL);
   return err;
}
#endif
