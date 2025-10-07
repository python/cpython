// Symbols and macros to supply platform-independent interfaces to mathematical
// functions and constants.

#ifndef Py_PYMATH_H
#define Py_PYMATH_H

/* High precision definition of pi and e (Euler)
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

/* Tau (2pi) to 40 digits, taken from tauday.com/tau-digits. */
#ifndef Py_MATH_TAU
#define Py_MATH_TAU 6.2831853071795864769252867665590057683943L
#endif

// Py_IS_NAN(X)
// Return 1 if float or double arg is a NaN, else 0.
#define Py_IS_NAN(X) isnan(X)

// Py_IS_INFINITY(X)
// Return 1 if float or double arg is an infinity, else 0.
#define Py_IS_INFINITY(X) isinf(X)

// Py_IS_FINITE(X)
// Return 1 if float or double arg is neither infinite nor NAN, else 0.
#define Py_IS_FINITE(X) isfinite(X)

// Py_INFINITY: Value that evaluates to a positive double infinity.
#ifndef Py_INFINITY
#  define Py_INFINITY ((double)INFINITY)
#endif

/* Py_HUGE_VAL should always be the same as Py_INFINITY.  But historically
 * this was not reliable and Python did not require IEEE floats and C99
 * conformity.  Prefer Py_INFINITY for new code.
 */
#ifndef Py_HUGE_VAL
#  define Py_HUGE_VAL HUGE_VAL
#endif

/* Py_NAN: Value that evaluates to a quiet Not-a-Number (NaN).  The sign is
 * undefined and normally not relevant, but e.g. fixed for float("nan").
 */
#if !defined(Py_NAN)
#    define Py_NAN ((double)NAN)
#endif

#endif /* Py_PYMATH_H */
