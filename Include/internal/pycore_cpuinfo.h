#ifndef Py_INTERNAL_CPUINFO_H
#define Py_INTERNAL_CPUINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdint.h>         // uint8_t

/* Macro indicating that the member is a CPUID bit. */
#define _Py_SIMD_FEAT       uint8_t
/* Macro indicating that the member is a XCR0 bit. */
#define _Py_SIMD_XCR0_BIT   uint8_t

typedef struct py_simd_features {
    /* Streaming SIMD Extensions */
    _Py_SIMD_FEAT sse: 1;
    _Py_SIMD_FEAT sse2: 1;
    _Py_SIMD_FEAT sse3: 1;
    _Py_SIMD_FEAT ssse3: 1; // Supplemental SSE3 instructions
    _Py_SIMD_FEAT sse41: 1; // SSE4.1
    _Py_SIMD_FEAT sse42: 1; // SSE4.2

    /* Advanced Vector Extensions */
    _Py_SIMD_FEAT avx: 1;
    _Py_SIMD_FEAT avx_ifma: 1;
    _Py_SIMD_FEAT avx_ne_convert: 1;

    _Py_SIMD_FEAT avx_vnni: 1;
    _Py_SIMD_FEAT avx_vnni_int8: 1;
    _Py_SIMD_FEAT avx_vnni_int16: 1;

    /* Advanced Vector Extensions 2. */
    _Py_SIMD_FEAT avx2: 1;

    /*
     * AVX-512 instruction set are grouped by the processor generation
     * that implements them (see https://en.wikipedia.org/wiki/AVX-512).
     *
     * We do not include GFNI, VPCLMULQDQ and VAES instructions since
     * they are not exactly AVX-512 per se, nor do we include BF16 or
     * FP16 since they operate on bfloat16 and binary16 (half-float).
     */
    _Py_SIMD_FEAT avx512_f: 1;
    _Py_SIMD_FEAT avx512_cd: 1;

    _Py_SIMD_FEAT avx512_er: 1;
    _Py_SIMD_FEAT avx512_pf: 1;

    _Py_SIMD_FEAT avx512_4fmaps: 1;
    _Py_SIMD_FEAT avx512_4vnniw: 1;

    _Py_SIMD_FEAT avx512_vpopcntdq: 1;

    _Py_SIMD_FEAT avx512_vl: 1;
    _Py_SIMD_FEAT avx512_dq: 1;
    _Py_SIMD_FEAT avx512_bw: 1;

    _Py_SIMD_FEAT avx512_ifma: 1;

    _Py_SIMD_FEAT avx512_vbmi: 1;

    _Py_SIMD_FEAT avx512_vnni: 1;

    _Py_SIMD_FEAT avx512_vbmi2: 1;
    _Py_SIMD_FEAT avx512_bitalg: 1;

    _Py_SIMD_FEAT avx512_vp2intersect: 1;

    _Py_SIMD_FEAT os_xsave: 1;  // XSAVE is supported

    /* XCR0 register bits */
    _Py_SIMD_XCR0_BIT xcr0_sse: 1;

    // On some Intel CPUs, it is possible for the CPU to support AVX2
    // instructions even though the underlying OS does not know about
    // AVX. In particular, only (SSE) XMM registers will be saved and
    // restored on context-switch, but not (AVX) YMM registers.
    _Py_SIMD_XCR0_BIT xcr0_avx: 1;
    _Py_SIMD_XCR0_BIT xcr0_avx512_opmask: 1;
    _Py_SIMD_XCR0_BIT xcr0_avx512_zmm_hi256: 1;
    _Py_SIMD_XCR0_BIT xcr0_avx512_hi16_zmm: 1;

    // We want the structure to be aligned correctly, namely
    // its size in bits must be a multiple of 8.
    // 
    // Whenever a field is added or removed above, update the
    // number of fields (35) and adjust the bitsize of 'done'.
    uint8_t done: 5;    // set if the structure was filled
} py_simd_features;

/*
 * Explicitly initialize all members to zero to guarantee that
 * we never have an un-initialized attribute at runtime which
 * could lead to an illegal instruction error.
 */
extern void
_Py_disable_simd_features(py_simd_features *flags);

/*
 * Apply a bitwise-OR on all flags in 'out' using those in 'src',
 * unconditionally updating 'out' (i.e. 'out->done' is ignored).
 *
 * This also sets 'out->done' to 1 at the end. 
 *
 * Note that the caller is responsible to ensure that the flags set to 1
 * must not lead to illegal instruction errors if the corresponding SIMD
 * instruction(s) are used.
 */
extern void
_Py_update_simd_features(py_simd_features *out, const py_simd_features *src);

/* Detect the available SIMD features on this machine. */
extern void
_Py_detect_simd_features(py_simd_features *flags);

#ifdef __cplusplus
}
#endif

#endif /* !Py_INTERNAL_CPUINFO_H */
