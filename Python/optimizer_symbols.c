#ifdef _Py_TIER2

#include "Python.h"

#include "pycore_code.h"
#include "pycore_frame.h"
#include "pycore_long.h"
#include "pycore_optimizer.h"
#include "pycore_stats.h"
#include "pycore_tuple.h"         // _PyTuple_FromArray()

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


static JitOptSymbol NO_SPACE_SYMBOL = {
    .tag = JIT_SYM_BOTTOM_TAG
};

static JitOptSymbol *
allocation_base(JitOptContext *ctx)
{
    return ctx->t_arena.arena;
}

JitOptSymbol *
out_of_space(JitOptContext *ctx)
{
    ctx->done = true;
    ctx->out_of_space = true;
    return &NO_SPACE_SYMBOL;
}

static JitOptSymbol *
sym_new(JitOptContext *ctx)
{
    JitOptSymbol *self = &ctx->t_arena.arena[ctx->t_arena.ty_curr_number];
    if (ctx->t_arena.ty_curr_number >= ctx->t_arena.ty_max_number) {
        OPT_STAT_INC(optimizer_failure_reason_no_memory);
        DPRINTF(1, "out of space for symbolic expression type\n");
        return NULL;
    }
    ctx->t_arena.ty_curr_number++;
    self->tag = JIT_SYM_UNKNOWN_TAG;
    return self;
}

static void make_const(JitOptSymbol *sym, PyObject *val)
{
    sym->tag = JIT_SYM_KNOWN_VALUE_TAG;
    sym->value.value = Py_NewRef(val);
}

static inline void
sym_set_bottom(JitOptContext *ctx, JitOptSymbol *sym)
{
    sym->tag = JIT_SYM_BOTTOM_TAG;
    ctx->done = true;
    ctx->contradiction = true;
}

bool
_Py_uop_sym_is_bottom(JitOptSymbol *sym)
{
    return sym->tag == JIT_SYM_BOTTOM_TAG;
}

bool
_Py_uop_sym_is_not_null(JitOptSymbol *sym) {
    return sym->tag == JIT_SYM_NON_NULL_TAG || sym->tag > JIT_SYM_BOTTOM_TAG;
}

bool
_Py_uop_sym_is_const(JitOptContext *ctx, JitOptSymbol *sym)
{
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        return true;
    }
    if (sym->tag == JIT_SYM_TRUTHINESS_TAG) {
        JitOptSymbol *value = allocation_base(ctx) + sym->truthiness.value;
        int truthiness = _Py_uop_sym_truthiness(ctx, value);
        if (truthiness < 0) {
            return false;
        }
        make_const(sym, (truthiness ^ sym->truthiness.invert) ? Py_True : Py_False);
        return true;
    }
    return false;
}

bool
_Py_uop_sym_is_null(JitOptSymbol *sym)
{
    return sym->tag == JIT_SYM_NULL_TAG;
}


PyObject *
_Py_uop_sym_get_const(JitOptContext *ctx, JitOptSymbol *sym)
{
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        return sym->value.value;
    }
    if (sym->tag == JIT_SYM_TRUTHINESS_TAG) {
        JitOptSymbol *value = allocation_base(ctx) + sym->truthiness.value;
        int truthiness = _Py_uop_sym_truthiness(ctx, value);
        if (truthiness < 0) {
            return NULL;
        }
        PyObject *res = (truthiness ^ sym->truthiness.invert) ? Py_True : Py_False;
        make_const(sym, res);
        return res;
    }
    return NULL;
}

void
_Py_uop_sym_set_type(JitOptContext *ctx, JitOptSymbol *sym, PyTypeObject *typ)
{
    JitSymType tag = sym->tag;
    switch(tag) {
        case JIT_SYM_NULL_TAG:
            sym_set_bottom(ctx, sym);
            return;
        case JIT_SYM_KNOWN_CLASS_TAG:
            if (sym->cls.type != typ) {
                sym_set_bottom(ctx, sym);
            }
            return;
        case JIT_SYM_TYPE_VERSION_TAG:
            if (sym->version.version == typ->tp_version_tag) {
                sym->tag = JIT_SYM_KNOWN_CLASS_TAG;
                sym->cls.type = typ;
                sym->cls.version = typ->tp_version_tag;
            }
            else {
                sym_set_bottom(ctx, sym);
            }
            return;
        case JIT_SYM_KNOWN_VALUE_TAG:
            if (Py_TYPE(sym->value.value) != typ) {
                Py_CLEAR(sym->value.value);
                sym_set_bottom(ctx, sym);
            }
            return;
        case JIT_SYM_TUPLE_TAG:
            if (typ != &PyTuple_Type) {
                sym_set_bottom(ctx, sym);
            }
            return;
        case JIT_SYM_BOTTOM_TAG:
            return;
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            sym->tag = JIT_SYM_KNOWN_CLASS_TAG;
            sym->cls.version = 0;
            sym->cls.type = typ;
            return;
        case JIT_SYM_TRUTHINESS_TAG:
            if (typ != &PyBool_Type) {
                sym_set_bottom(ctx, sym);
            }
            return;
    }
}

