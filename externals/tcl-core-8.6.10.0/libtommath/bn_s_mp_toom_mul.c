#include "tommath_private.h"
#ifdef BN_S_MP_TOOM_MUL_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* multiplication using the Toom-Cook 3-way algorithm
 *
 * Much more complicated than Karatsuba but has a lower
 * asymptotic running time of O(N**1.464).  This algorithm is
 * only particularly useful on VERY large inputs
 * (we're talking 1000s of digits here...).
*/

/*
   This file contains code from J. Arndt's book  "Matters Computational"
   and the accompanying FXT-library with permission of the author.
*/

/*
   Setup from

     Chung, Jaewook, and M. Anwar Hasan. "Asymmetric squaring formulae."
     18th IEEE Symposium on Computer Arithmetic (ARITH'07). IEEE, 2007.

   The interpolation from above needed one temporary variable more
   than the interpolation here:

     Bodrato, Marco, and Alberto Zanoni. "What about Toom-Cook matrices optimality."
     Centro Vito Volterra Universita di Roma Tor Vergata (2006)
*/

mp_err s_mp_toom_mul(const mp_int *a, const mp_int *b, mp_int *c)
{
   mp_int S1, S2, T1, a0, a1, a2, b0, b1, b2;
   int B, count;
   mp_err err;

   /* init temps */
   if ((err = mp_init_multi(&S1, &S2, &T1, NULL)) != MP_OKAY) {
      return err;
   }

   /* B */
   B = MP_MIN(a->used, b->used) / 3;

   /** a = a2 * x^2 + a1 * x + a0; */
   if ((err = mp_init_size(&a0, B)) != MP_OKAY)                   goto LBL_ERRa0;

   for (count = 0; count < B; count++) {
      a0.dp[count] = a->dp[count];
      a0.used++;
   }
   mp_clamp(&a0);
   if ((err = mp_init_size(&a1, B)) != MP_OKAY)                   goto LBL_ERRa1;
   for (; count < (2 * B); count++) {
      a1.dp[count - B] = a->dp[count];
      a1.used++;
   }
   mp_clamp(&a1);
   if ((err = mp_init_size(&a2, B + (a->used - (3 * B)))) != MP_OKAY) goto LBL_ERRa2;
   for (; count < a->used; count++) {
      a2.dp[count - (2 * B)] = a->dp[count];
      a2.used++;
   }
   mp_clamp(&a2);

   /** b = b2 * x^2 + b1 * x + b0; */
   if ((err = mp_init_size(&b0, B)) != MP_OKAY)                   goto LBL_ERRb0;
   for (count = 0; count < B; count++) {
      b0.dp[count] = b->dp[count];
      b0.used++;
   }
   mp_clamp(&b0);
   if ((err = mp_init_size(&b1, B)) != MP_OKAY)                   goto LBL_ERRb1;
   for (; count < (2 * B); count++) {
      b1.dp[count - B] = b->dp[count];
      b1.used++;
   }
   mp_clamp(&b1);
   if ((err = mp_init_size(&b2, B + (b->used - (3 * B)))) != MP_OKAY) goto LBL_ERRb2;
   for (; count < b->used; count++) {
      b2.dp[count - (2 * B)] = b->dp[count];
      b2.used++;
   }
   mp_clamp(&b2);

   /** \\ S1 = (a2+a1+a0) * (b2+b1+b0); */
   /** T1 = a2 + a1; */
   if ((err = mp_add(&a2, &a1, &T1)) != MP_OKAY)                  goto LBL_ERR;

   /** S2 = T1 + a0; */
   if ((err = mp_add(&T1, &a0, &S2)) != MP_OKAY)                  goto LBL_ERR;

   /** c = b2 + b1; */
   if ((err = mp_add(&b2, &b1, c)) != MP_OKAY)                    goto LBL_ERR;

   /** S1 = c + b0; */
   if ((err = mp_add(c, &b0, &S1)) != MP_OKAY)                    goto LBL_ERR;

   /** S1 = S1 * S2; */
   if ((err = mp_mul(&S1, &S2, &S1)) != MP_OKAY)                  goto LBL_ERR;

   /** \\S2 = (4*a2+2*a1+a0) * (4*b2+2*b1+b0); */
   /** T1 = T1 + a2; */
   if ((err = mp_add(&T1, &a2, &T1)) != MP_OKAY)                  goto LBL_ERR;

   /** T1 = T1 << 1; */
   if ((err = mp_mul_2(&T1, &T1)) != MP_OKAY)                     goto LBL_ERR;

   /** T1 = T1 + a0; */
   if ((err = mp_add(&T1, &a0, &T1)) != MP_OKAY)                  goto LBL_ERR;

   /** c = c + b2; */
   if ((err = mp_add(c, &b2, c)) != MP_OKAY)                      goto LBL_ERR;

   /** c = c << 1; */
   if ((err = mp_mul_2(c, c)) != MP_OKAY)                         goto LBL_ERR;

   /** c = c + b0; */
   if ((err = mp_add(c, &b0, c)) != MP_OKAY)                      goto LBL_ERR;

   /** S2 = T1 * c; */
   if ((err = mp_mul(&T1, c, &S2)) != MP_OKAY)                    goto LBL_ERR;

   /** \\S3 = (a2-a1+a0) * (b2-b1+b0); */
   /** a1 = a2 - a1; */
   if ((err = mp_sub(&a2, &a1, &a1)) != MP_OKAY)                  goto LBL_ERR;

   /** a1 = a1 + a0; */
   if ((err = mp_add(&a1, &a0, &a1)) != MP_OKAY)                  goto LBL_ERR;

   /** b1 = b2 - b1; */
   if ((err = mp_sub(&b2, &b1, &b1)) != MP_OKAY)                  goto LBL_ERR;

   /** b1 = b1 + b0; */
   if ((err = mp_add(&b1, &b0, &b1)) != MP_OKAY)                  goto LBL_ERR;

   /** a1 = a1 * b1; */
   if ((err = mp_mul(&a1, &b1, &a1)) != MP_OKAY)                  goto LBL_ERR;

   /** b1 = a2 * b2; */
   if ((err = mp_mul(&a2, &b2, &b1)) != MP_OKAY)                  goto LBL_ERR;

   /** \\S2 = (S2 - S3)/3; */
   /** S2 = S2 - a1; */
   if ((err = mp_sub(&S2, &a1, &S2)) != MP_OKAY)                  goto LBL_ERR;

   /** S2 = S2 / 3; \\ this is an exact division  */
   if ((err = mp_div_3(&S2, &S2, NULL)) != MP_OKAY)               goto LBL_ERR;

   /** a1 = S1 - a1; */
   if ((err = mp_sub(&S1, &a1, &a1)) != MP_OKAY)                  goto LBL_ERR;

   /** a1 = a1 >> 1; */
   if ((err = mp_div_2(&a1, &a1)) != MP_OKAY)                     goto LBL_ERR;

   /** a0 = a0 * b0; */
   if ((err = mp_mul(&a0, &b0, &a0)) != MP_OKAY)                  goto LBL_ERR;

   /** S1 = S1 - a0; */
   if ((err = mp_sub(&S1, &a0, &S1)) != MP_OKAY)                  goto LBL_ERR;

   /** S2 = S2 - S1; */
   if ((err = mp_sub(&S2, &S1, &S2)) != MP_OKAY)                  goto LBL_ERR;

   /** S2 = S2 >> 1; */
   if ((err = mp_div_2(&S2, &S2)) != MP_OKAY)                     goto LBL_ERR;

   /** S1 = S1 - a1; */
   if ((err = mp_sub(&S1, &a1, &S1)) != MP_OKAY)                  goto LBL_ERR;

   /** S1 = S1 - b1; */
   if ((err = mp_sub(&S1, &b1, &S1)) != MP_OKAY)                  goto LBL_ERR;

   /** T1 = b1 << 1; */
   if ((err = mp_mul_2(&b1, &T1)) != MP_OKAY)                     goto LBL_ERR;

   /** S2 = S2 - T1; */
   if ((err = mp_sub(&S2, &T1, &S2)) != MP_OKAY)                  goto LBL_ERR;

   /** a1 = a1 - S2; */
   if ((err = mp_sub(&a1, &S2, &a1)) != MP_OKAY)                  goto LBL_ERR;


   /** P = b1*x^4+ S2*x^3+ S1*x^2+ a1*x + a0; */
   if ((err = mp_lshd(&b1, 4 * B)) != MP_OKAY)                    goto LBL_ERR;
   if ((err = mp_lshd(&S2, 3 * B)) != MP_OKAY)                    goto LBL_ERR;
   if ((err = mp_add(&b1, &S2, &b1)) != MP_OKAY)                  goto LBL_ERR;
   if ((err = mp_lshd(&S1, 2 * B)) != MP_OKAY)                    goto LBL_ERR;
   if ((err = mp_add(&b1, &S1, &b1)) != MP_OKAY)                  goto LBL_ERR;
   if ((err = mp_lshd(&a1, 1 * B)) != MP_OKAY)                    goto LBL_ERR;
   if ((err = mp_add(&b1, &a1, &b1)) != MP_OKAY)                  goto LBL_ERR;
   if ((err = mp_add(&b1, &a0, c)) != MP_OKAY)                    goto LBL_ERR;

   /** a * b - P */


LBL_ERR:
   mp_clear(&b2);
LBL_ERRb2:
   mp_clear(&b1);
LBL_ERRb1:
   mp_clear(&b0);
LBL_ERRb0:
   mp_clear(&a2);
LBL_ERRa2:
   mp_clear(&a1);
LBL_ERRa1:
   mp_clear(&a0);
LBL_ERRa0:
   mp_clear_multi(&S1, &S2, &T1, NULL);
   return err;
}

#endif
