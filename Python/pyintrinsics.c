#include "Python.h"

#ifndef HAVE_BIT_LENGTH
static const unsigned char BitLengthTable[32] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

unsigned int _Py_bit_length(unsigned long d) {
    unsigned int d_bits = 0;
    while (d >= 32) {
        d_bits += 6;
        d >>= 6;
    }
    d_bits += (unsigned int)BitLengthTable[d];
    return d_bits;
}
#endif /* HAVE_BIT_LENGTH */
