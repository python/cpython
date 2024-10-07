/*
 * Python CPU SIMD features detection.
 *
 * See https://en.wikipedia.org/wiki/CPUID for details.
 */

/*
 * In order to properly maintain this file, the following rules should
 * be observed and enforced if possible:
 *
 * - Defining the SIMD_*_INSTRUCTIONS_DETECTION_GUARD macros should
 */

#include "Python.h"
#include "pycore_cpuinfo.h"

#include <stdint.h>     // UINT32_C()

/* Macro to mark a CPUID register function parameter as being used. */
#define CPUID_REG(PARAM)                PARAM
/* Macro to check one or more CPUID register bits. */
#define CPUID_CHECK_REG(REG, MASK)  ((((REG) & (MASK)) == (MASK)) ? 0 : 1)

/*
 * For simplicity, we only enable SIMD instructions for Intel CPUs,
 * even though we could support ARM NEON and POWER.
 */
#if defined(__x86_64__) && defined(__GNUC__)
#  include <cpuid.h>    // __cpuid_count()
#elif defined(_M_X64)
#  include <intrin.h>   // __cpuidex()
#else
#  undef  CPUID_REG
#  define CPUID_REG(PARAM)  Py_UNUSED(PARAM)
#endif

#if defined(CAN_COMPILE_SIMD_SSE_INSTRUCTIONS)              \
    || defined(CAN_COMPILE_SIMD_SSE2_INSTRUCTIONS)          \
    || defined(CAN_COMPILE_SIMD_SSE3_INSTRUCTIONS)          \
    || defined(CAN_COMPILE_SIMD_SSSE3_INSTRUCTIONS)         \
    || defined(CAN_COMPILE_SIMD_SSE4_1_INSTRUCTIONS)        \
    || defined(CAN_COMPILE_SIMD_SSE4_2_INSTRUCTIONS)        \
    // macros above should be sorted in an alphabetical order
/* Used to guard any SSE instructions detection code. */
#  define SIMD_SSE_INSTRUCTIONS_DETECTION_GUARD
#endif

#if defined(CAN_COMPILE_SIMD_AVX_INSTRUCTIONS)                  \
    || defined(CAN_COMPILE_SIMD_AVX_IFMA_INSTRUCTIONS)          \
    || defined(CAN_COMPILE_SIMD_AVX_NE_CONVERT_INSTRUCTIONS)    \
    || defined(CAN_COMPILE_SIMD_AVX_VNNI_INSTRUCTIONS)          \
    || defined(CAN_COMPILE_SIMD_AVX_VNNI_INT8_INSTRUCTIONS)     \
    || defined(CAN_COMPILE_SIMD_AVX_VNNI_INT16_INSTRUCTIONS)    \
    // macros above should be sorted in an alphabetical order
/* Used to guard any AVX instructions detection code. */
#  define SIMD_AVX_INSTRUCTIONS_DETECTION_GUARD
#endif

#if defined(CAN_COMPILE_SIMD_AVX2_INSTRUCTIONS)                 \
    // macros above should be sorted in an alphabetical order
/* Used to guard any AVX-2 instructions detection code. */
#  define SIMD_AVX2_INSTRUCTIONS_DETECTION_GUARD
#endif

#if defined(CAN_COMPILE_SIMD_AVX512_BITALG_INSTRUCTIONS)            \
    || defined(CAN_COMPILE_SIMD_AVX512_BW_INSTRUCTIONS)             \
    || defined(CAN_COMPILE_SIMD_AVX512_CD_INSTRUCTIONS)             \
    || defined(CAN_COMPILE_SIMD_AVX512_DQ_INSTRUCTIONS)             \
    || defined(CAN_COMPILE_SIMD_AVX512_ER_INSTRUCTIONS)             \
    || defined(CAN_COMPILE_SIMD_AVX512_F_INSTRUCTIONS)              \
    || defined(CAN_COMPILE_SIMD_AVX512_IFMA_INSTRUCTIONS)           \
    || defined(CAN_COMPILE_SIMD_AVX512_PF_INSTRUCTIONS)             \
    || defined(CAN_COMPILE_SIMD_AVX512_VBMI_INSTRUCTIONS)           \
    || defined(CAN_COMPILE_SIMD_AVX512_VBMI2_INSTRUCTIONS)          \
    || defined(CAN_COMPILE_SIMD_AVX512_VL_INSTRUCTIONS)             \
    || defined(CAN_COMPILE_SIMD_AVX512_VNNI_INSTRUCTIONS)           \
    || defined(CAN_COMPILE_SIMD_AVX512_VP2INTERSECT_INSTRUCTIONS)   \
    || defined(CAN_COMPILE_SIMD_AVX512_VPOPCNTDQ_INSTRUCTIONS)      \
    || defined(CAN_COMPILE_SIMD_AVX512_4FMAPS_INSTRUCTIONS)         \
    || defined(CAN_COMPILE_SIMD_AVX512_4VNNIW_INSTRUCTIONS)         \
    // macros above should be sorted in an alphabetical order
