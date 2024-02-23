/*
 * This file contains the support code for CPython's uops redundancy eliminator.
 * It also performs some simple optimizations.
 * It performs a traditional data-flow analysis[1] over the trace of uops.
 * Using the information gained, it chooses to emit, or skip certain instructions
 * if possible.
 *
 * [1] For information on data-flow analysis, please see
 * https://clang.llvm.org/docs/DataFlowAnalysisIntro.html
 *
 * */
#include "Python.h"
#include "opcode.h"
#include "pycore_dict.h"
#include "pycore_interp.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uop_metadata.h"
#include "pycore_dict.h"
#include "pycore_long.h"
#include "cpython/optimizer.h"
#include "pycore_optimizer.h"
#include "pycore_object.h"
#include "pycore_dict.h"
#include "pycore_function.h"
#include "pycore_uop_metadata.h"
#include "pycore_uop_ids.h"
#include "pycore_range.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Holds locals, stack, locals, stack ... co_consts (in that order)
#define MAX_ABSTRACT_INTERP_SIZE 4096

#define OVERALLOCATE_FACTOR 5

#define TY_ARENA_SIZE (UOP_MAX_TRACE_LENGTH * OVERALLOCATE_FACTOR)

// Need extras for root frame and for overflow frame (see TRACE_STACK_PUSH())
#define MAX_ABSTRACT_FRAME_DEPTH (TRACE_STACK_SIZE + 2)

#ifdef Py_DEBUG
    extern const char *_PyUOpName(int index);
    static const char *const DEBUG_ENV = "PYTHON_OPT_DEBUG";
    static inline int get_lltrace(void) {
        char *uop_debug = Py_GETENV(DEBUG_ENV);
        int lltrace = 0;
        if (uop_debug != NULL && *uop_debug >= '0') {
            lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
        }
        return lltrace;
    }
    #define DPRINTF(level, ...) \
    if (get_lltrace() >= (level)) { printf(__VA_ARGS__); }
#else
    #define DPRINTF(level, ...)
#endif


// Flags for below.
#define KNOWN      1 << 0
#define TRUE_CONST 1 << 1
#define IS_NULL    1 << 2
#define NOT_NULL   1 << 3

typedef struct {
    int flags;
    PyTypeObject *typ;
    // constant propagated value (might be NULL)
    PyObject *const_val;
} _Py_UOpsSymType;


typedef struct _Py_UOpsAbstractFrame {
    // Max stacklen
    int stack_len;
    int locals_len;

    _Py_UOpsSymType **stack_pointer;
    _Py_UOpsSymType **stack;
    _Py_UOpsSymType **locals;
} _Py_UOpsAbstractFrame;


typedef struct ty_arena {
    int ty_curr_number;
    int ty_max_number;
    _Py_UOpsSymType arena[TY_ARENA_SIZE];
} ty_arena;

// Tier 2 types meta interpreter
typedef struct _Py_UOpsAbstractInterpContext {
    PyObject_HEAD
    // The current "executing" frame.
    _Py_UOpsAbstractFrame *frame;
    _Py_UOpsAbstractFrame frames[MAX_ABSTRACT_FRAME_DEPTH];
    int curr_frame_depth;

    // Arena for the symbolic types.
    ty_arena t_arena;

    _Py_UOpsSymType **n_consumed;
    _Py_UOpsSymType **limit;
    _Py_UOpsSymType *locals_and_stack[MAX_ABSTRACT_INTERP_SIZE];
} _Py_UOpsAbstractInterpContext;

static inline _Py_UOpsSymType* sym_new_unknown(_Py_UOpsAbstractInterpContext *ctx);

// 0 on success, -1 on error.
static _Py_UOpsAbstractFrame *
ctx_frame_new(
    _Py_UOpsAbstractInterpContext *ctx,
    PyCodeObject *co,
    _Py_UOpsSymType **localsplus_start,
    int n_locals_already_filled,
    int curr_stackentries
)
{
    assert(ctx->curr_frame_depth < MAX_ABSTRACT_FRAME_DEPTH);
    _Py_UOpsAbstractFrame *frame = &ctx->frames[ctx->curr_frame_depth];

    frame->stack_len = co->co_stacksize;
    frame->locals_len = co->co_nlocalsplus;

    frame->locals = localsplus_start;
    frame->stack = frame->locals + co->co_nlocalsplus;
    frame->stack_pointer = frame->stack + curr_stackentries;
    ctx->n_consumed = localsplus_start + (co->co_nlocalsplus + co->co_stacksize);
    if (ctx->n_consumed >= ctx->limit) {
        return NULL;
    }


    // Initialize with the initial state of all local variables
    for (int i = n_locals_already_filled; i < co->co_nlocalsplus; i++) {
        _Py_UOpsSymType *local = sym_new_unknown(ctx);
        if (local == NULL) {
            return NULL;
        }
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stackentries; i++) {
        _Py_UOpsSymType *stackvar = sym_new_unknown(ctx);
        if (stackvar == NULL) {
            return NULL;
        }
        frame->stack[i] = stackvar;
    }

    return frame;
}

