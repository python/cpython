#include "tommath_private.h"
#ifdef BN_MP_KRONECKER_C

/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/*
   Kronecker symbol (a|p)
   Straightforward implementation of algorithm 1.4.10 in
   Henri Cohen: "A Course in Computational Algebraic Number Theory"

   @book{cohen2013course,
     title={A course in computational algebraic number theory},
     author={Cohen, Henri},
     volume={138},
     year={2013},
     publisher={Springer Science \& Business Media}
    }
 */
mp_err mp_kronecker(const mp_int *a, const mp_int *p, int *c)
{
   mp_int a1, p1, r;
   mp_err err;
   int v, k;

   static const int table[8] = {0, 1, 0, -1, 0, -1, 0, 1};

   if (MP_IS_ZERO(p)) {
      if ((a->used == 1) && (a->dp[0] == 1u)) {
         *c = 1;
      } else {
         *c = 0;
      }
      return MP_OKAY;
   }

   if (MP_IS_EVEN(a) && MP_IS_EVEN(p)) {
      *c = 0;
      return MP_OKAY;
   }

   if ((err = mp_init_copy(&a1, a)) != MP_OKAY) {
      return err;
   }
   if ((err = mp_init_copy(&p1, p)) != MP_OKAY) {
      goto LBL_KRON_0;
   }

   v = mp_cnt_lsb(&p1);
   if ((err = mp_div_2d(&p1, v, &p1, NULL)) != MP_OKAY) {
      goto LBL_KRON_1;
   }

   if ((v & 1) == 0) {
      k = 1;
   } else {
      k = table[a->dp[0] & 7u];
   }

   if (p1.sign == MP_NEG) {
      p1.sign = MP_ZPOS;
      if (a1.sign == MP_NEG) {
         k = -k;
      }
   }

   if ((err = mp_init(&r)) != MP_OKAY) {
      goto LBL_KRON_1;
   }

   for (;;) {
      if (MP_IS_ZERO(&a1)) {
         if (mp_cmp_d(&p1, 1uL) == MP_EQ) {
            *c = k;
            goto LBL_KRON;
         } else {
            *c = 0;
            goto LBL_KRON;
         }
      }

      v = mp_cnt_lsb(&a1);
      if ((err = mp_div_2d(&a1, v, &a1, NULL)) != MP_OKAY) {
         goto LBL_KRON;
      }

      if ((v & 1) == 1) {
         k = k * table[p1.dp[0] & 7u];
      }

      if (a1.sign == MP_NEG) {
         /*
          * Compute k = (-1)^((a1)*(p1-1)/4) * k
          * a1.dp[0] + 1 cannot overflow because the MSB
          * of the type mp_digit is not set by definition
          */
         if (((a1.dp[0] + 1u) & p1.dp[0] & 2u) != 0u) {
            k = -k;
         }
      } else {
         /* compute k = (-1)^((a1-1)*(p1-1)/4) * k */
         if ((a1.dp[0] & p1.dp[0] & 2u) != 0u) {
            k = -k;
         }
      }

      if ((err = mp_copy(&a1, &r)) != MP_OKAY) {
         goto LBL_KRON;
      }
      r.sign = MP_ZPOS;
      if ((err = mp_mod(&p1, &r, &a1)) != MP_OKAY) {
         goto LBL_KRON;
      }
      if ((err = mp_copy(&r, &p1)) != MP_OKAY) {
         goto LBL_KRON;
      }
   }

LBL_KRON:
   mp_clear(&r);
LBL_KRON_1:
   mp_clear(&p1);
LBL_KRON_0:
   mp_clear(&a1);

   return err;
}

#endif
