#include "Python.h"
#include "pycore_ast.h"           // expr_ty
#include <float.h>                // DBL_MAX_10_EXP
#include <stdbool.h>

/* This limited unparser is used to convert annotations back to strings
 * during compilation rather than being a full AST unparser.
 * See ast.unparse for a full unparser (written in Python)
 */

static PyObject *_str_open_br;
static PyObject *_str_dbl_open_br;
static PyObject *_str_close_br;
static PyObject *_str_dbl_close_br;
static PyObject *_str_inf;
static PyObject *_str_replace_inf;

/* Forward declarations for recursion via helper functions. */
static PyObject *
expr_as_unicode(expr_ty e, int level);
static int
append_ast_expr(_PyUnicodeWriter *writer, expr_ty e, int level);
static int
append_joinedstr(_PyUnicodeWriter *writer, expr_ty e, bool is_format_spec);
static int
append_formattedvalue(_PyUnicodeWriter *writer, expr_ty e);
static int
append_ast_slice(_PyUnicodeWriter *writer, expr_ty e);

static int
append_charp(_PyUnicodeWriter *writer, const char *charp)
{
    return _PyUnicodeWriter_WriteASCIIString(writer, charp, -1);
}

#define APPEND_STR_FINISH(str)  do { \
        return append_charp(writer, (str)); \
    } while (0)

#define APPEND_STR(str)  do { \
        if (-1 == append_charp(writer, (str))) { \
            return -1; \
        } \
    } while (0)

#define APPEND_STR_IF(cond, str)  do { \
        if ((cond) && -1 == append_charp(writer, (str))) { \
            return -1; \
        } \
    } while (0)

#define APPEND_STR_IF_NOT_FIRST(str)  do { \
        APPEND_STR_IF(!first, (str)); \
        first = false; \
    } while (0)

#define APPEND_EXPR(expr, pr)  do { \
        if (-1 == append_ast_expr(writer, (expr), (pr))) { \
            return -1; \
        } \
    } while (0)

#define APPEND(type, value)  do { \
        if (-1 == append_ast_ ## type(writer, (value))) { \
            return -1; \
        } \
    } while (0)

static int
append_repr(_PyUnicodeWriter *writer, PyObject *obj)
{
    PyObject *repr = PyObject_Repr(obj);

    if (!repr) {
        return -1;
    }

    if ((PyFloat_CheckExact(obj) && Py_IS_INFINITY(PyFloat_AS_DOUBLE(obj))) ||
       PyComplex_CheckExact(obj))
    {
        PyObject *new_repr = PyUnicode_Replace(
            repr,
            _str_inf,
            _str_replace_inf,
            -1
        );
        Py_DECREF(repr);
        if (!new_repr) {
            return -1;
        }
        repr = new_repr;
    }
    int ret = _PyUnicodeWriter_WriteStr(writer, repr);
    Py_DECREF(repr);
    return ret;
}

/* Priority levels */

enum {
    PR_TUPLE,
    PR_TEST,            /* 'if'-'else', 'lambda' */
    PR_OR,              /* 'or' */
    PR_AND,             /* 'and' */
    PR_NOT,             /* 'not' */
    PR_CMP,             /* '<', '>', '==', '>=', '<=', '!=',
                           'in', 'not in', 'is', 'is not' */
    PR_EXPR,
    PR_BOR = PR_EXPR,   /* '|' */
    PR_BXOR,            /* '^' */
    PR_BAND,            /* '&' */
    PR_SHIFT,           /* '<<', '>>' */
    PR_ARITH,           /* '+', '-' */
    PR_TERM,            /* '*', '@', '/', '%', '//' */
    PR_FACTOR,          /* unary '+', '-', '~' */
    PR_POWER,           /* '**' */
    PR_AWAIT,           /* 'await' */
    PR_ATOM,
};

