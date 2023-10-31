/* Math module -- standard C math library functions, pi and e */

/* Here are some comments from Tim Peters, extracted from the
   discussion attached to http://bugs.python.org/issue1640.  They
   describe the general aims of the math module with respect to
   special values, IEEE-754 floating-point exceptions, and Python
   exceptions.

These are the "spirit of 754" rules:

1. If the mathematical result is a real number, but of magnitude too
large to approximate by a machine float, overflow is signaled and the
result is an infinity (with the appropriate sign).

2. If the mathematical result is a real number, but of magnitude too
small to approximate by a machine float, underflow is signaled and the
result is a zero (with the appropriate sign).

3. At a singularity (a value x such that the limit of f(y) as y
approaches x exists and is an infinity), "divide by zero" is signaled
and the result is an infinity (with the appropriate sign).  This is
complicated a little by that the left-side and right-side limits may
not be the same; e.g., 1/x approaches +inf or -inf as x approaches 0
from the positive or negative directions.  In that specific case, the
sign of the zero determines the result of 1/0.

4. At a point where a function has no defined result in the extended
reals (i.e., the reals plus an infinity or two), invalid operation is
signaled and a NaN is returned.

And these are what Python has historically /tried/ to do (but not
always successfully, as platform libm behavior varies a lot):

For #1, raise OverflowError.

For #2, return a zero (with the appropriate sign if that happens by
accident ;-)).

For #3 and #4, raise ValueError.  It may have made sense to raise
Python's ZeroDivisionError in #3, but historically that's only been
raised for division by zero and mod by zero.

*/

/*
   In general, on an IEEE-754 platform the aim is to follow the C99
   standard, including Annex 'F', whenever possible.  Where the
   standard recommends raising the 'divide-by-zero' or 'invalid'
   floating-point exceptions, Python should raise a ValueError.  Where
   the standard recommends raising 'overflow', Python should raise an
   OverflowError.  In all other circumstances a value should be
   returned.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_bitutils.h"      // _Py_bit_length()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_object.h"        // _PyObject_LookupSpecial()
#include "pycore_pymath.h"        // _PY_SHORT_FLOAT_REPR
/* For DBL_EPSILON in _math.h */
#include <float.h>
/* For _Py_log1p with workarounds for buggy handling of zeros. */
#include "_math.h"
#include <stdbool.h>

#include "clinic/mathmodule.c.h"

/*[clinic input]
module math
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=76bc7002685dd942]*/


typedef struct {
    PyObject *str___ceil__;
    PyObject *str___floor__;
    PyObject *str___trunc__;
} math_module_state;

static inline math_module_state*
get_math_module_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (math_module_state *)state;
}

/*
Double and triple length extended precision algorithms from:

  Accurate Sum and Dot Product
  by Takeshi Ogita, Siegfried M. Rump, and Shin’Ichi Oishi
  https://doi.org/10.1137/030601818
  https://www.tuhh.de/ti3/paper/rump/OgRuOi05.pdf

*/

typedef struct{ double hi; double lo; } DoubleLength;

static DoubleLength
dl_fast_sum(double a, double b)
{
    /* Algorithm 1.1. Compensated summation of two floating point numbers. */
    assert(fabs(a) >= fabs(b));
    double x = a + b;
    double y = (a - x) + b;
    return (DoubleLength) {x, y};
}

static DoubleLength
dl_sum(double a, double b)
{
    /* Algorithm 3.1 Error-free transformation of the sum */
    double x = a + b;
    double z = x - a;
    double y = (a - (x - z)) + (b - z);
    return (DoubleLength) {x, y};
}

#ifndef UNRELIABLE_FMA

static DoubleLength
dl_mul(double x, double y)
{
    /* Algorithm 3.5. Error-free transformation of a product */
    double z = x * y;
    double zz = fma(x, y, -z);
    return (DoubleLength) {z, zz};
}

#else

/*
   The default implementation of dl_mul() depends on the C math library
   having an accurate fma() function as required by § 7.12.13.1 of the
   C99 standard.

   The UNRELIABLE_FMA option is provided as a slower but accurate
   alternative for builds where the fma() function is found wanting.
   The speed penalty may be modest (17% slower on an Apple M1 Max),
   so don't hesitate to enable this build option.

   The algorithms are from the T. J. Dekker paper:
   A Floating-Point Technique for Extending the Available Precision
   https://csclub.uwaterloo.ca/~pbarfuss/dekker1971.pdf
*/

static DoubleLength
dl_split(double x) {
    // Dekker (5.5) and (5.6).
    double t = x * 134217729.0;  // Veltkamp constant = 2.0 ** 27 + 1
    double hi = t - (t - x);
    double lo = x - hi;
    return (DoubleLength) {hi, lo};
}

static DoubleLength
dl_mul(double x, double y)
{
    // Dekker (5.12) and mul12()
    DoubleLength xx = dl_split(x);
    DoubleLength yy = dl_split(y);
    double p = xx.hi * yy.hi;
    double q = xx.hi * yy.lo + xx.lo * yy.hi;
    double z = p + q;
    double zz = p - z + q + xx.lo * yy.lo;
    return (DoubleLength) {z, zz};
}

#endif

typedef struct { double hi; double lo; double tiny; } TripleLength;

static const TripleLength tl_zero = {0.0, 0.0, 0.0};

static TripleLength
tl_fma(double x, double y, TripleLength total)
{
    /* Algorithm 5.10 with SumKVert for K=3 */
    DoubleLength pr = dl_mul(x, y);
    DoubleLength sm = dl_sum(total.hi, pr.hi);
    DoubleLength r1 = dl_sum(total.lo, pr.lo);
    DoubleLength r2 = dl_sum(r1.hi, sm.lo);
    return (TripleLength) {sm.hi, r2.hi, total.tiny + r1.lo + r2.lo};
}

static double
tl_to_d(TripleLength total)
{
    DoubleLength last = dl_sum(total.lo, total.hi);
    return total.tiny + last.lo + last.hi;
}


/*
   sin(pi*x), giving accurate results for all finite x (especially x
   integral or close to an integer).  This is here for use in the
   reflection formula for the gamma function.  It conforms to IEEE
   754-2008 for finite arguments, but not for infinities or nans.
*/

static const double pi = 3.141592653589793238462643383279502884197;
static const double logpi = 1.144729885849400174143427351353058711647;

/* Version of PyFloat_AsDouble() with in-line fast paths
   for exact floats and integers.  Gives a substantial
   speed improvement for extracting float arguments.
*/

#define ASSIGN_DOUBLE(target_var, obj, error_label)        \
    if (PyFloat_CheckExact(obj)) {                         \
        target_var = PyFloat_AS_DOUBLE(obj);               \
    }                                                      \
    else if (PyLong_CheckExact(obj)) {                     \
        target_var = PyLong_AsDouble(obj);                 \
        if (target_var == -1.0 && PyErr_Occurred()) {      \
            goto error_label;                              \
        }                                                  \
    }                                                      \
    else {                                                 \
        target_var = PyFloat_AsDouble(obj);                \
        if (target_var == -1.0 && PyErr_Occurred()) {      \
            goto error_label;                              \
        }                                                  \
    }

static double
m_sinpi(double x)
{
    double y, r;
    int n;
    /* this function should only ever be called for finite arguments */
    assert(Py_IS_FINITE(x));
    y = fmod(fabs(x), 2.0);
    n = (int)round(2.0*y);
    assert(0 <= n && n <= 4);
    switch (n) {
    case 0:
        r = sin(pi*y);
        break;
    case 1:
        r = cos(pi*(y-0.5));
        break;
    case 2:
        /* N.B. -sin(pi*(y-1.0)) is *not* equivalent: it would give
           -0.0 instead of 0.0 when y == 1.0. */
        r = sin(pi*(1.0-y));
        break;
    case 3:
        r = -cos(pi*(y-1.5));
        break;
    case 4:
        r = sin(pi*(y-2.0));
        break;
    default:
        Py_UNREACHABLE();
    }
    return copysign(1.0, x)*r;
}

/* Implementation of the real gamma function.  Kept here to work around
   issues (see e.g. gh-70309) with quality of libm's tgamma/lgamma implementations
   on various platforms (Windows, MacOS).  In extensive but non-exhaustive
   random tests, this function proved accurate to within <= 10 ulps across the
   entire float domain.  Note that accuracy may depend on the quality of the
   system math functions, the pow function in particular.  Special cases
   follow C99 annex F.  The parameters and method are tailored to platforms
   whose double format is the IEEE 754 binary64 format.

   Method: for x > 0.0 we use the Lanczos approximation with parameters N=13
   and g=6.024680040776729583740234375; these parameters are amongst those
   used by the Boost library.  Following Boost (again), we re-express the
   Lanczos sum as a rational function, and compute it that way.  The
   coefficients below were computed independently using MPFR, and have been
   double-checked against the coefficients in the Boost source code.

   For x < 0.0 we use the reflection formula.

   There's one minor tweak that deserves explanation: Lanczos' formula for
   Gamma(x) involves computing pow(x+g-0.5, x-0.5) / exp(x+g-0.5).  For many x
   values, x+g-0.5 can be represented exactly.  However, in cases where it
   can't be represented exactly the small error in x+g-0.5 can be magnified
   significantly by the pow and exp calls, especially for large x.  A cheap
   correction is to multiply by (1 + e*g/(x+g-0.5)), where e is the error
   involved in the computation of x+g-0.5 (that is, e = computed value of
   x+g-0.5 - exact value of x+g-0.5).  Here's the proof:

   Correction factor
   -----------------
   Write x+g-0.5 = y-e, where y is exactly representable as an IEEE 754
   double, and e is tiny.  Then:

     pow(x+g-0.5,x-0.5)/exp(x+g-0.5) = pow(y-e, x-0.5)/exp(y-e)
     = pow(y, x-0.5)/exp(y) * C,

   where the correction_factor C is given by

     C = pow(1-e/y, x-0.5) * exp(e)

   Since e is tiny, pow(1-e/y, x-0.5) ~ 1-(x-0.5)*e/y, and exp(x) ~ 1+e, so:

     C ~ (1-(x-0.5)*e/y) * (1+e) ~ 1 + e*(y-(x-0.5))/y

   But y-(x-0.5) = g+e, and g+e ~ g.  So we get C ~ 1 + e*g/y, and

     pow(x+g-0.5,x-0.5)/exp(x+g-0.5) ~ pow(y, x-0.5)/exp(y) * (1 + e*g/y),

   Note that for accuracy, when computing r*C it's better to do

     r + e*g/y*r;

   than

     r * (1 + e*g/y);

   since the addition in the latter throws away most of the bits of
   information in e*g/y.
*/

#define LANCZOS_N 13
static const double lanczos_g = 6.024680040776729583740234375;
static const double lanczos_g_minus_half = 5.524680040776729583740234375;
static const double lanczos_num_coeffs[LANCZOS_N] = {
    23531376880.410759688572007674451636754734846804940,
    42919803642.649098768957899047001988850926355848959,
    35711959237.355668049440185451547166705960488635843,
    17921034426.037209699919755754458931112671403265390,
    6039542586.3520280050642916443072979210699388420708,
    1439720407.3117216736632230727949123939715485786772,
    248874557.86205415651146038641322942321632125127801,
    31426415.585400194380614231628318205362874684987640,
    2876370.6289353724412254090516208496135991145378768,
    186056.26539522349504029498971604569928220784236328,
    8071.6720023658162106380029022722506138218516325024,
    210.82427775157934587250973392071336271166969580291,
    2.5066282746310002701649081771338373386264310793408
};

/* denominator is x*(x+1)*...*(x+LANCZOS_N-2) */
static const double lanczos_den_coeffs[LANCZOS_N] = {
    0.0, 39916800.0, 120543840.0, 150917976.0, 105258076.0, 45995730.0,
    13339535.0, 2637558.0, 357423.0, 32670.0, 1925.0, 66.0, 1.0};

/* gamma values for small positive integers, 1 though NGAMMA_INTEGRAL */
#define NGAMMA_INTEGRAL 23
static const double gamma_integral[NGAMMA_INTEGRAL] = {
    1.0, 1.0, 2.0, 6.0, 24.0, 120.0, 720.0, 5040.0, 40320.0, 362880.0,
    3628800.0, 39916800.0, 479001600.0, 6227020800.0, 87178291200.0,
    1307674368000.0, 20922789888000.0, 355687428096000.0,
    6402373705728000.0, 121645100408832000.0, 2432902008176640000.0,
    51090942171709440000.0, 1124000727777607680000.0,
};

/* Lanczos' sum L_g(x), for positive x */

static double
lanczos_sum(double x)
{
    double num = 0.0, den = 0.0;
    int i;
    assert(x > 0.0);
    /* evaluate the rational function lanczos_sum(x).  For large
       x, the obvious algorithm risks overflow, so we instead
       rescale the denominator and numerator of the rational
       function by x**(1-LANCZOS_N) and treat this as a
       rational function in 1/x.  This also reduces the error for
       larger x values.  The choice of cutoff point (5.0 below) is
       somewhat arbitrary; in tests, smaller cutoff values than
       this resulted in lower accuracy. */
    if (x < 5.0) {
        for (i = LANCZOS_N; --i >= 0; ) {
            num = num * x + lanczos_num_coeffs[i];
            den = den * x + lanczos_den_coeffs[i];
        }
    }
    else {
        for (i = 0; i < LANCZOS_N; i++) {
            num = num / x + lanczos_num_coeffs[i];
            den = den / x + lanczos_den_coeffs[i];
        }
    }
    return num/den;
}


static double
m_tgamma(double x)
{
    double absx, r, y, z, sqrtpow;

    /* special cases */
    if (!Py_IS_FINITE(x)) {
        if (Py_IS_NAN(x) || x > 0.0)
            return x;  /* tgamma(nan) = nan, tgamma(inf) = inf */
        else {
            errno = EDOM;
            return Py_NAN;  /* tgamma(-inf) = nan, invalid */
        }
    }
    if (x == 0.0) {
        errno = EDOM;
        /* tgamma(+-0.0) = +-inf, divide-by-zero */
        return copysign(Py_INFINITY, x);
    }

    /* integer arguments */
    if (x == floor(x)) {
        if (x < 0.0) {
            errno = EDOM;  /* tgamma(n) = nan, invalid for */
            return Py_NAN; /* negative integers n */
        }
        if (x <= NGAMMA_INTEGRAL)
            return gamma_integral[(int)x - 1];
    }
    absx = fabs(x);

    /* tiny arguments:  tgamma(x) ~ 1/x for x near 0 */
    if (absx < 1e-20) {
        r = 1.0/x;
        if (Py_IS_INFINITY(r))
            errno = ERANGE;
        return r;
    }

    /* large arguments: assuming IEEE 754 doubles, tgamma(x) overflows for
       x > 200, and underflows to +-0.0 for x < -200, not a negative
       integer. */
    if (absx > 200.0) {
        if (x < 0.0) {
            return 0.0/m_sinpi(x);
        }
        else {
            errno = ERANGE;
            return Py_HUGE_VAL;
        }
    }

    y = absx + lanczos_g_minus_half;
    /* compute error in sum */
    if (absx > lanczos_g_minus_half) {
        /* note: the correction can be foiled by an optimizing
           compiler that (incorrectly) thinks that an expression like
           a + b - a - b can be optimized to 0.0.  This shouldn't
           happen in a standards-conforming compiler. */
        double q = y - absx;
        z = q - lanczos_g_minus_half;
    }
    else {
        double q = y - lanczos_g_minus_half;
        z = q - absx;
    }
    z = z * lanczos_g / y;
    if (x < 0.0) {
        r = -pi / m_sinpi(absx) / absx * exp(y) / lanczos_sum(absx);
        r -= z * r;
        if (absx < 140.0) {
            r /= pow(y, absx - 0.5);
        }
        else {
            sqrtpow = pow(y, absx / 2.0 - 0.25);
            r /= sqrtpow;
            r /= sqrtpow;
        }
    }
    else {
        r = lanczos_sum(absx) / exp(y);
        r += z * r;
        if (absx < 140.0) {
            r *= pow(y, absx - 0.5);
        }
        else {
            sqrtpow = pow(y, absx / 2.0 - 0.25);
            r *= sqrtpow;
            r *= sqrtpow;
        }
    }
    if (Py_IS_INFINITY(r))
        errno = ERANGE;
    return r;
}

/*
   lgamma:  natural log of the absolute value of the Gamma function.
   For large arguments, Lanczos' formula works extremely well here.
*/

static double
m_lgamma(double x)
{
    double r;
    double absx;

    /* special cases */
    if (!Py_IS_FINITE(x)) {
        if (Py_IS_NAN(x))
            return x;  /* lgamma(nan) = nan */
        else
            return Py_HUGE_VAL; /* lgamma(+-inf) = +inf */
    }

    /* integer arguments */
    if (x == floor(x) && x <= 2.0) {
        if (x <= 0.0) {
            errno = EDOM;  /* lgamma(n) = inf, divide-by-zero for */
            return Py_HUGE_VAL; /* integers n <= 0 */
        }
        else {
            return 0.0; /* lgamma(1) = lgamma(2) = 0.0 */
        }
    }

    absx = fabs(x);
    /* tiny arguments: lgamma(x) ~ -log(fabs(x)) for small x */
    if (absx < 1e-20)
        return -log(absx);

    /* Lanczos' formula.  We could save a fraction of a ulp in accuracy by
       having a second set of numerator coefficients for lanczos_sum that
       absorbed the exp(-lanczos_g) term, and throwing out the lanczos_g
       subtraction below; it's probably not worth it. */
    r = log(lanczos_sum(absx)) - lanczos_g;
    r += (absx - 0.5) * (log(absx + lanczos_g - 0.5) - 1);
    if (x < 0.0)
        /* Use reflection formula to get value for negative x. */
        r = logpi - log(fabs(m_sinpi(absx))) - log(absx) - r;
    if (Py_IS_INFINITY(r))
        errno = ERANGE;
    return r;
}

