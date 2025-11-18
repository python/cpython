/* Complex math module */

/* much code borrowed from mathmodule.c */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_complexobject.h" // _Py_c_neg()
#include "pycore_pymath.h"        // _PY_SHORT_FLOAT_REPR
/* we need DBL_MAX, DBL_MIN, DBL_EPSILON, DBL_MANT_DIG and FLT_RADIX from
   float.h.  We assume that FLT_RADIX is either 2 or 16. */
#include <float.h>

/* For _Py_log1p with workarounds for buggy handling of zeros. */
#include "_math.h"

#include "clinic/cmathmodule.c.h"
/*[clinic input]
module cmath
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=308d6839f4a46333]*/

/*[python input]
class Py_complex_protected_converter(Py_complex_converter):
    def modify(self):
        return 'errno = 0;'


class Py_complex_protected_return_converter(CReturnConverter):
    type = "Py_complex"

    def render(self, function, data):
        self.declare(data)
        data.return_conversion.append("""
if (errno == EDOM) {
    PyErr_SetString(PyExc_ValueError, "math domain error");
    goto exit;
}
else if (errno == ERANGE) {
    PyErr_SetString(PyExc_OverflowError, "math range error");
    goto exit;
}
else {
    return_value = PyComplex_FromCComplex(_return_value);
}
""".strip())
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=8b27adb674c08321]*/

#if (FLT_RADIX != 2 && FLT_RADIX != 16)
#error "Modules/cmathmodule.c expects FLT_RADIX to be 2 or 16"
#endif

#ifndef M_LN2
#define M_LN2 (0.6931471805599453094) /* natural log of 2 */
#endif

#ifndef M_LN10
#define M_LN10 (2.302585092994045684) /* natural log of 10 */
#endif

/*
   CM_LARGE_DOUBLE is used to avoid spurious overflow in the sqrt, log,
   inverse trig and inverse hyperbolic trig functions.  Its log is used in the
   evaluation of exp, cos, cosh, sin, sinh, tan, and tanh to avoid unnecessary
   overflow.
 */

#define CM_LARGE_DOUBLE (DBL_MAX/4.)
#define CM_SQRT_LARGE_DOUBLE (sqrt(CM_LARGE_DOUBLE))
#define CM_LOG_LARGE_DOUBLE (log(CM_LARGE_DOUBLE))
#define CM_SQRT_DBL_MIN (sqrt(DBL_MIN))

/*
   CM_SCALE_UP is an odd integer chosen such that multiplication by
   2**CM_SCALE_UP is sufficient to turn a subnormal into a normal.
   CM_SCALE_DOWN is (-(CM_SCALE_UP+1)/2).  These scalings are used to compute
   square roots accurately when the real and imaginary parts of the argument
   are subnormal.
*/

#if FLT_RADIX==2
#define CM_SCALE_UP (2*(DBL_MANT_DIG/2) + 1)
#elif FLT_RADIX==16
#define CM_SCALE_UP (4*DBL_MANT_DIG+1)
#endif
#define CM_SCALE_DOWN (-(CM_SCALE_UP+1)/2)


/* forward declarations */
static Py_complex cmath_asinh_impl(PyObject *, Py_complex);
static Py_complex cmath_atanh_impl(PyObject *, Py_complex);
static Py_complex cmath_cosh_impl(PyObject *, Py_complex);
static Py_complex cmath_sinh_impl(PyObject *, Py_complex);
static Py_complex cmath_sqrt_impl(PyObject *, Py_complex);
static Py_complex cmath_tanh_impl(PyObject *, Py_complex);
static PyObject * math_error(void);

/* Code to deal with special values (infinities, NaNs, etc.). */

/* special_type takes a double and returns an integer code indicating
   the type of the double as follows:
*/

enum special_types {
    ST_NINF,            /* 0, negative infinity */
    ST_NEG,             /* 1, negative finite number (nonzero) */
    ST_NZERO,           /* 2, -0. */
    ST_PZERO,           /* 3, +0. */
    ST_POS,             /* 4, positive finite number (nonzero) */
    ST_PINF,            /* 5, positive infinity */
    ST_NAN              /* 6, Not a Number */
};

static enum special_types
special_type(double d)
{
    if (isfinite(d)) {
        if (d != 0) {
            if (copysign(1., d) == 1.)
                return ST_POS;
            else
                return ST_NEG;
        }
        else {
            if (copysign(1., d) == 1.)
                return ST_PZERO;
            else
                return ST_NZERO;
        }
    }
    if (isnan(d))
        return ST_NAN;
    if (copysign(1., d) == 1.)
        return ST_PINF;
    else
        return ST_NINF;
}

#define SPECIAL_VALUE(z, table)                       \
    if (!isfinite((z).real) || !isfinite((z).imag)) { \
        errno = 0;                                    \
        return table[special_type((z).real)]          \
                    [special_type((z).imag)];         \
    }

#define P Py_MATH_PI
#define P14 0.25*Py_MATH_PI
#define P12 0.5*Py_MATH_PI
#define P34 0.75*Py_MATH_PI
#define INF INFINITY
#define N Py_NAN
#define U -9.5426319407711027e33 /* unlikely value, used as placeholder */

/* First, the C functions that do the real work.  Each of the c_*
   functions computes and returns the C99 Annex G recommended result
   and also sets errno as follows: errno = 0 if no floating-point
   exception is associated with the result; errno = EDOM if C99 Annex
   G recommends raising divide-by-zero or invalid for this result; and
   errno = ERANGE where the overflow floating-point signal should be
   raised.
*/

static Py_complex acos_special_values[7][7];

/*[clinic input]
cmath.acos -> Py_complex_protected

    z: Py_complex_protected
    /

Return the arc cosine of z.
[clinic start generated code]*/

