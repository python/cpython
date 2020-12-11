#include "tommath_private.h"
#ifdef BN_MP_MONTGOMERY_SETUP_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* setups the montgomery reduction stuff */
mp_err mp_montgomery_setup(const mp_int *n, mp_digit *rho)
{
   mp_digit x, b;

   /* fast inversion mod 2**k
    *
    * Based on the fact that
    *
    * XA = 1 (mod 2**n)  =>  (X(2-XA)) A = 1 (mod 2**2n)
    *                    =>  2*X*A - X*X*A*A = 1
    *                    =>  2*(1) - (1)     = 1
    */
   b = n->dp[0];

   if ((b & 1u) == 0u) {
      return MP_VAL;
   }

   x = (((b + 2u) & 4u) << 1) + b; /* here x*a==1 mod 2**4 */
   x *= 2u - (b * x);              /* here x*a==1 mod 2**8 */
#if !defined(MP_8BIT)
   x *= 2u - (b * x);              /* here x*a==1 mod 2**16 */
#endif
#if defined(MP_64BIT) || !(defined(MP_8BIT) || defined(MP_16BIT))
   x *= 2u - (b * x);              /* here x*a==1 mod 2**32 */
#endif
#ifdef MP_64BIT
   x *= 2u - (b * x);              /* here x*a==1 mod 2**64 */
#endif

   /* rho = -1/m mod b */
   *rho = (mp_digit)(((mp_word)1 << (mp_word)MP_DIGIT_BIT) - x) & MP_MASK;

   return MP_OKAY;
}
#endif
