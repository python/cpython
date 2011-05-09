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

/*
   sin(pi*x), giving accurate results for all finite x (especially x
   integral or close to an integer).  This is here for use in the
   reflection formula for the gamma function.  It conforms to IEEE
   754-2008 for finite arguments, but not for infinities or nans.
*/

static const double pi = 3.141592653589793238462643383279502884197;
static const double sqrtpi = 1.772453850905516027298167483341145182798;
static const double logpi = 1.144729885849400174143427351353058711647;

static double
sinpi(double x)
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
        assert(0);  /* should never get here */
        r = -1.23e200; /* silence gcc warning */
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
        return 1.0/x; /* tgamma(+-0.0) = +-inf, divide-by-zero */
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
            return 0.0/sinpi(x);
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
        r = -pi / sinpi(absx) / absx * exp(y) / lanczos_sum(absx);
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
    double r, absx;

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
        r = logpi - log(fabs(sinpi(absx))) - log(absx) - r;
    if (Py_IS_INFINITY(r))
        errno = ERANGE;
    return r;
}

/*
   Implementations of the error function erf(x) and the complementary error
   function erfc(x).

   Method: following 'Numerical Recipes' by Flannery, Press et. al. (2nd ed.,
   Cambridge University Press), we use a series approximation for erf for
   small x, and a continued fraction approximation for erfc(x) for larger x;
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

/* Error function erf(x), for general x */

static double
m_erf(double x)
{
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
}

/* Complementary error function erfc(x), for general x. */

