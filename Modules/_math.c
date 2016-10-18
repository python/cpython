/* Definitions of some C99 math library functions, for those platforms
   that don't implement these functions already. */

#include "Python.h"
#include <float.h>
#include "_math.h"

/* The following copyright notice applies to the original
   implementations of acosh, asinh and atanh. */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#if !defined(HAVE_ACOSH) || !defined(HAVE_ASINH)
static const double ln2 = 6.93147180559945286227E-01;
static const double two_pow_p28 = 268435456.0; /* 2**28 */
#endif
#if !defined(HAVE_ASINH) || !defined(HAVE_ATANH)
static const double two_pow_m28 = 3.7252902984619141E-09; /* 2**-28 */
#endif
#if !defined(HAVE_ATANH) && !defined(Py_NAN)
static const double zero = 0.0;
#endif


#ifndef HAVE_ACOSH
/* acosh(x)
 * Method :
 *      Based on
 *            acosh(x) = log [ x + sqrt(x*x-1) ]
 *      we have
 *            acosh(x) := log(x)+ln2, if x is large; else
 *            acosh(x) := log(2x-1/(sqrt(x*x-1)+x)) if x>2; else
 *            acosh(x) := log1p(t+sqrt(2.0*t+t*t)); where t=x-1.
 *
 * Special cases:
 *      acosh(x) is NaN with signal if x<1.
 *      acosh(NaN) is NaN without signal.
 */

double
_Py_acosh(double x)
{
    if (Py_IS_NAN(x)) {
        return x+x;
    }
    if (x < 1.) {                       /* x < 1;  return a signaling NaN */
        errno = EDOM;
#ifdef Py_NAN
        return Py_NAN;
#else
        return (x-x)/(x-x);
#endif
    }
    else if (x >= two_pow_p28) {        /* x > 2**28 */
        if (Py_IS_INFINITY(x)) {
            return x+x;
        }
        else {
            return log(x) + ln2;          /* acosh(huge)=log(2x) */
        }
    }
    else if (x == 1.) {
        return 0.0;                     /* acosh(1) = 0 */
    }
    else if (x > 2.) {                  /* 2 < x < 2**28 */
        double t = x * x;
        return log(2.0 * x - 1.0 / (x + sqrt(t - 1.0)));
    }
    else {                              /* 1 < x <= 2 */
        double t = x - 1.0;
        return m_log1p(t + sqrt(2.0 * t + t * t));
    }
}
#endif   /* HAVE_ACOSH */


#ifndef HAVE_ASINH
/* asinh(x)
 * Method :
 *      Based on
 *              asinh(x) = sign(x) * log [ |x| + sqrt(x*x+1) ]
 *      we have
 *      asinh(x) := x  if  1+x*x=1,
 *               := sign(x)*(log(x)+ln2)) for large |x|, else
 *               := sign(x)*log(2|x|+1/(|x|+sqrt(x*x+1))) if|x|>2, else
 *               := sign(x)*log1p(|x| + x^2/(1 + sqrt(1+x^2)))
 */

double
_Py_asinh(double x)
{
    double w;
    double absx = fabs(x);

    if (Py_IS_NAN(x) || Py_IS_INFINITY(x)) {
        return x+x;
    }
    if (absx < two_pow_m28) {           /* |x| < 2**-28 */
        return x;                       /* return x inexact except 0 */
    }
    if (absx > two_pow_p28) {           /* |x| > 2**28 */
        w = log(absx) + ln2;
    }
    else if (absx > 2.0) {              /* 2 < |x| < 2**28 */
        w = log(2.0 * absx + 1.0 / (sqrt(x * x + 1.0) + absx));
    }
    else {                              /* 2**-28 <= |x| < 2= */
        double t = x*x;
        w = m_log1p(absx + t / (1.0 + sqrt(1.0 + t)));
    }
    return copysign(w, x);

}
#endif   /* HAVE_ASINH */