static Py_complex
cmath_acos_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=40bd42853fd460ae input=bd6cbd78ae851927]*/
{
    Py_complex s1, s2, r;

    SPECIAL_VALUE(z, acos_special_values);

    if (fabs(z.real) > CM_LARGE_DOUBLE || fabs(z.imag) > CM_LARGE_DOUBLE) {
        /* avoid unnecessary overflow for large arguments */
        r.real = atan2(fabs(z.imag), z.real);
        r.imag = -copysign(log(hypot(z.real/2., z.imag/2.)) +
                           M_LN2*2., z.imag);
    } else {
        s1.real = 1.-z.real;
        s1.imag = -z.imag;
        s1 = cmath_sqrt_impl(module, s1);
        s2.real = 1.+z.real;
        s2.imag = z.imag;
        s2 = cmath_sqrt_impl(module, s2);
        r.real = 2.*atan2(s1.real, s2.real);
        r.imag = asinh(s2.real*s1.imag - s2.imag*s1.real);
    }
    errno = 0;
    return r;
}


static Py_complex acosh_special_values[7][7];

/*[clinic input]
cmath.acosh = cmath.acos

Return the inverse hyperbolic cosine of z.
[clinic start generated code]*/

static Py_complex
cmath_acosh_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=3e2454d4fcf404ca input=3f61bee7d703e53c]*/
{
    Py_complex s1, s2, r;

    SPECIAL_VALUE(z, acosh_special_values);

    if (fabs(z.real) > CM_LARGE_DOUBLE || fabs(z.imag) > CM_LARGE_DOUBLE) {
        /* avoid unnecessary overflow for large arguments */
        r.real = log(hypot(z.real/2., z.imag/2.)) + M_LN2*2.;
        r.imag = atan2(z.imag, z.real);
    } else {
        s1.real = z.real - 1.;
        s1.imag = z.imag;
        s1 = cmath_sqrt_impl(module, s1);
        s2.real = z.real + 1.;
        s2.imag = z.imag;
        s2 = cmath_sqrt_impl(module, s2);
        r.real = asinh(s1.real*s2.real + s1.imag*s2.imag);
        r.imag = 2.*atan2(s1.imag, s2.real);
    }
    errno = 0;
    return r;
}

/*[clinic input]
cmath.asin = cmath.acos

Return the arc sine of z.
[clinic start generated code]*/

static Py_complex
cmath_asin_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=3b264cd1b16bf4e1 input=be0bf0cfdd5239c5]*/
{
    /* asin(z) = -i asinh(iz) */
    Py_complex s, r;
    s.real = -z.imag;
    s.imag = z.real;
    s = cmath_asinh_impl(module, s);
    r.real = s.imag;
    r.imag = -s.real;
    return r;
}


static Py_complex asinh_special_values[7][7];

/*[clinic input]
cmath.asinh = cmath.acos

Return the inverse hyperbolic sine of z.
[clinic start generated code]*/

static Py_complex
cmath_asinh_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=733d8107841a7599 input=5c09448fcfc89a79]*/
{
    Py_complex s1, s2, r;

    SPECIAL_VALUE(z, asinh_special_values);

    if (fabs(z.real) > CM_LARGE_DOUBLE || fabs(z.imag) > CM_LARGE_DOUBLE) {
        if (z.imag >= 0.) {
            r.real = copysign(log(hypot(z.real/2., z.imag/2.)) +
                              M_LN2*2., z.real);
        } else {
            r.real = -copysign(log(hypot(z.real/2., z.imag/2.)) +
                               M_LN2*2., -z.real);
        }
        r.imag = atan2(z.imag, fabs(z.real));
    } else {
        s1.real = 1.+z.imag;
        s1.imag = -z.real;
        s1 = cmath_sqrt_impl(module, s1);
        s2.real = 1.-z.imag;
        s2.imag = z.real;
        s2 = cmath_sqrt_impl(module, s2);
        r.real = asinh(s1.real*s2.imag-s2.real*s1.imag);
        r.imag = atan2(z.imag, s1.real*s2.real-s1.imag*s2.imag);
    }
    errno = 0;
    return r;
}


/*[clinic input]
cmath.atan = cmath.acos

Return the arc tangent of z.
[clinic start generated code]*/

static Py_complex
cmath_atan_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=b6bfc497058acba4 input=3b21ff7d5eac632a]*/
{
    /* atan(z) = -i atanh(iz) */
    Py_complex s, r;
    s.real = -z.imag;
    s.imag = z.real;
    s = cmath_atanh_impl(module, s);
    r.real = s.imag;
    r.imag = -s.real;
    return r;
}


static Py_complex atanh_special_values[7][7];

/*[clinic input]
cmath.atanh = cmath.acos

Return the inverse hyperbolic tangent of z.
[clinic start generated code]*/

static Py_complex
cmath_atanh_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=e83355f93a989c9e input=2b3fdb82fb34487b]*/
{
    Py_complex r;
    double ay, h;

    SPECIAL_VALUE(z, atanh_special_values);

    /* Reduce to case where z.real >= 0., using atanh(z) = -atanh(-z). */
    if (z.real < 0.) {
        return _Py_c_neg(cmath_atanh_impl(module, _Py_c_neg(z)));
    }

    ay = fabs(z.imag);
    if (z.real > CM_SQRT_LARGE_DOUBLE || ay > CM_SQRT_LARGE_DOUBLE) {
        /*
           if abs(z) is large then we use the approximation
           atanh(z) ~ 1/z +/- i*pi/2 (+/- depending on the sign
           of z.imag)
        */
        h = hypot(z.real/2., z.imag/2.);  /* safe from overflow */
        r.real = z.real/4./h/h;
        r.imag = copysign(Py_MATH_PI/2., z.imag);
        errno = 0;
    } else if (z.real == 1. && ay < CM_SQRT_DBL_MIN) {
        /* C99 standard says:  atanh(1+/-0.) should be inf +/- 0i */
        if (ay == 0.) {
            r.real = INF;
            r.imag = z.imag;
            errno = EDOM;
        } else {
            r.real = -log(sqrt(ay)/sqrt(hypot(ay, 2.)));
            r.imag = copysign(atan2(2., -ay)/2, z.imag);
            errno = 0;
        }
    } else {
        r.real = m_log1p(4.*z.real/((1-z.real)*(1-z.real) + ay*ay))/4.;
        r.imag = -atan2(-2.*z.imag, (1-z.real)*(1+z.real) - ay*ay)/2.;
        errno = 0;
    }
    return r;
}