static int
append_ast_boolop(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    Py_ssize_t i, value_count;
    asdl_expr_seq *values;
    const char *op = (e->v.BoolOp.op == And) ? " and " : " or ";
    int pr = (e->v.BoolOp.op == And) ? PR_AND : PR_OR;

    APPEND_STR_IF(level > pr, "(");

    values = e->v.BoolOp.values;
    value_count = asdl_seq_LEN(values);

    for (i = 0; i < value_count; ++i) {
        APPEND_STR_IF(i > 0, op);
        APPEND_EXPR((expr_ty)asdl_seq_GET(values, i), pr + 1);
    }

    APPEND_STR_IF(level > pr, ")");
    return 0;
}

static int
append_ast_binop(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    const char *op;
    int pr;
    bool rassoc = false;  /* is right-associative? */

    switch (e->v.BinOp.op) {
    case Add: op = " + "; pr = PR_ARITH; break;
    case Sub: op = " - "; pr = PR_ARITH; break;
    case Mult: op = " * "; pr = PR_TERM; break;
    case MatMult: op = " @ "; pr = PR_TERM; break;
    case Div: op = " / "; pr = PR_TERM; break;
    case Mod: op = " % "; pr = PR_TERM; break;
    case LShift: op = " << "; pr = PR_SHIFT; break;
    case RShift: op = " >> "; pr = PR_SHIFT; break;
    case BitOr: op = " | "; pr = PR_BOR; break;
    case BitXor: op = " ^ "; pr = PR_BXOR; break;
    case BitAnd: op = " & "; pr = PR_BAND; break;
    case FloorDiv: op = " // "; pr = PR_TERM; break;
    case Pow: op = " ** "; pr = PR_POWER; rassoc = true; break;
    default:
        PyErr_SetString(PyExc_SystemError,
                        "unknown binary operator");
        return -1;
    }

    APPEND_STR_IF(level > pr, "(");
    APPEND_EXPR(e->v.BinOp.left, pr + rassoc);
    APPEND_STR(op);
    APPEND_EXPR(e->v.BinOp.right, pr + !rassoc);
    APPEND_STR_IF(level > pr, ")");
    return 0;
}

static int
append_ast_unaryop(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    const char *op;
    int pr;

    switch (e->v.UnaryOp.op) {
    case Invert: op = "~"; pr = PR_FACTOR; break;
    case Not: op = "not "; pr = PR_NOT; break;
    case UAdd: op = "+"; pr = PR_FACTOR; break;
    case USub: op = "-"; pr = PR_FACTOR; break;
    default:
        PyErr_SetString(PyExc_SystemError,
                        "unknown unary operator");
        return -1;
    }

    APPEND_STR_IF(level > pr, "(");
    APPEND_STR(op);
    APPEND_EXPR(e->v.UnaryOp.operand, pr);
    APPEND_STR_IF(level > pr, ")");
    return 0;
}

static int
append_ast_arg(_PyUnicodeWriter *writer, arg_ty arg)
{
    if (-1 == _PyUnicodeWriter_WriteStr(writer, arg->arg)) {
        return -1;
    }
    if (arg->annotation) {
        APPEND_STR(": ");
        APPEND_EXPR(arg->annotation, PR_TEST);
    }
    return 0;
}

