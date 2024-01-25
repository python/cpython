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

#define PEEPHOLE_MAX_ATTEMPTS 10

#ifdef Py_DEBUG
    static const char *const DEBUG_ENV = "PYTHON_OPT_DEBUG";
    #define DPRINTF(level, ...) \
    if (lltrace >= (level)) { printf(__VA_ARGS__); }
#else
    #define DPRINTF(level, ...)
#endif

// This represents a value that "terminates" the symbolic.
static inline bool
op_is_terminal(uint32_t opcode)
{
    return (opcode == _LOAD_FAST ||
            opcode == _LOAD_FAST_CHECK ||
            opcode == _LOAD_FAST_AND_CLEAR ||
            opcode == INIT_FAST ||
            opcode == CACHE ||
            opcode == PUSH_NULL);
}

// This represents a value that is already on the stack.
static inline bool
op_is_stackvalue(uint32_t opcode)
{
    return (opcode == CACHE);
}

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

static const uint32_t IMMUTABLES =
    (
        1 << NULL_TYPE |
        1 << PYLONG_TYPE |
        1 << PYFLOAT_TYPE |
        1 << PYUNICODE_TYPE |
        1 << SELF_OR_NULL |
        1 << TRUE_CONST
    );

#define MAX_TYPE_WITH_REFINEMENT 2
typedef struct {
    // bitmask of types
    uint32_t types;
    // refinement data for the types
    uint64_t refinement[MAX_TYPE_WITH_REFINEMENT + 1];
    // constant propagated value (might be NULL)
    PyObject *const_val;
} _Py_UOpsSymType;


typedef struct _Py_UOpsSymbolicValue {
    // Value numbering but only for types and constant values.
    // https://en.wikipedia.org/wiki/Value_numbering
    _Py_UOpsSymType *ty_number;
    // More fields can be added later if we want to support
    // more optimizations.
} _Py_UOpsSymbolicValue;

typedef struct frame_info {
    // Only used in codegen for bookkeeping.
    struct frame_info *prev_frame_ir;
    // Localsplus of this frame.
    _Py_UOpsSymbolicValue **my_virtual_localsplus;
} frame_info;

