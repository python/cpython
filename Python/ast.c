/*
 * This file includes functions to transform a concrete syntax tree (CST) to
 * an abstract syntax tree (AST). The main function is PyAST_FromNode().
 *
 */
#include "Python.h"
#include "Python-ast.h"
#include "ast.h"
#include "token.h"
#include "pythonrun.h"

#include <assert.h>
#include <stdbool.h>

#define MAXLEVEL 200    /* Max parentheses level */

static int validate_stmts(asdl_stmt_seq *);
static int validate_exprs(asdl_expr_seq*, expr_context_ty, int);
static int _validate_nonempty_seq(asdl_seq *, const char *, const char *);
static int validate_stmt(stmt_ty);
static int validate_expr(expr_ty, expr_context_ty);

static int
validate_name(PyObject *name)
{
    assert(PyUnicode_Check(name));
    static const char * const forbidden[] = {
        "None",
        "True",
        "False",
        NULL
    };
    for (int i = 0; forbidden[i] != NULL; i++) {
        if (_PyUnicode_EqualToASCIIString(name, forbidden[i])) {
            PyErr_Format(PyExc_ValueError, "Name node can't be used with '%s' constant", forbidden[i]);
            return 0;
        }
    }
    return 1;
}

static int
validate_comprehension(asdl_comprehension_seq *gens)
{
    Py_ssize_t i;
    if (!asdl_seq_LEN(gens)) {
        PyErr_SetString(PyExc_ValueError, "comprehension with no generators");
        return 0;
    }
    for (i = 0; i < asdl_seq_LEN(gens); i++) {
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
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(keywords); i++)
        if (!validate_expr(((keyword_ty)asdl_seq_GET(keywords, i))->value, Load))
            return 0;
    return 1;
}

static int
validate_args(asdl_arg_seq *args)
{
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = asdl_seq_GET(args, i);
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
    default:
        Py_UNREACHABLE();
    }
}

static int
validate_arguments(arguments_ty args)
{
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
        PyObject *it;

        it = PyObject_GetIter(value);
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
        return validate_exprs(exp->v.BoolOp.values, Load, 0);
    case BinOp_kind:
        return validate_expr(exp->v.BinOp.left, Load) &&
            validate_expr(exp->v.BinOp.right, Load);
    case UnaryOp_kind:
        return validate_expr(exp->v.UnaryOp.operand, Load);
    case Lambda_kind:
        return validate_arguments(exp->v.Lambda.args) &&
            validate_expr(exp->v.Lambda.body, Load);
    case IfExp_kind:
        return validate_expr(exp->v.IfExp.test, Load) &&
            validate_expr(exp->v.IfExp.body, Load) &&
            validate_expr(exp->v.IfExp.orelse, Load);
    case Dict_kind:
        if (asdl_seq_LEN(exp->v.Dict.keys) != asdl_seq_LEN(exp->v.Dict.values)) {
            PyErr_SetString(PyExc_ValueError,
                            "Dict doesn't have the same number of keys as values");
            return 0;
        }
        /* null_ok=1 for keys expressions to allow dict unpacking to work in
           dict literals, i.e. ``{**{a:b}}`` */
        return validate_exprs(exp->v.Dict.keys, Load, /*null_ok=*/ 1) &&
            validate_exprs(exp->v.Dict.values, Load, /*null_ok=*/ 0);
    case Set_kind:
        return validate_exprs(exp->v.Set.elts, Load, 0);
#define COMP(NAME) \
        case NAME ## _kind: \
            return validate_comprehension(exp->v.NAME.generators) && \
                validate_expr(exp->v.NAME.elt, Load);
    COMP(ListComp)
    COMP(SetComp)
    COMP(GeneratorExp)
#undef COMP
    case DictComp_kind:
        return validate_comprehension(exp->v.DictComp.generators) &&
            validate_expr(exp->v.DictComp.key, Load) &&
            validate_expr(exp->v.DictComp.value, Load);
    case Yield_kind:
        return !exp->v.Yield.value || validate_expr(exp->v.Yield.value, Load);
    case YieldFrom_kind:
        return validate_expr(exp->v.YieldFrom.value, Load);
    case Await_kind:
        return validate_expr(exp->v.Await.value, Load);
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
        return validate_exprs(exp->v.Compare.comparators, Load, 0) &&
            validate_expr(exp->v.Compare.left, Load);
    case Call_kind:
        return validate_expr(exp->v.Call.func, Load) &&
            validate_exprs(exp->v.Call.args, Load, 0) &&
            validate_keywords(exp->v.Call.keywords);
    case Constant_kind:
        if (!validate_constant(exp->v.Constant.value)) {
            return 0;
        }
        return 1;
    case JoinedStr_kind:
        return validate_exprs(exp->v.JoinedStr.values, Load, 0);
    case FormattedValue_kind:
        if (validate_expr(exp->v.FormattedValue.value, Load) == 0)
            return 0;
        if (exp->v.FormattedValue.format_spec)
            return validate_expr(exp->v.FormattedValue.format_spec, Load);
        return 1;
    case Attribute_kind:
        return validate_expr(exp->v.Attribute.value, Load);
    case Subscript_kind:
        return validate_expr(exp->v.Subscript.slice, Load) &&
            validate_expr(exp->v.Subscript.value, Load);
    case Starred_kind:
        return validate_expr(exp->v.Starred.value, ctx);
    case Slice_kind:
        return (!exp->v.Slice.lower || validate_expr(exp->v.Slice.lower, Load)) &&
            (!exp->v.Slice.upper || validate_expr(exp->v.Slice.upper, Load)) &&
            (!exp->v.Slice.step || validate_expr(exp->v.Slice.step, Load));
    case List_kind:
        return validate_exprs(exp->v.List.elts, ctx, 0);
    case Tuple_kind:
        return validate_exprs(exp->v.Tuple.elts, ctx, 0);
    case NamedExpr_kind:
        return validate_expr(exp->v.NamedExpr.value, Load);
    /* This last case doesn't have any checking. */
    case Name_kind:
        return 1;
    }
    PyErr_SetString(PyExc_SystemError, "unexpected expression");
    return 0;
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
    return validate_nonempty_seq(targets, "targets", ctx == Del ? "Delete" : "Assign") &&
        validate_exprs(targets, ctx, 0);
}

