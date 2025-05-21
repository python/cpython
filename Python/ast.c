/*
 * This file exposes PyAST_Validate interface to check the integrity
 * of the given abstract syntax tree (potentially constructed manually).
 */
#include "Python.h"
#include "pycore_ast.h"           // asdl_stmt_seq
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()

#include <stdbool.h>              // bool


#define ENTER_RECURSIVE() \
if (Py_EnterRecursiveCall(" during compilation")) { \
    return 0; \
}

#define LEAVE_RECURSIVE() Py_LeaveRecursiveCall();

static int validate_stmts(asdl_stmt_seq *);
static int validate_exprs(asdl_expr_seq *, expr_context_ty, int);
static int validate_patterns(asdl_pattern_seq *, int);
static int validate_type_params(asdl_type_param_seq *);
static int _validate_nonempty_seq(asdl_seq *, const char *, const char *);
static int validate_stmt(stmt_ty);
static int validate_expr(expr_ty, expr_context_ty);
static int validate_pattern(pattern_ty, int);
static int validate_typeparam(type_param_ty);

#define VALIDATE_POSITIONS(node) \
    if (node->lineno > node->end_lineno) { \
        PyErr_Format(PyExc_ValueError, \
                     "AST node line range (%d, %d) is not valid", \
                     node->lineno, node->end_lineno); \
        return 0; \
    } \
    if ((node->lineno < 0 && node->end_lineno != node->lineno) || \
        (node->col_offset < 0 && node->col_offset != node->end_col_offset)) { \
        PyErr_Format(PyExc_ValueError, \
                     "AST node column range (%d, %d) for line range (%d, %d) is not valid", \
                     node->col_offset, node->end_col_offset, node->lineno, node->end_lineno); \
        return 0; \
    } \
    if (node->lineno == node->end_lineno && node->col_offset > node->end_col_offset) { \
        PyErr_Format(PyExc_ValueError, \
                     "line %d, column %d-%d is not a valid range", \
                     node->lineno, node->col_offset, node->end_col_offset); \
        return 0; \
    }

static int
validate_name(PyObject *name)
{
    assert(!PyErr_Occurred());
    assert(PyUnicode_Check(name));
    static const char * const forbidden[] = {
        "None",
        "True",
        "False",
        NULL
    };
    for (int i = 0; forbidden[i] != NULL; i++) {
        if (_PyUnicode_EqualToASCIIString(name, forbidden[i])) {
            PyErr_Format(PyExc_ValueError, "identifier field can't represent '%s' constant", forbidden[i]);
            return 0;
        }
    }
    return 1;
}

static int
validate_comprehension(asdl_comprehension_seq *gens)
{
    assert(!PyErr_Occurred());
    if (!asdl_seq_LEN(gens)) {
        PyErr_SetString(PyExc_ValueError, "comprehension with no generators");
        return 0;
    }
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(gens); i++) {
        comprehension_ty comp = asdl_seq_GET(gens, i);
        if (!validate_expr(comp->target, Store) ||
            !validate_expr(comp->iter, Load) ||
            !validate_exprs(comp->ifs, Load, 0))
            return 0;
    }
    return 1;
}

static int
validate_keywords(asdl_keyword_seq *keywords)
{
    assert(!PyErr_Occurred());
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(keywords); i++)
        if (!validate_expr((asdl_seq_GET(keywords, i))->value, Load))
            return 0;
    return 1;
}

static int
validate_args(asdl_arg_seq *args)
{
    assert(!PyErr_Occurred());
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = asdl_seq_GET(args, i);
        VALIDATE_POSITIONS(arg);
        if (arg->annotation && !validate_expr(arg->annotation, Load))
            return 0;
    }
    return 1;
}

static const char *
expr_context_name(expr_context_ty ctx)
{
    switch (ctx) {
    case Load:
        return "Load";
    case Store:
        return "Store";
    case Del:
        return "Del";
    // No default case so compiler emits warning for unhandled cases
    }
    Py_UNREACHABLE();
}

static int
validate_arguments(arguments_ty args)
{
    assert(!PyErr_Occurred());
    if (!validate_args(args->posonlyargs) || !validate_args(args->args)) {
        return 0;
    }
    if (args->vararg && args->vararg->annotation
        && !validate_expr(args->vararg->annotation, Load)) {
            return 0;
    }
    if (!validate_args(args->kwonlyargs))
        return 0;
    if (args->kwarg && args->kwarg->annotation
        && !validate_expr(args->kwarg->annotation, Load)) {
            return 0;
    }
    if (asdl_seq_LEN(args->defaults) > asdl_seq_LEN(args->posonlyargs) + asdl_seq_LEN(args->args)) {
        PyErr_SetString(PyExc_ValueError, "more positional defaults than args on arguments");
        return 0;
    }
    if (asdl_seq_LEN(args->kw_defaults) != asdl_seq_LEN(args->kwonlyargs)) {
        PyErr_SetString(PyExc_ValueError, "length of kwonlyargs is not the same as "
                        "kw_defaults on arguments");
        return 0;
    }
    return validate_exprs(args->defaults, Load, 0) && validate_exprs(args->kw_defaults, Load, 1);
}

