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

#include "Python.h"
#include "_math.h"

#include "clinic/mathmodule.c.h"

/*[clinic input]
module math
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=76bc7002685dd942]*/


/*
   sin(pi*x), giving accurate results for all finite x (especially x
   integral or close to an integer).  This is here for use in the
   reflection formula for the gamma function.  It conforms to IEEE
   754-2008 for finite arguments, but not for infinities or nans.
*/

static const double pi = 3.141592653589793238462643383279502884197;
static const double logpi = 1.144729885849400174143427351353058711647;
#if !defined(HAVE_ERF) || !defined(HAVE_ERFC)
static const double sqrtpi = 1.772453850905516027298167483341145182798;
#endif /* !defined(HAVE_ERF) || !defined(HAVE_ERFC) */


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

/* Implementation of the real gamma function.  In extensive but non-exhaustive
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

/* Constant for +infinity, generated in the same way as float('inf'). */

static double
m_inf(void)
{
#ifndef PY_NO_SHORT_FLOAT_REPR
    return _Py_dg_infinity(0);
#else
    return Py_HUGE_VAL;
#endif
}

/* Constant nan value, generated in the same way as float('nan'). */
/* We don't currently assume that Py_NAN is defined everywhere. */

#if !defined(PY_NO_SHORT_FLOAT_REPR) || defined(Py_NAN)

static double
m_nan(void)
{
#ifndef PY_NO_SHORT_FLOAT_REPR
    return _Py_dg_stdnan(0);
#else
    return Py_NAN;
#endif
}

#endif

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
        return copysign(Py_HUGE_VAL, x);
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

#if !defined(HAVE_ERF) || !defined(HAVE_ERFC)

/*
   Implementations of the error function erf(x) and the complementary error
   function erfc(x).

   Method: we use a series approximation for erf for small x, and a continued
   fraction approximation for erfc(x) for larger x;
   combined with the relations erf(-x) = -erf(x) and erfc(x) = 1.0 - erf(x),
   this gives us erf(x) and erfc(x) for all x.

   The series expansion used is:

      erf(x) = x*exp(-x*x)/sqrt(pi) * [
                     2/1 + 4/3 x**2 + 8/15 x**4 + 16/105 x**6 + ...]

   The coefficient of x**(2k-2) here is 4**k*factorial(k)/factorial(2*k).
   This series converges well for smallish x, but slowly for larger x.

   The continued fraction expansion used is:

      erfc(x) = x*exp(-x*x)/sqrt(pi) * [1/(0.5 + x**2 -) 0.5/(2.5 + x**2 - )
                              3.0/(4.5 + x**2 - ) 7.5/(6.5 + x**2 - ) ...]

   after the first term, the general term has the form:

      k*(k-0.5)/(2*k+0.5 + x**2 - ...).

   This expansion converges fast for larger x, but convergence becomes
   infinitely slow as x approaches 0.0.  The (somewhat naive) continued
   fraction evaluation algorithm used below also risks overflow for large x;
   but for large x, erfc(x) == 0.0 to within machine precision.  (For
   example, erfc(30.0) is approximately 2.56e-393).

   Parameters: use series expansion for abs(x) < ERF_SERIES_CUTOFF and
   continued fraction expansion for ERF_SERIES_CUTOFF <= abs(x) <
   ERFC_CONTFRAC_CUTOFF.  ERFC_SERIES_TERMS and ERFC_CONTFRAC_TERMS are the
   numbers of terms to use for the relevant expansions.  */

#define ERF_SERIES_CUTOFF 1.5
#define ERF_SERIES_TERMS 25
#define ERFC_CONTFRAC_CUTOFF 30.0
#define ERFC_CONTFRAC_TERMS 50

/*
   Error function, via power series.

   Given a finite float x, return an approximation to erf(x).
   Converges reasonably fast for small x.
*/

static double
m_erf_series(double x)
{
    double x2, acc, fk, result;
    int i, saved_errno;

    x2 = x * x;
    acc = 0.0;
    fk = (double)ERF_SERIES_TERMS + 0.5;
    for (i = 0; i < ERF_SERIES_TERMS; i++) {
        acc = 2.0 + x2 * acc / fk;
        fk -= 1.0;
    }
    /* Make sure the exp call doesn't affect errno;
       see m_erfc_contfrac for more. */
    saved_errno = errno;
    result = acc * x * exp(-x2) / sqrtpi;
    errno = saved_errno;
    return result;
}

