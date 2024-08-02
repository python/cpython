#ifdef _Py_TIER2

#include "Python.h"

#include "pycore_code.h"
#include "pycore_frame.h"
#include "pycore_long.h"
#include "pycore_optimizer.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Symbols
   =======

   See the diagram at
   https://github.com/faster-cpython/ideas/blob/main/3.13/redundancy_eliminator.md

   We represent the nodes in the diagram as follows
   (the flag bits are only defined in optimizer_symbols.c):
   - Top: no flag bits, typ and const_val are NULL.
   - NULL: IS_NULL flag set, type and const_val NULL.
   - Not NULL: NOT_NULL flag set, type and const_val NULL.
   - None/not None: not used. (None could be represented as any other constant.)
   - Known type: NOT_NULL flag set and typ set; const_val is NULL.
   - Known constant: NOT_NULL flag set, type set, const_val set.
   - Bottom: IS_NULL and NOT_NULL flags set, type and const_val NULL.
 */

// Flags for below.
#define IS_NULL    1 << 0
#define NOT_NULL   1 << 1

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

static _Py_UopsSymbol *
sym_new(_Py_UOpsContext *ctx)
{
    _Py_UopsSymbol *self = &ctx->t_arena.arena[ctx->t_arena.ty_curr_number];
    if (ctx->t_arena.ty_curr_number >= ctx->t_arena.ty_max_number) {
        OPT_STAT_INC(optimizer_failure_reason_no_memory);
        DPRINTF(1, "out of space for symbolic expression type\n");
        return NULL;
    }
    ctx->t_arena.ty_curr_number++;
    self->flags = 0;
    self->typ = NULL;
    self->const_val = NULL;

    return self;
}

static inline void
sym_set_flag(_Py_UopsSymbol *sym, int flag)
{
    sym->flags |= flag;
}

static inline void
sym_set_bottom(_Py_UopsSymbol *sym)
{
    sym_set_flag(sym, IS_NULL | NOT_NULL);
    sym->typ = NULL;
    Py_CLEAR(sym->const_val);
}

bool
_Py_uop_sym_is_bottom(_Py_UopsSymbol *sym)
{
    if ((sym->flags & IS_NULL) && (sym->flags & NOT_NULL)) {
        assert(sym->flags == (IS_NULL | NOT_NULL));
        assert(sym->typ == NULL);
        assert(sym->const_val == NULL);
        return true;
    }
    return false;
}

bool
_Py_uop_sym_is_not_null(_Py_UopsSymbol *sym)
{
    return sym->flags == NOT_NULL;
}

bool
_Py_uop_sym_is_null(_Py_UopsSymbol *sym)
{
    return sym->flags == IS_NULL;
}

bool
_Py_uop_sym_is_const(_Py_UopsSymbol *sym)
{
    return sym->const_val != NULL;
}

PyObject *
_Py_uop_sym_get_const(_Py_UopsSymbol *sym)
{
    return sym->const_val;
}

bool
_Py_uop_sym_set_type(_Py_UopsSymbol *sym, PyTypeObject *typ)
{
    assert(typ != NULL && PyType_Check(typ));
    if (sym->flags & IS_NULL) {
        sym_set_bottom(sym);
        return false;
    }
    if (sym->typ != NULL) {
        if (sym->typ != typ) {
            sym_set_bottom(sym);
            return false;
        }
    }
    else {
        sym_set_flag(sym, NOT_NULL);
        sym->typ = typ;
    }
    return true;
}

bool
_Py_uop_sym_set_const(_Py_UopsSymbol *sym, PyObject *const_val)
{
    assert(const_val != NULL);
    if (sym->flags & IS_NULL) {
        sym_set_bottom(sym);
        return false;
    }
    PyTypeObject *typ = Py_TYPE(const_val);
    if (sym->typ != NULL && sym->typ != typ) {
        sym_set_bottom(sym);
        return false;
    }
    if (sym->const_val != NULL) {
        if (sym->const_val != const_val) {
            // TODO: What if they're equal?
            sym_set_bottom(sym);
            return false;
        }
    }
    else {
        sym_set_flag(sym, NOT_NULL);
        sym->typ = typ;
        sym->const_val = Py_NewRef(const_val);
    }
    return true;
}

bool
_Py_uop_sym_set_null(_Py_UopsSymbol *sym)
{
    if (_Py_uop_sym_is_not_null(sym)) {
        sym_set_bottom(sym);
        return false;
    }
    sym_set_flag(sym, IS_NULL);
    return true;
}

bool
_Py_uop_sym_set_non_null(_Py_UopsSymbol *sym)
{
    if (_Py_uop_sym_is_null(sym)) {
        sym_set_bottom(sym);
        return false;
    }
    sym_set_flag(sym, NOT_NULL);
    return true;
}


_Py_UopsSymbol *
_Py_uop_sym_new_unknown(_Py_UOpsContext *ctx)
{
    return sym_new(ctx);
}

