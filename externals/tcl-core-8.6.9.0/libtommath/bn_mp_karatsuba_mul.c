#include <tommath.h>
#ifdef BN_MP_KARATSUBA_MUL_C
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

/* c = |a| * |b| using Karatsuba Multiplication using 
 * three half size multiplications
 *
 * Let B represent the radix [e.g. 2**DIGIT_BIT] and 
 * let n represent half of the number of digits in 
 * the min(a,b)
 *
 * a = a1 * B**n + a0
 * b = b1 * B**n + b0
 *
 * Then, a * b => 
   a1b1 * B**2n + ((a1 + a0)(b1 + b0) - (a0b0 + a1b1)) * B + a0b0
 *
 * Note that a1b1 and a0b0 are used twice and only need to be 
 * computed once.  So in total three half size (half # of 
 * digit) multiplications are performed, a0b0, a1b1 and 
 * (a1+b1)(a0+b0)
 *
 * Note that a multiplication of half the digits requires
 * 1/4th the number of single precision multiplications so in 
 * total after one call 25% of the single precision multiplications 
 * are saved.  Note also that the call to mp_mul can end up back 
 * in this function if the a0, a1, b0, or b1 are above the threshold.  
 * This is known as divide-and-conquer and leads to the famous 
 * O(N**lg(3)) or O(N**1.584) work which is asymptopically lower than 
 * the standard O(N**2) that the baseline/comba methods use.  
 * Generally though the overhead of this method doesn't pay off 
 * until a certain size (N ~ 80) is reached.
 */
int mp_karatsuba_mul (mp_int * a, mp_int * b, mp_int * c)
{
  mp_int  x0, x1, y0, y1, t1, x0y0, x1y1;
  int     B, err;

  /* default the return code to an error */
  err = MP_MEM;

  /* min # of digits */
  B = MIN (a->used, b->used);

  /* now divide in two */
  B = B >> 1;

  /* init copy all the temps */
  if (mp_init_size (&x0, B) != MP_OKAY)
    goto ERR;
  if (mp_init_size (&x1, a->used - B) != MP_OKAY)
    goto X0;
  if (mp_init_size (&y0, B) != MP_OKAY)
    goto X1;
  if (mp_init_size (&y1, b->used - B) != MP_OKAY)
    goto Y0;

  /* init temps */
  if (mp_init_size (&t1, B * 2) != MP_OKAY)
    goto Y1;
  if (mp_init_size (&x0y0, B * 2) != MP_OKAY)
    goto T1;
  if (mp_init_size (&x1y1, B * 2) != MP_OKAY)
    goto X0Y0;

  /* now shift the digits */
  x0.used = y0.used = B;
  x1.used = a->used - B;
  y1.used = b->used - B;

  {
    register int x;
    register mp_digit *tmpa, *tmpb, *tmpx, *tmpy;

    /* we copy the digits directly instead of using higher level functions
     * since we also need to shift the digits
     */
    tmpa = a->dp;
    tmpb = b->dp;

    tmpx = x0.dp;
    tmpy = y0.dp;
    for (x = 0; x < B; x++) {
      *tmpx++ = *tmpa++;
      *tmpy++ = *tmpb++;
    }

    tmpx = x1.dp;
    for (x = B; x < a->used; x++) {
      *tmpx++ = *tmpa++;
    }

    tmpy = y1.dp;
    for (x = B; x < b->used; x++) {
      *tmpy++ = *tmpb++;
    }
  }

  /* only need to clamp the lower words since by definition the 
   * upper words x1/y1 must have a known number of digits
   */
  mp_clamp (&x0);
  mp_clamp (&y0);

  /* now calc the products x0y0 and x1y1 */
  /* after this x0 is no longer required, free temp [x0==t2]! */
  if (mp_mul (&x0, &y0, &x0y0) != MP_OKAY)  
    goto X1Y1;          /* x0y0 = x0*y0 */
  if (mp_mul (&x1, &y1, &x1y1) != MP_OKAY)
    goto X1Y1;          /* x1y1 = x1*y1 */

  /* now calc x1+x0 and y1+y0 */
  if (s_mp_add (&x1, &x0, &t1) != MP_OKAY)
    goto X1Y1;          /* t1 = x1 - x0 */
  if (s_mp_add (&y1, &y0, &x0) != MP_OKAY)
    goto X1Y1;          /* t2 = y1 - y0 */
  if (mp_mul (&t1, &x0, &t1) != MP_OKAY)
    goto X1Y1;          /* t1 = (x1 + x0) * (y1 + y0) */

  /* add x0y0 */
  if (mp_add (&x0y0, &x1y1, &x0) != MP_OKAY)
    goto X1Y1;          /* t2 = x0y0 + x1y1 */
  if (s_mp_sub (&t1, &x0, &t1) != MP_OKAY)
    goto X1Y1;          /* t1 = (x1+x0)*(y1+y0) - (x1y1 + x0y0) */

  /* shift by B */
  if (mp_lshd (&t1, B) != MP_OKAY)
    goto X1Y1;          /* t1 = (x0y0 + x1y1 - (x1-x0)*(y1-y0))<<B */
  if (mp_lshd (&x1y1, B * 2) != MP_OKAY)
    goto X1Y1;          /* x1y1 = x1y1 << 2*B */

  if (mp_add (&x0y0, &t1, &t1) != MP_OKAY)
    goto X1Y1;          /* t1 = x0y0 + t1 */
  if (mp_add (&t1, &x1y1, c) != MP_OKAY)
    goto X1Y1;          /* t1 = x0y0 + t1 + x1y1 */

  /* Algorithm succeeded set the return code to MP_OKAY */
  err = MP_OKAY;

X1Y1:mp_clear (&x1y1);
X0Y0:mp_clear (&x0y0);
T1:mp_clear (&t1);
Y1:mp_clear (&y1);
Y0:mp_clear (&y0);
X1:mp_clear (&x1);
X0:mp_clear (&x0);
ERR:
  return err;
}
#endif