/*
   Complementary error function, via continued fraction expansion.

   Given a positive float x, return an approximation to erfc(x).  Converges
   reasonably fast for x large (say, x > 2.0), and should be safe from
   overflow if x and nterms are not too large.  On an IEEE 754 machine, with x
   <= 30.0, we're safe up to nterms = 100.  For x >= 30.0, erfc(x) is smaller
   than the smallest representable nonzero float.  */

static double
m_erfc_contfrac(double x)
{
    double x2, a, da, p, p_last, q, q_last, b, result;
    int i, saved_errno;

    if (x >= ERFC_CONTFRAC_CUTOFF)
        return 0.0;

    x2 = x*x;
    a = 0.0;
    da = 0.5;
    p = 1.0; p_last = 0.0;
    q = da + x2; q_last = 1.0;
    for (i = 0; i < ERFC_CONTFRAC_TERMS; i++) {
        double temp;
        a += da;
        da += 2.0;
        b = da + x2;
        temp = p; p = b*p - a*p_last; p_last = temp;
        temp = q; q = b*q - a*q_last; q_last = temp;
    }
    /* Issue #8986: On some platforms, exp sets errno on underflow to zero;
       save the current errno value so that we can restore it later. */
    saved_errno = errno;
    result = p / q * x * exp(-x2) / sqrtpi;
    errno = saved_errno;
    return result;
}

#endif /* !defined(HAVE_ERF) || !defined(HAVE_ERFC) */

/* Error function erf(x), for general x */

static double
m_erf(double x)
{
#ifdef HAVE_ERF
    return erf(x);
#else
    double absx, cf;

    if (Py_IS_NAN(x))
        return x;
    absx = fabs(x);
    if (absx < ERF_SERIES_CUTOFF)
        return m_erf_series(x);
    else {
        cf = m_erfc_contfrac(absx);
        return x > 0.0 ? 1.0 - cf : cf - 1.0;
    }
#endif
}

/* Complementary error function erfc(x), for general x. */

static double
m_erfc(double x)
{
#ifdef HAVE_ERFC
    return erfc(x);
#else
    double absx, cf;

    if (Py_IS_NAN(x))
        return x;
    absx = fabs(x);
    if (absx < ERF_SERIES_CUTOFF)
        return 1.0 - m_erf_series(x);
    else {
        cf = m_erfc_contfrac(absx);
        return x > 0.0 ? cf : 2.0 - cf;
    }
#endif
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
#ifdef HAVE_LOG2
        return log2(x);
#else
        double m;
        int e;
        m = frexp(x, &e);
        /* We want log2(m * 2**e) == log(m) / log(2) + e.  Care is needed when
         * x is just greater than 1.0: in that case e is 1, log(m) is negative,
         * and we get significant cancellation error from the addition of
         * log(m) / log(2) to e.  The slight rewrite of the expression below
         * avoids this problem.
         */
        if (x >= 1.0) {
            return log(2.0 * m) / log(2.0) + (e - 1);
        }
        else {
            return log(m) / log(2.0) + e;
        }
#endif
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


/*[clinic input]
math.gcd

    x as a: object
    y as b: object
    /

greatest common divisor of x and y
[clinic start generated code]*/

static PyObject *
math_gcd_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=7b2e0c151bd7a5d8 input=c2691e57fb2a98fa]*/
{
    PyObject *g;

    a = PyNumber_Index(a);
    if (a == NULL)
        return NULL;
    b = PyNumber_Index(b);
    if (b == NULL) {
        Py_DECREF(a);
        return NULL;
    }
    g = _PyLong_GCD(a, b);
    Py_DECREF(a);
    Py_DECREF(b);
    return g;
}


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
         * function result is less than one in absolute value.
         */
        if (fabs(x) < 1.0)
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
math_1_to_whatever(PyObject *arg, double (*func) (double),
                   PyObject *(*from_double_func) (double),
                   int can_overflow)
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

    return (*from_double_func)(r);
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
math_1(PyObject *arg, double (*func) (double), int can_overflow)
{
    return math_1_to_whatever(arg, func, PyFloat_FromDouble, can_overflow);
}

