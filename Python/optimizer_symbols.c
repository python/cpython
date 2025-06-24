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

/*

Symbols
=======

https://github.com/faster-cpython/ideas/blob/main/3.13/redundancy_eliminator.md

Logically, all symbols begin as UNKNOWN, and can transition downwards along the
edges of the lattice, but *never* upwards (see the diagram below). The UNKNOWN
state represents no information, and the BOTTOM state represents contradictory
information. Though symbols logically progress through all intermediate nodes,
we often skip in-between states for convenience:

   UNKNOWN
   |     |
NULL     |
|        |                <- Anything below this level is an object.
|        NON_NULL-+
|          |      |       <- Anything below this level has a known type version.
|    TYPE_VERSION |
|    |            |       <- Anything below this level has a known type.
|    KNOWN_CLASS  |
|    |  |  |   |  |
|    |  | INT* |  |
|    |  |  |   |  |       <- Anything below this level has a known truthiness.
|    |  |  |   |  TRUTHINESS
|    |  |  |   |  |
| TUPLE |  |   |  |
|    |  |  |   |  |       <- Anything below this level is a known constant.
|    KNOWN_VALUE--+
|    |                    <- Anything below this level is unreachable.
BOTTOM

For example, after guarding that the type of an UNKNOWN local is int, we can
narrow the symbol to KNOWN_CLASS (logically progressing though NON_NULL and
TYPE_VERSION to get there). Later, we may learn that it is falsey based on the
result of a truth test, which would allow us to narrow the symbol to KNOWN_VALUE
(with a value of integer zero). If at any point we encounter a float guard on
the same symbol, that would be a contradiction, and the symbol would be set to
BOTTOM (indicating that the code is unreachable).

INT* is a limited range int, currently a "compact" int.

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

JitOptRef
out_of_space_ref(JitOptContext *ctx)
{
    return PyJitRef_Wrap(out_of_space(ctx));
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
    self->tag = JIT_SYM_UNKNOWN_TAG;;
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
_Py_uop_sym_is_bottom(JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    return sym->tag == JIT_SYM_BOTTOM_TAG;
}

bool
_Py_uop_sym_is_not_null(JitOptRef ref) {
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    return sym->tag == JIT_SYM_NON_NULL_TAG || sym->tag > JIT_SYM_BOTTOM_TAG;
}

bool
_Py_uop_sym_is_const(JitOptContext *ctx, JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        return true;
    }
    if (sym->tag == JIT_SYM_TRUTHINESS_TAG) {
        JitOptSymbol *value = allocation_base(ctx) + sym->truthiness.value;
        int truthiness = _Py_uop_sym_truthiness(ctx, PyJitRef_Wrap(value));
        if (truthiness < 0) {
            return false;
        }
        make_const(sym, (truthiness ^ sym->truthiness.invert) ? Py_True : Py_False);
        return true;
    }
    return false;
}

bool
_Py_uop_sym_is_null(JitOptRef ref)
{
    return PyJitRef_Unwrap(ref)->tag == JIT_SYM_NULL_TAG;
}


PyObject *
_Py_uop_sym_get_const(JitOptContext *ctx, JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        return sym->value.value;
    }
    if (sym->tag == JIT_SYM_TRUTHINESS_TAG) {
        JitOptSymbol *value = allocation_base(ctx) + sym->truthiness.value;
        int truthiness = _Py_uop_sym_truthiness(ctx, PyJitRef_Wrap(value));
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
_Py_uop_sym_set_type(JitOptContext *ctx, JitOptRef ref, PyTypeObject *typ)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
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
        case JIT_SYM_COMPACT_INT:
            if (typ != &PyLong_Type) {
                sym_set_bottom(ctx, sym);
            }
            return;
    }
}

bool
_Py_uop_sym_set_type_version(JitOptContext *ctx, JitOptRef ref, unsigned int version)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    PyTypeObject *type = _PyType_LookupByVersion(version);
    if (type) {
        _Py_uop_sym_set_type(ctx, ref, type);
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
        case JIT_SYM_COMPACT_INT:
            if (version != PyLong_Type.tp_version_tag) {
                sym_set_bottom(ctx, sym);
                return false;
            }
            return true;
    }
    Py_UNREACHABLE();
}

void
_Py_uop_sym_set_const(JitOptContext *ctx, JitOptRef ref, PyObject *const_val)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
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
                Py_ssize_t len = _Py_uop_sym_tuple_length(ref);
                if (len == PyTuple_GET_SIZE(const_val)) {
                    for (Py_ssize_t i = 0; i < len; i++) {
                        JitOptRef sym_item = _Py_uop_sym_tuple_getitem(ctx, ref, i);
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
                (_Py_uop_sym_is_const(ctx, ref) &&
                 _Py_uop_sym_get_const(ctx, ref) != const_val))
            {
                sym_set_bottom(ctx, sym);
                return;
            }
            JitOptRef value = PyJitRef_Wrap(
                allocation_base(ctx) + sym->truthiness.value);
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
        case JIT_SYM_COMPACT_INT:
            if (_PyLong_CheckExactAndCompact(const_val)) {
                make_const(sym, const_val);
            }
            else {
                sym_set_bottom(ctx, sym);
            }
            return;
    }
}

void
_Py_uop_sym_set_null(JitOptContext *ctx, JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    if (sym->tag == JIT_SYM_UNKNOWN_TAG) {
        sym->tag = JIT_SYM_NULL_TAG;
    }
    else if (sym->tag > JIT_SYM_NULL_TAG) {
        sym_set_bottom(ctx, sym);
    }
}

void
_Py_uop_sym_set_non_null(JitOptContext *ctx, JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    if (sym->tag == JIT_SYM_UNKNOWN_TAG) {
        sym->tag = JIT_SYM_NON_NULL_TAG;
    }
    else if (sym->tag == JIT_SYM_NULL_TAG) {
        sym_set_bottom(ctx, sym);
    }
}

JitOptRef
_Py_uop_sym_new_unknown(JitOptContext *ctx)
{
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space_ref(ctx);
    }
    return PyJitRef_Wrap(res);
}

JitOptRef
_Py_uop_sym_new_not_null(JitOptContext *ctx)
{
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space_ref(ctx);
    }
    res->tag = JIT_SYM_NON_NULL_TAG;
    return PyJitRef_Wrap(res);
}

JitOptRef
_Py_uop_sym_new_type(JitOptContext *ctx, PyTypeObject *typ)
{
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space_ref(ctx);
    }
    JitOptRef ref = PyJitRef_Wrap(res);
    _Py_uop_sym_set_type(ctx, ref, typ);
    return ref;
}

// Adds a new reference to const_val, owned by the symbol.
JitOptRef
_Py_uop_sym_new_const(JitOptContext *ctx, PyObject *const_val)
{
    assert(const_val != NULL);
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space_ref(ctx);
    }
    JitOptRef ref = PyJitRef_Wrap(res);
    _Py_uop_sym_set_const(ctx, ref, const_val);
    return ref;
}

JitOptRef
_Py_uop_sym_new_null(JitOptContext *ctx)
{
    JitOptSymbol *null_sym = sym_new(ctx);
    if (null_sym == NULL) {
        return out_of_space_ref(ctx);
    }
    JitOptRef ref = PyJitRef_Wrap(null_sym);
    _Py_uop_sym_set_null(ctx, ref);
    return ref;
}

PyTypeObject *
_Py_uop_sym_get_type(JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    JitSymType tag = sym->tag;
    switch(tag) {
        case JIT_SYM_NULL_TAG:
        case JIT_SYM_BOTTOM_TAG:
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            return NULL;
        case JIT_SYM_KNOWN_CLASS_TAG:
            return sym->cls.type;
        case JIT_SYM_KNOWN_VALUE_TAG:
            return Py_TYPE(sym->value.value);
        case JIT_SYM_TYPE_VERSION_TAG:
            return _PyType_LookupByVersion(sym->version.version);
        case JIT_SYM_TUPLE_TAG:
            return &PyTuple_Type;
        case JIT_SYM_TRUTHINESS_TAG:
            return &PyBool_Type;
        case JIT_SYM_COMPACT_INT:
            return &PyLong_Type;

    }
    Py_UNREACHABLE();
}

unsigned int
_Py_uop_sym_get_type_version(JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
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
        case JIT_SYM_COMPACT_INT:
            return PyLong_Type.tp_version_tag;
    }
    Py_UNREACHABLE();
}

bool
_Py_uop_sym_has_type(JitOptRef sym)
{
    return _Py_uop_sym_get_type(sym) != NULL;
}

bool
_Py_uop_sym_matches_type(JitOptRef sym, PyTypeObject *typ)
{
    assert(typ != NULL && PyType_Check(typ));
    return _Py_uop_sym_get_type(sym) == typ;
}

bool
_Py_uop_sym_matches_type_version(JitOptRef sym, unsigned int version)
{
    return _Py_uop_sym_get_type_version(sym) == version;
}

int
_Py_uop_sym_truthiness(JitOptContext *ctx, JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    switch(sym->tag) {
        case JIT_SYM_NULL_TAG:
        case JIT_SYM_TYPE_VERSION_TAG:
        case JIT_SYM_BOTTOM_TAG:
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
        case JIT_SYM_COMPACT_INT:
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
            int truthiness = _Py_uop_sym_truthiness(ctx,
                                                    PyJitRef_Wrap(value));
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

JitOptRef
_Py_uop_sym_new_tuple(JitOptContext *ctx, int size, JitOptRef *args)
{
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space_ref(ctx);
    }
    if (size > MAX_SYMBOLIC_TUPLE_SIZE) {
        res->tag = JIT_SYM_KNOWN_CLASS_TAG;
        res->cls.type = &PyTuple_Type;
    }
    else {
        res->tag = JIT_SYM_TUPLE_TAG;
        res->tuple.length = size;
        for (int i = 0; i < size; i++) {
            res->tuple.items[i] = (uint16_t)(PyJitRef_Unwrap(args[i]) - allocation_base(ctx));
        }
    }
    return PyJitRef_Wrap(res);
}

JitOptRef
_Py_uop_sym_tuple_getitem(JitOptContext *ctx, JitOptRef ref, int item)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    assert(item >= 0);
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        PyObject *tuple = sym->value.value;
        if (PyTuple_CheckExact(tuple) && item < PyTuple_GET_SIZE(tuple)) {
            return _Py_uop_sym_new_const(ctx, PyTuple_GET_ITEM(tuple, item));
        }
    }
    else if (sym->tag == JIT_SYM_TUPLE_TAG && item < sym->tuple.length) {
        return PyJitRef_Wrap(allocation_base(ctx) + sym->tuple.items[item]);
    }
    return _Py_uop_sym_new_not_null(ctx);
}

int
_Py_uop_sym_tuple_length(JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
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
_Py_uop_symbol_is_immortal(JitOptSymbol *sym)
{
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        return _Py_IsImmortal(sym->value.value);
    }
    if (sym->tag == JIT_SYM_KNOWN_CLASS_TAG) {
        return sym->cls.type == &PyBool_Type;
    }
    return false;
}

bool
_Py_uop_sym_is_compact_int(JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    if (sym->tag == JIT_SYM_KNOWN_VALUE_TAG) {
        return (bool)_PyLong_CheckExactAndCompact(sym->value.value);
    }
    return sym->tag == JIT_SYM_COMPACT_INT;
}

bool
_Py_uop_sym_is_immortal(JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    return _Py_uop_symbol_is_immortal(sym);
}

void
_Py_uop_sym_set_compact_int(JitOptContext *ctx, JitOptRef ref)
{
    JitOptSymbol *sym = PyJitRef_Unwrap(ref);
    JitSymType tag = sym->tag;
    switch(tag) {
        case JIT_SYM_NULL_TAG:
            sym_set_bottom(ctx, sym);
            return;
        case JIT_SYM_KNOWN_CLASS_TAG:
            if (sym->cls.type == &PyLong_Type) {
                sym->tag = JIT_SYM_COMPACT_INT;
            } else {
                sym_set_bottom(ctx, sym);
            }
            return;
        case JIT_SYM_TYPE_VERSION_TAG:
            if (sym->version.version == PyLong_Type.tp_version_tag) {
                sym->tag = JIT_SYM_COMPACT_INT;
            }
            else {
                sym_set_bottom(ctx, sym);
            }
            return;
        case JIT_SYM_KNOWN_VALUE_TAG:
            if (!_PyLong_CheckExactAndCompact(sym->value.value)) {
                Py_CLEAR(sym->value.value);
                sym_set_bottom(ctx, sym);
            }
            return;
        case JIT_SYM_TUPLE_TAG:
        case JIT_SYM_TRUTHINESS_TAG:
            sym_set_bottom(ctx, sym);
            return;
        case JIT_SYM_BOTTOM_TAG:
        case JIT_SYM_COMPACT_INT:
            return;
        case JIT_SYM_NON_NULL_TAG:
        case JIT_SYM_UNKNOWN_TAG:
            sym->tag = JIT_SYM_COMPACT_INT;
            return;
    }
}

JitOptRef
_Py_uop_sym_new_truthiness(JitOptContext *ctx, JitOptRef ref, bool truthy)
{
    JitOptSymbol *value = PyJitRef_Unwrap(ref);
    // It's clearer to invert this in the signature:
    bool invert = !truthy;
    if (value->tag == JIT_SYM_TRUTHINESS_TAG && value->truthiness.invert == invert) {
        return ref;
    }
    JitOptSymbol *res = sym_new(ctx);
    if (res == NULL) {
        return out_of_space_ref(ctx);
    }
    int truthiness = _Py_uop_sym_truthiness(ctx, ref);
    if (truthiness < 0) {
        res->tag = JIT_SYM_TRUTHINESS_TAG;
        res->truthiness.invert = invert;
        res->truthiness.value = (uint16_t)(value - allocation_base(ctx));
    }
    else {
        make_const(res, (truthiness ^ invert) ? Py_True : Py_False);
    }
    return PyJitRef_Wrap(res);
}

JitOptRef
_Py_uop_sym_new_compact_int(JitOptContext *ctx)
{
    JitOptSymbol *sym = sym_new(ctx);
    if (sym == NULL) {
        return out_of_space_ref(ctx);
    }
    sym->tag = JIT_SYM_COMPACT_INT;
    return PyJitRef_Wrap(sym);
}

// 0 on success, -1 on error.
_Py_UOpsAbstractFrame *
_Py_uop_frame_new(
    JitOptContext *ctx,
    PyCodeObject *co,
    int curr_stackentries,
    JitOptRef *args,
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
        JitOptRef local = _Py_uop_sym_new_unknown(ctx);
        frame->locals[i] = local;
    }


    // Initialize the stack as well
    for (int i = 0; i < curr_stackentries; i++) {
        JitOptRef stackvar = _Py_uop_sym_new_unknown(ctx);
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
    static_assert(sizeof(JitOptSymbol) <= 3 * sizeof(uint64_t), "JitOptSymbol has grown");
    ctx->limit = ctx->locals_and_stack + MAX_ABSTRACT_INTERP_SIZE;
    ctx->n_consumed = ctx->locals_and_stack;
#ifdef Py_DEBUG // Aids debugging a little. There should never be NULL in the abstract interpreter.
    for (int i = 0 ; i < MAX_ABSTRACT_INTERP_SIZE; i++) {
        ctx->locals_and_stack[i] = PyJitRef_NULL;
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
    PyObject *val_big = NULL;
    PyObject *tuple = NULL;

    // Use a single 'sym' variable so copy-pasting tests is easier.
    JitOptRef ref = _Py_uop_sym_new_unknown(ctx);
    if (PyJitRef_IsNull(ref)) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(ref), "top is NULL");
    TEST_PREDICATE(!_Py_uop_sym_is_not_null(ref), "top is not NULL");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(ref, &PyLong_Type), "top matches a type");
    TEST_PREDICATE(!_Py_uop_sym_is_const(ctx, ref), "top is a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref) == NULL, "top as constant is not NULL");
    TEST_PREDICATE(!_Py_uop_sym_is_bottom(ref), "top is bottom");

    ref = PyJitRef_Wrap(make_bottom(ctx));
    if (PyJitRef_IsNull(ref)) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(ref), "bottom is NULL is not false");
    TEST_PREDICATE(!_Py_uop_sym_is_not_null(ref), "bottom is not NULL is not false");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(ref, &PyLong_Type), "bottom matches a type");
    TEST_PREDICATE(!_Py_uop_sym_is_const(ctx, ref), "bottom is a constant is not false");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref) == NULL, "bottom as constant is not NULL");
    TEST_PREDICATE(_Py_uop_sym_is_bottom(ref), "bottom isn't bottom");

    ref = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    if (PyJitRef_IsNull(ref)) {
        goto fail;
    }
    TEST_PREDICATE(!_Py_uop_sym_is_null(ref), "int is NULL");
    TEST_PREDICATE(_Py_uop_sym_is_not_null(ref), "int isn't not NULL");
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref, &PyLong_Type), "int isn't int");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(ref, &PyFloat_Type), "int matches float");
    TEST_PREDICATE(!_Py_uop_sym_is_const(ctx, ref), "int is a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref) == NULL, "int as constant is not NULL");

    _Py_uop_sym_set_type(ctx, ref, &PyLong_Type);  // Should be a no-op
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref, &PyLong_Type), "(int and int) isn't int");

    _Py_uop_sym_set_type(ctx, ref, &PyFloat_Type);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(ref), "(int and float) isn't bottom");

    val_42 = PyLong_FromLong(42);
    assert(val_42 != NULL);
    assert(_Py_IsImmortal(val_42));

    val_43 = PyLong_FromLong(43);
    assert(val_43 != NULL);
    assert(_Py_IsImmortal(val_43));

    ref = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    if (PyJitRef_IsNull(ref)) {
        goto fail;
    }
    _Py_uop_sym_set_const(ctx, ref, val_42);
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, ref) == 1, "bool(42) is not True");
    TEST_PREDICATE(!_Py_uop_sym_is_null(ref), "42 is NULL");
    TEST_PREDICATE(_Py_uop_sym_is_not_null(ref), "42 isn't not NULL");
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref, &PyLong_Type), "42 isn't an int");
    TEST_PREDICATE(!_Py_uop_sym_matches_type(ref, &PyFloat_Type), "42 matches float");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, ref), "42 is not a constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref) != NULL, "42 as constant is NULL");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref) == val_42, "42 as constant isn't 42");
    TEST_PREDICATE(_Py_uop_sym_is_immortal(ref), "42 is not immortal");

    _Py_uop_sym_set_type(ctx, ref, &PyLong_Type);  // Should be a no-op
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref, &PyLong_Type), "(42 and 42) isn't an int");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref) == val_42, "(42 and 42) as constant isn't 42");

    _Py_uop_sym_set_type(ctx, ref, &PyFloat_Type);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(ref), "(42 and float) isn't bottom");

    ref = _Py_uop_sym_new_type(ctx, &PyBool_Type);
    TEST_PREDICATE(_Py_uop_sym_is_immortal(ref), "a bool is not immortal");

    ref = _Py_uop_sym_new_type(ctx, &PyLong_Type);
    if (PyJitRef_IsNull(ref)) {
        goto fail;
    }
    _Py_uop_sym_set_const(ctx, ref, val_42);
    _Py_uop_sym_set_const(ctx, ref, val_43);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(ref), "(42 and 43) isn't bottom");


    ref = _Py_uop_sym_new_const(ctx, Py_None);
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, ref) == 0, "bool(None) is not False");
    ref = _Py_uop_sym_new_const(ctx, Py_False);
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, ref) == 0, "bool(False) is not False");
    ref = _Py_uop_sym_new_const(ctx, PyLong_FromLong(0));
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, ref) == 0, "bool(0) is not False");

    JitOptRef i1 = _Py_uop_sym_new_type(ctx, &PyFloat_Type);
    JitOptRef i2 = _Py_uop_sym_new_const(ctx, val_43);
    JitOptRef array[2] = { i1, i2 };
    ref = _Py_uop_sym_new_tuple(ctx, 2, array);
    TEST_PREDICATE(
        _Py_uop_sym_matches_type(_Py_uop_sym_tuple_getitem(ctx, ref, 0), &PyFloat_Type),
        "tuple item does not match value used to create tuple"
    );
    TEST_PREDICATE(
        _Py_uop_sym_get_const(ctx, _Py_uop_sym_tuple_getitem(ctx, ref, 1)) == val_43,
        "tuple item does not match value used to create tuple"
    );
    PyObject *pair[2] = { val_42, val_43 };
    tuple = _PyTuple_FromArray(pair, 2);
    ref = _Py_uop_sym_new_const(ctx, tuple);
    TEST_PREDICATE(
        _Py_uop_sym_get_const(ctx, _Py_uop_sym_tuple_getitem(ctx, ref, 1)) == val_43,
        "tuple item does not match value used to create tuple"
    );
    ref = _Py_uop_sym_new_type(ctx, &PyTuple_Type);
    TEST_PREDICATE(
        _Py_uop_sym_is_not_null(_Py_uop_sym_tuple_getitem(ctx, ref, 42)),
        "Unknown tuple item is not narrowed to non-NULL"
    );
    JitOptRef value = _Py_uop_sym_new_type(ctx, &PyBool_Type);
    ref = _Py_uop_sym_new_truthiness(ctx, value, false);
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref, &PyBool_Type), "truthiness is not boolean");
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, ref) == -1, "truthiness is not unknown");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, ref) == false, "truthiness is constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref) == NULL, "truthiness is not NULL");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, value) == false, "value is constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, value) == NULL, "value is not NULL");
    _Py_uop_sym_set_const(ctx, ref, Py_False);
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref, &PyBool_Type), "truthiness is not boolean");
    TEST_PREDICATE(_Py_uop_sym_truthiness(ctx, ref) == 0, "truthiness is not True");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, ref) == true, "truthiness is not constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref) == Py_False, "truthiness is not False");
    TEST_PREDICATE(_Py_uop_sym_is_const(ctx, value) == true, "value is not constant");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, value) == Py_True, "value is not True");


    val_big = PyNumber_Lshift(_PyLong_GetOne(), PyLong_FromLong(66));
    if (val_big == NULL) {
        goto fail;
    }

    JitOptRef ref_42 = _Py_uop_sym_new_const(ctx, val_42);
    JitOptRef ref_big = _Py_uop_sym_new_const(ctx, val_big);
    JitOptRef ref_int = _Py_uop_sym_new_compact_int(ctx);
    TEST_PREDICATE(_Py_uop_sym_is_compact_int(ref_42), "42 is not a compact int");
    TEST_PREDICATE(!_Py_uop_sym_is_compact_int(ref_big), "(1 << 66) is a compact int");
    TEST_PREDICATE(_Py_uop_sym_is_compact_int(ref_int), "compact int is not a compact int");
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref_int, &PyLong_Type), "compact int is not an int");

    _Py_uop_sym_set_type(ctx, ref_int, &PyLong_Type);  // Should have no effect
    TEST_PREDICATE(_Py_uop_sym_is_compact_int(ref_int), "compact int is not a compact int after cast");
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref_int, &PyLong_Type), "compact int is not an int after cast");

    _Py_uop_sym_set_type(ctx, ref_int, &PyFloat_Type);  // Should make it bottom
    TEST_PREDICATE(_Py_uop_sym_is_bottom(ref_int), "compact int cast to float isn't bottom");

    ref_int = _Py_uop_sym_new_compact_int(ctx);
    _Py_uop_sym_set_const(ctx, ref_int, val_43);
    TEST_PREDICATE(_Py_uop_sym_is_compact_int(ref_int), "43 is not a compact int");
    TEST_PREDICATE(_Py_uop_sym_matches_type(ref_int, &PyLong_Type), "43 is not an int");
    TEST_PREDICATE(_Py_uop_sym_get_const(ctx, ref_int) == val_43, "43 isn't 43");

    _Py_uop_abstractcontext_fini(ctx);
    Py_DECREF(val_42);
    Py_DECREF(val_43);
    Py_DECREF(val_big);
    Py_DECREF(tuple);
    Py_RETURN_NONE;

fail:
    _Py_uop_abstractcontext_fini(ctx);
    Py_XDECREF(val_42);
    Py_XDECREF(val_43);
    Py_XDECREF(val_big);
    Py_DECREF(tuple);
    return NULL;
}

#endif /* _Py_TIER2 */
