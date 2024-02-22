
#include "Python.h"

#include "cpython/optimizer.h"
#include "pycore_code.h"
#include "pycore_frame.h"
#include "pycore_optimizer.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Flags for below.
#define KNOWN      1 << 0
#define TRUE_CONST 1 << 1
#define IS_NULL    1 << 2
#define NOT_NULL   1 << 3

#ifdef Py_DEBUG
static inline int get_lltrace(void) {
    char *uop_debug = Py_GETENV("PYTHON_OPT_DEBUG");
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

// Takes a borrowed reference to const_val, turns that into a strong reference.
static _Py_UOpsSymType*
sym_new(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val)
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

bool
_Py_uop_sym_is_not_null(_Py_UOpsSymType *sym)
{
    return (sym->flags & (IS_NULL | NOT_NULL)) == NOT_NULL;
}

bool
_Py_uop_sym_is_null(_Py_UOpsSymType *sym)
{
    return (sym->flags & (IS_NULL | NOT_NULL)) == IS_NULL;
}

bool
_Py_uop_sym_is_const(_Py_UOpsSymType *sym)
{
    return (sym->flags & TRUE_CONST) != 0;
}

PyObject *
_Py_uop_sym_get_const(_Py_UOpsSymType *sym)
{
    assert(_Py_uop_sym_is_const(sym));
    assert(sym->const_val);
    return sym->const_val;
}

void
_Py_uop_sym_set_type(_Py_UOpsSymType *sym, PyTypeObject *tp)
{
    assert(PyType_Check(tp));
    sym->typ = tp;
    sym_set_flag(sym, KNOWN);
    sym_set_flag(sym, NOT_NULL);
}

void
_Py_uop_sym_set_null(_Py_UOpsSymType *sym)
{
    sym_set_flag(sym, IS_NULL);
    sym_set_flag(sym, KNOWN);
}


_Py_UOpsSymType *
_Py_uop_sym_new_unknown(_Py_UOpsAbstractInterpContext *ctx)
{
    return sym_new(ctx,NULL);
}

_Py_UOpsSymType *
_Py_uop_sym_new_not_null(_Py_UOpsAbstractInterpContext *ctx)
{
    _Py_UOpsSymType *res = _Py_uop_sym_new_unknown(ctx);
    if (res == NULL) {
        return NULL;
    }
    sym_set_flag(res, KNOWN);
    sym_set_flag(res, NOT_NULL);
    return res;
}

_Py_UOpsSymType *
_Py_uop_sym_new_type(_Py_UOpsAbstractInterpContext *ctx,
                      PyTypeObject *typ)
{
    _Py_UOpsSymType *res = sym_new(ctx,NULL);
    if (res == NULL) {
        return NULL;
    }
    _Py_uop_sym_set_type(res, typ);
    return res;
}

// Takes a borrowed reference to const_val.
_Py_UOpsSymType*
_Py_uop_sym_new_const(_Py_UOpsAbstractInterpContext *ctx, PyObject *const_val)
{
    assert(const_val != NULL);
    _Py_UOpsSymType *temp = sym_new(
        ctx,
        const_val
    );
    if (temp == NULL) {
        return NULL;
    }
    _Py_uop_sym_set_type(temp, Py_TYPE(const_val));
    sym_set_flag(temp, TRUE_CONST);
    sym_set_flag(temp, KNOWN);
    sym_set_flag(temp, NOT_NULL);
    return temp;
}

_Py_UOpsSymType*
_Py_uop_sym_new_null(_Py_UOpsAbstractInterpContext *ctx)
{
    _Py_UOpsSymType *null_sym = _Py_uop_sym_new_unknown(ctx);
    if (null_sym == NULL) {
        return NULL;
    }
    _Py_uop_sym_set_null(null_sym);
    return null_sym;
}

bool
_Py_uop_sym_matches_type(_Py_UOpsSymType *sym, PyTypeObject *typ)
{
    assert(typ == NULL || PyType_Check(typ));
    if (!sym_has_flag(sym, KNOWN)) {
        return false;
    }
    return sym->typ == typ;
}

// 0 on success, -1 on error.
_Py_UOpsAbstractFrame *
_Py_uop_ctx_frame_new(
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
        _Py_UOpsSymType *local = _Py_uop_sym_new_unknown(ctx);
        if (local == NULL) {
            return NULL;
        }
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stackentries; i++) {
        _Py_UOpsSymType *stackvar = _Py_uop_sym_new_unknown(ctx);
        if (stackvar == NULL) {
            return NULL;
        }
        frame->stack[i] = stackvar;
    }

    return frame;
}

void
_Py_uop_abstractcontext_fini(_Py_UOpsAbstractInterpContext *ctx)
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

int
_Py_uop_abstractcontext_init(
    _Py_UOpsAbstractInterpContext *ctx
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

    return 0;
}

int
_Py_uop_ctx_frame_pop(
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

#define TEST_PREDICATE(PRED, MSG) \
do { \
    if (!(PRED)) { \
        PyErr_SetString( \
            PyExc_AssertionError, \
            (MSG)); \
        goto fail; \
    } \
} while (0)

/*
static _Py_UOpsSymType *
make_bottom(_Py_UOpsAbstractInterpContext *ctx)
{
    _Py_UOpsSymType *sym = _Py_uop_sym_new_unknown(ctx);
    _Py_uop_sym_set_null(sym);
    _Py_uop_sym_set_type(sym, &PyLong_Type);
    return sym;
}*/

PyObject *
_Py_uop_symbols_test(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(ignored))
{
    _Py_UOpsAbstractInterpContext context;
    _Py_UOpsAbstractInterpContext *ctx = &context;
    _Py_uop_abstractcontext_init(ctx);

    _Py_UOpsSymType *top = _Py_uop_sym_new_unknown(ctx);
    if (top == NULL) {
        return NULL;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(top), "unknown is NULL");
    TEST_PREDICATE(!_Py_uop_sym_is_not_null(top), "unknown is not NULL");
    TEST_PREDICATE(!_Py_uop_sym_is_const(top), "unknown is a constant");
    // TEST_PREDICATE(_Py_uop_sym_get_const(top) == NULL, "unknown as constant is not NULL");

    // _Py_UOpsSymType *bottom = make_bottom(ctx);
    // TEST_PREDICATE(_Py_uop_sym_is_null(bottom), "bottom is NULL is not true");
    // TEST_PREDICATE(_Py_uop_sym_is_not_null(bottom), "bottom is not NULL is not true");
    // TEST_PREDICATE(_Py_uop_sym_is_const(bottom), "bottom is a constant is not true");

    _Py_UOpsSymType *int_type = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    TEST_PREDICATE(_Py_uop_sym_matches_type(int_type, &PyLong_Type), "inconsistent type");
    _Py_uop_sym_set_type(int_type, &PyLong_Type);
    TEST_PREDICATE(_Py_uop_sym_matches_type(int_type, &PyLong_Type), "inconsistent type");
    _Py_uop_sym_set_type(int_type, &PyFloat_Type);
    // TEST_PREDICATE(_Py_uop_sym_matches_type(int_type, &PyLong_Type), "bottom doesn't match int");

    _Py_uop_abstractcontext_fini(ctx);
    Py_RETURN_NONE;
fail:
    _Py_uop_abstractcontext_fini(ctx);
    return NULL;
}