/* Used to guard any AVX-512 instructions detection code. */
#  define SIMD_AVX512_INSTRUCTIONS_DETECTION_GUARD
#endif

// On macOS, checking the XCR0 register is NOT a guaranteed way
// to ensure the usability of AVX-512. As such, we disable the
// entire set of AVX-512 instructions.
//
// See https://stackoverflow.com/a/72523150/9579194.
//
// Additionally, AVX2 cannot be compiled on macOS ARM64 (yet it can be
// compiled on x86_64). However, since autoconf incorrectly assumes so
// when compiling a universal2 binary, we disable AVX for such builds.
#if defined(__APPLE__)
#  undef SIMD_AVX512_INSTRUCTIONS_DETECTION_GUARD
#  if defined(__arm64__)
#    undef SIMD_AVX_INSTRUCTIONS_DETECTION_GUARD
#    undef SIMD_AVX2_INSTRUCTIONS_DETECTION_GUARD
#  endif
#endif

#if defined(SIMD_SSE_INSTRUCTIONS_DETECTION_GUARD)      \
    || defined(SIMD_AVX_INSTRUCTIONS_DETECTION_GUARD)
/* Indicate that cpuid should be called once with EAX=1 and ECX=0. */
#  define SHOULD_DETECT_SIMD_FEATURES_L1
#endif

#if defined(SIMD_AVX2_INSTRUCTIONS_DETECTION_GUARD)         \
    || defined(SIMD_AVX512_INSTRUCTIONS_DETECTION_GUARD)
/* Indicate that cpuid should be called once with EAX=7 and ECX=0. */
#  define SHOULD_DETECT_SIMD_FEATURES_L7
#  define SHOULD_DETECT_SIMD_FEATURES_L7S0
#endif

#if defined(SIMD_AVX_INSTRUCTIONS_DETECTION_GUARD)
/* Indicate that cpuid should be called once with EAX=7 and ECX=1. */
#  define SHOULD_DETECT_SIMD_FEATURES_L7
#  define SHOULD_DETECT_SIMD_FEATURES_L7S1
#endif

/*
 * The macros below describe masks to apply on CPUID output registers.
 *
 * Each macro is of the form <REGISTER>_L<LEAF>[S<SUBLEAF>]_<FEATURE>,
 * where <> (resp. []) denotes a required (resp. optional) group and:
 *
 * - REGISTER is EAX, EBX, ECX or EDX,
 * - LEAF is the initial value of the EAX register (1 or 7),
 * - SUBLEAF is the initial value of the ECX register (omitted if 0), and
 * - FEATURE is a SIMD feature (with one or more specialized instructions).
 *
 * For maintainability, the flags are ordered by registers, leafs, subleafs,
 * and bits. See https://en.wikipedia.org/wiki/CPUID for the values.
 *
 * Note 1: The LEAF is also called the 'page' or the 'level'.
 * Note 2: The SUBLEAF is also referred to as the 'count'.
 */

/* CPUID (LEAF=1, SUBLEAF=0) */
#define ECX_L1_SSE3                 (UINT32_C(1) << 0)
#define ECX_L1_SSSE3                (UINT32_C(1) << 9)
#define ECX_L1_SSE4_1               (UINT32_C(1) << 19)
#define ECX_L1_SSE4_2               (UINT32_C(1) << 20)
#define ECX_L1_OSXSAVE              (UINT32_C(1) << 27)
#define ECX_L1_AVX                  (UINT32_C(1) << 28)

#define EDX_L1_SSE                  (UINT32_C(1) << 25)
#define EDX_L1_SSE2                 (UINT32_C(1) << 26)