bool
_Py_uop_sym_set_type_version(JitOptContext *ctx, JitOptSymbol *sym, unsigned int version)
{
    PyTypeObject *type = _PyType_LookupByVersion(version);
    if (type) {
        _Py_uop_sym_set_type(ctx, sym, type);
    }
    JitSymType tag = sym->tag;
    switch(tag) {
        case JIT_SYM_NULL_TAG:
            sym_set_bottom(ctx, sym);
            return false;
        case JIT_SYM_KNOWN_CLASS_TAG:
            if (sym->cls.type->tp_version_tag != version) {
                sym_set_bottom(ctx, sym);
                return false;
            }
            else {
                sym->cls.version = version;
                return true;
            }
        case JIT_SYM_KNOWN_VALUE_TAG:
            if (Py_TYPE(sym->value.value)->tp_version_tag != version) {
                Py_CLEAR(sym->value.value);
                sym_set_bottom(ctx, sym);
                return false;
            };
            return true;
        case JIT_SYM_TUPLE_TAG:
            if (PyTuple_Type.tp_version_tag != version) {
                sym_set_bottom(ctx, sym);
                return false;
            };
            return true;
        case JIT_SYM_TYPE_VERSION_TAG:
            if (sym->version.version != version) {
                sym_set_bottom(ctx, sym);
                return false;
            }
            return true;
        case JIT_SYM_BOTTOM_TAG:
            return false;
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            sym->tag = JIT_SYM_TYPE_VERSION_TAG;
            sym->version.version = version;
            return true;
        case JIT_SYM_TRUTHINESS_TAG:
            if (version != PyBool_Type.tp_version_tag) {
                sym_set_bottom(ctx, sym);
                return false;
            }
            return true;
    }
    Py_UNREACHABLE();
}

void
_Py_uop_sym_set_const(JitOptContext *ctx, JitOptSymbol *sym, PyObject *const_val)
{
    JitSymType tag = sym->tag;
    switch(tag) {
        case JIT_SYM_NULL_TAG:
            sym_set_bottom(ctx, sym);
            return;
        case JIT_SYM_KNOWN_CLASS_TAG:
            if (sym->cls.type != Py_TYPE(const_val)) {
                sym_set_bottom(ctx, sym);
                return;
            }
            make_const(sym, const_val);
            return;
        case JIT_SYM_KNOWN_VALUE_TAG:
            if (sym->value.value != const_val) {
                Py_CLEAR(sym->value.value);
                sym_set_bottom(ctx, sym);
            }
            return;
        case JIT_SYM_TUPLE_TAG:
            if (PyTuple_CheckExact(const_val)) {
                Py_ssize_t len = _Py_uop_sym_tuple_length(sym);
                if (len == PyTuple_GET_SIZE(const_val)) {
                    for (Py_ssize_t i = 0; i < len; i++) {
                        JitOptSymbol *sym_item = _Py_uop_sym_tuple_getitem(ctx, sym, i);
                        PyObject *item = PyTuple_GET_ITEM(const_val, i);
                        _Py_uop_sym_set_const(ctx, sym_item, item);
                    }
                    make_const(sym, const_val);
                    return;
                }
            }
            sym_set_bottom(ctx, sym);
            return;
        case JIT_SYM_TYPE_VERSION_TAG:
            if (sym->version.version != Py_TYPE(const_val)->tp_version_tag) {
                sym_set_bottom(ctx, sym);
                return;
            }
            make_const(sym, const_val);
            return;
        case JIT_SYM_BOTTOM_TAG:
            return;
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            make_const(sym, const_val);
            return;
        case JIT_SYM_TRUTHINESS_TAG:
            if (!PyBool_Check(const_val) ||
                (_Py_uop_sym_is_const(ctx, sym) &&
                 _Py_uop_sym_get_const(ctx, sym) != const_val))
            {
                sym_set_bottom(ctx, sym);
                return;
            }
            JitOptSymbol *value = allocation_base(ctx) + sym->truthiness.value;
            PyTypeObject *type = _Py_uop_sym_get_type(value);
            if (const_val == (sym->truthiness.invert ? Py_False : Py_True)) {
                // value is truthy. This is only useful for bool:
                if (type == &PyBool_Type) {
                    _Py_uop_sym_set_const(ctx, value, Py_True);
                }
            }
            // value is falsey:
            else if (type == &PyBool_Type) {
                _Py_uop_sym_set_const(ctx, value, Py_False);
            }
            else if (type == &PyLong_Type) {
                _Py_uop_sym_set_const(ctx, value, Py_GetConstant(Py_CONSTANT_ZERO));
            }
            else if (type == &PyUnicode_Type) {
                _Py_uop_sym_set_const(ctx, value, Py_GetConstant(Py_CONSTANT_EMPTY_STR));
            }
            // TODO: More types (GH-130415)!
            make_const(sym, const_val);
            return;
    }
}