static int
append_ast_args(_PyUnicodeWriter *writer, arguments_ty args)
{
    bool first;
    Py_ssize_t i, di, arg_count, posonlyarg_count, default_count;

    first = true;

    /* positional-only and positional arguments with defaults */
    posonlyarg_count = asdl_seq_LEN(args->posonlyargs);
    arg_count = asdl_seq_LEN(args->args);
    default_count = asdl_seq_LEN(args->defaults);
    for (i = 0; i < posonlyarg_count + arg_count; i++) {
        APPEND_STR_IF_NOT_FIRST(", ");
        if (i < posonlyarg_count){
            APPEND(arg, (arg_ty)asdl_seq_GET(args->posonlyargs, i));
        } else {
            APPEND(arg, (arg_ty)asdl_seq_GET(args->args, i-posonlyarg_count));
        }

        di = i - posonlyarg_count - arg_count + default_count;
        if (di >= 0) {
            APPEND_STR("=");
            APPEND_EXPR((expr_ty)asdl_seq_GET(args->defaults, di), PR_TEST);
        }
        if (posonlyarg_count && i + 1 == posonlyarg_count) {
            APPEND_STR(", /");
        }
    }

    /* vararg, or bare '*' if no varargs but keyword-only arguments present */
    if (args->vararg || asdl_seq_LEN(args->kwonlyargs)) {
        APPEND_STR_IF_NOT_FIRST(", ");
        APPEND_STR("*");
        if (args->vararg) {
            APPEND(arg, args->vararg);
        }
    }

    /* keyword-only arguments */
    arg_count = asdl_seq_LEN(args->kwonlyargs);
    default_count = asdl_seq_LEN(args->kw_defaults);
    for (i = 0; i < arg_count; i++) {
        APPEND_STR_IF_NOT_FIRST(", ");
        APPEND(arg, (arg_ty)asdl_seq_GET(args->kwonlyargs, i));

        di = i - arg_count + default_count;
        if (di >= 0) {
            expr_ty default_ = (expr_ty)asdl_seq_GET(args->kw_defaults, di);
            if (default_) {
                APPEND_STR("=");
                APPEND_EXPR(default_, PR_TEST);
            }
        }
    }

    /* **kwargs */
    if (args->kwarg) {
        APPEND_STR_IF_NOT_FIRST(", ");
        APPEND_STR("**");
        APPEND(arg, args->kwarg);
    }

    return 0;
}

static int
append_ast_lambda(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    APPEND_STR_IF(level > PR_TEST, "(");
    Py_ssize_t n_positional = (asdl_seq_LEN(e->v.Lambda.args->args) +
                               asdl_seq_LEN(e->v.Lambda.args->posonlyargs));
    APPEND_STR(n_positional ? "lambda " : "lambda");
    APPEND(args, e->v.Lambda.args);
    APPEND_STR(": ");
    APPEND_EXPR(e->v.Lambda.body, PR_TEST);
    APPEND_STR_IF(level > PR_TEST, ")");
    return 0;
}

static int
append_ast_ifexp(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    APPEND_STR_IF(level > PR_TEST, "(");
    APPEND_EXPR(e->v.IfExp.body, PR_TEST + 1);
    APPEND_STR(" if ");
    APPEND_EXPR(e->v.IfExp.test, PR_TEST + 1);
    APPEND_STR(" else ");
    APPEND_EXPR(e->v.IfExp.orelse, PR_TEST);
    APPEND_STR_IF(level > PR_TEST, ")");
    return 0;
}

static int
append_ast_dict(_PyUnicodeWriter *writer, expr_ty e)
{
    Py_ssize_t i, value_count;
    expr_ty key_node;

    APPEND_STR("{");
    value_count = asdl_seq_LEN(e->v.Dict.values);

    for (i = 0; i < value_count; i++) {
        APPEND_STR_IF(i > 0, ", ");
        key_node = (expr_ty)asdl_seq_GET(e->v.Dict.keys, i);
        if (key_node != NULL) {
            APPEND_EXPR(key_node, PR_TEST);
            APPEND_STR(": ");
            APPEND_EXPR((expr_ty)asdl_seq_GET(e->v.Dict.values, i), PR_TEST);
        }
        else {
            APPEND_STR("**");
            APPEND_EXPR((expr_ty)asdl_seq_GET(e->v.Dict.values, i), PR_EXPR);
        }
    }

    APPEND_STR_FINISH("}");
}

static int
append_ast_set(_PyUnicodeWriter *writer, expr_ty e)
{
    Py_ssize_t i, elem_count;

    APPEND_STR("{");
    elem_count = asdl_seq_LEN(e->v.Set.elts);
    for (i = 0; i < elem_count; i++) {
        APPEND_STR_IF(i > 0, ", ");
        APPEND_EXPR((expr_ty)asdl_seq_GET(e->v.Set.elts, i), PR_TEST);
    }

    APPEND_STR_FINISH("}");
}