/*[clinic input]
cmath.cos = cmath.acos

Return the cosine of z.
[clinic start generated code]*/

static Py_complex
cmath_cos_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=fd64918d5b3186db input=6022e39b77127ac7]*/
{
    /* cos(z) = cosh(iz) */
    Py_complex r;
    r.real = -z.imag;
    r.imag = z.real;
    r = cmath_cosh_impl(module, r);
    return r;
}


/* cosh(infinity + i*y) needs to be dealt with specially */
static Py_complex cosh_special_values[7][7];

/*[clinic input]
cmath.cosh = cmath.acos

Return the hyperbolic cosine of z.
[clinic start generated code]*/

static Py_complex
cmath_cosh_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=2e969047da601bdb input=d6b66339e9cc332b]*/
{
    Py_complex r;
    double x_minus_one;

    /* special treatment for cosh(+/-inf + iy) if y is not a NaN */
    if (!isfinite(z.real) || !isfinite(z.imag)) {
        if (isinf(z.real) && isfinite(z.imag) &&
            (z.imag != 0.)) {
            if (z.real > 0) {
                r.real = copysign(INF, cos(z.imag));
                r.imag = copysign(INF, sin(z.imag));
            }
            else {
                r.real = copysign(INF, cos(z.imag));
                r.imag = -copysign(INF, sin(z.imag));
            }
        }
        else {
            r = cosh_special_values[special_type(z.real)]
                                   [special_type(z.imag)];
        }
        /* need to set errno = EDOM if y is +/- infinity and x is not
           a NaN */
        if (isinf(z.imag) && !isnan(z.real))
            errno = EDOM;
        else
            errno = 0;
        return r;
    }

    if (fabs(z.real) > CM_LOG_LARGE_DOUBLE) {
        /* deal correctly with cases where cosh(z.real) overflows but
           cosh(z) does not. */
        x_minus_one = z.real - copysign(1., z.real);
        r.real = cos(z.imag) * cosh(x_minus_one) * Py_MATH_E;
        r.imag = sin(z.imag) * sinh(x_minus_one) * Py_MATH_E;
    } else {
        r.real = cos(z.imag) * cosh(z.real);
        r.imag = sin(z.imag) * sinh(z.real);
    }
    /* detect overflow, and set errno accordingly */
    if (isinf(r.real) || isinf(r.imag))
        errno = ERANGE;
    else
        errno = 0;
    return r;
}


/* exp(infinity + i*y) and exp(-infinity + i*y) need special treatment for
   finite y */
static Py_complex exp_special_values[7][7];

/*[clinic input]
cmath.exp = cmath.acos

Return the exponential value e**z.
[clinic start generated code]*/

static Py_complex
cmath_exp_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=edcec61fb9dfda6c input=8b9e6cf8a92174c3]*/
{
    Py_complex r;
    double l;

    if (!isfinite(z.real) || !isfinite(z.imag)) {
        if (isinf(z.real) && isfinite(z.imag)
            && (z.imag != 0.)) {
            if (z.real > 0) {
                r.real = copysign(INF, cos(z.imag));
                r.imag = copysign(INF, sin(z.imag));
            }
            else {
                r.real = copysign(0., cos(z.imag));
                r.imag = copysign(0., sin(z.imag));
            }
        }
        else {
            r = exp_special_values[special_type(z.real)]
                                  [special_type(z.imag)];
        }
        /* need to set errno = EDOM if y is +/- infinity and x is not
           a NaN and not -infinity */
        if (isinf(z.imag) &&
            (isfinite(z.real) ||
             (isinf(z.real) && z.real > 0)))
            errno = EDOM;
        else
            errno = 0;
        return r;
    }

    if (z.real > CM_LOG_LARGE_DOUBLE) {
        l = exp(z.real-1.);
        r.real = l*cos(z.imag)*Py_MATH_E;
        r.imag = l*sin(z.imag)*Py_MATH_E;
    } else {
        l = exp(z.real);
        r.real = l*cos(z.imag);
        r.imag = l*sin(z.imag);
    }
    /* detect overflow, and set errno accordingly */
    if (isinf(r.real) || isinf(r.imag))
        errno = ERANGE;
    else
        errno = 0;
    return r;
}

static Py_complex log_special_values[7][7];

static Py_complex
c_log(Py_complex z)
{
    /*
       The usual formula for the real part is log(hypot(z.real, z.imag)).
       There are four situations where this formula is potentially
       problematic:

       (1) the absolute value of z is subnormal.  Then hypot is subnormal,
       so has fewer than the usual number of bits of accuracy, hence may
       have large relative error.  This then gives a large absolute error
       in the log.  This can be solved by rescaling z by a suitable power
       of 2.

       (2) the absolute value of z is greater than DBL_MAX (e.g. when both
       z.real and z.imag are within a factor of 1/sqrt(2) of DBL_MAX)
       Again, rescaling solves this.

       (3) the absolute value of z is close to 1.  In this case it's
       difficult to achieve good accuracy, at least in part because a
       change of 1ulp in the real or imaginary part of z can result in a
       change of billions of ulps in the correctly rounded answer.

       (4) z = 0.  The simplest thing to do here is to call the
       floating-point log with an argument of 0, and let its behaviour
       (returning -infinity, signaling a floating-point exception, setting
       errno, or whatever) determine that of c_log.  So the usual formula
       is fine here.

     */

    Py_complex r;
    double ax, ay, am, an, h;

    SPECIAL_VALUE(z, log_special_values);

    ax = fabs(z.real);
    ay = fabs(z.imag);

    if (ax > CM_LARGE_DOUBLE || ay > CM_LARGE_DOUBLE) {
        r.real = log(hypot(ax/2., ay/2.)) + M_LN2;
    } else if (ax < DBL_MIN && ay < DBL_MIN) {
        if (ax > 0. || ay > 0.) {
            /* catch cases where hypot(ax, ay) is subnormal */
            r.real = log(hypot(ldexp(ax, DBL_MANT_DIG),
                     ldexp(ay, DBL_MANT_DIG))) - DBL_MANT_DIG*M_LN2;
        }
        else {
            /* log(+/-0. +/- 0i) */
            r.real = -INF;
            r.imag = atan2(z.imag, z.real);
            errno = EDOM;
            return r;
        }
    } else {
        h = hypot(ax, ay);
        if (0.71 <= h && h <= 1.73) {
            am = ax > ay ? ax : ay;  /* max(ax, ay) */
            an = ax > ay ? ay : ax;  /* min(ax, ay) */
            r.real = m_log1p((am-1)*(am+1)+an*an)/2.;
        } else {
            r.real = log(h);
        }
    }
    r.imag = atan2(z.imag, z.real);
    errno = 0;
    return r;
}