static int
validate_constant(PyObject *value)
{
    assert(!PyErr_Occurred());
    if (value == Py_None || value == Py_Ellipsis)
        return 1;

    if (PyLong_CheckExact(value)
            || PyFloat_CheckExact(value)
            || PyComplex_CheckExact(value)
            || PyBool_Check(value)
            || PyUnicode_CheckExact(value)
            || PyBytes_CheckExact(value))
        return 1;

    if (PyTuple_CheckExact(value) || PyFrozenSet_CheckExact(value)) {
        ENTER_RECURSIVE();

        PyObject *it = PyObject_GetIter(value);
        if (it == NULL)
            return 0;

        while (1) {
            PyObject *item = PyIter_Next(it);
            if (item == NULL) {
                if (PyErr_Occurred()) {
                    Py_DECREF(it);
                    return 0;
                }
                break;
            }

            if (!validate_constant(item)) {
                Py_DECREF(it);
                Py_DECREF(item);
                return 0;
            }
            Py_DECREF(item);
        }

        Py_DECREF(it);
        LEAVE_RECURSIVE();
        return 1;
    }

    if (!PyErr_Occurred()) {
        PyErr_Format(PyExc_TypeError,
                     "got an invalid type in Constant: %s",
                     _PyType_Name(Py_TYPE(value)));
    }
    return 0;
}

