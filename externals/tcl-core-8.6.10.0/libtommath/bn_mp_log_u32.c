#include "tommath_private.h"
#ifdef BN_MP_LOG_U32_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Compute log_{base}(a) */
static mp_word s_pow(mp_word base, mp_word exponent)
{
   mp_word result = 1;
   while (exponent != 0u) {
      if ((exponent & 1u) == 1u) {
         result *= base;
      }
      exponent >>= 1;
      base *= base;
   }

   return result;
}

static mp_digit s_digit_ilogb(mp_digit base, mp_digit n)
{
   mp_word bracket_low = 1, bracket_mid, bracket_high, N;
   mp_digit ret, high = 1uL, low = 0uL, mid;

   if (n < base) {
      return 0uL;
   }
   if (n == base) {
      return 1uL;
   }

   bracket_high = (mp_word) base ;
   N = (mp_word) n;

   while (bracket_high < N) {
      low = high;
      bracket_low = bracket_high;
      high <<= 1;
      bracket_high *= bracket_high;
   }

   while (((mp_digit)(high - low)) > 1uL) {
      mid = (low + high) >> 1;
      bracket_mid = bracket_low * s_pow(base, (mp_word)(mid - low));

      if (N < bracket_mid) {
         high = mid ;
         bracket_high = bracket_mid ;
      }
      if (N > bracket_mid) {
         low = mid ;
         bracket_low = bracket_mid ;
      }
      if (N == bracket_mid) {
         return (mp_digit) mid;
      }
   }

   if (bracket_high == N) {
      ret = high;
   } else {
      ret = low;
   }

   return ret;
}

/* TODO: output could be "int" because the output of mp_radix_size is int, too,
         as is the output of mp_bitcount.
         With the same problem: max size is INT_MAX * MP_DIGIT not INT_MAX only!
*/
mp_err mp_log_u32(const mp_int *a, unsigned int base, unsigned int *c)
{
   mp_err err;
   mp_ord cmp;
   unsigned int high, low, mid;
   mp_int bracket_low, bracket_high, bracket_mid, t, bi_base;

   err = MP_OKAY;

   if (a->sign == MP_NEG) {
      return MP_VAL;
   }

   if (MP_IS_ZERO(a)) {
      return MP_VAL;
   }

   if (base < 2u) {
      return MP_VAL;
   }

   /* A small shortcut for bases that are powers of two. */
   if ((base & (base - 1u)) == 0u) {
      int y, bit_count;
      for (y=0; (y < 7) && ((base & 1u) == 0u); y++) {
         base >>= 1;
      }
      bit_count = mp_count_bits(a) - 1;
      *c = (unsigned int)(bit_count/y);
      return MP_OKAY;
   }

   if (a->used == 1) {
      *c = (unsigned int)s_digit_ilogb(base, a->dp[0]);
      return err;
   }

   cmp = mp_cmp_d(a, base);
   if ((cmp == MP_LT) || (cmp == MP_EQ)) {
      *c = cmp == MP_EQ;
      return err;
   }

   if ((err =
           mp_init_multi(&bracket_low, &bracket_high,
                         &bracket_mid, &t, &bi_base, NULL)) != MP_OKAY) {
      return err;
   }

   low = 0u;
   mp_set(&bracket_low, 1uL);
   high = 1u;

   mp_set(&bracket_high, base);

   /*
       A kind of Giant-step/baby-step algorithm.
       Idea shamelessly stolen from https://programmingpraxis.com/2010/05/07/integer-logarithms/2/
       The effect is asymptotic, hence needs benchmarks to test if the Giant-step should be skipped
       for small n.
    */
   while (mp_cmp(&bracket_high, a) == MP_LT) {
      low = high;
      if ((err = mp_copy(&bracket_high, &bracket_low)) != MP_OKAY) {
         goto LBL_ERR;
      }
      high <<= 1;
      if ((err = mp_sqr(&bracket_high, &bracket_high)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }
   mp_set(&bi_base, base);

   while ((high - low) > 1u) {
      mid = (high + low) >> 1;

      if ((err = mp_expt_u32(&bi_base, mid - low, &t)) != MP_OKAY) {
         goto LBL_ERR;
      }
      if ((err = mp_mul(&bracket_low, &t, &bracket_mid)) != MP_OKAY) {
         goto LBL_ERR;
      }
      cmp = mp_cmp(a, &bracket_mid);
      if (cmp == MP_LT) {
         high = mid;
         mp_exch(&bracket_mid, &bracket_high);
      }
      if (cmp == MP_GT) {
         low = mid;
         mp_exch(&bracket_mid, &bracket_low);
      }
      if (cmp == MP_EQ) {
         *c = mid;
         goto LBL_END;
      }
   }

   *c = (mp_cmp(&bracket_high, a) == MP_EQ) ? high : low;

LBL_END:
LBL_ERR:
   mp_clear_multi(&bracket_low, &bracket_high, &bracket_mid,
                  &t, &bi_base, NULL);
   return err;
}


#endif