/*[clinic input]
cmath.log10 = cmath.acos

Return the base-10 logarithm of z.
[clinic start generated code]*/

static Py_complex
cmath_log10_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=2922779a7c38cbe1 input=cff5644f73c1519c]*/
{
    Py_complex r;
    int errno_save;

    r = c_log(z);
    errno_save = errno; /* just in case the divisions affect errno */
    r.real = r.real / M_LN10;
    r.imag = r.imag / M_LN10;
    errno = errno_save;
    return r;
}


/*[clinic input]
cmath.sin = cmath.acos

Return the sine of z.
[clinic start generated code]*/

static Py_complex
cmath_sin_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=980370d2ff0bb5aa input=2d3519842a8b4b85]*/
{
    /* sin(z) = -i sin(iz) */
    Py_complex s, r;
    s.real = -z.imag;
    s.imag = z.real;
    s = cmath_sinh_impl(module, s);
    r.real = s.imag;
    r.imag = -s.real;
    return r;
}


/* sinh(infinity + i*y) needs to be dealt with specially */
static Py_complex sinh_special_values[7][7];

/*[clinic input]
cmath.sinh = cmath.acos

Return the hyperbolic sine of z.
[clinic start generated code]*/

static Py_complex
cmath_sinh_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=38b0a6cce26f3536 input=d2d3fc8c1ddfd2dd]*/
{
    Py_complex r;
    double x_minus_one;

    /* special treatment for sinh(+/-inf + iy) if y is finite and
       nonzero */
    if (!isfinite(z.real) || !isfinite(z.imag)) {
        if (isinf(z.real) && isfinite(z.imag)
            && (z.imag != 0.)) {
            if (z.real > 0) {
                r.real = copysign(INF, cos(z.imag));
                r.imag = copysign(INF, sin(z.imag));
            }
            else {
                r.real = -copysign(INF, cos(z.imag));
                r.imag = copysign(INF, sin(z.imag));
            }
        }
        else {
            r = sinh_special_values[special_type(z.real)]
                                   [special_type(z.imag)];
        }
        /* need to set errno = EDOM if y is +/- infinity and x is not
           a NaN */
        if (isinf(z.imag) && !isnan(z.real))
            errno = EDOM;
        else
            errno = 0;
        return r;
    }

    if (fabs(z.real) > CM_LOG_LARGE_DOUBLE) {
        x_minus_one = z.real - copysign(1., z.real);
        r.real = cos(z.imag) * sinh(x_minus_one) * Py_MATH_E;
        r.imag = sin(z.imag) * cosh(x_minus_one) * Py_MATH_E;
    } else {
        r.real = cos(z.imag) * sinh(z.real);
        r.imag = sin(z.imag) * cosh(z.real);
    }
    /* detect overflow, and set errno accordingly */
    if (isinf(r.real) || isinf(r.imag))
        errno = ERANGE;
    else
        errno = 0;
    return r;
}


static Py_complex sqrt_special_values[7][7];

/*[clinic input]
cmath.sqrt = cmath.acos

Return the square root of z.
[clinic start generated code]*/

static Py_complex
cmath_sqrt_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=b6507b3029c339fc input=7088b166fc9a58c7]*/
{
    /*
       Method: use symmetries to reduce to the case when x = z.real and y
       = z.imag are nonnegative.  Then the real part of the result is
       given by

         s = sqrt((x + hypot(x, y))/2)

       and the imaginary part is

         d = (y/2)/s

       If either x or y is very large then there's a risk of overflow in
       computation of the expression x + hypot(x, y).  We can avoid this
       by rewriting the formula for s as:

         s = 2*sqrt(x/8 + hypot(x/8, y/8))

       This costs us two extra multiplications/divisions, but avoids the
       overhead of checking for x and y large.

       If both x and y are subnormal then hypot(x, y) may also be
       subnormal, so will lack full precision.  We solve this by rescaling
       x and y by a sufficiently large power of 2 to ensure that x and y
       are normal.
    */


    Py_complex r;
    double s,d;
    double ax, ay;

    SPECIAL_VALUE(z, sqrt_special_values);

    if (z.real == 0. && z.imag == 0.) {
        r.real = 0.;
        r.imag = z.imag;
        return r;
    }

    ax = fabs(z.real);
    ay = fabs(z.imag);

    if (ax < DBL_MIN && ay < DBL_MIN) {
        /* here we catch cases where hypot(ax, ay) is subnormal */
        ax = ldexp(ax, CM_SCALE_UP);
        s = ldexp(sqrt(ax + hypot(ax, ldexp(ay, CM_SCALE_UP))),
                  CM_SCALE_DOWN);
    } else {
        ax /= 8.;
        s = 2.*sqrt(ax + hypot(ax, ay/8.));
    }
    d = ay/(2.*s);

    if (z.real >= 0.) {
        r.real = s;
        r.imag = copysign(d, z.imag);
    } else {
        r.real = d;
        r.imag = copysign(s, z.imag);
    }
    errno = 0;
    return r;
}


/*[clinic input]
cmath.tan = cmath.acos

Return the tangent of z.
[clinic start generated code]*/

static Py_complex
cmath_tan_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=7c5f13158a72eb13 input=fc167e528767888e]*/
{
    /* tan(z) = -i tanh(iz) */
    Py_complex s, r;
    s.real = -z.imag;
    s.imag = z.real;
    s = cmath_tanh_impl(module, s);
    r.real = s.imag;
    r.imag = -s.real;
    return r;
}


