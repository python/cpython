#ifndef Py_INTERNAL_CPUINFO_H
#define Py_INTERNAL_CPUINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    /* Streaming SIMD Extensions */
    uint8_t sse: 1;
    uint8_t sse2: 1;
    uint8_t sse3: 1;
    uint8_t sse41: 1;       // SSE4.1
    uint8_t sse42: 1;       // SSE4.2

    /* Advanced Vector Extensions */
    uint8_t avx: 1;
    uint8_t avx2: 1;
    uint8_t avx512vbmi: 1;  // AVX-512 Vector Byte Manipulation Instructions

    uint8_t done;           // indicate whether the structure was filled or not
} py_cpu_simd_flags;

/* Detect the available SIMD features on this machine. */
extern void
_Py_detect_cpu_simd_features(py_cpu_simd_flags *flags);

/*
 * Apply a bitwise-OR on all flags in 'out' using those in 'src',
 * unconditionally updating 'out' (i.e. out->done is ignored) and
 * setting 'out->done' to 1.
 */
extern void
_Py_extend_cpu_simd_features(py_cpu_simd_flags *out, const py_cpu_simd_flags *src);

#ifdef __cplusplus
}
#endif

#endif /* !Py_INTERNAL_CPUINFO_H */
