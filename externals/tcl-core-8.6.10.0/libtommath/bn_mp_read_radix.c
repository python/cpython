#include "tommath_private.h"
#ifdef BN_MP_READ_RADIX_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#define MP_TOUPPER(c) ((((c) >= 'a') && ((c) <= 'z')) ? (((c) + 'A') - 'a') : (c))

/* read a string [ASCII] in a given radix */
mp_err mp_read_radix(mp_int *a, const char *str, int radix)
{
   mp_err   err;
   int      y;
   mp_sign  neg;
   unsigned pos;
   char     ch;

   /* zero the digit bignum */
   mp_zero(a);

   /* make sure the radix is ok */
   if ((radix < 2) || (radix > 64)) {
      return MP_VAL;
   }

   /* if the leading digit is a
    * minus set the sign to negative.
    */
   if (*str == '-') {
      ++str;
      neg = MP_NEG;
   } else {
      neg = MP_ZPOS;
   }

   /* set the integer to the default of zero */
   mp_zero(a);

   /* process each digit of the string */
   while (*str != '\0') {
      /* if the radix <= 36 the conversion is case insensitive
       * this allows numbers like 1AB and 1ab to represent the same  value
       * [e.g. in hex]
       */
      ch = (radix <= 36) ? (char)MP_TOUPPER((int)*str) : *str;
      pos = (unsigned)(ch - '(');
      if (mp_s_rmap_reverse_sz < pos) {
         break;
      }
      y = (int)mp_s_rmap_reverse[pos];

      /* if the char was found in the map
       * and is less than the given radix add it
       * to the number, otherwise exit the loop.
       */
      if ((y == 0xff) || (y >= radix)) {
         break;
      }
      if ((err = mp_mul_d(a, (mp_digit)radix, a)) != MP_OKAY) {
         return err;
      }
      if ((err = mp_add_d(a, (mp_digit)y, a)) != MP_OKAY) {
         return err;
      }
      ++str;
   }

   /* if an illegal character was found, fail. */
   if (!((*str == '\0') || (*str == '\r') || (*str == '\n'))) {
      mp_zero(a);
      return MP_VAL;
   }

   /* set the sign only if a != 0 */
   if (!MP_IS_ZERO(a)) {
      a->sign = neg;
   }
   return MP_OKAY;
}
#endif
