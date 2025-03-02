/*
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

/*
 * The enumeration describes masks to apply on CPUID output registers.
 *
 * Member names are Py_CPUID_MASK_<REGISTER>_L<LEAF>[S<SUBLEAF>]_<FEATURE>,
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
 *
 * The LEAF value should only 1 or 7 as other values may have different
 * meanings depending on the underlying architecture.
 */
// fmt: off
typedef enum py_cpuid_feature_mask {
/*[python input]
# {(LEAF, SUBLEAF, REGISTRY): {FEATURE: BIT}}
data = {
    (1, 0, 'ECX'): {
        'SSE3': 0,
        'PCLMULQDQ': 1,
        'SSSE3': 9,
        'FMA': 12,
        'SSE4_1': 19,
        'SSE4_2': 20,
        'POPCNT': 23,
        'XSAVE': 26,
        'OSXSAVE': 27,
        'AVX': 28,
    },
    (1, 0, 'EDX'): {
        'CMOV': 15,
        'SSE': 25,
        'SSE2': 26,
    },
    (7, 0, 'EBX'): {
        'AVX2': 5,
        'AVX512_F': 16,
        'AVX512_DQ': 17,
        'AVX512_IFMA': 21,
        'AVX512_PF': 26,
        'AVX512_ER': 27,
        'AVX512_CD': 28,
        'AVX512_BW': 30,
        'AVX512_VL': 31,
    },
    (7, 0, 'ECX'): {
        'AVX512_VBMI': 1,
        'AVX512_VBMI2': 6,
        'AVX512_VNNI': 11,
        'AVX512_BITALG': 12,
        'AVX512_VPOPCNTDQ': 14,
    },
    (7, 0, 'EDX'): {
        'AVX512_4VNNIW': 2,
        'AVX512_4FMAPS': 3,
        'AVX512_VP2INTERSECT': 8,
    },
    (7, 1, 'EAX'): {
        'AVX_VNNI': 4,
        'AVX_IFMA': 23,
    },
    (7, 1, 'EDX'): {
        'AVX_VNNI_INT8': 4,
        'AVX_NE_CONVERT': 5,
        'AVX_VNNI_INT16': 10,
    },
}

def get_member_name(leaf, subleaf, registry, name):
    node = f'L{leaf}S{subleaf}' if subleaf else f'L{leaf}'
    return f'Py_CPUID_MASK_{registry}_{node}_{name}'

def get_member_mask(bit):
    val = format(1 << bit, '008x')
    return f'= 0x{val},'

# The enumeration is rendered as follows:
#
#   <INDENT><MEMBER_NAME> <TAB>= 0x<MASK>, <TAB>// bit = BIT
#   ^       ^             ^    ^   ^       ^    ^
#
# where ^ indicates a column that is a multiple of 4, <MASK> has
# exactly 8 characters and <BIT> has at most 2 characters.

INDENT = ' ' * 4
# BUG(picnixz): Clinic does not like when '/' and '*' are put together.
COMMENT = '/' + '* '

def next_block(w):
    """Compute the smallest multiple of 4 strictly larger than *w*."""
    return ((w + 3) & ~0x03) if (w % 4) else (w + 4)

NAMESIZE = next_block(max(
    len(get_member_name(*group, name))
    for group, values in data.items()
    for name in values
))
MASKSIZE = 8 + next_block(len('= 0x,'))

for group, values in data.items():
    title = 'CPUID (LEAF={}, SUBLEAF={}) [{}]'.format(*group)
    print(INDENT, *COMMENT, title, *COMMENT[::-1], sep='')
    for name, bit in values.items():
        assert name, f"invalid entry in {group}"
        key = get_member_name(*group, name)
        assert 0 <= bit < 32, f"invalid bit value for {name!r}"
        val = get_member_mask(bit)

        member_name = key.ljust(NAMESIZE)
        member_mask = val.ljust(MASKSIZE)

        print(INDENT, member_name, member_mask, f'// bit = {bit}', sep='')
[python start generated code]*/
    /* CPUID (LEAF=1, SUBLEAF=0) [ECX] */
    Py_CPUID_MASK_ECX_L1_SSE3                   = 0x00000001,   // bit = 0
    Py_CPUID_MASK_ECX_L1_PCLMULQDQ              = 0x00000002,   // bit = 1
    Py_CPUID_MASK_ECX_L1_SSSE3                  = 0x00000200,   // bit = 9
    Py_CPUID_MASK_ECX_L1_FMA                    = 0x00001000,   // bit = 12
    Py_CPUID_MASK_ECX_L1_SSE4_1                 = 0x00080000,   // bit = 19
    Py_CPUID_MASK_ECX_L1_SSE4_2                 = 0x00100000,   // bit = 20
    Py_CPUID_MASK_ECX_L1_POPCNT                 = 0x00800000,   // bit = 23
    Py_CPUID_MASK_ECX_L1_XSAVE                  = 0x04000000,   // bit = 26
    Py_CPUID_MASK_ECX_L1_OSXSAVE                = 0x08000000,   // bit = 27
    Py_CPUID_MASK_ECX_L1_AVX                    = 0x10000000,   // bit = 28
    /* CPUID (LEAF=1, SUBLEAF=0) [EDX] */
    Py_CPUID_MASK_EDX_L1_CMOV                   = 0x00008000,   // bit = 15
    Py_CPUID_MASK_EDX_L1_SSE                    = 0x02000000,   // bit = 25
    Py_CPUID_MASK_EDX_L1_SSE2                   = 0x04000000,   // bit = 26
    /* CPUID (LEAF=7, SUBLEAF=0) [EBX] */
    Py_CPUID_MASK_EBX_L7_AVX2                   = 0x00000020,   // bit = 5
    Py_CPUID_MASK_EBX_L7_AVX512_F               = 0x00010000,   // bit = 16
    Py_CPUID_MASK_EBX_L7_AVX512_DQ              = 0x00020000,   // bit = 17
    Py_CPUID_MASK_EBX_L7_AVX512_IFMA            = 0x00200000,   // bit = 21
    Py_CPUID_MASK_EBX_L7_AVX512_PF              = 0x04000000,   // bit = 26
    Py_CPUID_MASK_EBX_L7_AVX512_ER              = 0x08000000,   // bit = 27
    Py_CPUID_MASK_EBX_L7_AVX512_CD              = 0x10000000,   // bit = 28
    Py_CPUID_MASK_EBX_L7_AVX512_BW              = 0x40000000,   // bit = 30
    Py_CPUID_MASK_EBX_L7_AVX512_VL              = 0x80000000,   // bit = 31
    /* CPUID (LEAF=7, SUBLEAF=0) [ECX] */
    Py_CPUID_MASK_ECX_L7_AVX512_VBMI            = 0x00000002,   // bit = 1
    Py_CPUID_MASK_ECX_L7_AVX512_VBMI2           = 0x00000040,   // bit = 6
    Py_CPUID_MASK_ECX_L7_AVX512_VNNI            = 0x00000800,   // bit = 11
    Py_CPUID_MASK_ECX_L7_AVX512_BITALG          = 0x00001000,   // bit = 12
    Py_CPUID_MASK_ECX_L7_AVX512_VPOPCNTDQ       = 0x00004000,   // bit = 14
    /* CPUID (LEAF=7, SUBLEAF=0) [EDX] */
    Py_CPUID_MASK_EDX_L7_AVX512_4VNNIW          = 0x00000004,   // bit = 2
    Py_CPUID_MASK_EDX_L7_AVX512_4FMAPS          = 0x00000008,   // bit = 3
    Py_CPUID_MASK_EDX_L7_AVX512_VP2INTERSECT    = 0x00000100,   // bit = 8
    /* CPUID (LEAF=7, SUBLEAF=1) [EAX] */
    Py_CPUID_MASK_EAX_L7S1_AVX_VNNI             = 0x00000010,   // bit = 4
    Py_CPUID_MASK_EAX_L7S1_AVX_IFMA             = 0x00800000,   // bit = 23
    /* CPUID (LEAF=7, SUBLEAF=1) [EDX] */
    Py_CPUID_MASK_EDX_L7S1_AVX_VNNI_INT8        = 0x00000010,   // bit = 4
    Py_CPUID_MASK_EDX_L7S1_AVX_NE_CONVERT       = 0x00000020,   // bit = 5
    Py_CPUID_MASK_EDX_L7S1_AVX_VNNI_INT16       = 0x00000400,   // bit = 10
/*[python end generated code: output=e53c5376296af250 input=4102387db46d5787]*/
} py_cpuid_feature_mask;
// fmt: on

