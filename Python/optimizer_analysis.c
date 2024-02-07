/*
 * This file contains the optimizer for CPython uops.
 * It performs a traditional data-flow analysis[1] over the trace of uops.
 * Using the information gained, it chooses to emit, or skip certain instructions
 * if possible.
 *
 * [1] For information on data-flow analysis, please see page 27 onwards in
 * https://ilyasergey.net/CS4212/_static/lectures/PLDI-Week-12-dataflow.pdf
 * Credits to the courses UPenn Compilers (CIS 341) and NUS Compiler Design (CS4212).
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

#define TY_ARENA_SIZE (UOP_MAX_TRACE_WORKING_LENGTH * OVERALLOCATE_FACTOR)

// Need extras for root frame and for overflow frame (see TRACE_STACK_PUSH())
#define MAX_ABSTRACT_FRAME_DEPTH (TRACE_STACK_SIZE + 2)

#ifdef Py_DEBUG
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

// See the interpreter DSL in ./Tools/cases_generator/interpreter_definition.md for what these correspond to.
typedef enum {
    // Types with refinement info
    GUARD_KEYS_VERSION_TYPE = 0,
    GUARD_TYPE_VERSION_TYPE = 1,
    // You might think this actually needs to encode oparg
    // info as well, see _CHECK_FUNCTION_EXACT_ARGS.
    // However, since oparg is tied to code object is tied to function version,
    // it should be safe if function version matches.
    PYFUNCTION_TYPE_VERSION_TYPE = 2,

    // Types without refinement info
    PYLONG_TYPE = 3,
    PYFLOAT_TYPE = 4,
    PYUNICODE_TYPE = 5,
    NULL_TYPE = 6,
    PYMETHOD_TYPE = 7,
    GUARD_DORV_VALUES_TYPE = 8,
    // Can't statically determine if self or null.
    // We use this over default unknown to catch more errors.
    SELF_OR_NULL = 9,

    // Represents something from LOAD_CONST which is truly constant.
    TRUE_CONST = 30,
    INVALID_TYPE = 31,
} _Py_UOpsSymExprTypeEnum;

#define MAX_TYPE_WITH_REFINEMENT PYFUNCTION_TYPE_VERSION_TYPE

static const uint32_t IMMUTABLES =
    (
        1 << NULL_TYPE |
        1 << PYLONG_TYPE |
        1 << PYFLOAT_TYPE |
        1 << PYUNICODE_TYPE |
        1 << SELF_OR_NULL |
        1 << TRUE_CONST
    );

typedef struct {
    // bitmask of types
    uint32_t types;
    // refinement data for the types
    uint64_t refinement[MAX_TYPE_WITH_REFINEMENT + 1];
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

typedef struct frequent_syms {
    _Py_UOpsSymType *push_nulL_sym;
} frequent_syms;

typedef struct uops_emitter {
    _PyUOpInstruction *writebuffer;
    _PyUOpInstruction *writebuffer_end;
    int curr_i;
} uops_emitter;

// Tier 2 types meta interpreter
typedef struct _Py_UOpsAbstractInterpContext {
    PyObject_HEAD
    // The current "executing" frame.
    _Py_UOpsAbstractFrame *frame;
    _Py_UOpsAbstractFrame frames[MAX_ABSTRACT_FRAME_DEPTH];
    int curr_frame_depth;

    // Arena for the symbolic types.
    ty_arena t_arena;

    frequent_syms frequent_syms;

    uops_emitter emitter;

    _Py_UOpsSymType **n_consumed;
    _Py_UOpsSymType **limit;
    _Py_UOpsSymType *locals_and_stack[MAX_ABSTRACT_INTERP_SIZE];
} _Py_UOpsAbstractInterpContext;

static inline _Py_UOpsSymType*
sym_init_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val);


static inline _Py_UOpsSymType* sym_init_unknown(_Py_UOpsAbstractInterpContext *ctx);

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
        _Py_UOpsSymType *local = sym_init_unknown(ctx);
        if (local == NULL) {
            return NULL;
        }
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stackentries; i++) {
        _Py_UOpsSymType *stackvar = sym_init_unknown(ctx);
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
    int ir_entries,
    _PyUOpInstruction *new_writebuffer
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

    // IR and sym setup
    ctx->frequent_syms.push_nulL_sym = NULL;

    // Emitter setup
    ctx->emitter.writebuffer = new_writebuffer;
    ctx->emitter.curr_i = 0;
    ctx->emitter.writebuffer_end = new_writebuffer + ir_entries;
    return 0;
}


static inline bool
sym_is_type(_Py_UOpsSymType *sym, _Py_UOpsSymExprTypeEnum typ);
static inline uint64_t
sym_type_get_refinement(_Py_UOpsSymType *sym, _Py_UOpsSymExprTypeEnum typ);

static inline PyFunctionObject *
extract_func_from_sym(_Py_UOpsSymType *callable_sym)
{
    assert(callable_sym != NULL);
    if (!sym_is_type(callable_sym, PYFUNCTION_TYPE_VERSION_TYPE)) {
        DPRINTF(1, "error: _PUSH_FRAME not function type\n");
        return NULL;
    }
    uint64_t func_version = sym_type_get_refinement(callable_sym, PYFUNCTION_TYPE_VERSION_TYPE);
    PyFunctionObject *func = _PyFunction_LookupByVersion((uint32_t)func_version);
    if (func == NULL) {
        OPT_STAT_INC(optimizer_failure_reason_null_function);
        DPRINTF(1, "error: _PUSH_FRAME cannot find func version\n");
        return NULL;
    }
    return func;
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

static void
sym_set_type_from_const(_Py_UOpsSymType *sym, PyObject *obj);

// Steals a reference to const_val
static _Py_UOpsSymType*
_Py_UOpsSymType_New(_Py_UOpsAbstractInterpContext *ctx,
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
    self->types = 0;

    if (const_val != NULL) {
        self->const_val = Py_NewRef(const_val);
    }

    return self;
}


static void
sym_set_type(_Py_UOpsSymType *sym, _Py_UOpsSymExprTypeEnum typ, uint64_t refinement)
{
    sym->types |= 1 << typ;
    if (typ <= MAX_TYPE_WITH_REFINEMENT) {
        sym->refinement[typ] = refinement;
    }
}

// We need to clear the type information on every escaping/impure instruction.
// Consider the following code
/*
foo.attr
bar() # opaque call
foo.attr
*/
// We can't propagate the type information of foo.attr over across bar
// (at least, not without re-installing guards). `bar()` may call random code
// that invalidates foo's type version tag.
static void
sym_copy_immutable_type_info(_Py_UOpsSymType *from_sym, _Py_UOpsSymType *to_sym)
{
    to_sym->types = (from_sym->types & IMMUTABLES);
    if (to_sym->types) {
        Py_XSETREF(to_sym->const_val, Py_XNewRef(from_sym->const_val));
    }
}


