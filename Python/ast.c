/*
 * This file exposes PyAST_Validate interface to check the integrity
 * of the given abstract syntax tree (potentially constructed manually).
 */
#include "Python.h"
#include "pycore_ast.h"           // asdl_stmt_seq
#include "pycore_pystate.h"       // _PyThreadState_GET()

#include <assert.h>

struct validator {
    int recursion_depth;            /* current recursion depth */
    int recursion_limit;            /* recursion limit */
};

static int validate_stmts(struct validator *, asdl_stmt_seq *);
static int validate_exprs(struct validator *, asdl_expr_seq*, expr_context_ty, int);
static int _validate_nonempty_seq(asdl_seq *, const char *, const char *);
static int validate_stmt(struct validator *, stmt_ty);
static int validate_expr(struct validator *, expr_ty, expr_context_ty);

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
validate_comprehension(struct validator *state, asdl_comprehension_seq *gens)
{
    Py_ssize_t i;
    if (!asdl_seq_LEN(gens)) {
        PyErr_SetString(PyExc_ValueError, "comprehension with no generators");
        return 0;
    }
    for (i = 0; i < asdl_seq_LEN(gens); i++) {
        comprehension_ty comp = asdl_seq_GET(gens, i);
        if (!validate_expr(state, comp->target, Store) ||
            !validate_expr(state, comp->iter, Load) ||
            !validate_exprs(state, comp->ifs, Load, 0))
            return 0;
    }
    return 1;
}

static int
validate_keywords(struct validator *state, asdl_keyword_seq *keywords)
{
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(keywords); i++)
        if (!validate_expr(state, (asdl_seq_GET(keywords, i))->value, Load))
            return 0;
    return 1;
}