static void
abstractcontext_fini(_Py_UOpsAbstractInterpContext *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->curr_frame_depth = 0;
    int tys = ctx->t_arena.ty_curr_number;
    for (int i = 0; i < tys; i++) {
        Py_CLEAR(ctx->t_arena.arena[i].const_val);
    }
}

static int
abstractcontext_init(
    _Py_UOpsAbstractInterpContext *ctx,
    PyCodeObject *co,
    int curr_stacklen,
    int ir_entries
)
{
    ctx->limit = ctx->locals_and_stack + MAX_ABSTRACT_INTERP_SIZE;
    ctx->n_consumed = ctx->locals_and_stack;
#ifdef Py_DEBUG // Aids debugging a little. There should never be NULL in the abstract interpreter.
    for (int i = 0 ; i < MAX_ABSTRACT_INTERP_SIZE; i++) {
        ctx->locals_and_stack[i] = NULL;
    }
#endif

    // Setup the arena for sym expressions.
    ctx->t_arena.ty_curr_number = 0;
    ctx->t_arena.ty_max_number = TY_ARENA_SIZE;

    // Frame setup
    ctx->curr_frame_depth = 0;
    _Py_UOpsAbstractFrame *frame = ctx_frame_new(ctx, co, ctx->n_consumed, 0, curr_stacklen);
    if (frame == NULL) {
        return -1;
    }
    ctx->curr_frame_depth++;
    ctx->frame = frame;
    return 0;
}


static int
ctx_frame_pop(
    _Py_UOpsAbstractInterpContext *ctx
)
{
    _Py_UOpsAbstractFrame *frame = ctx->frame;

    ctx->n_consumed = frame->locals;
    ctx->curr_frame_depth--;
    assert(ctx->curr_frame_depth >= 1);
    ctx->frame = &ctx->frames[ctx->curr_frame_depth - 1];

    return 0;
}


// Takes a borrowed reference to const_val, turns that into a strong reference.
static _Py_UOpsSymType*
sym_new(_Py_UOpsAbstractInterpContext *ctx,
                               PyObject *const_val)
{
    _Py_UOpsSymType *self = &ctx->t_arena.arena[ctx->t_arena.ty_curr_number];
    if (ctx->t_arena.ty_curr_number >= ctx->t_arena.ty_max_number) {
        OPT_STAT_INC(optimizer_failure_reason_no_memory);
        DPRINTF(1, "out of space for symbolic expression type\n");
        return NULL;
    }
    ctx->t_arena.ty_curr_number++;
    self->const_val = NULL;
    self->typ = NULL;
    self->flags = 0;

    if (const_val != NULL) {
        self->const_val = Py_NewRef(const_val);
    }

    return self;
}

static inline void
sym_set_flag(_Py_UOpsSymType *sym, int flag)
{
    sym->flags |= flag;
}

static inline bool
sym_has_flag(_Py_UOpsSymType *sym, int flag)
{
    return (sym->flags & flag) != 0;
}

static inline bool
sym_is_known(_Py_UOpsSymType *sym)
{
    return sym_has_flag(sym, KNOWN);
}

static inline bool
sym_is_not_null(_Py_UOpsSymType *sym)
{
    return (sym->flags & (IS_NULL | NOT_NULL)) == NOT_NULL;
}

static inline bool
sym_is_null(_Py_UOpsSymType *sym)
{
    return (sym->flags & (IS_NULL | NOT_NULL)) == IS_NULL;
}

static inline bool
sym_is_const(_Py_UOpsSymType *sym)
{
    return (sym->flags & TRUE_CONST) != 0;
}

static inline PyObject *
sym_get_const(_Py_UOpsSymType *sym)
{
    assert(sym_is_const(sym));
    assert(sym->const_val);
    return sym->const_val;
}

