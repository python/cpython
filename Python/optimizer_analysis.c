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
    // Symbolic version of co_consts
    int sym_consts_len;
    _Py_UOpsSymType **sym_consts;
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

    _Py_UOpsSymType **water_level;
    _Py_UOpsSymType **limit;
    _Py_UOpsSymType *locals_and_stack[MAX_ABSTRACT_INTERP_SIZE];
} _Py_UOpsAbstractInterpContext;

static inline _Py_UOpsSymType*
sym_init_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val);

static inline _Py_UOpsSymType **
create_sym_consts(_Py_UOpsAbstractInterpContext *ctx, PyObject *co_consts)
{
    Py_ssize_t co_const_len = PyTuple_GET_SIZE(co_consts);
    _Py_UOpsSymType **sym_consts = ctx->limit - co_const_len;
    ctx->limit -= co_const_len;
    if (ctx->limit <= ctx->water_level) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < co_const_len; i++) {
        _Py_UOpsSymType *res = sym_init_const(ctx, PyTuple_GET_ITEM(co_consts, i));
        if (res == NULL) {
            return NULL;
        }
        sym_consts[i] = res;
    }

    return sym_consts;
}

static inline _Py_UOpsSymType* sym_init_unknown(_Py_UOpsAbstractInterpContext *ctx);

// 0 on success, anything else is error.
static int
ctx_frame_push(
    _Py_UOpsAbstractInterpContext *ctx,
    PyCodeObject *co,
    _Py_UOpsSymType **localsplus_start,
    int curr_stackentries
)
{
    _Py_UOpsSymType **sym_consts = create_sym_consts(ctx, co->co_consts);
    if (sym_consts == NULL) {
        return -1;
    }
    assert(ctx->curr_frame_depth < MAX_ABSTRACT_FRAME_DEPTH);
    _Py_UOpsAbstractFrame *frame = &ctx->frames[ctx->curr_frame_depth];
    ctx->curr_frame_depth++;

    frame->sym_consts = sym_consts;
    frame->sym_consts_len = (int)Py_SIZE(co->co_consts);
    frame->stack_len = co->co_stacksize;
    frame->locals_len = co->co_nlocalsplus;

    frame->locals = localsplus_start;
    frame->stack = frame->locals + co->co_nlocalsplus;
    frame->stack_pointer = frame->stack + curr_stackentries;
    ctx->water_level = localsplus_start + (co->co_nlocalsplus + co->co_stacksize);
    if (ctx->water_level >= ctx->limit) {
        return -1;
    }


    // Initialize with the initial state of all local variables
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        _Py_UOpsSymType *local = sym_init_unknown(ctx);
        if (local == NULL) {
            return -1;
        }
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stackentries; i++) {
        _Py_UOpsSymType *stackvar = sym_init_unknown(ctx);
        if (stackvar == NULL) {
            return -1;
        }
        frame->stack[i] = stackvar;
    }

    ctx->frame = frame;
    return 0;
}

static void
abstractinterp_fini(_Py_UOpsAbstractInterpContext *self)
{
    if (self == NULL) {
        return;
    }
     self->curr_frame_depth = 0;
    int tys = self->t_arena.ty_curr_number;
    for (int i = 0; i < tys; i++) {
        Py_CLEAR(self->t_arena.arena[i].const_val);
    }
}