/*
   wrapper for atan2 that deals directly with special cases before
   delegating to the platform libm for the remaining cases.  This
   is necessary to get consistent behaviour across platforms.
   Windows, FreeBSD and alpha Tru64 are amongst platforms that don't
   always follow C99.
*/

static double
m_atan2(double y, double x)
{
    if (Py_IS_NAN(x) || Py_IS_NAN(y))
        return Py_NAN;
    if (Py_IS_INFINITY(y)) {
        if (Py_IS_INFINITY(x)) {
            if (copysign(1., x) == 1.)
                /* atan2(+-inf, +inf) == +-pi/4 */
                return copysign(0.25*Py_MATH_PI, y);
            else
                /* atan2(+-inf, -inf) == +-pi*3/4 */
                return copysign(0.75*Py_MATH_PI, y);
        }
        /* atan2(+-inf, x) == +-pi/2 for finite x */
        return copysign(0.5*Py_MATH_PI, y);
    }
    if (Py_IS_INFINITY(x) || y == 0.) {
        if (copysign(1., x) == 1.)
            /* atan2(+-y, +inf) = atan2(+-0, +x) = +-0. */
            return copysign(0., y);
        else
            /* atan2(+-y, -inf) = atan2(+-0., -x) = +-pi. */
            return copysign(Py_MATH_PI, y);
    }
    return atan2(y, x);
}


/* IEEE 754-style remainder operation: x - n*y where n*y is the nearest
   multiple of y to x, taking n even in the case of a tie. Assuming an IEEE 754
   binary floating-point format, the result is always exact. */

static double
m_remainder(double x, double y)
{
    /* Deal with most common case first. */
    if (Py_IS_FINITE(x) && Py_IS_FINITE(y)) {
        double absx, absy, c, m, r;

        if (y == 0.0) {
            return Py_NAN;
        }

        absx = fabs(x);
        absy = fabs(y);
        m = fmod(absx, absy);

        /*
           Warning: some subtlety here. What we *want* to know at this point is
           whether the remainder m is less than, equal to, or greater than half
           of absy. However, we can't do that comparison directly because we
           can't be sure that 0.5*absy is representable (the multiplication
           might incur precision loss due to underflow). So instead we compare
           m with the complement c = absy - m: m < 0.5*absy if and only if m <
           c, and so on. The catch is that absy - m might also not be
           representable, but it turns out that it doesn't matter:

           - if m > 0.5*absy then absy - m is exactly representable, by
             Sterbenz's lemma, so m > c
           - if m == 0.5*absy then again absy - m is exactly representable
             and m == c
           - if m < 0.5*absy then either (i) 0.5*absy is exactly representable,
             in which case 0.5*absy < absy - m, so 0.5*absy <= c and hence m <
             c, or (ii) absy is tiny, either subnormal or in the lowest normal
             binade. Then absy - m is exactly representable and again m < c.
        */

        c = absy - m;
        if (m < c) {
            r = m;
        }
        else if (m > c) {
            r = -c;
        }
        else {
            /*
               Here absx is exactly halfway between two multiples of absy,
               and we need to choose the even multiple. x now has the form

                   absx = n * absy + m

               for some integer n (recalling that m = 0.5*absy at this point).
               If n is even we want to return m; if n is odd, we need to
               return -m.

               So

                   0.5 * (absx - m) = (n/2) * absy

               and now reducing modulo absy gives us:

                                                  | m, if n is odd
                   fmod(0.5 * (absx - m), absy) = |
                                                  | 0, if n is even

               Now m - 2.0 * fmod(...) gives the desired result: m
               if n is even, -m if m is odd.

               Note that all steps in fmod(0.5 * (absx - m), absy)
               will be computed exactly, with no rounding error
               introduced.
            */
            assert(m == c);
            r = m - 2.0 * fmod(0.5 * (absx - m), absy);
        }
        return copysign(1.0, x) * r;
    }

    /* Special values. */
    if (Py_IS_NAN(x)) {
        return x;
    }
    if (Py_IS_NAN(y)) {
        return y;
    }
    if (Py_IS_INFINITY(x)) {
        return Py_NAN;
    }
    assert(Py_IS_INFINITY(y));
    return x;
}


/*
    Various platforms (Solaris, OpenBSD) do nonstandard things for log(0),
    log(-ve), log(NaN).  Here are wrappers for log and log10 that deal with
    special values directly, passing positive non-special values through to
    the system log/log10.
 */

static double
m_log(double x)
{
    if (Py_IS_FINITE(x)) {
        if (x > 0.0)
            return log(x);
        errno = EDOM;
        if (x == 0.0)
            return -Py_HUGE_VAL; /* log(0) = -inf */
        else
            return Py_NAN; /* log(-ve) = nan */
    }
    else if (Py_IS_NAN(x))
        return x; /* log(nan) = nan */
    else if (x > 0.0)
        return x; /* log(inf) = inf */
    else {
        errno = EDOM;
        return Py_NAN; /* log(-inf) = nan */
    }
}

/*
   log2: log to base 2.

   Uses an algorithm that should:

     (a) produce exact results for powers of 2, and
     (b) give a monotonic log2 (for positive finite floats),
         assuming that the system log is monotonic.
*/

static double
m_log2(double x)
{
    if (!Py_IS_FINITE(x)) {
        if (Py_IS_NAN(x))
            return x; /* log2(nan) = nan */
        else if (x > 0.0)
            return x; /* log2(+inf) = +inf */
        else {
            errno = EDOM;
            return Py_NAN; /* log2(-inf) = nan, invalid-operation */
        }
    }

    if (x > 0.0) {
        return log2(x);
    }
    else if (x == 0.0) {
        errno = EDOM;
        return -Py_HUGE_VAL; /* log2(0) = -inf, divide-by-zero */
    }
    else {
        errno = EDOM;
        return Py_NAN; /* log2(-inf) = nan, invalid-operation */
    }
}

static double
m_log10(double x)
{
    if (Py_IS_FINITE(x)) {
        if (x > 0.0)
            return log10(x);
        errno = EDOM;
        if (x == 0.0)
            return -Py_HUGE_VAL; /* log10(0) = -inf */
        else
            return Py_NAN; /* log10(-ve) = nan */
    }
    else if (Py_IS_NAN(x))
        return x; /* log10(nan) = nan */
    else if (x > 0.0)
        return x; /* log10(inf) = inf */
    else {
        errno = EDOM;
        return Py_NAN; /* log10(-inf) = nan */
    }
}


static PyObject *
math_gcd(PyObject *module, PyObject * const *args, Py_ssize_t nargs)
{
    PyObject *res, *x;
    Py_ssize_t i;

    if (nargs == 0) {
        return PyLong_FromLong(0);
    }
    res = PyNumber_Index(args[0]);
    if (res == NULL) {
        return NULL;
    }
    if (nargs == 1) {
        Py_SETREF(res, PyNumber_Absolute(res));
        return res;
    }

    PyObject *one = _PyLong_GetOne();  // borrowed ref
    for (i = 1; i < nargs; i++) {
        x = _PyNumber_Index(args[i]);
        if (x == NULL) {
            Py_DECREF(res);
            return NULL;
        }
        if (res == one) {
            /* Fast path: just check arguments.
               It is okay to use identity comparison here. */
            Py_DECREF(x);
            continue;
        }
        Py_SETREF(res, _PyLong_GCD(res, x));
        Py_DECREF(x);
        if (res == NULL) {
            return NULL;
        }
    }
    return res;
}

PyDoc_STRVAR(math_gcd_doc,
"gcd($module, *integers)\n"
"--\n"
"\n"
"Greatest Common Divisor.");


static PyObject *
long_lcm(PyObject *a, PyObject *b)
{
    PyObject *g, *m, *f, *ab;

    if (_PyLong_IsZero((PyLongObject *)a) || _PyLong_IsZero((PyLongObject *)b)) {
        return PyLong_FromLong(0);
    }
    g = _PyLong_GCD(a, b);
    if (g == NULL) {
        return NULL;
    }
    f = PyNumber_FloorDivide(a, g);
    Py_DECREF(g);
    if (f == NULL) {
        return NULL;
    }
    m = PyNumber_Multiply(f, b);
    Py_DECREF(f);
    if (m == NULL) {
        return NULL;
    }
    ab = PyNumber_Absolute(m);
    Py_DECREF(m);
    return ab;
}


static PyObject *
math_lcm(PyObject *module, PyObject * const *args, Py_ssize_t nargs)
{
    PyObject *res, *x;
    Py_ssize_t i;

    if (nargs == 0) {
        return PyLong_FromLong(1);
    }
    res = PyNumber_Index(args[0]);
    if (res == NULL) {
        return NULL;
    }
    if (nargs == 1) {
        Py_SETREF(res, PyNumber_Absolute(res));
        return res;
    }

    PyObject *zero = _PyLong_GetZero();  // borrowed ref
    for (i = 1; i < nargs; i++) {
        x = PyNumber_Index(args[i]);
        if (x == NULL) {
            Py_DECREF(res);
            return NULL;
        }
        if (res == zero) {
            /* Fast path: just check arguments.
               It is okay to use identity comparison here. */
            Py_DECREF(x);
            continue;
        }
        Py_SETREF(res, long_lcm(res, x));
        Py_DECREF(x);
        if (res == NULL) {
            return NULL;
        }
    }
    return res;
}


PyDoc_STRVAR(math_lcm_doc,
"lcm($module, *integers)\n"
"--\n"
"\n"
"Least Common Multiple.");


/* Call is_error when errno != 0, and where x is the result libm
 * returned.  is_error will usually set up an exception and return
 * true (1), but may return false (0) without setting up an exception.
 */
static int
is_error(double x)
{
    int result = 1;     /* presumption of guilt */
    assert(errno);      /* non-zero errno is a precondition for calling */
    if (errno == EDOM)
        PyErr_SetString(PyExc_ValueError, "math domain error");

    else if (errno == ERANGE) {
        /* ANSI C generally requires libm functions to set ERANGE
         * on overflow, but also generally *allows* them to set
         * ERANGE on underflow too.  There's no consistency about
         * the latter across platforms.
         * Alas, C99 never requires that errno be set.
         * Here we suppress the underflow errors (libm functions
         * should return a zero on underflow, and +- HUGE_VAL on
         * overflow, so testing the result for zero suffices to
         * distinguish the cases).
         *
         * On some platforms (Ubuntu/ia64) it seems that errno can be
         * set to ERANGE for subnormal results that do *not* underflow
         * to zero.  So to be safe, we'll ignore ERANGE whenever the
         * function result is less than 1.5 in absolute value.
         *
         * bpo-46018: Changed to 1.5 to ensure underflows in expm1()
         * are correctly detected, since the function may underflow
         * toward -1.0 rather than 0.0.
         */
        if (fabs(x) < 1.5)
            result = 0;
        else
            PyErr_SetString(PyExc_OverflowError,
                            "math range error");
    }
    else
        /* Unexpected math error */
        PyErr_SetFromErrno(PyExc_ValueError);
    return result;
}

/*
   math_1 is used to wrap a libm function f that takes a double
   argument and returns a double.

   The error reporting follows these rules, which are designed to do
   the right thing on C89/C99 platforms and IEEE 754/non IEEE 754
   platforms.

   - a NaN result from non-NaN inputs causes ValueError to be raised
   - an infinite result from finite inputs causes OverflowError to be
     raised if can_overflow is 1, or raises ValueError if can_overflow
     is 0.
   - if the result is finite and errno == EDOM then ValueError is
     raised
   - if the result is finite and nonzero and errno == ERANGE then
     OverflowError is raised

   The last rule is used to catch overflow on platforms which follow
   C89 but for which HUGE_VAL is not an infinity.

   For the majority of one-argument functions these rules are enough
   to ensure that Python's functions behave as specified in 'Annex F'
   of the C99 standard, with the 'invalid' and 'divide-by-zero'
   floating-point exceptions mapping to Python's ValueError and the
   'overflow' floating-point exception mapping to OverflowError.
   math_1 only works for functions that don't have singularities *and*
   the possibility of overflow; fortunately, that covers everything we
   care about right now.
*/

static PyObject *
math_1(PyObject *arg, double (*func) (double), int can_overflow)
{
    double x, r;
    x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    errno = 0;
    r = (*func)(x);
    if (Py_IS_NAN(r) && !Py_IS_NAN(x)) {
        PyErr_SetString(PyExc_ValueError,
                        "math domain error"); /* invalid arg */
        return NULL;
    }
    if (Py_IS_INFINITY(r) && Py_IS_FINITE(x)) {
        if (can_overflow)
            PyErr_SetString(PyExc_OverflowError,
                            "math range error"); /* overflow */
        else
            PyErr_SetString(PyExc_ValueError,
                            "math domain error"); /* singularity */
        return NULL;
    }
    if (Py_IS_FINITE(r) && errno && is_error(r))
        /* this branch unnecessary on most platforms */
        return NULL;

    return PyFloat_FromDouble(r);
}

/* variant of math_1, to be used when the function being wrapped is known to
   set errno properly (that is, errno = EDOM for invalid or divide-by-zero,
   errno = ERANGE for overflow). */

static PyObject *
math_1a(PyObject *arg, double (*func) (double))
{
    double x, r;
    x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    errno = 0;
    r = (*func)(x);
    if (errno && is_error(r))
        return NULL;
    return PyFloat_FromDouble(r);
}

/*
   math_2 is used to wrap a libm function f that takes two double
   arguments and returns a double.

   The error reporting follows these rules, which are designed to do
   the right thing on C89/C99 platforms and IEEE 754/non IEEE 754
   platforms.

   - a NaN result from non-NaN inputs causes ValueError to be raised
   - an infinite result from finite inputs causes OverflowError to be
     raised.
   - if the result is finite and errno == EDOM then ValueError is
     raised
   - if the result is finite and nonzero and errno == ERANGE then
     OverflowError is raised

   The last rule is used to catch overflow on platforms which follow
   C89 but for which HUGE_VAL is not an infinity.

   For most two-argument functions (copysign, fmod, hypot, atan2)
   these rules are enough to ensure that Python's functions behave as
   specified in 'Annex F' of the C99 standard, with the 'invalid' and
   'divide-by-zero' floating-point exceptions mapping to Python's
   ValueError and the 'overflow' floating-point exception mapping to
   OverflowError.
*/

