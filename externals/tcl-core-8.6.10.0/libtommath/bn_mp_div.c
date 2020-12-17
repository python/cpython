#include "tommath_private.h"
#ifdef BN_MP_DIV_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#ifdef BN_MP_DIV_SMALL

/* slower bit-bang division... also smaller */
mp_err mp_div(const mp_int *a, const mp_int *b, mp_int *c, mp_int *d)
{
   mp_int ta, tb, tq, q;
   int     n, n2;
   mp_err err;

   /* is divisor zero ? */
   if (MP_IS_ZERO(b)) {
      return MP_VAL;
   }

   /* if a < b then q=0, r = a */
   if (mp_cmp_mag(a, b) == MP_LT) {
      if (d != NULL) {
         err = mp_copy(a, d);
      } else {
         err = MP_OKAY;
      }
      if (c != NULL) {
         mp_zero(c);
      }
      return err;
   }

   /* init our temps */
   if ((err = mp_init_multi(&ta, &tb, &tq, &q, NULL)) != MP_OKAY) {
      return err;
   }


   mp_set(&tq, 1uL);
   n = mp_count_bits(a) - mp_count_bits(b);
   if ((err = mp_abs(a, &ta)) != MP_OKAY)                         goto LBL_ERR;
   if ((err = mp_abs(b, &tb)) != MP_OKAY)                         goto LBL_ERR;
   if ((err = mp_mul_2d(&tb, n, &tb)) != MP_OKAY)                 goto LBL_ERR;
   if ((err = mp_mul_2d(&tq, n, &tq)) != MP_OKAY)                 goto LBL_ERR;

   while (n-- >= 0) {
      if (mp_cmp(&tb, &ta) != MP_GT) {
         if ((err = mp_sub(&ta, &tb, &ta)) != MP_OKAY)            goto LBL_ERR;
         if ((err = mp_add(&q, &tq, &q)) != MP_OKAY)              goto LBL_ERR;
      }
      if ((err = mp_div_2d(&tb, 1, &tb, NULL)) != MP_OKAY)        goto LBL_ERR;
      if ((err = mp_div_2d(&tq, 1, &tq, NULL)) != MP_OKAY)        goto LBL_ERR;
   }

   /* now q == quotient and ta == remainder */
   n  = a->sign;
   n2 = (a->sign == b->sign) ? MP_ZPOS : MP_NEG;
   if (c != NULL) {
      mp_exch(c, &q);
      c->sign  = MP_IS_ZERO(c) ? MP_ZPOS : n2;
   }
   if (d != NULL) {
      mp_exch(d, &ta);
      d->sign = MP_IS_ZERO(d) ? MP_ZPOS : n;
   }
LBL_ERR:
   mp_clear_multi(&ta, &tb, &tq, &q, NULL);
   return err;
}

#else