static int
validate_args(struct validator *state, asdl_arg_seq *args)
{
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = asdl_seq_GET(args, i);
        if (arg->annotation && !validate_expr(state, arg->annotation, Load))
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
validate_arguments(struct validator *state, arguments_ty args)
{
    if (!validate_args(state, args->posonlyargs) || !validate_args(state, args->args)) {
        return 0;
    }
    if (args->vararg && args->vararg->annotation
        && !validate_expr(state, args->vararg->annotation, Load)) {
            return 0;
    }
    if (!validate_args(state, args->kwonlyargs))
        return 0;
    if (args->kwarg && args->kwarg->annotation
        && !validate_expr(state, args->kwarg->annotation, Load)) {
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
    return validate_exprs(state, args->defaults, Load, 0) && validate_exprs(state, args->kw_defaults, Load, 1);
}

static int
validate_constant(struct validator *state, PyObject *value)
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
        if (++state->recursion_depth > state->recursion_limit) {
            PyErr_SetString(PyExc_RecursionError,
                            "maximum recursion depth exceeded during compilation");
            return 0;
        }

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

            if (!validate_constant(state, item)) {
                Py_DECREF(it);
                Py_DECREF(item);
                return 0;
            }
            Py_DECREF(item);
        }

        Py_DECREF(it);
        --state->recursion_depth;
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
validate_expr(struct validator *state, expr_ty exp, expr_context_ty ctx)
{
    int ret;
    if (++state->recursion_depth > state->recursion_limit) {
        PyErr_SetString(PyExc_RecursionError,
                        "maximum recursion depth exceeded during compilation");
        return 0;
    }
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
        ret = validate_exprs(state, exp->v.BoolOp.values, Load, 0);
        break;
    case BinOp_kind:
        ret = validate_expr(state, exp->v.BinOp.left, Load) &&
            validate_expr(state, exp->v.BinOp.right, Load);
        break;
    case UnaryOp_kind:
        ret = validate_expr(state, exp->v.UnaryOp.operand, Load);
        break;
    case Lambda_kind:
        ret = validate_arguments(state, exp->v.Lambda.args) &&
            validate_expr(state, exp->v.Lambda.body, Load);
        break;
    case IfExp_kind:
        ret = validate_expr(state, exp->v.IfExp.test, Load) &&
            validate_expr(state, exp->v.IfExp.body, Load) &&
            validate_expr(state, exp->v.IfExp.orelse, Load);
        break;
    case Dict_kind:
        if (asdl_seq_LEN(exp->v.Dict.keys) != asdl_seq_LEN(exp->v.Dict.values)) {
            PyErr_SetString(PyExc_ValueError,
                            "Dict doesn't have the same number of keys as values");
            return 0;
        }
        /* null_ok=1 for keys expressions to allow dict unpacking to work in
           dict literals, i.e. ``{**{a:b}}`` */
        ret = validate_exprs(state, exp->v.Dict.keys, Load, /*null_ok=*/ 1) &&
            validate_exprs(state, exp->v.Dict.values, Load, /*null_ok=*/ 0);
        break;
    case Set_kind:
        ret = validate_exprs(state, exp->v.Set.elts, Load, 0);
        break;
#define COMP(NAME) \
        case NAME ## _kind: \
            ret = validate_comprehension(state, exp->v.NAME.generators) && \
                validate_expr(state, exp->v.NAME.elt, Load); \
            break;
    COMP(ListComp)
    COMP(SetComp)
    COMP(GeneratorExp)
#undef COMP
    case DictComp_kind:
        ret = validate_comprehension(state, exp->v.DictComp.generators) &&
            validate_expr(state, exp->v.DictComp.key, Load) &&
            validate_expr(state, exp->v.DictComp.value, Load);
        break;
    case Yield_kind:
        ret = !exp->v.Yield.value || validate_expr(state, exp->v.Yield.value, Load);
        break;
    case YieldFrom_kind:
        ret = validate_expr(state, exp->v.YieldFrom.value, Load);
        break;
    case Await_kind:
        ret = validate_expr(state, exp->v.Await.value, Load);
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
        ret = validate_exprs(state, exp->v.Compare.comparators, Load, 0) &&
            validate_expr(state, exp->v.Compare.left, Load);
        break;
    case Call_kind:
        ret = validate_expr(state, exp->v.Call.func, Load) &&
            validate_exprs(state, exp->v.Call.args, Load, 0) &&
            validate_keywords(state, exp->v.Call.keywords);
        break;
    case Constant_kind:
        if (!validate_constant(state, exp->v.Constant.value)) {
            return 0;
        }
        ret = 1;
        break;
    case JoinedStr_kind:
        ret = validate_exprs(state, exp->v.JoinedStr.values, Load, 0);
        break;
    case FormattedValue_kind:
        if (validate_expr(state, exp->v.FormattedValue.value, Load) == 0)
            return 0;
        if (exp->v.FormattedValue.format_spec) {
            ret = validate_expr(state, exp->v.FormattedValue.format_spec, Load);
            break;
        }
        ret = 1;
        break;
    case Attribute_kind:
        ret = validate_expr(state, exp->v.Attribute.value, Load);
        break;
    case Subscript_kind:
        ret = validate_expr(state, exp->v.Subscript.slice, Load) &&
            validate_expr(state, exp->v.Subscript.value, Load);
        break;
    case Starred_kind:
        ret = validate_expr(state, exp->v.Starred.value, ctx);
        break;
    case Slice_kind:
        ret = (!exp->v.Slice.lower || validate_expr(state, exp->v.Slice.lower, Load)) &&
            (!exp->v.Slice.upper || validate_expr(state, exp->v.Slice.upper, Load)) &&
            (!exp->v.Slice.step || validate_expr(state, exp->v.Slice.step, Load));
        break;
    case List_kind:
        ret = validate_exprs(state, exp->v.List.elts, ctx, 0);
        break;
    case Tuple_kind:
        ret = validate_exprs(state, exp->v.Tuple.elts, ctx, 0);
        break;
    case NamedExpr_kind:
        ret = validate_expr(state, exp->v.NamedExpr.value, Load);
        break;
    case MatchAs_kind:
        PyErr_SetString(PyExc_ValueError,
                        "MatchAs is only valid in match_case patterns");
        return 0;
    case MatchOr_kind:
        PyErr_SetString(PyExc_ValueError,
                        "MatchOr is only valid in match_case patterns");
        return 0;
    /* This last case doesn't have any checking. */
    case Name_kind:
        ret = 1;
        break;
    default:
        PyErr_SetString(PyExc_SystemError, "unexpected expression");
        return 0;
    }
    state->recursion_depth--;
    return ret;
}