static _Py_UOpsSymExprTypeEnum
pytype_to_type(PyTypeObject *tp)
{
    _Py_UOpsSymExprTypeEnum typ = INVALID_TYPE;
    if (tp == NULL) {
        typ = NULL_TYPE;
    }
    else if (tp == &PyLong_Type) {
        typ = PYLONG_TYPE;
    }
    else if (tp == &PyFloat_Type) {
        typ = PYFLOAT_TYPE;
    }
    else if (tp == &PyUnicode_Type) {
        typ = PYUNICODE_TYPE;
    }
    else if (tp == &PyFunction_Type) {
        typ = PYFUNCTION_TYPE_VERSION_TYPE;
    }
    else if (tp == &PyMethod_Type) {
        typ = PYMETHOD_TYPE;
    }
    return typ;
}

static void
sym_set_pytype(_Py_UOpsSymType *sym, PyTypeObject *tp, uint64_t refinement)
{
    assert(tp == NULL || PyType_Check(tp));
    sym_set_type(sym, pytype_to_type(tp), refinement);
}

static void
sym_set_type_from_const(_Py_UOpsSymType *sym, PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);

    if (tp->tp_version_tag != 0) {
        sym_set_type(sym, GUARD_TYPE_VERSION_TYPE, tp->tp_version_tag);
    }
    if (tp == &PyFunction_Type) {
        sym_set_type(sym, PYFUNCTION_TYPE_VERSION_TYPE,
                     ((PyFunctionObject *)(obj))->func_version);
    }
    else {
        sym_set_pytype(sym, tp, 0);
    }

}