static int
append_ast_list(_PyUnicodeWriter *writer, expr_ty e)
{
    Py_ssize_t i, elem_count;

    APPEND_STR("[");
    elem_count = asdl_seq_LEN(e->v.List.elts);
    for (i = 0; i < elem_count; i++) {
        APPEND_STR_IF(i > 0, ", ");
        APPEND_EXPR((expr_ty)asdl_seq_GET(e->v.List.elts, i), PR_TEST);
    }

    APPEND_STR_FINISH("]");
}

static int
append_ast_tuple(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    Py_ssize_t i, elem_count;

    elem_count = asdl_seq_LEN(e->v.Tuple.elts);

    if (elem_count == 0) {
        APPEND_STR_FINISH("()");
    }

    APPEND_STR_IF(level > PR_TUPLE, "(");

    for (i = 0; i < elem_count; i++) {
        APPEND_STR_IF(i > 0, ", ");
        APPEND_EXPR((expr_ty)asdl_seq_GET(e->v.Tuple.elts, i), PR_TEST);
    }

    APPEND_STR_IF(elem_count == 1, ",");
    APPEND_STR_IF(level > PR_TUPLE, ")");
    return 0;
}

static int
append_ast_comprehension(_PyUnicodeWriter *writer, comprehension_ty gen)
{
    Py_ssize_t i, if_count;

    APPEND_STR(gen->is_async ? " async for " : " for ");
    APPEND_EXPR(gen->target, PR_TUPLE);
    APPEND_STR(" in ");
    APPEND_EXPR(gen->iter, PR_TEST + 1);

    if_count = asdl_seq_LEN(gen->ifs);
    for (i = 0; i < if_count; i++) {
        APPEND_STR(" if ");
        APPEND_EXPR((expr_ty)asdl_seq_GET(gen->ifs, i), PR_TEST + 1);
    }
    return 0;
}

static int
append_ast_comprehensions(_PyUnicodeWriter *writer, asdl_comprehension_seq *comprehensions)
{
    Py_ssize_t i, gen_count;
    gen_count = asdl_seq_LEN(comprehensions);

    for (i = 0; i < gen_count; i++) {
        APPEND(comprehension, (comprehension_ty)asdl_seq_GET(comprehensions, i));
    }

    return 0;
}

static int
append_ast_genexp(_PyUnicodeWriter *writer, expr_ty e)
{
    APPEND_STR("(");
    APPEND_EXPR(e->v.GeneratorExp.elt, PR_TEST);
    APPEND(comprehensions, e->v.GeneratorExp.generators);
    APPEND_STR_FINISH(")");
}

static int
append_ast_listcomp(_PyUnicodeWriter *writer, expr_ty e)
{
    APPEND_STR("[");
    APPEND_EXPR(e->v.ListComp.elt, PR_TEST);
    APPEND(comprehensions, e->v.ListComp.generators);
    APPEND_STR_FINISH("]");
}

static int
append_ast_setcomp(_PyUnicodeWriter *writer, expr_ty e)
{
    APPEND_STR("{");
    APPEND_EXPR(e->v.SetComp.elt, PR_TEST);
    APPEND(comprehensions, e->v.SetComp.generators);
    APPEND_STR_FINISH("}");
}

static int
append_ast_dictcomp(_PyUnicodeWriter *writer, expr_ty e)
{
    APPEND_STR("{");
    APPEND_EXPR(e->v.DictComp.key, PR_TEST);
    APPEND_STR(": ");
    APPEND_EXPR(e->v.DictComp.value, PR_TEST);
    APPEND(comprehensions, e->v.DictComp.generators);
    APPEND_STR_FINISH("}");
}

