/*
 * @author Bénédikt Tran
 *
 * Interface for detecting the different CPUID flags in an opaque manner.
 * See https://en.wikipedia.org/wiki/CPUID for details on the bit values.
 *
 * If a module requires to support SIMD instructions, it should determine
 * the compiler flags and the instruction sets required for the intrinsics
 * to work.
 *
 * For the headers and expected CPUID bits needed by Intel intrinsics, see
 * https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html.
 */

#ifndef Py_INTERNAL_CPUINFO_H
#define Py_INTERNAL_CPUINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "Python.h"
#include "pycore_cpuinfo_cpuid_features.h"
#include "pycore_cpuinfo_xsave_features.h"

typedef struct _Py_cpuid_features_s {
    uint32_t maxleaf;
    /*
     * Macro to declare a member flag of '_Py_cpuid_features' as a uint8_t.
     * Whenever this macro is used, do not forget to update the number of
     * fields and the bitsize of the 'ready' member (see structure end).
     */
#define _Py_CPUID_DECL_FLAG(MEMBER_NAME)    uint8_t MEMBER_NAME:1
    // --- Streaming SIMD Extensions ------------------------------------------
    _Py_CPUID_DECL_FLAG(sse);
    _Py_CPUID_DECL_FLAG(sse2);
    _Py_CPUID_DECL_FLAG(sse3);
    _Py_CPUID_DECL_FLAG(ssse3); // Supplemental SSE3 instructions
    _Py_CPUID_DECL_FLAG(sse41); // SSE4.1
    _Py_CPUID_DECL_FLAG(sse42); // SSE4.2

    // --- Advanced Vector Extensions -----------------------------------------
    _Py_CPUID_DECL_FLAG(avx);
    _Py_CPUID_DECL_FLAG(avx_ifma);
    _Py_CPUID_DECL_FLAG(avx_ne_convert);

    _Py_CPUID_DECL_FLAG(avx_vnni);
    _Py_CPUID_DECL_FLAG(avx_vnni_int8);
    _Py_CPUID_DECL_FLAG(avx_vnni_int16);

    // --- Advanced Vector Extensions 2 ---------------------------------------
    _Py_CPUID_DECL_FLAG(avx2);

    // --- Advanced Vector Extensions (512-bit) -------------------------------
    /*
     * AVX-512 instruction set are grouped by the processor generation
     * that implements them (see https://en.wikipedia.org/wiki/AVX-512).
     *
     * We do not include GFNI, VPCLMULQDQ and VAES instructions since
     * they are not exactly AVX-512 per se, nor do we include BF16 or
     * FP16 since they operate on bfloat16 and binary16 (half-float).
     *
     * See https://en.wikipedia.org/wiki/AVX-512#Instruction_set for
     * the suffix meanings (for instance 'f' stands for 'Foundation').
     */
    _Py_CPUID_DECL_FLAG(avx512_f);
    _Py_CPUID_DECL_FLAG(avx512_cd);

    _Py_CPUID_DECL_FLAG(avx512_er);
    _Py_CPUID_DECL_FLAG(avx512_pf);

    _Py_CPUID_DECL_FLAG(avx512_4fmaps);
    _Py_CPUID_DECL_FLAG(avx512_4vnniw);

    _Py_CPUID_DECL_FLAG(avx512_vpopcntdq);

    _Py_CPUID_DECL_FLAG(avx512_vl);
    _Py_CPUID_DECL_FLAG(avx512_dq);
    _Py_CPUID_DECL_FLAG(avx512_bw);

    _Py_CPUID_DECL_FLAG(avx512_ifma);
    _Py_CPUID_DECL_FLAG(avx512_vbmi);

    _Py_CPUID_DECL_FLAG(avx512_vnni);

    _Py_CPUID_DECL_FLAG(avx512_vbmi2);
    _Py_CPUID_DECL_FLAG(avx512_bitalg);

    _Py_CPUID_DECL_FLAG(avx512_vp2intersect);

    // --- Instructions -------------------------------------------------------
    _Py_CPUID_DECL_FLAG(cmov);
    _Py_CPUID_DECL_FLAG(fma);
    _Py_CPUID_DECL_FLAG(popcnt);
    _Py_CPUID_DECL_FLAG(pclmulqdq);

    _Py_CPUID_DECL_FLAG(xsave);   // XSAVE/XRSTOR/XSETBV/XGETBV
    _Py_CPUID_DECL_FLAG(osxsave); // XSAVE is enabled by the OS

    // --- XCR0 register bits -------------------------------------------------
    _Py_CPUID_DECL_FLAG(xcr0_sse);
    // On some Intel CPUs, it is possible for the CPU to support AVX2
    // instructions even though the underlying OS does not know about
    // AVX. In particular, only (SSE) XMM registers will be saved and
    // restored on context-switch, but not (AVX) YMM registers.
    _Py_CPUID_DECL_FLAG(xcr0_avx);
    _Py_CPUID_DECL_FLAG(xcr0_avx512_opmask);
    _Py_CPUID_DECL_FLAG(xcr0_avx512_zmm_hi256);
    _Py_CPUID_DECL_FLAG(xcr0_avx512_hi16_zmm);
#undef _Py_CPUID_DECL_FLAG
    // Whenever a field is added or removed above, update the
    // number of fields (40) and adjust the bitsize of 'ready'
    // so that the size of this structure is a multiple of 8.
    uint8_t ready; // set if the structure is ready for usage
} _Py_cpuid_features;

/*
 * Explicitly set all members to zero to guarantee that
 * we never have a non-initialized attribute at runtime
 * which could lead to an illegal instruction error.
 *
 * This readiness state of 'flags' is ignored and left untouched.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(void)
_Py_cpuid_disable_features(_Py_cpuid_features *flags);

/*
 * Check whether the structure is ready and flags are inter-compatible,
 * returning 1 on success and 0 otherwise.
 *
 * The caller should disable all CPUID detected features if the check
 * fails to avoid encountering runtime illegal instruction errors.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(int)
_Py_cpuid_check_features(const _Py_cpuid_features *flags);

/*
 * Return 1 if all expected flags are set in 'actual', 0 otherwise.
 *
 * If 'actual' or 'expect' are not ready yet, this also returns 0.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(int)
_Py_cpuid_has_features(const _Py_cpuid_features *actual,
                       const _Py_cpuid_features *expect);

/*
 * Return 1 if 'actual' and 'expect' are identical, 0 otherwise.
 *
 * If 'actual' or 'expect' are not ready yet, this also returns 0.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(int)
_Py_cpuid_match_features(const _Py_cpuid_features *actual,
                         const _Py_cpuid_features *expect);

/*
 * Detect the available host features, storing the result in 'flags'.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(void)
_Py_cpuid_detect_features(_Py_cpuid_features *flags);

#ifdef __cplusplus
}
#endif

#endif /* !Py_INTERNAL_CPUINFO_H */