static inline void
sym_set_type(_Py_UOpsSymType *sym, PyTypeObject *tp)
{
    assert(PyType_Check(tp));
    sym->typ = tp;
    sym_set_flag(sym, KNOWN);
    sym_set_flag(sym, NOT_NULL);
}

static inline void
sym_set_null(_Py_UOpsSymType *sym)
{
    sym_set_flag(sym, IS_NULL);
    sym_set_flag(sym, KNOWN);
}


static inline _Py_UOpsSymType*
sym_new_unknown(_Py_UOpsAbstractInterpContext *ctx)
{
    return sym_new(ctx,NULL);
}

static inline _Py_UOpsSymType*
sym_new_known_notnull(_Py_UOpsAbstractInterpContext *ctx)
{
    _Py_UOpsSymType *res = sym_new_unknown(ctx);
    if (res == NULL) {
        return NULL;
    }
    sym_set_flag(res, KNOWN);
    sym_set_flag(res, NOT_NULL);
    return res;
}

static inline _Py_UOpsSymType*
sym_new_known_type(_Py_UOpsAbstractInterpContext *ctx,
                      PyTypeObject *typ)
{
    _Py_UOpsSymType *res = sym_new(ctx,NULL);
    if (res == NULL) {
        return NULL;
    }
    sym_set_type(res, typ);
    return res;
}

// Takes a borrowed reference to const_val.
static inline _Py_UOpsSymType*
sym_new_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val)
{
    assert(const_val != NULL);
    _Py_UOpsSymType *temp = sym_new(
        ctx,
        const_val
    );
    if (temp == NULL) {
        return NULL;
    }
    sym_set_type(temp, Py_TYPE(const_val));
    sym_set_flag(temp, TRUE_CONST);
    sym_set_flag(temp, KNOWN);
    sym_set_flag(temp, NOT_NULL);
    return temp;
}

static _Py_UOpsSymType*
sym_new_null(_Py_UOpsAbstractInterpContext *ctx)
{
    _Py_UOpsSymType *null_sym = sym_new_unknown(ctx);
    if (null_sym == NULL) {
        return NULL;
    }
    sym_set_null(null_sym);
    return null_sym;
}


static inline bool
sym_matches_type(_Py_UOpsSymType *sym, PyTypeObject *typ)
{
    assert(typ == NULL || PyType_Check(typ));
    if (!sym_has_flag(sym, KNOWN)) {
        return false;
    }
    return sym->typ == typ;
}


static inline bool
op_is_end(uint32_t opcode)
{
    return opcode == _EXIT_TRACE || opcode == _JUMP_TO_TOP;
}

static int
get_mutations(PyObject* dict) {
    assert(PyDict_CheckExact(dict));
    PyDictObject *d = (PyDictObject *)dict;
    return (d->ma_version_tag >> DICT_MAX_WATCHERS) & ((1 << DICT_WATCHED_MUTATION_BITS)-1);
}

static void
increment_mutations(PyObject* dict) {
    assert(PyDict_CheckExact(dict));
    PyDictObject *d = (PyDictObject *)dict;
    d->ma_version_tag += (1 << DICT_MAX_WATCHERS);
}

/* The first two dict watcher IDs are reserved for CPython,
 * so we don't need to check that they haven't been used */
#define BUILTINS_WATCHER_ID 0
#define GLOBALS_WATCHER_ID  1

static int
globals_watcher_callback(PyDict_WatchEvent event, PyObject* dict,
                         PyObject* key, PyObject* new_value)
{
    RARE_EVENT_STAT_INC(watched_globals_modification);
    assert(get_mutations(dict) < _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS);
    _Py_Executors_InvalidateDependency(_PyInterpreterState_GET(), dict);
    increment_mutations(dict);
    PyDict_Unwatch(GLOBALS_WATCHER_ID, dict);
    return 0;
}

