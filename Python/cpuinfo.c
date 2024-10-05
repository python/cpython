/*
 * Naive CPU SIMD features detection.
 *
 * See Modules/black2module.c.
 */

#include "Python.h"
#include "pycore_cpuinfo.h"

#include <stdbool.h>

#if defined(__x86_64__) && defined(__GNUC__)
#include <cpuid.h>
#elif defined(_M_X64)
#include <intrin.h>
#endif

// AVX2 cannot be compiled on macOS ARM64 (yet it can be compiled on x86_64).
// However, since autoconf incorrectly assumes so when compiling a universal2
// binary, we disable all AVX-related instructions.
#if defined(__APPLE__) && defined(__arm64__)
#  undef CAN_COMPILE_SIMD_AVX_INSTRUCTIONS
#  undef CAN_COMPILE_SIMD_AVX2_INSTRUCTIONS
#  undef CAN_COMPILE_SIMD_AVX512_VBMI_INSTRUCTIONS
#endif

#define EDX1_SSE            (1 << 25)   // sse, EDX, page 1, bit 25
#define EDX1_SSE2           (1 << 26)   // sse2, EDX, page 1, bit 26
#define ECX1_SSE3           (1 << 9)    // sse3, ECX, page 1, bit 0
#define ECX1_SSE4_1         (1 << 19)   // sse4.1, ECX, page 1, bit 19
#define ECX1_SSE4_2         (1 << 20)   // sse4.2, ECX, page 1, bit 20
#define ECX1_AVX            (1 << 28)   // avx, ECX, page 1, bit 28
#define EBX7_AVX2           (1 << 5)    // avx2, EBX, page 7, bit 5
#define ECX7_AVX512_VBMI    (1 << 1)    // avx512-vbmi, ECX, page 7, bit 1

void
detect_cpu_simd_features(cpu_simd_flags *flags)
{
    if (flags->done) {
        return;
    }

    int eax1 = 0, ebx1 = 0, ecx1 = 0, edx1 = 0;
    int eax7 = 0, ebx7 = 0, ecx7 = 0, edx7 = 0;
#if defined(__x86_64__) && defined(__GNUC__)
    __cpuid_count(1, 0, eax1, ebx1, ecx1, edx1);
    __cpuid_count(7, 0, eax7, ebx7, ecx7, edx7);
#elif defined(_M_X64)
    int info1[4] = {0};
    __cpuidex(info1, 1, 0);
    eax1 = info1[0];
    ebx1 = info1[1];
    ecx1 = info1[2];
    edx1 = info1[3];

    int info7[4] = {0};
    __cpuidex(info7, 7, 0);
    eax7 = info7[0];
    ebx7 = info7[1];
    ecx7 = info7[2];
    edx7 = info7[3];
#else
    // use (void) expressions to avoid warnings
    (void) eax1; (void) ebx1; (void) ecx1; (void) edx1;
    (void) eax7; (void) ebx7; (void) ecx7; (void) edx7;
#endif

#ifdef CAN_COMPILE_SIMD_SSE_INSTRUCTIONS
    flags->sse = (edx1 & EDX1_SSE) != 0;
#else
    flags->sse = false;
#endif
#ifdef CAN_COMPILE_SIMD_SSE2_INSTRUCTIONS
    flags->sse2 = (edx1 & EDX1_SSE2) != 0;
#else
    flags->sse2 = false;
#endif
#ifdef CAN_COMPILE_SIMD_SSE3_INSTRUCTIONS
    flags->sse3 = (ecx1 & ECX1_SSE3) != 0;
    #else
#endif
    flags->sse3 = false;
#ifdef CAN_COMPILE_SIMD_SSE4_1_INSTRUCTIONS
    flags->sse41 = (ecx1 & ECX1_SSE4_1) != 0;
#else
    flags->sse41 = false;
#endif
#ifdef CAN_COMPILE_SIMD_SSE4_2_INSTRUCTIONS
    flags->sse42 = (ecx1 & ECX1_SSE4_2) != 0;
#else
    flags->sse42 = false;
#endif
#ifdef CAN_COMPILE_SIMD_AVX_INSTRUCTIONS
    flags->avx = (ecx1 & ECX1_AVX) != 0;
#else
    flags->avx = false;
#endif
#ifdef CAN_COMPILE_SIMD_AVX2_INSTRUCTIONS
    flags->avx2 = (ebx7 & EBX7_AVX2) != 0;
#else
    flags->avx2 = false;
#endif
#ifdef CAN_COMPILE_SIMD_AVX512_VBMI_INSTRUCTIONS
    flags->avx512vbmi = (ecx7 & ECX7_AVX512_VBMI) != 0;
#else
    flags->avx512vbmi = false;
#endif

    flags->done = true;
}