/* tanh(infinity + i*y) needs to be dealt with specially */
static Py_complex tanh_special_values[7][7];

/*[clinic input]
cmath.tanh = cmath.acos

Return the hyperbolic tangent of z.
[clinic start generated code]*/

static Py_complex
cmath_tanh_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=36d547ef7aca116c input=22f67f9dc6d29685]*/
{
    /* Formula:

       tanh(x+iy) = (tanh(x)(1+tan(y)^2) + i tan(y)(1-tanh(x))^2) /
       (1+tan(y)^2 tanh(x)^2)

       To avoid excessive roundoff error, 1-tanh(x)^2 is better computed
       as 1/cosh(x)^2.  When abs(x) is large, we approximate 1-tanh(x)^2
       by 4 exp(-2*x) instead, to avoid possible overflow in the
       computation of cosh(x).

    */

    Py_complex r;
    double tx, ty, cx, txty, denom;

    /* special treatment for tanh(+/-inf + iy) if y is finite and
       nonzero */
    if (!isfinite(z.real) || !isfinite(z.imag)) {
        if (isinf(z.real) && isfinite(z.imag)
            && (z.imag != 0.)) {
            if (z.real > 0) {
                r.real = 1.0;
                r.imag = copysign(0.,
                                  2.*sin(z.imag)*cos(z.imag));
            }
            else {
                r.real = -1.0;
                r.imag = copysign(0.,
                                  2.*sin(z.imag)*cos(z.imag));
            }
        }
        else {
            r = tanh_special_values[special_type(z.real)]
                                   [special_type(z.imag)];
        }
        /* need to set errno = EDOM if z.imag is +/-infinity and
           z.real is finite */
        if (isinf(z.imag) && isfinite(z.real))
            errno = EDOM;
        else
            errno = 0;
        return r;
    }

    /* danger of overflow in 2.*z.imag !*/
    if (fabs(z.real) > CM_LOG_LARGE_DOUBLE) {
        r.real = copysign(1., z.real);
        r.imag = 4.*sin(z.imag)*cos(z.imag)*exp(-2.*fabs(z.real));
    } else {
        tx = tanh(z.real);
        ty = tan(z.imag);
        cx = 1./cosh(z.real);
        txty = tx*ty;
        denom = 1. + txty*txty;
        r.real = tx*(1.+ty*ty)/denom;
        r.imag = ((ty/denom)*cx)*cx;
    }
    errno = 0;
    return r;
}


/*[clinic input]
cmath.log

    z as x: Py_complex
    base as y_obj: object = NULL
    /

log(z[, base]) -> the logarithm of z to the given base.

If the base is not specified, returns the natural logarithm (base e) of z.
[clinic start generated code]*/

static PyObject *
cmath_log_impl(PyObject *module, Py_complex x, PyObject *y_obj)
/*[clinic end generated code: output=4effdb7d258e0d94 input=e1f81d4fcfd26497]*/
{
    Py_complex y;

    errno = 0;
    x = c_log(x);
    if (y_obj != NULL) {
        y = PyComplex_AsCComplex(y_obj);
        if (PyErr_Occurred()) {
            return NULL;
        }
        y = c_log(y);
        x = _Py_c_quot(x, y);
    }
    if (errno != 0)
        return math_error();
    return PyComplex_FromCComplex(x);
}


/* And now the glue to make them available from Python: */

static PyObject *
math_error(void)
{
    if (errno == EDOM)
        PyErr_SetString(PyExc_ValueError, "math domain error");
    else if (errno == ERANGE)
        PyErr_SetString(PyExc_OverflowError, "math range error");
    else    /* Unexpected math error */
        PyErr_SetFromErrno(PyExc_ValueError);
    return NULL;
}


/*[clinic input]
cmath.phase

    z: Py_complex
    /

Return argument, also known as the phase angle, of a complex.
[clinic start generated code]*/

static PyObject *
cmath_phase_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=50725086a7bfd253 input=5cf75228ba94b69d]*/
{
    double phi;

    errno = 0;
    phi = atan2(z.imag, z.real); /* should not cause any exception */
    if (errno != 0)
        return math_error();
    else
        return PyFloat_FromDouble(phi);
}

/*[clinic input]
cmath.polar

    z: Py_complex
    /

Convert a complex from rectangular coordinates to polar coordinates.

r is the distance from 0 and phi the phase angle.
[clinic start generated code]*/

static PyObject *
cmath_polar_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=d0a8147c41dbb654 input=26c353574fd1a861]*/
{
    double r, phi;

    errno = 0;
    phi = atan2(z.imag, z.real); /* should not cause any exception */
    r = _Py_c_abs(z); /* sets errno to ERANGE on overflow */
    if (errno != 0)
        return math_error();
    else
        return Py_BuildValue("dd", r, phi);
}

/*
  rect() isn't covered by the C99 standard, but it's not too hard to
  figure out 'spirit of C99' rules for special value handing:

    rect(x, t) should behave like exp(log(x) + it) for positive-signed x
    rect(x, t) should behave like -exp(log(-x) + it) for negative-signed x
    rect(nan, t) should behave like exp(nan + it), except that rect(nan, 0)
      gives nan +- i0 with the sign of the imaginary part unspecified.

*/

static Py_complex rect_special_values[7][7];

/*[clinic input]
cmath.rect

    r: double
    phi: double
    /

Convert from polar coordinates to rectangular coordinates.
[clinic start generated code]*/