typedef struct _Py_UOpsAbstractFrame {
    PyObject_HEAD
    // Strong reference.
    struct _Py_UOpsAbstractFrame *prev;
    // Borrowed reference.
    struct _Py_UOpsAbstractFrame *next;
    // Symbolic version of co_consts
    int sym_consts_len;
    _Py_UOpsSymbolicValue **sym_consts;
    // Max stacklen
    int stack_len;
    int locals_len;

    frame_info *frame_ir_entry;

    _Py_UOpsSymbolicValue **stack_pointer;
    _Py_UOpsSymbolicValue **stack;
    _Py_UOpsSymbolicValue **locals;
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

typedef struct creating_new_frame {
    _Py_UOpsSymbolicValue *func;
    _Py_UOpsSymbolicValue *self_or_null;
    _Py_UOpsSymbolicValue **args;
} creating_new_frame;


typedef struct frame_info_arena {
    int curr_number;
    int max_number;
    frame_info *arena;
} frame_info_arena;

typedef struct sym_arena {
    char *curr_available;
    char *end;
    char *arena;
} sym_arena;

typedef struct ty_arena {
    int ty_curr_number;
    int ty_max_number;
    _Py_UOpsSymType *arena;
} ty_arena;

typedef struct frequent_syms {
    _Py_UOpsSymbolicValue *push_nulL_sym;
} frequent_syms;

typedef struct uops_emitter {
    _PyUOpInstruction *writebuffer;
    _PyUOpInstruction *writebuffer_end;
    int curr_i;
} uops_emitter;

// Tier 2 types meta interpreter
typedef struct _Py_UOpsAbstractInterpContext {
    PyObject_HEAD
    // Stores the information for the upcoming new frame that is about to be created.
    // Corresponds to _INIT_CALL_PY_EXACT_ARGS.
    creating_new_frame new_frame_sym;
    // The current "executing" frame.
    _Py_UOpsAbstractFrame *frame;

    // An arena for the frame information.
    frame_info_arena frame_info;

    // Arena for the symbolic expression themselves.
    sym_arena s_arena;
    // Arena for the symbolic expressions' types.
    // This is separate from the s_arena so that we can free
    // all the constants easily.
    ty_arena t_arena;

    frequent_syms frequent_syms;

    uops_emitter emitter;

    _Py_UOpsSymbolicValue **water_level;
    _Py_UOpsSymbolicValue **limit;
    _Py_UOpsSymbolicValue *localsplus[1];
} _Py_UOpsAbstractInterpContext;

static void
abstractinterp_dealloc(PyObject *o)
{
    _Py_UOpsAbstractInterpContext *self = (_Py_UOpsAbstractInterpContext *)o;
    Py_XDECREF(self->frame);
    if (self->s_arena.arena != NULL) {
        int tys = self->t_arena.ty_curr_number;
        for (int i = 0; i < tys; i++) {
            Py_CLEAR(self->t_arena.arena[i].const_val);
        }
    }
    PyMem_Free(self->t_arena.arena);
    PyMem_Free(self->s_arena.arena);
    PyMem_Free(self->frame_info.arena);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyTypeObject _Py_UOpsAbstractInterpContext_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uops abstract interpreter's context",
    .tp_basicsize = sizeof(_Py_UOpsAbstractInterpContext) - sizeof(_Py_UOpsSymbolicValue *),
    .tp_itemsize = sizeof(_Py_UOpsSymbolicValue *),
    .tp_dealloc = (destructor)abstractinterp_dealloc,
    .tp_free = PyObject_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION
};

// Tags a _PUSH_FRAME with the frame info.
static frame_info *
ir_frame_push_info(_Py_UOpsAbstractInterpContext *ctx, _PyUOpInstruction *push_frame)
{
#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV(DEBUG_ENV);
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
    DPRINTF(3, "ir_frame_push_info\n");
#endif
    if (ctx->frame_info.curr_number >= ctx->frame_info.max_number) {
        DPRINTF(1, "ir_frame_push_info: ran out of space \n");
        return NULL;
    }
    frame_info *entry = &ctx->frame_info.arena[ctx->frame_info.curr_number];
    entry->my_virtual_localsplus = NULL;
    entry->prev_frame_ir = NULL;
    // root frame
    if (push_frame == NULL) {
        assert(ctx->frame_info.curr_number == 0);
        ctx->frame_info.curr_number++;
        return entry;
    }
    assert(push_frame->opcode == _PUSH_FRAME);
    push_frame->operand = (uintptr_t)entry;
    ctx->frame_info.curr_number++;
    return entry;
}

static inline _Py_UOpsAbstractFrame *
frame_new(_Py_UOpsAbstractInterpContext *ctx,
                          PyObject *co_consts, int stack_len, int locals_len,
                          int curr_stacklen, frame_info *frame_ir_entry);
static inline int
frame_push(_Py_UOpsAbstractInterpContext *ctx,
           _Py_UOpsAbstractFrame *frame,
           _Py_UOpsSymbolicValue **localsplus_start,
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
    frame_info *frame_info_arena = NULL;
    Py_ssize_t arena_size = (sizeof(_Py_UOpsSymbolicValue)) * ir_entries * OVERALLOCATE_FACTOR;
    Py_ssize_t ty_arena_size = (sizeof(_Py_UOpsSymType)) * ir_entries * OVERALLOCATE_FACTOR;
    Py_ssize_t frame_info_arena_size = (sizeof(frame_info)) * ir_entries * OVERALLOCATE_FACTOR;

    frame_info_arena = PyMem_Malloc(frame_info_arena_size);
    if (frame_info_arena == NULL) {
        goto error;
    }

    arena = (char *)PyMem_Malloc(arena_size);
    if (arena == NULL) {
        goto error;
    }

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

    // Setup frame info arena
    self->frame_info.curr_number = 0;
    self->frame_info.arena = frame_info_arena;
    self->frame_info.max_number = ir_entries * OVERALLOCATE_FACTOR;


    frame_info *root_frame = ir_frame_push_info(self, NULL);
    if (root_frame == NULL) {
        goto error;
    }

    self->limit = self->localsplus + MAX_ABSTRACT_INTERP_SIZE;
    self->water_level = self->localsplus;
    for (int i = 0 ; i < MAX_ABSTRACT_INTERP_SIZE; i++) {
        self->localsplus[i] = NULL;
    }


    // Setup the arena for sym expressions.
    self->s_arena.arena = arena;
    self->s_arena.curr_available = arena;
    assert(arena_size > 0);
    self->s_arena.end = arena + arena_size;
    self->t_arena.ty_curr_number = 0;
    self->t_arena.arena = t_arena;
    self->t_arena.ty_max_number = ir_entries * OVERALLOCATE_FACTOR;

    // Frame setup
    self->new_frame_sym.func = NULL;
    self->new_frame_sym.args = NULL;
    self->new_frame_sym.self_or_null = NULL;

    frame = frame_new(self, co->co_consts, stack_len, locals_len, curr_stacklen, root_frame);
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
    root_frame->my_virtual_localsplus = self->localsplus;

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
    PyMem_Free(frame_info_arena);
    if (self != NULL) {
        // Important so we don't double free them.
        self->t_arena.arena = NULL;
        self->s_arena.arena = NULL;
        self->frame_info.arena = NULL;
        self->frame = NULL;
    }
    Py_XDECREF(self);
    Py_XDECREF(frame);
    return NULL;
}

static inline _Py_UOpsSymbolicValue*
sym_init_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val, int const_idx);

