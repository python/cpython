// #include "Python.h"


// #ifdef HAVE_GCC_ASM_FOR_X87
// // Inline assembly for getting and setting the 387 FPU control word on
// // GCC/x86.
// #ifdef _Py_MEMORY_SANITIZER
// __attribute__((no_sanitize_memory))
// #endif
// unsigned short _Py_get_387controlword(void) {
//     unsigned short cw;
//     __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));
//     return cw;
// }

// void _Py_set_387controlword(unsigned short cw) {
//     __asm__ __volatile__ ("fldcw %0" : : "m" (cw));
// }
// #endif  // HAVE_GCC_ASM_FOR_X87

#include <stdio.h>
#include <math.h>

int isqrt(int n, int round_up) {
    if (n < 0) {
        // Raise ValueError if n is negative
        printf("isqrt() argument must be non-negative\n");
        return -1; // Return an error value
    }
    if (n == 0) {
        return 0; // Special case: square root of 0 is 0
    }

    int sqrt_floor = (int)sqrt(n); // Compute integer square root
    if (round_up) {
        // If round_up is True, return the smallest integer greater than or equal to the square root of n
        return sqrt_floor + ((sqrt_floor * sqrt_floor < n) ? 1 : 0);
    } else {
        // Otherwise, return the floor value of the square root of n
        return sqrt_floor;
    }
}