void
_Py_uop_sym_set_null(JitOptContext *ctx, JitOptSymbol *sym)
{
    if (sym->tag == JIT_SYM_UNKNOWN_TAG) {
        sym->tag = JIT_SYM_NULL_TAG;
    }
    else if (sym->tag > JIT_SYM_NULL_TAG) {
        sym_set_bottom(ctx, sym);
    }
}

void
_Py_uop_sym_set_non_null(JitOptContext *ctx, JitOptSymbol *sym)
{
    if (sym->tag == JIT_SYM_UNKNOWN_TAG) {
        sym->tag = JIT_SYM_NON_NULL_TAG;
    }
    else if (sym->tag == JIT_SYM_NULL_TAG) {
        sym_set_bottom(ctx, sym);
    }
}


JitOptSymbol *
_Py_uop_sym_new_unknown(JitOptContext *ctx)
{
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space(ctx);
    }
    return res;
}

JitOptSymbol *
_Py_uop_sym_new_not_null(JitOptContext *ctx)
{
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space(ctx);
    }
    res->tag = JIT_SYM_NON_NULL_TAG;
    return res;
}

JitOptSymbol *
_Py_uop_sym_new_type(JitOptContext *ctx, PyTypeObject *typ)
{
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space(ctx);
    }
    _Py_uop_sym_set_type(ctx, res, typ);
    return res;
}

// Adds a new reference to const_val, owned by the symbol.
JitOptSymbol *
_Py_uop_sym_new_const(JitOptContext *ctx, PyObject *const_val)
{
    assert(const_val != NULL);
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space(ctx);
    }
    _Py_uop_sym_set_const(ctx, res, const_val);
    return res;
}

JitOptSymbol *
_Py_uop_sym_new_null(JitOptContext *ctx)
{
    JitOptSymbol *null_sym = sym_new(ctx);
    if (null_sym == NULL) {
        return out_of_space(ctx);
    }
    _Py_uop_sym_set_null(ctx, null_sym);
    return null_sym;
}

PyTypeObject *
_Py_uop_sym_get_type(JitOptSymbol *sym)
{
    JitSymType tag = sym->tag;
    switch(tag) {
        case JIT_SYM_NULL_TAG:
        case JIT_SYM_TYPE_VERSION_TAG:
        case JIT_SYM_BOTTOM_TAG:
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            return NULL;
        case JIT_SYM_KNOWN_CLASS_TAG:
            return sym->cls.type;
        case JIT_SYM_KNOWN_VALUE_TAG:
            return Py_TYPE(sym->value.value);
        case JIT_SYM_TUPLE_TAG:
            return &PyTuple_Type;
        case JIT_SYM_TRUTHINESS_TAG:
            return &PyBool_Type;
    }
    Py_UNREACHABLE();
}

unsigned int
_Py_uop_sym_get_type_version(JitOptSymbol *sym)
{
    JitSymType tag = sym->tag;
    switch(tag) {
        case JIT_SYM_NULL_TAG:
        case JIT_SYM_BOTTOM_TAG:
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            return 0;
        case JIT_SYM_TYPE_VERSION_TAG:
            return sym->version.version;
        case JIT_SYM_KNOWN_CLASS_TAG:
            return sym->cls.version;
        case JIT_SYM_KNOWN_VALUE_TAG:
            return Py_TYPE(sym->value.value)->tp_version_tag;
        case JIT_SYM_TUPLE_TAG:
            return PyTuple_Type.tp_version_tag;
        case JIT_SYM_TRUTHINESS_TAG:
            return PyBool_Type.tp_version_tag;
    }
    Py_UNREACHABLE();
}

