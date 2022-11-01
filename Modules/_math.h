/* log1p(x) = log(1+x).  The log1p function is designed to avoid the
   significant loss of precision that arises from direct evaluation when x is
   small. Use the substitute from _math.h on all platforms: it includes
   workarounds for buggy handling of zeros.
 */

static double
_Py_log1p(double x)
{
    /* Some platforms supply a log1p function but don't respect the sign of
       zero:  log1p(-0.0) gives 0.0 instead of the correct result of -0.0.

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