/* CPUID (LEAF=7, SUBLEAF=0) */
#define EBX_L7_AVX2                 (UINT32_C(1) << 5)
#define EBX_L7_AVX512_F             (UINT32_C(1) << 16)
#define EBX_L7_AVX512_DQ            (UINT32_C(1) << 17)
#define EBX_L7_AVX512_IFMA          (UINT32_C(1) << 21)
#define EBX_L7_AVX512_PF            (UINT32_C(1) << 26)
#define EBX_L7_AVX512_ER            (UINT32_C(1) << 27)
#define EBX_L7_AVX512_CD            (UINT32_C(1) << 28)
#define EBX_L7_AVX512_BW            (UINT32_C(1) << 30)
#define EBX_L7_AVX512_VL            (UINT32_C(1) << 31)

#define ECX_L7_AVX512_VBMI          (UINT32_C(1) << 1)
#define ECX_L7_AVX512_VBMI2         (UINT32_C(1) << 6)
#define ECX_L7_AVX512_VNNI          (UINT32_C(1) << 11)
#define ECX_L7_AVX512_BITALG        (UINT32_C(1) << 12)
#define ECX_L7_AVX512_VPOPCNTDQ     (UINT32_C(1) << 14)

#define EDX_L7_AVX512_4VNNIW        (UINT32_C(1) << 2)
#define EDX_L7_AVX512_4FMAPS        (UINT32_C(1) << 3)
#define EDX_L7_AVX512_VP2INTERSECT  (UINT32_C(1) << 8)

/* CPUID (LEAF=7, SUBLEAF=1) */
#define EAX_L7S1_AVX_VNNI           (UINT32_C(1) << 4)
#define EAX_L7S1_AVX_IFMA           (UINT32_C(1) << 23)

#define EDX_L7S1_AVX_VNNI_INT8      (UINT32_C(1) << 4)
#define EDX_L7S1_AVX_NE_CONVERT     (UINT32_C(1) << 5)
#define EDX_L7S1_AVX_VNNI_INT16     (UINT32_C(1) << 10)

static inline void
get_cpuid_info(uint32_t level /* input eax */,
               uint32_t count /* input ecx */,
               uint32_t *CPUID_REG(eax),
               uint32_t *CPUID_REG(ebx),
               uint32_t *CPUID_REG(ecx),
               uint32_t *CPUID_REG(edx))
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

/* XSAVE State Components. */
#define XCR0_SSE                    (UINT32_C(1) << 1)
#define XCR0_AVX                    (UINT32_C(1) << 2)
#define XCR0_AVX512_OPMASK          (UINT32_C(1) << 5)
#define XCR0_AVX512_ZMM_HI256       (UINT32_C(1) << 6)
#define XCR0_AVX512_HI16_ZMM        (UINT32_C(1) << 7)

static inline uint64_t
get_xgetbv(uint32_t index)
{
#if defined(__x86_64__) && defined(__GNUC__)
    uint32_t eax = 0, edx = 0;
    __asm__ __volatile__("xgetbv" : "=a" (eax), "=d" (edx) : "c" (index));
    return ((uint64_t)edx << 32) | eax;
#elif defined (_MSC_VER)
    return (uint64_t)_xgetbv(index);
#else
    (void) index;
    return 0;
#endif
}

/* Highest Function Parameter and Manufacturer ID (LEAF=0, SUBLEAF=0). */
static inline uint32_t
detect_cpuid_maxleaf(void)
{
    uint32_t maxlevel = 0, ebx = 0, ecx = 0, edx = 0;
    get_cpuid_info(0, 0, &maxlevel, &ebx, &ecx, &edx);
    return maxlevel;
}

/* Processor Info and Feature Bits (LEAF=1, SUBLEAF=0). */
static inline void
detect_simd_features(py_simd_features *flags,
                     uint32_t eax, uint32_t ebx,
                     uint32_t ecx, uint32_t edx)
{
    // Keep the ordering and newlines as they are declared in the structure.
#ifdef SIMD_SSE_INSTRUCTIONS_DETECTION_GUARD
#ifdef CAN_COMPILE_SIMD_SSE_INSTRUCTIONS
    flags->sse = CPUID_CHECK_REG(edx, EDX_L1_SSE);
#endif
#ifdef CAN_COMPILE_SIMD_SSE2_INSTRUCTIONS
    flags->sse2 = CPUID_CHECK_REG(edx, EDX_L1_SSE2);
#endif
#ifdef CAN_COMPILE_SIMD_SSE3_INSTRUCTIONS
    flags->sse3 = CPUID_CHECK_REG(ecx, ECX_L1_SSE3);
#endif
#ifdef CAN_COMPILE_SIMD_SSSE3_INSTRUCTIONS
    flags->ssse3 = CPUID_CHECK_REG(ecx, ECX_L1_SSSE3);
#endif
#ifdef CAN_COMPILE_SIMD_SSE4_1_INSTRUCTIONS
    flags->sse41 = CPUID_CHECK_REG(ecx, ECX_L1_SSE4_1);
#endif
#ifdef CAN_COMPILE_SIMD_SSE4_2_INSTRUCTIONS
    flags->sse42 = CPUID_CHECK_REG(ecx, ECX_L1_SSE4_2);
#endif
#endif

#ifdef SIMD_AVX_INSTRUCTIONS_DETECTION_GUARD
#ifdef CAN_COMPILE_SIMD_AVX_INSTRUCTIONS
    flags->avx = CPUID_CHECK_REG(ecx, ECX_L1_AVX);
#endif
#endif

    flags->os_xsave = CPUID_CHECK_REG(ecx, ECX_L1_OSXSAVE);
}

