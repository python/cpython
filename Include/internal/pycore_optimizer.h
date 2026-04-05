#ifndef Py_INTERNAL_OPTIMIZER_H
#define Py_INTERNAL_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_typedefs.h"      // _PyInterpreterFrame
#include "pycore_uop.h"           // _PyUOpInstruction
#include "pycore_uop_ids.h"
#include "pycore_stackref.h"      // _PyStackRef
#include "pycore_optimizer_types.h"
#include <stdbool.h>


typedef struct _PyJitUopBuffer {
    _PyUOpInstruction *start;
    _PyUOpInstruction *next;
    _PyUOpInstruction *end;
} _PyJitUopBuffer;

typedef struct _JitOptRefBuffer {
    JitOptRef *used;
    JitOptRef *end;
} _JitOptRefBuffer;

typedef struct _JitOptContext {
    char done;
    char out_of_space;
    bool contradiction;
    // Has the builtins dict been watched?
    bool builtins_watched;
    // The current "executing" frame.
    _Py_UOpsAbstractFrame *frame;
    _Py_UOpsAbstractFrame frames[MAX_ABSTRACT_FRAME_DEPTH];
    int curr_frame_depth;

    // Arena for the symbolic types.
    ty_arena t_arena;

    /* To do -- We could make this more space efficient
     * by using a single array and growing the stack and
     * locals toward each other. */
    _JitOptRefBuffer locals;
    _JitOptRefBuffer stack;
    JitOptRef locals_array[ABSTRACT_INTERP_LOCALS_SIZE];
    JitOptRef stack_array[ABSTRACT_INTERP_STACK_SIZE];
    _PyJitUopBuffer out_buffer;
    _PyBloomFilter *dependencies;
} JitOptContext;


static inline void
uop_buffer_init(_PyJitUopBuffer *trace, _PyUOpInstruction *start, uint32_t size)
{
    trace->next = trace->start = start;
    trace->end = start + size;
}

static inline _PyUOpInstruction *
uop_buffer_last(_PyJitUopBuffer *trace)
{
    assert(trace->next > trace->start);
    return trace->next-1;
}

static inline int
uop_buffer_length(_PyJitUopBuffer *trace)
{
    return (int)(trace->next - trace->start);
}

static inline int
uop_buffer_remaining_space(_PyJitUopBuffer *trace)
{
    return (int)(trace->end - trace->next);
}

typedef struct _PyJitTracerInitialState {
    int stack_depth;
    int chain_depth;
    struct _PyExitData *exit;
    PyCodeObject *code; // Strong
    PyFunctionObject *func; // Strong
    struct _PyExecutorObject *executor; // Strong
    _Py_CODEUNIT *start_instr;
    _Py_CODEUNIT *close_loop_instr;
    _Py_CODEUNIT *jump_backward_instr;
} _PyJitTracerInitialState;

typedef struct _PyJitTracerPreviousState {
    int instr_oparg;
    int instr_stacklevel;
    _Py_CODEUNIT *instr;
    PyCodeObject *instr_code; // Strong
    struct _PyInterpreterFrame *instr_frame;
    PyObject *recorded_value; // Strong, may be NULL
} _PyJitTracerPreviousState;

typedef struct _PyJitTracerTranslatorState {
    int jump_backward_seen;
} _PyJitTracerTranslatorState;

typedef struct _PyJitTracerState {
    bool is_tracing;
    _PyJitTracerInitialState initial_state;
    _PyJitTracerPreviousState prev_state;
    _PyJitTracerTranslatorState translator_state;
    JitOptContext opt_context;
    _PyJitUopBuffer code_buffer;
    _PyJitUopBuffer out_buffer;
    _PyUOpInstruction uop_array[2 * UOP_MAX_TRACE_LENGTH];
} _PyJitTracerState;

typedef struct _PyExecutorLinkListNode {
    struct _PyExecutorObject *next;
    struct _PyExecutorObject *previous;
} _PyExecutorLinkListNode;