static int
abstractinterp_init(
    _Py_UOpsAbstractInterpContext *self,
    PyCodeObject *co,
    int curr_stacklen,
    int ir_entries,
    _PyUOpInstruction *new_writebuffer
)
{
    self->limit = self->locals_and_stack + MAX_ABSTRACT_INTERP_SIZE;
    self->water_level = self->locals_and_stack;
#ifdef Py_DEBUG // Aids debugging a little. There should never be NULL in the abstract interpreter.
    for (int i = 0 ; i < MAX_ABSTRACT_INTERP_SIZE; i++) {
        self->locals_and_stack[i] = NULL;
    }
#endif

    // Setup the arena for sym expressions.
    self->t_arena.ty_curr_number = 0;
    self->t_arena.ty_max_number = TY_ARENA_SIZE;

    // Frame setup
    self->curr_frame_depth = 0;
    if (ctx_frame_push(self, co, self->water_level, curr_stacklen) < 0) {
        return -1;
    }

    // IR and sym setup
    self->frequent_syms.push_nulL_sym = NULL;

    // Emitter setup
    self->emitter.writebuffer = new_writebuffer;
    self->emitter.curr_i = 0;
    self->emitter.writebuffer_end = new_writebuffer + ir_entries;
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

    ctx->water_level = frame->locals;
    ctx->curr_frame_depth--;
    assert(ctx->curr_frame_depth >= 1);
    ctx->frame = &ctx->frames[ctx->curr_frame_depth - 1];

    ctx->limit += frame->sym_consts_len;
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
        Py_INCREF(const_val);
        sym_set_type_from_const(self, const_val);
        self->const_val = const_val;
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

static void
sym_copy_immutable_type_info(_Py_UOpsSymType *from_sym, _Py_UOpsSymType *to_sym)
{
    to_sym->types = (from_sym->types & IMMUTABLES);
    if (to_sym->types) {
        Py_XSETREF(to_sym->const_val, Py_XNewRef(from_sym->const_val));
    }
}

static void
sym_set_type_from_const(_Py_UOpsSymType *sym, PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);

    if (tp->tp_version_tag != 0) {
        sym_set_type(sym, GUARD_TYPE_VERSION_TYPE, tp->tp_version_tag);
    }
    if (tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(obj);
        if(_PyDictOrValues_IsValues(dorv)) {
            sym_set_type(sym, GUARD_DORV_VALUES_TYPE, 0);
        }
    }
    if (tp == &PyLong_Type) {
        sym_set_type(sym, PYLONG_TYPE, 0);
    }
    else if (tp == &PyFloat_Type) {
        sym_set_type(sym, PYFLOAT_TYPE, 0);
    }
    else if (tp == &PyUnicode_Type) {
        sym_set_type(sym, PYUNICODE_TYPE, 0);
    }
    else if (tp == &PyFunction_Type) {
        sym_set_type(sym, PYFUNCTION_TYPE_VERSION_TYPE,
                     ((PyFunctionObject *)(obj))->func_version);
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
    sym_set_type(temp, TRUE_CONST, 0);
    return temp;
}

static _Py_UOpsSymType*
sym_init_push_null(_Py_UOpsAbstractInterpContext *ctx)
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
    return opcode == _EXIT_TRACE || opcode == _JUMP_TO_TOP ||
        opcode == _JUMP_ABSOLUTE;
}


static inline bool
op_is_bookkeeping(uint32_t opcode) {
    return (opcode == _SET_IP ||
            opcode == _CHECK_VALIDITY ||
            opcode == _SAVE_RETURN_OFFSET ||
            opcode == _RESUME_CHECK);
}


static inline bool
is_const(_Py_UOpsSymType *expr)
{
    return expr->const_val != NULL;
}

static inline PyObject *
get_const_borrow(_Py_UOpsSymType *expr)
{
    return expr->const_val;
}

static inline PyObject *
get_const(_Py_UOpsSymType *expr)
{
    return Py_NewRef(expr->const_val);
}


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

static inline bool
op_is_zappable(int opcode)
{
    switch(opcode) {
        case _SET_IP:
        case _CHECK_VALIDITY:
            return true;
        default:
            return _PyUop_Flags[opcode] & HAS_PURE_FLAG;
    }
}

static inline bool
op_count_loads(int opcode)
{
    switch(opcode) {
        case _LOAD_CONST_INLINE:
        case _LOAD_CONST:
        case _LOAD_FAST:
        case _LOAD_CONST_INLINE_BORROW:
            return 1;
        case _LOAD_CONST_INLINE_WITH_NULL:
        case _LOAD_CONST_INLINE_BORROW_WITH_NULL:
            return 2;
        default:
            return 0;
    }
}

static inline int
emit_const(uops_emitter *emitter,
       PyObject *const_val,
       int num_pops)
{
    _PyUOpInstruction shrink_stack = {_SHRINK_STACK, num_pops, 0, 0};
    // If all that precedes a _SHRINK_STACK is a bunch of loads,
    // then we can safely eliminate that without side effects.
    int load_count = 0;
    _PyUOpInstruction *back = emitter->writebuffer + emitter->curr_i - 1;
    while (back >= emitter->writebuffer &&
           load_count < num_pops &&
           op_is_zappable(back->opcode)) {
        load_count += op_count_loads(back->opcode);
        back--;
    }
    if (load_count == num_pops) {
        back = emitter->writebuffer + emitter->curr_i - 1;
        load_count = 0;
        // Back up over the previous loads and zap them.
        while(load_count < num_pops) {
            load_count += op_count_loads(back->opcode);
            if (back->opcode == _LOAD_CONST_INLINE ||
                back->opcode == _LOAD_CONST_INLINE_WITH_NULL) {
                PyObject *old_const_val = (PyObject *)back->operand;
                Py_DECREF(old_const_val);
                back->operand = (uintptr_t)NULL;
            }
            back->opcode = NOP;
            back--;
        }
    }
    else {
        if (emit_i(emitter, shrink_stack) < 0) {
            return -1;
        }
    }
    int load_const_opcode = _Py_IsImmortal(const_val)
                            ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE;
    if (load_const_opcode == _LOAD_CONST_INLINE) {
        Py_INCREF(const_val);
    }
    _PyUOpInstruction load_const = {load_const_opcode, 0, 0, (uintptr_t)const_val};
    if (emit_i(emitter, load_const) < 0) {
        return -1;
    }
    return 0;
}