bool
_Py_uop_sym_has_type(JitOptSymbol *sym)
{
    JitSymType tag = sym->tag;
    switch(tag) {
        case JIT_SYM_NULL_TAG:
        case JIT_SYM_TYPE_VERSION_TAG:
        case JIT_SYM_BOTTOM_TAG:
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            return false;
        case JIT_SYM_KNOWN_CLASS_TAG:
        case JIT_SYM_KNOWN_VALUE_TAG:
        case JIT_SYM_TUPLE_TAG:
        case JIT_SYM_TRUTHINESS_TAG:
            return true;
    }
    Py_UNREACHABLE();
}

bool
_Py_uop_sym_matches_type(JitOptSymbol *sym, PyTypeObject *typ)
{
    assert(typ != NULL && PyType_Check(typ));
    return _Py_uop_sym_get_type(sym) == typ;
}

bool
_Py_uop_sym_matches_type_version(JitOptSymbol *sym, unsigned int version)
{
    return _Py_uop_sym_get_type_version(sym) == version;
}

int
_Py_uop_sym_truthiness(JitOptContext *ctx, JitOptSymbol *sym)
{
    switch(sym->tag) {
        case JIT_SYM_NULL_TAG:
        case JIT_SYM_TYPE_VERSION_TAG:
        case JIT_SYM_BOTTOM_TAG:
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            return -1;
        case JIT_SYM_KNOWN_CLASS_TAG:
            /* TODO :
             * Instances of some classes are always
             * true. We should return 1 in those cases */
            return -1;
        case JIT_SYM_KNOWN_VALUE_TAG:
            break;
        case JIT_SYM_TUPLE_TAG:
            return sym->tuple.length != 0;
        case JIT_SYM_TRUTHINESS_TAG:
            ;
            JitOptSymbol *value = allocation_base(ctx) + sym->truthiness.value;
            int truthiness = _Py_uop_sym_truthiness(ctx, value);
            if (truthiness < 0) {
                return truthiness;
            }
            truthiness ^= sym->truthiness.invert;
            make_const(sym, truthiness ? Py_True : Py_False);
            return truthiness;
    }
    PyObject *value = sym->value.value;
    /* Only handle a few known safe types */
    if (value == Py_None) {
        return 0;
    }
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

JitOptSymbol *
_Py_uop_sym_new_tuple(JitOptContext *ctx, int size, JitOptSymbol **args)
{
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space(ctx);
    }
    if (size > MAX_SYMBOLIC_TUPLE_SIZE) {
        res->tag = JIT_SYM_KNOWN_CLASS_TAG;
        res->cls.type = &PyTuple_Type;
    }
    else {
        res->tag = JIT_SYM_TUPLE_TAG;
        res->tuple.length = size;
        for (int i = 0; i < size; i++) {
            res->tuple.items[i] = (uint16_t)(args[i] - allocation_base(ctx));
        }
    }
    return res;
}

JitOptSymbol *
_Py_uop_sym_tuple_getitem(JitOptContext *ctx, JitOptSymbol *sym, int item)
{
    assert(item >= 0);
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        PyObject *tuple = sym->value.value;
        if (PyTuple_CheckExact(tuple) && item < PyTuple_GET_SIZE(tuple)) {
            return _Py_uop_sym_new_const(ctx, PyTuple_GET_ITEM(tuple, item));
        }
    }
    else if (sym->tag == JIT_SYM_TUPLE_TAG && item < sym->tuple.length) {
        return allocation_base(ctx) + sym->tuple.items[item];
    }
    return _Py_uop_sym_new_unknown(ctx);
}

int
_Py_uop_sym_tuple_length(JitOptSymbol *sym)
{
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        PyObject *tuple = sym->value.value;
        if (PyTuple_CheckExact(tuple)) {
            return PyTuple_GET_SIZE(tuple);
        }
    }
    else if (sym->tag == JIT_SYM_TUPLE_TAG) {
        return sym->tuple.length;
    }
    return -1;
}

// Return true if known to be immortal.
bool
_Py_uop_sym_is_immortal(JitOptSymbol *sym)
{
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        return _Py_IsImmortal(sym->value.value);
    }
    if (sym->tag == JIT_SYM_KNOWN_CLASS_TAG) {
        return sym->cls.type == &PyBool_Type;
    }
    if (sym->tag == JIT_SYM_TRUTHINESS_TAG) {
        return true;
    }
    return false;
}

