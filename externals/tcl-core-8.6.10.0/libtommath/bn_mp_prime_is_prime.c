#include "tommath_private.h"
#ifdef BN_MP_PRIME_IS_PRIME_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* portable integer log of two with small footprint */
static unsigned int s_floor_ilog2(int value)
{
   unsigned int r = 0;
   while ((value >>= 1) != 0) {
      r++;
   }
   return r;
}


mp_err mp_prime_is_prime(const mp_int *a, int t, mp_bool *result)
{
   mp_int  b;
   int     ix, p_max = 0, size_a, len;
   mp_bool res;
   mp_err  err;
   unsigned int fips_rand, mask;

   /* default to no */
   *result = MP_NO;

   /* Some shortcuts */
   /* N > 3 */
   if (a->used == 1) {
      if ((a->dp[0] == 0u) || (a->dp[0] == 1u)) {
         *result = MP_NO;
         return MP_OKAY;
      }
      if (a->dp[0] == 2u) {
         *result = MP_YES;
         return MP_OKAY;
      }
   }

   /* N must be odd */
   if (MP_IS_EVEN(a)) {
      return MP_OKAY;
   }
   /* N is not a perfect square: floor(sqrt(N))^2 != N */
   if ((err = mp_is_square(a, &res)) != MP_OKAY) {
      return err;
   }
   if (res != MP_NO) {
      return MP_OKAY;
   }

   /* is the input equal to one of the primes in the table? */
   for (ix = 0; ix < PRIVATE_MP_PRIME_TAB_SIZE; ix++) {
      if (mp_cmp_d(a, s_mp_prime_tab[ix]) == MP_EQ) {
         *result = MP_YES;
         return MP_OKAY;
      }
   }
#ifdef MP_8BIT
   /* The search in the loop above was exhaustive in this case */
   if ((a->used == 1) && (PRIVATE_MP_PRIME_TAB_SIZE >= 31)) {
      return MP_OKAY;
   }
#endif

   /* first perform trial division */
   if ((err = s_mp_prime_is_divisible(a, &res)) != MP_OKAY) {
      return err;
   }

   /* return if it was trivially divisible */
   if (res == MP_YES) {
      return MP_OKAY;
   }

   /*
       Run the Miller-Rabin test with base 2 for the BPSW test.
    */
   if ((err = mp_init_set(&b, 2uL)) != MP_OKAY) {
      return err;
   }

   if ((err = mp_prime_miller_rabin(a, &b, &res)) != MP_OKAY) {
      goto LBL_B;
   }
   if (res == MP_NO) {
      goto LBL_B;
   }
   /*
      Rumours have it that Mathematica does a second M-R test with base 3.
      Other rumours have it that their strong L-S test is slightly different.
      It does not hurt, though, beside a bit of extra runtime.
   */
   b.dp[0]++;
   if ((err = mp_prime_miller_rabin(a, &b, &res)) != MP_OKAY) {
      goto LBL_B;
   }
   if (res == MP_NO) {
      goto LBL_B;
   }

   /*
    * Both, the Frobenius-Underwood test and the the Lucas-Selfridge test are quite
    * slow so if speed is an issue, define LTM_USE_ONLY_MR to use M-R tests with
    * bases 2, 3 and t random bases.
    */
#ifndef LTM_USE_ONLY_MR
   if (t >= 0) {
      /*
       * Use a Frobenius-Underwood test instead of the Lucas-Selfridge test for
       * MP_8BIT (It is unknown if the Lucas-Selfridge test works with 16-bit
       * integers but the necesssary analysis is on the todo-list).
       */
#if defined (MP_8BIT) || defined (LTM_USE_FROBENIUS_TEST)
      err = mp_prime_frobenius_underwood(a, &res);
      if ((err != MP_OKAY) && (err != MP_ITER)) {
         goto LBL_B;
      }
      if (res == MP_NO) {
         goto LBL_B;
      }
#else
      if ((err = mp_prime_strong_lucas_selfridge(a, &res)) != MP_OKAY) {
         goto LBL_B;
      }
      if (res == MP_NO) {
         goto LBL_B;
      }
#endif
   }
#endif

   /* run at least one Miller-Rabin test with a random base */
   if (t == 0) {
      t = 1;
   }

   /*
      Only recommended if the input range is known to be < 3317044064679887385961981

      It uses the bases necessary for a deterministic M-R test if the input is
      smaller than  3317044064679887385961981
      The caller has to check the size.
      TODO: can be made a bit finer grained but comparing is not free.
   */
   if (t < 0) {
      /*
          Sorenson, Jonathan; Webster, Jonathan (2015).
           "Strong Pseudoprimes to Twelve Prime Bases".
       */
      /* 0x437ae92817f9fc85b7e5 = 318665857834031151167461 */
      if ((err =   mp_read_radix(&b, "437ae92817f9fc85b7e5", 16)) != MP_OKAY) {
         goto LBL_B;
      }

      if (mp_cmp(a, &b) == MP_LT) {
         p_max = 12;
      } else {
         /* 0x2be6951adc5b22410a5fd = 3317044064679887385961981 */
         if ((err = mp_read_radix(&b, "2be6951adc5b22410a5fd", 16)) != MP_OKAY) {
            goto LBL_B;
         }

         if (mp_cmp(a, &b) == MP_LT) {
            p_max = 13;
         } else {
            err = MP_VAL;
            goto LBL_B;
         }
      }

      /* we did bases 2 and 3  already, skip them */
      for (ix = 2; ix < p_max; ix++) {
         mp_set(&b, s_mp_prime_tab[ix]);
         if ((err = mp_prime_miller_rabin(a, &b, &res)) != MP_OKAY) {
            goto LBL_B;
         }
         if (res == MP_NO) {
            goto LBL_B;
         }
      }
   }
   /*
       Do "t" M-R tests with random bases between 3 and "a".
       See Fips 186.4 p. 126ff
   */
   else if (t > 0) {
      /*
       * The mp_digit's have a defined bit-size but the size of the
       * array a.dp is a simple 'int' and this library can not assume full
       * compliance to the current C-standard (ISO/IEC 9899:2011) because
       * it gets used for small embeded processors, too. Some of those MCUs
       * have compilers that one cannot call standard compliant by any means.
       * Hence the ugly type-fiddling in the following code.
       */
      size_a = mp_count_bits(a);
      mask = (1u << s_floor_ilog2(size_a)) - 1u;
      /*
         Assuming the General Rieman hypothesis (never thought to write that in a
         comment) the upper bound can be lowered to  2*(log a)^2.
         E. Bach, "Explicit bounds for primality testing and related problems,"
         Math. Comp. 55 (1990), 355-380.

            size_a = (size_a/10) * 7;
            len = 2 * (size_a * size_a);

         E.g.: a number of size 2^2048 would be reduced to the upper limit

            floor(2048/10)*7 = 1428
            2 * 1428^2       = 4078368

         (would have been ~4030331.9962 with floats and natural log instead)
         That number is smaller than 2^28, the default bit-size of mp_digit.
      */

      /*
        How many tests, you might ask? Dana Jacobsen of Math::Prime::Util fame
        does exactly 1. In words: one. Look at the end of _GMP_is_prime() in
        Math-Prime-Util-GMP-0.50/primality.c if you do not believe it.

        The function mp_rand() goes to some length to use a cryptographically
        good PRNG. That also means that the chance to always get the same base
        in the loop is non-zero, although very low.
        If the BPSW test and/or the addtional Frobenious test have been
        performed instead of just the Miller-Rabin test with the bases 2 and 3,
        a single extra test should suffice, so such a very unlikely event
        will not do much harm.

        To preemptivly answer the dangling question: no, a witness does not
        need to be prime.
      */
      for (ix = 0; ix < t; ix++) {
         /* mp_rand() guarantees the first digit to be non-zero */
         if ((err = mp_rand(&b, 1)) != MP_OKAY) {
            goto LBL_B;
         }
         /*
          * Reduce digit before casting because mp_digit might be bigger than
          * an unsigned int and "mask" on the other side is most probably not.
          */
         fips_rand = (unsigned int)(b.dp[0] & (mp_digit) mask);
#ifdef MP_8BIT
         /*
          * One 8-bit digit is too small, so concatenate two if the size of
          * unsigned int allows for it.
          */
         if ((MP_SIZEOF_BITS(unsigned int)/2) >= MP_SIZEOF_BITS(mp_digit)) {
            if ((err = mp_rand(&b, 1)) != MP_OKAY) {
               goto LBL_B;
            }
            fips_rand <<= MP_SIZEOF_BITS(mp_digit);
            fips_rand |= (unsigned int) b.dp[0];
            fips_rand &= mask;
         }
#endif
         if (fips_rand > (unsigned int)(INT_MAX - MP_DIGIT_BIT)) {
            len = INT_MAX / MP_DIGIT_BIT;
         } else {
            len = (((int)fips_rand + MP_DIGIT_BIT) / MP_DIGIT_BIT);
         }
         /*  Unlikely. */
         if (len < 0) {
            ix--;
            continue;
         }
         /*
          * As mentioned above, one 8-bit digit is too small and
          * although it can only happen in the unlikely case that
          * an "unsigned int" is smaller than 16 bit a simple test
          * is cheap and the correction even cheaper.
          */
#ifdef MP_8BIT
         /* All "a" < 2^8 have been caught before */
         if (len == 1) {
            len++;
         }
#endif
         if ((err = mp_rand(&b, len)) != MP_OKAY) {
            goto LBL_B;
         }
         /*
          * That number might got too big and the witness has to be
          * smaller than "a"
          */
         len = mp_count_bits(&b);
         if (len >= size_a) {
            len = (len - size_a) + 1;
            if ((err = mp_div_2d(&b, len, &b, NULL)) != MP_OKAY) {
               goto LBL_B;
            }
         }
         /* Although the chance for b <= 3 is miniscule, try again. */
         if (mp_cmp_d(&b, 3uL) != MP_GT) {
            ix--;
            continue;
         }
         if ((err = mp_prime_miller_rabin(a, &b, &res)) != MP_OKAY) {
            goto LBL_B;
         }
         if (res == MP_NO) {
            goto LBL_B;
         }
      }
   }

   /* passed the test */
   *result = MP_YES;
LBL_B:
   mp_clear(&b);
   return err;
}

#endif