typedef struct {
    uint8_t opcode;
    uint8_t oparg;
    uint8_t valid;
    uint8_t chain_depth;  // Must be big enough for MAX_CHAIN_DEPTH - 1.
    bool cold;
    uint8_t pending_deletion;
    int32_t index;           // Index of ENTER_EXECUTOR (if code isn't NULL, below).
    int32_t bloom_array_idx;        // Index in interp->executor_blooms/executor_ptrs.
    _PyExecutorLinkListNode links;  // Used by deletion list.
    PyCodeObject *code;  // Weak (NULL if no corresponding ENTER_EXECUTOR).
} _PyVMData;

typedef struct _PyExitData {
    uint32_t target;
    uint16_t index:12;
    uint16_t stack_cache:2;
    uint16_t is_dynamic:1;
    uint16_t is_control_flow:1;
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
    _PyExitData exits[1];
} _PyExecutorObject;

// Export for '_opcode' shared extension (JIT compiler).
PyAPI_FUNC(_PyExecutorObject*) _Py_GetExecutor(PyCodeObject *code, int offset);

int _Py_ExecutorInit(_PyExecutorObject *, const _PyBloomFilter *);
void _Py_ExecutorDetach(_PyExecutorObject *);
PyAPI_FUNC(void) _Py_Executor_DependsOn(_PyExecutorObject *executor, void *obj);

/* We use a bloomfilter with k = 6, m = 256
 * The choice of k and the following constants
 * could do with a more rigorous analysis,
 * but here is a simple analysis:
 *
 * We want to keep the false positive rate low.
 * For n = 5 (a trace depends on 5 objects),
 * we expect 30 bits set, giving a false positive
 * rate of (30/256)**6 == 2.5e-6 which is plenty
 * good enough.
 *
 * However with n = 10 we expect 60 bits set (worst case),
 * giving a false positive of (60/256)**6 == 0.0001
 *
 * We choose k = 6, rather than a higher number as
 * it means the false positive rate grows slower for high n.
 *
 * n = 5, k = 6 => fp = 2.6e-6
 * n = 5, k = 8 => fp = 3.5e-7
 * n = 10, k = 6 => fp = 1.6e-4
 * n = 10, k = 8 => fp = 0.9e-4
 * n = 15, k = 6 => fp = 0.18%
 * n = 15, k = 8 => fp = 0.23%
 * n = 20, k = 6 => fp = 1.1%
 * n = 20, k = 8 => fp = 2.3%
 *
 * The above analysis assumes perfect hash functions,
 * but those don't exist, so the real false positive
 * rates may be worse.
 */

#define _Py_BLOOM_FILTER_K 6
#define _Py_BLOOM_FILTER_SEED 20221211

static inline uint64_t
address_to_hash(void *ptr) {
    assert(ptr != NULL);
    uint64_t uhash = _Py_BLOOM_FILTER_SEED;
    uintptr_t addr = (uintptr_t)ptr;
    for (int i = 0; i < SIZEOF_VOID_P; i++) {
        uhash ^= addr & 255;
        uhash *= (uint64_t)PyHASH_MULTIPLIER;
        addr >>= 8;
    }
    return uhash;
}

static inline void
_Py_BloomFilter_Init(_PyBloomFilter *bloom)
{
    for (int i = 0; i < _Py_BLOOM_FILTER_WORDS; i++) {
        bloom->bits[i] = 0;
    }
}

static inline void
_Py_BloomFilter_Add(_PyBloomFilter *bloom, void *ptr)
{
    uint64_t hash = address_to_hash(ptr);
    assert(_Py_BLOOM_FILTER_K <= 8);
    for (int i = 0; i < _Py_BLOOM_FILTER_K; i++) {
        uint8_t bits = hash & 255;
        bloom->bits[bits >> _Py_BLOOM_FILTER_WORD_SHIFT] |=
            (_Py_bloom_filter_word_t)1 << (bits & (_Py_BLOOM_FILTER_BITS_PER_WORD - 1));
        hash >>= 8;
    }
}

