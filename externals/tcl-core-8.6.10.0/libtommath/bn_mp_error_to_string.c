#include "tommath_private.h"
#ifdef BN_MP_ERROR_TO_STRING_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* return a char * string for a given code */
const char *mp_error_to_string(mp_err code)
{
   switch (code) {
   case MP_OKAY:
      return "Successful";
   case MP_ERR:
      return "Unknown error";
   case MP_MEM:
      return "Out of heap";
   case MP_VAL:
      return "Value out of range";
   case MP_ITER:
      return "Max. iterations reached";
   case MP_BUF:
      return "Buffer overflow";
   default:
      return "Invalid error code";
   }
}

#endif