static PyObject *
cmath_rect_impl(PyObject *module, double r, double phi)
/*[clinic end generated code: output=385a0690925df2d5 input=24c5646d147efd69]*/
{
    Py_complex z;
    errno = 0;

    /* deal with special values */
    if (!isfinite(r) || !isfinite(phi)) {
        /* if r is +/-infinity and phi is finite but nonzero then
           result is (+-INF +-INF i), but we need to compute cos(phi)
           and sin(phi) to figure out the signs. */
        if (isinf(r) && (isfinite(phi)
                                  && (phi != 0.))) {
            if (r > 0) {
                z.real = copysign(INF, cos(phi));
                z.imag = copysign(INF, sin(phi));
            }
            else {
                z.real = -copysign(INF, cos(phi));
                z.imag = -copysign(INF, sin(phi));
            }
        }
        else {
            z = rect_special_values[special_type(r)]
                                   [special_type(phi)];
        }
        /* need to set errno = EDOM if r is a nonzero number and phi
           is infinite */
        if (r != 0. && !isnan(r) && isinf(phi))
            errno = EDOM;
        else
            errno = 0;
    }
    else if (phi == 0.0) {
        /* Workaround for buggy results with phi=-0.0 on OS X 10.8.  See
           bugs.python.org/issue18513. */
        z.real = r;
        z.imag = r * phi;
        errno = 0;
    }
    else {
        z.real = r * cos(phi);
        z.imag = r * sin(phi);
        errno = 0;
    }

    if (errno != 0)
        return math_error();
    else
        return PyComplex_FromCComplex(z);
}

/*[clinic input]
@permit_long_summary
cmath.isfinite = cmath.polar

Return True if both the real and imaginary parts of z are finite, else False.
[clinic start generated code]*/

static PyObject *
cmath_isfinite_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=ac76611e2c774a36 input=e224f5c36d94f5da]*/
{
    return PyBool_FromLong(isfinite(z.real) && isfinite(z.imag));
}

/*[clinic input]
cmath.isnan = cmath.polar

Checks if the real or imaginary part of z not a number (NaN).
[clinic start generated code]*/

static PyObject *
cmath_isnan_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=e7abf6e0b28beab7 input=71799f5d284c9baf]*/
{
    return PyBool_FromLong(isnan(z.real) || isnan(z.imag));
}

/*[clinic input]
cmath.isinf = cmath.polar

Checks if the real or imaginary part of z is infinite.
[clinic start generated code]*/

static PyObject *
cmath_isinf_impl(PyObject *module, Py_complex z)
/*[clinic end generated code: output=502a75a79c773469 input=363df155c7181329]*/
{
    return PyBool_FromLong(isinf(z.real) || isinf(z.imag));
}

/*[clinic input]
@permit_long_docstring_body
cmath.isclose -> bool

    a: Py_complex
    b: Py_complex
    *
    rel_tol: double = 1e-09
        maximum difference for being considered "close", relative to the
        magnitude of the input values
    abs_tol: double = 0.0
        maximum difference for being considered "close", regardless of the
        magnitude of the input values

Determine whether two complex numbers are close in value.

Return True if a is close in value to b, and False otherwise.

For the values to be considered close, the difference between them must be
smaller than at least one of the tolerances.

-inf, inf and NaN behave similarly to the IEEE 754 Standard. That is, NaN is
not close to anything, even itself. inf and -inf are only close to themselves.
[clinic start generated code]*/

static int
cmath_isclose_impl(PyObject *module, Py_complex a, Py_complex b,
                   double rel_tol, double abs_tol)