static PyObject *
math_2(PyObject *const *args, Py_ssize_t nargs,
       double (*func) (double, double), const char *funcname)
{
    double x, y, r;
    if (!_PyArg_CheckPositional(funcname, nargs, 2, 2))
        return NULL;
    x = PyFloat_AsDouble(args[0]);
    if (x == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    y = PyFloat_AsDouble(args[1]);
    if (y == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    errno = 0;
    r = (*func)(x, y);
    if (Py_IS_NAN(r)) {
        if (!Py_IS_NAN(x) && !Py_IS_NAN(y))
            errno = EDOM;
        else
            errno = 0;
    }
    else if (Py_IS_INFINITY(r)) {
        if (Py_IS_FINITE(x) && Py_IS_FINITE(y))
            errno = ERANGE;
        else
            errno = 0;
    }
    if (errno && is_error(r))
        return NULL;
    else
        return PyFloat_FromDouble(r);
}

#define FUNC1(funcname, func, can_overflow, docstring)                  \
    static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
        return math_1(args, func, can_overflow);                            \
    }\
    PyDoc_STRVAR(math_##funcname##_doc, docstring);

#define FUNC1A(funcname, func, docstring)                               \
    static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
        return math_1a(args, func);                                     \
    }\
    PyDoc_STRVAR(math_##funcname##_doc, docstring);

#define FUNC2(funcname, func, docstring) \
    static PyObject * math_##funcname(PyObject *self, PyObject *const *args, Py_ssize_t nargs) { \
        return math_2(args, nargs, func, #funcname); \
    }\
    PyDoc_STRVAR(math_##funcname##_doc, docstring);

FUNC1(acos, acos, 0,
      "acos($module, x, /)\n--\n\n"
      "Return the arc cosine (measured in radians) of x.\n\n"
      "The result is between 0 and pi.")
FUNC1(acosh, acosh, 0,
      "acosh($module, x, /)\n--\n\n"
      "Return the inverse hyperbolic cosine of x.")
FUNC1(asin, asin, 0,
      "asin($module, x, /)\n--\n\n"
      "Return the arc sine (measured in radians) of x.\n\n"
      "The result is between -pi/2 and pi/2.")
FUNC1(asinh, asinh, 0,
      "asinh($module, x, /)\n--\n\n"
      "Return the inverse hyperbolic sine of x.")
FUNC1(atan, atan, 0,
      "atan($module, x, /)\n--\n\n"
      "Return the arc tangent (measured in radians) of x.\n\n"
      "The result is between -pi/2 and pi/2.")
FUNC2(atan2, m_atan2,
      "atan2($module, y, x, /)\n--\n\n"
      "Return the arc tangent (measured in radians) of y/x.\n\n"
      "Unlike atan(y/x), the signs of both x and y are considered.")
FUNC1(atanh, atanh, 0,
      "atanh($module, x, /)\n--\n\n"
      "Return the inverse hyperbolic tangent of x.")
FUNC1(cbrt, cbrt, 0,
      "cbrt($module, x, /)\n--\n\n"
      "Return the cube root of x.")

/*[clinic input]
math.ceil

    x as number: object
    /

Return the ceiling of x as an Integral.

This is the smallest integer >= x.
[clinic start generated code]*/

static PyObject *
math_ceil(PyObject *module, PyObject *number)
/*[clinic end generated code: output=6c3b8a78bc201c67 input=2725352806399cab]*/
{
    double x;

    if (PyFloat_CheckExact(number)) {
        x = PyFloat_AS_DOUBLE(number);
    }
    else {
        math_module_state *state = get_math_module_state(module);
        PyObject *method = _PyObject_LookupSpecial(number, state->str___ceil__);
        if (method != NULL) {
            PyObject *result = _PyObject_CallNoArgs(method);
            Py_DECREF(method);
            return result;
        }
        if (PyErr_Occurred())
            return NULL;
        x = PyFloat_AsDouble(number);
        if (x == -1.0 && PyErr_Occurred())
            return NULL;
    }
    return PyLong_FromDouble(ceil(x));
}

FUNC2(copysign, copysign,
      "copysign($module, x, y, /)\n--\n\n"
       "Return a float with the magnitude (absolute value) of x but the sign of y.\n\n"
      "On platforms that support signed zeros, copysign(1.0, -0.0)\n"
      "returns -1.0.\n")
FUNC1(cos, cos, 0,
      "cos($module, x, /)\n--\n\n"
      "Return the cosine of x (measured in radians).")
FUNC1(cosh, cosh, 1,
      "cosh($module, x, /)\n--\n\n"
      "Return the hyperbolic cosine of x.")
FUNC1A(erf, erf,
       "erf($module, x, /)\n--\n\n"
       "Error function at x.")
FUNC1A(erfc, erfc,
       "erfc($module, x, /)\n--\n\n"
       "Complementary error function at x.")
FUNC1(exp, exp, 1,
      "exp($module, x, /)\n--\n\n"
      "Return e raised to the power of x.")
FUNC1(exp2, exp2, 1,
      "exp2($module, x, /)\n--\n\n"
      "Return 2 raised to the power of x.")
FUNC1(expm1, expm1, 1,
      "expm1($module, x, /)\n--\n\n"
      "Return exp(x)-1.\n\n"
      "This function avoids the loss of precision involved in the direct "
      "evaluation of exp(x)-1 for small x.")
FUNC1(fabs, fabs, 0,
      "fabs($module, x, /)\n--\n\n"
      "Return the absolute value of the float x.")

/*[clinic input]
math.floor

    x as number: object
    /

Return the floor of x as an Integral.

This is the largest integer <= x.
[clinic start generated code]*/

static PyObject *
math_floor(PyObject *module, PyObject *number)
/*[clinic end generated code: output=c6a65c4884884b8a input=63af6b5d7ebcc3d6]*/
{
    double x;

    if (PyFloat_CheckExact(number)) {
        x = PyFloat_AS_DOUBLE(number);
    }
    else {
        math_module_state *state = get_math_module_state(module);
        PyObject *method = _PyObject_LookupSpecial(number, state->str___floor__);
        if (method != NULL) {
            PyObject *result = _PyObject_CallNoArgs(method);
            Py_DECREF(method);
            return result;
        }
        if (PyErr_Occurred())
            return NULL;
        x = PyFloat_AsDouble(number);
        if (x == -1.0 && PyErr_Occurred())
            return NULL;
    }
    return PyLong_FromDouble(floor(x));
}

FUNC1A(gamma, m_tgamma,
      "gamma($module, x, /)\n--\n\n"
      "Gamma function at x.")
FUNC1A(lgamma, m_lgamma,
      "lgamma($module, x, /)\n--\n\n"
      "Natural logarithm of absolute value of Gamma function at x.")
FUNC1(log1p, m_log1p, 0,
      "log1p($module, x, /)\n--\n\n"
      "Return the natural logarithm of 1+x (base e).\n\n"
      "The result is computed in a way which is accurate for x near zero.")
FUNC2(remainder, m_remainder,
      "remainder($module, x, y, /)\n--\n\n"
      "Difference between x and the closest integer multiple of y.\n\n"
      "Return x - n*y where n*y is the closest integer multiple of y.\n"
      "In the case where x is exactly halfway between two multiples of\n"
      "y, the nearest even value of n is used. The result is always exact.")
FUNC1(sin, sin, 0,
      "sin($module, x, /)\n--\n\n"
      "Return the sine of x (measured in radians).")
FUNC1(sinh, sinh, 1,
      "sinh($module, x, /)\n--\n\n"
      "Return the hyperbolic sine of x.")
FUNC1(sqrt, sqrt, 0,
      "sqrt($module, x, /)\n--\n\n"
      "Return the square root of x.")
FUNC1(tan, tan, 0,
      "tan($module, x, /)\n--\n\n"
      "Return the tangent of x (measured in radians).")
FUNC1(tanh, tanh, 0,
      "tanh($module, x, /)\n--\n\n"
      "Return the hyperbolic tangent of x.")

/* Precision summation function as msum() by Raymond Hettinger in
   <http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/393090>,
   enhanced with the exact partials sum and roundoff from Mark
   Dickinson's post at <http://bugs.python.org/file10357/msum4.py>.
   See those links for more details, proofs and other references.

   Note 1: IEEE 754 floating-point semantics with a rounding mode of
   roundTiesToEven are assumed.

   Note 2: No provision is made for intermediate overflow handling;
   therefore, fsum([1e+308, -1e+308, 1e+308]) returns 1e+308 while
   fsum([1e+308, 1e+308, -1e+308]) raises an OverflowError due to the
   overflow of the first partial sum.

   Note 3: The algorithm has two potential sources of fragility. First, C
   permits arithmetic operations on `double`s to be performed in an
   intermediate format whose range and precision may be greater than those of
   `double` (see for example C99 §5.2.4.2.2, paragraph 8). This can happen for
   example on machines using the now largely historical x87 FPUs. In this case,
   `fsum` can produce incorrect results. If `FLT_EVAL_METHOD` is `0` or `1`, or
   `FLT_EVAL_METHOD` is `2` and `long double` is identical to `double`, then we
   should be safe from this source of errors. Second, an aggressively
   optimizing compiler can re-associate operations so that (for example) the
   statement `yr = hi - x;` is treated as `yr = (x + y) - x` and then
   re-associated as `yr = y + (x - x)`, giving `y = yr` and `lo = 0.0`. That
   re-association would be in violation of the C standard, and should not occur
   except possibly in the presence of unsafe optimizations (e.g., -ffast-math,
   -fassociative-math). Such optimizations should be avoided for this module.

   Note 4: The signature of math.fsum() differs from builtins.sum()
   because the start argument doesn't make sense in the context of
   accurate summation.  Since the partials table is collapsed before
   returning a result, sum(seq2, start=sum(seq1)) may not equal the
   accurate result returned by sum(itertools.chain(seq1, seq2)).
*/

#define NUM_PARTIALS  32  /* initial partials array size, on stack */

/* Extend the partials array p[] by doubling its size. */
static int                          /* non-zero on error */
_fsum_realloc(double **p_ptr, Py_ssize_t  n,
             double  *ps,    Py_ssize_t *m_ptr)
{
    void *v = NULL;
    Py_ssize_t m = *m_ptr;

    m += m;  /* double */
    if (n < m && (size_t)m < ((size_t)PY_SSIZE_T_MAX / sizeof(double))) {
        double *p = *p_ptr;
        if (p == ps) {
            v = PyMem_Malloc(sizeof(double) * m);
            if (v != NULL)
                memcpy(v, ps, sizeof(double) * n);
        }
        else
            v = PyMem_Realloc(p, sizeof(double) * m);
    }
    if (v == NULL) {        /* size overflow or no memory */
        PyErr_SetString(PyExc_MemoryError, "math.fsum partials");
        return 1;
    }
    *p_ptr = (double*) v;
    *m_ptr = m;
    return 0;
}

/* Full precision summation of a sequence of floats.

   def msum(iterable):
       partials = []  # sorted, non-overlapping partial sums
       for x in iterable:
           i = 0
           for y in partials:
               if abs(x) < abs(y):
                   x, y = y, x
               hi = x + y
               lo = y - (hi - x)
               if lo:
                   partials[i] = lo
                   i += 1
               x = hi
           partials[i:] = [x]
       return sum_exact(partials)

   Rounded x+y stored in hi with the roundoff stored in lo.  Together hi+lo
   are exactly equal to x+y.  The inner loop applies hi/lo summation to each
   partial so that the list of partial sums remains exact.

   Sum_exact() adds the partial sums exactly and correctly rounds the final
   result (using the round-half-to-even rule).  The items in partials remain
   non-zero, non-special, non-overlapping and strictly increasing in
   magnitude, but possibly not all having the same sign.

   Depends on IEEE 754 arithmetic guarantees and half-even rounding.
*/

/*[clinic input]
math.fsum

    seq: object
    /

Return an accurate floating point sum of values in the iterable seq.

Assumes IEEE-754 floating point arithmetic.
[clinic start generated code]*/

static PyObject *
math_fsum(PyObject *module, PyObject *seq)
/*[clinic end generated code: output=ba5c672b87fe34fc input=c51b7d8caf6f6e82]*/
{
    PyObject *item, *iter, *sum = NULL;
    Py_ssize_t i, j, n = 0, m = NUM_PARTIALS;
    double x, y, t, ps[NUM_PARTIALS], *p = ps;
    double xsave, special_sum = 0.0, inf_sum = 0.0;
    double hi, yr, lo = 0.0;

    iter = PyObject_GetIter(seq);
    if (iter == NULL)
        return NULL;

    for(;;) {           /* for x in iterable */
        assert(0 <= n && n <= m);
        assert((m == NUM_PARTIALS && p == ps) ||
               (m >  NUM_PARTIALS && p != NULL));

        item = PyIter_Next(iter);
        if (item == NULL) {
            if (PyErr_Occurred())
                goto _fsum_error;
            break;
        }
        ASSIGN_DOUBLE(x, item, error_with_item);
        Py_DECREF(item);

        xsave = x;
        for (i = j = 0; j < n; j++) {       /* for y in partials */
            y = p[j];
            if (fabs(x) < fabs(y)) {
                t = x; x = y; y = t;
            }
            hi = x + y;
            yr = hi - x;
            lo = y - yr;
            if (lo != 0.0)
                p[i++] = lo;
            x = hi;
        }

        n = i;                              /* ps[i:] = [x] */
        if (x != 0.0) {
            if (! Py_IS_FINITE(x)) {
                /* a nonfinite x could arise either as
                   a result of intermediate overflow, or
                   as a result of a nan or inf in the
                   summands */
                if (Py_IS_FINITE(xsave)) {
                    PyErr_SetString(PyExc_OverflowError,
                          "intermediate overflow in fsum");
                    goto _fsum_error;
                }
                if (Py_IS_INFINITY(xsave))
                    inf_sum += xsave;
                special_sum += xsave;
                /* reset partials */
                n = 0;
            }
            else if (n >= m && _fsum_realloc(&p, n, ps, &m))
                goto _fsum_error;
            else
                p[n++] = x;
        }
    }

    if (special_sum != 0.0) {
        if (Py_IS_NAN(inf_sum))
            PyErr_SetString(PyExc_ValueError,
                            "-inf + inf in fsum");
        else
            sum = PyFloat_FromDouble(special_sum);
        goto _fsum_error;
    }

    hi = 0.0;
    if (n > 0) {
        hi = p[--n];
        /* sum_exact(ps, hi) from the top, stop when the sum becomes
           inexact. */
        while (n > 0) {
            x = hi;
            y = p[--n];
            assert(fabs(y) < fabs(x));
            hi = x + y;
            yr = hi - x;
            lo = y - yr;
            if (lo != 0.0)
                break;
        }
        /* Make half-even rounding work across multiple partials.
           Needed so that sum([1e-16, 1, 1e16]) will round-up the last
           digit to two instead of down to zero (the 1e-16 makes the 1
           slightly closer to two).  With a potential 1 ULP rounding
           error fixed-up, math.fsum() can guarantee commutativity. */
        if (n > 0 && ((lo < 0.0 && p[n-1] < 0.0) ||
                      (lo > 0.0 && p[n-1] > 0.0))) {
            y = lo * 2.0;
            x = hi + y;
            yr = x - hi;
            if (y == yr)
                hi = x;
        }
    }
    sum = PyFloat_FromDouble(hi);

  _fsum_error:
    Py_DECREF(iter);
    if (p != ps)
        PyMem_Free(p);
    return sum;

  error_with_item:
    Py_DECREF(item);
    goto _fsum_error;
}

#undef NUM_PARTIALS


static unsigned long
count_set_bits(unsigned long n)
{
    unsigned long count = 0;
    while (n != 0) {
        ++count;
        n &= n - 1; /* clear least significant bit */
    }
    return count;
}

/* Integer square root

Given a nonnegative integer `n`, we want to compute the largest integer
`a` for which `a * a <= n`, or equivalently the integer part of the exact
square root of `n`.

We use an adaptive-precision pure-integer version of Newton's iteration. Given
a positive integer `n`, the algorithm produces at each iteration an integer
approximation `a` to the square root of `n >> s` for some even integer `s`,
with `s` decreasing as the iterations progress. On the final iteration, `s` is
zero and we have an approximation to the square root of `n` itself.

At every step, the approximation `a` is strictly within 1.0 of the true square
root, so we have

    (a - 1)**2 < (n >> s) < (a + 1)**2

After the final iteration, a check-and-correct step is needed to determine
whether `a` or `a - 1` gives the desired integer square root of `n`.

The algorithm is remarkable in its simplicity. There's no need for a
per-iteration check-and-correct step, and termination is straightforward: the
number of iterations is known in advance (it's exactly `floor(log2(log2(n)))`
for `n > 1`). The only tricky part of the correctness proof is in establishing
that the bound `(a - 1)**2 < (n >> s) < (a + 1)**2` is maintained from one
iteration to the next. A sketch of the proof of this is given below.

In addition to the proof sketch, a formal, computer-verified proof
of correctness (using Lean) of an equivalent recursive algorithm can be found
here:

    https://github.com/mdickinson/snippets/blob/master/proofs/isqrt/src/isqrt.lean


Here's Python code equivalent to the C implementation below:

    def isqrt(n):
        """
        Return the integer part of the square root of the input.
        """
        n = operator.index(n)

        if n < 0:
            raise ValueError("isqrt() argument must be nonnegative")
        if n == 0:
            return 0

        c = (n.bit_length() - 1) // 2
        a = 1
        d = 0
        for s in reversed(range(c.bit_length())):
            # Loop invariant: (a-1)**2 < (n >> 2*(c - d)) < (a+1)**2
            e = d
            d = c >> s
            a = (a << d - e - 1) + (n >> 2*c - e - d + 1) // a

        return a - (a*a > n)


Sketch of proof of correctness
------------------------------

The delicate part of the correctness proof is showing that the loop invariant
is preserved from one iteration to the next. That is, just before the line

    a = (a << d - e - 1) + (n >> 2*c - e - d + 1) // a

is executed in the above code, we know that

    (1)  (a - 1)**2 < (n >> 2*(c - e)) < (a + 1)**2.

(since `e` is always the value of `d` from the previous iteration). We must
prove that after that line is executed, we have

    (a - 1)**2 < (n >> 2*(c - d)) < (a + 1)**2

To facilitate the proof, we make some changes of notation. Write `m` for
`n >> 2*(c-d)`, and write `b` for the new value of `a`, so

    b = (a << d - e - 1) + (n >> 2*c - e - d + 1) // a

or equivalently:

    (2)  b = (a << d - e - 1) + (m >> d - e + 1) // a

Then we can rewrite (1) as:

    (3)  (a - 1)**2 < (m >> 2*(d - e)) < (a + 1)**2

and we must show that (b - 1)**2 < m < (b + 1)**2.

From this point on, we switch to mathematical notation, so `/` means exact
division rather than integer division and `^` is used for exponentiation. We
use the `√` symbol for the exact square root. In (3), we can remove the
implicit floor operation to give:

    (4)  (a - 1)^2 < m / 4^(d - e) < (a + 1)^2

Taking square roots throughout (4), scaling by `2^(d-e)`, and rearranging gives

    (5)  0 <= | 2^(d-e)a - √m | < 2^(d-e)

Squaring and dividing through by `2^(d-e+1) a` gives

    (6)  0 <= 2^(d-e-1) a + m / (2^(d-e+1) a) - √m < 2^(d-e-1) / a

We'll show below that `2^(d-e-1) <= a`. Given that, we can replace the
right-hand side of (6) with `1`, and now replacing the central
term `m / (2^(d-e+1) a)` with its floor in (6) gives

    (7) -1 < 2^(d-e-1) a + m // 2^(d-e+1) a - √m < 1

Or equivalently, from (2):

    (7) -1 < b - √m < 1

and rearranging gives that `(b-1)^2 < m < (b+1)^2`, which is what we needed
to prove.

We're not quite done: we still have to prove the inequality `2^(d - e - 1) <=
a` that was used to get line (7) above. From the definition of `c`, we have
`4^c <= n`, which implies

    (8)  4^d <= m

also, since `e == d >> 1`, `d` is at most `2e + 1`, from which it follows
that `2d - 2e - 1 <= d` and hence that

    (9)  4^(2d - 2e - 1) <= m

Dividing both sides by `4^(d - e)` gives

    (10)  4^(d - e - 1) <= m / 4^(d - e)

But we know from (4) that `m / 4^(d-e) < (a + 1)^2`, hence

    (11)  4^(d - e - 1) < (a + 1)^2

Now taking square roots of both sides and observing that both `2^(d-e-1)` and
`a` are integers gives `2^(d - e - 1) <= a`, which is what we needed. This
completes the proof sketch.

*/

/*
    The _approximate_isqrt_tab table provides approximate square roots for
    16-bit integers. For any n in the range 2**14 <= n < 2**16, the value

        a = _approximate_isqrt_tab[(n >> 8) - 64]

    is an approximate square root of n, satisfying (a - 1)**2 < n < (a + 1)**2.

    The table was computed in Python using the expression:

        [min(round(sqrt(256*n + 128)), 255) for n in range(64, 256)]
*/

static const uint8_t _approximate_isqrt_tab[192] = {
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
    140, 141, 142, 143, 144, 144, 145, 146, 147, 148, 149, 150,
    151, 151, 152, 153, 154, 155, 156, 156, 157, 158, 159, 160,
    160, 161, 162, 163, 164, 164, 165, 166, 167, 167, 168, 169,
    170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178,
    179, 179, 180, 181, 181, 182, 183, 183, 184, 185, 186, 186,
    187, 188, 188, 189, 190, 190, 191, 192, 192, 193, 194, 194,
    195, 196, 196, 197, 198, 198, 199, 200, 200, 201, 201, 202,
    203, 203, 204, 205, 205, 206, 206, 207, 208, 208, 209, 210,
    210, 211, 211, 212, 213, 213, 214, 214, 215, 216, 216, 217,
    217, 218, 219, 219, 220, 220, 221, 221, 222, 223, 223, 224,
    224, 225, 225, 226, 227, 227, 228, 228, 229, 229, 230, 230,
    231, 232, 232, 233, 233, 234, 234, 235, 235, 236, 237, 237,
    238, 238, 239, 239, 240, 240, 241, 241, 242, 242, 243, 243,
    244, 244, 245, 246, 246, 247, 247, 248, 248, 249, 249, 250,
    250, 251, 251, 252, 252, 253, 253, 254, 254, 255, 255, 255,
};

/* Approximate square root of a large 64-bit integer.

   Given `n` satisfying `2**62 <= n < 2**64`, return `a`
   satisfying `(a - 1)**2 < n < (a + 1)**2`. */

static inline uint32_t
_approximate_isqrt(uint64_t n)
{
    uint32_t u = _approximate_isqrt_tab[(n >> 56) - 64];
    u = (u << 7) + (uint32_t)(n >> 41) / u;
    return (u << 15) + (uint32_t)((n >> 17) / u);
}

/*[clinic input]
math.isqrt

    n: object
    /

Return the integer part of the square root of the input.
[clinic start generated code]*/

static PyObject *
math_isqrt(PyObject *module, PyObject *n)
/*[clinic end generated code: output=35a6f7f980beab26 input=5b6e7ae4fa6c43d6]*/
{
    int a_too_large, c_bit_length;
    size_t c, d;
    uint64_t m;
    uint32_t u;
    PyObject *a = NULL, *b;

    n = _PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }

    if (_PyLong_IsNegative((PyLongObject *)n)) {
        PyErr_SetString(
            PyExc_ValueError,
            "isqrt() argument must be nonnegative");
        goto error;
    }
    if (_PyLong_IsZero((PyLongObject *)n)) {
        Py_DECREF(n);
        return PyLong_FromLong(0);
    }

    /* c = (n.bit_length() - 1) // 2 */
    c = _PyLong_NumBits(n);
    if (c == (size_t)(-1)) {
        goto error;
    }
    c = (c - 1U) / 2U;

    /* Fast path: if c <= 31 then n < 2**64 and we can compute directly with a
       fast, almost branch-free algorithm. */
    if (c <= 31U) {
        int shift = 31 - (int)c;
        m = (uint64_t)PyLong_AsUnsignedLongLong(n);
        Py_DECREF(n);
        if (m == (uint64_t)(-1) && PyErr_Occurred()) {
            return NULL;
        }
        u = _approximate_isqrt(m << 2*shift) >> shift;
        u -= (uint64_t)u * u > m;
        return PyLong_FromUnsignedLong(u);
    }

    /* Slow path: n >= 2**64. We perform the first five iterations in C integer
       arithmetic, then switch to using Python long integers. */

    /* From n >= 2**64 it follows that c.bit_length() >= 6. */
    c_bit_length = 6;
    while ((c >> c_bit_length) > 0U) {
        ++c_bit_length;
    }

    /* Initialise d and a. */
    d = c >> (c_bit_length - 5);
    b = _PyLong_Rshift(n, 2U*c - 62U);
    if (b == NULL) {
        goto error;
    }
    m = (uint64_t)PyLong_AsUnsignedLongLong(b);
    Py_DECREF(b);
    if (m == (uint64_t)(-1) && PyErr_Occurred()) {
        goto error;
    }
    u = _approximate_isqrt(m) >> (31U - d);
    a = PyLong_FromUnsignedLong(u);
    if (a == NULL) {
        goto error;
    }

    for (int s = c_bit_length - 6; s >= 0; --s) {
        PyObject *q;
        size_t e = d;

        d = c >> s;

        /* q = (n >> 2*c - e - d + 1) // a */
        q = _PyLong_Rshift(n, 2U*c - d - e + 1U);
        if (q == NULL) {
            goto error;
        }
        Py_SETREF(q, PyNumber_FloorDivide(q, a));
        if (q == NULL) {
            goto error;
        }

        /* a = (a << d - 1 - e) + q */
        Py_SETREF(a, _PyLong_Lshift(a, d - 1U - e));
        if (a == NULL) {
            Py_DECREF(q);
            goto error;
        }
        Py_SETREF(a, PyNumber_Add(a, q));
        Py_DECREF(q);
        if (a == NULL) {
            goto error;
        }
    }

    /* The correct result is either a or a - 1. Figure out which, and
       decrement a if necessary. */

    /* a_too_large = n < a * a */
    b = PyNumber_Multiply(a, a);
    if (b == NULL) {
        goto error;
    }
    a_too_large = PyObject_RichCompareBool(n, b, Py_LT);
    Py_DECREF(b);
    if (a_too_large == -1) {
        goto error;
    }

    if (a_too_large) {
        Py_SETREF(a, PyNumber_Subtract(a, _PyLong_GetOne()));
    }
    Py_DECREF(n);
    return a;

  error:
    Py_XDECREF(a);
    Py_DECREF(n);
    return NULL;
}

/* Divide-and-conquer factorial algorithm
 *
 * Based on the formula and pseudo-code provided at:
 * http://www.luschny.de/math/factorial/binarysplitfact.html
 *
 * Faster algorithms exist, but they're more complicated and depend on
 * a fast prime factorization algorithm.
 *
 * Notes on the algorithm
 * ----------------------
 *
 * factorial(n) is written in the form 2**k * m, with m odd.  k and m are
 * computed separately, and then combined using a left shift.
 *
 * The function factorial_odd_part computes the odd part m (i.e., the greatest
 * odd divisor) of factorial(n), using the formula:
 *
 *   factorial_odd_part(n) =
 *
 *        product_{i >= 0} product_{0 < j <= n / 2**i, j odd} j
 *
 * Example: factorial_odd_part(20) =
 *
 *        (1) *
 *        (1) *
 *        (1 * 3 * 5) *
 *        (1 * 3 * 5 * 7 * 9) *
 *        (1 * 3 * 5 * 7 * 9 * 11 * 13 * 15 * 17 * 19)
 *
 * Here i goes from large to small: the first term corresponds to i=4 (any
 * larger i gives an empty product), and the last term corresponds to i=0.
 * Each term can be computed from the last by multiplying by the extra odd
 * numbers required: e.g., to get from the penultimate term to the last one,
 * we multiply by (11 * 13 * 15 * 17 * 19).
 *
 * To see a hint of why this formula works, here are the same numbers as above
 * but with the even parts (i.e., the appropriate powers of 2) included.  For
 * each subterm in the product for i, we multiply that subterm by 2**i:
 *
 *   factorial(20) =
 *
 *        (16) *
 *        (8) *
 *        (4 * 12 * 20) *
 *        (2 * 6 * 10 * 14 * 18) *
 *        (1 * 3 * 5 * 7 * 9 * 11 * 13 * 15 * 17 * 19)
 *
 * The factorial_partial_product function computes the product of all odd j in
 * range(start, stop) for given start and stop.  It's used to compute the
 * partial products like (11 * 13 * 15 * 17 * 19) in the example above.  It
 * operates recursively, repeatedly splitting the range into two roughly equal
 * pieces until the subranges are small enough to be computed using only C
 * integer arithmetic.
 *
 * The two-valuation k (i.e., the exponent of the largest power of 2 dividing
 * the factorial) is computed independently in the main math_factorial
 * function.  By standard results, its value is:
 *
 *    two_valuation = n//2 + n//4 + n//8 + ....
 *
 * It can be shown (e.g., by complete induction on n) that two_valuation is
 * equal to n - count_set_bits(n), where count_set_bits(n) gives the number of
 * '1'-bits in the binary expansion of n.
 */

/* factorial_partial_product: Compute product(range(start, stop, 2)) using
 * divide and conquer.  Assumes start and stop are odd and stop > start.
 * max_bits must be >= bit_length(stop - 2). */

static PyObject *
factorial_partial_product(unsigned long start, unsigned long stop,
                          unsigned long max_bits)
{
    unsigned long midpoint, num_operands;
    PyObject *left = NULL, *right = NULL, *result = NULL;

    /* If the return value will fit an unsigned long, then we can
     * multiply in a tight, fast loop where each multiply is O(1).
     * Compute an upper bound on the number of bits required to store
     * the answer.
     *
     * Storing some integer z requires floor(lg(z))+1 bits, which is
     * conveniently the value returned by bit_length(z).  The
     * product x*y will require at most
     * bit_length(x) + bit_length(y) bits to store, based
     * on the idea that lg product = lg x + lg y.
     *
     * We know that stop - 2 is the largest number to be multiplied.  From
     * there, we have: bit_length(answer) <= num_operands *
     * bit_length(stop - 2)
     */

    num_operands = (stop - start) / 2;
    /* The "num_operands <= 8 * SIZEOF_LONG" check guards against the
     * unlikely case of an overflow in num_operands * max_bits. */
    if (num_operands <= 8 * SIZEOF_LONG &&
        num_operands * max_bits <= 8 * SIZEOF_LONG) {
        unsigned long j, total;
        for (total = start, j = start + 2; j < stop; j += 2)
            total *= j;
        return PyLong_FromUnsignedLong(total);
    }

    /* find midpoint of range(start, stop), rounded up to next odd number. */
    midpoint = (start + num_operands) | 1;
    left = factorial_partial_product(start, midpoint,
                                     _Py_bit_length(midpoint - 2));
    if (left == NULL)
        goto error;
    right = factorial_partial_product(midpoint, stop, max_bits);
    if (right == NULL)
        goto error;
    result = PyNumber_Multiply(left, right);

  error:
    Py_XDECREF(left);
    Py_XDECREF(right);
    return result;
}

/* factorial_odd_part:  compute the odd part of factorial(n). */

static PyObject *
factorial_odd_part(unsigned long n)
{
    long i;
    unsigned long v, lower, upper;
    PyObject *partial, *tmp, *inner, *outer;

    inner = PyLong_FromLong(1);
    if (inner == NULL)
        return NULL;
    outer = Py_NewRef(inner);

    upper = 3;
    for (i = _Py_bit_length(n) - 2; i >= 0; i--) {
        v = n >> i;
        if (v <= 2)
            continue;
        lower = upper;
        /* (v + 1) | 1 = least odd integer strictly larger than n / 2**i */
        upper = (v + 1) | 1;
        /* Here inner is the product of all odd integers j in the range (0,
           n/2**(i+1)].  The factorial_partial_product call below gives the
           product of all odd integers j in the range (n/2**(i+1), n/2**i]. */
        partial = factorial_partial_product(lower, upper, _Py_bit_length(upper-2));
        /* inner *= partial */
        if (partial == NULL)
            goto error;
        tmp = PyNumber_Multiply(inner, partial);
        Py_DECREF(partial);
        if (tmp == NULL)
            goto error;
        Py_SETREF(inner, tmp);
        /* Now inner is the product of all odd integers j in the range (0,
           n/2**i], giving the inner product in the formula above. */

        /* outer *= inner; */
        tmp = PyNumber_Multiply(outer, inner);
        if (tmp == NULL)
            goto error;
        Py_SETREF(outer, tmp);
    }
    Py_DECREF(inner);
    return outer;

  error:
    Py_DECREF(outer);
    Py_DECREF(inner);
    return NULL;
}


/* Lookup table for small factorial values */

static const unsigned long SmallFactorials[] = {
    1, 1, 2, 6, 24, 120, 720, 5040, 40320,
    362880, 3628800, 39916800, 479001600,
#if SIZEOF_LONG >= 8
    6227020800, 87178291200, 1307674368000,
    20922789888000, 355687428096000, 6402373705728000,
    121645100408832000, 2432902008176640000
#endif
};

/*[clinic input]
math.factorial

    n as arg: object
    /

Find n!.

Raise a ValueError if x is negative or non-integral.
[clinic start generated code]*/

static PyObject *
math_factorial(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=6686f26fae00e9ca input=713fb771677e8c31]*/
{
    long x, two_valuation;
    int overflow;
    PyObject *result, *odd_part;

    x = PyLong_AsLongAndOverflow(arg, &overflow);
    if (x == -1 && PyErr_Occurred()) {
        return NULL;
    }
    else if (overflow == 1) {
        PyErr_Format(PyExc_OverflowError,
                     "factorial() argument should not exceed %ld",
                     LONG_MAX);
        return NULL;
    }
    else if (overflow == -1 || x < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "factorial() not defined for negative values");
        return NULL;
    }

    /* use lookup table if x is small */
    if (x < (long)Py_ARRAY_LENGTH(SmallFactorials))
        return PyLong_FromUnsignedLong(SmallFactorials[x]);

    /* else express in the form odd_part * 2**two_valuation, and compute as
       odd_part << two_valuation. */
    odd_part = factorial_odd_part(x);
    if (odd_part == NULL)
        return NULL;
    two_valuation = x - count_set_bits(x);
    result = _PyLong_Lshift(odd_part, two_valuation);
    Py_DECREF(odd_part);
    return result;
}


/*[clinic input]
math.trunc

    x: object
    /

Truncates the Real x to the nearest Integral toward 0.

Uses the __trunc__ magic method.
[clinic start generated code]*/

static PyObject *
math_trunc(PyObject *module, PyObject *x)
/*[clinic end generated code: output=34b9697b707e1031 input=2168b34e0a09134d]*/
{
    PyObject *trunc, *result;

    if (PyFloat_CheckExact(x)) {
        return PyFloat_Type.tp_as_number->nb_int(x);
    }

    if (!_PyType_IsReady(Py_TYPE(x))) {
        if (PyType_Ready(Py_TYPE(x)) < 0)
            return NULL;
    }

    math_module_state *state = get_math_module_state(module);
    trunc = _PyObject_LookupSpecial(x, state->str___trunc__);
    if (trunc == NULL) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_TypeError,
                         "type %.100s doesn't define __trunc__ method",
                         Py_TYPE(x)->tp_name);
        return NULL;
    }
    result = _PyObject_CallNoArgs(trunc);
    Py_DECREF(trunc);
    return result;
}