_Py_UopsSymbol *
_Py_uop_sym_new_not_null(_Py_UOpsContext *ctx)
{
    _Py_UopsSymbol *res = _Py_uop_sym_new_unknown(ctx);
    if (res == NULL) {
        return NULL;
    }
    sym_set_flag(res, NOT_NULL);
    return res;
}

_Py_UopsSymbol *
_Py_uop_sym_new_type(_Py_UOpsContext *ctx, PyTypeObject *typ)
{
    _Py_UopsSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return NULL;
    }
    _Py_uop_sym_set_type(res, typ);
    return res;
}

// Adds a new reference to const_val, owned by the symbol.
_Py_UopsSymbol *
_Py_uop_sym_new_const(_Py_UOpsContext *ctx, PyObject *const_val)
{
    assert(const_val != NULL);
    _Py_UopsSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return NULL;
    }
    _Py_uop_sym_set_const(res, const_val);
    return res;
}

_Py_UopsSymbol *
_Py_uop_sym_new_null(_Py_UOpsContext *ctx)
{
    _Py_UopsSymbol *null_sym = _Py_uop_sym_new_unknown(ctx);
    if (null_sym == NULL) {
        return NULL;
    }
    _Py_uop_sym_set_null(null_sym);
    return null_sym;
}

PyTypeObject *
_Py_uop_sym_get_type(_Py_UopsSymbol *sym)
{
    if (_Py_uop_sym_is_bottom(sym)) {
        return NULL;
    }
    return sym->typ;
}

bool
_Py_uop_sym_has_type(_Py_UopsSymbol *sym)
{
    if (_Py_uop_sym_is_bottom(sym)) {
        return false;
    }
    return sym->typ != NULL;
}

bool
_Py_uop_sym_matches_type(_Py_UopsSymbol *sym, PyTypeObject *typ)
{
    assert(typ != NULL && PyType_Check(typ));
    return _Py_uop_sym_get_type(sym) == typ;
}

int
_Py_uop_sym_truthiness(_Py_UopsSymbol *sym)
{
    /* There are some non-constant values for
     * which `bool(val)` always evaluates to
     * True or False, such as tuples with known
     * length, but unknown contents, or bound-methods.
     * This function will need updating
     * should we support those values.
     */
    if (_Py_uop_sym_is_bottom(sym)) {
        return -1;
    }
    if (!_Py_uop_sym_is_const(sym)) {
        return -1;
    }
    PyObject *value = _Py_uop_sym_get_const(sym);
    if (value == Py_None) {
        return 0;
    }
    /* Only handle a few known safe types */
    PyTypeObject *tp = Py_TYPE(value);
    if (tp == &PyLong_Type) {
        return !_PyLong_IsZero((PyLongObject *)value);
    }
    if (tp == &PyUnicode_Type) {
        return value != &_Py_STR(empty);
    }
    if (tp == &PyBool_Type) {
        return value == Py_True;
    }
    return -1;
}

// 0 on success, -1 on error.
_Py_UOpsAbstractFrame *
_Py_uop_frame_new(
    _Py_UOpsContext *ctx,
    PyCodeObject *co,
    int curr_stackentries,
    _Py_UopsSymbol **args,
    int arg_len)
{
    assert(ctx->curr_frame_depth < MAX_ABSTRACT_FRAME_DEPTH);
    _Py_UOpsAbstractFrame *frame = &ctx->frames[ctx->curr_frame_depth];

    frame->stack_len = co->co_stacksize;
    frame->locals_len = co->co_nlocalsplus;

    frame->locals = ctx->n_consumed;
    frame->stack = frame->locals + co->co_nlocalsplus;
    frame->stack_pointer = frame->stack + curr_stackentries;
    ctx->n_consumed = ctx->n_consumed + (co->co_nlocalsplus + co->co_stacksize);
    if (ctx->n_consumed >= ctx->limit) {
        return NULL;
    }

    // Initialize with the initial state of all local variables
    for (int i = 0; i < arg_len; i++) {
        frame->locals[i] = args[i];
    }

    for (int i = arg_len; i < co->co_nlocalsplus; i++) {
        _Py_UopsSymbol *local = _Py_uop_sym_new_unknown(ctx);
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stackentries; i++) {
        _Py_UopsSymbol *stackvar = _Py_uop_sym_new_unknown(ctx);
        frame->stack[i] = stackvar;
    }

    return frame;
}

void
_Py_uop_abstractcontext_fini(_Py_UOpsContext *ctx)
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
_Py_uop_abstractcontext_init(_Py_UOpsContext *ctx)
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
_Py_uop_frame_pop(_Py_UOpsContext *ctx)
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

static _Py_UopsSymbol *
make_bottom(_Py_UOpsContext *ctx)
{
    _Py_UopsSymbol *sym = _Py_uop_sym_new_unknown(ctx);
    _Py_uop_sym_set_null(sym);
    _Py_uop_sym_set_non_null(sym);
    return sym;
}

