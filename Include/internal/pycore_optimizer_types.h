#ifndef Py_INTERNAL_OPTIMIZER_TYPES_H
#define Py_INTERNAL_OPTIMIZER_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdbool.h>
#include "pycore_uop.h"  // UOP_MAX_TRACE_LENGTH

// Holds locals, stack, locals, stack ... (in that order)
#define MAX_ABSTRACT_INTERP_SIZE 512

#define TY_ARENA_SIZE (UOP_MAX_TRACE_LENGTH * 5)

// Need extras for root frame and for overflow frame (see TRACE_STACK_PUSH())
#define MAX_ABSTRACT_FRAME_DEPTH (16)

// The maximum number of side exits that we can take before requiring forward
// progress (and inserting a new ENTER_EXECUTOR instruction). In practice, this
// is the "maximum amount of polymorphism" that an isolated trace tree can
// handle before rejoining the rest of the program.
#define MAX_CHAIN_DEPTH 4

/* Symbols */
/* See explanation in optimizer_symbols.c */


typedef enum _JitSymType {
    JIT_SYM_UNKNOWN_TAG = 1,
    JIT_SYM_NULL_TAG = 2,
    JIT_SYM_NON_NULL_TAG = 3,
    JIT_SYM_BOTTOM_TAG = 4,
    JIT_SYM_TYPE_VERSION_TAG = 5,
    JIT_SYM_KNOWN_CLASS_TAG = 6,
    JIT_SYM_KNOWN_VALUE_TAG = 7,
    JIT_SYM_TUPLE_TAG = 8,
    JIT_SYM_TRUTHINESS_TAG = 9,
    JIT_SYM_COMPACT_INT = 10,
    JIT_SYM_PREDICATE_TAG = 11,
    JIT_SYM_RECORDED_VALUE_TAG = 12,
    JIT_SYM_RECORDED_TYPE_TAG = 13,
    JIT_SYM_RECORDED_GEN_FUNC_TAG = 14,
} JitSymType;

typedef struct _jit_opt_known_class {
    uint8_t tag;
    uint32_t version;
    PyTypeObject *type;
} JitOptKnownClass;

typedef struct _jit_opt_known_version {
    uint8_t tag;
    uint32_t version;
} JitOptKnownVersion;

typedef struct _jit_opt_known_value {
    uint8_t tag;
    PyObject *value;
} JitOptKnownValue;

#define MAX_SYMBOLIC_TUPLE_SIZE 7

typedef struct _jit_opt_tuple {
    uint8_t tag;
    uint8_t length;
    uint16_t items[MAX_SYMBOLIC_TUPLE_SIZE];
} JitOptTuple;

typedef struct {
    uint8_t tag;
    bool invert;
    uint16_t value;
} JitOptTruthiness;

typedef enum {
    JIT_PRED_IS,
    JIT_PRED_IS_NOT,
    JIT_PRED_EQ,
    JIT_PRED_NE,
} JitOptPredicateKind;

typedef struct {
    uint8_t tag;
    uint8_t kind;
    uint16_t lhs;
    uint16_t rhs;
} JitOptPredicate;

typedef struct _jit_opt_recorded_value {
    uint8_t tag;
    bool known_type;
    PyObject *value;
} JitOptRecordedValue;

typedef struct _jit_opt_recorded_type {
    uint8_t tag;
    PyTypeObject *type;
} JitOptRecordedType;

/* Represents a generator, but we record the
 * function as the generator is emphemeral */
typedef struct _jit_opt_recorded_gen_func {
    uint8_t tag;
    PyFunctionObject *func;
} JitOptRecordedGenFunc;

typedef struct {
    uint8_t tag;
} JitOptCompactInt;

typedef union _jit_opt_symbol {
    uint8_t tag;
    JitOptKnownClass cls;
    JitOptKnownValue value;
    JitOptKnownVersion version;
    JitOptTuple tuple;
    JitOptTruthiness truthiness;
    JitOptCompactInt compact;
    JitOptPredicate predicate;
    JitOptRecordedValue recorded_value;
    JitOptRecordedType recorded_type;
    JitOptRecordedGenFunc recorded_gen_func;
} JitOptSymbol;

// This mimics the _PyStackRef API
typedef union {
    uintptr_t bits;
} JitOptRef;

typedef struct _Py_UOpsAbstractFrame {
    bool globals_watched;
    // The version number of the globals dicts, once checked. 0 if unchecked.
    uint32_t globals_checked_version;
    // Max stacklen
    int stack_len;
    int locals_len;
    PyFunctionObject *func;
    PyCodeObject *code;

    JitOptRef *stack_pointer;
    JitOptRef *stack;
    JitOptRef *locals;
} _Py_UOpsAbstractFrame;

typedef struct ty_arena {
    int ty_curr_number;
    int ty_max_number;
    JitOptSymbol arena[TY_ARENA_SIZE];
} ty_arena;


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPTIMIZER_TYPES_H */
