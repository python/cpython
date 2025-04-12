/* Workarounds for buggy complex number arithmetic implementations. */

#ifndef Py_HAVE_C_COMPLEX
#  error "this header file should only be included if Py_HAVE_C_COMPLEX is defined"
#endif

#include <complex.h>

/* Other compilers (than clang), that claims to
   implement C11 *and* define __STDC_IEC_559_COMPLEX__ - don't have
   issue with CMPLX().  This is specific to glibc & clang combination:
   https://sourceware.org/bugzilla/show_bug.cgi?id=26287

   Here we fallback to using __builtin_complex(), available in clang
   v12+.  Else CMPLX implemented following C11 6.2.5p13: "Each complex type
   has the same representation and alignment requirements as an array
   type containing exactly two elements of the corresponding real type;
   the first element is equal to the real part, and the second element
   to the imaginary part, of the complex number.
 */
#if !defined(CMPLX)
#  if defined(__clang__) && __has_builtin(__builtin_complex)
#    define CMPLX(x, y) __builtin_complex ((double) (x), (double) (y))
#    define CMPLXF(x, y) __builtin_complex ((float) (x), (float) (y))
#    define CMPLXL(x, y) __builtin_complex ((long double) (x), (long double) (y))
#  else
static inline double complex
CMPLX(double real, double imag)
{
    double complex z;
    ((double *)(&z))[0] = real;
    ((double *)(&z))[1] = imag;
    return z;
}

static inline float complex
CMPLXF(float real, float imag)
{
    float complex z;
    ((float *)(&z))[0] = real;
    ((float *)(&z))[1] = imag;
    return z;
}

static inline long double complex
CMPLXL(long double real, long double imag)
{
    long double complex z;
    ((long double *)(&z))[0] = real;
    ((long double *)(&z))[1] = imag;
    return z;
}
#  endif
#endif
