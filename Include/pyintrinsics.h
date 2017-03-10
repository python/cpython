#ifndef Py_PYINTRINSICS_H
#define Py_PYINTRINSICS_H

/* Return the smallest integer k such that n < 2**k, or 0 if n == 0.
 * Equivalent to floor(lg(x))+1.  Also equivalent to: bitwidth_of_type -
 * count_leading_zero_bits(x)
 */

#if defined(__GNUC__) && (((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)) || (__GNUC__ >= 4))
#define HAVE_BIT_LENGTH
static inline unsigned int _Py_bit_length(unsigned long d) {
    return d ? (8 * sizeof(unsigned long) - __builtin_clzl(d)) : 0;
}
#elif defined(_MSC_VER)
#define HAVE_BIT_LENGTH
#pragma intrinsic(_BitScanReverse)
#include <intrin.h>
static inline unsigned int _Py_bit_length(unsigned long d) {
    unsigned long idx;
    if (_BitScanReverse(&idx, d))
        return idx + 1;
    else
        return 0;
}
#else
extern unsigned int _Py_bit_length(unsigned long);
#endif

#endif /* Py_PYINTRINSICS_H */
