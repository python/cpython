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
#include "pycore_interp.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uop_metadata.h"
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

#define MAX_ABSTRACT_INTERP_SIZE 2048

#define OVERALLOCATE_FACTOR 3

#define PEEPHOLE_MAX_ATTEMPTS 5

#ifdef Py_DEBUG
    static const char *const DEBUG_ENV = "PYTHON_OPT_DEBUG";
    static inline int get_lltrace() {
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
    GUARD_DORV_VALUES_INST_ATTR_FROM_DICT_TYPE = 9,
    // Can't statically determine if self or null.
    SELF_OR_NULL = 10,

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
    PyObject_HEAD
    // Strong reference.
    struct _Py_UOpsAbstractFrame *prev;
    // Borrowed reference.
    struct _Py_UOpsAbstractFrame *next;
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

static void
abstractframe_dealloc(_Py_UOpsAbstractFrame *self)
{
    PyMem_Free(self->sym_consts);
    Py_XDECREF(self->prev);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyTypeObject _Py_UOpsAbstractFrame_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uops abstract frame",
    .tp_basicsize = sizeof(_Py_UOpsAbstractFrame) ,
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)abstractframe_dealloc,
    .tp_free = PyObject_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION
};

typedef struct ty_arena {
    int ty_curr_number;
    int ty_max_number;
    _Py_UOpsSymType *arena;
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

    // Arena for the symbolic types.
    ty_arena t_arena;

    frequent_syms frequent_syms;

    uops_emitter emitter;

    _Py_UOpsSymType **water_level;
    _Py_UOpsSymType **limit;
    _Py_UOpsSymType *locals_and_stack[1];
} _Py_UOpsAbstractInterpContext;

static void
abstractinterp_dealloc(PyObject *o)
{
    _Py_UOpsAbstractInterpContext *self = (_Py_UOpsAbstractInterpContext *)o;
    Py_XDECREF(self->frame);
    if (self->t_arena.arena != NULL) {
        int tys = self->t_arena.ty_curr_number;
        for (int i = 0; i < tys; i++) {
            Py_CLEAR(self->t_arena.arena[i].const_val);
        }
    }
    PyMem_Free(self->t_arena.arena);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyTypeObject _Py_UOpsAbstractInterpContext_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uops abstract interpreter's context",
    .tp_basicsize = sizeof(_Py_UOpsAbstractInterpContext) - sizeof(_Py_UOpsSymType *),
    .tp_itemsize = sizeof(_Py_UOpsSymType *),
    .tp_dealloc = (destructor)abstractinterp_dealloc,
    .tp_free = PyObject_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION
};

static inline _Py_UOpsAbstractFrame *
frame_new(_Py_UOpsAbstractInterpContext *ctx,
                          PyObject *co_consts, int stack_len, int locals_len,
                          int curr_stacklen);
static inline int
frame_push(_Py_UOpsAbstractInterpContext *ctx,
           _Py_UOpsAbstractFrame *frame,
           _Py_UOpsSymType **localsplus_start,
           int locals_len,
           int curr_stacklen,
           int total_len);

static inline int
frame_initalize(_Py_UOpsAbstractInterpContext *ctx, _Py_UOpsAbstractFrame *frame,
                int locals_len, int curr_stacklen);