static int
validate_pattern(expr_ty p)
{
    // Coming soon (thanks Batuhan)!
    return 1;
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
validate_assignlist(struct validator *state, asdl_expr_seq *targets, expr_context_ty ctx)
{
    return validate_nonempty_seq(targets, "targets", ctx == Del ? "Delete" : "Assign") &&
        validate_exprs(state, targets, ctx, 0);
}

static int
validate_body(struct validator *state, asdl_stmt_seq *body, const char *owner)
{
    return validate_nonempty_seq(body, "body", owner) && validate_stmts(state, body);
}

static int
validate_stmt(struct validator *state, stmt_ty stmt)
{
    int ret;
    Py_ssize_t i;
    if (++state->recursion_depth > state->recursion_limit) {
        PyErr_SetString(PyExc_RecursionError,
                        "maximum recursion depth exceeded during compilation");
        return 0;
    }
    switch (stmt->kind) {
    case FunctionDef_kind:
        ret = validate_body(state, stmt->v.FunctionDef.body, "FunctionDef") &&
            validate_arguments(state, stmt->v.FunctionDef.args) &&
            validate_exprs(state, stmt->v.FunctionDef.decorator_list, Load, 0) &&
            (!stmt->v.FunctionDef.returns ||
             validate_expr(state, stmt->v.FunctionDef.returns, Load));
        break;
    case ClassDef_kind:
        ret = validate_body(state, stmt->v.ClassDef.body, "ClassDef") &&
            validate_exprs(state, stmt->v.ClassDef.bases, Load, 0) &&
            validate_keywords(state, stmt->v.ClassDef.keywords) &&
            validate_exprs(state, stmt->v.ClassDef.decorator_list, Load, 0);
        break;
    case Return_kind:
        ret = !stmt->v.Return.value || validate_expr(state, stmt->v.Return.value, Load);
        break;
    case Delete_kind:
        ret = validate_assignlist(state, stmt->v.Delete.targets, Del);
        break;
    case Assign_kind:
        ret = validate_assignlist(state, stmt->v.Assign.targets, Store) &&
            validate_expr(state, stmt->v.Assign.value, Load);
        break;
    case AugAssign_kind:
        ret = validate_expr(state, stmt->v.AugAssign.target, Store) &&
            validate_expr(state, stmt->v.AugAssign.value, Load);
        break;
    case AnnAssign_kind:
        if (stmt->v.AnnAssign.target->kind != Name_kind &&
            stmt->v.AnnAssign.simple) {
            PyErr_SetString(PyExc_TypeError,
                            "AnnAssign with simple non-Name target");
            return 0;
        }
        ret = validate_expr(state, stmt->v.AnnAssign.target, Store) &&
               (!stmt->v.AnnAssign.value ||
                validate_expr(state, stmt->v.AnnAssign.value, Load)) &&
               validate_expr(state, stmt->v.AnnAssign.annotation, Load);
        break;
    case For_kind:
        ret = validate_expr(state, stmt->v.For.target, Store) &&
            validate_expr(state, stmt->v.For.iter, Load) &&
            validate_body(state, stmt->v.For.body, "For") &&
            validate_stmts(state, stmt->v.For.orelse);
        break;
    case AsyncFor_kind:
        ret = validate_expr(state, stmt->v.AsyncFor.target, Store) &&
            validate_expr(state, stmt->v.AsyncFor.iter, Load) &&
            validate_body(state, stmt->v.AsyncFor.body, "AsyncFor") &&
            validate_stmts(state, stmt->v.AsyncFor.orelse);
        break;
    case While_kind:
        ret = validate_expr(state, stmt->v.While.test, Load) &&
            validate_body(state, stmt->v.While.body, "While") &&
            validate_stmts(state, stmt->v.While.orelse);
        break;
    case If_kind:
        ret = validate_expr(state, stmt->v.If.test, Load) &&
            validate_body(state, stmt->v.If.body, "If") &&
            validate_stmts(state, stmt->v.If.orelse);
        break;
    case With_kind:
        if (!validate_nonempty_seq(stmt->v.With.items, "items", "With"))
            return 0;
        for (i = 0; i < asdl_seq_LEN(stmt->v.With.items); i++) {
            withitem_ty item = asdl_seq_GET(stmt->v.With.items, i);
            if (!validate_expr(state, item->context_expr, Load) ||
                (item->optional_vars && !validate_expr(state, item->optional_vars, Store)))
                return 0;
        }
        ret = validate_body(state, stmt->v.With.body, "With");
        break;
    case AsyncWith_kind:
        if (!validate_nonempty_seq(stmt->v.AsyncWith.items, "items", "AsyncWith"))
            return 0;
        for (i = 0; i < asdl_seq_LEN(stmt->v.AsyncWith.items); i++) {
            withitem_ty item = asdl_seq_GET(stmt->v.AsyncWith.items, i);
            if (!validate_expr(state, item->context_expr, Load) ||
                (item->optional_vars && !validate_expr(state, item->optional_vars, Store)))
                return 0;
        }
        ret = validate_body(state, stmt->v.AsyncWith.body, "AsyncWith");
        break;
    case Match_kind:
        if (!validate_expr(state, stmt->v.Match.subject, Load)
            || !validate_nonempty_seq(stmt->v.Match.cases, "cases", "Match")) {
            return 0;
        }
        for (i = 0; i < asdl_seq_LEN(stmt->v.Match.cases); i++) {
            match_case_ty m = asdl_seq_GET(stmt->v.Match.cases, i);
            if (!validate_pattern(m->pattern)
                || (m->guard && !validate_expr(state, m->guard, Load))
                || !validate_body(state, m->body, "match_case")) {
                return 0;
            }
        }
        ret = 1;
        break;
    case Raise_kind:
        if (stmt->v.Raise.exc) {
            ret = validate_expr(state, stmt->v.Raise.exc, Load) &&
                (!stmt->v.Raise.cause || validate_expr(state, stmt->v.Raise.cause, Load));
            break;
        }
        if (stmt->v.Raise.cause) {
            PyErr_SetString(PyExc_ValueError, "Raise with cause but no exception");
            return 0;
        }
        ret = 1;
        break;
    case Try_kind:
        if (!validate_body(state, stmt->v.Try.body, "Try"))
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
                 !validate_expr(state, handler->v.ExceptHandler.type, Load)) ||
                !validate_body(state, handler->v.ExceptHandler.body, "ExceptHandler"))
                return 0;
        }
        ret = (!asdl_seq_LEN(stmt->v.Try.finalbody) ||
                validate_stmts(state, stmt->v.Try.finalbody)) &&
            (!asdl_seq_LEN(stmt->v.Try.orelse) ||
             validate_stmts(state, stmt->v.Try.orelse));
        break;
    case Assert_kind:
        ret = validate_expr(state, stmt->v.Assert.test, Load) &&
            (!stmt->v.Assert.msg || validate_expr(state, stmt->v.Assert.msg, Load));
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
        ret = validate_expr(state, stmt->v.Expr.value, Load);
        break;
    case AsyncFunctionDef_kind:
        ret = validate_body(state, stmt->v.AsyncFunctionDef.body, "AsyncFunctionDef") &&
            validate_arguments(state, stmt->v.AsyncFunctionDef.args) &&
            validate_exprs(state, stmt->v.AsyncFunctionDef.decorator_list, Load, 0) &&
            (!stmt->v.AsyncFunctionDef.returns ||
             validate_expr(state, stmt->v.AsyncFunctionDef.returns, Load));
        break;
    case Pass_kind:
    case Break_kind:
    case Continue_kind:
        ret = 1;
        break;
    default:
        PyErr_SetString(PyExc_SystemError, "unexpected statement");
        return 0;
    }
    state->recursion_depth--;
    return ret;
}

