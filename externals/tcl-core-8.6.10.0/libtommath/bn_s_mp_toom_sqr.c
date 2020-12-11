#include "tommath_private.h"
#ifdef BN_S_MP_TOOM_SQR_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* squaring using Toom-Cook 3-way algorithm */

/*
   This file contains code from J. Arndt's book  "Matters Computational"
   and the accompanying FXT-library with permission of the author.
*/

/* squaring using Toom-Cook 3-way algorithm */
/*
   Setup and interpolation from algorithm SQR_3 in

     Chung, Jaewook, and M. Anwar Hasan. "Asymmetric squaring formulae."
     18th IEEE Symposium on Computer Arithmetic (ARITH'07). IEEE, 2007.

*/
mp_err s_mp_toom_sqr(const mp_int *a, mp_int *b)
{
   mp_int S0, a0, a1, a2;
   mp_digit *tmpa, *tmpc;
   int B, count;
   mp_err err;


   /* init temps */
   if ((err = mp_init(&S0)) != MP_OKAY) {
      return err;
   }

   /* B */
   B = a->used / 3;

   /** a = a2 * x^2 + a1 * x + a0; */
   if ((err = mp_init_size(&a0, B)) != MP_OKAY)                   goto LBL_ERRa0;

   a0.used = B;
   if ((err = mp_init_size(&a1, B)) != MP_OKAY)                   goto LBL_ERRa1;
   a1.used = B;
   if ((err = mp_init_size(&a2, B + (a->used - (3 * B)))) != MP_OKAY) goto LBL_ERRa2;

   tmpa = a->dp;
   tmpc = a0.dp;
   for (count = 0; count < B; count++) {
      *tmpc++ = *tmpa++;
   }
   tmpc = a1.dp;
   for (; count < (2 * B); count++) {
      *tmpc++ = *tmpa++;
   }
   tmpc = a2.dp;
   for (; count < a->used; count++) {
      *tmpc++ = *tmpa++;
      a2.used++;
   }
   mp_clamp(&a0);
   mp_clamp(&a1);
   mp_clamp(&a2);

   /** S0 = a0^2;  */
   if ((err = mp_sqr(&a0, &S0)) != MP_OKAY)                       goto LBL_ERR;

   /** \\S1 = (a2 + a1 + a0)^2 */
   /** \\S2 = (a2 - a1 + a0)^2  */
   /** \\S1 = a0 + a2; */
   /** a0 = a0 + a2; */
   if ((err = mp_add(&a0, &a2, &a0)) != MP_OKAY)                  goto LBL_ERR;
   /** \\S2 = S1 - a1; */
   /** b = a0 - a1; */
   if ((err = mp_sub(&a0, &a1, b)) != MP_OKAY)                    goto LBL_ERR;
   /** \\S1 = S1 + a1; */
   /** a0 = a0 + a1; */
   if ((err = mp_add(&a0, &a1, &a0)) != MP_OKAY)                  goto LBL_ERR;
   /** \\S1 = S1^2;  */
   /** a0 = a0^2; */
   if ((err = mp_sqr(&a0, &a0)) != MP_OKAY)                       goto LBL_ERR;
   /** \\S2 = S2^2;  */
   /** b = b^2; */
   if ((err = mp_sqr(b, b)) != MP_OKAY)                           goto LBL_ERR;

   /** \\ S3 = 2 * a1 * a2  */
   /** \\S3 = a1 * a2;  */
   /** a1 = a1 * a2; */
   if ((err = mp_mul(&a1, &a2, &a1)) != MP_OKAY)                  goto LBL_ERR;
   /** \\S3 = S3 << 1;  */
   /** a1 = a1 << 1; */
   if ((err = mp_mul_2(&a1, &a1)) != MP_OKAY)                     goto LBL_ERR;

   /** \\S4 = a2^2;  */
   /** a2 = a2^2; */
   if ((err = mp_sqr(&a2, &a2)) != MP_OKAY)                       goto LBL_ERR;

   /** \\ tmp = (S1 + S2)/2  */
   /** \\tmp = S1 + S2; */
   /** b = a0 + b; */
   if ((err = mp_add(&a0, b, b)) != MP_OKAY)                      goto LBL_ERR;
   /** \\tmp = tmp >> 1; */
   /** b = b >> 1; */
   if ((err = mp_div_2(b, b)) != MP_OKAY)                         goto LBL_ERR;

   /** \\ S1 = S1 - tmp - S3  */
   /** \\S1 = S1 - tmp; */
   /** a0 = a0 - b; */
   if ((err = mp_sub(&a0, b, &a0)) != MP_OKAY)                    goto LBL_ERR;
   /** \\S1 = S1 - S3;  */
   /** a0 = a0 - a1; */
   if ((err = mp_sub(&a0, &a1, &a0)) != MP_OKAY)                  goto LBL_ERR;

   /** \\S2 = tmp - S4 -S0  */
   /** \\S2 = tmp - S4;  */
   /** b = b - a2; */
   if ((err = mp_sub(b, &a2, b)) != MP_OKAY)                      goto LBL_ERR;
   /** \\S2 = S2 - S0;  */
   /** b = b - S0; */
   if ((err = mp_sub(b, &S0, b)) != MP_OKAY)                      goto LBL_ERR;


   /** \\P = S4*x^4 + S3*x^3 + S2*x^2 + S1*x + S0; */
   /** P = a2*x^4 + a1*x^3 + b*x^2 + a0*x + S0; */

   if ((err = mp_lshd(&a2, 4 * B)) != MP_OKAY)                    goto LBL_ERR;
   if ((err = mp_lshd(&a1, 3 * B)) != MP_OKAY)                    goto LBL_ERR;
   if ((err = mp_lshd(b, 2 * B)) != MP_OKAY)                      goto LBL_ERR;
   if ((err = mp_lshd(&a0, 1 * B)) != MP_OKAY)                    goto LBL_ERR;
   if ((err = mp_add(&a2, &a1, &a2)) != MP_OKAY)                  goto LBL_ERR;
   if ((err = mp_add(&a2, b, b)) != MP_OKAY)                      goto LBL_ERR;
   if ((err = mp_add(b, &a0, b)) != MP_OKAY)                      goto LBL_ERR;
   if ((err = mp_add(b, &S0, b)) != MP_OKAY)                      goto LBL_ERR;
   /** a^2 - P  */


LBL_ERR:
   mp_clear(&a2);
LBL_ERRa2:
   mp_clear(&a1);
LBL_ERRa1:
   mp_clear(&a0);
LBL_ERRa0:
   mp_clear(&S0);

   return err;
}

#endif