static inline _Py_UOpsSymType*
sym_init_unknown(_Py_UOpsAbstractInterpContext *ctx)
{
    return _Py_UOpsSymType_New(ctx,NULL);
}

// Takes a borrowed reference to const_val.
static inline _Py_UOpsSymType*
sym_init_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val)
{
    assert(const_val != NULL);
    _Py_UOpsSymType *temp = _Py_UOpsSymType_New(
        ctx,
        const_val
    );
    if (temp == NULL) {
        return NULL;
    }
    sym_set_type_from_const(temp, const_val);
    sym_set_type(temp, TRUE_CONST, 0);
    return temp;
}

static _Py_UOpsSymType*
sym_init_null(_Py_UOpsAbstractInterpContext *ctx)
{
    if (ctx->frequent_syms.push_nulL_sym != NULL) {
        return ctx->frequent_syms.push_nulL_sym;
    }
    _Py_UOpsSymType *null_sym = sym_init_unknown(ctx);
    if (null_sym == NULL) {
        return NULL;
    }
    sym_set_type(null_sym, NULL_TYPE, 0);
    ctx->frequent_syms.push_nulL_sym = null_sym;
    return null_sym;
}

static inline bool
sym_is_type(_Py_UOpsSymType *sym, _Py_UOpsSymExprTypeEnum typ)
{
    if ((sym->types & (1 << typ)) == 0) {
        return false;
    }
    return true;
}

static inline bool
sym_matches_type(_Py_UOpsSymType *sym, _Py_UOpsSymExprTypeEnum typ, uint64_t refinement)
{
    if (!sym_is_type(sym, typ)) {
        return false;
    }
    if (typ <= MAX_TYPE_WITH_REFINEMENT) {
        return sym->refinement[typ] == refinement;
    }
    return true;
}

static inline bool
sym_matches_pytype(_Py_UOpsSymType *sym, PyTypeObject *typ, uint64_t refinement)
{
    assert(typ == NULL || PyType_Check(typ));
    return sym_matches_type(sym, pytype_to_type(typ), refinement);
}

static uint64_t
sym_type_get_refinement(_Py_UOpsSymType *sym, _Py_UOpsSymExprTypeEnum typ)
{
    assert(sym_is_type(sym, typ));
    assert(typ <= MAX_TYPE_WITH_REFINEMENT);
    return sym->refinement[typ];
}


static inline bool
op_is_end(uint32_t opcode)
{
    return opcode == _EXIT_TRACE || opcode == _JUMP_TO_TOP;
}


static inline bool
op_is_bookkeeping(uint32_t opcode) {
    return (opcode == _SET_IP ||
            opcode == _CHECK_VALIDITY ||
            opcode == _SAVE_RETURN_OFFSET ||
            opcode == _RESUME_CHECK);
}

static inline bool
op_is_data_movement_only(uint32_t opcode) {
    return (opcode == _STORE_FAST || opcode == STORE_FAST_MAYBE_NULL);
}

#ifdef Py_DEBUG
static inline bool
is_const(_Py_UOpsSymType *expr)
{
    return expr->const_val != NULL;
}
#endif

static int
clear_locals_type_info(_Py_UOpsAbstractInterpContext *ctx) {
    int locals_entries = ctx->frame->locals_len;
    for (int i = 0; i < locals_entries; i++) {
        _Py_UOpsSymType *new_local = sym_init_unknown(ctx);
        if (new_local == NULL) {
            return -1;
        }
        sym_copy_immutable_type_info(ctx->frame->locals[i], new_local);
        ctx->frame->locals[i] = new_local;
    }
    return 0;
}

