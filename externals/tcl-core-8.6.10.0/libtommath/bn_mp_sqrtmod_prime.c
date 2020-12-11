#include "tommath_private.h"
#ifdef BN_MP_SQRTMOD_PRIME_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Tonelli-Shanks algorithm
 * https://en.wikipedia.org/wiki/Tonelli%E2%80%93Shanks_algorithm
 * https://gmplib.org/list-archives/gmp-discuss/2013-April/005300.html
 *
 */

mp_err mp_sqrtmod_prime(const mp_int *n, const mp_int *prime, mp_int *ret)
{
   mp_err err;
   int legendre;
   mp_int t1, C, Q, S, Z, M, T, R, two;
   mp_digit i;

   /* first handle the simple cases */
   if (mp_cmp_d(n, 0uL) == MP_EQ) {
      mp_zero(ret);
      return MP_OKAY;
   }
   if (mp_cmp_d(prime, 2uL) == MP_EQ)                            return MP_VAL; /* prime must be odd */
   if ((err = mp_kronecker(n, prime, &legendre)) != MP_OKAY)        return err;
   if (legendre == -1)                                           return MP_VAL; /* quadratic non-residue mod prime */

   if ((err = mp_init_multi(&t1, &C, &Q, &S, &Z, &M, &T, &R, &two, NULL)) != MP_OKAY) {
      return err;
   }

   /* SPECIAL CASE: if prime mod 4 == 3
    * compute directly: err = n^(prime+1)/4 mod prime
    * Handbook of Applied Cryptography algorithm 3.36
    */
   if ((err = mp_mod_d(prime, 4uL, &i)) != MP_OKAY)               goto cleanup;
   if (i == 3u) {
      if ((err = mp_add_d(prime, 1uL, &t1)) != MP_OKAY)           goto cleanup;
      if ((err = mp_div_2(&t1, &t1)) != MP_OKAY)                  goto cleanup;
      if ((err = mp_div_2(&t1, &t1)) != MP_OKAY)                  goto cleanup;
      if ((err = mp_exptmod(n, &t1, prime, ret)) != MP_OKAY)      goto cleanup;
      err = MP_OKAY;
      goto cleanup;
   }

   /* NOW: Tonelli-Shanks algorithm */

   /* factor out powers of 2 from prime-1, defining Q and S as: prime-1 = Q*2^S */
   if ((err = mp_copy(prime, &Q)) != MP_OKAY)                    goto cleanup;
   if ((err = mp_sub_d(&Q, 1uL, &Q)) != MP_OKAY)                 goto cleanup;
   /* Q = prime - 1 */
   mp_zero(&S);
   /* S = 0 */
   while (MP_IS_EVEN(&Q)) {
      if ((err = mp_div_2(&Q, &Q)) != MP_OKAY)                    goto cleanup;
      /* Q = Q / 2 */
      if ((err = mp_add_d(&S, 1uL, &S)) != MP_OKAY)               goto cleanup;
      /* S = S + 1 */
   }

   /* find a Z such that the Legendre symbol (Z|prime) == -1 */
   mp_set_u32(&Z, 2u);
   /* Z = 2 */
   for (;;) {
      if ((err = mp_kronecker(&Z, prime, &legendre)) != MP_OKAY)     goto cleanup;
      if (legendre == -1) break;
      if ((err = mp_add_d(&Z, 1uL, &Z)) != MP_OKAY)               goto cleanup;
      /* Z = Z + 1 */
   }

   if ((err = mp_exptmod(&Z, &Q, prime, &C)) != MP_OKAY)         goto cleanup;
   /* C = Z ^ Q mod prime */
   if ((err = mp_add_d(&Q, 1uL, &t1)) != MP_OKAY)                goto cleanup;
   if ((err = mp_div_2(&t1, &t1)) != MP_OKAY)                    goto cleanup;
   /* t1 = (Q + 1) / 2 */
   if ((err = mp_exptmod(n, &t1, prime, &R)) != MP_OKAY)         goto cleanup;
   /* R = n ^ ((Q + 1) / 2) mod prime */
   if ((err = mp_exptmod(n, &Q, prime, &T)) != MP_OKAY)          goto cleanup;
   /* T = n ^ Q mod prime */
   if ((err = mp_copy(&S, &M)) != MP_OKAY)                       goto cleanup;
   /* M = S */
   mp_set_u32(&two, 2u);

   for (;;) {
      if ((err = mp_copy(&T, &t1)) != MP_OKAY)                    goto cleanup;
      i = 0;
      for (;;) {
         if (mp_cmp_d(&t1, 1uL) == MP_EQ) break;
         if ((err = mp_exptmod(&t1, &two, prime, &t1)) != MP_OKAY) goto cleanup;
         i++;
      }
      if (i == 0u) {
         if ((err = mp_copy(&R, ret)) != MP_OKAY)                  goto cleanup;
         err = MP_OKAY;
         goto cleanup;
      }
      if ((err = mp_sub_d(&M, i, &t1)) != MP_OKAY)                goto cleanup;
      if ((err = mp_sub_d(&t1, 1uL, &t1)) != MP_OKAY)             goto cleanup;
      if ((err = mp_exptmod(&two, &t1, prime, &t1)) != MP_OKAY)   goto cleanup;
      /* t1 = 2 ^ (M - i - 1) */
      if ((err = mp_exptmod(&C, &t1, prime, &t1)) != MP_OKAY)     goto cleanup;
      /* t1 = C ^ (2 ^ (M - i - 1)) mod prime */
      if ((err = mp_sqrmod(&t1, prime, &C)) != MP_OKAY)           goto cleanup;
      /* C = (t1 * t1) mod prime */
      if ((err = mp_mulmod(&R, &t1, prime, &R)) != MP_OKAY)       goto cleanup;
      /* R = (R * t1) mod prime */
      if ((err = mp_mulmod(&T, &C, prime, &T)) != MP_OKAY)        goto cleanup;
      /* T = (T * C) mod prime */
      mp_set(&M, i);
      /* M = i */
   }

cleanup:
   mp_clear_multi(&t1, &C, &Q, &S, &Z, &M, &T, &R, &two, NULL);
   return err;
}

#endif
