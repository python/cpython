#ifndef Py_INTERNAL_OPTIMIZER_H
#define Py_INTERNAL_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_typedefs.h"      // _PyInterpreterFrame
#include "pycore_uop_ids.h"
#include <stdbool.h>


typedef struct _PyExecutorLinkListNode {
    struct _PyExecutorObject *next;
    struct _PyExecutorObject *previous;
} _PyExecutorLinkListNode;


/* Bloom filter with m = 256
 * https://en.wikipedia.org/wiki/Bloom_filter */
#define _Py_BLOOM_FILTER_WORDS 8

typedef struct {
    uint32_t bits[_Py_BLOOM_FILTER_WORDS];
} _PyBloomFilter;

typedef struct {
    uint8_t opcode;
    uint8_t oparg;
    uint8_t valid:1;
    uint8_t linked:1;
    uint8_t chain_depth:6;  // Must be big enough for MAX_CHAIN_DEPTH - 1.
    bool warm;
    int index;           // Index of ENTER_EXECUTOR (if code isn't NULL, below).
    _PyBloomFilter bloom;
    _PyExecutorLinkListNode links;
    PyCodeObject *code;  // Weak (NULL if no corresponding ENTER_EXECUTOR).
} _PyVMData;

/* Depending on the format,
 * the 32 bits between the oparg and operand are:
 * UOP_FORMAT_TARGET:
 *    uint32_t target;
 * UOP_FORMAT_JUMP
 *    uint16_t jump_target;
 *    uint16_t error_target;
 */