static int
append_ast_compare(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    const char *op;
    Py_ssize_t i, comparator_count;
    asdl_expr_seq *comparators;
    asdl_int_seq *ops;

    APPEND_STR_IF(level > PR_CMP, "(");

    comparators = e->v.Compare.comparators;
    ops = e->v.Compare.ops;
    comparator_count = asdl_seq_LEN(comparators);
    assert(comparator_count > 0);
    assert(comparator_count == asdl_seq_LEN(ops));

    APPEND_EXPR(e->v.Compare.left, PR_CMP + 1);

    for (i = 0; i < comparator_count; i++) {
        switch ((cmpop_ty)asdl_seq_GET(ops, i)) {
        case Eq:
            op = " == ";
            break;
        case NotEq:
            op = " != ";
            break;
        case Lt:
            op = " < ";
            break;
        case LtE:
            op = " <= ";
            break;
        case Gt:
            op = " > ";
            break;
        case GtE:
            op = " >= ";
            break;
        case Is:
            op = " is ";
            break;
        case IsNot:
            op = " is not ";
            break;
        case In:
            op = " in ";
            break;
        case NotIn:
            op = " not in ";
            break;
        default:
            PyErr_SetString(PyExc_SystemError,
                            "unexpected comparison kind");
            return -1;
        }

        APPEND_STR(op);
        APPEND_EXPR((expr_ty)asdl_seq_GET(comparators, i), PR_CMP + 1);
    }

    APPEND_STR_IF(level > PR_CMP, ")");
    return 0;
}

static int
append_ast_keyword(_PyUnicodeWriter *writer, keyword_ty kw)
{
    if (kw->arg == NULL) {
        APPEND_STR("**");
    }
    else {
        if (-1 == _PyUnicodeWriter_WriteStr(writer, kw->arg)) {
            return -1;
        }

        APPEND_STR("=");
    }

    APPEND_EXPR(kw->value, PR_TEST);
    return 0;
}

static int
append_ast_call(_PyUnicodeWriter *writer, expr_ty e)
{
    bool first;
    Py_ssize_t i, arg_count, kw_count;
    expr_ty expr;

    APPEND_EXPR(e->v.Call.func, PR_ATOM);

    arg_count = asdl_seq_LEN(e->v.Call.args);
    kw_count = asdl_seq_LEN(e->v.Call.keywords);
    if (arg_count == 1 && kw_count == 0) {
        expr = (expr_ty)asdl_seq_GET(e->v.Call.args, 0);
        if (expr->kind == GeneratorExp_kind) {
            /* Special case: a single generator expression. */
            return append_ast_genexp(writer, expr);
        }
    }

    APPEND_STR("(");

    first = true;
    for (i = 0; i < arg_count; i++) {
        APPEND_STR_IF_NOT_FIRST(", ");
        APPEND_EXPR((expr_ty)asdl_seq_GET(e->v.Call.args, i), PR_TEST);
    }

    for (i = 0; i < kw_count; i++) {
        APPEND_STR_IF_NOT_FIRST(", ");
        APPEND(keyword, (keyword_ty)asdl_seq_GET(e->v.Call.keywords, i));
    }

    APPEND_STR_FINISH(")");
}

static PyObject *
escape_braces(PyObject *orig)
{
    PyObject *temp;
    PyObject *result;
    temp = PyUnicode_Replace(orig, _str_open_br, _str_dbl_open_br, -1);
    if (!temp) {
        return NULL;
    }
    result = PyUnicode_Replace(temp, _str_close_br, _str_dbl_close_br, -1);
    Py_DECREF(temp);
    return result;
}

static int
append_fstring_unicode(_PyUnicodeWriter *writer, PyObject *unicode)
{
    PyObject *escaped;
    int result = -1;
    escaped = escape_braces(unicode);
    if (escaped) {
        result = _PyUnicodeWriter_WriteStr(writer, escaped);
        Py_DECREF(escaped);
    }
    return result;
}

