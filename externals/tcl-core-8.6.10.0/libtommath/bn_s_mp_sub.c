#include "tommath_private.h"
#ifdef BN_S_MP_SUB_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* low level subtraction (assumes |a| > |b|), HAC pp.595 Algorithm 14.9 */
mp_err s_mp_sub(const mp_int *a, const mp_int *b, mp_int *c)
{
   int    olduse, min, max;
   mp_err err;

   /* find sizes */
   min = b->used;
   max = a->used;

   /* init result */
   if (c->alloc < max) {
      if ((err = mp_grow(c, max)) != MP_OKAY) {
         return err;
      }
   }
   olduse = c->used;
   c->used = max;

   {
      mp_digit u, *tmpa, *tmpb, *tmpc;
      int i;

      /* alias for digit pointers */
      tmpa = a->dp;
      tmpb = b->dp;
      tmpc = c->dp;

      /* set carry to zero */
      u = 0;
      for (i = 0; i < min; i++) {
         /* T[i] = A[i] - B[i] - U */
         *tmpc = (*tmpa++ - *tmpb++) - u;

         /* U = carry bit of T[i]
          * Note this saves performing an AND operation since
          * if a carry does occur it will propagate all the way to the
          * MSB.  As a result a single shift is enough to get the carry
          */
         u = *tmpc >> (MP_SIZEOF_BITS(mp_digit) - 1u);

         /* Clear carry from T[i] */
         *tmpc++ &= MP_MASK;
      }

      /* now copy higher words if any, e.g. if A has more digits than B  */
      for (; i < max; i++) {
         /* T[i] = A[i] - U */
         *tmpc = *tmpa++ - u;

         /* U = carry bit of T[i] */
         u = *tmpc >> (MP_SIZEOF_BITS(mp_digit) - 1u);

         /* Clear carry from T[i] */
         *tmpc++ &= MP_MASK;
      }

      /* clear digits above used (since we may not have grown result above) */
      MP_ZERO_DIGITS(tmpc, olduse - c->used);
   }

   mp_clamp(c);
   return MP_OKAY;
}

#endif
