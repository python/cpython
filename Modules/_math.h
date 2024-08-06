/* log1p(x) = log(1+x).  The log1p function is designed to avoid the
   significant loss of precision that arises from direct evaluation when x is
   small. Use the substitute from _math.h on all platforms: it includes
   workarounds for buggy handling of zeros.
 */

static double
_Py_log1p(double x)
{
    /* Some platforms (e.g. MacOS X 10.8, see gh-59682) supply a log1p function
       but don't respect the sign of zero:  log1p(-0.0) gives 0.0 instead of
       the correct result of -0.0.

       To save fiddling with configure tests and platform checks, we handle the
       special case of zero input directly on all platforms.
    */
    if (x == 0.0) {
        return x;
    }
    else {
        return log1p(x);
    }
}

#define m_log1p _Py_log1p

/*
   wrapper for atan2 that deals directly with special cases before
   delegating to the platform libm for the remaining cases.  This
   is necessary to get consistent behaviour across platforms.
   Windows, FreeBSD and alpha Tru64 are amongst platforms that don't
   always follow C99.  Windows screws up atan2 for inf and nan, and
   alpha Tru64 5.1 doesn't follow C99 for atan2(0., 0.).
*/

static double
_Py_atan2(double y, double x)
{
    if (isnan(x) || isnan(y))
        return Py_NAN;
    if (isinf(y)) {
        if (isinf(x)) {
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
    if (isinf(x) || y == 0.) {
        if (copysign(1., x) == 1.)
            /* atan2(+-y, +inf) = atan2(+-0, +x) = +-0. */
            return copysign(0., y);
        else
            /* atan2(+-y, -inf) = atan2(+-0., -x) = +-pi. */
            return copysign(Py_MATH_PI, y);
    }
    return atan2(y, x);
}

#define m_atan2 _Py_atan2
