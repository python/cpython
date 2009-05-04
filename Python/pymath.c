#include "Python.h"

#ifdef X87_DOUBLE_ROUNDING
/* On x86 platforms using an x87 FPU, this function is called from the
   Py_FORCE_DOUBLE macro (defined in pymath.h) to force a floating-point
   number out of an 80-bit x87 FPU register and into a 64-bit memory location,
   thus rounding from extended precision to double precision. */
double _Py_force_double(double x)
{
	volatile double y;
	y = x;
	return y;
}
#endif

#ifndef HAVE_HYPOT
double hypot(double x, double y)
{
	double yx;

	x = fabs(x);
	y = fabs(y);
	if (x < y) {
		double temp = x;
		x = y;
		y = temp;
	}
	if (x == 0.)
		return 0.;
	else {
		yx = y/x;
		return x*sqrt(1.+yx*yx);
	}
}
#endif /* HAVE_HYPOT */

#ifndef HAVE_COPYSIGN
double
copysign(double x, double y)
{
	/* use atan2 to distinguish -0. from 0. */
	if (y > 0. || (y == 0. && atan2(y, -1.) > 0.)) {
		return fabs(x);
	} else {
		return -fabs(x);
	}
}
#endif /* HAVE_COPYSIGN */

#ifndef HAVE_LOG1P
#include <float.h>

double
log1p(double x)
{
	/* For x small, we use the following approach.  Let y be the nearest
	   float to 1+x, then

	     1+x = y * (1 - (y-1-x)/y)

	   so log(1+x) = log(y) + log(1-(y-1-x)/y).  Since (y-1-x)/y is tiny,
	   the second term is well approximated by (y-1-x)/y.  If abs(x) >=
	   DBL_EPSILON/2 or the rounding-mode is some form of round-to-nearest
	   then y-1-x will be exactly representable, and is computed exactly
	   by (y-1)-x.

	   If abs(x) < DBL_EPSILON/2 and the rounding mode is not known to be
	   round-to-nearest then this method is slightly dangerous: 1+x could
	   be rounded up to 1+DBL_EPSILON instead of down to 1, and in that
	   case y-1-x will not be exactly representable any more and the
	   result can be off by many ulps.  But this is easily fixed: for a
	   floating-point number |x| < DBL_EPSILON/2., the closest
	   floating-point number to log(1+x) is exactly x.
	*/

	double y;
	if (fabs(x) < DBL_EPSILON/2.) {
		return x;
	} else if (-0.5 <= x && x <= 1.) {
		/* WARNING: it's possible than an overeager compiler
		   will incorrectly optimize the following two lines
		   to the equivalent of "return log(1.+x)". If this
		   happens, then results from log1p will be inaccurate
		   for small x. */
		y = 1.+x;
		return log(y)-((y-1.)-x)/y;
	} else {
		/* NaNs and infinities should end up here */
		return log(1.+x);
	}
}
#endif /* HAVE_LOG1P */

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

static const double ln2 = 6.93147180559945286227E-01;
static const double two_pow_m28 = 3.7252902984619141E-09; /* 2**-28 */
static const double two_pow_p28 = 268435456.0; /* 2**28 */
static const double zero = 0.0;

/* asinh(x)
 * Method :
 *	Based on 
 *		asinh(x) = sign(x) * log [ |x| + sqrt(x*x+1) ]
 *	we have
 *	asinh(x) := x  if  1+x*x=1,
 *		 := sign(x)*(log(x)+ln2)) for large |x|, else
 *		 := sign(x)*log(2|x|+1/(|x|+sqrt(x*x+1))) if|x|>2, else
 *		 := sign(x)*log1p(|x| + x^2/(1 + sqrt(1+x^2)))  
 */

#ifndef HAVE_ASINH
double
asinh(double x)
{	
	double w;
	double absx = fabs(x);

	if (Py_IS_NAN(x) || Py_IS_INFINITY(x)) {
		return x+x;
	}
	if (absx < two_pow_m28) {	/* |x| < 2**-28 */
		return x;	/* return x inexact except 0 */
	} 
	if (absx > two_pow_p28) {	/* |x| > 2**28 */
		w = log(absx)+ln2;
	}
	else if (absx > 2.0) {		/* 2 < |x| < 2**28 */
		w = log(2.0*absx + 1.0 / (sqrt(x*x + 1.0) + absx));
	}
	else {				/* 2**-28 <= |x| < 2= */
		double t = x*x;
		w = log1p(absx + t / (1.0 + sqrt(1.0 + t)));
	}
	return copysign(w, x);
	
}
#endif /* HAVE_ASINH */

/* acosh(x)
 * Method :
 *      Based on
 *	      acosh(x) = log [ x + sqrt(x*x-1) ]
 *      we have
 *	      acosh(x) := log(x)+ln2, if x is large; else
 *	      acosh(x) := log(2x-1/(sqrt(x*x-1)+x)) if x>2; else
 *	      acosh(x) := log1p(t+sqrt(2.0*t+t*t)); where t=x-1.
 *
 * Special cases:
 *      acosh(x) is NaN with signal if x<1.
 *      acosh(NaN) is NaN without signal.
 */

#ifndef HAVE_ACOSH
double
acosh(double x)
{
	if (Py_IS_NAN(x)) {
		return x+x;
	}
	if (x < 1.) {			/* x < 1;  return a signaling NaN */
		errno = EDOM;
#ifdef Py_NAN
		return Py_NAN;
#else
		return (x-x)/(x-x);
#endif
	}
	else if (x >= two_pow_p28) {	/* x > 2**28 */
		if (Py_IS_INFINITY(x)) {
			return x+x;
		} else {
			return log(x)+ln2;	/* acosh(huge)=log(2x) */
		}
	}
	else if (x == 1.) {
		return 0.0;			/* acosh(1) = 0 */
	}
	else if (x > 2.) {			/* 2 < x < 2**28 */
		double t = x*x;
		return log(2.0*x - 1.0 / (x + sqrt(t - 1.0)));
	}
	else {				/* 1 < x <= 2 */
		double t = x - 1.0;
		return log1p(t + sqrt(2.0*t + t*t));
	}
}
#endif /* HAVE_ACOSH */

/* atanh(x)
 * Method :
 *    1.Reduced x to positive by atanh(-x) = -atanh(x)
 *    2.For x>=0.5
 *		  1	      2x			  x
 *      atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
 *		  2	     1 - x		      1 - x
 *
 *      For x<0.5
 *      atanh(x) = 0.5*log1p(2x+2x*x/(1-x))
 *
 * Special cases:
 *      atanh(x) is NaN if |x| >= 1 with signal;
 *      atanh(NaN) is that NaN with no signal;
 *
 */

#ifndef HAVE_ATANH
double
atanh(double x)
{
	double absx;
	double t;

	if (Py_IS_NAN(x)) {
		return x+x;
	}
	absx = fabs(x);
	if (absx >= 1.) {		/* |x| >= 1 */
		errno = EDOM;
#ifdef Py_NAN
		return Py_NAN;
#else
		return x/zero;
#endif
	}
	if (absx < two_pow_m28) {	/* |x| < 2**-28 */
		return x;
	}
	if (absx < 0.5) {		/* |x| < 0.5 */
		t = absx+absx;
		t = 0.5 * log1p(t + t*absx / (1.0 - absx));
	} 
	else {				/* 0.5 <= |x| <= 1.0 */
		t = 0.5 * log1p((absx + absx) / (1.0 - absx));
	}
	return copysign(t, x);
}
#endif /* HAVE_ATANH */
