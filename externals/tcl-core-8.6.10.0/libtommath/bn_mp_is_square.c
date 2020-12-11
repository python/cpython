#include "tommath_private.h"
#ifdef BN_MP_IS_SQUARE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Check if remainders are possible squares - fast exclude non-squares */
static const char rem_128[128] = {
   0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
   0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
   1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
   1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
   0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
   1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
   1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
   1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1
};

static const char rem_105[105] = {
   0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
   0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1,
   0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1,
   1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
   0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
   1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1,
   1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1
};

/* Store non-zero to ret if arg is square, and zero if not */
mp_err mp_is_square(const mp_int *arg, mp_bool *ret)
{
   mp_err        err;
   mp_digit      c;
   mp_int        t;
   unsigned long r;

   /* Default to Non-square :) */
   *ret = MP_NO;

   if (arg->sign == MP_NEG) {
      return MP_VAL;
   }

   if (MP_IS_ZERO(arg)) {
      return MP_OKAY;
   }

   /* First check mod 128 (suppose that MP_DIGIT_BIT is at least 7) */
   if (rem_128[127u & arg->dp[0]] == (char)1) {
      return MP_OKAY;
   }

   /* Next check mod 105 (3*5*7) */
   if ((err = mp_mod_d(arg, 105uL, &c)) != MP_OKAY) {
      return err;
   }
   if (rem_105[c] == (char)1) {
      return MP_OKAY;
   }


   if ((err = mp_init_u32(&t, 11u*13u*17u*19u*23u*29u*31u)) != MP_OKAY) {
      return err;
   }
   if ((err = mp_mod(arg, &t, &t)) != MP_OKAY) {
      goto LBL_ERR;
   }
   r = mp_get_u32(&t);
   /* Check for other prime modules, note it's not an ERROR but we must
    * free "t" so the easiest way is to goto LBL_ERR.  We know that err
    * is already equal to MP_OKAY from the mp_mod call
    */
   if (((1uL<<(r%11uL)) & 0x5C4uL) != 0uL)         goto LBL_ERR;
   if (((1uL<<(r%13uL)) & 0x9E4uL) != 0uL)         goto LBL_ERR;
   if (((1uL<<(r%17uL)) & 0x5CE8uL) != 0uL)        goto LBL_ERR;
   if (((1uL<<(r%19uL)) & 0x4F50CuL) != 0uL)       goto LBL_ERR;
   if (((1uL<<(r%23uL)) & 0x7ACCA0uL) != 0uL)      goto LBL_ERR;
   if (((1uL<<(r%29uL)) & 0xC2EDD0CuL) != 0uL)     goto LBL_ERR;
   if (((1uL<<(r%31uL)) & 0x6DE2B848uL) != 0uL)    goto LBL_ERR;

   /* Final check - is sqr(sqrt(arg)) == arg ? */
   if ((err = mp_sqrt(arg, &t)) != MP_OKAY) {
      goto LBL_ERR;
   }
   if ((err = mp_sqr(&t, &t)) != MP_OKAY) {
      goto LBL_ERR;
   }

   *ret = (mp_cmp_mag(&t, arg) == MP_EQ) ? MP_YES : MP_NO;
LBL_ERR:
   mp_clear(&t);
   return err;
}
#endif
