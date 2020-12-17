#include "tommath_private.h"
#ifdef BN_S_MP_INVMOD_SLOW_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* hac 14.61, pp608 */
mp_err s_mp_invmod_slow(const mp_int *a, const mp_int *b, mp_int *c)
{
   mp_int  x, y, u, v, A, B, C, D;
   mp_err  err;

   /* b cannot be negative */
   if ((b->sign == MP_NEG) || MP_IS_ZERO(b)) {
      return MP_VAL;
   }

   /* init temps */
   if ((err = mp_init_multi(&x, &y, &u, &v,
                            &A, &B, &C, &D, NULL)) != MP_OKAY) {
      return err;
   }

   /* x = a, y = b */
   if ((err = mp_mod(a, b, &x)) != MP_OKAY)                       goto LBL_ERR;
   if ((err = mp_copy(b, &y)) != MP_OKAY)                         goto LBL_ERR;

   /* 2. [modified] if x,y are both even then return an error! */
   if (MP_IS_EVEN(&x) && MP_IS_EVEN(&y)) {
      err = MP_VAL;
      goto LBL_ERR;
   }

   /* 3. u=x, v=y, A=1, B=0, C=0,D=1 */
   if ((err = mp_copy(&x, &u)) != MP_OKAY)                        goto LBL_ERR;
   if ((err = mp_copy(&y, &v)) != MP_OKAY)                        goto LBL_ERR;
   mp_set(&A, 1uL);
   mp_set(&D, 1uL);

top:
   /* 4.  while u is even do */
   while (MP_IS_EVEN(&u)) {
      /* 4.1 u = u/2 */
      if ((err = mp_div_2(&u, &u)) != MP_OKAY)                    goto LBL_ERR;

      /* 4.2 if A or B is odd then */
      if (MP_IS_ODD(&A) || MP_IS_ODD(&B)) {
         /* A = (A+y)/2, B = (B-x)/2 */
         if ((err = mp_add(&A, &y, &A)) != MP_OKAY)               goto LBL_ERR;
         if ((err = mp_sub(&B, &x, &B)) != MP_OKAY)               goto LBL_ERR;
      }
      /* A = A/2, B = B/2 */
      if ((err = mp_div_2(&A, &A)) != MP_OKAY)                    goto LBL_ERR;
      if ((err = mp_div_2(&B, &B)) != MP_OKAY)                    goto LBL_ERR;
   }

   /* 5.  while v is even do */
   while (MP_IS_EVEN(&v)) {
      /* 5.1 v = v/2 */
      if ((err = mp_div_2(&v, &v)) != MP_OKAY)                    goto LBL_ERR;

      /* 5.2 if C or D is odd then */
      if (MP_IS_ODD(&C) || MP_IS_ODD(&D)) {
         /* C = (C+y)/2, D = (D-x)/2 */
         if ((err = mp_add(&C, &y, &C)) != MP_OKAY)               goto LBL_ERR;
         if ((err = mp_sub(&D, &x, &D)) != MP_OKAY)               goto LBL_ERR;
      }
      /* C = C/2, D = D/2 */
      if ((err = mp_div_2(&C, &C)) != MP_OKAY)                    goto LBL_ERR;
      if ((err = mp_div_2(&D, &D)) != MP_OKAY)                    goto LBL_ERR;
   }

   /* 6.  if u >= v then */
   if (mp_cmp(&u, &v) != MP_LT) {
      /* u = u - v, A = A - C, B = B - D */
      if ((err = mp_sub(&u, &v, &u)) != MP_OKAY)                  goto LBL_ERR;

      if ((err = mp_sub(&A, &C, &A)) != MP_OKAY)                  goto LBL_ERR;

      if ((err = mp_sub(&B, &D, &B)) != MP_OKAY)                  goto LBL_ERR;
   } else {
      /* v - v - u, C = C - A, D = D - B */
      if ((err = mp_sub(&v, &u, &v)) != MP_OKAY)                  goto LBL_ERR;

      if ((err = mp_sub(&C, &A, &C)) != MP_OKAY)                  goto LBL_ERR;

      if ((err = mp_sub(&D, &B, &D)) != MP_OKAY)                  goto LBL_ERR;
   }

   /* if not zero goto step 4 */
   if (!MP_IS_ZERO(&u)) {
      goto top;
   }

   /* now a = C, b = D, gcd == g*v */

   /* if v != 1 then there is no inverse */
   if (mp_cmp_d(&v, 1uL) != MP_EQ) {
      err = MP_VAL;
      goto LBL_ERR;
   }

   /* if its too low */
   while (mp_cmp_d(&C, 0uL) == MP_LT) {
      if ((err = mp_add(&C, b, &C)) != MP_OKAY)                   goto LBL_ERR;
   }

   /* too big */
   while (mp_cmp_mag(&C, b) != MP_LT) {
      if ((err = mp_sub(&C, b, &C)) != MP_OKAY)                   goto LBL_ERR;
   }

   /* C is now the inverse */
   mp_exch(&C, c);
   err = MP_OKAY;
LBL_ERR:
   mp_clear_multi(&x, &y, &u, &v, &A, &B, &C, &D, NULL);
   return err;
}
#endif