static int
validate_body(asdl_stmt_seq *body, const char *owner)
{
    return validate_nonempty_seq(body, "body", owner) && validate_stmts(body);
}

static int
validate_stmt(stmt_ty stmt)
{
    Py_ssize_t i;
    switch (stmt->kind) {
    case FunctionDef_kind:
        return validate_body(stmt->v.FunctionDef.body, "FunctionDef") &&
            validate_arguments(stmt->v.FunctionDef.args) &&
            validate_exprs(stmt->v.FunctionDef.decorator_list, Load, 0) &&
            (!stmt->v.FunctionDef.returns ||
             validate_expr(stmt->v.FunctionDef.returns, Load));
    case ClassDef_kind:
        return validate_body(stmt->v.ClassDef.body, "ClassDef") &&
            validate_exprs(stmt->v.ClassDef.bases, Load, 0) &&
            validate_keywords(stmt->v.ClassDef.keywords) &&
            validate_exprs(stmt->v.ClassDef.decorator_list, Load, 0);
    case Return_kind:
        return !stmt->v.Return.value || validate_expr(stmt->v.Return.value, Load);
    case Delete_kind:
        return validate_assignlist(stmt->v.Delete.targets, Del);
    case Assign_kind:
        return validate_assignlist(stmt->v.Assign.targets, Store) &&
            validate_expr(stmt->v.Assign.value, Load);
    case AugAssign_kind:
        return validate_expr(stmt->v.AugAssign.target, Store) &&
            validate_expr(stmt->v.AugAssign.value, Load);
    case AnnAssign_kind:
        if (stmt->v.AnnAssign.target->kind != Name_kind &&
            stmt->v.AnnAssign.simple) {
            PyErr_SetString(PyExc_TypeError,
                            "AnnAssign with simple non-Name target");
            return 0;
        }
        return validate_expr(stmt->v.AnnAssign.target, Store) &&
               (!stmt->v.AnnAssign.value ||
                validate_expr(stmt->v.AnnAssign.value, Load)) &&
               validate_expr(stmt->v.AnnAssign.annotation, Load);
    case For_kind:
        return validate_expr(stmt->v.For.target, Store) &&
            validate_expr(stmt->v.For.iter, Load) &&
            validate_body(stmt->v.For.body, "For") &&
            validate_stmts(stmt->v.For.orelse);
    case AsyncFor_kind:
        return validate_expr(stmt->v.AsyncFor.target, Store) &&
            validate_expr(stmt->v.AsyncFor.iter, Load) &&
            validate_body(stmt->v.AsyncFor.body, "AsyncFor") &&
            validate_stmts(stmt->v.AsyncFor.orelse);
    case While_kind:
        return validate_expr(stmt->v.While.test, Load) &&
            validate_body(stmt->v.While.body, "While") &&
            validate_stmts(stmt->v.While.orelse);
    case If_kind:
        return validate_expr(stmt->v.If.test, Load) &&
            validate_body(stmt->v.If.body, "If") &&
            validate_stmts(stmt->v.If.orelse);
    case With_kind:
        if (!validate_nonempty_seq(stmt->v.With.items, "items", "With"))
            return 0;
        for (i = 0; i < asdl_seq_LEN(stmt->v.With.items); i++) {
            withitem_ty item = asdl_seq_GET(stmt->v.With.items, i);
            if (!validate_expr(item->context_expr, Load) ||
                (item->optional_vars && !validate_expr(item->optional_vars, Store)))
                return 0;
        }
        return validate_body(stmt->v.With.body, "With");
    case AsyncWith_kind:
        if (!validate_nonempty_seq(stmt->v.AsyncWith.items, "items", "AsyncWith"))
            return 0;
        for (i = 0; i < asdl_seq_LEN(stmt->v.AsyncWith.items); i++) {
            withitem_ty item = asdl_seq_GET(stmt->v.AsyncWith.items, i);
            if (!validate_expr(item->context_expr, Load) ||
                (item->optional_vars && !validate_expr(item->optional_vars, Store)))
                return 0;
        }
        return validate_body(stmt->v.AsyncWith.body, "AsyncWith");
    case Raise_kind:
        if (stmt->v.Raise.exc) {
            return validate_expr(stmt->v.Raise.exc, Load) &&
                (!stmt->v.Raise.cause || validate_expr(stmt->v.Raise.cause, Load));
        }
        if (stmt->v.Raise.cause) {
            PyErr_SetString(PyExc_ValueError, "Raise with cause but no exception");
            return 0;
        }
        return 1;
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
        for (i = 0; i < asdl_seq_LEN(stmt->v.Try.handlers); i++) {
            excepthandler_ty handler = asdl_seq_GET(stmt->v.Try.handlers, i);
            if ((handler->v.ExceptHandler.type &&
                 !validate_expr(handler->v.ExceptHandler.type, Load)) ||
                !validate_body(handler->v.ExceptHandler.body, "ExceptHandler"))
                return 0;
        }
        return (!asdl_seq_LEN(stmt->v.Try.finalbody) ||
                validate_stmts(stmt->v.Try.finalbody)) &&
            (!asdl_seq_LEN(stmt->v.Try.orelse) ||
             validate_stmts(stmt->v.Try.orelse));
    case Assert_kind:
        return validate_expr(stmt->v.Assert.test, Load) &&
            (!stmt->v.Assert.msg || validate_expr(stmt->v.Assert.msg, Load));
    case Import_kind:
        return validate_nonempty_seq(stmt->v.Import.names, "names", "Import");
    case ImportFrom_kind:
        if (stmt->v.ImportFrom.level < 0) {
            PyErr_SetString(PyExc_ValueError, "Negative ImportFrom level");
            return 0;
        }
        return validate_nonempty_seq(stmt->v.ImportFrom.names, "names", "ImportFrom");
    case Global_kind:
        return validate_nonempty_seq(stmt->v.Global.names, "names", "Global");
    case Nonlocal_kind:
        return validate_nonempty_seq(stmt->v.Nonlocal.names, "names", "Nonlocal");
    case Expr_kind:
        return validate_expr(stmt->v.Expr.value, Load);
    case AsyncFunctionDef_kind:
        return validate_body(stmt->v.AsyncFunctionDef.body, "AsyncFunctionDef") &&
            validate_arguments(stmt->v.AsyncFunctionDef.args) &&
            validate_exprs(stmt->v.AsyncFunctionDef.decorator_list, Load, 0) &&
            (!stmt->v.AsyncFunctionDef.returns ||
             validate_expr(stmt->v.AsyncFunctionDef.returns, Load));
    case Pass_kind:
    case Break_kind:
    case Continue_kind:
        return 1;
    default:
        PyErr_SetString(PyExc_SystemError, "unexpected statement");
        return 0;
    }
}

static int
validate_stmts(asdl_stmt_seq *seq)
{
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(seq); i++) {
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
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(exprs); i++) {
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

int
PyAST_Validate(mod_ty mod)
{
    int res = 0;

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
    default:
        PyErr_SetString(PyExc_SystemError, "impossible module node");
        res = 0;
        break;
    }
    return res;
}

PyObject *
_PyAST_GetDocString(asdl_stmt_seq *body)
{
    if (!asdl_seq_LEN(body)) {
        return NULL;
    }
    stmt_ty st = (stmt_ty)asdl_seq_GET(body, 0);
    if (st->kind != Expr_kind) {
        return NULL;
    }
    expr_ty e = st->v.Expr.value;
    if (e->kind == Constant_kind && PyUnicode_CheckExact(e->v.Constant.value)) {
        return e->v.Constant.value;
    }
    return NULL;
}
