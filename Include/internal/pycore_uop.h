#ifndef Py_CORE_UOP_H
#define Py_CORE_UOP_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdint.h>
/* Depending on the format,
 * the 32 bits between the oparg and operand are:
 * UOP_FORMAT_TARGET:
 *    uint32_t target;
 * UOP_FORMAT_JUMP
 *    uint16_t jump_target;
 *    uint16_t error_target;
 */
typedef struct _PyUOpInstruction{
    uint16_t opcode:15;
    uint16_t format:1;
    uint16_t oparg;
    union {
        uint32_t target;
        struct {
            uint16_t jump_target;
            uint16_t error_target;
        };
    };
    uint64_t operand0;  // A cache entry
    uint64_t operand1;
#ifdef Py_STATS
    uint64_t execution_count;
#endif
} _PyUOpInstruction;

// This is the length of the trace we translate initially.
#if defined(Py_DEBUG) && defined(_Py_JIT)
    // With asserts, the stencils are a lot larger
#define UOP_MAX_TRACE_LENGTH 1000
#else
#define UOP_MAX_TRACE_LENGTH 2500
#endif

/* Bloom filter with m = 256
 * https://en.wikipedia.org/wiki/Bloom_filter */
#ifdef HAVE_GCC_UINT128_T
#define _Py_BLOOM_FILTER_WORDS 2
typedef __uint128_t _Py_bloom_filter_word_t;
#else
#define _Py_BLOOM_FILTER_WORDS 4
typedef uint64_t _Py_bloom_filter_word_t;
#endif

#define _Py_BLOOM_FILTER_BITS_PER_WORD \
    ((int)(sizeof(_Py_bloom_filter_word_t) * 8))
#define _Py_BLOOM_FILTER_WORD_SHIFT \
    ((sizeof(_Py_bloom_filter_word_t) == 16) ? 7 : 6)

typedef struct {
    _Py_bloom_filter_word_t bits[_Py_BLOOM_FILTER_WORDS];
} _PyBloomFilter;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UOP_H */
