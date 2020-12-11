#include "tommath_private.h"
#ifdef BN_MP_PRIME_FERMAT_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* performs one Fermat test.
 *
 * If "a" were prime then b**a == b (mod a) since the order of
 * the multiplicative sub-group would be phi(a) = a-1.  That means
 * it would be the same as b**(a mod (a-1)) == b**1 == b (mod a).
 *
 * Sets result to 1 if the congruence holds, or zero otherwise.
 */
mp_err mp_prime_fermat(const mp_int *a, const mp_int *b, mp_bool *result)
{
   mp_int  t;
   mp_err  err;

   /* default to composite  */
   *result = MP_NO;

   /* ensure b > 1 */
   if (mp_cmp_d(b, 1uL) != MP_GT) {
      return MP_VAL;
   }

   /* init t */
   if ((err = mp_init(&t)) != MP_OKAY) {
      return err;
   }

   /* compute t = b**a mod a */
   if ((err = mp_exptmod(b, a, a, &t)) != MP_OKAY) {
      goto LBL_T;
   }

   /* is it equal to b? */
   if (mp_cmp(&t, b) == MP_EQ) {
      *result = MP_YES;
   }

   err = MP_OKAY;
LBL_T:
   mp_clear(&t);
   return err;
}
#endif