static double
m_erfc(double x)
{
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
   arguments and returns a double.

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
    PyFPE_START_PROTECT("in math_1", return 0);
    r = (*func)(x);
    PyFPE_END_PROTECT(r);
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
    PyFPE_START_PROTECT("in math_1a", return 0);
    r = (*func)(x);
    PyFPE_END_PROTECT(r);
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
math_1_to_int(PyObject *arg, double (*func) (double), int can_overflow)
{
    return math_1_to_whatever(arg, func, PyLong_FromDouble, can_overflow);
}

static PyObject *
math_2(PyObject *args, double (*func) (double, double), char *funcname)
{
    PyObject *ox, *oy;
    double x, y, r;
    if (! PyArg_UnpackTuple(args, funcname, 2, 2, &ox, &oy))
        return NULL;
    x = PyFloat_AsDouble(ox);
    y = PyFloat_AsDouble(oy);
    if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
        return NULL;
    errno = 0;
    PyFPE_START_PROTECT("in math_2", return 0);
    r = (*func)(x, y);
    PyFPE_END_PROTECT(r);
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
    static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
        return math_2(args, func, #funcname); \
    }\
    PyDoc_STRVAR(math_##funcname##_doc, docstring);

FUNC1(acos, acos, 0,
      "acos(x)\n\nReturn the arc cosine (measured in radians) of x.")
FUNC1(acosh, m_acosh, 0,
      "acosh(x)\n\nReturn the hyperbolic arc cosine (measured in radians) of x.")
FUNC1(asin, asin, 0,
      "asin(x)\n\nReturn the arc sine (measured in radians) of x.")
FUNC1(asinh, m_asinh, 0,
      "asinh(x)\n\nReturn the hyperbolic arc sine (measured in radians) of x.")
FUNC1(atan, atan, 0,
      "atan(x)\n\nReturn the arc tangent (measured in radians) of x.")
FUNC2(atan2, m_atan2,
      "atan2(y, x)\n\nReturn the arc tangent (measured in radians) of y/x.\n"
      "Unlike atan(y/x), the signs of both x and y are considered.")
FUNC1(atanh, m_atanh, 0,
      "atanh(x)\n\nReturn the hyperbolic arc tangent (measured in radians) of x.")

static PyObject * math_ceil(PyObject *self, PyObject *number) {
    static PyObject *ceil_str = NULL;
    PyObject *method, *result;

    method = _PyObject_LookupSpecial(number, "__ceil__", &ceil_str);
    if (method == NULL) {
        if (PyErr_Occurred())
            return NULL;
        return math_1_to_int(number, ceil, 0);
    }
    result = PyObject_CallFunctionObjArgs(method, NULL);
    Py_DECREF(method);
    return result;
}

PyDoc_STRVAR(math_ceil_doc,
             "ceil(x)\n\nReturn the ceiling of x as an int.\n"
             "This is the smallest integral value >= x.");

FUNC2(copysign, copysign,
      "copysign(x, y)\n\nReturn x with the sign of y.")
FUNC1(cos, cos, 0,
      "cos(x)\n\nReturn the cosine of x (measured in radians).")
FUNC1(cosh, cosh, 1,
      "cosh(x)\n\nReturn the hyperbolic cosine of x.")
FUNC1A(erf, m_erf,
       "erf(x)\n\nError function at x.")
FUNC1A(erfc, m_erfc,
       "erfc(x)\n\nComplementary error function at x.")
FUNC1(exp, exp, 1,
      "exp(x)\n\nReturn e raised to the power of x.")
FUNC1(expm1, m_expm1, 1,
      "expm1(x)\n\nReturn exp(x)-1.\n"
      "This function avoids the loss of precision involved in the direct "
      "evaluation of exp(x)-1 for small x.")
FUNC1(fabs, fabs, 0,
      "fabs(x)\n\nReturn the absolute value of the float x.")

static PyObject * math_floor(PyObject *self, PyObject *number) {
    static PyObject *floor_str = NULL;
    PyObject *method, *result;

    method = _PyObject_LookupSpecial(number, "__floor__", &floor_str);
    if (method == NULL) {
        if (PyErr_Occurred())
            return NULL;
        return math_1_to_int(number, floor, 0);
    }
    result = PyObject_CallFunctionObjArgs(method, NULL);
    Py_DECREF(method);
    return result;
}

PyDoc_STRVAR(math_floor_doc,
             "floor(x)\n\nReturn the floor of x as an int.\n"
             "This is the largest integral value <= x.");

FUNC1A(gamma, m_tgamma,
      "gamma(x)\n\nGamma function at x.")
FUNC1A(lgamma, m_lgamma,
      "lgamma(x)\n\nNatural logarithm of absolute value of Gamma function at x.")
FUNC1(log1p, m_log1p, 0,
      "log1p(x)\n\nReturn the natural logarithm of 1+x (base e).\n"
      "The result is computed in a way which is accurate for x near zero.")
FUNC1(sin, sin, 0,
      "sin(x)\n\nReturn the sine of x (measured in radians).")
FUNC1(sinh, sinh, 1,
      "sinh(x)\n\nReturn the hyperbolic sine of x.")
FUNC1(sqrt, sqrt, 0,
      "sqrt(x)\n\nReturn the square root of x.")
FUNC1(tan, tan, 0,
      "tan(x)\n\nReturn the tangent of x (measured in radians).")
FUNC1(tanh, tanh, 0,
      "tanh(x)\n\nReturn the hyperbolic tangent of x.")

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

   Note 5: The signature of math.fsum() differs from __builtin__.sum()
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
    if (n < m && m < (PY_SSIZE_T_MAX / sizeof(double))) {
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

static PyObject*
math_fsum(PyObject *self, PyObject *seq)
{
    PyObject *item, *iter, *sum = NULL;
    Py_ssize_t i, j, n = 0, m = NUM_PARTIALS;
    double x, y, t, ps[NUM_PARTIALS], *p = ps;
    double xsave, special_sum = 0.0, inf_sum = 0.0;
    volatile double hi, yr, lo;

    iter = PyObject_GetIter(seq);
    if (iter == NULL)
        return NULL;

    PyFPE_START_PROTECT("fsum", Py_DECREF(iter); return NULL)

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
        x = PyFloat_AsDouble(item);
        Py_DECREF(item);
        if (PyErr_Occurred())
            goto _fsum_error;

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
    PyFPE_END_PROTECT(hi)
    Py_DECREF(iter);
    if (p != ps)
        PyMem_Free(p);
    return sum;
}

#undef NUM_PARTIALS

PyDoc_STRVAR(math_fsum_doc,
"fsum(iterable)\n\n\
Return an accurate floating point sum of values in the iterable.\n\
Assumes IEEE-754 floating point arithmetic.");

/* Return the smallest integer k such that n < 2**k, or 0 if n == 0.
 * Equivalent to floor(lg(x))+1.  Also equivalent to: bitwidth_of_type -
 * count_leading_zero_bits(x)
 */

/* XXX: This routine does more or less the same thing as
 * bits_in_digit() in Objects/longobject.c.  Someday it would be nice to
 * consolidate them.  On BSD, there's a library function called fls()
 * that we could use, and GCC provides __builtin_clz().
 */

static unsigned long
bit_length(unsigned long n)
{
    unsigned long len = 0;
    while (n != 0) {
        ++len;
        n >>= 1;
    }
    return len;
}

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

/* Divide-and-conquer factorial algorithm
 *
 * Based on the formula and psuedo-code provided at:
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
                                     bit_length(midpoint - 2));
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
    for (i = bit_length(n) - 2; i >= 0; i--) {
        v = n >> i;
        if (v <= 2)
            continue;
        lower = upper;
        /* (v + 1) | 1 = least odd integer strictly larger than n / 2**i */
        upper = (v + 1) | 1;
        /* Here inner is the product of all odd integers j in the range (0,
           n/2**(i+1)].  The factorial_partial_product call below gives the
           product of all odd integers j in the range (n/2**(i+1), n/2**i]. */
        partial = factorial_partial_product(lower, upper, bit_length(upper-2));
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

    goto done;

  error:
    Py_DECREF(outer);
  done:
    Py_DECREF(inner);
    return outer;
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

static PyObject *
math_factorial(PyObject *self, PyObject *arg)
{
    long x;
    PyObject *result, *odd_part, *two_valuation;

    if (PyFloat_Check(arg)) {
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
        x = PyLong_AsLong(lx);
        Py_DECREF(lx);
    }
    else
        x = PyLong_AsLong(arg);

    if (x == -1 && PyErr_Occurred())
        return NULL;
    if (x < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "factorial() not defined for negative values");
        return NULL;
    }

    /* use lookup table if x is small */
    if (x < (long)(sizeof(SmallFactorials)/sizeof(SmallFactorials[0])))
        return PyLong_FromUnsignedLong(SmallFactorials[x]);

    /* else express in the form odd_part * 2**two_valuation, and compute as
       odd_part << two_valuation. */
    odd_part = factorial_odd_part(x);
    if (odd_part == NULL)
        return NULL;
    two_valuation = PyLong_FromLong(x - count_set_bits(x));
    if (two_valuation == NULL) {
        Py_DECREF(odd_part);
        return NULL;
    }
    result = PyNumber_Lshift(odd_part, two_valuation);
    Py_DECREF(two_valuation);
    Py_DECREF(odd_part);
    return result;
}

PyDoc_STRVAR(math_factorial_doc,
"factorial(x) -> Integral\n"
"\n"
"Find x!. Raise a ValueError if x is negative or non-integral.");

static PyObject *
math_trunc(PyObject *self, PyObject *number)
{
    static PyObject *trunc_str = NULL;
    PyObject *trunc, *result;

    if (Py_TYPE(number)->tp_dict == NULL) {
        if (PyType_Ready(Py_TYPE(number)) < 0)
            return NULL;
    }

    trunc = _PyObject_LookupSpecial(number, "__trunc__", &trunc_str);
    if (trunc == NULL) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_TypeError,
                         "type %.100s doesn't define __trunc__ method",
                         Py_TYPE(number)->tp_name);
        return NULL;
    }
    result = PyObject_CallFunctionObjArgs(trunc, NULL);
    Py_DECREF(trunc);
    return result;
}

PyDoc_STRVAR(math_trunc_doc,
"trunc(x:Real) -> Integral\n"
"\n"
"Truncates x to the nearest Integral toward 0. Uses the __trunc__ magic method.");

static PyObject *
math_frexp(PyObject *self, PyObject *arg)
{
    int i;
    double x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    /* deal with special cases directly, to sidestep platform
       differences */
    if (Py_IS_NAN(x) || Py_IS_INFINITY(x) || !x) {
        i = 0;
    }
    else {
        PyFPE_START_PROTECT("in math_frexp", return 0);
        x = frexp(x, &i);
        PyFPE_END_PROTECT(x);
    }
    return Py_BuildValue("(di)", x, i);
}

PyDoc_STRVAR(math_frexp_doc,
"frexp(x)\n"
"\n"
"Return the mantissa and exponent of x, as pair (m, e).\n"
"m is a float and e is an int, such that x = m * 2.**e.\n"
"If x is 0, m and e are both 0.  Else 0.5 <= abs(m) < 1.0.");

static PyObject *
math_ldexp(PyObject *self, PyObject *args)
{
    double x, r;
    PyObject *oexp;
    long exp;
    int overflow;
    if (! PyArg_ParseTuple(args, "dO:ldexp", &x, &oexp))
        return NULL;

    if (PyLong_Check(oexp)) {
        /* on overflow, replace exponent with either LONG_MAX
           or LONG_MIN, depending on the sign. */
        exp = PyLong_AsLongAndOverflow(oexp, &overflow);
        if (exp == -1 && PyErr_Occurred())
            return NULL;
        if (overflow)
            exp = overflow < 0 ? LONG_MIN : LONG_MAX;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Expected an int or long as second argument "
                        "to ldexp.");
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
        PyFPE_START_PROTECT("in math_ldexp", return 0);
        r = ldexp(x, (int)exp);
        PyFPE_END_PROTECT(r);
        if (Py_IS_INFINITY(r))
            errno = ERANGE;
    }

    if (errno && is_error(r))
        return NULL;
    return PyFloat_FromDouble(r);
}