#define DECREF_INPUTS_AND_REUSE_FLOAT(left, right, dval, result) \
do { \
    { \
        result = PyFloat_FromDouble(dval); \
        if ((result) == NULL) goto error; \
    } \
} while (0)

#define DEOPT_IF(COND, INSTNAME) \
    if ((COND)) {                \
        goto guard_required;         \
    }

#ifndef Py_DEBUG
#define GETITEM(ctx, i) (ctx->frame->sym_consts[(i)])
#else
static inline _Py_UOpsSymType *
GETITEM(_Py_UOpsAbstractInterpContext *ctx, Py_ssize_t i) {
    assert(i < ctx->frame->sym_consts_len);
    return ctx->frame->sym_consts[i];
}
#endif

static int
uop_abstract_interpret_single_inst(
    _PyUOpInstruction *inst,
    _PyUOpInstruction *end,
    _Py_UOpsAbstractInterpContext *ctx
)
{
#define STACK_LEVEL()     ((int)(stack_pointer - ctx->frame->stack))
#define STACK_SIZE()      (ctx->frame->stack_len)
#define BASIC_STACKADJ(n) (stack_pointer += n)

#ifdef Py_DEBUG
    #define STACK_GROW(n)   do { \
                                assert(n >= 0); \
                                BASIC_STACKADJ(n); \
                                if (STACK_LEVEL() > STACK_SIZE()) { \
                                    DPRINTF(2, "err: %d, %d\n", STACK_SIZE(), STACK_LEVEL())\
                                } \
                                assert(STACK_LEVEL() <= STACK_SIZE()); \
                            } while (0)
    #define STACK_SHRINK(n) do { \
                                assert(n >= 0); \
                                assert(STACK_LEVEL() >= n); \
                                BASIC_STACKADJ(-(n)); \
                            } while (0)
#else
    #define STACK_GROW(n)          BASIC_STACKADJ(n)
    #define STACK_SHRINK(n)        BASIC_STACKADJ(-(n))
#endif
#define PEEK(idx)              (((stack_pointer)[-(idx)]))
#define GETLOCAL(idx)          ((ctx->frame->locals[idx]))

#define CURRENT_OPARG() (oparg)

#define CURRENT_OPERAND() (operand)

