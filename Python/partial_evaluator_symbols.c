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

   Documentation TODO gh-120619
 */

// Flags for below.
#define IS_NULL    1 << 0
#define NOT_NULL   1 << 1
#define NO_SPACE   1 << 2

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

static _Py_UopsPESymbol NO_SPACE_SYMBOL = {
    .flags = IS_NULL | NOT_NULL | NO_SPACE,
    .const_val = NULL,
};

static _Py_UopsPESlot
out_of_space(_Py_UOpsPEContext *ctx)
{
    ctx->done = true;
    ctx->out_of_space = true;
    return (_Py_UopsPESlot){&NO_SPACE_SYMBOL, NULL};
}

static _Py_UopsPESlot
sym_new(_Py_UOpsPEContext *ctx)
{
    _Py_UopsPESymbol *self = &ctx->sym_arena.arena[ctx->sym_arena.sym_curr_number];
    if (ctx->sym_arena.sym_curr_number >= ctx->sym_arena.sym_max_number) {
        OPT_STAT_INC(optimizer_failure_reason_no_memory);
        DPRINTF(1, "out of space for symbolic expression type\n");
        return (_Py_UopsPESlot){NULL, NULL};
    }
    ctx->sym_arena.sym_curr_number++;
    self->flags = 0;
    self->const_val = NULL;

    return (_Py_UopsPESlot){self, NULL};
}

static inline void
sym_set_flag(_Py_UopsPESlot *sym, int flag)
{
    sym->sym->flags |= flag;
}

static inline void
sym_set_bottom(_Py_UOpsPEContext *ctx, _Py_UopsPESlot *sym)
{
    sym_set_flag(sym, IS_NULL | NOT_NULL);
    Py_CLEAR(sym->sym->const_val);
    ctx->done = true;
    ctx->contradiction = true;
}

bool
_Py_uop_pe_sym_is_bottom(_Py_UopsPESlot *sym)
{
    if ((sym->sym->flags & IS_NULL) && (sym->sym->flags & NOT_NULL)) {
        assert(sym->sym->flags == (IS_NULL | NOT_NULL));
        assert(sym->sym->const_val == NULL);
        return true;
    }
    return false;
}

bool
_Py_uop_pe_sym_is_not_null(_Py_UopsPESlot *sym)
{
    return sym->sym->flags == NOT_NULL;
}

bool
_Py_uop_pe_sym_is_null(_Py_UopsPESlot *sym)
{
    return sym->sym->flags == IS_NULL;
}

bool
_Py_uop_pe_sym_is_const(_Py_UopsPESlot *sym)
{
    return sym->sym->const_val != NULL;
}

PyObject *
_Py_uop_pe_sym_get_const(_Py_UopsPESlot *sym)
{
    return sym->sym->const_val;
}

void
_Py_uop_pe_sym_set_const(_Py_UOpsPEContext *ctx, _Py_UopsPESlot *sym, PyObject *const_val)
{
    assert(const_val != NULL);
    if (sym->sym->flags & IS_NULL) {
        sym_set_bottom(ctx, sym);
    }
    if (sym->sym->const_val != NULL) {
        if (sym->sym->const_val != const_val) {
            // TODO: What if they're equal?
            sym_set_bottom(ctx, sym);
        }
    }
    else {
        sym_set_flag(sym, NOT_NULL);
        sym->sym->const_val = Py_NewRef(const_val);
    }
}

void
_Py_uop_pe_sym_set_null(_Py_UOpsPEContext *ctx, _Py_UopsPESlot *sym)
{
    if (_Py_uop_pe_sym_is_not_null(sym)) {
        sym_set_bottom(ctx, sym);
    }
    sym_set_flag(sym, IS_NULL);
}

void
_Py_uop_pe_sym_set_non_null(_Py_UOpsPEContext *ctx, _Py_UopsPESlot *sym)
{
    if (_Py_uop_pe_sym_is_null(sym)) {
        sym_set_bottom(ctx, sym);
    }
    sym_set_flag(sym, NOT_NULL);
}


_Py_UopsPESlot
_Py_uop_pe_sym_new_unknown(_Py_UOpsPEContext *ctx)
{
    return sym_new(ctx);
}