/* integer signed division.
 * c*b + d == a [e.g. a/b, c=quotient, d=remainder]
 * HAC pp.598 Algorithm 14.20
 *
 * Note that the description in HAC is horribly
 * incomplete.  For example, it doesn't consider
 * the case where digits are removed from 'x' in
 * the inner loop.  It also doesn't consider the
 * case that y has fewer than three digits, etc..
 *
 * The overall algorithm is as described as
 * 14.20 from HAC but fixed to treat these cases.
*/
mp_err mp_div(const mp_int *a, const mp_int *b, mp_int *c, mp_int *d)
{
   mp_int  q, x, y, t1, t2;
   int     n, t, i, norm;
   mp_sign neg;
   mp_err  err;

   /* is divisor zero ? */
   if (MP_IS_ZERO(b)) {
      return MP_VAL;
   }

   /* if a < b then q=0, r = a */
   if (mp_cmp_mag(a, b) == MP_LT) {
      if (d != NULL) {
         err = mp_copy(a, d);
      } else {
         err = MP_OKAY;
      }
      if (c != NULL) {
         mp_zero(c);
      }
      return err;
   }

   if ((err = mp_init_size(&q, a->used + 2)) != MP_OKAY) {
      return err;
   }
   q.used = a->used + 2;

   if ((err = mp_init(&t1)) != MP_OKAY)                           goto LBL_Q;

   if ((err = mp_init(&t2)) != MP_OKAY)                           goto LBL_T1;

   if ((err = mp_init_copy(&x, a)) != MP_OKAY)                    goto LBL_T2;

   if ((err = mp_init_copy(&y, b)) != MP_OKAY)                    goto LBL_X;

   /* fix the sign */
   neg = (a->sign == b->sign) ? MP_ZPOS : MP_NEG;
   x.sign = y.sign = MP_ZPOS;

   /* normalize both x and y, ensure that y >= b/2, [b == 2**MP_DIGIT_BIT] */
   norm = mp_count_bits(&y) % MP_DIGIT_BIT;
   if (norm < (MP_DIGIT_BIT - 1)) {
      norm = (MP_DIGIT_BIT - 1) - norm;
      if ((err = mp_mul_2d(&x, norm, &x)) != MP_OKAY)             goto LBL_Y;
      if ((err = mp_mul_2d(&y, norm, &y)) != MP_OKAY)             goto LBL_Y;
   } else {
      norm = 0;
   }

   /* note hac does 0 based, so if used==5 then its 0,1,2,3,4, e.g. use 4 */
   n = x.used - 1;
   t = y.used - 1;

   /* while (x >= y*b**n-t) do { q[n-t] += 1; x -= y*b**{n-t} } */
   /* y = y*b**{n-t} */
   if ((err = mp_lshd(&y, n - t)) != MP_OKAY)                     goto LBL_Y;

   while (mp_cmp(&x, &y) != MP_LT) {
      ++(q.dp[n - t]);
      if ((err = mp_sub(&x, &y, &x)) != MP_OKAY)                  goto LBL_Y;
   }

   /* reset y by shifting it back down */
   mp_rshd(&y, n - t);

   /* step 3. for i from n down to (t + 1) */
   for (i = n; i >= (t + 1); i--) {
      if (i > x.used) {
         continue;
      }

      /* step 3.1 if xi == yt then set q{i-t-1} to b-1,
       * otherwise set q{i-t-1} to (xi*b + x{i-1})/yt */
      if (x.dp[i] == y.dp[t]) {
         q.dp[(i - t) - 1] = ((mp_digit)1 << (mp_digit)MP_DIGIT_BIT) - (mp_digit)1;
      } else {
         mp_word tmp;
         tmp = (mp_word)x.dp[i] << (mp_word)MP_DIGIT_BIT;
         tmp |= (mp_word)x.dp[i - 1];
         tmp /= (mp_word)y.dp[t];
         if (tmp > (mp_word)MP_MASK) {
            tmp = MP_MASK;
         }
         q.dp[(i - t) - 1] = (mp_digit)(tmp & (mp_word)MP_MASK);
      }

      /* while (q{i-t-1} * (yt * b + y{t-1})) >
               xi * b**2 + xi-1 * b + xi-2

         do q{i-t-1} -= 1;
      */
      q.dp[(i - t) - 1] = (q.dp[(i - t) - 1] + 1uL) & (mp_digit)MP_MASK;
      do {
         q.dp[(i - t) - 1] = (q.dp[(i - t) - 1] - 1uL) & (mp_digit)MP_MASK;

         /* find left hand */
         mp_zero(&t1);
         t1.dp[0] = ((t - 1) < 0) ? 0u : y.dp[t - 1];
         t1.dp[1] = y.dp[t];
         t1.used = 2;
         if ((err = mp_mul_d(&t1, q.dp[(i - t) - 1], &t1)) != MP_OKAY) goto LBL_Y;

         /* find right hand */
         t2.dp[0] = ((i - 2) < 0) ? 0u : x.dp[i - 2];
         t2.dp[1] = x.dp[i - 1]; /* i >= 1 always holds */
         t2.dp[2] = x.dp[i];
         t2.used = 3;
      } while (mp_cmp_mag(&t1, &t2) == MP_GT);

      /* step 3.3 x = x - q{i-t-1} * y * b**{i-t-1} */
      if ((err = mp_mul_d(&y, q.dp[(i - t) - 1], &t1)) != MP_OKAY) goto LBL_Y;

      if ((err = mp_lshd(&t1, (i - t) - 1)) != MP_OKAY)           goto LBL_Y;

      if ((err = mp_sub(&x, &t1, &x)) != MP_OKAY)                 goto LBL_Y;

      /* if x < 0 then { x = x + y*b**{i-t-1}; q{i-t-1} -= 1; } */
      if (x.sign == MP_NEG) {
         if ((err = mp_copy(&y, &t1)) != MP_OKAY)                 goto LBL_Y;
         if ((err = mp_lshd(&t1, (i - t) - 1)) != MP_OKAY)        goto LBL_Y;
         if ((err = mp_add(&x, &t1, &x)) != MP_OKAY)              goto LBL_Y;

         q.dp[(i - t) - 1] = (q.dp[(i - t) - 1] - 1uL) & MP_MASK;
      }
   }

   /* now q is the quotient and x is the remainder
    * [which we have to normalize]
    */

   /* get sign before writing to c */
   x.sign = (x.used == 0) ? MP_ZPOS : a->sign;

   if (c != NULL) {
      mp_clamp(&q);
      mp_exch(&q, c);
      c->sign = neg;
   }

   if (d != NULL) {
      if ((err = mp_div_2d(&x, norm, &x, NULL)) != MP_OKAY)       goto LBL_Y;
      mp_exch(&x, d);
   }

   err = MP_OKAY;

LBL_Y:
   mp_clear(&y);
LBL_X:
   mp_clear(&x);
LBL_T2:
   mp_clear(&t2);
LBL_T1:
   mp_clear(&t1);
LBL_Q:
   mp_clear(&q);
   return err;
}

#endif

#endif
