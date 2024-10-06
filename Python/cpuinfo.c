/*
 * Python CPU SIMD features detection.
 *
 * See https://en.wikipedia.org/wiki/CPUID for details.
 */

#include "Python.h"
#include "pycore_cpuinfo.h"

#define CPUID_REG(ARG)    ARG

/*
 * For simplicity, we only enable SIMD instructions for Intel CPUs,
 * even though we could support ARM NEON and POWER.
 */
#if defined(__x86_64__) && defined(__GNUC__)
#  include <cpuid.h>
#elif defined(_M_X64)
#  include <intrin.h>
#else
#  undef  CPUID_REG
#  define CPUID_REG(ARG)    Py_UNUSED(ARG)
#endif

// AVX2 cannot be compiled on macOS ARM64 (yet it can be compiled on x86_64).
// However, since autoconf incorrectly assumes so when compiling a universal2
// binary, we disable all AVX-related instructions.
#if defined(__APPLE__) && defined(__arm64__)
#  undef CAN_COMPILE_SIMD_AVX_INSTRUCTIONS
#  undef CAN_COMPILE_SIMD_AVX2_INSTRUCTIONS
#  undef CAN_COMPILE_SIMD_AVX512_VBMI_INSTRUCTIONS
#endif

/*
 * The macros below describe masks to apply on CPUID output registers.
 *
 * Each macro is of the form [REGISTER][PAGE]_[FEATURE] where
 *
 * - REGISTER is either EBX, ECX or EDX,
 * - PAGE is either 1 or 7 depending, and
 * - FEATURE is an SIMD instruction set.
 */
#define EDX1_SSE            (1 << 25)   // sse, EDX, page 1, bit 25
#define EDX1_SSE2           (1 << 26)   // sse2, EDX, page 1, bit 26
#define ECX1_SSE3           (1 << 9)    // sse3, ECX, page 1, bit 0
#define ECX1_SSE4_1         (1 << 19)   // sse4.1, ECX, page 1, bit 19
#define ECX1_SSE4_2         (1 << 20)   // sse4.2, ECX, page 1, bit 20
#define ECX1_AVX            (1 << 28)   // avx, ECX, page 1, bit 28
#define EBX7_AVX2           (1 << 5)    // avx2, EBX, page 7, bit 5
#define ECX7_AVX512_VBMI    (1 << 1)    // avx512-vbmi, ECX, page 7, bit 1

#define CHECK_CPUID_REGISTER(REGISTER, MASK) ((REGISTER) & (MASK)) == 0 ? 0 : 1

/*
 * Indicate whether the CPUID input EAX=1 may be needed to
 * detect SIMD basic features (e.g., SSE).
 */
#if defined(CAN_COMPILE_SIMD_SSE_INSTRUCTIONS)              \
    || defined(CAN_COMPILE_SIMD_SSE2_INSTRUCTIONS)          \
    || defined(CAN_COMPILE_SIMD_SSE3_INSTRUCTIONS)          \
    || defined(CAN_COMPILE_SIMD_SSE4_1_INSTRUCTIONS)        \
    || defined(CAN_COMPILE_SIMD_SSE4_2_INSTRUCTIONS)        \
    || defined(CAN_COMPILE_SIMD_AVX_INSTRUCTIONS)
#  define MAY_DETECT_CPUID_SIMD_FEATURES
#endif

/*
 * Indicate whether the CPUID input EAX=7 may be needed to
 * detect SIMD extended features (e.g., AVX2 or AVX-512).
 */
#if defined(CAN_COMPILE_SIMD_AVX2_INSTRUCTIONS)             \
    || defined(CAN_COMPILE_SIMD_AVX512_VBMI_INSTRUCTIONS)
#  define MAY_DETECT_CPUID_SIMD_EXTENDED_FEATURES
#endif