typedef struct {
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

typedef struct {
    uint32_t target;
    _Py_BackoffCounter temperature;
    struct _PyExecutorObject *executor;
} _PyExitData;

typedef struct _PyExecutorObject {
    PyObject_VAR_HEAD
    const _PyUOpInstruction *trace;
    _PyVMData vm_data; /* Used by the VM, but opaque to the optimizer */
    uint32_t exit_count;
    uint32_t code_size;
    size_t jit_size;
    void *jit_code;
    void *jit_side_entry;
    _PyExitData exits[1];
} _PyExecutorObject;

/* If pending deletion list gets large enough, then scan,
 * and free any executors that aren't executing
 * i.e. any that aren't a thread's current_executor. */
#define EXECUTOR_DELETE_LIST_MAX 100

// Export for '_opcode' shared extension (JIT compiler).
PyAPI_FUNC(_PyExecutorObject*) _Py_GetExecutor(PyCodeObject *code, int offset);

void _Py_ExecutorInit(_PyExecutorObject *, const _PyBloomFilter *);
void _Py_ExecutorDetach(_PyExecutorObject *);
void _Py_BloomFilter_Init(_PyBloomFilter *);
void _Py_BloomFilter_Add(_PyBloomFilter *bloom, void *obj);
PyAPI_FUNC(void) _Py_Executor_DependsOn(_PyExecutorObject *executor, void *obj);

#define _Py_MAX_ALLOWED_BUILTINS_MODIFICATIONS 3
#define _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS 6

#ifdef _Py_TIER2
PyAPI_FUNC(void) _Py_Executors_InvalidateDependency(PyInterpreterState *interp, void *obj, int is_invalidation);
PyAPI_FUNC(void) _Py_Executors_InvalidateAll(PyInterpreterState *interp, int is_invalidation);
PyAPI_FUNC(void) _Py_Executors_InvalidateCold(PyInterpreterState *interp);

#else
#  define _Py_Executors_InvalidateDependency(A, B, C) ((void)0)
#  define _Py_Executors_InvalidateAll(A, B) ((void)0)
#  define _Py_Executors_InvalidateCold(A) ((void)0)

#endif

// Used as the threshold to trigger executor invalidation when
// trace_run_counter is greater than this value.
#define JIT_CLEANUP_THRESHOLD 100000

// This is the length of the trace we project initially.
#define UOP_MAX_TRACE_LENGTH 1600

#define TRACE_STACK_SIZE 5

int _Py_uop_analyze_and_optimize(_PyInterpreterFrame *frame,
    _PyUOpInstruction *trace, int trace_len, int curr_stackentries,
    _PyBloomFilter *dependencies);

extern PyTypeObject _PyUOpExecutor_Type;


#define UOP_FORMAT_TARGET 0
#define UOP_FORMAT_JUMP 1

static inline uint32_t uop_get_target(const _PyUOpInstruction *inst)
{
    assert(inst->format == UOP_FORMAT_TARGET);
    return inst->target;
}

static inline uint16_t uop_get_jump_target(const _PyUOpInstruction *inst)
{
    assert(inst->format == UOP_FORMAT_JUMP);
    return inst->jump_target;
}

static inline uint16_t uop_get_error_target(const _PyUOpInstruction *inst)
{
    assert(inst->format != UOP_FORMAT_TARGET);
    return inst->error_target;
}

// Holds locals, stack, locals, stack ... co_consts (in that order)
#define MAX_ABSTRACT_INTERP_SIZE 4096

#define TY_ARENA_SIZE (UOP_MAX_TRACE_LENGTH * 5)

// Need extras for root frame and for overflow frame (see TRACE_STACK_PUSH())
#define MAX_ABSTRACT_FRAME_DEPTH (TRACE_STACK_SIZE + 2)

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

typedef union _jit_opt_symbol {
    uint8_t tag;
    JitOptKnownClass cls;
    JitOptKnownValue value;
    JitOptKnownVersion version;
    JitOptTuple tuple;
    JitOptTruthiness truthiness;
} JitOptSymbol;



struct _Py_UOpsAbstractFrame {
    // Max stacklen
    int stack_len;
    int locals_len;

    JitOptSymbol **stack_pointer;
    JitOptSymbol **stack;
    JitOptSymbol **locals;
};

typedef struct _Py_UOpsAbstractFrame _Py_UOpsAbstractFrame;

typedef struct ty_arena {
    int ty_curr_number;
    int ty_max_number;
    JitOptSymbol arena[TY_ARENA_SIZE];
} ty_arena;

typedef struct _JitOptContext {
    char done;
    char out_of_space;
    bool contradiction;
    // The current "executing" frame.
    _Py_UOpsAbstractFrame *frame;
    _Py_UOpsAbstractFrame frames[MAX_ABSTRACT_FRAME_DEPTH];
    int curr_frame_depth;

    // Arena for the symbolic types.
    ty_arena t_arena;

    JitOptSymbol **n_consumed;
    JitOptSymbol **limit;
    JitOptSymbol *locals_and_stack[MAX_ABSTRACT_INTERP_SIZE];
} JitOptContext;

extern bool _Py_uop_sym_is_null(JitOptSymbol *sym);
extern bool _Py_uop_sym_is_not_null(JitOptSymbol *sym);
extern bool _Py_uop_sym_is_const(JitOptContext *ctx, JitOptSymbol *sym);
extern PyObject *_Py_uop_sym_get_const(JitOptContext *ctx, JitOptSymbol *sym);
extern JitOptSymbol *_Py_uop_sym_new_unknown(JitOptContext *ctx);
extern JitOptSymbol *_Py_uop_sym_new_not_null(JitOptContext *ctx);
extern JitOptSymbol *_Py_uop_sym_new_type(
    JitOptContext *ctx, PyTypeObject *typ);
extern JitOptSymbol *_Py_uop_sym_new_const(JitOptContext *ctx, PyObject *const_val);
extern JitOptSymbol *_Py_uop_sym_new_null(JitOptContext *ctx);
extern bool _Py_uop_sym_has_type(JitOptSymbol *sym);
extern bool _Py_uop_sym_matches_type(JitOptSymbol *sym, PyTypeObject *typ);
extern bool _Py_uop_sym_matches_type_version(JitOptSymbol *sym, unsigned int version);
extern void _Py_uop_sym_set_null(JitOptContext *ctx, JitOptSymbol *sym);
extern void _Py_uop_sym_set_non_null(JitOptContext *ctx, JitOptSymbol *sym);
extern void _Py_uop_sym_set_type(JitOptContext *ctx, JitOptSymbol *sym, PyTypeObject *typ);
extern bool _Py_uop_sym_set_type_version(JitOptContext *ctx, JitOptSymbol *sym, unsigned int version);
extern void _Py_uop_sym_set_const(JitOptContext *ctx, JitOptSymbol *sym, PyObject *const_val);
extern bool _Py_uop_sym_is_bottom(JitOptSymbol *sym);
extern int _Py_uop_sym_truthiness(JitOptContext *ctx, JitOptSymbol *sym);
extern PyTypeObject *_Py_uop_sym_get_type(JitOptSymbol *sym);
extern bool _Py_uop_sym_is_immortal(JitOptSymbol *sym);
extern JitOptSymbol *_Py_uop_sym_new_tuple(JitOptContext *ctx, int size, JitOptSymbol **args);
extern JitOptSymbol *_Py_uop_sym_tuple_getitem(JitOptContext *ctx, JitOptSymbol *sym, int item);
extern int _Py_uop_sym_tuple_length(JitOptSymbol *sym);
extern JitOptSymbol *_Py_uop_sym_new_truthiness(JitOptContext *ctx, JitOptSymbol *value, bool truthy);

extern void _Py_uop_abstractcontext_init(JitOptContext *ctx);
extern void _Py_uop_abstractcontext_fini(JitOptContext *ctx);

extern _Py_UOpsAbstractFrame *_Py_uop_frame_new(
    JitOptContext *ctx,
    PyCodeObject *co,
    int curr_stackentries,
    JitOptSymbol **args,
    int arg_len);
extern int _Py_uop_frame_pop(JitOptContext *ctx);

PyAPI_FUNC(PyObject *) _Py_uop_symbols_test(PyObject *self, PyObject *ignored);

PyAPI_FUNC(int) _PyOptimizer_Optimize(_PyInterpreterFrame *frame, _Py_CODEUNIT *start, _PyExecutorObject **exec_ptr, int chain_depth);

static inline int is_terminator(const _PyUOpInstruction *uop)
{
    int opcode = uop->opcode;
    return (
        opcode == _EXIT_TRACE ||
        opcode == _JUMP_TO_TOP
    );
}

PyAPI_FUNC(int) _PyDumpExecutors(FILE *out);
#ifdef _Py_TIER2
extern void _Py_ClearExecutorDeletionList(PyInterpreterState *interp);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPTIMIZER_H */