static int
validate_expr(expr_ty exp, expr_context_ty ctx)
{
    assert(!PyErr_Occurred());
    VALIDATE_POSITIONS(exp);
    int ret = -1;
    ENTER_RECURSIVE();
    int check_ctx = 1;
    expr_context_ty actual_ctx;

    /* First check expression context. */
    switch (exp->kind) {
    case Attribute_kind:
        actual_ctx = exp->v.Attribute.ctx;
        break;
    case Subscript_kind:
        actual_ctx = exp->v.Subscript.ctx;
        break;
    case Starred_kind:
        actual_ctx = exp->v.Starred.ctx;
        break;
    case Name_kind:
        if (!validate_name(exp->v.Name.id)) {
            return 0;
        }
        actual_ctx = exp->v.Name.ctx;
        break;
    case List_kind:
        actual_ctx = exp->v.List.ctx;
        break;
    case Tuple_kind:
        actual_ctx = exp->v.Tuple.ctx;
        break;
    default:
        if (ctx != Load) {
            PyErr_Format(PyExc_ValueError, "expression which can't be "
                         "assigned to in %s context", expr_context_name(ctx));
            return 0;
        }
        check_ctx = 0;
        /* set actual_ctx to prevent gcc warning */
        actual_ctx = 0;
    }
    if (check_ctx && actual_ctx != ctx) {
        PyErr_Format(PyExc_ValueError, "expression must have %s context but has %s instead",
                     expr_context_name(ctx), expr_context_name(actual_ctx));
        return 0;
    }

    /* Now validate expression. */
    switch (exp->kind) {
    case BoolOp_kind:
        if (asdl_seq_LEN(exp->v.BoolOp.values) < 2) {
            PyErr_SetString(PyExc_ValueError, "BoolOp with less than 2 values");
            return 0;
        }
        ret = validate_exprs(exp->v.BoolOp.values, Load, 0);
        break;
    case BinOp_kind:
        ret = validate_expr(exp->v.BinOp.left, Load) &&
            validate_expr(exp->v.BinOp.right, Load);
        break;
    case UnaryOp_kind:
        ret = validate_expr(exp->v.UnaryOp.operand, Load);
        break;
    case Lambda_kind:
        ret = validate_arguments(exp->v.Lambda.args) &&
            validate_expr(exp->v.Lambda.body, Load);
        break;
    case IfExp_kind:
        ret = validate_expr(exp->v.IfExp.test, Load) &&
            validate_expr(exp->v.IfExp.body, Load) &&
            validate_expr(exp->v.IfExp.orelse, Load);
        break;
    case Dict_kind:
        if (asdl_seq_LEN(exp->v.Dict.keys) != asdl_seq_LEN(exp->v.Dict.values)) {
            PyErr_SetString(PyExc_ValueError,
                            "Dict doesn't have the same number of keys as values");
            return 0;
        }
        /* null_ok=1 for keys expressions to allow dict unpacking to work in
           dict literals, i.e. ``{**{a:b}}`` */
        ret = validate_exprs(exp->v.Dict.keys, Load, /*null_ok=*/ 1) &&
            validate_exprs(exp->v.Dict.values, Load, /*null_ok=*/ 0);
        break;
    case Set_kind:
        ret = validate_exprs(exp->v.Set.elts, Load, 0);
        break;
#define COMP(NAME) \
        case NAME ## _kind: \
            ret = validate_comprehension(exp->v.NAME.generators) && \
                validate_expr(exp->v.NAME.elt, Load); \
            break;
    COMP(ListComp)
    COMP(SetComp)
    COMP(GeneratorExp)
#undef COMP
    case DictComp_kind:
        ret = validate_comprehension(exp->v.DictComp.generators) &&
            validate_expr(exp->v.DictComp.key, Load) &&
            validate_expr(exp->v.DictComp.value, Load);
        break;
    case Yield_kind:
        ret = !exp->v.Yield.value || validate_expr(exp->v.Yield.value, Load);
        break;
    case YieldFrom_kind:
        ret = validate_expr(exp->v.YieldFrom.value, Load);
        break;
    case Await_kind:
        ret = validate_expr(exp->v.Await.value, Load);
        break;
    case Compare_kind:
        if (!asdl_seq_LEN(exp->v.Compare.comparators)) {
            PyErr_SetString(PyExc_ValueError, "Compare with no comparators");
            return 0;
        }
        if (asdl_seq_LEN(exp->v.Compare.comparators) !=
            asdl_seq_LEN(exp->v.Compare.ops)) {
            PyErr_SetString(PyExc_ValueError, "Compare has a different number "
                            "of comparators and operands");
            return 0;
        }
        ret = validate_exprs(exp->v.Compare.comparators, Load, 0) &&
            validate_expr(exp->v.Compare.left, Load);
        break;
    case Call_kind:
        ret = validate_expr(exp->v.Call.func, Load) &&
            validate_exprs(exp->v.Call.args, Load, 0) &&
            validate_keywords(exp->v.Call.keywords);
        break;
    case Constant_kind:
        if (!validate_constant(exp->v.Constant.value)) {
            return 0;
        }
        ret = 1;
        break;
    case JoinedStr_kind:
        ret = validate_exprs(exp->v.JoinedStr.values, Load, 0);
        break;
    case TemplateStr_kind:
        ret = validate_exprs(exp->v.TemplateStr.values, Load, 0);
        break;
    case FormattedValue_kind:
        if (validate_expr(exp->v.FormattedValue.value, Load) == 0)
            return 0;
        if (exp->v.FormattedValue.format_spec) {
            ret = validate_expr(exp->v.FormattedValue.format_spec, Load);
            break;
        }
        ret = 1;
        break;
    case Interpolation_kind:
        if (validate_expr(exp->v.Interpolation.value, Load) == 0)
            return 0;
        if (exp->v.Interpolation.format_spec) {
            ret = validate_expr(exp->v.Interpolation.format_spec, Load);
            break;
        }
        ret = 1;
        break;
    case Attribute_kind:
        ret = validate_expr(exp->v.Attribute.value, Load);
        break;
    case Subscript_kind:
        ret = validate_expr(exp->v.Subscript.slice, Load) &&
            validate_expr(exp->v.Subscript.value, Load);
        break;
    case Starred_kind:
        ret = validate_expr(exp->v.Starred.value, ctx);
        break;
    case Slice_kind:
        ret = (!exp->v.Slice.lower || validate_expr(exp->v.Slice.lower, Load)) &&
            (!exp->v.Slice.upper || validate_expr(exp->v.Slice.upper, Load)) &&
            (!exp->v.Slice.step || validate_expr(exp->v.Slice.step, Load));
        break;
    case List_kind:
        ret = validate_exprs(exp->v.List.elts, ctx, 0);
        break;
    case Tuple_kind:
        ret = validate_exprs(exp->v.Tuple.elts, ctx, 0);
        break;
    case NamedExpr_kind:
        if (exp->v.NamedExpr.target->kind != Name_kind) {
            PyErr_SetString(PyExc_TypeError,
                            "NamedExpr target must be a Name");
            return 0;
        }
        ret = validate_expr(exp->v.NamedExpr.value, Load);
        break;
    /* This last case doesn't have any checking. */
    case Name_kind:
        ret = 1;
        break;
    // No default case so compiler emits warning for unhandled cases
    }
    if (ret < 0) {
        PyErr_SetString(PyExc_SystemError, "unexpected expression");
        ret = 0;
    }
    LEAVE_RECURSIVE();
    return ret;
}


// Note: the ensure_literal_* functions are only used to validate a restricted
//       set of non-recursive literals that have already been checked with
//       validate_expr, so they don't accept the validator state
static int
ensure_literal_number(expr_ty exp, bool allow_real, bool allow_imaginary)
{
    assert(exp->kind == Constant_kind);
    PyObject *value = exp->v.Constant.value;
    return (allow_real && PyFloat_CheckExact(value)) ||
           (allow_real && PyLong_CheckExact(value)) ||
           (allow_imaginary && PyComplex_CheckExact(value));
}