static _Py_UOpsAbstractInterpContext *
abstractinterp_context_new(PyCodeObject *co,
                                  int curr_stacklen,
                                  int ir_entries,
                                  _PyUOpInstruction *new_writebuffer)
{
    int locals_len = co->co_nlocalsplus;
    int stack_len = co->co_stacksize;
    _Py_UOpsAbstractFrame *frame = NULL;
    _Py_UOpsAbstractInterpContext *self = NULL;
    char *arena = NULL;
    _Py_UOpsSymType *t_arena = NULL;
    Py_ssize_t ty_arena_size = (sizeof(_Py_UOpsSymType)) * ir_entries * OVERALLOCATE_FACTOR;

    t_arena = (_Py_UOpsSymType *)PyMem_Malloc(ty_arena_size);
    if (t_arena == NULL) {
        goto error;
    }



    self = PyObject_NewVar(_Py_UOpsAbstractInterpContext,
                               &_Py_UOpsAbstractInterpContext_Type,
                               MAX_ABSTRACT_INTERP_SIZE);
    if (self == NULL) {
        goto error;
    }

    self->limit = self->locals_and_stack + MAX_ABSTRACT_INTERP_SIZE;
    self->water_level = self->locals_and_stack;
    for (int i = 0 ; i < MAX_ABSTRACT_INTERP_SIZE; i++) {
        self->locals_and_stack[i] = NULL;
    }


    // Setup the arena for sym expressions.
    self->t_arena.ty_curr_number = 0;
    self->t_arena.arena = t_arena;
    self->t_arena.ty_max_number = ir_entries * OVERALLOCATE_FACTOR;

    // Frame setup

    frame = frame_new(self, co->co_consts, stack_len, locals_len, curr_stacklen);
    if (frame == NULL) {
        goto error;
    }
    if (frame_push(self, frame, self->water_level, locals_len, curr_stacklen,
               stack_len + locals_len) < 0) {
        goto error;
    }
    if (frame_initalize(self, frame, locals_len, curr_stacklen) < 0) {
        goto error;
    }
    self->frame = frame;
    assert(frame != NULL);

    // IR and sym setup
    self->frequent_syms.push_nulL_sym = NULL;

    // Emitter setup
    self->emitter.writebuffer = new_writebuffer;
    self->emitter.curr_i = 0;
    self->emitter.writebuffer_end = new_writebuffer + ir_entries;

    return self;

error:
    PyMem_Free(arena);
    PyMem_Free(t_arena);
    if (self != NULL) {
        // Important so we don't double free them.
        self->t_arena.arena = NULL;
        self->frame = NULL;
    }
    Py_XDECREF(self);
    Py_XDECREF(frame);
    return NULL;
}

static inline _Py_UOpsSymType*
sym_init_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val, int const_idx);

static inline _Py_UOpsSymType **
create_sym_consts(_Py_UOpsAbstractInterpContext *ctx, PyObject *co_consts)
{
    Py_ssize_t co_const_len = PyTuple_GET_SIZE(co_consts);
    _Py_UOpsSymType **sym_consts = PyMem_New(_Py_UOpsSymType *, co_const_len);
    if (sym_consts == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < co_const_len; i++) {
        _Py_UOpsSymType *res = sym_init_const(ctx, Py_NewRef(PyTuple_GET_ITEM(co_consts, i)), (int)i);
        if (res == NULL) {
            goto error;
        }
        sym_consts[i] = res;
    }

    return sym_consts;
error:
    PyMem_Free(sym_consts);
    return NULL;
}


static inline _Py_UOpsSymType*
sym_init_unknown(_Py_UOpsAbstractInterpContext *ctx);

static void
sym_copy_immutable_type_info(_Py_UOpsSymType *from_sym, _Py_UOpsSymType *to_sym);

/*
 * The reason why we have a separate frame_push and frame_initialize is to mimic
 * what CPython's frame push does. This also prepares for inlining.
 * */
static inline int
frame_push(_Py_UOpsAbstractInterpContext *ctx,
           _Py_UOpsAbstractFrame *frame,
           _Py_UOpsSymType **localsplus_start,
           int locals_len,
           int curr_stacklen,
           int total_len)
{
    frame->locals = localsplus_start;
    frame->stack = frame->locals + locals_len;
    frame->stack_pointer = frame->stack + curr_stacklen;
    ctx->water_level = localsplus_start + total_len;
    if (ctx->water_level > ctx->limit) {
        return -1;
    }
    return 0;
}

