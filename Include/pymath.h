#ifndef Py_PYMATH_H
#define Py_PYMATH_H

#include "pyconfig.h" /* include for defines */

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/**************************************************************************
Symbols and macros to supply platform-independent interfaces to mathematical
functions and constants
**************************************************************************/

/* Python provides implementations for copysign, acosh, asinh, atanh, 
 * log1p and hypot in Python/pymath.c just in case your math library doesn't
 * provide the functions.
 *
 *Note: PC/pyconfig.h defines copysign as _copysign
 */
#ifndef HAVE_COPYSIGN
extern double copysign(doube, double);
#endif

#ifndef HAVE_ACOSH
extern double acosh(double);
#endif

#ifndef HAVE_ASINH
extern double asinh(double);
#endif

#ifndef HAVE_ATANH
extern double atanh(double);
#endif

#ifndef HAVE_LOG1P
extern double log1p(double);
#endif

#ifndef HAVE_HYPOT
extern double hypot(double, double);
#endif

/* extra declarations */
#ifndef _MSC_VER
#ifndef __STDC__
extern double fmod (double, double);
extern double frexp (double, int *);
extern double ldexp (double, int);
extern double modf (double, double *);
extern double pow(double, double);
#endif /* __STDC__ */
#endif /* _MSC_VER */

#ifdef _OSF_SOURCE
/* OSF1 5.1 doesn't make these available with XOPEN_SOURCE_EXTENDED defined */
extern int finite(double);
extern double copysign(double, double);
#endif

/* High precision defintion of pi and e (Euler)
 * The values are taken from libc6's math.h.
 */
#ifndef Py_MATH_PIl
#define Py_MATH_PIl 3.1415926535897932384626433832795029L
#endif
#ifndef Py_MATH_PI
#define Py_MATH_PI 3.14159265358979323846
#endif

#ifndef Py_MATH_El
#define Py_MATH_El 2.7182818284590452353602874713526625L
#endif

#ifndef Py_MATH_E
#define Py_MATH_E 2.7182818284590452354
#endif

/* Py_IS_NAN(X)
 * Return 1 if float or double arg is a NaN, else 0.
 * Caution:
 *     X is evaluated more than once.
 *     This may not work on all platforms.  Each platform has *some*
 *     way to spell this, though -- override in pyconfig.h if you have
 *     a platform where it doesn't work.
 * Note: PC/pyconfig.h defines Py_IS_NAN as _isnan
 */
#ifndef Py_IS_NAN
#ifdef HAVE_ISNAN
#define Py_IS_NAN(X) isnan(X)
#else
#define Py_IS_NAN(X) ((X) != (X))
#endif
#endif

/* Py_IS_INFINITY(X)
 * Return 1 if float or double arg is an infinity, else 0.
 * Caution:
 *    X is evaluated more than once.
 *    This implementation may set the underflow flag if |X| is very small;
 *    it really can't be implemented correctly (& easily) before C99.
 *    Override in pyconfig.h if you have a better spelling on your platform.
 * Note: PC/pyconfig.h defines Py_IS_INFINITY as _isinf
 */
#ifndef Py_IS_INFINITY
#ifdef HAVE_ISINF
#define Py_IS_INFINITY(X) isinf(X)
#else
#define Py_IS_INFINITY(X) ((X) && (X)*0.5 == (X))
#endif
#endif

/* Py_IS_FINITE(X)
 * Return 1 if float or double arg is neither infinite nor NAN, else 0.
 * Some compilers (e.g. VisualStudio) have intrisics for this, so a special
 * macro for this particular test is useful
 * Note: PC/pyconfig.h defines Py_IS_FINITE as _finite
 */
#ifndef Py_IS_FINITE
#ifdef HAVE_FINITE
#define Py_IS_FINITE(X) finite(X)
#else
#define Py_IS_FINITE(X) (!Py_IS_INFINITY(X) && !Py_IS_NAN(X))
#endif
#endif

/* HUGE_VAL is supposed to expand to a positive double infinity.  Python
 * uses Py_HUGE_VAL instead because some platforms are broken in this
 * respect.  We used to embed code in pyport.h to try to worm around that,
 * but different platforms are broken in conflicting ways.  If you're on
 * a platform where HUGE_VAL is defined incorrectly, fiddle your Python
 * config to #define Py_HUGE_VAL to something that works on your platform.
 */
#ifndef Py_HUGE_VAL
#define Py_HUGE_VAL HUGE_VAL
#endif

/* Py_NAN
 * A value that evaluates to a NaN. On IEEE 754 platforms INF*0 or
 * INF/INF works. Define Py_NO_NAN in pyconfig.h if your platform
 * doesn't support NaNs.
 */
#if !defined(Py_NAN) && !defined(Py_NO_NAN)
#define Py_NAN (Py_HUGE_VAL * 0.)
#endif

/* Py_OVERFLOWED(X)
 * Return 1 iff a libm function overflowed.  Set errno to 0 before calling
 * a libm function, and invoke this macro after, passing the function
 * result.
 * Caution:
 *    This isn't reliable.  C99 no longer requires libm to set errno under
 *	  any exceptional condition, but does require +- HUGE_VAL return
 *	  values on overflow.  A 754 box *probably* maps HUGE_VAL to a
 *	  double infinity, and we're cool if that's so, unless the input
 *	  was an infinity and an infinity is the expected result.  A C89
 *	  system sets errno to ERANGE, so we check for that too.  We're
 *	  out of luck if a C99 754 box doesn't map HUGE_VAL to +Inf, or
 *	  if the returned result is a NaN, or if a C89 box returns HUGE_VAL
 *	  in non-overflow cases.
 *    X is evaluated more than once.
 * Some platforms have better way to spell this, so expect some #ifdef'ery.
 *
 * OpenBSD uses 'isinf()' because a compiler bug on that platform causes
 * the longer macro version to be mis-compiled. This isn't optimal, and
 * should be removed once a newer compiler is available on that platform.
 * The system that had the failure was running OpenBSD 3.2 on Intel, with
 * gcc 2.95.3.
 *
 * According to Tim's checkin, the FreeBSD systems use isinf() to work
 * around a FPE bug on that platform.
 */
#if defined(__FreeBSD__) || defined(__OpenBSD__)
#define Py_OVERFLOWED(X) isinf(X)
#else
#define Py_OVERFLOWED(X) ((X) != 0.0 && (errno == ERANGE ||    \
					 (X) == Py_HUGE_VAL || \
					 (X) == -Py_HUGE_VAL))
#endif

#endif /* Py_PYMATH_H */