static PyObject *
convert_global_to_const(_PyUOpInstruction *inst, PyObject *obj)
{
    assert(inst->opcode == _LOAD_GLOBAL_MODULE || inst->opcode == _LOAD_GLOBAL_BUILTINS || inst->opcode == _LOAD_ATTR_MODULE);
    assert(PyDict_CheckExact(obj));
    PyDictObject *dict = (PyDictObject *)obj;
    assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE);
    PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dict->ma_keys);
    assert(inst->operand <= UINT16_MAX);
    if ((int)inst->operand >= dict->ma_keys->dk_nentries) {
        return NULL;
    }
    PyObject *res = entries[inst->operand].me_value;
    if (res == NULL) {
        return NULL;
    }
    if (_Py_IsImmortal(res)) {
        inst->opcode = (inst->oparg & 1) ? _LOAD_CONST_INLINE_BORROW_WITH_NULL : _LOAD_CONST_INLINE_BORROW;
    }
    else {
        inst->opcode = (inst->oparg & 1) ? _LOAD_CONST_INLINE_WITH_NULL : _LOAD_CONST_INLINE;
    }
    inst->operand = (uint64_t)res;
    return res;
}

static int
incorrect_keys(_PyUOpInstruction *inst, PyObject *obj)
{
    if (!PyDict_CheckExact(obj)) {
        return 1;
    }
    PyDictObject *dict = (PyDictObject *)obj;
    if (dict->ma_keys->dk_version != inst->operand) {
        return 1;
    }
    return 0;
}

/* Returns 1 if successfully optimized
 *         0 if the trace is not suitable for optimization (yet)
 *        -1 if there was an error. */
static int
remove_globals(_PyInterpreterFrame *frame, _PyUOpInstruction *buffer,
               int buffer_size, _PyBloomFilter *dependencies)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *builtins = frame->f_builtins;
    if (builtins != interp->builtins) {
        return 1;
    }
    PyObject *globals = frame->f_globals;
    assert(PyFunction_Check(((PyFunctionObject *)frame->f_funcobj)));
    assert(((PyFunctionObject *)frame->f_funcobj)->func_builtins == builtins);
    assert(((PyFunctionObject *)frame->f_funcobj)->func_globals == globals);
    /* In order to treat globals as constants, we need to
     * know that the globals dict is the one we expected, and
     * that it hasn't changed
     * In order to treat builtins as constants,  we need to
     * know that the builtins dict is the one we expected, and
     * that it hasn't changed and that the global dictionary's
     * keys have not changed */

    /* These values represent stacks of booleans (one bool per bit).
     * Pushing a frame shifts left, popping a frame shifts right. */
    uint32_t builtins_checked = 0;
    uint32_t builtins_watched = 0;
    uint32_t globals_checked = 0;
    uint32_t globals_watched = 0;
    if (interp->dict_state.watchers[GLOBALS_WATCHER_ID] == NULL) {
        interp->dict_state.watchers[GLOBALS_WATCHER_ID] = globals_watcher_callback;
    }
    for (int pc = 0; pc < buffer_size; pc++) {
        _PyUOpInstruction *inst = &buffer[pc];
        int opcode = inst->opcode;
        switch(opcode) {
            case _GUARD_BUILTINS_VERSION:
                if (incorrect_keys(inst, builtins)) {
                    return 0;
                }
                if (interp->rare_events.builtin_dict >= _Py_MAX_ALLOWED_BUILTINS_MODIFICATIONS) {
                    continue;
                }
                if ((builtins_watched & 1) == 0) {
                    PyDict_Watch(BUILTINS_WATCHER_ID, builtins);
                    builtins_watched |= 1;
                }
                if (builtins_checked & 1) {
                    buffer[pc].opcode = NOP;
                }
                else {
                    buffer[pc].opcode = _CHECK_BUILTINS;
                    buffer[pc].operand = (uintptr_t)builtins;
                    builtins_checked |= 1;
                }
                break;
            case _GUARD_GLOBALS_VERSION:
                if (incorrect_keys(inst, globals)) {
                    return 0;
                }
                uint64_t watched_mutations = get_mutations(globals);
                if (watched_mutations >= _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
                    continue;
                }
                if ((globals_watched & 1) == 0) {
                    PyDict_Watch(GLOBALS_WATCHER_ID, globals);
                    _Py_BloomFilter_Add(dependencies, globals);
                    globals_watched |= 1;
                }
                if (globals_checked & 1) {
                    buffer[pc].opcode = NOP;
                }
                else {
                    buffer[pc].opcode = _CHECK_GLOBALS;
                    buffer[pc].operand = (uintptr_t)globals;
                    globals_checked |= 1;
                }
                break;
            case _LOAD_GLOBAL_BUILTINS:
                if (globals_checked & builtins_checked & globals_watched & builtins_watched & 1) {
                    convert_global_to_const(inst, builtins);
                }
                break;
            case _LOAD_GLOBAL_MODULE:
                if (globals_checked & globals_watched & 1) {
                    convert_global_to_const(inst, globals);
                }
                break;
            case _PUSH_FRAME:
            {
                globals_checked <<= 1;
                globals_watched <<= 1;
                builtins_checked <<= 1;
                builtins_watched <<= 1;
                PyFunctionObject *func = (PyFunctionObject *)buffer[pc].operand;
                if (func == NULL) {
                    return 1;
                }
                assert(PyFunction_Check(func));
                globals = func->func_globals;
                builtins = func->func_builtins;
                if (builtins != interp->builtins) {
                    return 1;
                }
                break;
            }
            case _POP_FRAME:
            {
                globals_checked >>= 1;
                globals_watched >>= 1;
                builtins_checked >>= 1;
                builtins_watched >>= 1;
                PyFunctionObject *func = (PyFunctionObject *)buffer[pc].operand;
                assert(PyFunction_Check(func));
                globals = func->func_globals;
                builtins = func->func_builtins;
                break;
            }
            default:
                if (op_is_end(opcode)) {
                    return 1;
                }
                break;
        }
    }
    return 0;
}



