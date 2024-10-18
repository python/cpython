#include "Python.h"


#ifdef HAVE_GCC_ASM_FOR_X87
// Inline assembly for getting and setting the 387 FPU control word on
// GCC/x86.
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
#endif  // HAVE_GCC_ASM_FOR_X87

#ifdef HAVE_GCC_ASM_FOR_ARMV7
// Inline assembly for getting and setting the ARMv7-FPSCR control/status word on
// GCC/arm.
#ifdef _Py_MEMORY_SANITIZER
__attribute__((no_sanitize_memory))
#endif
uint32_t _Py_get_fpscr(void) {
    uint32_t fpscr;
    __asm__ __volatile__ ("vmrs %0, fpscr" : "=m" (cw));
    return fpscr;
}

void _Py_set_fpscr(uint32_t cw) {
    __asm__ __volatile__ ("vmsr fpscr, %0" : : "m" (cw));
}
#endif // HAVE_GCC_ASM_FOR_ARMV7