static int
ensure_literal_negative(expr_ty exp, bool allow_real, bool allow_imaginary)
{
    assert(exp->kind == UnaryOp_kind);
    // Must be negation ...
    if (exp->v.UnaryOp.op != USub) {
        return 0;
    }
    // ... of a constant ...
    expr_ty operand = exp->v.UnaryOp.operand;
    if (operand->kind != Constant_kind) {
        return 0;
    }
    // ... number
    return ensure_literal_number(operand, allow_real, allow_imaginary);
}

static int
ensure_literal_complex(expr_ty exp)
{
    assert(exp->kind == BinOp_kind);
    expr_ty left = exp->v.BinOp.left;
    expr_ty right = exp->v.BinOp.right;
    // Ensure op is addition or subtraction
    if (exp->v.BinOp.op != Add && exp->v.BinOp.op != Sub) {
        return 0;
    }
    // Check LHS is a real number (potentially signed)
    switch (left->kind)
    {
        case Constant_kind:
            if (!ensure_literal_number(left, /*real=*/true, /*imaginary=*/false)) {
                return 0;
            }
            break;
        case UnaryOp_kind:
            if (!ensure_literal_negative(left, /*real=*/true, /*imaginary=*/false)) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    // Check RHS is an imaginary number (no separate sign allowed)
    switch (right->kind)
    {
        case Constant_kind:
            if (!ensure_literal_number(right, /*real=*/false, /*imaginary=*/true)) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

static int
validate_pattern_match_value(expr_ty exp)
{
    assert(!PyErr_Occurred());
    if (!validate_expr(exp, Load)) {
        return 0;
    }

    switch (exp->kind)
    {
        case Constant_kind:
            /* Ellipsis and immutable sequences are not allowed.
               For True, False and None, MatchSingleton() should
               be used */
            if (!validate_expr(exp, Load)) {
                return 0;
            }
            PyObject *literal = exp->v.Constant.value;
            if (PyLong_CheckExact(literal) || PyFloat_CheckExact(literal) ||
                PyBytes_CheckExact(literal) || PyComplex_CheckExact(literal) ||
                PyUnicode_CheckExact(literal)) {
                return 1;
            }
            PyErr_SetString(PyExc_ValueError,
                            "unexpected constant inside of a literal pattern");
            return 0;
        case Attribute_kind:
            // Constants and attribute lookups are always permitted
            return 1;
        case UnaryOp_kind:
            // Negated numbers are permitted (whether real or imaginary)
            // Compiler will complain if AST folding doesn't create a constant
            if (ensure_literal_negative(exp, /*real=*/true, /*imaginary=*/true)) {
                return 1;
            }
            break;
        case BinOp_kind:
            // Complex literals are permitted
            // Compiler will complain if AST folding doesn't create a constant
            if (ensure_literal_complex(exp)) {
                return 1;
            }
            break;
        case JoinedStr_kind:
        case TemplateStr_kind:
            // Handled in the later stages
            return 1;
        default:
            break;
    }
    PyErr_SetString(PyExc_ValueError,
                    "patterns may only match literals and attribute lookups");
    return 0;
}

static int
validate_capture(PyObject *name)
{
    assert(!PyErr_Occurred());
    if (_PyUnicode_EqualToASCIIString(name, "_")) {
        PyErr_Format(PyExc_ValueError, "can't capture name '_' in patterns");
        return 0;
    }
    return validate_name(name);
}

static int
validate_pattern(pattern_ty p, int star_ok)
{
    assert(!PyErr_Occurred());
    VALIDATE_POSITIONS(p);
    int ret = -1;
    ENTER_RECURSIVE();
    switch (p->kind) {
        case MatchValue_kind:
            ret = validate_pattern_match_value(p->v.MatchValue.value);
            break;
        case MatchSingleton_kind:
            ret = p->v.MatchSingleton.value == Py_None || PyBool_Check(p->v.MatchSingleton.value);
            if (!ret) {
                PyErr_SetString(PyExc_ValueError,
                                "MatchSingleton can only contain True, False and None");
            }
            break;
        case MatchSequence_kind:
            ret = validate_patterns(p->v.MatchSequence.patterns, /*star_ok=*/1);
            break;
        case MatchMapping_kind:
            if (asdl_seq_LEN(p->v.MatchMapping.keys) != asdl_seq_LEN(p->v.MatchMapping.patterns)) {
                PyErr_SetString(PyExc_ValueError,
                                "MatchMapping doesn't have the same number of keys as patterns");
                ret = 0;
                break;
            }

            if (p->v.MatchMapping.rest && !validate_capture(p->v.MatchMapping.rest)) {
                ret = 0;
                break;
            }

            asdl_expr_seq *keys = p->v.MatchMapping.keys;
            for (Py_ssize_t i = 0; i < asdl_seq_LEN(keys); i++) {
                expr_ty key = asdl_seq_GET(keys, i);
                if (key->kind == Constant_kind) {
                    PyObject *literal = key->v.Constant.value;
                    if (literal == Py_None || PyBool_Check(literal)) {
                        /* validate_pattern_match_value will ensure the key
                           doesn't contain True, False and None but it is
                           syntactically valid, so we will pass those on in
                           a special case. */
                        continue;
                    }
                }
                if (!validate_pattern_match_value(key)) {
                    ret = 0;
                    break;
                }
            }
            if (ret == 0) {
                break;
            }
            ret = validate_patterns(p->v.MatchMapping.patterns, /*star_ok=*/0);
            break;
        case MatchClass_kind:
            if (asdl_seq_LEN(p->v.MatchClass.kwd_attrs) != asdl_seq_LEN(p->v.MatchClass.kwd_patterns)) {
                PyErr_SetString(PyExc_ValueError,
                                "MatchClass doesn't have the same number of keyword attributes as patterns");
                ret = 0;
                break;
            }
            if (!validate_expr(p->v.MatchClass.cls, Load)) {
                ret = 0;
                break;
            }

            expr_ty cls = p->v.MatchClass.cls;
            while (1) {
                if (cls->kind == Name_kind) {
                    break;
                }
                else if (cls->kind == Attribute_kind) {
                    cls = cls->v.Attribute.value;
                    continue;
                }
                else {
                    PyErr_SetString(PyExc_ValueError,
                                    "MatchClass cls field can only contain Name or Attribute nodes.");
                    ret = 0;
                    break;
                }
            }
            if (ret == 0) {
                break;
            }

            for (Py_ssize_t i = 0; i < asdl_seq_LEN(p->v.MatchClass.kwd_attrs); i++) {
                PyObject *identifier = asdl_seq_GET(p->v.MatchClass.kwd_attrs, i);
                if (!validate_name(identifier)) {
                    ret = 0;
                    break;
                }
            }
            if (ret == 0) {
                break;
            }

            if (!validate_patterns(p->v.MatchClass.patterns, /*star_ok=*/0)) {
                ret = 0;
                break;
            }

            ret = validate_patterns(p->v.MatchClass.kwd_patterns, /*star_ok=*/0);
            break;
        case MatchStar_kind:
            if (!star_ok) {
                PyErr_SetString(PyExc_ValueError, "can't use MatchStar here");
                ret = 0;
                break;
            }
            ret = p->v.MatchStar.name == NULL || validate_capture(p->v.MatchStar.name);
            break;
        case MatchAs_kind:
            if (p->v.MatchAs.name && !validate_capture(p->v.MatchAs.name)) {
                ret = 0;
                break;
            }
            if (p->v.MatchAs.pattern == NULL) {
                ret = 1;
            }
            else if (p->v.MatchAs.name == NULL) {
                PyErr_SetString(PyExc_ValueError,
                                "MatchAs must specify a target name if a pattern is given");
                ret = 0;
            }
            else {
                ret = validate_pattern(p->v.MatchAs.pattern, /*star_ok=*/0);
            }
            break;
        case MatchOr_kind:
            if (asdl_seq_LEN(p->v.MatchOr.patterns) < 2) {
                PyErr_SetString(PyExc_ValueError,
                                "MatchOr requires at least 2 patterns");
                ret = 0;
                break;
            }
            ret = validate_patterns(p->v.MatchOr.patterns, /*star_ok=*/0);
            break;
    // No default case, so the compiler will emit a warning if new pattern
    // kinds are added without being handled here
    }
    if (ret < 0) {
        PyErr_SetString(PyExc_SystemError, "unexpected pattern");
        ret = 0;
    }
    LEAVE_RECURSIVE();
    return ret;
}

static int
_validate_nonempty_seq(asdl_seq *seq, const char *what, const char *owner)
{
    if (asdl_seq_LEN(seq))
        return 1;
    PyErr_Format(PyExc_ValueError, "empty %s on %s", what, owner);
    return 0;
}
#define validate_nonempty_seq(seq, what, owner) _validate_nonempty_seq((asdl_seq*)seq, what, owner)

static int
validate_assignlist(asdl_expr_seq *targets, expr_context_ty ctx)
{
    assert(!PyErr_Occurred());
    return validate_nonempty_seq(targets, "targets", ctx == Del ? "Delete" : "Assign") &&
        validate_exprs(targets, ctx, 0);
}

static int
validate_body(asdl_stmt_seq *body, const char *owner)
{
    assert(!PyErr_Occurred());
    return validate_nonempty_seq(body, "body", owner) && validate_stmts(body);
}

static int
validate_stmt(stmt_ty stmt)
{
    assert(!PyErr_Occurred());
    VALIDATE_POSITIONS(stmt);
    int ret = -1;
    ENTER_RECURSIVE();
    switch (stmt->kind) {
    case FunctionDef_kind:
        ret = validate_body(stmt->v.FunctionDef.body, "FunctionDef") &&
            validate_type_params(stmt->v.FunctionDef.type_params) &&
            validate_arguments(stmt->v.FunctionDef.args) &&
            validate_exprs(stmt->v.FunctionDef.decorator_list, Load, 0) &&
            (!stmt->v.FunctionDef.returns ||
             validate_expr(stmt->v.FunctionDef.returns, Load));
        break;
    case ClassDef_kind:
        ret = validate_body(stmt->v.ClassDef.body, "ClassDef") &&
            validate_type_params(stmt->v.ClassDef.type_params) &&
            validate_exprs(stmt->v.ClassDef.bases, Load, 0) &&
            validate_keywords(stmt->v.ClassDef.keywords) &&
            validate_exprs(stmt->v.ClassDef.decorator_list, Load, 0);
        break;
    case Return_kind:
        ret = !stmt->v.Return.value || validate_expr(stmt->v.Return.value, Load);
        break;
    case Delete_kind:
        ret = validate_assignlist(stmt->v.Delete.targets, Del);
        break;
    case Assign_kind:
        ret = validate_assignlist(stmt->v.Assign.targets, Store) &&
            validate_expr(stmt->v.Assign.value, Load);
        break;
    case AugAssign_kind:
        ret = validate_expr(stmt->v.AugAssign.target, Store) &&
            validate_expr(stmt->v.AugAssign.value, Load);
        break;
    case AnnAssign_kind:
        if (stmt->v.AnnAssign.target->kind != Name_kind &&
            stmt->v.AnnAssign.simple) {
            PyErr_SetString(PyExc_TypeError,
                            "AnnAssign with simple non-Name target");
            return 0;
        }
        ret = validate_expr(stmt->v.AnnAssign.target, Store) &&
               (!stmt->v.AnnAssign.value ||
                validate_expr(stmt->v.AnnAssign.value, Load)) &&
               validate_expr(stmt->v.AnnAssign.annotation, Load);
        break;
    case TypeAlias_kind:
        if (stmt->v.TypeAlias.name->kind != Name_kind) {
            PyErr_SetString(PyExc_TypeError,
                            "TypeAlias with non-Name name");
            return 0;
        }
        ret = validate_expr(stmt->v.TypeAlias.name, Store) &&
            validate_type_params(stmt->v.TypeAlias.type_params) &&
            validate_expr(stmt->v.TypeAlias.value, Load);
        break;
    case For_kind:
        ret = validate_expr(stmt->v.For.target, Store) &&
            validate_expr(stmt->v.For.iter, Load) &&
            validate_body(stmt->v.For.body, "For") &&
            validate_stmts(stmt->v.For.orelse);
        break;
    case AsyncFor_kind:
        ret = validate_expr(stmt->v.AsyncFor.target, Store) &&
            validate_expr(stmt->v.AsyncFor.iter, Load) &&
            validate_body(stmt->v.AsyncFor.body, "AsyncFor") &&
            validate_stmts(stmt->v.AsyncFor.orelse);
        break;
    case While_kind:
        ret = validate_expr(stmt->v.While.test, Load) &&
            validate_body(stmt->v.While.body, "While") &&
            validate_stmts(stmt->v.While.orelse);
        break;
    case If_kind:
        ret = validate_expr(stmt->v.If.test, Load) &&
            validate_body(stmt->v.If.body, "If") &&
            validate_stmts(stmt->v.If.orelse);
        break;
    case With_kind:
        if (!validate_nonempty_seq(stmt->v.With.items, "items", "With"))
            return 0;
        for (Py_ssize_t i = 0; i < asdl_seq_LEN(stmt->v.With.items); i++) {
            withitem_ty item = asdl_seq_GET(stmt->v.With.items, i);
            if (!validate_expr(item->context_expr, Load) ||
                (item->optional_vars && !validate_expr(item->optional_vars, Store)))
                return 0;
        }
        ret = validate_body(stmt->v.With.body, "With");
        break;
    case AsyncWith_kind:
        if (!validate_nonempty_seq(stmt->v.AsyncWith.items, "items", "AsyncWith"))
            return 0;
        for (Py_ssize_t i = 0; i < asdl_seq_LEN(stmt->v.AsyncWith.items); i++) {
            withitem_ty item = asdl_seq_GET(stmt->v.AsyncWith.items, i);
            if (!validate_expr(item->context_expr, Load) ||
                (item->optional_vars && !validate_expr(item->optional_vars, Store)))
                return 0;
        }
        ret = validate_body(stmt->v.AsyncWith.body, "AsyncWith");
        break;
    case Match_kind:
        if (!validate_expr(stmt->v.Match.subject, Load)
            || !validate_nonempty_seq(stmt->v.Match.cases, "cases", "Match")) {
            return 0;
        }
        for (Py_ssize_t i = 0; i < asdl_seq_LEN(stmt->v.Match.cases); i++) {
            match_case_ty m = asdl_seq_GET(stmt->v.Match.cases, i);
            if (!validate_pattern(m->pattern, /*star_ok=*/0)
                || (m->guard && !validate_expr(m->guard, Load))
                || !validate_body(m->body, "match_case")) {
                return 0;
            }
        }
        ret = 1;
        break;
    case Raise_kind:
        if (stmt->v.Raise.exc) {
            ret = validate_expr(stmt->v.Raise.exc, Load) &&
                (!stmt->v.Raise.cause || validate_expr(stmt->v.Raise.cause, Load));
            break;
        }
        if (stmt->v.Raise.cause) {
            PyErr_SetString(PyExc_ValueError, "Raise with cause but no exception");
            return 0;
        }
        ret = 1;
        break;
    case Try_kind:
        if (!validate_body(stmt->v.Try.body, "Try"))
            return 0;
        if (!asdl_seq_LEN(stmt->v.Try.handlers) &&
            !asdl_seq_LEN(stmt->v.Try.finalbody)) {
            PyErr_SetString(PyExc_ValueError, "Try has neither except handlers nor finalbody");
            return 0;
        }
        if (!asdl_seq_LEN(stmt->v.Try.handlers) &&
            asdl_seq_LEN(stmt->v.Try.orelse)) {
            PyErr_SetString(PyExc_ValueError, "Try has orelse but no except handlers");
            return 0;
        }
        for (Py_ssize_t i = 0; i < asdl_seq_LEN(stmt->v.Try.handlers); i++) {
            excepthandler_ty handler = asdl_seq_GET(stmt->v.Try.handlers, i);
            VALIDATE_POSITIONS(handler);
            if ((handler->v.ExceptHandler.type &&
                 !validate_expr(handler->v.ExceptHandler.type, Load)) ||
                !validate_body(handler->v.ExceptHandler.body, "ExceptHandler"))
                return 0;
        }
        ret = (!asdl_seq_LEN(stmt->v.Try.finalbody) ||
                validate_stmts(stmt->v.Try.finalbody)) &&
            (!asdl_seq_LEN(stmt->v.Try.orelse) ||
             validate_stmts(stmt->v.Try.orelse));
        break;
    case TryStar_kind:
        if (!validate_body(stmt->v.TryStar.body, "TryStar"))
            return 0;
        if (!asdl_seq_LEN(stmt->v.TryStar.handlers) &&
            !asdl_seq_LEN(stmt->v.TryStar.finalbody)) {
            PyErr_SetString(PyExc_ValueError, "TryStar has neither except handlers nor finalbody");
            return 0;
        }
        if (!asdl_seq_LEN(stmt->v.TryStar.handlers) &&
            asdl_seq_LEN(stmt->v.TryStar.orelse)) {
            PyErr_SetString(PyExc_ValueError, "TryStar has orelse but no except handlers");
            return 0;
        }
        for (Py_ssize_t i = 0; i < asdl_seq_LEN(stmt->v.TryStar.handlers); i++) {
            excepthandler_ty handler = asdl_seq_GET(stmt->v.TryStar.handlers, i);
            if ((handler->v.ExceptHandler.type &&
                 !validate_expr(handler->v.ExceptHandler.type, Load)) ||
                !validate_body(handler->v.ExceptHandler.body, "ExceptHandler"))
                return 0;
        }
        ret = (!asdl_seq_LEN(stmt->v.TryStar.finalbody) ||
                validate_stmts(stmt->v.TryStar.finalbody)) &&
            (!asdl_seq_LEN(stmt->v.TryStar.orelse) ||
             validate_stmts(stmt->v.TryStar.orelse));
        break;
    case Assert_kind:
        ret = validate_expr(stmt->v.Assert.test, Load) &&
            (!stmt->v.Assert.msg || validate_expr(stmt->v.Assert.msg, Load));
        break;
    case Import_kind:
        ret = validate_nonempty_seq(stmt->v.Import.names, "names", "Import");
        break;
    case ImportFrom_kind:
        if (stmt->v.ImportFrom.level < 0) {
            PyErr_SetString(PyExc_ValueError, "Negative ImportFrom level");
            return 0;
        }
        ret = validate_nonempty_seq(stmt->v.ImportFrom.names, "names", "ImportFrom");
        break;
    case Global_kind:
        ret = validate_nonempty_seq(stmt->v.Global.names, "names", "Global");
        break;
    case Nonlocal_kind:
        ret = validate_nonempty_seq(stmt->v.Nonlocal.names, "names", "Nonlocal");
        break;
    case Expr_kind:
        ret = validate_expr(stmt->v.Expr.value, Load);
        break;
    case AsyncFunctionDef_kind:
        ret = validate_body(stmt->v.AsyncFunctionDef.body, "AsyncFunctionDef") &&
            validate_type_params(stmt->v.AsyncFunctionDef.type_params) &&
            validate_arguments(stmt->v.AsyncFunctionDef.args) &&
            validate_exprs(stmt->v.AsyncFunctionDef.decorator_list, Load, 0) &&
            (!stmt->v.AsyncFunctionDef.returns ||
             validate_expr(stmt->v.AsyncFunctionDef.returns, Load));
        break;
    case Pass_kind:
    case Break_kind:
    case Continue_kind:
        ret = 1;
        break;
    // No default case so compiler emits warning for unhandled cases
    }
    if (ret < 0) {
        PyErr_SetString(PyExc_SystemError, "unexpected statement");
        ret = 0;
    }
    LEAVE_RECURSIVE();
    return ret;
}

static int
validate_stmts(asdl_stmt_seq *seq)
{
    assert(!PyErr_Occurred());
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(seq); i++) {
        stmt_ty stmt = asdl_seq_GET(seq, i);
        if (stmt) {
            if (!validate_stmt(stmt))
                return 0;
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                            "None disallowed in statement list");
            return 0;
        }
    }
    return 1;
}

static int
validate_exprs(asdl_expr_seq *exprs, expr_context_ty ctx, int null_ok)
{
    assert(!PyErr_Occurred());
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(exprs); i++) {
        expr_ty expr = asdl_seq_GET(exprs, i);
        if (expr) {
            if (!validate_expr(expr, ctx))
                return 0;
        }
        else if (!null_ok) {
            PyErr_SetString(PyExc_ValueError,
                            "None disallowed in expression list");
            return 0;
        }

    }
    return 1;
}