#define STACK_LEVEL()     ((int)(stack_pointer - ctx->frame->stack))

#define GETLOCAL(idx)          ((ctx->frame->locals[idx]))

#define REPLACE_OP(INST, OP, ARG, OPERAND)    \
    INST->opcode = OP;            \
    INST->oparg = ARG;            \
    INST->operand = OPERAND;

#define OUT_OF_SPACE_IF_NULL(EXPR)     \
    do {                               \
        if ((EXPR) == NULL) {          \
            goto out_of_space;         \
        }                              \
    } while (0);

#define _LOAD_ATTR_NOT_NULL \
    do {                    \
    OUT_OF_SPACE_IF_NULL(attr = sym_new_known_notnull(ctx)); \
    OUT_OF_SPACE_IF_NULL(null = sym_new_null(ctx)); \
    } while (0);


/* 1 for success, 0 for not ready, cannot error at the moment. */
static int
uop_redundancy_eliminator(
    PyCodeObject *co,
    _PyUOpInstruction *trace,
    int trace_len,
    int curr_stacklen,
    _PyBloomFilter *dependencies
)
{

    _Py_UOpsAbstractInterpContext context;
    _Py_UOpsAbstractInterpContext *ctx = &context;

    if (abstractcontext_init(
        ctx,
        co, curr_stacklen,
        trace_len) < 0) {
        goto out_of_space;
    }

    for (_PyUOpInstruction *this_instr = trace;
         this_instr < trace + trace_len && !op_is_end(this_instr->opcode);
         this_instr++) {

        int oparg = this_instr->oparg;
        uint32_t opcode = this_instr->opcode;

        _Py_UOpsSymType **stack_pointer = ctx->frame->stack_pointer;

        DPRINTF(3, "Abstract interpreting %s:%d ",
                _PyUOpName(opcode),
                oparg);
        switch (opcode) {
#include "tier2_redundancy_eliminator_cases.c.h"

            default:
                DPRINTF(1, "Unknown opcode in abstract interpreter\n");
                Py_UNREACHABLE();
        }
        assert(ctx->frame != NULL);
        DPRINTF(3, " stack_level %d\n", STACK_LEVEL());
        ctx->frame->stack_pointer = stack_pointer;
        assert(STACK_LEVEL() >= 0);
    }

    abstractcontext_fini(ctx);
    return 1;

out_of_space:
    DPRINTF(1, "Out of space in abstract interpreter\n");
    abstractcontext_fini(ctx);
    return 0;

error:
    DPRINTF(1, "Encountered error in abstract interpreter\n");
    abstractcontext_fini(ctx);
    return 0;
}