static inline bool
bloom_filter_may_contain(const _PyBloomFilter *bloom, const _PyBloomFilter *hashes)
{
    for (int i = 0; i < _Py_BLOOM_FILTER_WORDS; i++) {
        if ((bloom->bits[i] & hashes->bits[i]) != hashes->bits[i]) {
            return false;
        }
    }
    return true;
}

#define _Py_MAX_ALLOWED_BUILTINS_MODIFICATIONS 3
#define _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS 6

#ifdef _Py_TIER2
PyAPI_FUNC(void) _Py_Executors_InvalidateDependency(PyInterpreterState *interp, void *obj, int is_invalidation);
PyAPI_FUNC(void) _Py_Executors_InvalidateAll(PyInterpreterState *interp, int is_invalidation);
PyAPI_FUNC(void) _Py_Executors_InvalidateCold(PyInterpreterState *interp);

#else
#  define _Py_Executors_InvalidateDependency(A, B, C) ((void)0)
#  define _Py_Executors_InvalidateAll(A, B) ((void)0)

#endif

// Used as the threshold to trigger executor invalidation when
// executor_creation_counter is greater than this value.
// This value is arbitrary and was not optimized.
#define JIT_CLEANUP_THRESHOLD 1000

int _Py_uop_analyze_and_optimize(
    _PyThreadStateImpl *tstate,
    _PyUOpInstruction *input, int trace_len, int curr_stackentries,
    _PyUOpInstruction *output, _PyBloomFilter *dependencies);

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


#define REF_IS_BORROWED 1
#define REF_IS_UNIQUE   2
#define REF_IS_INVALID  3
#define REF_TAG_BITS    3

#define REF_GET_TAG(x)   ((uintptr_t)(x) & (REF_TAG_BITS))
#define REF_CLEAR_TAG(x) ((uintptr_t)(x) & (~REF_TAG_BITS))

#define JIT_BITS_TO_PTR_MASKED(REF) ((JitOptSymbol *)(((REF).bits) & (~REF_TAG_BITS)))

static inline JitOptSymbol *
PyJitRef_Unwrap(JitOptRef ref)
{
    return JIT_BITS_TO_PTR_MASKED(ref);
}

bool _Py_uop_symbol_is_immortal(JitOptSymbol *sym);


static inline JitOptRef
PyJitRef_Wrap(JitOptSymbol *sym)
{
    return (JitOptRef){.bits=(uintptr_t)sym};
}

static inline JitOptRef
PyJitRef_WrapInvalid(void *ptr)
{
    return (JitOptRef){.bits = REF_CLEAR_TAG((uintptr_t)ptr) | REF_IS_INVALID};
}

static inline bool
PyJitRef_IsInvalid(JitOptRef ref)
{
    return REF_GET_TAG(ref.bits) == REF_IS_INVALID;
}

static inline JitOptRef
PyJitRef_MakeUnique(JitOptRef ref)
{
    return (JitOptRef){ REF_CLEAR_TAG(ref.bits) | REF_IS_UNIQUE };
}

static inline bool
PyJitRef_IsUnique(JitOptRef ref)
{
    return REF_GET_TAG(ref.bits) == REF_IS_UNIQUE;
}

static inline JitOptRef
PyJitRef_StripBorrowInfo(JitOptRef ref)
{
    if (PyJitRef_IsUnique(ref)) {
        return ref;
    }
    return (JitOptRef){ .bits = REF_CLEAR_TAG(ref.bits) };
}

static inline JitOptRef
PyJitRef_StripReferenceInfo(JitOptRef ref)
{
    return PyJitRef_Wrap(PyJitRef_Unwrap(ref));
}

static inline JitOptRef
PyJitRef_RemoveUnique(JitOptRef ref)
{
    if (PyJitRef_IsUnique(ref)) {
        ref = PyJitRef_StripReferenceInfo(ref);
    }
    return ref;
}

static inline JitOptRef
PyJitRef_Borrow(JitOptRef ref)
{
    return (JitOptRef){ .bits = REF_CLEAR_TAG(ref.bits) | REF_IS_BORROWED };
}