static int
validate_stmts(struct validator *state, asdl_stmt_seq *seq)
{
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(seq); i++) {
        stmt_ty stmt = asdl_seq_GET(seq, i);
        if (stmt) {
            if (!validate_stmt(state, stmt))
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
validate_exprs(struct validator *state, asdl_expr_seq *exprs, expr_context_ty ctx, int null_ok)
{
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(exprs); i++) {
        expr_ty expr = asdl_seq_GET(exprs, i);
        if (expr) {
            if (!validate_expr(state, expr, ctx))
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

/* See comments in symtable.c. */
#define COMPILER_STACK_FRAME_SCALE 3

int
_PyAST_Validate(mod_ty mod)
{
    int res = 0;
    struct validator state;
    PyThreadState *tstate;
    int recursion_limit = Py_GetRecursionLimit();
    int starting_recursion_depth;

    /* Setup recursion depth check counters */
    tstate = _PyThreadState_GET();
    if (!tstate) {
        return 0;
    }
    /* Be careful here to prevent overflow. */
    starting_recursion_depth = (tstate->recursion_depth < INT_MAX / COMPILER_STACK_FRAME_SCALE) ?
        tstate->recursion_depth * COMPILER_STACK_FRAME_SCALE : tstate->recursion_depth;
    state.recursion_depth = starting_recursion_depth;
    state.recursion_limit = (recursion_limit < INT_MAX / COMPILER_STACK_FRAME_SCALE) ?
        recursion_limit * COMPILER_STACK_FRAME_SCALE : recursion_limit;

    switch (mod->kind) {
    case Module_kind:
        res = validate_stmts(&state, mod->v.Module.body);
        break;
    case Interactive_kind:
        res = validate_stmts(&state, mod->v.Interactive.body);
        break;
    case Expression_kind:
        res = validate_expr(&state, mod->v.Expression.body, Load);
        break;
    default:
        PyErr_SetString(PyExc_SystemError, "impossible module node");
        res = 0;
        break;
    }

    /* Check that the recursion depth counting balanced correctly */
    if (res && state.recursion_depth != starting_recursion_depth) {
        PyErr_Format(PyExc_SystemError,
            "AST validator recursion depth mismatch (before=%d, after=%d)",
            starting_recursion_depth, state.recursion_depth);
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