static int
validate_patterns(asdl_pattern_seq *patterns, int star_ok)
{
    assert(!PyErr_Occurred());
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(patterns); i++) {
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        if (!validate_pattern(pattern, star_ok)) {
            return 0;
        }
    }
    return 1;
}

static int
validate_typeparam(type_param_ty tp)
{
    VALIDATE_POSITIONS(tp);
    int ret = -1;
    switch (tp->kind) {
        case TypeVar_kind:
            ret = validate_name(tp->v.TypeVar.name) &&
                (!tp->v.TypeVar.bound ||
                 validate_expr(tp->v.TypeVar.bound, Load)) &&
                (!tp->v.TypeVar.default_value ||
                 validate_expr(tp->v.TypeVar.default_value, Load));
            break;
        case ParamSpec_kind:
            ret = validate_name(tp->v.ParamSpec.name) &&
                (!tp->v.ParamSpec.default_value ||
                 validate_expr(tp->v.ParamSpec.default_value, Load));
            break;
        case TypeVarTuple_kind:
            ret = validate_name(tp->v.TypeVarTuple.name) &&
                (!tp->v.TypeVarTuple.default_value ||
                 validate_expr(tp->v.TypeVarTuple.default_value, Load));
            break;
    }
    return ret;
}

static int
validate_type_params(asdl_type_param_seq *tps)
{
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(tps); i++) {
        type_param_ty tp = asdl_seq_GET(tps, i);
        if (tp) {
            if (!validate_typeparam(tp))
                return 0;
        }
    }
    return 1;
}