static void
remove_unneeded_uops(_PyUOpInstruction *buffer, int buffer_size)
{
    /* Remove _SET_IP and _CHECK_VALIDITY where possible.
     * _SET_IP is needed if the following instruction escapes or
     * could error. _CHECK_VALIDITY is needed if the previous
     * instruction could have escaped. */
    int last_set_ip = -1;
    bool may_have_escaped = true;
    for (int pc = 0; pc < buffer_size; pc++) {
        int opcode = buffer[pc].opcode;
        switch (opcode) {
            case _SET_IP:
                buffer[pc].opcode = NOP;
                last_set_ip = pc;
                break;
            case _CHECK_VALIDITY:
                if (may_have_escaped) {
                    may_have_escaped = false;
                }
                else {
                    buffer[pc].opcode = NOP;
                }
                break;
            case _CHECK_VALIDITY_AND_SET_IP:
                if (may_have_escaped) {
                    may_have_escaped = false;
                    buffer[pc].opcode = _CHECK_VALIDITY;
                }
                else {
                    buffer[pc].opcode = NOP;
                }
                last_set_ip = pc;
                break;
            case _POP_TOP:
            {
                _PyUOpInstruction *last = &buffer[pc-1];
                while (last->opcode == _NOP) {
                    last--;
                }
                if (last->opcode == _LOAD_CONST_INLINE  ||
                    last->opcode == _LOAD_CONST_INLINE_BORROW ||
                    last->opcode == _LOAD_FAST ||
                    last->opcode == _COPY
                ) {
                    last->opcode = _NOP;
                    buffer[pc].opcode = NOP;
                }
                break;
            }
            case _JUMP_TO_TOP:
            case _EXIT_TRACE:
                return;
            default:
            {
                bool needs_ip = false;
                if (_PyUop_Flags[opcode] & HAS_ESCAPES_FLAG) {
                    needs_ip = true;
                    may_have_escaped = true;
                }
                if (_PyUop_Flags[opcode] & HAS_ERROR_FLAG) {
                    needs_ip = true;
                }
                if (needs_ip && last_set_ip >= 0) {
                    if (buffer[last_set_ip].opcode == _CHECK_VALIDITY) {
                        buffer[last_set_ip].opcode = _CHECK_VALIDITY_AND_SET_IP;
                    }
                    else {
                        assert(buffer[last_set_ip].opcode == _NOP);
                        buffer[last_set_ip].opcode = _SET_IP;
                    }
                    last_set_ip = -1;
                }
            }
        }
    }
}

static void
peephole_opt(_PyInterpreterFrame *frame, _PyUOpInstruction *buffer, int buffer_size)
{
    PyCodeObject *co = (PyCodeObject *)frame->f_executable;
    for (int pc = 0; pc < buffer_size; pc++) {
        int opcode = buffer[pc].opcode;
        switch(opcode) {
            case _LOAD_CONST: {
                assert(co != NULL);
                PyObject *val = PyTuple_GET_ITEM(co->co_consts, buffer[pc].oparg);
                buffer[pc].opcode = _Py_IsImmortal(val) ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE;
                buffer[pc].operand = (uintptr_t)val;
                break;
            }
            case _CHECK_PEP_523:
            {
                /* Setting the eval frame function invalidates
                 * all executors, so no need to check dynamically */
                if (_PyInterpreterState_GET()->eval_frame == NULL) {
                    buffer[pc].opcode = _NOP;
                }
                break;
            }
            case _PUSH_FRAME:
            case _POP_FRAME:
            {
                PyFunctionObject *func = (PyFunctionObject *)buffer[pc].operand;
                if (func == NULL) {
                    co = NULL;
                }
                else {
                    assert(PyFunction_Check(func));
                    co = (PyCodeObject *)func->func_code;
                }
                break;
            }
            case _JUMP_TO_TOP:
            case _EXIT_TRACE:
                return;
        }
    }
}

//  0 - failure, no error raised, just fall back to Tier 1
// -1 - failure, and raise error
//  1 - optimizer success
int
_Py_uop_analyze_and_optimize(
    _PyInterpreterFrame *frame,
    _PyUOpInstruction *buffer,
    int buffer_size,
    int curr_stacklen,
    _PyBloomFilter *dependencies
)
{
    OPT_STAT_INC(optimizer_attempts);

    int err = remove_globals(frame, buffer, buffer_size, dependencies);
    if (err == 0) {
        goto not_ready;
    }
    if (err < 0) {
        goto error;
    }

    peephole_opt(frame, buffer, buffer_size);

    err = uop_redundancy_eliminator(
        (PyCodeObject *)frame->f_executable, buffer,
        buffer_size, curr_stacklen, dependencies);

    if (err == 0) {
        goto not_ready;
    }
    assert(err == 1);

    remove_unneeded_uops(buffer, buffer_size);

    OPT_STAT_INC(optimizer_successes);
    return 1;
not_ready:
    return 0;
error:
    return -1;
}
