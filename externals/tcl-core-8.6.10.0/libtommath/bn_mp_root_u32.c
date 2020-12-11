#include "tommath_private.h"
#ifdef BN_MP_ROOT_U32_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* find the n'th root of an integer
 *
 * Result found such that (c)**b <= a and (c+1)**b > a
 *
 * This algorithm uses Newton's approximation
 * x[i+1] = x[i] - f(x[i])/f'(x[i])
 * which will find the root in log(N) time where
 * each step involves a fair bit.
 */
mp_err mp_root_u32(const mp_int *a, unsigned int b, mp_int *c)
{
   mp_int t1, t2, t3, a_;
   mp_ord cmp;
   int    ilog2;
   mp_err err;

   /* input must be positive if b is even */
   if (((b & 1u) == 0u) && (a->sign == MP_NEG)) {
      return MP_VAL;
   }

   if ((err = mp_init_multi(&t1, &t2, &t3, NULL)) != MP_OKAY) {
      return err;
   }

   /* if a is negative fudge the sign but keep track */
   a_ = *a;
   a_.sign = MP_ZPOS;

   /* Compute seed: 2^(log_2(n)/b + 2)*/
   ilog2 = mp_count_bits(a);

   /*
     If "b" is larger than INT_MAX it is also larger than
     log_2(n) because the bit-length of the "n" is measured
     with an int and hence the root is always < 2 (two).
   */
   if (b > (unsigned int)(INT_MAX/2)) {
      mp_set(c, 1uL);
      c->sign = a->sign;
      err = MP_OKAY;
      goto LBL_ERR;
   }

   /* "b" is smaller than INT_MAX, we can cast safely */
   if (ilog2 < (int)b) {
      mp_set(c, 1uL);
      c->sign = a->sign;
      err = MP_OKAY;
      goto LBL_ERR;
   }
   ilog2 =  ilog2 / ((int)b);
   if (ilog2 == 0) {
      mp_set(c, 1uL);
      c->sign = a->sign;
      err = MP_OKAY;
      goto LBL_ERR;
   }
   /* Start value must be larger than root */
   ilog2 += 2;
   if ((err = mp_2expt(&t2,ilog2)) != MP_OKAY)                    goto LBL_ERR;
   do {
      /* t1 = t2 */
      if ((err = mp_copy(&t2, &t1)) != MP_OKAY)                   goto LBL_ERR;

      /* t2 = t1 - ((t1**b - a) / (b * t1**(b-1))) */

      /* t3 = t1**(b-1) */
      if ((err = mp_expt_u32(&t1, b - 1u, &t3)) != MP_OKAY)       goto LBL_ERR;

      /* numerator */
      /* t2 = t1**b */
      if ((err = mp_mul(&t3, &t1, &t2)) != MP_OKAY)               goto LBL_ERR;

      /* t2 = t1**b - a */
      if ((err = mp_sub(&t2, &a_, &t2)) != MP_OKAY)               goto LBL_ERR;

      /* denominator */
      /* t3 = t1**(b-1) * b  */
      if ((err = mp_mul_d(&t3, b, &t3)) != MP_OKAY)               goto LBL_ERR;

      /* t3 = (t1**b - a)/(b * t1**(b-1)) */
      if ((err = mp_div(&t2, &t3, &t3, NULL)) != MP_OKAY)         goto LBL_ERR;

      if ((err = mp_sub(&t1, &t3, &t2)) != MP_OKAY)               goto LBL_ERR;

      /*
          Number of rounds is at most log_2(root). If it is more it
          got stuck, so break out of the loop and do the rest manually.
       */
      if (ilog2-- == 0) {
         break;
      }
   }  while (mp_cmp(&t1, &t2) != MP_EQ);

   /* result can be off by a few so check */
   /* Loop beneath can overshoot by one if found root is smaller than actual root */
   for (;;) {
      if ((err = mp_expt_u32(&t1, b, &t2)) != MP_OKAY)            goto LBL_ERR;
      cmp = mp_cmp(&t2, &a_);
      if (cmp == MP_EQ) {
         err = MP_OKAY;
         goto LBL_ERR;
      }
      if (cmp == MP_LT) {
         if ((err = mp_add_d(&t1, 1uL, &t1)) != MP_OKAY)          goto LBL_ERR;
      } else {
         break;
      }
   }
   /* correct overshoot from above or from recurrence */
   for (;;) {
      if ((err = mp_expt_u32(&t1, b, &t2)) != MP_OKAY)            goto LBL_ERR;
      if (mp_cmp(&t2, &a_) == MP_GT) {
         if ((err = mp_sub_d(&t1, 1uL, &t1)) != MP_OKAY)          goto LBL_ERR;
      } else {
         break;
      }
   }

   /* set the result */
   mp_exch(&t1, c);

   /* set the sign of the result */
   c->sign = a->sign;

   err = MP_OKAY;

LBL_ERR:
   mp_clear_multi(&t1, &t2, &t3, NULL);
   return err;
}

#endif