PyDoc_STRVAR(math_ldexp_doc,
"ldexp(x, i)\n\n\
Return x * (2**i).");

static PyObject *
math_modf(PyObject *self, PyObject *arg)
{
    double y, x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    /* some platforms don't do the right thing for NaNs and
       infinities, so we take care of special cases directly. */
    if (!Py_IS_FINITE(x)) {
        if (Py_IS_INFINITY(x))
            return Py_BuildValue("(dd)", copysign(0., x), x);
        else if (Py_IS_NAN(x))
            return Py_BuildValue("(dd)", x, x);
    }

    errno = 0;
    PyFPE_START_PROTECT("in math_modf", return 0);
    x = modf(x, &y);
    PyFPE_END_PROTECT(x);
    return Py_BuildValue("(dd)", x, y);
}

PyDoc_STRVAR(math_modf_doc,
"modf(x)\n"
"\n"
"Return the fractional and integer parts of x.  Both results carry the sign\n"
"of x and are floats.");

/* A decent logarithm is easy to compute even for huge longs, but libm can't
   do that by itself -- loghelper can.  func is log or log10, and name is
   "log" or "log10".  Note that overflow of the result isn't possible: a long
   can contain no more than INT_MAX * SHIFT bits, so has value certainly less
   than 2**(2**64 * 2**16) == 2**2**80, and log2 of that is 2**80, which is
   small enough to fit in an IEEE single.  log and log10 are even smaller.
   However, intermediate overflow is possible for a long if the number of bits
   in that long is larger than PY_SSIZE_T_MAX. */