/* Extended Feature Bits (LEAF=7, SUBLEAF=0). */
static inline void
detect_simd_extended_features_ecx_0(py_simd_features *flags,
                                    uint8_t eax, uint8_t ebx,
                                    uint8_t ecx, uint8_t edx)
{
    // Keep the ordering and newlines as they are declared in the structure.
#ifdef SIMD_AVX2_INSTRUCTIONS_DETECTION_GUARD
#ifdef CAN_COMPILE_SIMD_AVX2_INSTRUCTIONS
    flags->avx2 = CPUID_CHECK_REG(ebx, EBX_L7_AVX2);
#endif
#endif

#ifdef SIMD_AVX512_INSTRUCTIONS_DETECTION_GUARD
#ifdef CAN_COMPILE_SIMD_AVX512_F_INSTRUCTIONS
    flags->avx512_f = CPUID_CHECK_REG(ebx, EBX_L7_AVX512_F);
#endif
#ifdef CAN_COMPILE_SIMD_AVX512_CD_INSTRUCTIONS
    flags->avx512_cd = CPUID_CHECK_REG(ebx, EBX_L7_AVX512_CD);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_ER_INSTRUCTIONS
    flags->avx512_er = CPUID_CHECK_REG(ebx, EBX_L7_AVX512_ER);
#endif
#ifdef CAN_COMPILE_SIMD_AVX512_PF_INSTRUCTIONS
    flags->avx512_pf = CPUID_CHECK_REG(ebx, EBX_L7_AVX512_PF);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_4FMAPS_INSTRUCTIONS
    flags->avx512_4fmaps = CPUID_CHECK_REG(edx, EDX_L7_AVX512_4FMAPS);
#endif
#ifdef CAN_COMPILE_SIMD_AVX512_4VNNIW_INSTRUCTIONS
    flags->avx512_4vnniw = CPUID_CHECK_REG(edx, EDX_L7_AVX512_4VNNIW);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_VPOPCNTDQ_INSTRUCTIONS
    flags->avx512_vpopcntdq = CPUID_CHECK_REG(ecx, ECX_L7_AVX512_VPOPCNTDQ);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_VL_INSTRUCTIONS
    flags->avx512_vl = CPUID_CHECK_REG(ebx, EBX_L7_AVX512_VL);
#endif
#ifdef CAN_COMPILE_SIMD_AVX512_DQ_INSTRUCTIONS
    flags->avx512_dq = CPUID_CHECK_REG(ebx, EBX_L7_AVX512_DQ);
#endif
#ifdef CAN_COMPILE_SIMD_AVX512_BW_INSTRUCTIONS
    flags->avx512_bw = CPUID_CHECK_REG(ebx, EBX_L7_AVX512_BW);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_IFMA_INSTRUCTIONS
    flags->avx512_ifma = CPUID_CHECK_REG(ebx, EBX_L7_AVX512_IFMA);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_VBMI_INSTRUCTIONS
    flags->avx512_vbmi = CPUID_CHECK_REG(ecx, ECX_L7_AVX512_VBMI);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_VNNI_INSTRUCTIONS
    flags->avx512_vnni = CPUID_CHECK_REG(ecx, ECX_L7_AVX512_VNNI);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_VBMI2_INSTRUCTIONS
    flags->avx512_vbmi2 = CPUID_CHECK_REG(ecx, ECX_L7_AVX512_VBMI2);
#endif
#ifdef CAN_COMPILE_SIMD_AVX512_BITALG_INSTRUCTIONS
    flags->avx512_bitalg = CPUID_CHECK_REG(ecx, ECX_L7_AVX512_BITALG);
#endif

#ifdef CAN_COMPILE_SIMD_AVX512_VP2INTERSECT_INSTRUCTIONS
    flags->avx512_vp2intersect = CPUID_CHECK_REG(edx, EDX_L7_AVX512_VP2INTERSECT);
#endif
#endif
}