_Py_UopsPESlot
_Py_uop_pe_sym_new_not_null(_Py_UOpsPEContext *ctx)
{
    _Py_UopsPESlot res = _Py_uop_pe_sym_new_unknown(ctx);
    if (res.sym == NULL) {
        return out_of_space(ctx);
    }
    sym_set_flag(&res, NOT_NULL);
    return res;
}

// Adds a new reference to const_val, owned by the symbol.
_Py_UopsPESlot
_Py_uop_pe_sym_new_const(_Py_UOpsPEContext *ctx, PyObject *const_val)
{
    assert(const_val != NULL);
    _Py_UopsPESlot res = sym_new(ctx);
    if (res.sym == NULL) {
        return out_of_space(ctx);
    }
    _Py_uop_pe_sym_set_const(ctx, &res, const_val);
    return res;
}

_Py_UopsPESlot
_Py_uop_pe_sym_new_null(_Py_UOpsPEContext *ctx)
{
    _Py_UopsPESlot null_sym = _Py_uop_pe_sym_new_unknown(ctx);
    if (null_sym.sym == NULL) {
        return out_of_space(ctx);
    }
    _Py_uop_pe_sym_set_null(ctx, &null_sym);
    return null_sym;
}

int
_Py_uop_pe_sym_truthiness(_Py_UopsPESlot *sym)
{
    /* There are some non-constant values for
     * which `bool(val)` always evaluates to
     * True or False, such as tuples with known
     * length, but unknown contents, or bound-methods.
     * This function will need updating
     * should we support those values.
     */
    if (_Py_uop_pe_sym_is_bottom(sym)) {
        return -1;
    }
    if (!_Py_uop_pe_sym_is_const(sym)) {
        return -1;
    }
    PyObject *value = _Py_uop_pe_sym_get_const(sym);
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

void
_Py_uop_sym_set_origin_inst_override(_Py_UopsPESlot *sym, _PyUOpInstruction *origin)
{
    sym->origin_inst = origin;
}

_PyUOpInstruction *
_Py_uop_sym_get_origin(_Py_UopsPESlot *sym)
{
    return sym->origin_inst;
}

bool
_Py_uop_sym_is_virtual(_Py_UopsPESlot *sym)
{
    if (!sym->origin_inst) {
        return false;
    }
    else {
        return sym->origin_inst->is_virtual;
    }
}

// 0 on success, -1 on error.
_Py_UOpsPEAbstractFrame *
_Py_uop_pe_frame_new(
    _Py_UOpsPEContext *ctx,
    PyCodeObject *co,
    int curr_stackentries,
    _Py_UopsPESlot *args,
    int arg_len)
{
    assert(ctx->curr_frame_depth < MAX_ABSTRACT_FRAME_DEPTH);
    _Py_UOpsPEAbstractFrame *frame = &ctx->frames[ctx->curr_frame_depth];

    frame->stack_len = co->co_stacksize;
    frame->locals_len = co->co_nlocalsplus;

    frame->locals = ctx->n_consumed;
    frame->stack = frame->locals + co->co_nlocalsplus;
    frame->stack_pointer = frame->stack + curr_stackentries;
    ctx->n_consumed = ctx->n_consumed + (co->co_nlocalsplus + co->co_stacksize);
    if (ctx->n_consumed >= ctx->limit) {
        ctx->done = true;
        ctx->out_of_space = true;
        return NULL;
    }

    // Initialize with the initial state of all local variables
    for (int i = 0; i < arg_len; i++) {
        frame->locals[i] = args[i];
    }

    for (int i = arg_len; i < co->co_nlocalsplus; i++) {
        _Py_UopsPESlot local = _Py_uop_pe_sym_new_unknown(ctx);
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stackentries; i++) {
        _Py_UopsPESlot stackvar = _Py_uop_pe_sym_new_unknown(ctx);
        frame->stack[i] = stackvar;
    }

    return frame;
}

void
_Py_uop_pe_abstractcontext_fini(_Py_UOpsPEContext *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->curr_frame_depth = 0;
    int tys = ctx->sym_arena.sym_curr_number;
    for (int i = 0; i < tys; i++) {
        Py_CLEAR(ctx->sym_arena.arena[i].const_val);
    }
}

void
_Py_uop_pe_abstractcontext_init(_Py_UOpsPEContext *ctx)
{
    ctx->limit = ctx->locals_and_stack + MAX_ABSTRACT_INTERP_SIZE;
    ctx->n_consumed = ctx->locals_and_stack;
#ifdef Py_DEBUG // Aids debugging a little. There should never be NULL in the abstract interpreter.
    for (int i = 0 ; i < MAX_ABSTRACT_INTERP_SIZE; i++) {
        ctx->locals_and_stack[i] = (_Py_UopsPESlot){NULL, NULL};
    }
#endif

    // Setup the arena for sym expressions.
    ctx->sym_arena.sym_curr_number = 0;
    ctx->sym_arena.sym_max_number = TY_ARENA_SIZE;

    // Frame setup
    ctx->curr_frame_depth = 0;
}

int
_Py_uop_pe_frame_pop(_Py_UOpsPEContext *ctx)
{
    _Py_UOpsPEAbstractFrame *frame = ctx->frame;
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

static _Py_UopsPESlot
make_bottom(_Py_UOpsPEContext *ctx)
{
    _Py_UopsPESlot sym = _Py_uop_pe_sym_new_unknown(ctx);
    _Py_uop_pe_sym_set_null(ctx, &sym);
    _Py_uop_pe_sym_set_non_null(ctx, &sym);
    return sym;
}

PyObject *
_Py_uop_pe_symbols_test(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(ignored))
{
    _Py_UOpsPEContext context;
    _Py_UOpsPEContext *ctx = &context;
    _Py_uop_pe_abstractcontext_init(ctx);
    PyObject *val_42 = NULL;
    PyObject *val_43 = NULL;

    // Use a single 'sym' variable so copy-pasting tests is easier.
    _Py_UopsPESlot sym = _Py_uop_pe_sym_new_unknown(ctx);
    if (sym.sym == NULL) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_pe_sym_is_null(&sym), "top is NULL");
    TEST_PREDICATE(!_Py_uop_pe_sym_is_not_null(&sym), "top is not NULL");
    TEST_PREDICATE(!_Py_uop_pe_sym_is_const(&sym), "top is a constant");
    TEST_PREDICATE(_Py_uop_pe_sym_get_const(&sym) == NULL, "top as constant is not NULL");
    TEST_PREDICATE(!_Py_uop_pe_sym_is_bottom(&sym), "top is bottom");

    sym = make_bottom(ctx);
    if (sym.sym == NULL) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_pe_sym_is_null(&sym), "bottom is NULL is not false");
    TEST_PREDICATE(!_Py_uop_pe_sym_is_not_null(&sym), "bottom is not NULL is not false");
    TEST_PREDICATE(!_Py_uop_pe_sym_is_const(&sym), "bottom is a constant is not false");
    TEST_PREDICATE(_Py_uop_pe_sym_get_const(&sym) == NULL, "bottom as constant is not NULL");
    TEST_PREDICATE(_Py_uop_pe_sym_is_bottom(&sym), "bottom isn't bottom");

    sym = _Py_uop_pe_sym_new_const(ctx, Py_None);
    TEST_PREDICATE(_Py_uop_pe_sym_truthiness(&sym) == 0, "bool(None) is not False");
    sym = _Py_uop_pe_sym_new_const(ctx, Py_False);
    TEST_PREDICATE(_Py_uop_pe_sym_truthiness(&sym) == 0, "bool(False) is not False");
    sym = _Py_uop_pe_sym_new_const(ctx, PyLong_FromLong(0));
    TEST_PREDICATE(_Py_uop_pe_sym_truthiness(&sym) == 0, "bool(0) is not False");

    _Py_uop_pe_abstractcontext_fini(ctx);
    Py_DECREF(val_42);
    Py_DECREF(val_43);
    Py_RETURN_NONE;

fail:
    _Py_uop_pe_abstractcontext_fini(ctx);
    Py_XDECREF(val_42);
    Py_XDECREF(val_43);
    return NULL;
}

#endif /* _Py_TIER2 */
