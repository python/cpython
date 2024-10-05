#ifndef Py_INTERNAL_CPUINFO_H
#define Py_INTERNAL_CPUINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdbool.h>

typedef struct {
    bool sse, sse2, sse3, sse41, sse42, avx, avx2, avx512vbmi;
    bool done;
} cpu_simd_flags;

extern void
detect_cpu_simd_features(cpu_simd_flags *flags);

#ifdef __cplusplus
}
#endif

#endif /* !Py_INTERNAL_CPUINFO_H */