JitOptSymbol *
_Py_uop_sym_new_truthiness(JitOptContext *ctx, JitOptSymbol *value, bool truthy)
{
    // It's clearer to invert this in the signature:
    bool invert = !truthy;
    if (value->tag == JIT_SYM_TRUTHINESS_TAG && value->truthiness.invert == invert) {
        return value;
    }
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space(ctx);
    }
    int truthiness = _Py_uop_sym_truthiness(ctx, value);
    if (truthiness < 0) {
        res->tag = JIT_SYM_TRUTHINESS_TAG;
        res->truthiness.invert = invert;
        res->truthiness.value = (uint16_t)(value - allocation_base(ctx));
    }
    else {
        make_const(res, (truthiness ^ invert) ? Py_True : Py_False);
    }
    return res;
}

// 0 on success, -1 on error.
_Py_UOpsAbstractFrame *
_Py_uop_frame_new(
    JitOptContext *ctx,
    PyCodeObject *co,
    int curr_stackentries,
    JitOptSymbol **args,
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
        ctx->done = true;
        ctx->out_of_space = true;
        return NULL;
    }

    // Initialize with the initial state of all local variables
    for (int i = 0; i < arg_len; i++) {
        frame->locals[i] = args[i];
    }

    for (int i = arg_len; i < co->co_nlocalsplus; i++) {
        JitOptSymbol *local = _Py_uop_sym_new_unknown(ctx);
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stackentries; i++) {
        JitOptSymbol *stackvar = _Py_uop_sym_new_unknown(ctx);
        frame->stack[i] = stackvar;
    }

    return frame;
}

void
_Py_uop_abstractcontext_fini(JitOptContext *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->curr_frame_depth = 0;
    int tys = ctx->t_arena.ty_curr_number;
    for (int i = 0; i < tys; i++) {
        JitOptSymbol *sym = &ctx->t_arena.arena[i];
        if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
            Py_CLEAR(sym->value.value);
        }
    }
}

void
_Py_uop_abstractcontext_init(JitOptContext *ctx)
{
    static_assert(sizeof(JitOptSymbol) <= 2 * sizeof(uint64_t), "JitOptSymbol has grown");
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
}

int
_Py_uop_frame_pop(JitOptContext *ctx)
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

static JitOptSymbol *
make_bottom(JitOptContext *ctx)
{
    JitOptSymbol *sym = sym_new(ctx);
    sym->tag = JIT_SYM_BOTTOM_TAG;
    return sym;
}

