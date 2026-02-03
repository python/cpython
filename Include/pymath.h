// Symbols and macros to supply platform-independent interfaces to mathematical
// functions and constants.

#ifndef Py_PYMATH_H
#define Py_PYMATH_H

/* High precision definition of pi and e (Euler)
 * The values are taken from libc6's math.h.
 */
// Deprecated since Python 3.15.
#ifndef Py_MATH_PIl
#define Py_MATH_PIl 3.1415926535897932384626433832795029L
#endif
#ifndef Py_MATH_PI
#define Py_MATH_PI 3.14159265358979323846
#endif

// Deprecated since Python 3.15.
#ifndef Py_MATH_El
#define Py_MATH_El 2.7182818284590452353602874713526625L
#endif

#ifndef Py_MATH_E
#define Py_MATH_E 2.7182818284590452354
#endif

/* Tau (2pi) to 40 digits, taken from tauday.com/tau-digits. */
#ifndef Py_MATH_TAU
#define Py_MATH_TAU 6.2831853071795864769252867665590057683943L
#endif

// Py_IS_NAN(X)
// Return 1 if float or double arg is a NaN, else 0.
// Soft deprecated since Python 3.14, use isnan() instead.
#define Py_IS_NAN(X) isnan(X)

// Py_IS_INFINITY(X)
// Return 1 if float or double arg is an infinity, else 0.
// Soft deprecated since Python 3.14, use isinf() instead.
#define Py_IS_INFINITY(X) isinf(X)

// Py_IS_FINITE(X)
// Return 1 if float or double arg is neither infinite nor NAN, else 0.
// Soft deprecated since Python 3.14, use isfinite() instead.
#define Py_IS_FINITE(X) isfinite(X)

// Py_INFINITY: Value that evaluates to a positive double infinity.
// Soft deprecated since Python 3.15, use INFINITY instead.
#ifndef Py_INFINITY
#  define Py_INFINITY ((double)INFINITY)
#endif

/* Py_HUGE_VAL should always be the same as Py_INFINITY.  But historically
 * this was not reliable and Python did not require IEEE floats and C99
 * conformity.  The macro was soft deprecated in Python 3.14, use INFINITY instead.
 */
#ifndef Py_HUGE_VAL
#  define Py_HUGE_VAL HUGE_VAL
#endif

/* Py_NAN: Value that evaluates to a quiet Not-a-Number (NaN).  The sign is
 * undefined and normally not relevant, but e.g. fixed for float("nan").
 *
 * Note: On Solaris, NAN is a function address, hence arithmetic is impossible.
 * For that reason, we instead use the built-in call if available or fallback
 * to a generic NaN computed from strtod() as a last resort.
 *
 * See https://github.com/python/cpython/issues/136006 for details.
 */
#if !defined(Py_NAN)
#  if defined(__sun)
#    if _Py__has_builtin(__builtin_nanf)
#       define Py_NAN   ((double)__builtin_nanf(""))
#    else
#       include <stdlib.h>
#       define Py_NAN   (strtod("NAN", NULL))
#    endif
#  else
#    define Py_NAN      ((double)NAN)
#  endif
#endif

#endif /* Py_PYMATH_H */