/*[clinic input]
math.frexp

    x: double
    /

Return the mantissa and exponent of x, as pair (m, e).

m is a float and e is an int, such that x = m * 2.**e.
If x is 0, m and e are both 0.  Else 0.5 <= abs(m) < 1.0.
[clinic start generated code]*/

static PyObject *
math_frexp_impl(PyObject *module, double x)
/*[clinic end generated code: output=03e30d252a15ad4a input=96251c9e208bc6e9]*/
{
    int i;
    /* deal with special cases directly, to sidestep platform
       differences */
    if (Py_IS_NAN(x) || Py_IS_INFINITY(x) || !x) {
        i = 0;
    }
    else {
        x = frexp(x, &i);
    }
    return Py_BuildValue("(di)", x, i);
}


/*[clinic input]
math.ldexp

    x: double
    i: object
    /

Return x * (2**i).

This is essentially the inverse of frexp().
[clinic start generated code]*/

static PyObject *
math_ldexp_impl(PyObject *module, double x, PyObject *i)
/*[clinic end generated code: output=b6892f3c2df9cc6a input=17d5970c1a40a8c1]*/
{
    double r;
    long exp;
    int overflow;

    if (PyLong_Check(i)) {
        /* on overflow, replace exponent with either LONG_MAX
           or LONG_MIN, depending on the sign. */
        exp = PyLong_AsLongAndOverflow(i, &overflow);
        if (exp == -1 && PyErr_Occurred())
            return NULL;
        if (overflow)
            exp = overflow < 0 ? LONG_MIN : LONG_MAX;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Expected an int as second argument to ldexp.");
        return NULL;
    }

    if (x == 0. || !Py_IS_FINITE(x)) {
        /* NaNs, zeros and infinities are returned unchanged */
        r = x;
        errno = 0;
    } else if (exp > INT_MAX) {
        /* overflow */
        r = copysign(Py_HUGE_VAL, x);
        errno = ERANGE;
    } else if (exp < INT_MIN) {
        /* underflow to +-0 */
        r = copysign(0., x);
        errno = 0;
    } else {
        errno = 0;
        r = ldexp(x, (int)exp);
        if (Py_IS_INFINITY(r))
            errno = ERANGE;
    }

    if (errno && is_error(r))
        return NULL;
    return PyFloat_FromDouble(r);
}