/* Extended Feature Bits (LEAF=7, SUBLEAF=1). */
static inline void
detect_simd_extended_features_ecx_1(py_simd_features *flags,
                                    uint8_t eax, uint8_t ebx,
                                    uint8_t ecx, uint8_t edx)
{
    // Keep the ordering and newlines as they are declared in the structure.
#ifdef SIMD_AVX_INSTRUCTIONS_DETECTION_GUARD
#ifdef CAN_COMPILE_SIMD_AVX_NE_CONVERT_INSTRUCTIONS
    flags->avx_ne_convert = CPUID_CHECK_REG(edx, EDX_L7S1_AVX_NE_CONVERT);
#endif

#ifdef CAN_COMPILE_SIMD_AVX_IFMA_INSTRUCTIONS
    flags->avx_ifma = CPUID_CHECK_REG(eax, EAX_L7S1_AVX_IFMA);
#endif

#ifdef CAN_COMPILE_SIMD_AVX_VNNI_INSTRUCTIONS
    flags->avx_vnni = CPUID_CHECK_REG(eax, EAX_L7S1_AVX_VNNI);
#endif
#ifdef CAN_COMPILE_SIMD_AVX_VNNI_INT8_INSTRUCTIONS
    flags->avx_vnni_int8 = CPUID_CHECK_REG(edx, EDX_L7S1_AVX_VNNI_INT8);
#endif
#ifdef CAN_COMPILE_SIMD_AVX_VNNI_INT16_INSTRUCTIONS
    flags->avx_vnni_int16 = CPUID_CHECK_REG(edx, EDX_L7S1_AVX_VNNI_INT16);
#endif
#endif
}

static inline void
detect_simd_xsave_state(py_simd_features *flags)
{
    uint64_t xcr0 = flags->os_xsave ? get_xgetbv(0) : 0;
    flags->xcr0_sse = CPUID_CHECK_REG(xcr0, XCR0_SSE);

    flags->xcr0_avx = CPUID_CHECK_REG(xcr0, XCR0_AVX);

    flags->xcr0_avx512_opmask = CPUID_CHECK_REG(xcr0, XCR0_AVX512_OPMASK);
    flags->xcr0_avx512_zmm_hi256 = CPUID_CHECK_REG(xcr0, XCR0_AVX512_ZMM_HI256);
    flags->xcr0_avx512_hi16_zmm = CPUID_CHECK_REG(xcr0, XCR0_AVX512_HI16_ZMM);
}

static inline void
finalize_simd_features(py_simd_features *flags)
{
    assert(flags->done == 0);
    // Here, any flag that may depend on others should be correctly set
    // at runtime to avoid illegal instruction errors.
    flags->done = 1;
}

/*
 * Return 0 if flags are compatible and correctly set and -1 otherwise.
 *
 * If this function returns -1, 'flags' should disable all SIMD features
 * to avoid encountering a possible illegal instruction error at runtime.
 */
static inline int
validate_simd_features(const py_simd_features *flags)
{
    if (flags->done != 1) {
        return -1;
    }

    // AVX-512/F is required to support any other AVX-512 instruction set
    uint8_t avx512_require_f = (
        flags->avx512_cd || flags->avx512_er || flags->avx512_pf ||
        flags->avx512_vl || flags->avx512_dq || flags->avx512_bw ||
        flags->avx512_ifma ||
        flags->avx512_vbmi ||
        flags->avx512_4fmaps || flags->avx512_4vnniw ||
        flags->avx512_vpopcntdq ||
        flags->avx512_vnni || flags->avx512_vbmi2 || flags->avx512_bitalg ||
        flags->avx512_vp2intersect
    );
    if (!flags->avx512_f && !avx512_require_f) {
        return -1;
    }

    return 0;
}

