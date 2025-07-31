/**
 * @author Bénédikt Tran
 * @seealso @file Tools/cpuinfo/libcpuinfo/features/cpuid.py
 *
 * The enumeration describes masks to apply on CPUID output registers.
 *
 * Member names are _Py_CPUID_MASK_<REGISTER>_L<LEAF>[S<SUBLEAF>]_<FEATURE>,
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

#ifndef Py_INTERNAL_CPUINFO_CPUID_FEATURES_H
#define Py_INTERNAL_CPUINFO_CPUID_FEATURES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "Python.h"

/*[python input]
import os, sys
sys.path.insert(0, os.path.realpath(os.path.join(os.getcwd(), "Tools/cpuinfo")))
from libcpuinfo.features.cpuid import make_cpuid_features_constants
print(make_cpuid_features_constants())
[python start generated code]*/
// clang-format off
/** Constants for CPUID features */
/* CPUID (LEAF=1, SUBLEAF=0) [ECX] */
#define _Py_CPUID_MASK_ECX_L1_SSE3                  0x00000001  // bit = 0
#define _Py_CPUID_MASK_ECX_L1_PCLMULQDQ             0x00000002  // bit = 1
#define _Py_CPUID_MASK_ECX_L1_SSSE3                 0x00000200  // bit = 9
#define _Py_CPUID_MASK_ECX_L1_FMA                   0x00001000  // bit = 12
#define _Py_CPUID_MASK_ECX_L1_SSE4_1                0x00080000  // bit = 19
#define _Py_CPUID_MASK_ECX_L1_SSE4_2                0x00100000  // bit = 20
#define _Py_CPUID_MASK_ECX_L1_POPCNT                0x00800000  // bit = 23
#define _Py_CPUID_MASK_ECX_L1_XSAVE                 0x04000000  // bit = 26
#define _Py_CPUID_MASK_ECX_L1_OSXSAVE               0x08000000  // bit = 27
#define _Py_CPUID_MASK_ECX_L1_AVX                   0x10000000  // bit = 28

/* CPUID (LEAF=1, SUBLEAF=0) [EDX] */
#define _Py_CPUID_MASK_EDX_L1_CMOV                  0x00008000  // bit = 15
#define _Py_CPUID_MASK_EDX_L1_SSE                   0x02000000  // bit = 25
#define _Py_CPUID_MASK_EDX_L1_SSE2                  0x04000000  // bit = 26

/* CPUID (LEAF=7, SUBLEAF=0) [EBX] */
#define _Py_CPUID_MASK_EBX_L7_AVX2                  0x00000020  // bit = 5
#define _Py_CPUID_MASK_EBX_L7_AVX512_F              0x00010000  // bit = 16
#define _Py_CPUID_MASK_EBX_L7_AVX512_DQ             0x00020000  // bit = 17
#define _Py_CPUID_MASK_EBX_L7_AVX512_IFMA           0x00200000  // bit = 21
#define _Py_CPUID_MASK_EBX_L7_AVX512_PF             0x04000000  // bit = 26
#define _Py_CPUID_MASK_EBX_L7_AVX512_ER             0x08000000  // bit = 27
#define _Py_CPUID_MASK_EBX_L7_AVX512_CD             0x10000000  // bit = 28
#define _Py_CPUID_MASK_EBX_L7_AVX512_BW             0x40000000  // bit = 30
#define _Py_CPUID_MASK_EBX_L7_AVX512_VL             0x80000000  // bit = 31

/* CPUID (LEAF=7, SUBLEAF=0) [ECX] */
#define _Py_CPUID_MASK_ECX_L7_AVX512_VBMI           0x00000002  // bit = 1
#define _Py_CPUID_MASK_ECX_L7_AVX512_VBMI2          0x00000040  // bit = 6
#define _Py_CPUID_MASK_ECX_L7_AVX512_VNNI           0x00000800  // bit = 11
#define _Py_CPUID_MASK_ECX_L7_AVX512_BITALG         0x00001000  // bit = 12
#define _Py_CPUID_MASK_ECX_L7_AVX512_VPOPCNTDQ      0x00004000  // bit = 14

/* CPUID (LEAF=7, SUBLEAF=0) [EDX] */
#define _Py_CPUID_MASK_EDX_L7_AVX512_4VNNIW         0x00000004  // bit = 2
#define _Py_CPUID_MASK_EDX_L7_AVX512_4FMAPS         0x00000008  // bit = 3
#define _Py_CPUID_MASK_EDX_L7_AVX512_VP2INTERSECT   0x00000100  // bit = 8

/* CPUID (LEAF=7, SUBLEAF=1) [EAX] */
#define _Py_CPUID_MASK_EAX_L7S1_AVX_VNNI            0x00000010  // bit = 4
#define _Py_CPUID_MASK_EAX_L7S1_AVX_IFMA            0x00800000  // bit = 23

/* CPUID (LEAF=7, SUBLEAF=1) [EDX] */
#define _Py_CPUID_MASK_EDX_L7S1_AVX_VNNI_INT8       0x00000010  // bit = 4
#define _Py_CPUID_MASK_EDX_L7S1_AVX_NE_CONVERT      0x00000020  // bit = 5
#define _Py_CPUID_MASK_EDX_L7S1_AVX_VNNI_INT16      0x00000400  // bit = 10
// clang-format on
/*[python end generated code: output=e9112f064e2effec input=71ec6b4356052ec3]*/

#ifdef __cplusplus
}
#endif

#endif // !Py_INTERNAL_CPUINFO_CPUID_FEATURES_H
