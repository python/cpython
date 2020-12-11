#include "tommath_private.h"
#ifdef BN_MP_EXTEUCLID_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* Extended euclidean algorithm of (a, b) produces
   a*u1 + b*u2 = u3
 */
mp_err mp_exteuclid(const mp_int *a, const mp_int *b, mp_int *U1, mp_int *U2, mp_int *U3)
{
   mp_int u1, u2, u3, v1, v2, v3, t1, t2, t3, q, tmp;
   mp_err err;

   if ((err = mp_init_multi(&u1, &u2, &u3, &v1, &v2, &v3, &t1, &t2, &t3, &q, &tmp, NULL)) != MP_OKAY) {
      return err;
   }

   /* initialize, (u1,u2,u3) = (1,0,a) */
   mp_set(&u1, 1uL);
   if ((err = mp_copy(a, &u3)) != MP_OKAY)                        goto LBL_ERR;

   /* initialize, (v1,v2,v3) = (0,1,b) */
   mp_set(&v2, 1uL);
   if ((err = mp_copy(b, &v3)) != MP_OKAY)                        goto LBL_ERR;

   /* loop while v3 != 0 */
   while (!MP_IS_ZERO(&v3)) {
      /* q = u3/v3 */
      if ((err = mp_div(&u3, &v3, &q, NULL)) != MP_OKAY)          goto LBL_ERR;

      /* (t1,t2,t3) = (u1,u2,u3) - (v1,v2,v3)q */
      if ((err = mp_mul(&v1, &q, &tmp)) != MP_OKAY)               goto LBL_ERR;
      if ((err = mp_sub(&u1, &tmp, &t1)) != MP_OKAY)              goto LBL_ERR;
      if ((err = mp_mul(&v2, &q, &tmp)) != MP_OKAY)               goto LBL_ERR;
      if ((err = mp_sub(&u2, &tmp, &t2)) != MP_OKAY)              goto LBL_ERR;
      if ((err = mp_mul(&v3, &q, &tmp)) != MP_OKAY)               goto LBL_ERR;
      if ((err = mp_sub(&u3, &tmp, &t3)) != MP_OKAY)              goto LBL_ERR;

      /* (u1,u2,u3) = (v1,v2,v3) */
      if ((err = mp_copy(&v1, &u1)) != MP_OKAY)                   goto LBL_ERR;
      if ((err = mp_copy(&v2, &u2)) != MP_OKAY)                   goto LBL_ERR;
      if ((err = mp_copy(&v3, &u3)) != MP_OKAY)                   goto LBL_ERR;

      /* (v1,v2,v3) = (t1,t2,t3) */
      if ((err = mp_copy(&t1, &v1)) != MP_OKAY)                   goto LBL_ERR;
      if ((err = mp_copy(&t2, &v2)) != MP_OKAY)                   goto LBL_ERR;
      if ((err = mp_copy(&t3, &v3)) != MP_OKAY)                   goto LBL_ERR;
   }

   /* make sure U3 >= 0 */
   if (u3.sign == MP_NEG) {
      if ((err = mp_neg(&u1, &u1)) != MP_OKAY)                    goto LBL_ERR;
      if ((err = mp_neg(&u2, &u2)) != MP_OKAY)                    goto LBL_ERR;
      if ((err = mp_neg(&u3, &u3)) != MP_OKAY)                    goto LBL_ERR;
   }

   /* copy result out */
   if (U1 != NULL) {
      mp_exch(U1, &u1);
   }
   if (U2 != NULL) {
      mp_exch(U2, &u2);
   }
   if (U3 != NULL) {
      mp_exch(U3, &u3);
   }

   err = MP_OKAY;
LBL_ERR:
   mp_clear_multi(&u1, &u2, &u3, &v1, &v2, &v3, &t1, &t2, &t3, &q, &tmp, NULL);
   return err;
}
#endif
