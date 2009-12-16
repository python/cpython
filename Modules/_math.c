/* Definitions of some C99 math library functions, for those platforms
   that don't implement these functions already. */

#include <float.h>
#include <math.h>

/* Mathematically, expm1(x) = exp(x) - 1.  The expm1 function is designed
   to avoid the significant loss of precision that arises from direct
   evaluation of the expression exp(x) - 1, for x near 0. */

double
_Py_expm1(double x)
{
    /* For abs(x) >= log(2), it's safe to evaluate exp(x) - 1 directly; this
       also works fine for infinities and nans.

       For smaller x, we can use a method due to Kahan that achieves close to
       full accuracy.
    */

    if (fabs(x) < 0.7) {
        double u;
        u = exp(x);
        if (u == 1.0)
            return x;
        else
            return (u - 1.0) * x / log(u);
    }
    else
        return exp(x) - 1.0;
}