static int
append_fstring_element(_PyUnicodeWriter *writer, expr_ty e, bool is_format_spec)
{
    switch (e->kind) {
    case Constant_kind:
        return append_fstring_unicode(writer, e->v.Constant.value);
    case JoinedStr_kind:
        return append_joinedstr(writer, e, is_format_spec);
    case FormattedValue_kind:
        return append_formattedvalue(writer, e);
    default:
        PyErr_SetString(PyExc_SystemError,
                        "unknown expression kind inside f-string");
        return -1;
    }
}

/* Build body separately to enable wrapping the entire stream of Strs,
   Constants and FormattedValues in one opening and one closing quote. */
static PyObject *
build_fstring_body(asdl_expr_seq *values, bool is_format_spec)
{
    Py_ssize_t i, value_count;
    _PyUnicodeWriter body_writer;
    _PyUnicodeWriter_Init(&body_writer);
    body_writer.min_length = 256;
    body_writer.overallocate = 1;

    value_count = asdl_seq_LEN(values);
    for (i = 0; i < value_count; ++i) {
        if (-1 == append_fstring_element(&body_writer,
                                         (expr_ty)asdl_seq_GET(values, i),
                                         is_format_spec
                                         )) {
            _PyUnicodeWriter_Dealloc(&body_writer);
            return NULL;
        }
    }

    return _PyUnicodeWriter_Finish(&body_writer);
}

static int
append_joinedstr(_PyUnicodeWriter *writer, expr_ty e, bool is_format_spec)
{
    int result = -1;
    PyObject *body = build_fstring_body(e->v.JoinedStr.values, is_format_spec);
    if (!body) {
        return -1;
    }

    if (!is_format_spec) {
        if (-1 != append_charp(writer, "f") &&
            -1 != append_repr(writer, body))
        {
            result = 0;
        }
    }
    else {
        result = _PyUnicodeWriter_WriteStr(writer, body);
    }
    Py_DECREF(body);
    return result;
}

static int
append_formattedvalue(_PyUnicodeWriter *writer, expr_ty e)
{
    const char *conversion;
    const char *outer_brace = "{";
    /* Grammar allows PR_TUPLE, but use >PR_TEST for adding parenthesis
       around a lambda with ':' */
    PyObject *temp_fv_str = expr_as_unicode(e->v.FormattedValue.value, PR_TEST + 1);
    if (!temp_fv_str) {
        return -1;
    }
    if (PyUnicode_Find(temp_fv_str, _str_open_br, 0, 1, 1) == 0) {
        /* Expression starts with a brace, split it with a space from the outer
           one. */
        outer_brace = "{ ";
    }
    if (-1 == append_charp(writer, outer_brace)) {
        Py_DECREF(temp_fv_str);
        return -1;
    }
    if (-1 == _PyUnicodeWriter_WriteStr(writer, temp_fv_str)) {
        Py_DECREF(temp_fv_str);
        return -1;
    }
    Py_DECREF(temp_fv_str);

    if (e->v.FormattedValue.conversion > 0) {
        switch (e->v.FormattedValue.conversion) {
        case 'a':
            conversion = "!a";
            break;
        case 'r':
            conversion = "!r";
            break;
        case 's':
            conversion = "!s";
            break;
        default:
            PyErr_SetString(PyExc_SystemError,
                            "unknown f-value conversion kind");
            return -1;
        }
        APPEND_STR(conversion);
    }
    if (e->v.FormattedValue.format_spec) {
        if (-1 == _PyUnicodeWriter_WriteASCIIString(writer, ":", 1) ||
            -1 == append_fstring_element(writer,
                                         e->v.FormattedValue.format_spec,
                                         true
                                        ))
        {
            return -1;
        }
    }

    APPEND_STR_FINISH("}");
}

