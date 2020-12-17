#include "tommath_private.h"
#ifdef BN_S_MP_PRIME_IS_DIVISIBLE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* determines if an integers is divisible by one
 * of the first PRIME_SIZE primes or not
 *
 * sets result to 0 if not, 1 if yes
 */
mp_err s_mp_prime_is_divisible(const mp_int *a, mp_bool *result)
{
   int      ix;
   mp_err   err;
   mp_digit res;

   /* default to not */
   *result = MP_NO;

   for (ix = 0; ix < PRIVATE_MP_PRIME_TAB_SIZE; ix++) {
      /* what is a mod LBL_prime_tab[ix] */
      if ((err = mp_mod_d(a, s_mp_prime_tab[ix], &res)) != MP_OKAY) {
         return err;
      }

      /* is the residue zero? */
      if (res == 0u) {
         *result = MP_YES;
         return MP_OKAY;
      }
   }

   return MP_OKAY;
}
#endif