/*[clinic input]
math.modf

    x: double
    /

Return the fractional and integer parts of x.

Both results carry the sign of x and are floats.
[clinic start generated code]*/

static PyObject *
math_modf_impl(PyObject *module, double x)
/*[clinic end generated code: output=90cee0260014c3c0 input=b4cfb6786afd9035]*/
{
    double y;
    /* some platforms don't do the right thing for NaNs and
       infinities, so we take care of special cases directly. */
    if (Py_IS_INFINITY(x))
        return Py_BuildValue("(dd)", copysign(0., x), x);
    else if (Py_IS_NAN(x))
        return Py_BuildValue("(dd)", x, x);

    errno = 0;
    x = modf(x, &y);
    return Py_BuildValue("(dd)", x, y);
}


/* A decent logarithm is easy to compute even for huge ints, but libm can't
   do that by itself -- loghelper can.  func is log or log10, and name is
   "log" or "log10".  Note that overflow of the result isn't possible: an int
   can contain no more than INT_MAX * SHIFT bits, so has value certainly less
   than 2**(2**64 * 2**16) == 2**2**80, and log2 of that is 2**80, which is
   small enough to fit in an IEEE single.  log and log10 are even smaller.
   However, intermediate overflow is possible for an int if the number of bits
   in that int is larger than PY_SSIZE_T_MAX. */

static PyObject*
loghelper(PyObject* arg, double (*func)(double))
{
    /* If it is int, do it ourselves. */
    if (PyLong_Check(arg)) {
        double x, result;
        Py_ssize_t e;

        /* Negative or zero inputs give a ValueError. */
        if (!_PyLong_IsPositive((PyLongObject *)arg)) {
            PyErr_SetString(PyExc_ValueError,
                            "math domain error");
            return NULL;
        }

        x = PyLong_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            if (!PyErr_ExceptionMatches(PyExc_OverflowError))
                return NULL;
            /* Here the conversion to double overflowed, but it's possible
               to compute the log anyway.  Clear the exception and continue. */
            PyErr_Clear();
            x = _PyLong_Frexp((PyLongObject *)arg, &e);
            if (x == -1.0 && PyErr_Occurred())
                return NULL;
            /* Value is ~= x * 2**e, so the log ~= log(x) + log(2) * e. */
            result = func(x) + func(2.0) * e;
        }
        else
            /* Successfully converted x to a double. */
            result = func(x);
        return PyFloat_FromDouble(result);
    }

    /* Else let libm handle it by itself. */
    return math_1(arg, func, 0);
}


/* AC: cannot convert yet, see gh-102839 and gh-89381, waiting
   for support of multiple signatures */
static PyObject *
math_log(PyObject *module, PyObject * const *args, Py_ssize_t nargs)
{
    PyObject *num, *den;
    PyObject *ans;

    if (!_PyArg_CheckPositional("log", nargs, 1, 2))
        return NULL;

    num = loghelper(args[0], m_log);
    if (num == NULL || nargs == 1)
        return num;

    den = loghelper(args[1], m_log);
    if (den == NULL) {
        Py_DECREF(num);
        return NULL;
    }

    ans = PyNumber_TrueDivide(num, den);
    Py_DECREF(num);
    Py_DECREF(den);
    return ans;
}

PyDoc_STRVAR(math_log_doc,
"log(x, [base=math.e])\n\
Return the logarithm of x to the given base.\n\n\
If the base is not specified, returns the natural logarithm (base e) of x.");

/*[clinic input]
math.log2

    x: object
    /

Return the base 2 logarithm of x.
[clinic start generated code]*/

static PyObject *
math_log2(PyObject *module, PyObject *x)
/*[clinic end generated code: output=5425899a4d5d6acb input=08321262bae4f39b]*/
{
    return loghelper(x, m_log2);
}


/*[clinic input]
math.log10

    x: object
    /

Return the base 10 logarithm of x.
[clinic start generated code]*/

static PyObject *
math_log10(PyObject *module, PyObject *x)
/*[clinic end generated code: output=be72a64617df9c6f input=b2469d02c6469e53]*/
{
    return loghelper(x, m_log10);
}


/*[clinic input]
math.fmod

    x: double
    y: double
    /

Return fmod(x, y), according to platform C.

x % y may differ.
[clinic start generated code]*/

static PyObject *
math_fmod_impl(PyObject *module, double x, double y)
/*[clinic end generated code: output=7559d794343a27b5 input=4f84caa8cfc26a03]*/
{
    double r;
    /* fmod(x, +/-Inf) returns x for finite x. */
    if (Py_IS_INFINITY(y) && Py_IS_FINITE(x))
        return PyFloat_FromDouble(x);
    errno = 0;
    r = fmod(x, y);
    if (Py_IS_NAN(r)) {
        if (!Py_IS_NAN(x) && !Py_IS_NAN(y))
            errno = EDOM;
        else
            errno = 0;
    }
    if (errno && is_error(r))
        return NULL;
    else
        return PyFloat_FromDouble(r);
}

/*
Given a *vec* of values, compute the vector norm:

    sqrt(sum(x ** 2 for x in vec))

The *max* variable should be equal to the largest fabs(x).
The *n* variable is the length of *vec*.
If n==0, then *max* should be 0.0.
If an infinity is present in the vec, *max* should be INF.
The *found_nan* variable indicates whether some member of
the *vec* is a NaN.

To avoid overflow/underflow and to achieve high accuracy giving results
that are almost always correctly rounded, four techniques are used:

* lossless scaling using a power-of-two scaling factor
* accurate squaring using Veltkamp-Dekker splitting [1]
  or an equivalent with an fma() call
* compensated summation using a variant of the Neumaier algorithm [2]
* differential correction of the square root [3]

The usual presentation of the Neumaier summation algorithm has an
expensive branch depending on which operand has the larger
magnitude.  We avoid this cost by arranging the calculation so that
fabs(csum) is always as large as fabs(x).

To establish the invariant, *csum* is initialized to 1.0 which is
always larger than x**2 after scaling or after division by *max*.
After the loop is finished, the initial 1.0 is subtracted out for a
net zero effect on the final sum.  Since *csum* will be greater than
1.0, the subtraction of 1.0 will not cause fractional digits to be
dropped from *csum*.

To get the full benefit from compensated summation, the largest
addend should be in the range: 0.5 <= |x| <= 1.0.  Accordingly,
scaling or division by *max* should not be skipped even if not
otherwise needed to prevent overflow or loss of precision.

The assertion that hi*hi <= 1.0 is a bit subtle.  Each vector element
gets scaled to a magnitude below 1.0.  The Veltkamp-Dekker splitting
algorithm gives a *hi* value that is correctly rounded to half
precision.  When a value at or below 1.0 is correctly rounded, it
never goes above 1.0.  And when values at or below 1.0 are squared,
they remain at or below 1.0, thus preserving the summation invariant.

Another interesting assertion is that csum+lo*lo == csum. In the loop,
each scaled vector element has a magnitude less than 1.0.  After the
Veltkamp split, *lo* has a maximum value of 2**-27.  So the maximum
value of *lo* squared is 2**-54.  The value of ulp(1.0)/2.0 is 2**-53.
Given that csum >= 1.0, we have:
    lo**2 <= 2**-54 < 2**-53 == 1/2*ulp(1.0) <= ulp(csum)/2
Since lo**2 is less than 1/2 ulp(csum), we have csum+lo*lo == csum.

To minimize loss of information during the accumulation of fractional
values, each term has a separate accumulator.  This also breaks up
sequential dependencies in the inner loop so the CPU can maximize
floating point throughput. [4]  On an Apple M1 Max, hypot(*vec)
takes only 3.33 µsec when len(vec) == 1000.

The square root differential correction is needed because a
correctly rounded square root of a correctly rounded sum of
squares can still be off by as much as one ulp.

The differential correction starts with a value *x* that is
the difference between the square of *h*, the possibly inaccurately
rounded square root, and the accurately computed sum of squares.
The correction is the first order term of the Maclaurin series
expansion of sqrt(h**2 + x) == h + x/(2*h) + O(x**2). [5]

Essentially, this differential correction is equivalent to one
refinement step in Newton's divide-and-average square root
algorithm, effectively doubling the number of accurate bits.
This technique is used in Dekker's SQRT2 algorithm and again in
Borges' ALGORITHM 4 and 5.

The hypot() function is faithfully rounded (less than 1 ulp error)
and usually correctly rounded (within 1/2 ulp).  The squaring
step is exact.  The Neumaier summation computes as if in doubled
precision (106 bits) and has the advantage that its input squares
are non-negative so that the condition number of the sum is one.
The square root with a differential correction is likewise computed
as if in doubled precision.

For n <= 1000, prior to the final addition that rounds the overall
result, the internal accuracy of "h" together with its correction of
"x / (2.0 * h)" is at least 100 bits. [6] Also, hypot() was tested
against a Decimal implementation with prec=300.  After 100 million
trials, no incorrectly rounded examples were found.  In addition,
perfect commutativity (all permutations are exactly equal) was
verified for 1 billion random inputs with n=5. [7]

References:

1. Veltkamp-Dekker splitting: http://csclub.uwaterloo.ca/~pbarfuss/dekker1971.pdf
2. Compensated summation:  http://www.ti3.tu-harburg.de/paper/rump/Ru08b.pdf
3. Square root differential correction:  https://arxiv.org/pdf/1904.09481.pdf
4. Data dependency graph:  https://bugs.python.org/file49439/hypot.png
5. https://www.wolframalpha.com/input/?i=Maclaurin+series+sqrt%28h**2+%2B+x%29+at+x%3D0
6. Analysis of internal accuracy:  https://bugs.python.org/file49484/best_frac.py
7. Commutativity test:  https://bugs.python.org/file49448/test_hypot_commutativity.py

*/

static inline double
vector_norm(Py_ssize_t n, double *vec, double max, int found_nan)
{
    double x, h, scale, csum = 1.0, frac1 = 0.0, frac2 = 0.0;
    DoubleLength pr, sm;
    int max_e;
    Py_ssize_t i;

    if (Py_IS_INFINITY(max)) {
        return max;
    }
    if (found_nan) {
        return Py_NAN;
    }
    if (max == 0.0 || n <= 1) {
        return max;
    }
    frexp(max, &max_e);
    if (max_e < -1023) {
        /* When max_e < -1023, ldexp(1.0, -max_e) would overflow. */
        for (i=0 ; i < n ; i++) {
            vec[i] /= DBL_MIN;          // convert subnormals to normals
        }
        return DBL_MIN * vector_norm(n, vec, max / DBL_MIN, found_nan);
    }
    scale = ldexp(1.0, -max_e);
    assert(max * scale >= 0.5);
    assert(max * scale < 1.0);
    for (i=0 ; i < n ; i++) {
        x = vec[i];
        assert(Py_IS_FINITE(x) && fabs(x) <= max);
        x *= scale;                     // lossless scaling
        assert(fabs(x) < 1.0);
        pr = dl_mul(x, x);              // lossless squaring
        assert(pr.hi <= 1.0);
        sm = dl_fast_sum(csum, pr.hi);  // lossless addition
        csum = sm.hi;
        frac1 += pr.lo;                 // lossy addition
        frac2 += sm.lo;                 // lossy addition
    }
    h = sqrt(csum - 1.0 + (frac1 + frac2));
    pr = dl_mul(-h, h);
    sm = dl_fast_sum(csum, pr.hi);
    csum = sm.hi;
    frac1 += pr.lo;
    frac2 += sm.lo;
    x = csum - 1.0 + (frac1 + frac2);
    h +=  x / (2.0 * h);                 // differential correction
    return h / scale;
}

#define NUM_STACK_ELEMS 16

/*[clinic input]
math.dist

    p: object
    q: object
    /

Return the Euclidean distance between two points p and q.

The points should be specified as sequences (or iterables) of
coordinates.  Both inputs must have the same dimension.

Roughly equivalent to:
    sqrt(sum((px - qx) ** 2.0 for px, qx in zip(p, q)))
[clinic start generated code]*/

static PyObject *
math_dist_impl(PyObject *module, PyObject *p, PyObject *q)
/*[clinic end generated code: output=56bd9538d06bbcfe input=74e85e1b6092e68e]*/
{
    PyObject *item;
    double max = 0.0;
    double x, px, qx, result;
    Py_ssize_t i, m, n;
    int found_nan = 0, p_allocated = 0, q_allocated = 0;
    double diffs_on_stack[NUM_STACK_ELEMS];
    double *diffs = diffs_on_stack;

    if (!PyTuple_Check(p)) {
        p = PySequence_Tuple(p);
        if (p == NULL) {
            return NULL;
        }
        p_allocated = 1;
    }
    if (!PyTuple_Check(q)) {
        q = PySequence_Tuple(q);
        if (q == NULL) {
            if (p_allocated) {
                Py_DECREF(p);
            }
            return NULL;
        }
        q_allocated = 1;
    }

    m = PyTuple_GET_SIZE(p);
    n = PyTuple_GET_SIZE(q);
    if (m != n) {
        PyErr_SetString(PyExc_ValueError,
                        "both points must have the same number of dimensions");
        goto error_exit;
    }
    if (n > NUM_STACK_ELEMS) {
        diffs = (double *) PyObject_Malloc(n * sizeof(double));
        if (diffs == NULL) {
            PyErr_NoMemory();
            goto error_exit;
        }
    }
    for (i=0 ; i<n ; i++) {
        item = PyTuple_GET_ITEM(p, i);
        ASSIGN_DOUBLE(px, item, error_exit);
        item = PyTuple_GET_ITEM(q, i);
        ASSIGN_DOUBLE(qx, item, error_exit);
        x = fabs(px - qx);
        diffs[i] = x;
        found_nan |= Py_IS_NAN(x);
        if (x > max) {
            max = x;
        }
    }
    result = vector_norm(n, diffs, max, found_nan);
    if (diffs != diffs_on_stack) {
        PyObject_Free(diffs);
    }
    if (p_allocated) {
        Py_DECREF(p);
    }
    if (q_allocated) {
        Py_DECREF(q);
    }
    return PyFloat_FromDouble(result);

  error_exit:
    if (diffs != diffs_on_stack) {
        PyObject_Free(diffs);
    }
    if (p_allocated) {
        Py_DECREF(p);
    }
    if (q_allocated) {
        Py_DECREF(q);
    }
    return NULL;
}

/* AC: cannot convert yet, waiting for *args support */
static PyObject *
math_hypot(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    Py_ssize_t i;
    PyObject *item;
    double max = 0.0;
    double x, result;
    int found_nan = 0;
    double coord_on_stack[NUM_STACK_ELEMS];
    double *coordinates = coord_on_stack;

    if (nargs > NUM_STACK_ELEMS) {
        coordinates = (double *) PyObject_Malloc(nargs * sizeof(double));
        if (coordinates == NULL) {
            return PyErr_NoMemory();
        }
    }
    for (i = 0; i < nargs; i++) {
        item = args[i];
        ASSIGN_DOUBLE(x, item, error_exit);
        x = fabs(x);
        coordinates[i] = x;
        found_nan |= Py_IS_NAN(x);
        if (x > max) {
            max = x;
        }
    }
    result = vector_norm(nargs, coordinates, max, found_nan);
    if (coordinates != coord_on_stack) {
        PyObject_Free(coordinates);
    }
    return PyFloat_FromDouble(result);

  error_exit:
    if (coordinates != coord_on_stack) {
        PyObject_Free(coordinates);
    }
    return NULL;
}

#undef NUM_STACK_ELEMS

PyDoc_STRVAR(math_hypot_doc,
             "hypot(*coordinates) -> value\n\n\
Multidimensional Euclidean distance from the origin to a point.\n\
\n\
Roughly equivalent to:\n\
    sqrt(sum(x**2 for x in coordinates))\n\
\n\
For a two dimensional point (x, y), gives the hypotenuse\n\
using the Pythagorean theorem:  sqrt(x*x + y*y).\n\
\n\
For example, the hypotenuse of a 3/4/5 right triangle is:\n\
\n\
    >>> hypot(3.0, 4.0)\n\
    5.0\n\
");

/** sumprod() ***************************************************************/

/* Forward declaration */
static inline int _check_long_mult_overflow(long a, long b);

static inline bool
long_add_would_overflow(long a, long b)
{
    return (a > 0) ? (b > LONG_MAX - a) : (b < LONG_MIN - a);
}

/*[clinic input]
math.sumprod

    p: object
    q: object
    /

Return the sum of products of values from two iterables p and q.

Roughly equivalent to:

    sum(itertools.starmap(operator.mul, zip(p, q, strict=True)))

For float and mixed int/float inputs, the intermediate products
and sums are computed with extended precision.
[clinic start generated code]*/