#ifndef HAVE_ATANH
/* atanh(x)
 * Method :
 *    1.Reduced x to positive by atanh(-x) = -atanh(x)
 *    2.For x>=0.5
 *                  1              2x                          x
 *      atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * -------)
 *                  2             1 - x                      1 - x
 *
 *      For x<0.5
 *      atanh(x) = 0.5*log1p(2x+2x*x/(1-x))
 *
 * Special cases:
 *      atanh(x) is NaN if |x| >= 1 with signal;
 *      atanh(NaN) is that NaN with no signal;
 *
 */

double
_Py_atanh(double x)
{
    double absx;
    double t;

    if (Py_IS_NAN(x)) {
        return x+x;
    }
    absx = fabs(x);
    if (absx >= 1.) {                   /* |x| >= 1 */
        errno = EDOM;
#ifdef Py_NAN
        return Py_NAN;
#else
        return x / zero;
#endif
    }
    if (absx < two_pow_m28) {           /* |x| < 2**-28 */
        return x;
    }
    if (absx < 0.5) {                   /* |x| < 0.5 */
        t = absx+absx;
        t = 0.5 * m_log1p(t + t*absx / (1.0 - absx));
    }
    else {                              /* 0.5 <= |x| <= 1.0 */
        t = 0.5 * m_log1p((absx + absx) / (1.0 - absx));
    }
    return copysign(t, x);
}
#endif   /* HAVE_ATANH */


#ifndef HAVE_EXPM1
/* Mathematically, expm1(x) = exp(x) - 1.  The expm1 function is designed
   to avoid the significant loss of precision that arises from direct
   evaluation of the expression exp(x) - 1, for x near 0. */

double
_Py_expm1(double x)
{
    /* For abs(x) >= log(2), it's safe to evaluate exp(x) - 1 directly; this
       also works fine for infinities and nans.

       For smaller x, we can use a method due to Kahan that achieves close to
       full accuracy.
    */

    if (fabs(x) < 0.7) {
        double u;
        u = exp(x);
        if (u == 1.0)
            return x;
        else
            return (u - 1.0) * x / log(u);
    }
    else
        return exp(x) - 1.0;
}
#endif   /* HAVE_EXPM1 */


/* log1p(x) = log(1+x).  The log1p function is designed to avoid the
   significant loss of precision that arises from direct evaluation when x is
   small. */

double
_Py_log1p(double x)
{
#ifdef HAVE_LOG1P
    /* Some platforms supply a log1p function but don't respect the sign of
       zero:  log1p(-0.0) gives 0.0 instead of the correct result of -0.0.

       To save fiddling with configure tests and platform checks, we handle the
       special case of zero input directly on all platforms.
    */
    if (x == 0.0) {
        return x;
    }
    else {
        return log1p(x);
    }
#else
    /* For x small, we use the following approach.  Let y be the nearest float
       to 1+x, then

         1+x = y * (1 - (y-1-x)/y)

       so log(1+x) = log(y) + log(1-(y-1-x)/y).  Since (y-1-x)/y is tiny, the
       second term is well approximated by (y-1-x)/y.  If abs(x) >=
       DBL_EPSILON/2 or the rounding-mode is some form of round-to-nearest
       then y-1-x will be exactly representable, and is computed exactly by
       (y-1)-x.

       If abs(x) < DBL_EPSILON/2 and the rounding mode is not known to be
       round-to-nearest then this method is slightly dangerous: 1+x could be
       rounded up to 1+DBL_EPSILON instead of down to 1, and in that case
       y-1-x will not be exactly representable any more and the result can be
       off by many ulps.  But this is easily fixed: for a floating-point
       number |x| < DBL_EPSILON/2., the closest floating-point number to
       log(1+x) is exactly x.
    */

    double y;
    if (fabs(x) < DBL_EPSILON / 2.) {
        return x;
    }
    else if (-0.5 <= x && x <= 1.) {
        /* WARNING: it's possible that an overeager compiler
           will incorrectly optimize the following two lines
           to the equivalent of "return log(1.+x)". If this
           happens, then results from log1p will be inaccurate
           for small x. */
        y = 1.+x;
        return log(y) - ((y - 1.) - x) / y;
    }
    else {
        /* NaNs and infinities should end up here */
        return log(1.+x);
    }
#endif /* ifdef HAVE_LOG1P */
}