PyObject *
_Py_uop_symbols_test(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(ignored))
{
    JitOptContext context;
    JitOptContext *ctx = &context;
    _Py_uop_abstractcontext_init(ctx);
    PyObject *val_42 = NULL;
    PyObject *val_43 = NULL;
    PyObject *tuple = NULL;

    // Use a single 'sym' variable so copy-pasting tests is easier.
    JitOptSymbol *sym = _Py_uop_sym_new_unknown(ctx);
    if (sym == NULL) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(sym), "top is NULL");
    TEST_PREDICATE(!_Py_uop_sym_is_not_null(sym), "top is not NULL");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(sym, &PyLong_Type), "top matches a type");
    TEST_PREDICATE(!_Py_uop_sym_is_const(ctx, sym), "top is a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, sym) == NULL, "top as constant is not NULL");
    TEST_PREDICATE(!_Py_uop_sym_is_bottom(sym), "top is bottom");

    sym = make_bottom(ctx);
    if (sym == NULL) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(sym), "bottom is NULL is not false");
    TEST_PREDICATE(!_Py_uop_sym_is_not_null(sym), "bottom is not NULL is not false");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(sym, &PyLong_Type), "bottom matches a type");
    TEST_PREDICATE(!_Py_uop_sym_is_const(ctx, sym), "bottom is a constant is not false");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, sym) == NULL, "bottom as constant is not NULL");
    TEST_PREDICATE(_Py_uop_sym_is_bottom(sym), "bottom isn't bottom");

    sym = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    if (sym == NULL) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(sym), "int is NULL");
    TEST_PREDICATE(_Py_uop_sym_is_not_null(sym), "int isn't not NULL");
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyLong_Type), "int isn't int");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(sym, &PyFloat_Type), "int matches float");
    TEST_PREDICATE(!_Py_uop_sym_is_const(ctx, sym), "int is a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, sym) == NULL, "int as constant is not NULL");

    _Py_uop_sym_set_type(ctx, sym, &PyLong_Type);  // Should be a no-op
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyLong_Type), "(int and int) isn't int");

    _Py_uop_sym_set_type(ctx, sym, &PyFloat_Type);  // Should make it bottom
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
    _Py_uop_sym_set_const(ctx, sym, val_42);
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, sym) == 1, "bool(42) is not True");
    TEST_PREDICATE(!_Py_uop_sym_is_null(sym), "42 is NULL");
    TEST_PREDICATE(_Py_uop_sym_is_not_null(sym), "42 isn't not NULL");
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyLong_Type), "42 isn't an int");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(sym, &PyFloat_Type), "42 matches float");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, sym), "42 is not a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, sym) != NULL, "42 as constant is NULL");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, sym) == val_42, "42 as constant isn't 42");
    TEST_PREDICATE(_Py_uop_sym_is_immortal(sym), "42 is not immortal");

    _Py_uop_sym_set_type(ctx, sym, &PyLong_Type);  // Should be a no-op
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyLong_Type), "(42 and 42) isn't an int");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, sym) == val_42, "(42 and 42) as constant isn't 42");

    _Py_uop_sym_set_type(ctx, sym, &PyFloat_Type);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(sym), "(42 and float) isn't bottom");

    sym = _Py_uop_sym_new_type(ctx, &PyBool_Type);
    TEST_PREDICATE(_Py_uop_sym_is_immortal(sym), "a bool is not immortal");

    sym = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    if (sym == NULL) {
        goto fail;
    }
    _Py_uop_sym_set_const(ctx, sym, val_42);
    _Py_uop_sym_set_const(ctx, sym, val_43);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(sym), "(42 and 43) isn't bottom");


    sym = _Py_uop_sym_new_const(ctx, Py_None);
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, sym) == 0, "bool(None) is not False");
    sym = _Py_uop_sym_new_const(ctx, Py_False);
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, sym) == 0, "bool(False) is not False");
    sym = _Py_uop_sym_new_const(ctx, PyLong_FromLong(0));
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, sym) == 0, "bool(0) is not False");

    JitOptSymbol *i1 = _Py_uop_sym_new_type(ctx, &PyFloat_Type);
    JitOptSymbol *i2 = _Py_uop_sym_new_const(ctx, val_43);
    JitOptSymbol *array[2] = { i1, i2 };
    sym = _Py_uop_sym_new_tuple(ctx, 2, array);
    TEST_PREDICATE(
        _Py_uop_sym_matches_type(_Py_uop_sym_tuple_getitem(ctx, sym, 0), &PyFloat_Type),
        "tuple item does not match value used to create tuple"
    );
    TEST_PREDICATE(
        _Py_uop_sym_get_const(ctx, _Py_uop_sym_tuple_getitem(ctx, sym, 1)) == val_43,
        "tuple item does not match value used to create tuple"
    );
    PyObject *pair[2] = { val_42, val_43 };
    tuple = _PyTuple_FromArray(pair, 2);
    sym = _Py_uop_sym_new_const(ctx, tuple);
    TEST_PREDICATE(
        _Py_uop_sym_get_const(ctx, _Py_uop_sym_tuple_getitem(ctx, sym, 1)) == val_43,
        "tuple item does not match value used to create tuple"
    );
    JitOptSymbol *value = _Py_uop_sym_new_type(ctx, &PyBool_Type);
    sym = _Py_uop_sym_new_truthiness(ctx, value, false);
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyBool_Type), "truthiness is not boolean");
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, sym) == -1, "truthiness is not unknown");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, sym) == false, "truthiness is constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, sym) == NULL, "truthiness is not NULL");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, value) == false, "value is constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, value) == NULL, "value is not NULL");
    _Py_uop_sym_set_const(ctx, sym, Py_False);
    TEST_PREDICATE(_Py_uop_sym_matches_type(sym, &PyBool_Type), "truthiness is not boolean");
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, sym) == 0, "truthiness is not True");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, sym) == true, "truthiness is not constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, sym) == Py_False, "truthiness is not False");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, value) == true, "value is not constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, value) == Py_True, "value is not True");
    _Py_uop_abstractcontext_fini(ctx);
    Py_DECREF(val_42);
    Py_DECREF(val_43);
    Py_DECREF(tuple);
    Py_RETURN_NONE;

fail:
    _Py_uop_abstractcontext_fini(ctx);
    Py_XDECREF(val_42);
    Py_XDECREF(val_43);
    Py_DECREF(tuple);
    return NULL;
}

#endif /* _Py_TIER2 */
