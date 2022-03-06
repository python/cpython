// Symbols and macros to supply platform-independent interfaces to mathematical
// functions and constants.

#ifndef Py_PYMATH_H
#define Py_PYMATH_H

#include "pyconfig.h"             // HAVE_DECL_ISNAN

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


/* On x86, Py_FORCE_DOUBLE forces a floating-point number out of an x87 FPU
   register and into a 64-bit memory location, rounding from extended
   precision to double precision in the process.  On other platforms it does
   nothing. */

/* we take double rounding as evidence of x87 usage */
#ifndef Py_LIMITED_API
#ifndef Py_FORCE_DOUBLE
#  ifdef X87_DOUBLE_ROUNDING
PyAPI_FUNC(double) _Py_force_double(double);
#    define Py_FORCE_DOUBLE(X) (_Py_force_double(X))
#  else
#    define Py_FORCE_DOUBLE(X) (X)
#  endif
#endif
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
#  if defined HAVE_DECL_ISNAN && HAVE_DECL_ISNAN == 1
#    define Py_IS_NAN(X) isnan(X)
#  else
#    define Py_IS_NAN(X) ((X) != (X))
#  endif
#endif

/* Py_IS_INFINITY(X)
 * Return 1 if float or double arg is an infinity, else 0.
 * Caution:
 *    X is evaluated more than once.
 *    This implementation may set the underflow flag if |X| is very small;
 *    it really can't be implemented correctly (& easily) before C99.
 *    Override in pyconfig.h if you have a better spelling on your platform.
 *  Py_FORCE_DOUBLE is used to avoid getting false negatives from a
 *    non-infinite value v sitting in an 80-bit x87 register such that
 *    v becomes infinite when spilled from the register to 64-bit memory.
 * Note: PC/pyconfig.h defines Py_IS_INFINITY as _isinf
 */
#ifndef Py_IS_INFINITY
#  if defined HAVE_DECL_ISINF && HAVE_DECL_ISINF == 1
#    define Py_IS_INFINITY(X) isinf(X)
#  else
#    define Py_IS_INFINITY(X) ((X) &&                                   \
                               (Py_FORCE_DOUBLE(X)*0.5 == Py_FORCE_DOUBLE(X)))
#  endif
#endif

/* Py_IS_FINITE(X)
 * Return 1 if float or double arg is neither infinite nor NAN, else 0.
 * Some compilers (e.g. VisualStudio) have intrinsics for this, so a special
 * macro for this particular test is useful
 * Note: PC/pyconfig.h defines Py_IS_FINITE as _finite
 */
#ifndef Py_IS_FINITE
#  if defined HAVE_DECL_ISFINITE && HAVE_DECL_ISFINITE == 1
#    define Py_IS_FINITE(X) isfinite(X)
#  elif defined HAVE_FINITE
#    define Py_IS_FINITE(X) finite(X)
#  else
#    define Py_IS_FINITE(X) (!Py_IS_INFINITY(X) && !Py_IS_NAN(X))
#  endif
#endif

/* HUGE_VAL is supposed to expand to a positive double infinity.  Python
 * uses Py_HUGE_VAL instead because some platforms are broken in this
 * respect.  We used to embed code in pyport.h to try to worm around that,
 * but different platforms are broken in conflicting ways.  If you're on
 * a platform where HUGE_VAL is defined incorrectly, fiddle your Python
 * config to #define Py_HUGE_VAL to something that works on your platform.
 */
#ifndef Py_HUGE_VAL
#  define Py_HUGE_VAL HUGE_VAL
#endif

/* Py_NAN
 * A value that evaluates to a NaN. On IEEE 754 platforms INF*0 or
 * INF/INF works. Define Py_NO_NAN in pyconfig.h if your platform
 * doesn't support NaNs.
 */
#if !defined(Py_NAN) && !defined(Py_NO_NAN)
#  if !defined(__INTEL_COMPILER)
#    define Py_NAN (Py_HUGE_VAL * 0.)
#  else /* __INTEL_COMPILER */
#    if defined(ICC_NAN_STRICT)
        #pragma float_control(push)
        #pragma float_control(precise, on)
        #pragma float_control(except,  on)
        Py_NO_INLINE static double __icc_nan()
        {
            return sqrt(-1.0);
        }
        #pragma float_control (pop)
#       define Py_NAN __icc_nan()
#    else /* ICC_NAN_RELAXED as default for Intel Compiler */
        static const union { unsigned char buf[8]; double __icc_nan; } __nan_store = {0,0,0,0,0,0,0xf8,0x7f};
#       define Py_NAN (__nan_store.__icc_nan)
#    endif /* ICC_NAN_STRICT */
#  endif /* __INTEL_COMPILER */
#endif

#endif /* Py_PYMATH_H */