int
_PyAST_Validate(mod_ty mod)
{
    assert(!PyErr_Occurred());
    int res = -1;

    switch (mod->kind) {
    case Module_kind:
        res = validate_stmts(mod->v.Module.body);
        break;
    case Interactive_kind:
        res = validate_stmts(mod->v.Interactive.body);
        break;
    case Expression_kind:
        res = validate_expr(mod->v.Expression.body, Load);
        break;
    case FunctionType_kind:
        res = validate_exprs(mod->v.FunctionType.argtypes, Load, /*null_ok=*/0) &&
              validate_expr(mod->v.FunctionType.returns, Load);
        break;
    // No default case so compiler emits warning for unhandled cases
    }

    if (res < 0) {
        PyErr_SetString(PyExc_SystemError, "impossible module node");
        return 0;
    }
    return res;
}

PyObject *
_PyAST_GetDocString(asdl_stmt_seq *body)
{
    if (!asdl_seq_LEN(body)) {
        return NULL;
    }
    stmt_ty st = asdl_seq_GET(body, 0);
    if (st->kind != Expr_kind) {
        return NULL;
    }
    expr_ty e = st->v.Expr.value;
    if (e->kind == Constant_kind && PyUnicode_CheckExact(e->v.Constant.value)) {
        return e->v.Constant.value;
    }
    return NULL;
}