static const JitOptRef PyJitRef_NULL = {.bits = REF_IS_BORROWED};

static inline bool
PyJitRef_IsNull(JitOptRef ref)
{
    return ref.bits == PyJitRef_NULL.bits;
}

static inline int
PyJitRef_IsBorrowed(JitOptRef ref)
{
    return REF_GET_TAG(ref.bits) == REF_IS_BORROWED;
}

extern bool _Py_uop_sym_is_null(JitOptRef sym);
extern bool _Py_uop_sym_is_not_null(JitOptRef sym);
extern bool _Py_uop_sym_is_const(JitOptContext *ctx, JitOptRef sym);
extern PyObject *_Py_uop_sym_get_const(JitOptContext *ctx, JitOptRef sym);
extern JitOptRef _Py_uop_sym_new_unknown(JitOptContext *ctx);
extern JitOptRef _Py_uop_sym_new_not_null(JitOptContext *ctx);
extern JitOptRef _Py_uop_sym_new_type(
    JitOptContext *ctx, PyTypeObject *typ);

extern JitOptRef _Py_uop_sym_new_const(JitOptContext *ctx, PyObject *const_val);
extern JitOptRef _Py_uop_sym_new_const_steal(JitOptContext *ctx, PyObject *const_val);
bool _Py_uop_sym_is_safe_const(JitOptContext *ctx, JitOptRef sym);
bool _Py_uop_sym_is_not_container(JitOptRef sym);
_PyStackRef _Py_uop_sym_get_const_as_stackref(JitOptContext *ctx, JitOptRef sym);
extern JitOptRef _Py_uop_sym_new_null(JitOptContext *ctx);
extern bool _Py_uop_sym_has_type(JitOptRef sym);
extern bool _Py_uop_sym_matches_type(JitOptRef sym, PyTypeObject *typ);
extern bool _Py_uop_sym_matches_type_version(JitOptRef sym, unsigned int version);
extern void _Py_uop_sym_set_null(JitOptContext *ctx, JitOptRef sym);
extern void _Py_uop_sym_set_non_null(JitOptContext *ctx, JitOptRef sym);
extern void _Py_uop_sym_set_type(JitOptContext *ctx, JitOptRef sym, PyTypeObject *typ);
extern bool _Py_uop_sym_set_type_version(JitOptContext *ctx, JitOptRef sym, unsigned int version);
extern void _Py_uop_sym_set_const(JitOptContext *ctx, JitOptRef sym, PyObject *const_val);
extern bool _Py_uop_sym_is_bottom(JitOptRef sym);
extern int _Py_uop_sym_truthiness(JitOptContext *ctx, JitOptRef sym);
extern PyTypeObject *_Py_uop_sym_get_type(JitOptRef sym);
extern JitOptRef _Py_uop_sym_new_tuple(JitOptContext *ctx, int size, JitOptRef *args);
extern JitOptRef _Py_uop_sym_tuple_getitem(JitOptContext *ctx, JitOptRef sym, Py_ssize_t item);
extern Py_ssize_t _Py_uop_sym_tuple_length(JitOptRef sym);
extern JitOptRef _Py_uop_sym_new_truthiness(JitOptContext *ctx, JitOptRef value, bool truthy);
extern bool _Py_uop_sym_is_compact_int(JitOptRef sym);
extern JitOptRef _Py_uop_sym_new_compact_int(JitOptContext *ctx);
extern void _Py_uop_sym_set_compact_int(JitOptContext *ctx,  JitOptRef sym);
extern JitOptRef _Py_uop_sym_new_predicate(JitOptContext *ctx, JitOptRef lhs_ref, JitOptRef rhs_ref, JitOptPredicateKind kind);
extern void _Py_uop_sym_apply_predicate_narrowing(JitOptContext *ctx, JitOptRef sym, bool branch_is_true);
extern void _Py_uop_sym_set_recorded_value(JitOptContext *ctx, JitOptRef sym, PyObject *value);
extern void _Py_uop_sym_set_recorded_type(JitOptContext *ctx, JitOptRef sym, PyTypeObject *type);
extern void _Py_uop_sym_set_recorded_gen_func(JitOptContext *ctx, JitOptRef ref, PyFunctionObject *value);
extern PyCodeObject *_Py_uop_sym_get_probable_func_code(JitOptRef sym);
extern PyObject *_Py_uop_sym_get_probable_value(JitOptRef sym);
extern PyTypeObject *_Py_uop_sym_get_probable_type(JitOptRef sym);
extern JitOptRef *_Py_uop_sym_set_stack_depth(JitOptContext *ctx, int stack_depth, JitOptRef *current_sp);
extern uint32_t _Py_uop_sym_get_func_version(JitOptRef ref);
bool _Py_uop_sym_set_func_version(JitOptContext *ctx, JitOptRef ref, uint32_t version);

