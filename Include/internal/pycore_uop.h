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
#define UOP_MAX_TRACE_LENGTH 3000
#define UOP_BUFFER_SIZE (UOP_MAX_TRACE_LENGTH * sizeof(_PyUOpInstruction))

/* Bloom filter with m = 256
 * https://en.wikipedia.org/wiki/Bloom_filter */
#define _Py_BLOOM_FILTER_WORDS 8

typedef struct {
    uint32_t bits[_Py_BLOOM_FILTER_WORDS];
} _PyBloomFilter;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UOP_H */