/*[clinic end generated code: output=8a2486cc6e0014d1 input=0d45feea7c626f47]*/
{
    double diff;

    /* sanity check on the inputs */
    if (rel_tol < 0.0 || abs_tol < 0.0 ) {
        PyErr_SetString(PyExc_ValueError,
                        "tolerances must be non-negative");
        return -1;
    }

    if ( (a.real == b.real) && (a.imag == b.imag) ) {
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

    if (isinf(a.real) || isinf(a.imag) || isinf(b.real) || isinf(b.imag)) {
        return 0;
    }

    /* now do the regular computation
       this is essentially the "weak" test from the Boost library
    */

    diff = _Py_c_abs(_Py_c_diff(a, b));

    return (((diff <= rel_tol * _Py_c_abs(b)) ||
             (diff <= rel_tol * _Py_c_abs(a))) ||
            (diff <= abs_tol));
}

PyDoc_STRVAR(module_doc,
"This module provides access to mathematical functions for complex\n"
"numbers.");

static PyMethodDef cmath_methods[] = {
    CMATH_ACOS_METHODDEF
    CMATH_ACOSH_METHODDEF
    CMATH_ASIN_METHODDEF
    CMATH_ASINH_METHODDEF
    CMATH_ATAN_METHODDEF
    CMATH_ATANH_METHODDEF
    CMATH_COS_METHODDEF
    CMATH_COSH_METHODDEF
    CMATH_EXP_METHODDEF
    CMATH_ISCLOSE_METHODDEF
    CMATH_ISFINITE_METHODDEF
    CMATH_ISINF_METHODDEF
    CMATH_ISNAN_METHODDEF
    CMATH_LOG_METHODDEF
    CMATH_LOG10_METHODDEF
    CMATH_PHASE_METHODDEF
    CMATH_POLAR_METHODDEF
    CMATH_RECT_METHODDEF
    CMATH_SIN_METHODDEF
    CMATH_SINH_METHODDEF
    CMATH_SQRT_METHODDEF
    CMATH_TAN_METHODDEF
    CMATH_TANH_METHODDEF
    {NULL, NULL}  /* sentinel */
};

static int
cmath_exec(PyObject *mod)
{
    if (PyModule_Add(mod, "pi", PyFloat_FromDouble(Py_MATH_PI)) < 0) {
        return -1;
    }
    if (PyModule_Add(mod, "e", PyFloat_FromDouble(Py_MATH_E)) < 0) {
        return -1;
    }
    // 2pi
    if (PyModule_Add(mod, "tau", PyFloat_FromDouble(Py_MATH_TAU)) < 0) {
        return -1;
    }
    if (PyModule_Add(mod, "inf", PyFloat_FromDouble(INFINITY)) < 0) {
        return -1;
    }

    Py_complex infj = {0.0, INFINITY};
    if (PyModule_Add(mod, "infj", PyComplex_FromCComplex(infj)) < 0) {
        return -1;
    }
    if (PyModule_Add(mod, "nan", PyFloat_FromDouble(fabs(Py_NAN))) < 0) {
        return -1;
    }
    Py_complex nanj = {0.0, fabs(Py_NAN)};
    if (PyModule_Add(mod, "nanj", PyComplex_FromCComplex(nanj)) < 0) {
        return -1;
    }

    /* initialize special value tables */

#define INIT_SPECIAL_VALUES(NAME, BODY) { Py_complex* p = (Py_complex*)NAME; BODY }
#define C(REAL, IMAG) p->real = REAL; p->imag = IMAG; ++p;

    INIT_SPECIAL_VALUES(acos_special_values, {
      C(P34,INF) C(P,INF)  C(P,INF)  C(P,-INF)  C(P,-INF)  C(P34,-INF) C(N,INF)
      C(P12,INF) C(U,U)    C(U,U)    C(U,U)     C(U,U)     C(P12,-INF) C(N,N)
      C(P12,INF) C(U,U)    C(P12,0.) C(P12,-0.) C(U,U)     C(P12,-INF) C(P12,N)
      C(P12,INF) C(U,U)    C(P12,0.) C(P12,-0.) C(U,U)     C(P12,-INF) C(P12,N)
      C(P12,INF) C(U,U)    C(U,U)    C(U,U)     C(U,U)     C(P12,-INF) C(N,N)
      C(P14,INF) C(0.,INF) C(0.,INF) C(0.,-INF) C(0.,-INF) C(P14,-INF) C(N,INF)
      C(N,INF)   C(N,N)    C(N,N)    C(N,N)     C(N,N)     C(N,-INF)   C(N,N)
    })

    INIT_SPECIAL_VALUES(acosh_special_values, {
      C(INF,-P34) C(INF,-P)  C(INF,-P)  C(INF,P)  C(INF,P)  C(INF,P34) C(INF,N)
      C(INF,-P12) C(U,U)     C(U,U)     C(U,U)    C(U,U)    C(INF,P12) C(N,N)
      C(INF,-P12) C(U,U)     C(0.,-P12) C(0.,P12) C(U,U)    C(INF,P12) C(N,P12)
      C(INF,-P12) C(U,U)     C(0.,-P12) C(0.,P12) C(U,U)    C(INF,P12) C(N,P12)
      C(INF,-P12) C(U,U)     C(U,U)     C(U,U)    C(U,U)    C(INF,P12) C(N,N)
      C(INF,-P14) C(INF,-0.) C(INF,-0.) C(INF,0.) C(INF,0.) C(INF,P14) C(INF,N)
      C(INF,N)    C(N,N)     C(N,N)     C(N,N)    C(N,N)    C(INF,N)   C(N,N)
    })

    INIT_SPECIAL_VALUES(asinh_special_values, {
      C(-INF,-P14) C(-INF,-0.) C(-INF,-0.) C(-INF,0.) C(-INF,0.) C(-INF,P14) C(-INF,N)
      C(-INF,-P12) C(U,U)      C(U,U)      C(U,U)     C(U,U)     C(-INF,P12) C(N,N)
      C(-INF,-P12) C(U,U)      C(-0.,-0.)  C(-0.,0.)  C(U,U)     C(-INF,P12) C(N,N)
      C(INF,-P12)  C(U,U)      C(0.,-0.)   C(0.,0.)   C(U,U)     C(INF,P12)  C(N,N)
      C(INF,-P12)  C(U,U)      C(U,U)      C(U,U)     C(U,U)     C(INF,P12)  C(N,N)
      C(INF,-P14)  C(INF,-0.)  C(INF,-0.)  C(INF,0.)  C(INF,0.)  C(INF,P14)  C(INF,N)
      C(INF,N)     C(N,N)      C(N,-0.)    C(N,0.)    C(N,N)     C(INF,N)    C(N,N)
    })

    INIT_SPECIAL_VALUES(atanh_special_values, {
      C(-0.,-P12) C(-0.,-P12) C(-0.,-P12) C(-0.,P12) C(-0.,P12) C(-0.,P12) C(-0.,N)
      C(-0.,-P12) C(U,U)      C(U,U)      C(U,U)     C(U,U)     C(-0.,P12) C(N,N)
      C(-0.,-P12) C(U,U)      C(-0.,-0.)  C(-0.,0.)  C(U,U)     C(-0.,P12) C(-0.,N)
      C(0.,-P12)  C(U,U)      C(0.,-0.)   C(0.,0.)   C(U,U)     C(0.,P12)  C(0.,N)
      C(0.,-P12)  C(U,U)      C(U,U)      C(U,U)     C(U,U)     C(0.,P12)  C(N,N)
      C(0.,-P12)  C(0.,-P12)  C(0.,-P12)  C(0.,P12)  C(0.,P12)  C(0.,P12)  C(0.,N)
      C(0.,-P12)  C(N,N)      C(N,N)      C(N,N)     C(N,N)     C(0.,P12)  C(N,N)
    })

    INIT_SPECIAL_VALUES(cosh_special_values, {
      C(INF,N) C(U,U) C(INF,0.)  C(INF,-0.) C(U,U) C(INF,N) C(INF,N)
      C(N,N)   C(U,U) C(U,U)     C(U,U)     C(U,U) C(N,N)   C(N,N)
      C(N,0.)  C(U,U) C(1.,0.)   C(1.,-0.)  C(U,U) C(N,0.)  C(N,0.)
      C(N,0.)  C(U,U) C(1.,-0.)  C(1.,0.)   C(U,U) C(N,0.)  C(N,0.)
      C(N,N)   C(U,U) C(U,U)     C(U,U)     C(U,U) C(N,N)   C(N,N)
      C(INF,N) C(U,U) C(INF,-0.) C(INF,0.)  C(U,U) C(INF,N) C(INF,N)
      C(N,N)   C(N,N) C(N,0.)    C(N,0.)    C(N,N) C(N,N)   C(N,N)
    })

    INIT_SPECIAL_VALUES(exp_special_values, {
      C(0.,0.) C(U,U) C(0.,-0.)  C(0.,0.)  C(U,U) C(0.,0.) C(0.,0.)
      C(N,N)   C(U,U) C(U,U)     C(U,U)    C(U,U) C(N,N)   C(N,N)
      C(N,N)   C(U,U) C(1.,-0.)  C(1.,0.)  C(U,U) C(N,N)   C(N,N)
      C(N,N)   C(U,U) C(1.,-0.)  C(1.,0.)  C(U,U) C(N,N)   C(N,N)
      C(N,N)   C(U,U) C(U,U)     C(U,U)    C(U,U) C(N,N)   C(N,N)
      C(INF,N) C(U,U) C(INF,-0.) C(INF,0.) C(U,U) C(INF,N) C(INF,N)
      C(N,N)   C(N,N) C(N,-0.)   C(N,0.)   C(N,N) C(N,N)   C(N,N)
    })

    INIT_SPECIAL_VALUES(log_special_values, {
      C(INF,-P34) C(INF,-P)  C(INF,-P)   C(INF,P)   C(INF,P)  C(INF,P34)  C(INF,N)
      C(INF,-P12) C(U,U)     C(U,U)      C(U,U)     C(U,U)    C(INF,P12)  C(N,N)
      C(INF,-P12) C(U,U)     C(-INF,-P)  C(-INF,P)  C(U,U)    C(INF,P12)  C(N,N)
      C(INF,-P12) C(U,U)     C(-INF,-0.) C(-INF,0.) C(U,U)    C(INF,P12)  C(N,N)
      C(INF,-P12) C(U,U)     C(U,U)      C(U,U)     C(U,U)    C(INF,P12)  C(N,N)
      C(INF,-P14) C(INF,-0.) C(INF,-0.)  C(INF,0.)  C(INF,0.) C(INF,P14)  C(INF,N)
      C(INF,N)    C(N,N)     C(N,N)      C(N,N)     C(N,N)    C(INF,N)    C(N,N)
    })

    INIT_SPECIAL_VALUES(sinh_special_values, {
      C(INF,N) C(U,U) C(-INF,-0.) C(-INF,0.) C(U,U) C(INF,N) C(INF,N)
      C(N,N)   C(U,U) C(U,U)      C(U,U)     C(U,U) C(N,N)   C(N,N)
      C(0.,N)  C(U,U) C(-0.,-0.)  C(-0.,0.)  C(U,U) C(0.,N)  C(0.,N)
      C(0.,N)  C(U,U) C(0.,-0.)   C(0.,0.)   C(U,U) C(0.,N)  C(0.,N)
      C(N,N)   C(U,U) C(U,U)      C(U,U)     C(U,U) C(N,N)   C(N,N)
      C(INF,N) C(U,U) C(INF,-0.)  C(INF,0.)  C(U,U) C(INF,N) C(INF,N)
      C(N,N)   C(N,N) C(N,-0.)    C(N,0.)    C(N,N) C(N,N)   C(N,N)
    })

    INIT_SPECIAL_VALUES(sqrt_special_values, {
      C(INF,-INF) C(0.,-INF) C(0.,-INF) C(0.,INF) C(0.,INF) C(INF,INF) C(N,INF)
      C(INF,-INF) C(U,U)     C(U,U)     C(U,U)    C(U,U)    C(INF,INF) C(N,N)
      C(INF,-INF) C(U,U)     C(0.,-0.)  C(0.,0.)  C(U,U)    C(INF,INF) C(N,N)
      C(INF,-INF) C(U,U)     C(0.,-0.)  C(0.,0.)  C(U,U)    C(INF,INF) C(N,N)
      C(INF,-INF) C(U,U)     C(U,U)     C(U,U)    C(U,U)    C(INF,INF) C(N,N)
      C(INF,-INF) C(INF,-0.) C(INF,-0.) C(INF,0.) C(INF,0.) C(INF,INF) C(INF,N)
      C(INF,-INF) C(N,N)     C(N,N)     C(N,N)    C(N,N)    C(INF,INF) C(N,N)
    })

    INIT_SPECIAL_VALUES(tanh_special_values, {
      C(-1.,0.) C(U,U) C(-1.,-0.) C(-1.,0.) C(U,U) C(-1.,0.) C(-1.,0.)
      C(N,N)    C(U,U) C(U,U)     C(U,U)    C(U,U) C(N,N)    C(N,N)
      C(-0.0,N)    C(U,U) C(-0.,-0.) C(-0.,0.) C(U,U) C(-0.0,N)    C(-0.,N)
      C(0.0,N)    C(U,U) C(0.,-0.)  C(0.,0.)  C(U,U) C(0.0,N)    C(0.,N)
      C(N,N)    C(U,U) C(U,U)     C(U,U)    C(U,U) C(N,N)    C(N,N)
      C(1.,0.)  C(U,U) C(1.,-0.)  C(1.,0.)  C(U,U) C(1.,0.)  C(1.,0.)
      C(N,N)    C(N,N) C(N,-0.)   C(N,0.)   C(N,N) C(N,N)    C(N,N)
    })

    INIT_SPECIAL_VALUES(rect_special_values, {
      C(INF,N) C(U,U) C(-INF,0.) C(-INF,-0.) C(U,U) C(INF,N) C(INF,N)
      C(N,N)   C(U,U) C(U,U)     C(U,U)      C(U,U) C(N,N)   C(N,N)
      C(0.,0.) C(U,U) C(-0.,0.)  C(-0.,-0.)  C(U,U) C(0.,0.) C(0.,0.)
      C(0.,0.) C(U,U) C(0.,-0.)  C(0.,0.)    C(U,U) C(0.,0.) C(0.,0.)
      C(N,N)   C(U,U) C(U,U)     C(U,U)      C(U,U) C(N,N)   C(N,N)
      C(INF,N) C(U,U) C(INF,-0.) C(INF,0.)   C(U,U) C(INF,N) C(INF,N)
      C(N,N)   C(N,N) C(N,0.)    C(N,0.)     C(N,N) C(N,N)   C(N,N)
    })
    return 0;
}

static PyModuleDef_Slot cmath_slots[] = {
    {Py_mod_exec, cmath_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef cmathmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "cmath",
    .m_doc = module_doc,
    .m_size = 0,
    .m_methods = cmath_methods,
    .m_slots = cmath_slots
};

PyMODINIT_FUNC
PyInit_cmath(void)
{
    return PyModuleDef_Init(&cmathmodule);
}