static PyObject *
math_sumprod_impl(PyObject *module, PyObject *p, PyObject *q)
/*[clinic end generated code: output=6722dbfe60664554 input=82be54fe26f87e30]*/
{
    PyObject *p_i = NULL, *q_i = NULL, *term_i = NULL, *new_total = NULL;
    PyObject *p_it, *q_it, *total;
    iternextfunc p_next, q_next;
    bool p_stopped = false, q_stopped = false;
    bool int_path_enabled = true, int_total_in_use = false;
    bool flt_path_enabled = true, flt_total_in_use = false;
    long int_total = 0;
    TripleLength flt_total = tl_zero;

    p_it = PyObject_GetIter(p);
    if (p_it == NULL) {
        return NULL;
    }
    q_it = PyObject_GetIter(q);
    if (q_it == NULL) {
        Py_DECREF(p_it);
        return NULL;
    }
    total = PyLong_FromLong(0);
    if (total == NULL) {
        Py_DECREF(p_it);
        Py_DECREF(q_it);
        return NULL;
    }
    p_next = *Py_TYPE(p_it)->tp_iternext;
    q_next = *Py_TYPE(q_it)->tp_iternext;
    while (1) {
        bool finished;

        assert (p_i == NULL);
        assert (q_i == NULL);
        assert (term_i == NULL);
        assert (new_total == NULL);

        assert (p_it != NULL);
        assert (q_it != NULL);
        assert (total != NULL);

        p_i = p_next(p_it);
        if (p_i == NULL) {
            if (PyErr_Occurred()) {
                if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
                    goto err_exit;
                }
                PyErr_Clear();
            }
            p_stopped = true;
        }
        q_i = q_next(q_it);
        if (q_i == NULL) {
            if (PyErr_Occurred()) {
                if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
                    goto err_exit;
                }
                PyErr_Clear();
            }
            q_stopped = true;
        }
        if (p_stopped != q_stopped) {
            PyErr_Format(PyExc_ValueError, "Inputs are not the same length");
            goto err_exit;
        }
        finished = p_stopped & q_stopped;

        if (int_path_enabled) {

            if (!finished && PyLong_CheckExact(p_i) & PyLong_CheckExact(q_i)) {
                int overflow;
                long int_p, int_q, int_prod;

                int_p = PyLong_AsLongAndOverflow(p_i, &overflow);
                if (overflow) {
                    goto finalize_int_path;
                }
                int_q = PyLong_AsLongAndOverflow(q_i, &overflow);
                if (overflow) {
                    goto finalize_int_path;
                }
                if (_check_long_mult_overflow(int_p, int_q)) {
                    goto finalize_int_path;
                }
                int_prod = int_p * int_q;
                if (long_add_would_overflow(int_total, int_prod)) {
                    goto finalize_int_path;
                }
                int_total += int_prod;
                int_total_in_use = true;
                Py_CLEAR(p_i);
                Py_CLEAR(q_i);
                continue;
            }

          finalize_int_path:
            // We're finished, overflowed, or have a non-int
            int_path_enabled = false;
            if (int_total_in_use) {
                term_i = PyLong_FromLong(int_total);
                if (term_i == NULL) {
                    goto err_exit;
                }
                new_total = PyNumber_Add(total, term_i);
                if (new_total == NULL) {
                    goto err_exit;
                }
                Py_SETREF(total, new_total);
                new_total = NULL;
                Py_CLEAR(term_i);
                int_total = 0;   // An ounce of prevention, ...
                int_total_in_use = false;
            }
        }

        if (flt_path_enabled) {

            if (!finished) {
                double flt_p, flt_q;
                bool p_type_float = PyFloat_CheckExact(p_i);
                bool q_type_float = PyFloat_CheckExact(q_i);
                if (p_type_float && q_type_float) {
                    flt_p = PyFloat_AS_DOUBLE(p_i);
                    flt_q = PyFloat_AS_DOUBLE(q_i);
                } else if (p_type_float && (PyLong_CheckExact(q_i) || PyBool_Check(q_i))) {
                    /* We care about float/int pairs and int/float pairs because
                       they arise naturally in several use cases such as price
                       times quantity, measurements with integer weights, or
                       data selected by a vector of bools. */
                    flt_p = PyFloat_AS_DOUBLE(p_i);
                    flt_q = PyLong_AsDouble(q_i);
                    if (flt_q == -1.0 && PyErr_Occurred()) {
                        PyErr_Clear();
                        goto finalize_flt_path;
                    }
                } else if (q_type_float && (PyLong_CheckExact(p_i) || PyBool_Check(p_i))) {
                    flt_q = PyFloat_AS_DOUBLE(q_i);
                    flt_p = PyLong_AsDouble(p_i);
                    if (flt_p == -1.0 && PyErr_Occurred()) {
                        PyErr_Clear();
                        goto finalize_flt_path;
                    }
                } else {
                    goto finalize_flt_path;
                }
                TripleLength new_flt_total = tl_fma(flt_p, flt_q, flt_total);
                if (isfinite(new_flt_total.hi)) {
                    flt_total = new_flt_total;
                    flt_total_in_use = true;
                    Py_CLEAR(p_i);
                    Py_CLEAR(q_i);
                    continue;
                }
            }

          finalize_flt_path:
            // We're finished, overflowed, have a non-float, or got a non-finite value
            flt_path_enabled = false;
            if (flt_total_in_use) {
                term_i = PyFloat_FromDouble(tl_to_d(flt_total));
                if (term_i == NULL) {
                    goto err_exit;
                }
                new_total = PyNumber_Add(total, term_i);
                if (new_total == NULL) {
                    goto err_exit;
                }
                Py_SETREF(total, new_total);
                new_total = NULL;
                Py_CLEAR(term_i);
                flt_total = tl_zero;
                flt_total_in_use = false;
            }
        }

        assert(!int_total_in_use);
        assert(!flt_total_in_use);
        if (finished) {
            goto normal_exit;
        }
        term_i = PyNumber_Multiply(p_i, q_i);
        if (term_i == NULL) {
            goto err_exit;
        }
        new_total = PyNumber_Add(total, term_i);
        if (new_total == NULL) {
            goto err_exit;
        }
        Py_SETREF(total, new_total);
        new_total = NULL;
        Py_CLEAR(p_i);
        Py_CLEAR(q_i);
        Py_CLEAR(term_i);
    }

 normal_exit:
    Py_DECREF(p_it);
    Py_DECREF(q_it);
    return total;

 err_exit:
    Py_DECREF(p_it);
    Py_DECREF(q_it);
    Py_DECREF(total);
    Py_XDECREF(p_i);
    Py_XDECREF(q_i);
    Py_XDECREF(term_i);
    Py_XDECREF(new_total);
    return NULL;
}


/* pow can't use math_2, but needs its own wrapper: the problem is
   that an infinite result can arise either as a result of overflow
   (in which case OverflowError should be raised) or as a result of
   e.g. 0.**-5. (for which ValueError needs to be raised.)
*/

/*[clinic input]
math.pow

    x: double
    y: double
    /

Return x**y (x to the power of y).
[clinic start generated code]*/

static PyObject *
math_pow_impl(PyObject *module, double x, double y)
/*[clinic end generated code: output=fff93e65abccd6b0 input=c26f1f6075088bfd]*/
{
    double r;
    int odd_y;

    /* deal directly with IEEE specials, to cope with problems on various
       platforms whose semantics don't exactly match C99 */
    r = 0.; /* silence compiler warning */
    if (!Py_IS_FINITE(x) || !Py_IS_FINITE(y)) {
        errno = 0;
        if (Py_IS_NAN(x))
            r = y == 0. ? 1. : x; /* NaN**0 = 1 */
        else if (Py_IS_NAN(y))
            r = x == 1. ? 1. : y; /* 1**NaN = 1 */
        else if (Py_IS_INFINITY(x)) {
            odd_y = Py_IS_FINITE(y) && fmod(fabs(y), 2.0) == 1.0;
            if (y > 0.)
                r = odd_y ? x : fabs(x);
            else if (y == 0.)
                r = 1.;
            else /* y < 0. */
                r = odd_y ? copysign(0., x) : 0.;
        }
        else {
            assert(Py_IS_INFINITY(y));
            if (fabs(x) == 1.0)
                r = 1.;
            else if (y > 0. && fabs(x) > 1.0)
                r = y;
            else if (y < 0. && fabs(x) < 1.0) {
                r = -y; /* result is +inf */
            }
            else
                r = 0.;
        }
    }
    else {
        /* let libm handle finite**finite */
        errno = 0;
        r = pow(x, y);
        /* a NaN result should arise only from (-ve)**(finite
           non-integer); in this case we want to raise ValueError. */
        if (!Py_IS_FINITE(r)) {
            if (Py_IS_NAN(r)) {
                errno = EDOM;
            }
            /*
               an infinite result here arises either from:
               (A) (+/-0.)**negative (-> divide-by-zero)
               (B) overflow of x**y with x and y finite
            */
            else if (Py_IS_INFINITY(r)) {
                if (x == 0.)
                    errno = EDOM;
                else
                    errno = ERANGE;
            }
        }
    }

    if (errno && is_error(r))
        return NULL;
    else
        return PyFloat_FromDouble(r);
}


static const double degToRad = Py_MATH_PI / 180.0;
static const double radToDeg = 180.0 / Py_MATH_PI;

/*[clinic input]
math.degrees

    x: double
    /

Convert angle x from radians to degrees.
[clinic start generated code]*/

static PyObject *
math_degrees_impl(PyObject *module, double x)
/*[clinic end generated code: output=7fea78b294acd12f input=81e016555d6e3660]*/
{
    return PyFloat_FromDouble(x * radToDeg);
}


/*[clinic input]
math.radians

    x: double
    /

Convert angle x from degrees to radians.
[clinic start generated code]*/

static PyObject *
math_radians_impl(PyObject *module, double x)
/*[clinic end generated code: output=34daa47caf9b1590 input=91626fc489fe3d63]*/
{
    return PyFloat_FromDouble(x * degToRad);
}


/*[clinic input]
math.isfinite

    x: double
    /

Return True if x is neither an infinity nor a NaN, and False otherwise.
[clinic start generated code]*/

static PyObject *
math_isfinite_impl(PyObject *module, double x)
/*[clinic end generated code: output=8ba1f396440c9901 input=46967d254812e54a]*/
{
    return PyBool_FromLong((long)Py_IS_FINITE(x));
}


/*[clinic input]
math.isnan

    x: double
    /

Return True if x is a NaN (not a number), and False otherwise.
[clinic start generated code]*/

static PyObject *
math_isnan_impl(PyObject *module, double x)
/*[clinic end generated code: output=f537b4d6df878c3e input=935891e66083f46a]*/
{
    return PyBool_FromLong((long)Py_IS_NAN(x));
}


/*[clinic input]
math.isinf

    x: double
    /

Return True if x is a positive or negative infinity, and False otherwise.
[clinic start generated code]*/

static PyObject *
math_isinf_impl(PyObject *module, double x)
/*[clinic end generated code: output=9f00cbec4de7b06b input=32630e4212cf961f]*/
{
    return PyBool_FromLong((long)Py_IS_INFINITY(x));
}


/*[clinic input]
math.isclose -> bool

    a: double
    b: double
    *
    rel_tol: double = 1e-09
        maximum difference for being considered "close", relative to the
        magnitude of the input values
    abs_tol: double = 0.0
        maximum difference for being considered "close", regardless of the
        magnitude of the input values

Determine whether two floating point numbers are close in value.

Return True if a is close in value to b, and False otherwise.

For the values to be considered close, the difference between them
must be smaller than at least one of the tolerances.

-inf, inf and NaN behave similarly to the IEEE 754 Standard.  That
is, NaN is not close to anything, even itself.  inf and -inf are
only close to themselves.
[clinic start generated code]*/

static int
math_isclose_impl(PyObject *module, double a, double b, double rel_tol,
                  double abs_tol)
/*[clinic end generated code: output=b73070207511952d input=f28671871ea5bfba]*/
{
    double diff = 0.0;

    /* sanity check on the inputs */
    if (rel_tol < 0.0 || abs_tol < 0.0 ) {
        PyErr_SetString(PyExc_ValueError,
                        "tolerances must be non-negative");
        return -1;
    }

    if ( a == b ) {
        /* short circuit exact equality -- needed to catch two infinities of
           the same sign. And perhaps speeds things up a bit sometimes.
        */
        return 1;
    }

    /* This catches the case of two infinities of opposite sign, or
       one infinity and one finite number. Two infinities of opposite
       sign would otherwise have an infinite relative tolerance.
       Two infinities of the same sign are caught by the equality check
       above.
    */

    if (Py_IS_INFINITY(a) || Py_IS_INFINITY(b)) {
        return 0;
    }

    /* now do the regular computation
       this is essentially the "weak" test from the Boost library
    */

    diff = fabs(b - a);

    return (((diff <= fabs(rel_tol * b)) ||
             (diff <= fabs(rel_tol * a))) ||
            (diff <= abs_tol));
}

static inline int
_check_long_mult_overflow(long a, long b) {

    /* From Python2's int_mul code:

    Integer overflow checking for * is painful:  Python tried a couple ways, but
    they didn't work on all platforms, or failed in endcases (a product of
    -sys.maxint-1 has been a particular pain).

    Here's another way:

    The native long product x*y is either exactly right or *way* off, being
    just the last n bits of the true product, where n is the number of bits
    in a long (the delivered product is the true product plus i*2**n for
    some integer i).

    The native double product (double)x * (double)y is subject to three
    rounding errors:  on a sizeof(long)==8 box, each cast to double can lose
    info, and even on a sizeof(long)==4 box, the multiplication can lose info.
    But, unlike the native long product, it's not in *range* trouble:  even
    if sizeof(long)==32 (256-bit longs), the product easily fits in the
    dynamic range of a double.  So the leading 50 (or so) bits of the double
    product are correct.

    We check these two ways against each other, and declare victory if they're
    approximately the same.  Else, because the native long product is the only
    one that can lose catastrophic amounts of information, it's the native long
    product that must have overflowed.

    */

    long longprod = (long)((unsigned long)a * b);
    double doubleprod = (double)a * (double)b;
    double doubled_longprod = (double)longprod;

    if (doubled_longprod == doubleprod) {
        return 0;
    }

    const double diff = doubled_longprod - doubleprod;
    const double absdiff = diff >= 0.0 ? diff : -diff;
    const double absprod = doubleprod >= 0.0 ? doubleprod : -doubleprod;

    if (32.0 * absdiff <= absprod) {
        return 0;
    }

    return 1;
}

/*[clinic input]
math.prod

    iterable: object
    /
    *
    start: object(c_default="NULL") = 1

Calculate the product of all the elements in the input iterable.

The default start value for the product is 1.

When the iterable is empty, return the start value.  This function is
intended specifically for use with numeric values and may reject
non-numeric types.
[clinic start generated code]*/

static PyObject *
math_prod_impl(PyObject *module, PyObject *iterable, PyObject *start)
/*[clinic end generated code: output=36153bedac74a198 input=4c5ab0682782ed54]*/
{
    PyObject *result = start;
    PyObject *temp, *item, *iter;

    iter = PyObject_GetIter(iterable);
    if (iter == NULL) {
        return NULL;
    }

    if (result == NULL) {
        result = _PyLong_GetOne();
    }
    Py_INCREF(result);
#ifndef SLOW_PROD
    /* Fast paths for integers keeping temporary products in C.
     * Assumes all inputs are the same type.
     * If the assumption fails, default to use PyObjects instead.
    */
    if (PyLong_CheckExact(result)) {
        int overflow;
        long i_result = PyLong_AsLongAndOverflow(result, &overflow);
        /* If this already overflowed, don't even enter the loop. */
        if (overflow == 0) {
            Py_SETREF(result, NULL);
        }
        /* Loop over all the items in the iterable until we finish, we overflow
         * or we found a non integer element */
        while (result == NULL) {
            item = PyIter_Next(iter);
            if (item == NULL) {
                Py_DECREF(iter);
                if (PyErr_Occurred()) {
                    return NULL;
                }
                return PyLong_FromLong(i_result);
            }
            if (PyLong_CheckExact(item)) {
                long b = PyLong_AsLongAndOverflow(item, &overflow);
                if (overflow == 0 && !_check_long_mult_overflow(i_result, b)) {
                    long x = i_result * b;
                    i_result = x;
                    Py_DECREF(item);
                    continue;
                }
            }
            /* Either overflowed or is not an int.
             * Restore real objects and process normally */
            result = PyLong_FromLong(i_result);
            if (result == NULL) {
                Py_DECREF(item);
                Py_DECREF(iter);
                return NULL;
            }
            temp = PyNumber_Multiply(result, item);
            Py_DECREF(result);
            Py_DECREF(item);
            result = temp;
            if (result == NULL) {
                Py_DECREF(iter);
                return NULL;
            }
        }
    }

    /* Fast paths for floats keeping temporary products in C.
     * Assumes all inputs are the same type.
     * If the assumption fails, default to use PyObjects instead.
    */
    if (PyFloat_CheckExact(result)) {
        double f_result = PyFloat_AS_DOUBLE(result);
        Py_SETREF(result, NULL);
        while(result == NULL) {
            item = PyIter_Next(iter);
            if (item == NULL) {
                Py_DECREF(iter);
                if (PyErr_Occurred()) {
                    return NULL;
                }
                return PyFloat_FromDouble(f_result);
            }
            if (PyFloat_CheckExact(item)) {
                f_result *= PyFloat_AS_DOUBLE(item);
                Py_DECREF(item);
                continue;
            }
            if (PyLong_CheckExact(item)) {
                long value;
                int overflow;
                value = PyLong_AsLongAndOverflow(item, &overflow);
                if (!overflow) {
                    f_result *= (double)value;
                    Py_DECREF(item);
                    continue;
                }
            }
            result = PyFloat_FromDouble(f_result);
            if (result == NULL) {
                Py_DECREF(item);
                Py_DECREF(iter);
                return NULL;
            }
            temp = PyNumber_Multiply(result, item);
            Py_DECREF(result);
            Py_DECREF(item);
            result = temp;
            if (result == NULL) {
                Py_DECREF(iter);
                return NULL;
            }
        }
    }
#endif
    /* Consume rest of the iterable (if any) that could not be handled
     * by specialized functions above.*/
    for(;;) {
        item = PyIter_Next(iter);
        if (item == NULL) {
            /* error, or end-of-sequence */
            if (PyErr_Occurred()) {
                Py_SETREF(result, NULL);
            }
            break;
        }
        temp = PyNumber_Multiply(result, item);
        Py_DECREF(result);
        Py_DECREF(item);
        result = temp;
        if (result == NULL)
            break;
    }
    Py_DECREF(iter);
    return result;
}


