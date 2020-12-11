#include "tommath_private.h"
#ifdef BN_MP_EXPTMOD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* this is a shell function that calls either the normal or Montgomery
 * exptmod functions.  Originally the call to the montgomery code was
 * embedded in the normal function but that wasted alot of stack space
 * for nothing (since 99% of the time the Montgomery code would be called)
 */
mp_err mp_exptmod(const mp_int *G, const mp_int *X, const mp_int *P, mp_int *Y)
{
   int dr;

   /* modulus P must be positive */
   if (P->sign == MP_NEG) {
      return MP_VAL;
   }

   /* if exponent X is negative we have to recurse */
   if (X->sign == MP_NEG) {
      mp_int tmpG, tmpX;
      mp_err err;

      if (!MP_HAS(MP_INVMOD)) {
         return MP_VAL;
      }

      if ((err = mp_init_multi(&tmpG, &tmpX, NULL)) != MP_OKAY) {
         return err;
      }

      /* first compute 1/G mod P */
      if ((err = mp_invmod(G, P, &tmpG)) != MP_OKAY) {
         goto LBL_ERR;
      }

      /* now get |X| */
      if ((err = mp_abs(X, &tmpX)) != MP_OKAY) {
         goto LBL_ERR;
      }

      /* and now compute (1/G)**|X| instead of G**X [X < 0] */
      err = mp_exptmod(&tmpG, &tmpX, P, Y);
LBL_ERR:
      mp_clear_multi(&tmpG, &tmpX, NULL);
      return err;
   }

   /* modified diminished radix reduction */
   if (MP_HAS(MP_REDUCE_IS_2K_L) && MP_HAS(MP_REDUCE_2K_L) && MP_HAS(S_MP_EXPTMOD) &&
       (mp_reduce_is_2k_l(P) == MP_YES)) {
      return s_mp_exptmod(G, X, P, Y, 1);
   }

   /* is it a DR modulus? default to no */
   dr = (MP_HAS(MP_DR_IS_MODULUS) && (mp_dr_is_modulus(P) == MP_YES)) ? 1 : 0;

   /* if not, is it a unrestricted DR modulus? */
   if (MP_HAS(MP_REDUCE_IS_2K) && (dr == 0)) {
      dr = (mp_reduce_is_2k(P) == MP_YES) ? 2 : 0;
   }

   /* if the modulus is odd or dr != 0 use the montgomery method */
   if (MP_HAS(S_MP_EXPTMOD_FAST) && (MP_IS_ODD(P) || (dr != 0))) {
      return s_mp_exptmod_fast(G, X, P, Y, dr);
   } else if (MP_HAS(S_MP_EXPTMOD)) {
      /* otherwise use the generic Barrett reduction technique */
      return s_mp_exptmod(G, X, P, Y, 0);
   } else {
      /* no exptmod for evens */
      return MP_VAL;
   }
}

#endif