/* XSAVE state components (XCR0 control register) */
typedef enum py_xsave_feature_mask {
    Py_XSAVE_MASK_XCR0_SSE              = 0x00000002,   // bit = 1
    Py_XSAVE_MASK_XCR0_AVX              = 0x00000004,   // bit = 2
    Py_XSAVE_MASK_XCR0_AVX512_OPMASK    = 0x00000020,   // bit = 5
    Py_XSAVE_MASK_XCR0_AVX512_ZMM_HI256 = 0x00000040,   // bit = 6
    Py_XSAVE_MASK_XCR0_AVX512_HI16_ZMM  = 0x00000080,   // bit = 7
} py_xsave_feature_mask;

typedef struct py_cpuid_features {
    uint32_t maxleaf;
    /* Macro to declare a member flag of 'py_cpuid_features' as a uint8_t. */
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

    _Py_CPUID_DECL_FLAG(xsave);     // XSAVE/XRSTOR/XSETBV/XGETBV
    _Py_CPUID_DECL_FLAG(osxsave);   // XSAVE is enabled by the OS

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
} py_cpuid_features;

/*
 * Explicitly initialize all members to zero to guarantee that
 * we never have an un-initialized attribute at runtime which
 * could lead to an illegal instruction error.
 *
 * This does not mark 'flags' as being ready yet.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(void)
_Py_cpuid_disable_features(py_cpuid_features *flags);

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
_Py_cpuid_check_features(const py_cpuid_features *flags);

/*
 * Return 1 if all expected flags are set in 'actual', 0 otherwise.
 *
 * If 'actual' or 'expect' are not ready yet, this also returns 0.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(int)
_Py_cpuid_has_features(const py_cpuid_features *actual,
                       const py_cpuid_features *expect);

/*
 * Return 1 if 'actual' and 'expect' are identical, 0 otherwise.
 *
 * If 'actual' or 'expect' are not ready yet, this also returns 0.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(int)
_Py_cpuid_match_features(const py_cpuid_features *actual,
                         const py_cpuid_features *expect);

/*
 * Detect the available features on this machine, storing the result in 'flags'.
 *
 * Note: This function does not set any exception and thus never fails.
 */
PyAPI_FUNC(void)
_Py_cpuid_detect_features(py_cpuid_features *flags);

#ifdef __cplusplus
}
#endif

#endif /* !Py_INTERNAL_CPUINFO_H */