PyObject *
_Py_uop_symbols_test(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(ignored))
{
    _Py_UOpsContext context;
    _Py_UOpsContext *ctx = &context;
    _Py_uop_abstractcontext_init(ctx);
    PyObject *val_42 = NULL;
    PyObject *val_43 = NULL;

    // Use a single 'sym' variable so copy-pasting tests is easier.
    _Py_UopsSymbol *sym = _Py_uop_sym_new_unknown(ctx);
    if (sym == NULL) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(sym), "top is NULL");
    TEST_PREDICATE(!_Py_uop_sym_is_not_null(sym), "top is not NULL");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(sym, &PyLong_Type), "top matches a type");
    TEST_PREDICATE(!_Py_uop_sym_is_const(sym), "top is a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(sym) == NULL, "top as constant is not NULL");
    TEST_PREDICATE(!_Py_uop_sym_is_bottom(sym), "top is bottom");

    sym = make_bottom(ctx);
    if (sym == NULL) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(sym), "bottom is NULL is not false");
    TEST_PREDICATE(!_Py_uop_sym_is_not_null(sym), "bottom is not NULL is not false");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(sym, &PyLong_Type), "bottom matches a type");
    TEST_PREDICATE(!_Py_uop_sym_is_const(sym), "bottom is a constant is not false");
    TEST_PREDICATE(_Py_uop_sym_get_const(sym) == NULL, "bottom as constant is not NULL");
    TEST_PREDICATE(_Py_uop_sym_is_bottom(sym), "bottom isn't bottom");

    sym = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    if (sym == NULL) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(sym), "int is NULL");
    TEST_PREDICATE(_Py_uop_sym_is_not_null(sym), "int isn't not NULL");
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyLong_Type), "int isn't int");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(sym, &PyFloat_Type), "int matches float");
    TEST_PREDICATE(!_Py_uop_sym_is_const(sym), "int is a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(sym) == NULL, "int as constant is not NULL");

    _Py_uop_sym_set_type(sym, &PyLong_Type);  // Should be a no-op
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyLong_Type), "(int and int) isn't int");

    _Py_uop_sym_set_type(sym, &PyFloat_Type);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(sym), "(int and float) isn't bottom");

    val_42 = PyLong_FromLong(42);
    assert(val_42 != NULL);
    assert(_Py_IsImmortal(val_42));

    val_43 = PyLong_FromLong(43);
    assert(val_43 != NULL);
    assert(_Py_IsImmortal(val_43));

    sym = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    if (sym == NULL) {
        goto fail;
    }
    _Py_uop_sym_set_const(sym, val_42);
    TEST_PREDICATE(_Py_uop_sym_truthiness(sym) == 1, "bool(42) is not True");
    TEST_PREDICATE(!_Py_uop_sym_is_null(sym), "42 is NULL");
    TEST_PREDICATE(_Py_uop_sym_is_not_null(sym), "42 isn't not NULL");
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyLong_Type), "42 isn't an int");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(sym, &PyFloat_Type), "42 matches float");
    TEST_PREDICATE(_Py_uop_sym_is_const(sym), "42 is not a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(sym) != NULL, "42 as constant is NULL");
    TEST_PREDICATE(_Py_uop_sym_get_const(sym) == val_42, "42 as constant isn't 42");

    _Py_uop_sym_set_type(sym, &PyLong_Type);  // Should be a no-op
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyLong_Type), "(42 and 42) isn't an int");
    TEST_PREDICATE(_Py_uop_sym_get_const(sym) == val_42, "(42 and 42) as constant isn't 42");

    _Py_uop_sym_set_type(sym, &PyFloat_Type);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(sym), "(42 and float) isn't bottom");

    sym = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    if (sym == NULL) {
        goto fail;
    }
    _Py_uop_sym_set_const(sym, val_42);
    _Py_uop_sym_set_const(sym, val_43);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(sym), "(42 and 43) isn't bottom");


    sym = _Py_uop_sym_new_const(ctx, Py_None);
    TEST_PREDICATE(_Py_uop_sym_truthiness(sym) == 0, "bool(None) is not False");
    sym = _Py_uop_sym_new_const(ctx, Py_False);
    TEST_PREDICATE(_Py_uop_sym_truthiness(sym) == 0, "bool(False) is not False");
    sym = _Py_uop_sym_new_const(ctx, PyLong_FromLong(0));
    TEST_PREDICATE(_Py_uop_sym_truthiness(sym) == 0, "bool(0) is not False");

    _Py_uop_abstractcontext_fini(ctx);
    Py_DECREF(val_42);
    Py_DECREF(val_43);
    Py_RETURN_NONE;

fail:
    _Py_uop_abstractcontext_fini(ctx);
    Py_XDECREF(val_42);
    Py_XDECREF(val_43);
    return NULL;
}

#endif /* _Py_TIER2 */