static inline int
frame_initalize(_Py_UOpsAbstractInterpContext *ctx, _Py_UOpsAbstractFrame *frame,
                int locals_len, int curr_stacklen)
{
    // Initialize with the initial state of all local variables
    for (int i = 0; i < locals_len; i++) {
        _Py_UOpsSymType *local = sym_init_unknown(ctx);
        if (local == NULL) {
            goto error;
        }
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stacklen; i++) {
        _Py_UOpsSymType *stackvar = sym_init_unknown(ctx);
        if (stackvar == NULL) {
            goto error;
        }
        frame->stack[i] = stackvar;
    }

    return 0;

error:
    return -1;
}

static inline _Py_UOpsAbstractFrame *
frame_new(_Py_UOpsAbstractInterpContext *ctx,
                          PyObject *co_consts, int stack_len, int locals_len,
                          int curr_stacklen)
{
    _Py_UOpsSymType **sym_consts = create_sym_consts(ctx, co_consts);
    if (sym_consts == NULL) {
        return NULL;
    }
    _Py_UOpsAbstractFrame *frame = PyObject_New(_Py_UOpsAbstractFrame,
                                                      &_Py_UOpsAbstractFrame_Type);
    if (frame == NULL) {
        PyMem_Free(sym_consts);
        return NULL;
    }


    frame->sym_consts = sym_consts;
    frame->sym_consts_len = (int)Py_SIZE(co_consts);
    frame->stack_len = stack_len;
    frame->locals_len = locals_len;
    frame->prev = NULL;
    frame->next = NULL;

    return frame;
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


// 0 on success, anything else is error.
static int
ctx_frame_push(
    _Py_UOpsAbstractInterpContext *ctx,
    PyCodeObject *co,
    _Py_UOpsSymType **localsplus_start
)
{
    _Py_UOpsAbstractFrame *frame = frame_new(ctx,
        co->co_consts, co->co_stacksize,
        co->co_nlocalsplus,
        0);
    if (frame == NULL) {
        return -1;
    }
    if (frame_push(ctx, frame, localsplus_start, co->co_nlocalsplus, 0,
               co->co_nlocalsplus + co->co_stacksize) < 0) {
        return -1;
    }
    if (frame_initalize(ctx, frame, co->co_nlocalsplus, 0) < 0) {
        return -1;
    }

    frame->prev = ctx->frame;
    ctx->frame->next = frame;
    ctx->frame = frame;


    return 0;
}

static int
ctx_frame_pop(
    _Py_UOpsAbstractInterpContext *ctx
)
{
    _Py_UOpsAbstractFrame *frame = ctx->frame;
    ctx->frame = frame->prev;
    assert(ctx->frame != NULL);
    frame->prev = NULL;

    ctx->water_level = frame->locals;
    Py_DECREF(frame);
    ctx->frame->next = NULL;
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

    self->const_val = NULL;
    self->types = 0;

    if (const_val != NULL) {
        sym_set_type_from_const(self, const_val);
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

// Note: for this, to_sym MUST point to brand new sym.
static void
sym_copy_immutable_type_info(_Py_UOpsSymType *from_sym, _Py_UOpsSymType *to_sym)
{
    to_sym->types = (from_sym->types & IMMUTABLES);
    if (to_sym->types) {
        to_sym->const_val = Py_XNewRef(from_sym->const_val);
    }
}

// Steals a reference to obj
static void
sym_set_type_from_const(_Py_UOpsSymType *sym, PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    sym->const_val = obj;

    if (tp == &PyLong_Type) {
        sym_set_type(sym, PYLONG_TYPE, 0);
    }
    else if (tp == &PyFloat_Type) {
        sym_set_type(sym, PYFLOAT_TYPE, 0);
    }
    else if (tp == &PyUnicode_Type) {
        sym_set_type(sym, PYUNICODE_TYPE, 0);
    }

    if (tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        PyDictOrValues *dorv = _PyObject_DictOrValuesPointer(obj);

        if (_PyDictOrValues_IsValues(*dorv) ||
            _PyObject_MakeInstanceAttributesFromDict(obj, dorv)) {
            sym_set_type(sym, GUARD_DORV_VALUES_INST_ATTR_FROM_DICT_TYPE, 0);

            PyTypeObject *owner_cls = tp;
            PyHeapTypeObject *owner_heap_type = (PyHeapTypeObject *)owner_cls;
            sym_set_type(
                sym,
                GUARD_KEYS_VERSION_TYPE,
                owner_heap_type->ht_cached_keys->dk_version
            );
        }

        if (!_PyDictOrValues_IsValues(*dorv)) {
            sym_set_type(sym, GUARD_DORV_VALUES_TYPE, 0);
        }
    }
}


static inline _Py_UOpsSymType*
sym_init_unknown(_Py_UOpsAbstractInterpContext *ctx)
{
    return _Py_UOpsSymType_New(ctx,NULL);
}

// Steals a reference to const_val
static inline _Py_UOpsSymType*
sym_init_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val, int const_idx)
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
    return opcode == _EXIT_TRACE || opcode == _JUMP_TO_TOP;
}

static inline bool
op_is_guard(uint32_t opcode)
{
    return _PyUop_Flags[opcode] & HAS_GUARD_FLAG;
}

static inline bool
op_is_pure(uint32_t opcode)
{
    return _PyUop_Flags[opcode] & HAS_PURE_FLAG;
}

static inline bool
op_is_bookkeeping(uint32_t opcode) {
    return (opcode == _SET_IP ||
            opcode == _CHECK_VALIDITY ||
            opcode == _SAVE_RETURN_OFFSET ||
            opcode == _RESUME_CHECK);
}

static inline bool
op_is_specially_handled(uint32_t opcode)
{
    return _PyUop_Flags[opcode] & HAS_SPECIAL_OPT_FLAG;
}

static inline bool
is_const(_Py_UOpsSymType *expr)
{
    return expr->const_val != NULL;
}

static inline PyObject *
get_const(_Py_UOpsSymType *expr)
{
    return expr->const_val;
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
    if (emitter->curr_i < 0) {
        DPRINTF(2, "out of emission space\n");
        return -1;
    }
    if (emitter->writebuffer + emitter->curr_i >= emitter->writebuffer_end) {
        DPRINTF(2, "out of emission space\n");
        return -1;
    }
    if (inst.opcode == _NOP) {
        return 0;
    }
    DPRINTF(2, "Emitting instruction at [%d] op: %s, oparg: %d, target: %d, operand: %" PRIu64 " \n",
            emitter->curr_i,
            (inst.opcode >= 300 ? _PyOpcode_uop_name : _PyOpcode_OpName)[inst.opcode],
            inst.oparg,
            inst.target,
            inst.operand);
    emitter->writebuffer[emitter->curr_i] = inst;
    emitter->curr_i++;
    return 0;
}

static inline int
emit_const(uops_emitter *emitter,
       PyObject *const_val,
       _PyUOpInstruction shrink_stack)
{
    if (emit_i(emitter, shrink_stack) < 0) {
        return -1;
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

typedef enum {
    ABSTRACT_INTERP_ERROR,
    ABSTRACT_INTERP_NORMAL,
} AbstractInterpExitCodes;


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
    _PyUOpInstruction shrink_stack = {_SHRINK_STACK, 0, 0, 0};


    DPRINTF(3, "Abstract interpreting %s:%d ",
            (opcode >= 300 ? _PyOpcode_uop_name : _PyOpcode_OpName)[opcode],
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
            PyObject *val = get_const(PEEK(1));
            new_inst.opcode = _Py_IsImmortal(val) ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE;
            if (new_inst.opcode == _LOAD_CONST_INLINE) {
                Py_INCREF(val);
            }
            new_inst.operand = (uintptr_t)val;
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
            // Set stack pointer to the callable.
            stack_pointer += -1 - oparg;
            break;
        }

        case _PUSH_FRAME: {
            int argcount = oparg;
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
            // Bound method fiddling, same as _INIT_CALL_PY_EXACT_ARGS
            if (!sym_is_type(self_or_null, NULL_TYPE) &&
                !sym_is_type(self_or_null, SELF_OR_NULL)) {
                args--;
                argcount++;
            }
            // This is _PUSH_FRAME's stack effect
            STACK_SHRINK(1);
            ctx->frame->stack_pointer = stack_pointer;
            if (ctx_frame_push(ctx, co, ctx->water_level) != 0){
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
        return ABSTRACT_INTERP_ERROR;
    }

    return ABSTRACT_INTERP_NORMAL;

pop_2_error_tier_two:
    STACK_SHRINK(1);
    STACK_SHRINK(1);
error:
    DPRINTF(1, "Encountered error in abstract interpreter\n");
    return ABSTRACT_INTERP_ERROR;

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

    _Py_UOpsAbstractInterpContext *ctx = NULL;

    ctx = abstractinterp_context_new(
        co, curr_stacklen,
        trace_len, new_trace);
    if (ctx == NULL) {
        goto error;
    }
    _PyUOpInstruction *curr = NULL;
    _PyUOpInstruction *end = NULL;
    AbstractInterpExitCodes status = ABSTRACT_INTERP_NORMAL;
    bool first_impure = true;
    int res = 0;

loop_peeling:
    curr = trace;
    end = trace + trace_len;
    first_impure = true;
    ;
    while (curr < end && !op_is_end(curr->opcode)) {

        if (!op_is_pure(curr->opcode) &&
            !op_is_specially_handled(curr->opcode) &&
            !op_is_bookkeeping(curr->opcode) &&
            !op_is_guard(curr->opcode)) {
            DPRINTF(3, "Impure %s\n", (curr->opcode >= 300 ? _PyOpcode_uop_name : _PyOpcode_OpName)[curr->opcode]);
            if (first_impure) {
                if (clear_locals_type_info(ctx) < 0) {
                    goto error;
                }
            }
            first_impure = false;
        }
        else {
            first_impure = true;
        }


        status = uop_abstract_interpret_single_inst(
            curr, end, ctx
        );
        if (status == ABSTRACT_INTERP_ERROR) {
            goto error;
        }

        curr++;

    }

    assert(op_is_end(curr->opcode));

    // If we end in a loop, and we have a lot of space left, peel the loop for added type stability
    // https://en.wikipedia.org/wiki/Loop_splitting
    if (!did_loop_peel && curr->opcode == _JUMP_TO_TOP &&
        ((ctx->emitter.curr_i * 3) < (int)(ctx->emitter.writebuffer_end - ctx->emitter.writebuffer))) {
        did_loop_peel = true;
        _PyUOpInstruction jump_header = {_JUMP_ABSOLUTE_HEADER, (ctx->emitter.curr_i), 0, 0};
        if (emit_i(&ctx->emitter, jump_header) < 0) {
            goto error;
        }
        DPRINTF(2, "loop_peeling!\n");
        goto loop_peeling;
    }
    else {
        if (did_loop_peel) {
            assert(curr->opcode == _JUMP_TO_TOP);
            _PyUOpInstruction jump_abs = {_JUMP_ABSOLUTE, (ctx->emitter.curr_i), 0, 0};
            if (emit_i(&ctx->emitter, jump_abs) < 0) {
                goto error;
            }
        } else {
            if (emit_i(&ctx->emitter, *curr) < 0) {
                goto error;
            }
        }
    }


    res = ctx->emitter.curr_i;
    Py_DECREF(ctx);

    return res;

error:
    Py_XDECREF(ctx);
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
        else if (opcode == _JUMP_TO_TOP || opcode == _EXIT_TRACE) {
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

static inline bool
op_is_zappable(int opcode)
{
    switch(opcode) {
        case _SET_IP:
        case _CHECK_VALIDITY:
        case _LOAD_CONST_INLINE:
        case _LOAD_CONST:
        case _LOAD_FAST:
        case _LOAD_CONST_INLINE_BORROW:
            return true;
        default:
            return false;
    }
}

static inline bool
op_is_load(int opcode)
{
    return (opcode == _LOAD_CONST_INLINE ||
        opcode == _LOAD_CONST ||
        opcode == LOAD_FAST ||
        opcode == _LOAD_CONST_INLINE_BORROW);
}

static int
peephole_optimizations(_PyUOpInstruction *buffer, int buffer_size)
{
    bool done = true;
    for (int i = 0; i < buffer_size; i++) {
        _PyUOpInstruction *curr = buffer + i;
        int oparg = curr->oparg;
        switch(curr->opcode) {
            case _SHRINK_STACK: {
                // If all that precedes a _SHRINK_STACK is a bunch of loads,
                // then we can safely eliminate that without side effects.
                int load_count = 0;
                _PyUOpInstruction *back = curr-1;
                while(op_is_zappable(back->opcode) &&
                    load_count < oparg) {
                    load_count += op_is_load(back->opcode);
                    back--;
                }
                if (load_count == oparg) {
                    done = false;
                    curr->opcode = NOP;
                    back = curr-1;
                    load_count = 0;
                    while(load_count < oparg) {
                        load_count += op_is_load(back->opcode);
                        if (back->opcode == _LOAD_CONST_INLINE) {
                            PyObject *const_val = (PyObject *)back->operand;
                            Py_CLEAR(const_val);
                        }
                        back->opcode = NOP;
                        back--;
                    }
                }
                break;
            }
            case _CHECK_PEP_523:
                /* Setting the eval frame function invalidates
                 * all executors, so no need to check dynamically */
                if (_PyInterpreterState_GET()->eval_frame == NULL) {
                    curr->opcode = _NOP;
                }
                break;
            default:
                break;
        }
    }
    return done;
}


int
_Py_uop_analyze_and_optimize(
    PyCodeObject *co,
    _PyUOpInstruction *buffer,
    int buffer_size,
    int curr_stacklen
)
{
    OPT_STAT_INC(optimizer_attempts);
    _PyUOpInstruction *temp_writebuffer = NULL;
    bool err_occurred = false;
    bool done = false;

    temp_writebuffer = PyMem_New(_PyUOpInstruction, buffer_size);
    if (temp_writebuffer == NULL) {
        goto error;
    }

    // Pass: Abstract interpretation and symbolic analysis
    int new_trace_len = uop_abstract_interpret(
        co, buffer, temp_writebuffer,
        buffer_size, curr_stacklen);

    if (new_trace_len < 0) {
        goto error;
    }

    for (int peephole_attempts = 0; peephole_attempts < PEEPHOLE_MAX_ATTEMPTS &&
        !peephole_optimizations(temp_writebuffer, new_trace_len);
         peephole_attempts++) {

    }

    remove_unneeded_uops(temp_writebuffer, new_trace_len);

    // Fill in our new trace!
    memcpy(buffer, temp_writebuffer, new_trace_len * sizeof(_PyUOpInstruction));

    PyMem_Free(temp_writebuffer);

    // _NOP out the rest of the buffer.

    // Fill up the rest of the buffer with NOPs
    _PyUOpInstruction *after = buffer + new_trace_len + 1;
    while (after < (buffer + buffer_size)) {
        after->opcode = _NOP;
        after++;
    }

    OPT_STAT_INC(optimizer_successes);
    return 0;
error:
    // The only valid error we can raise is MemoryError.
    // Other times it's not really errors but things like not being able
    // to fetch a function version because the function got deleted.
    err_occurred = PyErr_Occurred();
    PyMem_Free(temp_writebuffer);
    for (int peephole_attempts = 0; peephole_attempts < PEEPHOLE_MAX_ATTEMPTS &&
                                    !done;
         peephole_attempts++) {
        done = peephole_optimizations(buffer, buffer_size);
    }
    remove_unneeded_uops(buffer, buffer_size);
    return err_occurred ? -1 : 0;
}