static inline int
emit_i(uops_emitter *emitter,
       _PyUOpInstruction inst)
{
    if (emitter->writebuffer + emitter->curr_i >= emitter->writebuffer_end) {
        OPT_STAT_INC(optimizer_failure_reason_no_writebuffer);
        DPRINTF(1, "out of emission space\n");
        return -1;
    }
    if (inst.opcode == _NOP) {
        return 0;
    }
    DPRINTF(2, "Emitting instruction at [%d] op: %s, oparg: %d, target: %d, operand: %" PRIu64 " \n",
            emitter->curr_i,
            _PyOpcode_uop_name[inst.opcode],
            inst.oparg,
            inst.target,
            inst.operand);
    emitter->writebuffer[emitter->curr_i] = inst;
    emitter->curr_i++;
    return 0;
}

static int
uop_abstract_interpret_single_inst(
    _PyUOpInstruction *inst,
    _PyUOpInstruction *end,
    _Py_UOpsAbstractInterpContext *ctx
)
{
#define STACK_LEVEL()     ((int)(stack_pointer - ctx->frame->stack))

#define GETLOCAL(idx)          ((ctx->frame->locals[idx]))

#define REPLACE_OP(op, arg, oper)    \
    new_inst.opcode = op;            \
    new_inst.oparg = arg;            \
    new_inst.operand = oper;


    int oparg = inst->oparg;
    uint32_t opcode = inst->opcode;

    _Py_UOpsSymType **stack_pointer = ctx->frame->stack_pointer;
    _PyUOpInstruction new_inst = *inst;

    DPRINTF(3, "Abstract interpreting %s:%d ",
            _PyOpcode_uop_name[opcode],
            oparg);
    switch (opcode) {
#include "abstract_interp_cases.c.h"
        default:
            DPRINTF(1, "Unknown opcode in abstract interpreter\n");
            Py_UNREACHABLE();
    }
    assert(ctx->frame != NULL);
    DPRINTF(3, " stack_level %d\n", STACK_LEVEL());
    ctx->frame->stack_pointer = stack_pointer;
    assert(STACK_LEVEL() >= 0);

    if (emit_i(&ctx->emitter, new_inst) < 0) {
        return -1;
    }

    return 0;

error:
    DPRINTF(1, "Encountered error in abstract interpreter\n");
    return -1;

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

static int
globals_watcher_callback(PyDict_WatchEvent event, PyObject* dict,
                         PyObject* key, PyObject* new_value)
{
    if (event == PyDict_EVENT_CLONED) {
        return 0;
    }
    uint64_t watched_mutations = get_mutations(dict);
    if (watched_mutations < _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
        _Py_Executors_InvalidateDependency(_PyInterpreterState_GET(), dict);
        increment_mutations(dict);
    }
    else {
        PyDict_Unwatch(1, dict);
    }
    return 0;
}


static void
global_to_const(_PyUOpInstruction *inst, PyObject *obj)
{
    assert(inst->opcode == _LOAD_GLOBAL_MODULE || inst->opcode == _LOAD_GLOBAL_BUILTINS);
    assert(PyDict_CheckExact(obj));
    PyDictObject *dict = (PyDictObject *)obj;
    assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE);
    PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dict->ma_keys);
    assert(inst->operand <= UINT16_MAX);
    PyObject *res = entries[inst->operand].me_value;
    if (res == NULL) {
        return;
    }
    if (_Py_IsImmortal(res)) {
        inst->opcode = (inst->oparg & 1) ? _LOAD_CONST_INLINE_BORROW_WITH_NULL : _LOAD_CONST_INLINE_BORROW;
    }
    else {
        inst->opcode = (inst->oparg & 1) ? _LOAD_CONST_INLINE_WITH_NULL : _LOAD_CONST_INLINE;
    }
    inst->operand = (uint64_t)res;
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

/* The first two dict watcher IDs are reserved for CPython,
 * so we don't need to check that they haven't been used */