static inline void
get_cpuid_info(int32_t level /* input eax */,
               int32_t count /* input ecx */,
               int32_t *CPUID_REG(eax),
               int32_t *CPUID_REG(ebx),
               int32_t *CPUID_REG(ecx),
               int32_t *CPUID_REG(edx))
{
#if defined(__x86_64__) && defined(__GNUC__)
    __cpuid_count(level, count, *eax, *ebx, *ecx, *edx);
#elif defined(_M_X64)
    int32_t info[4] = {0};
    __cpuidex(info, level, count);
    *eax = info[0];
    *ebx = info[1];
    *ecx = info[2];
    *edx = info[3];
#endif
}

/* Processor Info and Feature Bits (EAX=1, ECX=0). */
static inline void
detect_cpu_simd_features(py_cpu_simd_flags *flags)
{
    int32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    get_cpuid_info(1, 0, &eax, &ebx, &ecx, &edx);
#ifdef CAN_COMPILE_SIMD_SSE_INSTRUCTIONS
    flags->sse = CHECK_CPUID_REGISTER(edx, EDX1_SSE);
#endif
#ifdef CAN_COMPILE_SIMD_SSE2_INSTRUCTIONS
    flags->sse2 = CHECK_CPUID_REGISTER(edx, EDX1_SSE2);
#endif
#ifdef CAN_COMPILE_SIMD_SSE3_INSTRUCTIONS
    flags->sse3 = CHECK_CPUID_REGISTER(ecx, ECX1_SSE3);
#endif
#ifdef CAN_COMPILE_SIMD_SSE4_1_INSTRUCTIONS
    flags->sse41 = CHECK_CPUID_REGISTER(ecx, ECX1_SSE4_1);
#endif
#ifdef CAN_COMPILE_SIMD_SSE4_2_INSTRUCTIONS
    flags->sse42 = CHECK_CPUID_REGISTER(ecx, ECX1_SSE4_2);
#endif
#ifdef CAN_COMPILE_SIMD_AVX_INSTRUCTIONS
    flags->avx = CHECK_CPUID_REGISTER(ecx, ECX1_AVX);
#endif
}

/* Extended Feature Bits (EAX=7, ECX=0). */
static inline void
detect_cpu_simd_extended_features(py_cpu_simd_flags *flags)
{
    int32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    get_cpuid_info(7, 0, &eax, &ebx, &ecx, &edx);
#ifdef CAN_COMPILE_SIMD_AVX2_INSTRUCTIONS
    flags->avx2 = CHECK_CPUID_REGISTER(ebx, EBX7_AVX2);
#endif
#ifdef CAN_COMPILE_SIMD_AVX512_VBMI_INSTRUCTIONS
    flags->avx512vbmi = CHECK_CPUID_REGISTER(ecx, ECX7_AVX512_VBMI);
#endif
}

void
_Py_detect_cpu_simd_features(py_cpu_simd_flags *flags)
{
    if (flags->done) {
        return;
    }
#ifdef MAY_DETECT_CPUID_SIMD_FEATURES
    detect_cpu_simd_features(flags);
#else
    flags->sse = flags->sse2 = flags->sse3 = flags->sse41 = flags->sse42 = 0;
    flags->avx = 0;
#endif
#ifdef MAY_DETECT_CPUID_SIMD_EXTENDED_FEATURES
    detect_cpu_simd_extended_features(flags);
#else
    flags->avx2 = flags->avx512vbmi = 0;
#endif
    flags->done = 1;
}

void
_Py_extend_cpu_simd_features(py_cpu_simd_flags *out,
                             const py_cpu_simd_flags *src)
{
#define UPDATE(FLAG)   out->FLAG |= src->FLAG
    UPDATE(sse);
    UPDATE(sse2);
    UPDATE(sse3);
    UPDATE(sse41);
    UPDATE(sse42);

    UPDATE(avx);
    UPDATE(avx2);
    UPDATE(avx512vbmi);
#undef UPDATE
    out->done = 1;
}
