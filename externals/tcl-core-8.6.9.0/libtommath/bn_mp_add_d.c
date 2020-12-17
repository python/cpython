#include <tommath.h>
#ifdef BN_MP_ADD_D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis
 *
 * LibTomMath is a library that provides multiple-precision
 * integer arithmetic as well as number theoretic functionality.
 *
 * The library was designed directly after the MPI library by
 * Michael Fromberger but has been written from scratch with
 * additional optimizations in place.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* single digit addition */
int
mp_add_d (mp_int * a, mp_digit b, mp_int * c)
{
  int     res, ix, oldused;
  mp_digit *tmpa, *tmpc, mu;

  /* grow c as required */
  if (c->alloc < a->used + 1) {
     if ((res = mp_grow(c, a->used + 1)) != MP_OKAY) {
        return res;
     }
  }

  /* if a is negative and |a| >= b, call c = |a| - b */
  if (a->sign == MP_NEG && (a->used > 1 || a->dp[0] >= b)) {
     /* temporarily fix sign of a */
     a->sign = MP_ZPOS;

     /* c = |a| - b */
     res = mp_sub_d(a, b, c);

     /* fix sign  */
     a->sign = c->sign = MP_NEG;

     /* clamp */
     mp_clamp(c);

     return res;
  }

  /* old number of used digits in c */
  oldused = c->used;

  /* sign always positive */
  c->sign = MP_ZPOS;

  /* source alias */
  tmpa    = a->dp;

  /* destination alias */
  tmpc    = c->dp;

  /* if a is positive */
  if (a->sign == MP_ZPOS) {
     /* add digit, after this we're propagating
      * the carry.
      */
     *tmpc   = *tmpa++ + b;
     mu      = *tmpc >> DIGIT_BIT;
     *tmpc++ &= MP_MASK;

     /* now handle rest of the digits */
     for (ix = 1; ix < a->used; ix++) {
        *tmpc   = *tmpa++ + mu;
        mu      = *tmpc >> DIGIT_BIT;
        *tmpc++ &= MP_MASK;
     }
     /* set final carry */
     ix++;
     *tmpc++  = mu;

     /* setup size */
     c->used = a->used + 1;
  } else {
     /* a was negative and |a| < b */
     c->used  = 1;

     /* the result is a single digit */
     if (a->used == 1) {
        *tmpc++  =  b - a->dp[0];
     } else {
        *tmpc++  =  b;
     }

     /* setup count so the clearing of oldused
      * can fall through correctly
      */
     ix       = 1;
  }

  /* now zero to oldused */
  while (ix++ < oldused) {
     *tmpc++ = 0;
  }
  mp_clamp(c);

  return MP_OKAY;
}

#endif

/* $Source$ */
/* $Revision: 0.41 $ */
/* $Date: 2007-04-18 09:58:18 +0000 $ */
