#include "Python.h"

#ifdef X87_DOUBLE_ROUNDING
/* On x86 platforms using an x87 FPU, this function is called from the
   Py_FORCE_DOUBLE macro (defined in pymath.h) to force a floating-point
   number out of an 80-bit x87 FPU register and into a 64-bit memory location,
   thus rounding from extended precision to double precision. */
double _Py_force_double(double x)
{
    volatile double y;
    y = x;
    return y;
}
#endif

#ifdef HAVE_GCC_ASM_FOR_X87

/* inline assembly for getting and setting the 387 FPU control word on
   gcc/x86 */
#ifdef _Py_MEMORY_SANITIZER
__attribute__((no_sanitize_memory))
#endif
unsigned short _Py_get_387controlword(void) {
    unsigned short cw;
    __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));
    return cw;
}

void _Py_set_387controlword(unsigned short cw) {
    __asm__ __volatile__ ("fldcw %0" : : "m" (cw));
}

#endif


#ifndef HAVE_HYPOT
double hypot(double x, double y)
{
    double yx;

    x = fabs(x);
    y = fabs(y);
    if (x < y) {
        double temp = x;
        x = y;
        y = temp;
    }
    if (x == 0.)
        return 0.;
    else {
        yx = y/x;
        return x*sqrt(1.+yx*yx);
    }
}
#endif /* HAVE_HYPOT */

#ifndef HAVE_COPYSIGN
double
copysign(double x, double y)
{
    /* use atan2 to distinguish -0. from 0. */
    if (y > 0. || (y == 0. && atan2(y, -1.) > 0.)) {
        return fabs(x);
    } else {
        return -fabs(x);
    }
}
#endif /* HAVE_COPYSIGN */

#ifndef HAVE_ROUND
double
round(double x)
{
    double absx, y;
    absx = fabs(x);
    y = floor(absx);
    if (absx - y >= 0.5)
        y += 1.0;
    return copysign(y, x);
}
#endif /* HAVE_ROUND */

#if defined(HAVE_BUILTIN_CLZL) || defined(HAVE_BUILTIN_BSRL)

#include <limits.h>

#  ifdef MS_WINDOWS
#    include <intrin.h>
#    pragma intrinsic(_BitScanReverse)

static inline int
__builtin_clzl(unsigned long x)
 {
   unsigned long clz = 0;
   _BitScanReverse (&clz, x);
   return (clz + 1);
 }
#  endif /* MS_WINDOWS */

unsigned int _Py_bit_length(unsigned long d) {
   return d ? CHAR_BIT * sizeof (d) - __builtin_clzl (d) : 0;
}

#else /* !(defined(HAVE_BUILTIN_CLZL) || defined(HAVE_BUILTIN_BSRL)) */

unsigned int _Py_bit_length(unsigned long d) {
#if SIZEOF_LONG > 4
#  if SIZEOF_LONG > 8
#  error _Py_bit_length should be fixed for sizeof (unsigned long) > 8
#  endif
  
  int shift = (d >> (1 << 5) != 0) << 5;
  unsigned int ui_value = d >> shift;
#else /* 32 bits and less */
  int shift = 0;
  unsigned int ui_value = d;
#endif /* 64/32 bits */
  int bits = shift;

  shift = (ui_value >> (1 << 4) != 0) << 4;
  bits |= shift;
  ui_value >>= shift;

  shift = (ui_value >> (1 << 3) != 0) << 3;
  bits |= shift;
  ui_value >>= shift;

  shift = (ui_value >> (1 << 2) != 0) << 2;
  bits |= shift;
  ui_value >>= shift;

  shift = (ui_value >> (1 << 1) != 0) << 1;
  bits |= shift;
  ui_value >>= shift;

  bits += ui_value ^ (ui_value > 2);
  return (bits);
}

#endif /* defined(HAVE_BUILTIN_CLZL) || defined(HAVE_BUILTIN_BSRL) */