#define BUILTINS_WATCHER_ID 0
#define GLOBALS_WATCHER_ID  1

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
    if (interp->dict_state.watchers[1] == NULL) {
        interp->dict_state.watchers[1] = globals_watcher_callback;
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
                    global_to_const(inst, builtins);
                }
                break;
            case _LOAD_GLOBAL_MODULE:
                if (globals_checked & globals_watched & 1) {
                    global_to_const(inst, globals);
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

static int
uop_redundancy_eliminator(
    PyCodeObject *co,
    _PyUOpInstruction *trace,
    _PyUOpInstruction *new_trace,
    int trace_len,
    int curr_stacklen
)
{

    _Py_UOpsAbstractInterpContext ctx;

    if (abstractcontext_init(
        &ctx,
        co, curr_stacklen,
        trace_len, new_trace) < 0) {
        goto error;
    }
    _PyUOpInstruction *curr = NULL;
    _PyUOpInstruction *end = NULL;
    int status = 0;
    bool needs_clear_locals = true;
    int res = 0;

    curr = trace;
    end = trace + trace_len;
    needs_clear_locals = true;
    ;
    while (curr < end && !op_is_end(curr->opcode)) {

        if (!(_PyUop_Flags[curr->opcode] & HAS_PURE_FLAG) &&
            !op_is_bookkeeping(curr->opcode) &&
            !op_is_data_movement_only(curr->opcode) &&
            !(_PyUop_Flags[curr->opcode] & HAS_PASSTHROUGH_FLAG)) {
            DPRINTF(3, "Impure %s\n", _PyOpcode_uop_name[curr->opcode]);
            if (needs_clear_locals) {
                if (clear_locals_type_info(&ctx) < 0) {
                    goto error;
                }
            }
            needs_clear_locals = false;
        }
        else {
            needs_clear_locals = true;
        }

        status = uop_abstract_interpret_single_inst(
            curr, end, &ctx
        );
        if (status == -1) {
            goto error;
        }

        curr++;

    }

    assert(op_is_end(curr->opcode));

    if (emit_i(&ctx.emitter, *curr) < 0) {
        goto error;
    }

    res = ctx.emitter.curr_i;
    abstractcontext_fini(&ctx);

    return res;

error:
    abstractcontext_fini(&ctx);
    return -1;
}


static void
remove_unneeded_uops(_PyUOpInstruction *buffer, int buffer_size)
{
    int last_set_ip = -1;
    bool maybe_invalid = false;
    for (int pc = 0; pc < buffer_size; pc++) {
        int opcode = buffer[pc].opcode;
        if (opcode == _SET_IP) {
            buffer[pc].opcode = NOP;
            last_set_ip = pc;
        }
        else if (opcode == _CHECK_VALIDITY) {
            if (maybe_invalid) {
                maybe_invalid = false;
            }
            else {
                buffer[pc].opcode = NOP;
            }
        }
        else if (op_is_end(opcode)) {
            break;
        }
        else {
            if (_PyUop_Flags[opcode] & HAS_ESCAPES_FLAG) {
                maybe_invalid = true;
                if (last_set_ip >= 0) {
                    buffer[last_set_ip].opcode = _SET_IP;
                }
            }
            if ((_PyUop_Flags[opcode] & HAS_ERROR_FLAG) || opcode == _PUSH_FRAME) {
                if (last_set_ip >= 0) {
                    buffer[last_set_ip].opcode = _SET_IP;
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
    _PyUOpInstruction temp_writebuffer[UOP_MAX_TRACE_WORKING_LENGTH];

    int err = remove_globals(frame, buffer, buffer_size, dependencies);
    if (err <= 0) {
        goto error;
    }

    peephole_opt(frame, buffer, buffer_size);

    // Pass: Abstract interpretation and symbolic analysis
    int new_trace_len = uop_redundancy_eliminator(
        (PyCodeObject *)frame->f_executable, buffer, temp_writebuffer,
        buffer_size, curr_stacklen);

    if (new_trace_len < 0) {
        goto error;
    }



    remove_unneeded_uops(temp_writebuffer, new_trace_len);

    // Fill in our new trace!
    memcpy(buffer, temp_writebuffer, new_trace_len * sizeof(_PyUOpInstruction));


    OPT_STAT_INC(optimizer_successes);
    return 1;
error:

    // The only valid error we can raise is MemoryError.
    // Other times it's not really errors but things like not being able
    // to fetch a function version because the function got deleted.
    return PyErr_Occurred() ? -1 : 0;
}