static PyObject*
loghelper(PyObject* arg, double (*func)(double), char *funcname)
{
    /* If it is long, do it ourselves. */
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

static PyObject *
math_log(PyObject *self, PyObject *args)
{
    PyObject *arg;
    PyObject *base = NULL;
    PyObject *num, *den;
    PyObject *ans;

    if (!PyArg_UnpackTuple(args, "log", 1, 2, &arg, &base))
        return NULL;

    num = loghelper(arg, m_log, "log");
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

PyDoc_STRVAR(math_log_doc,
"log(x[, base])\n\n\
Return the logarithm of x to the given base.\n\
If the base not specified, returns the natural logarithm (base e) of x.");

static PyObject *
math_log2(PyObject *self, PyObject *arg)
{
    return loghelper(arg, m_log2, "log2");
}

PyDoc_STRVAR(math_log2_doc,
"log2(x)\n\nReturn the base 2 logarithm of x.");

static PyObject *
math_log10(PyObject *self, PyObject *arg)
{
    return loghelper(arg, m_log10, "log10");
}

PyDoc_STRVAR(math_log10_doc,
"log10(x)\n\nReturn the base 10 logarithm of x.");

static PyObject *
math_fmod(PyObject *self, PyObject *args)
{
    PyObject *ox, *oy;
    double r, x, y;
    if (! PyArg_UnpackTuple(args, "fmod", 2, 2, &ox, &oy))
        return NULL;
    x = PyFloat_AsDouble(ox);
    y = PyFloat_AsDouble(oy);
    if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
        return NULL;
    /* fmod(x, +/-Inf) returns x for finite x. */
    if (Py_IS_INFINITY(y) && Py_IS_FINITE(x))
        return PyFloat_FromDouble(x);
    errno = 0;
    PyFPE_START_PROTECT("in math_fmod", return 0);
    r = fmod(x, y);
    PyFPE_END_PROTECT(r);
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

PyDoc_STRVAR(math_fmod_doc,
"fmod(x, y)\n\nReturn fmod(x, y), according to platform C."
"  x % y may differ.");

static PyObject *
math_hypot(PyObject *self, PyObject *args)
{
    PyObject *ox, *oy;
    double r, x, y;
    if (! PyArg_UnpackTuple(args, "hypot", 2, 2, &ox, &oy))
        return NULL;
    x = PyFloat_AsDouble(ox);
    y = PyFloat_AsDouble(oy);
    if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
        return NULL;
    /* hypot(x, +/-Inf) returns Inf, even if x is a NaN. */
    if (Py_IS_INFINITY(x))
        return PyFloat_FromDouble(fabs(x));
    if (Py_IS_INFINITY(y))
        return PyFloat_FromDouble(fabs(y));
    errno = 0;
    PyFPE_START_PROTECT("in math_hypot", return 0);
    r = hypot(x, y);
    PyFPE_END_PROTECT(r);
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

PyDoc_STRVAR(math_hypot_doc,
"hypot(x, y)\n\nReturn the Euclidean distance, sqrt(x*x + y*y).");

/* pow can't use math_2, but needs its own wrapper: the problem is
   that an infinite result can arise either as a result of overflow
   (in which case OverflowError should be raised) or as a result of
   e.g. 0.**-5. (for which ValueError needs to be raised.)
*/

static PyObject *
math_pow(PyObject *self, PyObject *args)
{
    PyObject *ox, *oy;
    double r, x, y;
    int odd_y;

    if (! PyArg_UnpackTuple(args, "pow", 2, 2, &ox, &oy))
        return NULL;
    x = PyFloat_AsDouble(ox);
    y = PyFloat_AsDouble(oy);
    if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
        return NULL;

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
        PyFPE_START_PROTECT("in math_pow", return 0);
        r = pow(x, y);
        PyFPE_END_PROTECT(r);
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

PyDoc_STRVAR(math_pow_doc,
"pow(x, y)\n\nReturn x**y (x to the power of y).");

static const double degToRad = Py_MATH_PI / 180.0;
static const double radToDeg = 180.0 / Py_MATH_PI;

static PyObject *
math_degrees(PyObject *self, PyObject *arg)
{
    double x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    return PyFloat_FromDouble(x * radToDeg);
}

PyDoc_STRVAR(math_degrees_doc,
"degrees(x)\n\n\
Convert angle x from radians to degrees.");

static PyObject *
math_radians(PyObject *self, PyObject *arg)
{
    double x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    return PyFloat_FromDouble(x * degToRad);
}

PyDoc_STRVAR(math_radians_doc,
"radians(x)\n\n\
Convert angle x from degrees to radians.");

static PyObject *
math_isfinite(PyObject *self, PyObject *arg)
{
    double x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    return PyBool_FromLong((long)Py_IS_FINITE(x));
}

PyDoc_STRVAR(math_isfinite_doc,
"isfinite(x) -> bool\n\n\
Return True if x is neither an infinity nor a NaN, and False otherwise.");

static PyObject *
math_isnan(PyObject *self, PyObject *arg)
{
    double x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    return PyBool_FromLong((long)Py_IS_NAN(x));
}

PyDoc_STRVAR(math_isnan_doc,
"isnan(x) -> bool\n\n\
Return True if x is a NaN (not a number), and False otherwise.");

static PyObject *
math_isinf(PyObject *self, PyObject *arg)
{
    double x = PyFloat_AsDouble(arg);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    return PyBool_FromLong((long)Py_IS_INFINITY(x));
}

PyDoc_STRVAR(math_isinf_doc,
"isinf(x) -> bool\n\n\
Return True if x is a positive or negative infinity, and False otherwise.");

static PyMethodDef math_methods[] = {
    {"acos",            math_acos,      METH_O,         math_acos_doc},
    {"acosh",           math_acosh,     METH_O,         math_acosh_doc},
    {"asin",            math_asin,      METH_O,         math_asin_doc},
    {"asinh",           math_asinh,     METH_O,         math_asinh_doc},
    {"atan",            math_atan,      METH_O,         math_atan_doc},
    {"atan2",           math_atan2,     METH_VARARGS,   math_atan2_doc},
    {"atanh",           math_atanh,     METH_O,         math_atanh_doc},
    {"ceil",            math_ceil,      METH_O,         math_ceil_doc},
    {"copysign",        math_copysign,  METH_VARARGS,   math_copysign_doc},
    {"cos",             math_cos,       METH_O,         math_cos_doc},
    {"cosh",            math_cosh,      METH_O,         math_cosh_doc},
    {"degrees",         math_degrees,   METH_O,         math_degrees_doc},
    {"erf",             math_erf,       METH_O,         math_erf_doc},
    {"erfc",            math_erfc,      METH_O,         math_erfc_doc},
    {"exp",             math_exp,       METH_O,         math_exp_doc},
    {"expm1",           math_expm1,     METH_O,         math_expm1_doc},
    {"fabs",            math_fabs,      METH_O,         math_fabs_doc},
    {"factorial",       math_factorial, METH_O,         math_factorial_doc},
    {"floor",           math_floor,     METH_O,         math_floor_doc},
    {"fmod",            math_fmod,      METH_VARARGS,   math_fmod_doc},
    {"frexp",           math_frexp,     METH_O,         math_frexp_doc},
    {"fsum",            math_fsum,      METH_O,         math_fsum_doc},
    {"gamma",           math_gamma,     METH_O,         math_gamma_doc},
    {"hypot",           math_hypot,     METH_VARARGS,   math_hypot_doc},
    {"isfinite",        math_isfinite,  METH_O,         math_isfinite_doc},
    {"isinf",           math_isinf,     METH_O,         math_isinf_doc},
    {"isnan",           math_isnan,     METH_O,         math_isnan_doc},
    {"ldexp",           math_ldexp,     METH_VARARGS,   math_ldexp_doc},
    {"lgamma",          math_lgamma,    METH_O,         math_lgamma_doc},
    {"log",             math_log,       METH_VARARGS,   math_log_doc},
    {"log1p",           math_log1p,     METH_O,         math_log1p_doc},
    {"log10",           math_log10,     METH_O,         math_log10_doc},
    {"log2",            math_log2,      METH_O,         math_log2_doc},
    {"modf",            math_modf,      METH_O,         math_modf_doc},
    {"pow",             math_pow,       METH_VARARGS,   math_pow_doc},
    {"radians",         math_radians,   METH_O,         math_radians_doc},
    {"sin",             math_sin,       METH_O,         math_sin_doc},
    {"sinh",            math_sinh,      METH_O,         math_sinh_doc},
    {"sqrt",            math_sqrt,      METH_O,         math_sqrt_doc},
    {"tan",             math_tan,       METH_O,         math_tan_doc},
    {"tanh",            math_tanh,      METH_O,         math_tanh_doc},
    {"trunc",           math_trunc,     METH_O,         math_trunc_doc},
    {NULL,              NULL}           /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module is always available.  It provides access to the\n"
"mathematical functions defined by the C standard.");


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

    finally:
    return m;
}