void
_Py_disable_simd_features(py_simd_features *flags)
{
    // Keep the ordering and newlines as they are declared in the structure.
#define ZERO(FLAG)   flags->FLAG = 0
    ZERO(sse);
    ZERO(sse2);
    ZERO(sse3);
    ZERO(ssse3);
    ZERO(sse41);
    ZERO(sse42);

    ZERO(avx);
    ZERO(avx_ifma);
    ZERO(avx_ne_convert);

    ZERO(avx_vnni);
    ZERO(avx_vnni_int8);
    ZERO(avx_vnni_int16);

    ZERO(avx2);

    ZERO(avx512_f);
    ZERO(avx512_cd);

    ZERO(avx512_er);
    ZERO(avx512_pf);

    ZERO(avx512_4fmaps);
    ZERO(avx512_4vnniw);

    ZERO(avx512_vpopcntdq);

    ZERO(avx512_vl);
    ZERO(avx512_dq);
    ZERO(avx512_bw);

    ZERO(avx512_ifma);

    ZERO(avx512_vbmi);

    ZERO(avx512_vnni);

    ZERO(avx512_vbmi2);
    ZERO(avx512_bitalg);

    ZERO(avx512_vp2intersect);

    ZERO(os_xsave);

    ZERO(xcr0_sse);
    ZERO(xcr0_avx);
    ZERO(xcr0_avx512_opmask);
    ZERO(xcr0_avx512_zmm_hi256);
    ZERO(xcr0_avx512_hi16_zmm);
#undef ZERO
}

void
_Py_update_simd_features(py_simd_features *out,
                         const py_simd_features *src)
{
    // Keep the ordering and newlines as they are declared in the structure.
#define UPDATE(FLAG)   out->FLAG |= src->FLAG
    UPDATE(sse);
    UPDATE(sse2);
    UPDATE(sse3);
    UPDATE(ssse3);
    UPDATE(sse41);
    UPDATE(sse42);

    UPDATE(avx);
    UPDATE(avx_ifma);
    UPDATE(avx_ne_convert);

    UPDATE(avx_vnni);
    UPDATE(avx_vnni_int8);
    UPDATE(avx_vnni_int16);

    UPDATE(avx2);

    UPDATE(avx512_f);
    UPDATE(avx512_cd);

    UPDATE(avx512_er);
    UPDATE(avx512_pf);

    UPDATE(avx512_4fmaps);
    UPDATE(avx512_4vnniw);

    UPDATE(avx512_vpopcntdq);

    UPDATE(avx512_vl);
    UPDATE(avx512_dq);
    UPDATE(avx512_bw);

    UPDATE(avx512_ifma);

    UPDATE(avx512_vbmi);

    UPDATE(avx512_vnni);

    UPDATE(avx512_vbmi2);
    UPDATE(avx512_bitalg);

    UPDATE(avx512_vp2intersect);

    UPDATE(os_xsave);

    UPDATE(xcr0_sse);
    UPDATE(xcr0_avx);
    UPDATE(xcr0_avx512_opmask);
    UPDATE(xcr0_avx512_zmm_hi256);
    UPDATE(xcr0_avx512_hi16_zmm);
#undef UPDATE
    out->done = 1;
}

void
_Py_detect_simd_features(py_simd_features *flags)
{
    if (flags->done) {
        return;
    }
    _Py_disable_simd_features(flags);
    uint32_t maxleaf = detect_cpuid_maxleaf();
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
#ifdef SHOULD_DETECT_SIMD_FEATURES_L1
    if (maxleaf >= 1) {
        eax = 0, ebx = 0, ecx = 0, edx = 0;
        get_cpuid_info(1, 0, &eax, &ebx, &ecx, &edx);
        detect_simd_features(flags, eax, ebx, ecx, edx);
        if (flags->os_xsave) {
            detect_simd_xsave_state(flags);
        }
    }
#else
    (void) maxleaf;
    (void) eax; (void) ebx; (void) ecx; (void) edx;
#endif
#ifdef SHOULD_DETECT_SIMD_FEATURES_L7
    if (maxleaf >= 7) {
#ifdef SHOULD_DETECT_SIMD_FEATURES_L7S0
        eax = 0, ebx = 0, ecx = 0, edx = 0;
        get_cpuid_info(7, 0, &eax, &ebx, &ecx, &edx);
        detect_simd_extended_features_ecx_0(flags, eax, ebx, ecx, edx);
#endif
#ifdef SHOULD_DETECT_SIMD_FEATURES_L7S1
        eax = 0, ebx = 0, ecx = 0, edx = 0;
        get_cpuid_info(7, 1, &eax, &ebx, &ecx, &edx);
        detect_simd_extended_features_ecx_1(flags, eax, ebx, ecx, edx);
#endif
    }
#else
    (void) maxleaf;
    (void) eax; (void) ebx; (void) ecx; (void) edx;
#endif
    finalize_simd_features(flags);
    if (validate_simd_features(flags) < 0) {
        _Py_disable_simd_features(flags);
    }
}
