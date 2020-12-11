#include "tommath_private.h"
#ifdef BN_MP_SET_DOUBLE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#if defined(__STDC_IEC_559__) || defined(__GCC_IEC_559)
mp_err mp_set_double(mp_int *a, double b)
{
   uint64_t frac;
   int exp;
   mp_err err;
   union {
      double   dbl;
      uint64_t bits;
   } cast;
   cast.dbl = b;

   exp = (int)((unsigned)(cast.bits >> 52) & 0x7FFu);
   frac = (cast.bits & ((1uLL << 52) - 1uLL)) | (1uLL << 52);

   if (exp == 0x7FF) { /* +-inf, NaN */
      return MP_VAL;
   }
   exp -= 1023 + 52;

   mp_set_u64(a, frac);

   err = (exp < 0) ? mp_div_2d(a, -exp, a, NULL) : mp_mul_2d(a, exp, a);
   if (err != MP_OKAY) {
      return err;
   }

   if (((cast.bits >> 63) != 0uLL) && !MP_IS_ZERO(a)) {
      a->sign = MP_NEG;
   }

   return MP_OKAY;
}
#else
/* pragma message() not supported by several compilers (in mostly older but still used versions) */
#  ifdef _MSC_VER
#    pragma message("mp_set_double implementation is only available on platforms with IEEE754 floating point format")
#  else
#    warning "mp_set_double implementation is only available on platforms with IEEE754 floating point format"
#  endif
#endif
#endif