static int
append_ast_constant(_PyUnicodeWriter *writer, PyObject *constant)
{
    if (PyTuple_CheckExact(constant)) {
        Py_ssize_t i, elem_count;

        elem_count = PyTuple_GET_SIZE(constant);
        APPEND_STR("(");
        for (i = 0; i < elem_count; i++) {
            APPEND_STR_IF(i > 0, ", ");
            if (append_ast_constant(writer, PyTuple_GET_ITEM(constant, i)) < 0) {
                return -1;
            }
        }

        APPEND_STR_IF(elem_count == 1, ",");
        APPEND_STR(")");
        return 0;
    }
    return append_repr(writer, constant);
}

static int
append_ast_attribute(_PyUnicodeWriter *writer, expr_ty e)
{
    const char *period;
    expr_ty v = e->v.Attribute.value;
    APPEND_EXPR(v, PR_ATOM);

    /* Special case: integers require a space for attribute access to be
       unambiguous. */
    if (v->kind == Constant_kind && PyLong_CheckExact(v->v.Constant.value)) {
        period = " .";
    }
    else {
        period = ".";
    }
    APPEND_STR(period);

    return _PyUnicodeWriter_WriteStr(writer, e->v.Attribute.attr);
}

static int
append_ast_slice(_PyUnicodeWriter *writer, expr_ty e)
{
    if (e->v.Slice.lower) {
        APPEND_EXPR(e->v.Slice.lower, PR_TEST);
    }

    APPEND_STR(":");

    if (e->v.Slice.upper) {
        APPEND_EXPR(e->v.Slice.upper, PR_TEST);
    }

    if (e->v.Slice.step) {
        APPEND_STR(":");
        APPEND_EXPR(e->v.Slice.step, PR_TEST);
    }
    return 0;
}

static int
append_ast_subscript(_PyUnicodeWriter *writer, expr_ty e)
{
    APPEND_EXPR(e->v.Subscript.value, PR_ATOM);
    int level = PR_TUPLE;
    expr_ty slice = e->v.Subscript.slice;
    if (slice->kind == Tuple_kind) {
        for (Py_ssize_t i = 0; i < asdl_seq_LEN(slice->v.Tuple.elts); i++) {
            expr_ty element = asdl_seq_GET(slice->v.Tuple.elts, i);
            if (element->kind == Starred_kind) {
                ++level;
                break;
            }
        }
    }
    APPEND_STR("[");
    APPEND_EXPR(e->v.Subscript.slice, level);
    APPEND_STR_FINISH("]");
}

static int
append_ast_starred(_PyUnicodeWriter *writer, expr_ty e)
{
    APPEND_STR("*");
    APPEND_EXPR(e->v.Starred.value, PR_EXPR);
    return 0;
}

static int
append_ast_yield(_PyUnicodeWriter *writer, expr_ty e)
{
    if (!e->v.Yield.value) {
        APPEND_STR_FINISH("(yield)");
    }

    APPEND_STR("(yield ");
    APPEND_EXPR(e->v.Yield.value, PR_TEST);
    APPEND_STR_FINISH(")");
}

static int
append_ast_yield_from(_PyUnicodeWriter *writer, expr_ty e)
{
    APPEND_STR("(yield from ");
    APPEND_EXPR(e->v.YieldFrom.value, PR_TEST);
    APPEND_STR_FINISH(")");
}

static int
append_ast_await(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    APPEND_STR_IF(level > PR_AWAIT, "(");
    APPEND_STR("await ");
    APPEND_EXPR(e->v.Await.value, PR_ATOM);
    APPEND_STR_IF(level > PR_AWAIT, ")");
    return 0;
}

static int
append_named_expr(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    APPEND_STR_IF(level > PR_TUPLE, "(");
    APPEND_EXPR(e->v.NamedExpr.target, PR_ATOM);
    APPEND_STR(" := ");
    APPEND_EXPR(e->v.NamedExpr.value, PR_ATOM);
    APPEND_STR_IF(level > PR_TUPLE, ")");
    return 0;
}