static PyObject *
math_2(PyObject *const *args, Py_ssize_t nargs,
       double (*func) (double, double), const char *funcname)
{
    double x, y, r;
    if (!_PyArg_CheckPositional(funcname, nargs, 2, 2))
        return NULL;
    x = PyFloat_AsDouble(args[0]);
    y = PyFloat_AsDouble(args[1]);
    if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
        return NULL;
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
FUNC1(acosh, m_acosh, 0,
      "acosh($module, x, /)\n--\n\n"
      "Return the inverse hyperbolic cosine of x.")
FUNC1(asin, asin, 0,
      "asin($module, x, /)\n--\n\n"
      "Return the arc sine (measured in radians) of x.\n\n"
      "The result is between -pi/2 and pi/2.")
FUNC1(asinh, m_asinh, 0,
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
FUNC1(atanh, m_atanh, 0,
      "atanh($module, x, /)\n--\n\n"
      "Return the inverse hyperbolic tangent of x.")

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
    _Py_IDENTIFIER(__ceil__);

    if (!PyFloat_CheckExact(number)) {
        PyObject *method = _PyObject_LookupSpecial(number, &PyId___ceil__);
        if (method != NULL) {
            PyObject *result = _PyObject_CallNoArg(method);
            Py_DECREF(method);
            return result;
        }
        if (PyErr_Occurred())
            return NULL;
    }
    double x = PyFloat_AsDouble(number);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;

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
FUNC1A(erf, m_erf,
       "erf($module, x, /)\n--\n\n"
       "Error function at x.")
FUNC1A(erfc, m_erfc,
       "erfc($module, x, /)\n--\n\n"
       "Complementary error function at x.")
FUNC1(exp, exp, 1,
      "exp($module, x, /)\n--\n\n"
      "Return e raised to the power of x.")
FUNC1(expm1, m_expm1, 1,
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
    _Py_IDENTIFIER(__floor__);

    if (!PyFloat_CheckExact(number)) {
        PyObject *method = _PyObject_LookupSpecial(number, &PyId___floor__);
        if (method != NULL) {
            PyObject *result = _PyObject_CallNoArg(method);
            Py_DECREF(method);
            return result;
        }
        if (PyErr_Occurred())
            return NULL;
    }
    double x = PyFloat_AsDouble(number);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;

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

   Note 1: IEEE 754R floating point semantics are assumed,
   but the current implementation does not re-establish special
   value semantics across iterations (i.e. handling -Inf + Inf).

   Note 2:  No provision is made for intermediate overflow handling;
   therefore, sum([1e+308, 1e-308, 1e+308]) returns 1e+308 while
   sum([1e+308, 1e+308, 1e-308]) raises an OverflowError due to the
   overflow of the first partial sum.

   Note 3: The intermediate values lo, yr, and hi are declared volatile so
   aggressive compilers won't algebraically reduce lo to always be exactly 0.0.
   Also, the volatile declaration forces the values to be stored in memory as
   regular doubles instead of extended long precision (80-bit) values.  This
   prevents double rounding because any addition or subtraction of two doubles
   can be resolved exactly into double-sized hi and lo values.  As long as the
   hi value gets forced into a double before yr and lo are computed, the extra
   bits in downstream extended precision operations (x87 for example) will be
   exactly zero and therefore can be losslessly stored back into a double,
   thereby preventing double rounding.

   Note 4: A similar implementation is in Modules/cmathmodule.c.
   Be sure to update both when making changes.

   Note 5: The signature of math.fsum() differs from builtins.sum()
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
    volatile double hi, yr, lo;

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


/* Approximate square root of a large 64-bit integer.

   Given `n` satisfying `2**62 <= n < 2**64`, return `a`
   satisfying `(a - 1)**2 < n < (a + 1)**2`. */

static uint64_t
_approximate_isqrt(uint64_t n)
{
    uint32_t u = 1U + (n >> 62);
    u = (u << 1) + (n >> 59) / u;
    u = (u << 3) + (n >> 53) / u;
    u = (u << 7) + (n >> 41) / u;
    return (u << 15) + (n >> 17) / u;
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
    uint64_t m, u;
    PyObject *a = NULL, *b;

    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }

    if (_PyLong_Sign(n) < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "isqrt() argument must be nonnegative");
        goto error;
    }
    if (_PyLong_Sign(n) == 0) {
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
       fast, almost branch-free algorithm. In the final correction, we use `u*u
       - 1 >= m` instead of the simpler `u*u > m` in order to get the correct
       result in the corner case where `u=2**32`. */
    if (c <= 31U) {
        m = (uint64_t)PyLong_AsUnsignedLongLong(n);
        Py_DECREF(n);
        if (m == (uint64_t)(-1) && PyErr_Occurred()) {
            return NULL;
        }
        u = _approximate_isqrt(m << (62U - 2U*c)) >> (31U - c);
        u -= u * u - 1U >= m;
        return PyLong_FromUnsignedLongLong((unsigned long long)u);
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
    a = PyLong_FromUnsignedLongLong((unsigned long long)u);
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
        Py_SETREF(a, PyNumber_Subtract(a, _PyLong_One));
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
 *        (1 * 3 * 5 * 7 * 9)
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
    outer = inner;
    Py_INCREF(outer);

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
        Py_DECREF(inner);
        inner = tmp;
        /* Now inner is the product of all odd integers j in the range (0,
           n/2**i], giving the inner product in the formula above. */

        /* outer *= inner; */
        tmp = PyNumber_Multiply(outer, inner);
        if (tmp == NULL)
            goto error;
        Py_DECREF(outer);
        outer = tmp;
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

    x as arg: object
    /

Find x!.

Raise a ValueError if x is negative or non-integral.
[clinic start generated code]*/

static PyObject *
math_factorial(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=6686f26fae00e9ca input=6d1c8105c0d91fb4]*/
{
    long x, two_valuation;
    int overflow;
    PyObject *result, *odd_part, *pyint_form;

    if (PyFloat_Check(arg)) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                         "Using factorial() with floats is deprecated",
                         1) < 0)
        {
            return NULL;
        }
        PyObject *lx;
        double dx = PyFloat_AS_DOUBLE((PyFloatObject *)arg);
        if (!(Py_IS_FINITE(dx) && dx == floor(dx))) {
            PyErr_SetString(PyExc_ValueError,
                            "factorial() only accepts integral values");
            return NULL;
        }
        lx = PyLong_FromDouble(dx);
        if (lx == NULL)
            return NULL;
        x = PyLong_AsLongAndOverflow(lx, &overflow);
        Py_DECREF(lx);
    }
    else {
        pyint_form = PyNumber_Index(arg);
        if (pyint_form == NULL) {
            return NULL;
        }
        x = PyLong_AsLongAndOverflow(pyint_form, &overflow);
        Py_DECREF(pyint_form);
    }

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
    _Py_IDENTIFIER(__trunc__);
    PyObject *trunc, *result;

    if (PyFloat_CheckExact(x)) {
        return PyFloat_Type.tp_as_number->nb_int(x);
    }

    if (Py_TYPE(x)->tp_dict == NULL) {
        if (PyType_Ready(Py_TYPE(x)) < 0)
            return NULL;
    }

    trunc = _PyObject_LookupSpecial(x, &PyId___trunc__);
    if (trunc == NULL) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_TypeError,
                         "type %.100s doesn't define __trunc__ method",
                         Py_TYPE(x)->tp_name);
        return NULL;
    }
    result = _PyObject_CallNoArg(trunc);
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
    if (!Py_IS_FINITE(x)) {
        if (Py_IS_INFINITY(x))
            return Py_BuildValue("(dd)", copysign(0., x), x);
        else if (Py_IS_NAN(x))
            return Py_BuildValue("(dd)", x, x);
    }

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
loghelper(PyObject* arg, double (*func)(double), const char *funcname)
{
    /* If it is int, do it ourselves. */
    if (PyLong_Check(arg)) {
        double x, result;
        Py_ssize_t e;

        /* Negative or zero inputs give a ValueError. */
        if (Py_SIZE(arg) <= 0) {
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


/*[clinic input]
math.log

    x:    object
    [
    base: object(c_default="NULL") = math.e
    ]
    /

Return the logarithm of x to the given base.

If the base not specified, returns the natural logarithm (base e) of x.
[clinic start generated code]*/

static PyObject *
math_log_impl(PyObject *module, PyObject *x, int group_right_1,
              PyObject *base)
/*[clinic end generated code: output=7b5a39e526b73fc9 input=0f62d5726cbfebbd]*/
{
    PyObject *num, *den;
    PyObject *ans;

    num = loghelper(x, m_log, "log");
    if (num == NULL || base == NULL)
        return num;

    den = loghelper(base, m_log, "log");
    if (den == NULL) {
        Py_DECREF(num);
        return NULL;
    }

    ans = PyNumber_TrueDivide(num, den);
    Py_DECREF(num);
    Py_DECREF(den);
    return ans;
}


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
    return loghelper(x, m_log2, "log2");
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
    return loghelper(x, m_log10, "log10");
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
Given an *n* length *vec* of values and a value *max*, compute:

    max * sqrt(sum((x / max) ** 2 for x in vec))

The value of the *max* variable must be non-negative and
equal to the absolute value of the largest magnitude
entry in the vector.  If n==0, then *max* should be 0.0.
If an infinity is present in the vec, *max* should be INF.

The *found_nan* variable indicates whether some member of
the *vec* is a NaN.

To improve accuracy and to increase the number of cases where
vector_norm() is commutative, we use a variant of Neumaier
summation specialized to exploit that we always know that
|csum| >= |x|.

The *csum* variable tracks the cumulative sum and *frac* tracks
the cumulative fractional errors at each step.  Since this
variant assumes that |csum| >= |x| at each step, we establish
the precondition by starting the accumulation from 1.0 which
represents the largest possible value of (x/max)**2.

After the loop is finished, the initial 1.0 is subtracted out
for a net zero effect on the final sum.  Since *csum* will be
greater than 1.0, the subtraction of 1.0 will not cause
fractional digits to be dropped from *csum*.

*/

static inline double
vector_norm(Py_ssize_t n, double *vec, double max, int found_nan)
{
    double x, csum = 1.0, oldcsum, frac = 0.0;
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
    for (i=0 ; i < n ; i++) {
        x = vec[i];
        assert(Py_IS_FINITE(x) && fabs(x) <= max);
        x /= max;
        x = x*x;
        oldcsum = csum;
        csum += x;
        assert(csum >= x);
        frac += (oldcsum - csum) + x;
    }
    return max * sqrt(csum - 1.0 + frac);
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
        return NULL;

    }
    if (n > NUM_STACK_ELEMS) {
        diffs = (double *) PyObject_Malloc(n * sizeof(double));
        if (diffs == NULL) {
            return PyErr_NoMemory();
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
        else if (Py_IS_INFINITY(y)) {
            if (fabs(x) == 1.0)
                r = 1.;
            else if (y > 0. && fabs(x) > 1.0)
                r = y;
            else if (y < 0. && fabs(x) < 1.0) {
                r = -y; /* result is +inf */
                if (x == 0.) /* 0**-inf: divide-by-zero */
                    errno = EDOM;
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
        result = PyLong_FromLong(1);
        if (result == NULL) {
            Py_DECREF(iter);
            return NULL;
        }
    } else {
        Py_INCREF(result);
    }
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
            Py_DECREF(result);
            result = NULL;
        }
        /* Loop over all the items in the iterable until we finish, we overflow
         * or we found a non integer element */
        while(result == NULL) {
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
        Py_DECREF(result);
        result = NULL;
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
                Py_DECREF(result);
                result = NULL;
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
    PyObject *result = NULL, *factor = NULL;
    int overflow, cmp;
    long long i, factors;

    if (k == Py_None) {
        return math_factorial(module, n);
    }
    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }
    if (!PyLong_CheckExact(n)) {
        Py_SETREF(n, _PyLong_Copy((PyLongObject *)n));
        if (n == NULL) {
            return NULL;
        }
    }
    k = PyNumber_Index(k);
    if (k == NULL) {
        Py_DECREF(n);
        return NULL;
    }
    if (!PyLong_CheckExact(k)) {
        Py_SETREF(k, _PyLong_Copy((PyLongObject *)k));
        if (k == NULL) {
            Py_DECREF(n);
            return NULL;
        }
    }

    if (Py_SIZE(n) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a non-negative integer");
        goto error;
    }
    if (Py_SIZE(k) < 0) {
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

    factors = PyLong_AsLongLongAndOverflow(k, &overflow);
    if (overflow > 0) {
        PyErr_Format(PyExc_OverflowError,
                     "k must not exceed %lld",
                     LLONG_MAX);
        goto error;
    }
    else if (factors == -1) {
        /* k is nonnegative, so a return value of -1 can only indicate error */
        goto error;
    }

    if (factors == 0) {
        result = PyLong_FromLong(1);
        goto done;
    }

    result = n;
    Py_INCREF(result);
    if (factors == 1) {
        goto done;
    }

    factor = n;
    Py_INCREF(factor);
    for (i = 1; i < factors; ++i) {
        Py_SETREF(factor, PyNumber_Subtract(factor, _PyLong_One));
        if (factor == NULL) {
            goto error;
        }
        Py_SETREF(result, PyNumber_Multiply(result, factor));
        if (result == NULL) {
            goto error;
        }
    }
    Py_DECREF(factor);

done:
    Py_DECREF(n);
    Py_DECREF(k);
    return result;

error:
    Py_XDECREF(factor);
    Py_XDECREF(result);
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
    PyObject *result = NULL, *factor = NULL, *temp;
    int overflow, cmp;
    long long i, factors;

    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }
    if (!PyLong_CheckExact(n)) {
        Py_SETREF(n, _PyLong_Copy((PyLongObject *)n));
        if (n == NULL) {
            return NULL;
        }
    }
    k = PyNumber_Index(k);
    if (k == NULL) {
        Py_DECREF(n);
        return NULL;
    }
    if (!PyLong_CheckExact(k)) {
        Py_SETREF(k, _PyLong_Copy((PyLongObject *)k));
        if (k == NULL) {
            Py_DECREF(n);
            return NULL;
        }
    }

    if (Py_SIZE(n) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a non-negative integer");
        goto error;
    }
    if (Py_SIZE(k) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "k must be a non-negative integer");
        goto error;
    }

    /* k = min(k, n - k) */
    temp = PyNumber_Subtract(n, k);
    if (temp == NULL) {
        goto error;
    }
    if (Py_SIZE(temp) < 0) {
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

    factors = PyLong_AsLongLongAndOverflow(k, &overflow);
    if (overflow > 0) {
        PyErr_Format(PyExc_OverflowError,
                     "min(n - k, k) must not exceed %lld",
                     LLONG_MAX);
        goto error;
    }
    if (factors == -1) {
        /* k is nonnegative, so a return value of -1 can only indicate error */
        goto error;
    }

    if (factors == 0) {
        result = PyLong_FromLong(1);
        goto done;
    }

    result = n;
    Py_INCREF(result);
    if (factors == 1) {
        goto done;
    }

    factor = n;
    Py_INCREF(factor);
    for (i = 1; i < factors; ++i) {
        Py_SETREF(factor, PyNumber_Subtract(factor, _PyLong_One));
        if (factor == NULL) {
            goto error;
        }
        Py_SETREF(result, PyNumber_Multiply(result, factor));
        if (result == NULL) {
            goto error;
        }

        temp = PyLong_FromUnsignedLongLong((unsigned long long)i + 1);
        if (temp == NULL) {
            goto error;
        }
        Py_SETREF(result, PyNumber_FloorDivide(result, temp));
        Py_DECREF(temp);
        if (result == NULL) {
            goto error;
        }
    }
    Py_DECREF(factor);

done:
    Py_DECREF(n);
    Py_DECREF(k);
    return result;

error:
    Py_XDECREF(factor);
    Py_XDECREF(result);
    Py_DECREF(n);
    Py_DECREF(k);
    return NULL;
}


/*[clinic input]
math.nextafter

    x: double
    y: double
    /

Return the next floating-point value after x towards y.
[clinic start generated code]*/

static PyObject *
math_nextafter_impl(PyObject *module, double x, double y)
/*[clinic end generated code: output=750c8266c1c540ce input=02b2d50cd1d9f9b6]*/
{
    double f = nextafter(x, y);
    return PyFloat_FromDouble(f);
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
    double inf = m_inf();
    double x2 = nextafter(x, inf);
    if (Py_IS_INFINITY(x2)) {
        /* special case: x is the largest positive representable float */
        x2 = nextafter(x, -inf);
        return x - x2;
    }
    return x2 - x;
}


static PyMethodDef math_methods[] = {
    {"acos",            math_acos,      METH_O,         math_acos_doc},
    {"acosh",           math_acosh,     METH_O,         math_acosh_doc},
    {"asin",            math_asin,      METH_O,         math_asin_doc},
    {"asinh",           math_asinh,     METH_O,         math_asinh_doc},
    {"atan",            math_atan,      METH_O,         math_atan_doc},
    {"atan2",           (PyCFunction)(void(*)(void))math_atan2,     METH_FASTCALL,  math_atan2_doc},
    {"atanh",           math_atanh,     METH_O,         math_atanh_doc},
    MATH_CEIL_METHODDEF
    {"copysign",        (PyCFunction)(void(*)(void))math_copysign,  METH_FASTCALL,  math_copysign_doc},
    {"cos",             math_cos,       METH_O,         math_cos_doc},
    {"cosh",            math_cosh,      METH_O,         math_cosh_doc},
    MATH_DEGREES_METHODDEF
    MATH_DIST_METHODDEF
    {"erf",             math_erf,       METH_O,         math_erf_doc},
    {"erfc",            math_erfc,      METH_O,         math_erfc_doc},
    {"exp",             math_exp,       METH_O,         math_exp_doc},
    {"expm1",           math_expm1,     METH_O,         math_expm1_doc},
    {"fabs",            math_fabs,      METH_O,         math_fabs_doc},
    MATH_FACTORIAL_METHODDEF
    MATH_FLOOR_METHODDEF
    MATH_FMOD_METHODDEF
    MATH_FREXP_METHODDEF
    MATH_FSUM_METHODDEF
    {"gamma",           math_gamma,     METH_O,         math_gamma_doc},
    MATH_GCD_METHODDEF
    {"hypot",           (PyCFunction)(void(*)(void))math_hypot,     METH_FASTCALL,  math_hypot_doc},
    MATH_ISCLOSE_METHODDEF
    MATH_ISFINITE_METHODDEF
    MATH_ISINF_METHODDEF
    MATH_ISNAN_METHODDEF
    MATH_ISQRT_METHODDEF
    MATH_LDEXP_METHODDEF
    {"lgamma",          math_lgamma,    METH_O,         math_lgamma_doc},
    MATH_LOG_METHODDEF
    {"log1p",           math_log1p,     METH_O,         math_log1p_doc},
    MATH_LOG10_METHODDEF
    MATH_LOG2_METHODDEF
    MATH_MODF_METHODDEF
    MATH_POW_METHODDEF
    MATH_RADIANS_METHODDEF
    {"remainder",       (PyCFunction)(void(*)(void))math_remainder, METH_FASTCALL,  math_remainder_doc},
    {"sin",             math_sin,       METH_O,         math_sin_doc},
    {"sinh",            math_sinh,      METH_O,         math_sinh_doc},
    {"sqrt",            math_sqrt,      METH_O,         math_sqrt_doc},
    {"tan",             math_tan,       METH_O,         math_tan_doc},
    {"tanh",            math_tanh,      METH_O,         math_tanh_doc},
    MATH_TRUNC_METHODDEF
    MATH_PROD_METHODDEF
    MATH_PERM_METHODDEF
    MATH_COMB_METHODDEF
    MATH_NEXTAFTER_METHODDEF
    MATH_ULP_METHODDEF
    {NULL,              NULL}           /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module provides access to the mathematical functions\n"
"defined by the C standard.");


static struct PyModuleDef mathmodule = {
    PyModuleDef_HEAD_INIT,
    "math",
    module_doc,
    -1,
    math_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_math(void)
{
    PyObject *m;

    m = PyModule_Create(&mathmodule);
    if (m == NULL)
        goto finally;

    PyModule_AddObject(m, "pi", PyFloat_FromDouble(Py_MATH_PI));
    PyModule_AddObject(m, "e", PyFloat_FromDouble(Py_MATH_E));
    PyModule_AddObject(m, "tau", PyFloat_FromDouble(Py_MATH_TAU));  /* 2pi */
    PyModule_AddObject(m, "inf", PyFloat_FromDouble(m_inf()));
#if !defined(PY_NO_SHORT_FLOAT_REPR) || defined(Py_NAN)
    PyModule_AddObject(m, "nan", PyFloat_FromDouble(m_nan()));
#endif

  finally:
    return m;
}
