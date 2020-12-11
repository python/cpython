#include "tommath_private.h"
#ifdef BN_S_MP_KARATSUBA_SQR_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Karatsuba squaring, computes b = a*a using three
 * half size squarings
 *
 * See comments of karatsuba_mul for details.  It
 * is essentially the same algorithm but merely
 * tuned to perform recursive squarings.
 */
mp_err s_mp_karatsuba_sqr(const mp_int *a, mp_int *b)
{
   mp_int  x0, x1, t1, t2, x0x0, x1x1;
   int     B;
   mp_err  err = MP_MEM;

   /* min # of digits */
   B = a->used;

   /* now divide in two */
   B = B >> 1;

   /* init copy all the temps */
   if (mp_init_size(&x0, B) != MP_OKAY)
      goto LBL_ERR;
   if (mp_init_size(&x1, a->used - B) != MP_OKAY)
      goto X0;

   /* init temps */
   if (mp_init_size(&t1, a->used * 2) != MP_OKAY)
      goto X1;
   if (mp_init_size(&t2, a->used * 2) != MP_OKAY)
      goto T1;
   if (mp_init_size(&x0x0, B * 2) != MP_OKAY)
      goto T2;
   if (mp_init_size(&x1x1, (a->used - B) * 2) != MP_OKAY)
      goto X0X0;

   {
      int x;
      mp_digit *dst, *src;

      src = a->dp;

      /* now shift the digits */
      dst = x0.dp;
      for (x = 0; x < B; x++) {
         *dst++ = *src++;
      }

      dst = x1.dp;
      for (x = B; x < a->used; x++) {
         *dst++ = *src++;
      }
   }

   x0.used = B;
   x1.used = a->used - B;

   mp_clamp(&x0);

   /* now calc the products x0*x0 and x1*x1 */
   if (mp_sqr(&x0, &x0x0) != MP_OKAY)
      goto X1X1;           /* x0x0 = x0*x0 */
   if (mp_sqr(&x1, &x1x1) != MP_OKAY)
      goto X1X1;           /* x1x1 = x1*x1 */

   /* now calc (x1+x0)**2 */
   if (s_mp_add(&x1, &x0, &t1) != MP_OKAY)
      goto X1X1;           /* t1 = x1 - x0 */
   if (mp_sqr(&t1, &t1) != MP_OKAY)
      goto X1X1;           /* t1 = (x1 - x0) * (x1 - x0) */

   /* add x0y0 */
   if (s_mp_add(&x0x0, &x1x1, &t2) != MP_OKAY)
      goto X1X1;           /* t2 = x0x0 + x1x1 */
   if (s_mp_sub(&t1, &t2, &t1) != MP_OKAY)
      goto X1X1;           /* t1 = (x1+x0)**2 - (x0x0 + x1x1) */

   /* shift by B */
   if (mp_lshd(&t1, B) != MP_OKAY)
      goto X1X1;           /* t1 = (x0x0 + x1x1 - (x1-x0)*(x1-x0))<<B */
   if (mp_lshd(&x1x1, B * 2) != MP_OKAY)
      goto X1X1;           /* x1x1 = x1x1 << 2*B */

   if (mp_add(&x0x0, &t1, &t1) != MP_OKAY)
      goto X1X1;           /* t1 = x0x0 + t1 */
   if (mp_add(&t1, &x1x1, b) != MP_OKAY)
      goto X1X1;           /* t1 = x0x0 + t1 + x1x1 */

   err = MP_OKAY;

X1X1:
   mp_clear(&x1x1);
X0X0:
   mp_clear(&x0x0);
T2:
   mp_clear(&t2);
T1:
   mp_clear(&t1);
X1:
   mp_clear(&x1);
X0:
   mp_clear(&x0);
LBL_ERR:
   return err;
}
#endif