static inline _Py_UOpsSymbolicValue **
create_sym_consts(_Py_UOpsAbstractInterpContext *ctx, PyObject *co_consts)
{
    Py_ssize_t co_const_len = PyTuple_GET_SIZE(co_consts);
    _Py_UOpsSymbolicValue **sym_consts = PyMem_New(_Py_UOpsSymbolicValue *, co_const_len);
    if (sym_consts == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < co_const_len; i++) {
        _Py_UOpsSymbolicValue *res = sym_init_const(ctx, Py_NewRef(PyTuple_GET_ITEM(co_consts, i)), (int)i);
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


static inline _Py_UOpsSymbolicValue*
sym_init_unknown(_Py_UOpsAbstractInterpContext *ctx);

static void
sym_copy_immutable_type_info(_Py_UOpsSymbolicValue *from_sym, _Py_UOpsSymbolicValue *to_sym);

/*
 * The reason why we have a separate frame_push and frame_initialize is to mimic
 * what CPython's frame push does. This also prepares for inlining.
 * */
static inline int
frame_push(_Py_UOpsAbstractInterpContext *ctx,
           _Py_UOpsAbstractFrame *frame,
           _Py_UOpsSymbolicValue **localsplus_start,
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
        _Py_UOpsSymbolicValue *local = sym_init_unknown(ctx);
        if (local == NULL) {
            goto error;
        }
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stacklen; i++) {
        _Py_UOpsSymbolicValue *stackvar = sym_init_unknown(ctx);
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
                          int curr_stacklen, frame_info *frame_ir_entry)
{
    _Py_UOpsSymbolicValue **sym_consts = create_sym_consts(ctx, co_consts);
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

    frame->frame_ir_entry = frame_ir_entry;
    return frame;
}

static inline bool
sym_is_type(_Py_UOpsSymbolicValue *sym, _Py_UOpsSymExprTypeEnum typ);
static inline uint64_t
sym_type_get_refinement(_Py_UOpsSymbolicValue *sym, _Py_UOpsSymExprTypeEnum typ);

static inline PyFunctionObject *
extract_func_from_sym(creating_new_frame *frame_sym)
{
#ifdef Py_DEBUG
char *uop_debug = Py_GETENV(DEBUG_ENV);
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
    DPRINTF(3, "extract_func_from_sym\n");
#endif
        _Py_UOpsSymbolicValue *callable_sym = frame_sym->func;
        assert(callable_sym != NULL);
        if (!sym_is_type(callable_sym, PYFUNCTION_TYPE_VERSION_TYPE)) {
            DPRINTF(1, "error: _PUSH_FRAME not function type\n");
            return NULL;
        }
        uint64_t func_version = sym_type_get_refinement(callable_sym, PYFUNCTION_TYPE_VERSION_TYPE);
        PyFunctionObject *func = _PyFunction_LookupByVersion((uint32_t)func_version);
        if (func == NULL) {
            DPRINTF(1, "error: _PUSH_FRAME cannot find func version\n");
            return NULL;
        }
        return func;
}


// 0 on success, anything else is error.
static int
ctx_frame_push(
    _Py_UOpsAbstractInterpContext *ctx,
    frame_info *frame_ir_entry,
    PyCodeObject *co,
    _Py_UOpsSymbolicValue **localsplus_start
)
{
    assert(frame_ir_entry != NULL);
    _Py_UOpsAbstractFrame *frame = frame_new(ctx,
        co->co_consts, co->co_stacksize,
        co->co_nlocalsplus,
        0, frame_ir_entry);
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

    frame_ir_entry->my_virtual_localsplus = localsplus_start;

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
sym_set_type_from_const(_Py_UOpsSymbolicValue *sym, PyObject *obj);

// Steals a reference to const_val
static _Py_UOpsSymbolicValue*
_Py_UOpsSymbolicValue_New(_Py_UOpsAbstractInterpContext *ctx,
                               PyObject *const_val)
{
#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV(DEBUG_ENV);
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
#endif

    _Py_UOpsSymbolicValue *self = (_Py_UOpsSymbolicValue *)ctx->s_arena.curr_available;
    ctx->s_arena.curr_available += sizeof(_Py_UOpsSymbolicValue) + sizeof(_Py_UOpsSymbolicValue *);
    if (ctx->s_arena.curr_available >= ctx->s_arena.end) {
        DPRINTF(1, "out of space for symbolic expression\n");
        return NULL;
    }

    _Py_UOpsSymType *ty = &ctx->t_arena.arena[ctx->t_arena.ty_curr_number];
    if (ctx->t_arena.ty_curr_number >= ctx->t_arena.ty_max_number) {
        DPRINTF(1, "out of space for symbolic expression type\n");
        return NULL;
    }
    ctx->t_arena.ty_curr_number++;
    ty->const_val = NULL;
    ty->types = 0;

    self->ty_number = ty;
    self->ty_number->types = 0;
    if (const_val != NULL) {
        sym_set_type_from_const(self, const_val);
    }

    return self;
}


static void
sym_set_type(_Py_UOpsSymbolicValue *sym, _Py_UOpsSymExprTypeEnum typ, uint64_t refinement)
{
    sym->ty_number->types |= 1 << typ;
    if (typ <= MAX_TYPE_WITH_REFINEMENT) {
        sym->ty_number->refinement[typ] = refinement;
    }
}

static void
sym_copy_type_number(_Py_UOpsSymbolicValue *from_sym, _Py_UOpsSymbolicValue *to_sym)
{
    to_sym->ty_number = from_sym->ty_number;
}

// Note: for this, to_sym MUST point to brand new sym.
static void
sym_copy_immutable_type_info(_Py_UOpsSymbolicValue *from_sym, _Py_UOpsSymbolicValue *to_sym)
{
    to_sym->ty_number->types = (from_sym->ty_number->types & IMMUTABLES);
    if (to_sym->ty_number->types) {
        to_sym->ty_number->const_val = Py_XNewRef(from_sym->ty_number->const_val);
    }
}

// Steals a reference to obj
static void
sym_set_type_from_const(_Py_UOpsSymbolicValue *sym, PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    sym->ty_number->const_val = obj;

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


static inline _Py_UOpsSymbolicValue*
sym_init_unknown(_Py_UOpsAbstractInterpContext *ctx)
{
    return _Py_UOpsSymbolicValue_New(ctx,NULL);
}

// Steals a reference to const_val
static inline _Py_UOpsSymbolicValue*
sym_init_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val, int const_idx)
{
    assert(const_val != NULL);
    _Py_UOpsSymbolicValue *temp = _Py_UOpsSymbolicValue_New(
        ctx,
        const_val
    );
    if (temp == NULL) {
        return NULL;
    }
    sym_set_type(temp, TRUE_CONST, 0);
    return temp;
}

static _Py_UOpsSymbolicValue*
sym_init_push_null(_Py_UOpsAbstractInterpContext *ctx)
{
    if (ctx->frequent_syms.push_nulL_sym != NULL) {
        return ctx->frequent_syms.push_nulL_sym;
    }
    _Py_UOpsSymbolicValue *null_sym = sym_init_unknown(ctx);
    if (null_sym == NULL) {
        return NULL;
    }
    sym_set_type(null_sym, NULL_TYPE, 0);
    ctx->frequent_syms.push_nulL_sym = null_sym;
    return null_sym;
}

static inline bool
sym_is_type(_Py_UOpsSymbolicValue *sym, _Py_UOpsSymExprTypeEnum typ)
{
    if ((sym->ty_number->types & (1 << typ)) == 0) {
        return false;
    }
    return true;
}

static inline bool
sym_matches_type(_Py_UOpsSymbolicValue *sym, _Py_UOpsSymExprTypeEnum typ, uint64_t refinement)
{
    if (!sym_is_type(sym, typ)) {
        return false;
    }
    if (typ <= MAX_TYPE_WITH_REFINEMENT) {
        return sym->ty_number->refinement[typ] == refinement;
    }
    return true;
}

static uint64_t
sym_type_get_refinement(_Py_UOpsSymbolicValue *sym, _Py_UOpsSymExprTypeEnum typ)
{
    assert(sym_is_type(sym, typ));
    assert(typ <= MAX_TYPE_WITH_REFINEMENT);
    return sym->ty_number->refinement[typ];
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
is_const(_Py_UOpsSymbolicValue *expr)
{
    return expr->ty_number->const_val != NULL;
}

static inline PyObject *
get_const(_Py_UOpsSymbolicValue *expr)
{
    return expr->ty_number->const_val;
}


static int
clear_locals_type_info(_Py_UOpsAbstractInterpContext *ctx) {
    int locals_entries = ctx->frame->locals_len;
    for (int i = 0; i < locals_entries; i++) {
        _Py_UOpsSymbolicValue *new_local = sym_init_unknown(ctx);
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
#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV(DEBUG_ENV);
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
#endif
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
#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV(DEBUG_ENV);
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
#endif
    if (emit_i(emitter, shrink_stack) < 0) {
        return -1;
    }
    int load_const_opcode = _Py_IsImmortal(const_val)
                            ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE;
    if (load_const_opcode == _LOAD_CONST_INLINE) {
        Py_INCREF(const_val);
    }
    _PyUOpInstruction load_const = {load_const_opcode, 0, 0, const_val};
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
static inline _Py_UOpsSymbolicValue *
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
#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV(DEBUG_ENV);
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
#endif

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

#define STAT_INC(opname, name) ((void)0)
#define TIER_TWO_ONLY ((void)0)

    int oparg = inst->oparg;
    uint32_t opcode = inst->opcode;
    uint64_t operand = inst->operand;

    _Py_UOpsSymbolicValue **stack_pointer = ctx->frame->stack_pointer;
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
            _Py_UOpsSymbolicValue * local = GETLOCAL(oparg);
            _Py_UOpsSymbolicValue * new_local = sym_init_unknown(ctx);
            if (new_local == NULL) {
                goto error;
            }
            sym_copy_type_number(local, new_local);
            PEEK(1) = new_local;
            break;
        }
        case LOAD_FAST: {
            STACK_GROW(1);
            _Py_UOpsSymbolicValue * local = GETLOCAL(oparg);
            // Might be NULL - replace with LOAD_FAST_CHECK
            if (sym_is_type(local, NULL_TYPE)) {
                _PyUOpInstruction temp = *inst;
                temp.opcode = LOAD_FAST_CHECK;
                _Py_UOpsSymbolicValue * new_local = sym_init_unknown(ctx);
                if (new_local == NULL) {
                    goto error;
                }
                sym_copy_type_number(local, new_local);
                PEEK(1) = new_local;
                break;
            }
            // Guaranteed by the CPython bytecode compiler to not be uninitialized.
            PEEK(1) = GETLOCAL(oparg);
            assert(PEEK(1));

            break;
        }
        case LOAD_FAST_AND_CLEAR: {
            STACK_GROW(1);
            PEEK(1) = GETLOCAL(oparg);
            ctx->frame->stack_pointer = stack_pointer;
            _Py_UOpsSymbolicValue *new_local =  sym_init_unknown(ctx);
            if (new_local == NULL) {
                goto error;
            }
            sym_set_type(new_local, NULL_TYPE, 0);
            GETLOCAL(oparg) = new_local;
            break;
        }
        case LOAD_CONST: {
            STACK_GROW(1);
            PEEK(1) = (_Py_UOpsSymbolicValue *)GETITEM(
                ctx, oparg);
            assert(PEEK(1)->ty_number->const_val != NULL);
            break;
        }
        case STORE_FAST_MAYBE_NULL:
        case STORE_FAST: {
            _Py_UOpsSymbolicValue *value = PEEK(1);
            _Py_UOpsSymbolicValue *new_local = sym_init_unknown(ctx);
            if (new_local == NULL) {
                goto error;
            }
            sym_copy_type_number(value, new_local);
            GETLOCAL(oparg) = new_local;
            STACK_SHRINK(1);
            break;
        }
        case COPY: {
            _Py_UOpsSymbolicValue *bottom = PEEK(1 + (oparg - 1));
            STACK_GROW(1);
            _Py_UOpsSymbolicValue *temp = sym_init_unknown(ctx);
            if (temp == NULL) {
                goto error;
            }
            PEEK(1) = temp;
            sym_copy_type_number(bottom, temp);
            break;
        }

        case POP_TOP: {
            STACK_SHRINK(1);
            break;
        }

        case PUSH_NULL: {
            STACK_GROW(1);
            _Py_UOpsSymbolicValue *null_sym =  sym_init_push_null(ctx);
            if (null_sym == NULL) {
                goto error;
            }
            PEEK(1) = null_sym;
            break;
        }

        case _INIT_CALL_PY_EXACT_ARGS: {
            _Py_UOpsSymbolicValue **__args_;
            _Py_UOpsSymbolicValue *__self_or_null_;
            _Py_UOpsSymbolicValue *__callable_;
            _Py_UOpsSymbolicValue *__new_frame_;
            __args_ = &stack_pointer[-oparg];
            __self_or_null_ = stack_pointer[-1 - oparg];
            __callable_ = stack_pointer[-2 - oparg];
            // Store the frame symbolic to extract information later
            assert(ctx->new_frame_sym.func == NULL);
            ctx->new_frame_sym.func = __callable_;
            ctx->new_frame_sym.self_or_null = __self_or_null_;
            ctx->new_frame_sym.args = __args_;
            __new_frame_ = _Py_UOpsSymbolicValue_New(ctx, NULL);
            if (__new_frame_ == NULL) {
                goto error;
            }
            stack_pointer[-2 - oparg] = (_Py_UOpsSymbolicValue *)__new_frame_;
            stack_pointer += -1 - oparg;
            break;
        }

        case _PUSH_FRAME: {
            int argcount = oparg;
            // TOS is the new frame.
            STACK_SHRINK(1);
            ctx->frame->stack_pointer = stack_pointer;
            frame_info *frame_ir_entry = ir_frame_push_info(ctx, inst);
            if (frame_ir_entry == NULL) {
                goto error;
            }

            PyFunctionObject *func = extract_func_from_sym(&ctx->new_frame_sym);
            if (func == NULL) {
                goto error;
            }
            PyCodeObject *co = (PyCodeObject *)func->func_code;

            _Py_UOpsSymbolicValue *self_or_null = ctx->new_frame_sym.self_or_null;
            assert(self_or_null != NULL);
            _Py_UOpsSymbolicValue **args = ctx->new_frame_sym.args;
            assert(args != NULL);
            ctx->new_frame_sym.func = NULL;
            ctx->new_frame_sym.self_or_null = NULL;
            ctx->new_frame_sym.args = NULL;
            // Bound method fiddling, same as _INIT_CALL_PY_EXACT_ARGS
            if (!sym_is_type(self_or_null, NULL_TYPE)) {
                args--;
                argcount++;
            }
            if (ctx_frame_push(
                ctx,
                frame_ir_entry,
                co,
                ctx->water_level
                ) != 0){
                goto error;
            }
            stack_pointer = ctx->frame->stack_pointer;
            // Cannot determine statically, so we can't propagate types.
            if (!sym_is_type(self_or_null, SELF_OR_NULL)) {
                for (int i = 0; i < argcount; i++) {
                    sym_copy_type_number(args[i], ctx->frame->locals[i]);
                }
            }
            break;
        }

        case _POP_FRAME: {
            assert(STACK_LEVEL() == 1);
            _Py_UOpsSymbolicValue *retval = PEEK(1);
            STACK_SHRINK(1);
            ctx->frame->stack_pointer = stack_pointer;

            if (ctx_frame_pop(ctx) != 0){
                goto error;
            }
            stack_pointer = ctx->frame->stack_pointer;
            // Push retval into new frame.
            STACK_GROW(1);
            _Py_UOpsSymbolicValue *new_retval = sym_init_unknown(ctx);
            if (new_retval == NULL) {
                goto error;
            }
            PEEK(1) = new_retval;
            sym_copy_type_number(retval, new_retval);
            break;
        }

        case SWAP: {
            _Py_UOpsSymbolicValue *top;
            _Py_UOpsSymbolicValue *bottom;
            top = stack_pointer[-1];
            bottom = stack_pointer[-2 - (oparg-2)];
            assert(oparg >= 2);

            _Py_UOpsSymbolicValue *new_top = sym_init_unknown(ctx);
            if (new_top == NULL) {
                goto error;
            }
            sym_copy_type_number(top, new_top);

            _Py_UOpsSymbolicValue *new_bottom = sym_init_unknown(ctx);
            if (new_bottom == NULL) {
                goto error;
            }
            sym_copy_type_number(bottom, new_bottom);

            stack_pointer[-2 - (oparg-2)] = new_top;
            stack_pointer[-1] = new_bottom;
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

#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV(DEBUG_ENV);
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
#endif
    // Initialize the symbolic consts

    _Py_UOpsAbstractInterpContext *ctx = NULL;

    ctx = abstractinterp_context_new(
        co, curr_stacklen,
        trace_len, new_trace);
    if (ctx == NULL) {
        goto error;
    }

    _PyUOpInstruction *curr = trace;
    _PyUOpInstruction *end = trace + trace_len;
    AbstractInterpExitCodes status = ABSTRACT_INTERP_NORMAL;

    bool first_impure = true;
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
    if (emit_i(&ctx->emitter, *curr) < 0) {
        goto error;
    }

    Py_DECREF(ctx);

    return (int)(curr - trace);

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
                // If all that precedes a _SHRINK_STACK is a bunch of LOAD_FAST,
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
    _PyUOpInstruction *temp_writebuffer = NULL;
    bool err_occurred = false;

    temp_writebuffer = PyMem_New(_PyUOpInstruction, buffer_size * OVERALLOCATE_FACTOR);
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

    return 0;
error:
    // The only valid error we can raise is MemoryError.
    // Other times it's not really errors but things like not being able
    // to fetch a function version because the function got deleted.
    err_occurred = PyErr_Occurred();
    PyMem_Free(temp_writebuffer);
    remove_unneeded_uops(buffer, buffer_size);
    return err_occurred ? -1 : 0;
}