#define TIER_TWO_ONLY ((void)0)

    int oparg = inst->oparg;
    uint32_t opcode = inst->opcode;
    uint64_t operand = inst->operand;

    _Py_UOpsSymType **stack_pointer = ctx->frame->stack_pointer;
    _PyUOpInstruction new_inst = *inst;

    DPRINTF(3, "Abstract interpreting %s:%d ",
            _PyOpcode_uop_name[opcode],
            oparg);
    switch (opcode) {
#include "abstract_interp_cases.c.h"
        // Note: LOAD_FAST_CHECK is not pure!!!
        case LOAD_FAST_CHECK: {
            STACK_GROW(1);
            _Py_UOpsSymType *local = GETLOCAL(oparg);
            // We guarantee this will error - just bail and don't optimize it.
            if (sym_is_type(local, NULL_TYPE)) {
                goto error;
            }
            PEEK(1) = local;
            break;
        }
        case LOAD_FAST: {
            STACK_GROW(1);
            _Py_UOpsSymType * local = GETLOCAL(oparg);
            if (sym_is_type(local, NULL_TYPE)) {
                Py_UNREACHABLE();
            }
            // Guaranteed by the CPython bytecode compiler to not be uninitialized.
            PEEK(1) = GETLOCAL(oparg);
            assert(PEEK(1));

            break;
        }
        case LOAD_FAST_AND_CLEAR: {
            STACK_GROW(1);
            PEEK(1) = GETLOCAL(oparg);
            break;
        }
        case LOAD_CONST: {
            STACK_GROW(1);
            PEEK(1) = (_Py_UOpsSymType *)GETITEM(
                ctx, oparg);
            assert(is_const(PEEK(1)));
            // Peephole: inline constants.
            PyObject *val = get_const_borrow(PEEK(1));
            new_inst.opcode = _Py_IsImmortal(val) ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE;
            if (new_inst.opcode == _LOAD_CONST_INLINE) {
                Py_INCREF(val);
            }
            new_inst.operand = (uintptr_t)val;
            break;
        }
        case _LOAD_CONST_INLINE:
        case _LOAD_CONST_INLINE_BORROW:
        {
            _Py_UOpsSymType *sym_const = sym_init_const(ctx, (PyObject *)inst->operand);
            if (sym_const == NULL) {
                goto error;
            }
            // We need to incref it for it to safely decref in the
            // executor finalizer.
            if (opcode == _LOAD_CONST_INLINE) {
                Py_INCREF(inst->operand);
            }
            STACK_GROW(1);
            PEEK(1) = sym_const;
            assert(is_const(PEEK(1)));
            break;
        }
        case _LOAD_CONST_INLINE_WITH_NULL:
        case _LOAD_CONST_INLINE_BORROW_WITH_NULL:
        {
            _Py_UOpsSymType *sym_const = sym_init_const(ctx, (PyObject *)inst->operand);
            if (sym_const == NULL) {
                goto error;
            }
            // We need to incref it for it to safely decref in the
            // executor finalizer.
            if (opcode == _LOAD_CONST_INLINE_WITH_NULL) {
                Py_INCREF(inst->operand);
            }
            STACK_GROW(1);
            PEEK(1) = sym_const;
            assert(is_const(PEEK(1)));
            _Py_UOpsSymType *null_sym =  sym_init_push_null(ctx);
            if (null_sym == NULL) {
                goto error;
            }
            STACK_GROW(1);
            PEEK(1) = null_sym;
            break;
        }
        case STORE_FAST_MAYBE_NULL:
        case STORE_FAST: {
            _Py_UOpsSymType *value = PEEK(1);
            GETLOCAL(oparg) = value;
            STACK_SHRINK(1);
            break;
        }
        case COPY: {
            _Py_UOpsSymType *bottom = PEEK(1 + (oparg - 1));
            STACK_GROW(1);
            PEEK(1) = bottom;
            break;
        }

        case PUSH_NULL: {
            STACK_GROW(1);
            _Py_UOpsSymType *null_sym =  sym_init_push_null(ctx);
            if (null_sym == NULL) {
                goto error;
            }
            PEEK(1) = null_sym;
            break;
        }

        case _INIT_CALL_PY_EXACT_ARGS: {
            // Don't put in the new frame. Leave it be so that _PUSH_FRAME
            // can extract callable, self_or_null and args later.
            // This also means our stack pointer diverges from the real VM.

            // IMPORTANT: make sure there is no interference
            // between this and _PUSH_FRAME. That is a required invariant.
            break;
        }

        case _PUSH_FRAME: {
            // From _INIT_CALL_PY_EXACT_ARGS

            int argcount = oparg;
            // _INIT_CALL_PY_EXACT_ARGS's real stack effect in the VM.
            stack_pointer += -1 - oparg;
            // TOS is the new callable, above it self_or_null and args

            PyFunctionObject *func = extract_func_from_sym(PEEK(1));
            if (func == NULL) {
                goto error;
            }
            PyCodeObject *co = (PyCodeObject *)func->func_code;

            _Py_UOpsSymType *self_or_null = PEEK(0);
            assert(self_or_null != NULL);
            _Py_UOpsSymType **args = &PEEK(-1);
            assert(args != NULL);
            if (!sym_is_type(self_or_null, NULL_TYPE) &&
                !sym_is_type(self_or_null, SELF_OR_NULL)) {
                // Bound method fiddling, same as _INIT_CALL_PY_EXACT_ARGS in
                // VM
                args--;
                argcount++;
            }
            // This is _PUSH_FRAME's stack effect
            STACK_SHRINK(1);
            ctx->frame->stack_pointer = stack_pointer;
            if (ctx_frame_push(ctx, co, ctx->water_level, 0) != 0){
                goto error;
            }
            stack_pointer = ctx->frame->stack_pointer;
            // Cannot determine statically, so we can't propagate types.
            if (!sym_is_type(self_or_null, SELF_OR_NULL)) {
                for (int i = 0; i < argcount; i++) {
                    ctx->frame->locals[i] = args[i];
                }
            }
            break;
        }

        case _POP_FRAME: {
            assert(STACK_LEVEL() == 1);
            _Py_UOpsSymType *retval = PEEK(1);
            STACK_SHRINK(1);
            ctx->frame->stack_pointer = stack_pointer;

            if (ctx_frame_pop(ctx) != 0){
                goto error;
            }
            stack_pointer = ctx->frame->stack_pointer;
            // Push retval into new frame.
            STACK_GROW(1);
            PEEK(1) = retval;
            break;
        }

        case _CHECK_PEP_523:
            /* Setting the eval frame function invalidates
             * all executors, so no need to check dynamically */
            if (_PyInterpreterState_GET()->eval_frame == NULL) {
                new_inst.opcode = _NOP;
            }
            break;
        case _CHECK_GLOBALS:
        case _CHECK_BUILTINS:
        case _SET_IP:
        case _CHECK_VALIDITY:
        case _SAVE_RETURN_OFFSET:
            break;
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

pop_2_error_tier_two:
    STACK_SHRINK(1);
    STACK_SHRINK(1);
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
uop_abstract_interpret(
    PyCodeObject *co,
    _PyUOpInstruction *trace,
    _PyUOpInstruction *new_trace,
    int trace_len,
    int curr_stacklen
)
{
    bool did_loop_peel = false;

    _Py_UOpsAbstractInterpContext ctx;

    if (abstractinterp_init(
        &ctx,
        co, curr_stacklen,
        trace_len, new_trace) < 0) {
        goto error;
    }
    _PyUOpInstruction *curr = NULL;
    _PyUOpInstruction *end = NULL;
    int status = 0;
    bool needs_clear_locals = true;
    bool has_enough_space_to_duplicate_loop = true;
    int res = 0;

loop_peeling:
    curr = trace;
    end = trace + trace_len;
    needs_clear_locals = true;
    ;
    while (curr < end && !op_is_end(curr->opcode)) {

        if (!(_PyUop_Flags[curr->opcode] & HAS_PURE_FLAG) &&
            !(_PyUop_Flags[curr->opcode] & HAS_SPECIAL_OPT_FLAG) &&
            !op_is_bookkeeping(curr->opcode) &&
            !(_PyUop_Flags[curr->opcode] & HAS_GUARD_FLAG)) {
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

    // If we end in a loop, and we have a lot of space left, peel the loop for
    // poor man's loop invariant code motion for guards
    // https://en.wikipedia.org/wiki/Loop_splitting
    has_enough_space_to_duplicate_loop = ((ctx.emitter.curr_i * 3) <
        (int)(ctx.emitter.writebuffer_end - ctx.emitter.writebuffer));
    if (!did_loop_peel && curr->opcode == _JUMP_TO_TOP && has_enough_space_to_duplicate_loop) {
        OPT_STAT_INC(loop_body_duplication_attempts);
        did_loop_peel = true;
        _PyUOpInstruction jump_header = {_JUMP_ABSOLUTE_HEADER, (ctx.emitter.curr_i), 0, 0};
        if (emit_i(&ctx.emitter, jump_header) < 0) {
            goto error;
        }
        DPRINTF(1, "loop_peeling!\n");
        goto loop_peeling;
    }
    else {
#if  defined(Py_STATS) || defined(Py_DEBUG)
        if(!did_loop_peel && curr->opcode == _JUMP_TO_TOP && !has_enough_space_to_duplicate_loop)  {
            OPT_STAT_INC(loop_body_duplication_no_mem);
            DPRINTF(1, "no space for loop peeling\n");
        }
#endif
        if (did_loop_peel) {
            OPT_STAT_INC(loop_body_duplication_successes);
            assert(curr->opcode == _JUMP_TO_TOP);
            _PyUOpInstruction jump_abs = {_JUMP_ABSOLUTE, (ctx.emitter.curr_i), 0, 0};
            if (emit_i(&ctx.emitter, jump_abs) < 0) {
                goto error;
            }
        } else {
            if (emit_i(&ctx.emitter, *curr) < 0) {
                goto error;
            }
        }
    }


    res = ctx.emitter.curr_i;
    abstractinterp_fini(&ctx);

    return res;

error:
    abstractinterp_fini(&ctx);
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

    // Pass: Abstract interpretation and symbolic analysis
    int new_trace_len = uop_abstract_interpret(
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