/* least significant 64 bits of the odd part of factorial(n), for n in range(128).

Python code to generate the values:

    import math

    for n in range(128):
        fac = math.factorial(n)
        fac_odd_part = fac // (fac & -fac)
        reduced_fac_odd_part = fac_odd_part % (2**64)
        print(f"{reduced_fac_odd_part:#018x}u")
*/
static const uint64_t reduced_factorial_odd_part[] = {
    0x0000000000000001u, 0x0000000000000001u, 0x0000000000000001u, 0x0000000000000003u,
    0x0000000000000003u, 0x000000000000000fu, 0x000000000000002du, 0x000000000000013bu,
    0x000000000000013bu, 0x0000000000000b13u, 0x000000000000375fu, 0x0000000000026115u,
    0x000000000007233fu, 0x00000000005cca33u, 0x0000000002898765u, 0x00000000260eeeebu,
    0x00000000260eeeebu, 0x0000000286fddd9bu, 0x00000016beecca73u, 0x000001b02b930689u,
    0x00000870d9df20adu, 0x0000b141df4dae31u, 0x00079dd498567c1bu, 0x00af2e19afc5266du,
    0x020d8a4d0f4f7347u, 0x335281867ec241efu, 0x9b3093d46fdd5923u, 0x5e1f9767cc5866b1u,
    0x92dd23d6966aced7u, 0xa30d0f4f0a196e5bu, 0x8dc3e5a1977d7755u, 0x2ab8ce915831734bu,
    0x2ab8ce915831734bu, 0x81d2a0bc5e5fdcabu, 0x9efcac82445da75bu, 0xbc8b95cf58cde171u,
    0xa0e8444a1f3cecf9u, 0x4191deb683ce3ffdu, 0xddd3878bc84ebfc7u, 0xcb39a64b83ff3751u,
    0xf8203f7993fc1495u, 0xbd2a2a78b35f4bddu, 0x84757be6b6d13921u, 0x3fbbcfc0b524988bu,
    0xbd11ed47c8928df9u, 0x3c26b59e41c2f4c5u, 0x677a5137e883fdb3u, 0xff74e943b03b93ddu,
    0xfe5ebbcb10b2bb97u, 0xb021f1de3235e7e7u, 0x33509eb2e743a58fu, 0x390f9da41279fb7du,
    0xe5cb0154f031c559u, 0x93074695ba4ddb6du, 0x81c471caa636247fu, 0xe1347289b5a1d749u,
    0x286f21c3f76ce2ffu, 0x00be84a2173e8ac7u, 0x1595065ca215b88bu, 0xf95877595b018809u,
    0x9c2efe3c5516f887u, 0x373294604679382bu, 0xaf1ff7a888adcd35u, 0x18ddf279a2c5800bu,
    0x18ddf279a2c5800bu, 0x505a90e2542582cbu, 0x5bacad2cd8d5dc2bu, 0xfe3152bcbff89f41u,
    0xe1467e88bf829351u, 0xb8001adb9e31b4d5u, 0x2803ac06a0cbb91fu, 0x1904b5d698805799u,
    0xe12a648b5c831461u, 0x3516abbd6160cfa9u, 0xac46d25f12fe036du, 0x78bfa1da906b00efu,
    0xf6390338b7f111bdu, 0x0f25f80f538255d9u, 0x4ec8ca55b8db140fu, 0x4ff670740b9b30a1u,
    0x8fd032443a07f325u, 0x80dfe7965c83eeb5u, 0xa3dc1714d1213afdu, 0x205b7bbfcdc62007u,
    0xa78126bbe140a093u, 0x9de1dc61ca7550cfu, 0x84f0046d01b492c5u, 0x2d91810b945de0f3u,
    0xf5408b7f6008aa71u, 0x43707f4863034149u, 0xdac65fb9679279d5u, 0xc48406e7d1114eb7u,
    0xa7dc9ed3c88e1271u, 0xfb25b2efdb9cb30du, 0x1bebda0951c4df63u, 0x5c85e975580ee5bdu,
    0x1591bc60082cb137u, 0x2c38606318ef25d7u, 0x76ca72f7c5c63e27u, 0xf04a75d17baa0915u,
    0x77458175139ae30du, 0x0e6c1330bc1b9421u, 0xdf87d2b5797e8293u, 0xefa5c703e1e68925u,
    0x2b6b1b3278b4f6e1u, 0xceee27b382394249u, 0xd74e3829f5dab91du, 0xfdb17989c26b5f1fu,
    0xc1b7d18781530845u, 0x7b4436b2105a8561u, 0x7ba7c0418372a7d7u, 0x9dbc5c67feb6c639u,
    0x502686d7f6ff6b8fu, 0x6101855406be7a1fu, 0x9956afb5806930e7u, 0xe1f0ee88af40f7c5u,
    0x984b057bda5c1151u, 0x9a49819acc13ea05u, 0x8ef0dead0896ef27u, 0x71f7826efe292b21u,
    0xad80a480e46986efu, 0x01cdc0ebf5e0c6f7u, 0x6e06f839968f68dbu, 0xdd5943ab56e76139u,
    0xcdcf31bf8604c5e7u, 0x7e2b4a847054a1cbu, 0x0ca75697a4d3d0f5u, 0x4703f53ac514a98bu,
};

/* inverses of reduced_factorial_odd_part values modulo 2**64.

Python code to generate the values:

    import math

    for n in range(128):
        fac = math.factorial(n)
        fac_odd_part = fac // (fac & -fac)
        inverted_fac_odd_part = pow(fac_odd_part, -1, 2**64)
        print(f"{inverted_fac_odd_part:#018x}u")
*/
static const uint64_t inverted_factorial_odd_part[] = {
    0x0000000000000001u, 0x0000000000000001u, 0x0000000000000001u, 0xaaaaaaaaaaaaaaabu,
    0xaaaaaaaaaaaaaaabu, 0xeeeeeeeeeeeeeeefu, 0x4fa4fa4fa4fa4fa5u, 0x2ff2ff2ff2ff2ff3u,
    0x2ff2ff2ff2ff2ff3u, 0x938cc70553e3771bu, 0xb71c27cddd93e49fu, 0xb38e3229fcdee63du,
    0xe684bb63544a4cbfu, 0xc2f684917ca340fbu, 0xf747c9cba417526du, 0xbb26eb51d7bd49c3u,
    0xbb26eb51d7bd49c3u, 0xb0a7efb985294093u, 0xbe4b8c69f259eabbu, 0x6854d17ed6dc4fb9u,
    0xe1aa904c915f4325u, 0x3b8206df131cead1u, 0x79c6009fea76fe13u, 0xd8c5d381633cd365u,
    0x4841f12b21144677u, 0x4a91ff68200b0d0fu, 0x8f9513a58c4f9e8bu, 0x2b3e690621a42251u,
    0x4f520f00e03c04e7u, 0x2edf84ee600211d3u, 0xadcaa2764aaacdfdu, 0x161f4f9033f4fe63u,
    0x161f4f9033f4fe63u, 0xbada2932ea4d3e03u, 0xcec189f3efaa30d3u, 0xf7475bb68330bf91u,
    0x37eb7bf7d5b01549u, 0x46b35660a4e91555u, 0xa567c12d81f151f7u, 0x4c724007bb2071b1u,
    0x0f4a0cce58a016bdu, 0xfa21068e66106475u, 0x244ab72b5a318ae1u, 0x366ce67e080d0f23u,
    0xd666fdae5dd2a449u, 0xd740ddd0acc06a0du, 0xb050bbbb28e6f97bu, 0x70b003fe890a5c75u,
    0xd03aabff83037427u, 0x13ec4ca72c783bd7u, 0x90282c06afdbd96fu, 0x4414ddb9db4a95d5u,
    0xa2c68735ae6832e9u, 0xbf72d71455676665u, 0xa8469fab6b759b7fu, 0xc1e55b56e606caf9u,
    0x40455630fc4a1cffu, 0x0120a7b0046d16f7u, 0xa7c3553b08faef23u, 0x9f0bfd1b08d48639u,
    0xa433ffce9a304d37u, 0xa22ad1d53915c683u, 0xcb6cbc723ba5dd1du, 0x547fb1b8ab9d0ba3u,
    0x547fb1b8ab9d0ba3u, 0x8f15a826498852e3u, 0x32e1a03f38880283u, 0x3de4cce63283f0c1u,
    0x5dfe6667e4da95b1u, 0xfda6eeeef479e47du, 0xf14de991cc7882dfu, 0xe68db79247630ca9u,
    0xa7d6db8207ee8fa1u, 0x255e1f0fcf034499u, 0xc9a8990e43dd7e65u, 0x3279b6f289702e0fu,
    0xe7b5905d9b71b195u, 0x03025ba41ff0da69u, 0xb7df3d6d3be55aefu, 0xf89b212ebff2b361u,
    0xfe856d095996f0adu, 0xd6e533e9fdf20f9du, 0xf8c0e84a63da3255u, 0xa677876cd91b4db7u,
    0x07ed4f97780d7d9bu, 0x90a8705f258db62fu, 0xa41bbb2be31b1c0du, 0x6ec28690b038383bu,
    0xdb860c3bb2edd691u, 0x0838286838a980f9u, 0x558417a74b36f77du, 0x71779afc3646ef07u,
    0x743cda377ccb6e91u, 0x7fdf9f3fe89153c5u, 0xdc97d25df49b9a4bu, 0x76321a778eb37d95u,
    0x7cbb5e27da3bd487u, 0x9cff4ade1a009de7u, 0x70eb166d05c15197u, 0xdcf0460b71d5fe3du,
    0x5ac1ee5260b6a3c5u, 0xc922dedfdd78efe1u, 0xe5d381dc3b8eeb9bu, 0xd57e5347bafc6aadu,
    0x86939040983acd21u, 0x395b9d69740a4ff9u, 0x1467299c8e43d135u, 0x5fe440fcad975cdfu,
    0xcaa9a39794a6ca8du, 0xf61dbd640868dea1u, 0xac09d98d74843be7u, 0x2b103b9e1a6b4809u,
    0x2ab92d16960f536fu, 0x6653323d5e3681dfu, 0xefd48c1c0624e2d7u, 0xa496fefe04816f0du,
    0x1754a7b07bbdd7b1u, 0x23353c829a3852cdu, 0xbf831261abd59097u, 0x57a8e656df0618e1u,
    0x16e9206c3100680fu, 0xadad4c6ee921dac7u, 0x635f2b3860265353u, 0xdd6d0059f44b3d09u,
    0xac4dd6b894447dd7u, 0x42ea183eeaa87be3u, 0x15612d1550ee5b5du, 0x226fa19d656cb623u,
};

/* exponent of the largest power of 2 dividing factorial(n), for n in range(68)

Python code to generate the values:

import math

for n in range(128):
    fac = math.factorial(n)
    fac_trailing_zeros = (fac & -fac).bit_length() - 1
    print(fac_trailing_zeros)
*/

static const uint8_t factorial_trailing_zeros[] = {
     0,  0,  1,  1,  3,  3,  4,  4,  7,  7,  8,  8, 10, 10, 11, 11,  //  0-15
    15, 15, 16, 16, 18, 18, 19, 19, 22, 22, 23, 23, 25, 25, 26, 26,  // 16-31
    31, 31, 32, 32, 34, 34, 35, 35, 38, 38, 39, 39, 41, 41, 42, 42,  // 32-47
    46, 46, 47, 47, 49, 49, 50, 50, 53, 53, 54, 54, 56, 56, 57, 57,  // 48-63
    63, 63, 64, 64, 66, 66, 67, 67, 70, 70, 71, 71, 73, 73, 74, 74,  // 64-79
    78, 78, 79, 79, 81, 81, 82, 82, 85, 85, 86, 86, 88, 88, 89, 89,  // 80-95
    94, 94, 95, 95, 97, 97, 98, 98, 101, 101, 102, 102, 104, 104, 105, 105,  // 96-111
    109, 109, 110, 110, 112, 112, 113, 113, 116, 116, 117, 117, 119, 119, 120, 120,  // 112-127
};

/* Number of permutations and combinations.
 * P(n, k) = n! / (n-k)!
 * C(n, k) = P(n, k) / k!
 */

/* Calculate C(n, k) for n in the 63-bit range. */
static PyObject *
perm_comb_small(unsigned long long n, unsigned long long k, int iscomb)
{
    assert(k != 0);

    /* For small enough n and k the result fits in the 64-bit range and can
     * be calculated without allocating intermediate PyLong objects. */
    if (iscomb) {
        /* Maps k to the maximal n so that 2*k-1 <= n <= 127 and C(n, k)
         * fits into a uint64_t.  Exclude k = 1, because the second fast
         * path is faster for this case.*/
        static const unsigned char fast_comb_limits1[] = {
            0, 0, 127, 127, 127, 127, 127, 127,  // 0-7
            127, 127, 127, 127, 127, 127, 127, 127,  // 8-15
            116, 105, 97, 91, 86, 82, 78, 76,  // 16-23
            74, 72, 71, 70, 69, 68, 68, 67,  // 24-31
            67, 67, 67,  // 32-34
        };
        if (k < Py_ARRAY_LENGTH(fast_comb_limits1) && n <= fast_comb_limits1[k]) {
            /*
                comb(n, k) fits into a uint64_t. We compute it as

                    comb_odd_part << shift

                where 2**shift is the largest power of two dividing comb(n, k)
                and comb_odd_part is comb(n, k) >> shift. comb_odd_part can be
                calculated efficiently via arithmetic modulo 2**64, using three
                lookups and two uint64_t multiplications.
            */
            uint64_t comb_odd_part = reduced_factorial_odd_part[n]
                                   * inverted_factorial_odd_part[k]
                                   * inverted_factorial_odd_part[n - k];
            int shift = factorial_trailing_zeros[n]
                      - factorial_trailing_zeros[k]
                      - factorial_trailing_zeros[n - k];
            return PyLong_FromUnsignedLongLong(comb_odd_part << shift);
        }

        /* Maps k to the maximal n so that 2*k-1 <= n <= 127 and C(n, k)*k
         * fits into a long long (which is at least 64 bit).  Only contains
         * items larger than in fast_comb_limits1. */
        static const unsigned long long fast_comb_limits2[] = {
            0, ULLONG_MAX, 4294967296ULL, 3329022, 102570, 13467, 3612, 1449,  // 0-7
            746, 453, 308, 227, 178, 147,  // 8-13
        };
        if (k < Py_ARRAY_LENGTH(fast_comb_limits2) && n <= fast_comb_limits2[k]) {
            /* C(n, k) = C(n, k-1) * (n-k+1) / k */
            unsigned long long result = n;
            for (unsigned long long i = 1; i < k;) {
                result *= --n;
                result /= ++i;
            }
            return PyLong_FromUnsignedLongLong(result);
        }
    }
    else {
        /* Maps k to the maximal n so that k <= n and P(n, k)
         * fits into a long long (which is at least 64 bit). */
        static const unsigned long long fast_perm_limits[] = {
            0, ULLONG_MAX, 4294967296ULL, 2642246, 65537, 7133, 1627, 568,  // 0-7
            259, 142, 88, 61, 45, 36, 30, 26,  // 8-15
            24, 22, 21, 20, 20,  // 16-20
        };
        if (k < Py_ARRAY_LENGTH(fast_perm_limits) && n <= fast_perm_limits[k]) {
            if (n <= 127) {
                /* P(n, k) fits into a uint64_t. */
                uint64_t perm_odd_part = reduced_factorial_odd_part[n]
                                       * inverted_factorial_odd_part[n - k];
                int shift = factorial_trailing_zeros[n]
                          - factorial_trailing_zeros[n - k];
                return PyLong_FromUnsignedLongLong(perm_odd_part << shift);
            }

            /* P(n, k) = P(n, k-1) * (n-k+1) */
            unsigned long long result = n;
            for (unsigned long long i = 1; i < k;) {
                result *= --n;
                ++i;
            }
            return PyLong_FromUnsignedLongLong(result);
        }
    }

    /* For larger n use recursive formulas:
     *
     *   P(n, k) = P(n, j) * P(n-j, k-j)
     *   C(n, k) = C(n, j) * C(n-j, k-j) // C(k, j)
     */
    unsigned long long j = k / 2;
    PyObject *a, *b;
    a = perm_comb_small(n, j, iscomb);
    if (a == NULL) {
        return NULL;
    }
    b = perm_comb_small(n - j, k - j, iscomb);
    if (b == NULL) {
        goto error;
    }
    Py_SETREF(a, PyNumber_Multiply(a, b));
    Py_DECREF(b);
    if (iscomb && a != NULL) {
        b = perm_comb_small(k, j, 1);
        if (b == NULL) {
            goto error;
        }
        Py_SETREF(a, PyNumber_FloorDivide(a, b));
        Py_DECREF(b);
    }
    return a;

error:
    Py_DECREF(a);
    return NULL;
}

/* Calculate P(n, k) or C(n, k) using recursive formulas.
 * It is more efficient than sequential multiplication thanks to
 * Karatsuba multiplication.
 */
