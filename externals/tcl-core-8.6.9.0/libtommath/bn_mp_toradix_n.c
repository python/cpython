#include <tommath.h>
#ifdef BN_MP_TORADIX_N_C
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
 * Tom St Denis, tomstdenis@gmail.com, http://math.libtomcrypt.com
 */

/* stores a bignum as a ASCII string in a given radix (2..64) 
 *
 * Stores upto maxlen-1 chars and always a NULL byte 
 */
int mp_toradix_n(mp_int * a, char *str, int radix, int maxlen)
{
  int     res, digs;
  mp_int  t;
  mp_digit d;
  char   *_s = str;

  /* check range of the maxlen, radix */
  if (maxlen < 2 || radix < 2 || radix > 64) {
    return MP_VAL;
  }

  /* quick out if its zero */
  if (mp_iszero(a) == MP_YES) {
     *str++ = '0';
     *str = '\0';
     return MP_OKAY;
  }

  if ((res = mp_init_copy (&t, a)) != MP_OKAY) {
    return res;
  }

  /* if it is negative output a - */
  if (t.sign == MP_NEG) {
    /* we have to reverse our digits later... but not the - sign!! */
    ++_s;

    /* store the flag and mark the number as positive */
    *str++ = '-';
    t.sign = MP_ZPOS;
 
    /* subtract a char */
    --maxlen;
  }

  digs = 0;
  while (mp_iszero (&t) == 0) {
    if (--maxlen < 1) {
       /* no more room */
       break;
    }
    if ((res = mp_div_d (&t, (mp_digit) radix, &t, &d)) != MP_OKAY) {
      mp_clear (&t);
      return res;
    }
    *str++ = mp_s_rmap[d];
    ++digs;
  }

  /* reverse the digits of the string.  In this case _s points
   * to the first digit [exluding the sign] of the number
   */
  bn_reverse ((unsigned char *)_s, digs);

  /* append a NULL so the string is properly terminated */
  *str = '\0';

  mp_clear (&t);
  return MP_OKAY;
}

#endif