extern void _Py_uop_abstractcontext_init(JitOptContext *ctx, _PyBloomFilter *dependencies);
extern void _Py_uop_abstractcontext_fini(JitOptContext *ctx);

extern _Py_UOpsAbstractFrame *_Py_uop_frame_new(
    JitOptContext *ctx,
    PyCodeObject *co,
    JitOptRef *args,
    int arg_len);

extern _Py_UOpsAbstractFrame *_Py_uop_frame_new_from_symbol(
    JitOptContext *ctx,
    JitOptRef callable,
    JitOptRef *args,
    int arg_len);

extern int _Py_uop_frame_pop(JitOptContext *ctx, PyCodeObject *co);

PyAPI_FUNC(PyObject *) _Py_uop_symbols_test(PyObject *self, PyObject *ignored);

PyAPI_FUNC(int) _PyOptimizer_Optimize(_PyInterpreterFrame *frame, PyThreadState *tstate);

static inline _PyExecutorObject *_PyExecutor_FromExit(_PyExitData *exit)
{
    _PyExitData *exit0 = exit - exit->index;
    return (_PyExecutorObject *)(((char *)exit0) - offsetof(_PyExecutorObject, exits));
}

extern _PyExecutorObject *_PyExecutor_GetColdExecutor(void);
extern _PyExecutorObject *_PyExecutor_GetColdDynamicExecutor(void);

PyAPI_FUNC(void) _PyExecutor_ClearExit(_PyExitData *exit);

extern void _PyExecutor_Free(_PyExecutorObject *self);

PyAPI_FUNC(int) _PyDumpExecutors(FILE *out);
#ifdef _Py_TIER2
PyAPI_FUNC(void) _Py_ClearExecutorDeletionList(PyInterpreterState *interp);
#endif

PyAPI_FUNC(int) _PyJit_translate_single_bytecode_to_trace(PyThreadState *tstate, _PyInterpreterFrame *frame, _Py_CODEUNIT *next_instr, int stop_tracing_opcode);

PyAPI_FUNC(int)
_PyJit_TryInitializeTracing(PyThreadState *tstate, _PyInterpreterFrame *frame,
    _Py_CODEUNIT *curr_instr, _Py_CODEUNIT *start_instr,
    _Py_CODEUNIT *close_loop_instr, _PyStackRef *stack_pointer, int chain_depth, _PyExitData *exit,
    int oparg, _PyExecutorObject *current_executor);

PyAPI_FUNC(void) _PyJit_FinalizeTracing(PyThreadState *tstate, int err);
PyAPI_FUNC(bool) _PyJit_EnterExecutorShouldStopTracing(int og_opcode);

void _PyPrintExecutor(_PyExecutorObject *executor, const _PyUOpInstruction *marker);
void _PyJit_TracerFree(_PyThreadStateImpl *_tstate);

#ifdef _Py_TIER2
typedef void (*_Py_RecordFuncPtr)(_PyInterpreterFrame *frame, _PyStackRef *stackpointer, int oparg, PyObject **recorded_value);
PyAPI_DATA(const _Py_RecordFuncPtr) _PyOpcode_RecordFunctions[];
PyAPI_DATA(const uint8_t) _PyOpcode_RecordFunctionIndices[256];
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPTIMIZER_H */