static PyObject *
perm_comb(PyObject *n, unsigned long long k, int iscomb)
{
    if (k == 0) {
        return PyLong_FromLong(1);
    }
    if (k == 1) {
        return Py_NewRef(n);
    }

    /* P(n, k) = P(n, j) * P(n-j, k-j) */
    /* C(n, k) = C(n, j) * C(n-j, k-j) // C(k, j) */
    unsigned long long j = k / 2;
    PyObject *a, *b;
    a = perm_comb(n, j, iscomb);
    if (a == NULL) {
        return NULL;
    }
    PyObject *t = PyLong_FromUnsignedLongLong(j);
    if (t == NULL) {
        goto error;
    }
    n = PyNumber_Subtract(n, t);
    Py_DECREF(t);
    if (n == NULL) {
        goto error;
    }
    b = perm_comb(n, k - j, iscomb);
    Py_DECREF(n);
    if (b == NULL) {
        goto error;
    }
    Py_SETREF(a, PyNumber_Multiply(a, b));
    Py_DECREF(b);
    if (iscomb && a != NULL) {
        b = perm_comb_small(k, j, 1);
        if (b == NULL) {
            goto error;
        }
        Py_SETREF(a, PyNumber_FloorDivide(a, b));
        Py_DECREF(b);
    }
    return a;

error:
    Py_DECREF(a);
    return NULL;
}

/*[clinic input]
math.perm

    n: object
    k: object = None
    /

Number of ways to choose k items from n items without repetition and with order.

Evaluates to n! / (n - k)! when k <= n and evaluates
to zero when k > n.

If k is not specified or is None, then k defaults to n
and the function returns n!.

Raises TypeError if either of the arguments are not integers.
Raises ValueError if either of the arguments are negative.
[clinic start generated code]*/

static PyObject *
math_perm_impl(PyObject *module, PyObject *n, PyObject *k)
/*[clinic end generated code: output=e021a25469653e23 input=5311c5a00f359b53]*/
{
    PyObject *result = NULL;
    int overflow, cmp;
    long long ki, ni;

    if (k == Py_None) {
        return math_factorial(module, n);
    }
    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }
    k = PyNumber_Index(k);
    if (k == NULL) {
        Py_DECREF(n);
        return NULL;
    }
    assert(PyLong_CheckExact(n) && PyLong_CheckExact(k));

    if (_PyLong_IsNegative((PyLongObject *)n)) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a non-negative integer");
        goto error;
    }
    if (_PyLong_IsNegative((PyLongObject *)k)) {
        PyErr_SetString(PyExc_ValueError,
                        "k must be a non-negative integer");
        goto error;
    }

    cmp = PyObject_RichCompareBool(n, k, Py_LT);
    if (cmp != 0) {
        if (cmp > 0) {
            result = PyLong_FromLong(0);
            goto done;
        }
        goto error;
    }

    ki = PyLong_AsLongLongAndOverflow(k, &overflow);
    assert(overflow >= 0 && !PyErr_Occurred());
    if (overflow > 0) {
        PyErr_Format(PyExc_OverflowError,
                     "k must not exceed %lld",
                     LLONG_MAX);
        goto error;
    }
    assert(ki >= 0);

    ni = PyLong_AsLongLongAndOverflow(n, &overflow);
    assert(overflow >= 0 && !PyErr_Occurred());
    if (!overflow && ki > 1) {
        assert(ni >= 0);
        result = perm_comb_small((unsigned long long)ni,
                                 (unsigned long long)ki, 0);
    }
    else {
        result = perm_comb(n, (unsigned long long)ki, 0);
    }

done:
    Py_DECREF(n);
    Py_DECREF(k);
    return result;

error:
    Py_DECREF(n);
    Py_DECREF(k);
    return NULL;
}

/*[clinic input]
math.comb

    n: object
    k: object
    /

Number of ways to choose k items from n items without repetition and without order.

Evaluates to n! / (k! * (n - k)!) when k <= n and evaluates
to zero when k > n.

Also called the binomial coefficient because it is equivalent
to the coefficient of k-th term in polynomial expansion of the
expression (1 + x)**n.

Raises TypeError if either of the arguments are not integers.
Raises ValueError if either of the arguments are negative.

[clinic start generated code]*/

static PyObject *
math_comb_impl(PyObject *module, PyObject *n, PyObject *k)
/*[clinic end generated code: output=bd2cec8d854f3493 input=9a05315af2518709]*/
{
    PyObject *result = NULL, *temp;
    int overflow, cmp;
    long long ki, ni;

    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }
    k = PyNumber_Index(k);
    if (k == NULL) {
        Py_DECREF(n);
        return NULL;
    }
    assert(PyLong_CheckExact(n) && PyLong_CheckExact(k));

    if (_PyLong_IsNegative((PyLongObject *)n)) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a non-negative integer");
        goto error;
    }
    if (_PyLong_IsNegative((PyLongObject *)k)) {
        PyErr_SetString(PyExc_ValueError,
                        "k must be a non-negative integer");
        goto error;
    }

    ni = PyLong_AsLongLongAndOverflow(n, &overflow);
    assert(overflow >= 0 && !PyErr_Occurred());
    if (!overflow) {
        assert(ni >= 0);
        ki = PyLong_AsLongLongAndOverflow(k, &overflow);
        assert(overflow >= 0 && !PyErr_Occurred());
        if (overflow || ki > ni) {
            result = PyLong_FromLong(0);
            goto done;
        }
        assert(ki >= 0);

        ki = Py_MIN(ki, ni - ki);
        if (ki > 1) {
            result = perm_comb_small((unsigned long long)ni,
                                     (unsigned long long)ki, 1);
            goto done;
        }
        /* For k == 1 just return the original n in perm_comb(). */
    }
    else {
        /* k = min(k, n - k) */
        temp = PyNumber_Subtract(n, k);
        if (temp == NULL) {
            goto error;
        }
        assert(PyLong_Check(temp));
        if (_PyLong_IsNegative((PyLongObject *)temp)) {
            Py_DECREF(temp);
            result = PyLong_FromLong(0);
            goto done;
        }
        cmp = PyObject_RichCompareBool(temp, k, Py_LT);
        if (cmp > 0) {
            Py_SETREF(k, temp);
        }
        else {
            Py_DECREF(temp);
            if (cmp < 0) {
                goto error;
            }
        }

        ki = PyLong_AsLongLongAndOverflow(k, &overflow);
        assert(overflow >= 0 && !PyErr_Occurred());
        if (overflow) {
            PyErr_Format(PyExc_OverflowError,
                         "min(n - k, k) must not exceed %lld",
                         LLONG_MAX);
            goto error;
        }
        assert(ki >= 0);
    }

    result = perm_comb(n, (unsigned long long)ki, 1);

done:
    Py_DECREF(n);
    Py_DECREF(k);
    return result;

error:
    Py_DECREF(n);
    Py_DECREF(k);
    return NULL;
}


/*[clinic input]
math.nextafter

    x: double
    y: double
    /
    *
    steps: object = None

Return the floating-point value the given number of steps after x towards y.

If steps is not specified or is None, it defaults to 1.

Raises a TypeError, if x or y is not a double, or if steps is not an integer.
Raises ValueError if steps is negative.
[clinic start generated code]*/

static PyObject *
math_nextafter_impl(PyObject *module, double x, double y, PyObject *steps)
/*[clinic end generated code: output=cc6511f02afc099e input=7f2a5842112af2b4]*/
{
#if defined(_AIX)
    if (x == y) {
        /* On AIX 7.1, libm nextafter(-0.0, +0.0) returns -0.0.
           Bug fixed in bos.adt.libm 7.2.2.0 by APAR IV95512. */
        return PyFloat_FromDouble(y);
    }
    if (Py_IS_NAN(x)) {
        return PyFloat_FromDouble(x);
    }
    if (Py_IS_NAN(y)) {
        return PyFloat_FromDouble(y);
    }
#endif
    if (steps == Py_None) {
        // fast path: we default to one step.
        return PyFloat_FromDouble(nextafter(x, y));
    }
    steps = PyNumber_Index(steps);
    if (steps == NULL) {
        return NULL;
    }
    assert(PyLong_CheckExact(steps));
    if (_PyLong_IsNegative((PyLongObject *)steps)) {
        PyErr_SetString(PyExc_ValueError,
                        "steps must be a non-negative integer");
        Py_DECREF(steps);
        return NULL;
    }

    unsigned long long usteps_ull = PyLong_AsUnsignedLongLong(steps);
    // Conveniently, uint64_t and double have the same number of bits
    // on all the platforms we care about.
    // So if an overflow occurs, we can just use UINT64_MAX.
    Py_DECREF(steps);
    if (usteps_ull >= UINT64_MAX) {
        // This branch includes the case where an error occurred, since
        // (unsigned long long)(-1) = ULLONG_MAX >= UINT64_MAX. Note that
        // usteps_ull can be strictly larger than UINT64_MAX on a machine
        // where unsigned long long has width > 64 bits.
        if (PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                PyErr_Clear();
            }
            else {
                return NULL;
            }
        }
        usteps_ull = UINT64_MAX;
    }
    assert(usteps_ull <= UINT64_MAX);
    uint64_t usteps = (uint64_t)usteps_ull;

    if (usteps == 0) {
        return PyFloat_FromDouble(x);
    }
    if (Py_IS_NAN(x)) {
        return PyFloat_FromDouble(x);
    }
    if (Py_IS_NAN(y)) {
        return PyFloat_FromDouble(y);
    }

    // We assume that double and uint64_t have the same endianness.
    // This is not guaranteed by the C-standard, but it is true for
    // all platforms we care about. (The most likely form of violation
    // would be a "mixed-endian" double.)
    union pun {double f; uint64_t i;};
    union pun ux = {x}, uy = {y};
    if (ux.i == uy.i) {
        return PyFloat_FromDouble(x);
    }

    const uint64_t sign_bit = 1ULL<<63;

    uint64_t ax = ux.i & ~sign_bit;
    uint64_t ay = uy.i & ~sign_bit;

    // opposite signs
    if (((ux.i ^ uy.i) & sign_bit)) {
        // NOTE: ax + ay can never overflow, because their most significant bit
        // ain't set.
        if (ax + ay <= usteps) {
            return PyFloat_FromDouble(uy.f);
        // This comparison has to use <, because <= would get +0.0 vs -0.0
        // wrong.
        } else if (ax < usteps) {
            union pun result = {.i = (uy.i & sign_bit) | (usteps - ax)};
            return PyFloat_FromDouble(result.f);
        } else {
            ux.i -= usteps;
            return PyFloat_FromDouble(ux.f);
        }
    // same sign
    } else if (ax > ay) {
        if (ax - ay >= usteps) {
            ux.i -= usteps;
            return PyFloat_FromDouble(ux.f);
        } else {
            return PyFloat_FromDouble(uy.f);
        }
    } else {
        if (ay - ax >= usteps) {
            ux.i += usteps;
            return PyFloat_FromDouble(ux.f);
        } else {
            return PyFloat_FromDouble(uy.f);
        }
    }
}


/*[clinic input]
math.ulp -> double

    x: double
    /

Return the value of the least significant bit of the float x.
[clinic start generated code]*/

static double
math_ulp_impl(PyObject *module, double x)
/*[clinic end generated code: output=f5207867a9384dd4 input=31f9bfbbe373fcaa]*/
{
    if (Py_IS_NAN(x)) {
        return x;
    }
    x = fabs(x);
    if (Py_IS_INFINITY(x)) {
        return x;
    }
    double inf = Py_INFINITY;
    double x2 = nextafter(x, inf);
    if (Py_IS_INFINITY(x2)) {
        /* special case: x is the largest positive representable float */
        x2 = nextafter(x, -inf);
        return x - x2;
    }
    return x2 - x;
}

static int
math_exec(PyObject *module)
{

    math_module_state *state = get_math_module_state(module);
    state->str___ceil__ = PyUnicode_InternFromString("__ceil__");
    if (state->str___ceil__ == NULL) {
        return -1;
    }
    state->str___floor__ = PyUnicode_InternFromString("__floor__");
    if (state->str___floor__ == NULL) {
        return -1;
    }
    state->str___trunc__ = PyUnicode_InternFromString("__trunc__");
    if (state->str___trunc__ == NULL) {
        return -1;
    }
    if (PyModule_Add(module, "pi", PyFloat_FromDouble(Py_MATH_PI)) < 0) {
        return -1;
    }
    if (PyModule_Add(module, "e", PyFloat_FromDouble(Py_MATH_E)) < 0) {
        return -1;
    }
    // 2pi
    if (PyModule_Add(module, "tau", PyFloat_FromDouble(Py_MATH_TAU)) < 0) {
        return -1;
    }
    if (PyModule_Add(module, "inf", PyFloat_FromDouble(Py_INFINITY)) < 0) {
        return -1;
    }
    if (PyModule_Add(module, "nan", PyFloat_FromDouble(fabs(Py_NAN))) < 0) {
        return -1;
    }
    return 0;
}

static int
math_clear(PyObject *module)
{
    math_module_state *state = get_math_module_state(module);
    Py_CLEAR(state->str___ceil__);
    Py_CLEAR(state->str___floor__);
    Py_CLEAR(state->str___trunc__);
    return 0;
}

static void
math_free(void *module)
{
    math_clear((PyObject *)module);
}

static PyMethodDef math_methods[] = {
    {"acos",            math_acos,      METH_O,         math_acos_doc},
    {"acosh",           math_acosh,     METH_O,         math_acosh_doc},
    {"asin",            math_asin,      METH_O,         math_asin_doc},
    {"asinh",           math_asinh,     METH_O,         math_asinh_doc},
    {"atan",            math_atan,      METH_O,         math_atan_doc},
    {"atan2",           _PyCFunction_CAST(math_atan2),     METH_FASTCALL,  math_atan2_doc},
    {"atanh",           math_atanh,     METH_O,         math_atanh_doc},
    {"cbrt",            math_cbrt,      METH_O,         math_cbrt_doc},
    MATH_CEIL_METHODDEF
    {"copysign",        _PyCFunction_CAST(math_copysign),  METH_FASTCALL,  math_copysign_doc},
    {"cos",             math_cos,       METH_O,         math_cos_doc},
    {"cosh",            math_cosh,      METH_O,         math_cosh_doc},
    MATH_DEGREES_METHODDEF
    MATH_DIST_METHODDEF
    {"erf",             math_erf,       METH_O,         math_erf_doc},
    {"erfc",            math_erfc,      METH_O,         math_erfc_doc},
    {"exp",             math_exp,       METH_O,         math_exp_doc},
    {"exp2",            math_exp2,      METH_O,         math_exp2_doc},
    {"expm1",           math_expm1,     METH_O,         math_expm1_doc},
    {"fabs",            math_fabs,      METH_O,         math_fabs_doc},
    MATH_FACTORIAL_METHODDEF
    MATH_FLOOR_METHODDEF
    MATH_FMOD_METHODDEF
    MATH_FREXP_METHODDEF
    MATH_FSUM_METHODDEF
    {"gamma",           math_gamma,     METH_O,         math_gamma_doc},
    {"gcd",             _PyCFunction_CAST(math_gcd),       METH_FASTCALL,  math_gcd_doc},
    {"hypot",           _PyCFunction_CAST(math_hypot),     METH_FASTCALL,  math_hypot_doc},
    MATH_ISCLOSE_METHODDEF
    MATH_ISFINITE_METHODDEF
    MATH_ISINF_METHODDEF
    MATH_ISNAN_METHODDEF
    MATH_ISQRT_METHODDEF
    {"lcm",             _PyCFunction_CAST(math_lcm),       METH_FASTCALL,  math_lcm_doc},
    MATH_LDEXP_METHODDEF
    {"lgamma",          math_lgamma,    METH_O,         math_lgamma_doc},
    {"log",             _PyCFunction_CAST(math_log),       METH_FASTCALL,  math_log_doc},
    {"log1p",           math_log1p,     METH_O,         math_log1p_doc},
    MATH_LOG10_METHODDEF
    MATH_LOG2_METHODDEF
    MATH_MODF_METHODDEF
    MATH_POW_METHODDEF
    MATH_RADIANS_METHODDEF
    {"remainder",       _PyCFunction_CAST(math_remainder), METH_FASTCALL,  math_remainder_doc},
    {"sin",             math_sin,       METH_O,         math_sin_doc},
    {"sinh",            math_sinh,      METH_O,         math_sinh_doc},
    {"sqrt",            math_sqrt,      METH_O,         math_sqrt_doc},
    {"tan",             math_tan,       METH_O,         math_tan_doc},
    {"tanh",            math_tanh,      METH_O,         math_tanh_doc},
    MATH_SUMPROD_METHODDEF
    MATH_TRUNC_METHODDEF
    MATH_PROD_METHODDEF
    MATH_PERM_METHODDEF
    MATH_COMB_METHODDEF
    MATH_NEXTAFTER_METHODDEF
    MATH_ULP_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static PyModuleDef_Slot math_slots[] = {
    {Py_mod_exec, math_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

PyDoc_STRVAR(module_doc,
"This module provides access to the mathematical functions\n"
"defined by the C standard.");

static struct PyModuleDef mathmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "math",
    .m_doc = module_doc,
    .m_size = sizeof(math_module_state),
    .m_methods = math_methods,
    .m_slots = math_slots,
    .m_clear = math_clear,
    .m_free = math_free,
};

PyMODINIT_FUNC
PyInit_math(void)
{
    return PyModuleDef_Init(&mathmodule);
}