static int
append_ast_expr(_PyUnicodeWriter *writer, expr_ty e, int level)
{
    switch (e->kind) {
    case BoolOp_kind:
        return append_ast_boolop(writer, e, level);
    case BinOp_kind:
        return append_ast_binop(writer, e, level);
    case UnaryOp_kind:
        return append_ast_unaryop(writer, e, level);
    case Lambda_kind:
        return append_ast_lambda(writer, e, level);
    case IfExp_kind:
        return append_ast_ifexp(writer, e, level);
    case Dict_kind:
        return append_ast_dict(writer, e);
    case Set_kind:
        return append_ast_set(writer, e);
    case GeneratorExp_kind:
        return append_ast_genexp(writer, e);
    case ListComp_kind:
        return append_ast_listcomp(writer, e);
    case SetComp_kind:
        return append_ast_setcomp(writer, e);
    case DictComp_kind:
        return append_ast_dictcomp(writer, e);
    case Yield_kind:
        return append_ast_yield(writer, e);
    case YieldFrom_kind:
        return append_ast_yield_from(writer, e);
    case Await_kind:
        return append_ast_await(writer, e, level);
    case Compare_kind:
        return append_ast_compare(writer, e, level);
    case Call_kind:
        return append_ast_call(writer, e);
    case Constant_kind:
        if (e->v.Constant.value == Py_Ellipsis) {
            APPEND_STR_FINISH("...");
        }
        if (e->v.Constant.kind != NULL
            && -1 == _PyUnicodeWriter_WriteStr(writer, e->v.Constant.kind)) {
            return -1;
        }
        return append_ast_constant(writer, e->v.Constant.value);
    case JoinedStr_kind:
        return append_joinedstr(writer, e, false);
    case FormattedValue_kind:
        return append_formattedvalue(writer, e);
    /* The following exprs can be assignment targets. */
    case Attribute_kind:
        return append_ast_attribute(writer, e);
    case Subscript_kind:
        return append_ast_subscript(writer, e);
    case Starred_kind:
        return append_ast_starred(writer, e);
    case Slice_kind:
        return append_ast_slice(writer, e);
    case Name_kind:
        return _PyUnicodeWriter_WriteStr(writer, e->v.Name.id);
    case List_kind:
        return append_ast_list(writer, e);
    case Tuple_kind:
        return append_ast_tuple(writer, e, level);
    case NamedExpr_kind:
        return append_named_expr(writer, e, level);
    // No default so compiler emits a warning for unhandled cases
    }
    PyErr_SetString(PyExc_SystemError,
                    "unknown expression kind");
    return -1;
}

static int
maybe_init_static_strings(void)
{
    if (!_str_open_br &&
        !(_str_open_br = PyUnicode_InternFromString("{"))) {
        return -1;
    }
    if (!_str_dbl_open_br &&
        !(_str_dbl_open_br = PyUnicode_InternFromString("{{"))) {
        return -1;
    }
    if (!_str_close_br &&
        !(_str_close_br = PyUnicode_InternFromString("}"))) {
        return -1;
    }
    if (!_str_dbl_close_br &&
        !(_str_dbl_close_br = PyUnicode_InternFromString("}}"))) {
        return -1;
    }
    if (!_str_inf &&
        !(_str_inf = PyUnicode_FromString("inf"))) {
        return -1;
    }
    if (!_str_replace_inf &&
        !(_str_replace_inf = PyUnicode_FromFormat("1e%d", 1 + DBL_MAX_10_EXP))) {
        return -1;
    }
    return 0;
}

static PyObject *
expr_as_unicode(expr_ty e, int level)
{
    _PyUnicodeWriter writer;
    _PyUnicodeWriter_Init(&writer);
    writer.min_length = 256;
    writer.overallocate = 1;
    if (-1 == maybe_init_static_strings() ||
        -1 == append_ast_expr(&writer, e, level))
    {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }
    return _PyUnicodeWriter_Finish(&writer);
}

PyObject *
_PyAST_ExprAsUnicode(expr_ty e)
{
    return expr_as_unicode(e, PR_TEST);
}
