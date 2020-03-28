/*
 * This file includes functions to transform a concrete syntax tree (CST) to
 * an abstract syntax tree (AST). The main function is PyAST_FromNode().
 *
 */
#include "Python.h"
#include "Python-ast.h"
#include "node.h"
#include "ast.h"
#include "token.h"
#include "pythonrun.h"

#include <assert.h>
#include <stdbool.h>

#define MAXLEVEL 200    /* Max parentheses level */

static int validate_stmts(asdl_seq *);
static int validate_exprs(asdl_seq *, expr_context_ty, int);
static int validate_nonempty_seq(asdl_seq *, const char *, const char *);
static int validate_stmt(stmt_ty);
static int validate_expr(expr_ty, expr_context_ty);

static int
validate_comprehension(asdl_seq *gens)
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
validate_keywords(asdl_seq *keywords)
{
    Py_ssize_t i;
    for (i = 0; i < asdl_seq_LEN(keywords); i++)
        if (!validate_expr(((keyword_ty)asdl_seq_GET(keywords, i))->value, Load))
            return 0;
    return 1;
}

static int
validate_args(asdl_seq *args)
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
validate_nonempty_seq(asdl_seq *seq, const char *what, const char *owner)
{
    if (asdl_seq_LEN(seq))
        return 1;
    PyErr_Format(PyExc_ValueError, "empty %s on %s", what, owner);
    return 0;
}

static int
validate_assignlist(asdl_seq *targets, expr_context_ty ctx)
{
    return validate_nonempty_seq(targets, "targets", ctx == Del ? "Delete" : "Assign") &&
        validate_exprs(targets, ctx, 0);
}

static int
validate_body(asdl_seq *body, const char *owner)
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
validate_stmts(asdl_seq *seq)
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
validate_exprs(asdl_seq *exprs, expr_context_ty ctx, int null_ok)
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

/* This is done here, so defines like "test" don't interfere with AST use above. */
#include "grammar.h"
#include "parsetok.h"
#include "graminit.h"

/* Data structure used internally */
struct compiling {
    PyArena *c_arena; /* Arena for allocating memory. */
    PyObject *c_filename; /* filename */
    PyObject *c_normalize; /* Normalization function from unicodedata. */
    int c_feature_version; /* Latest minor version of Python for allowed features */
};

static asdl_seq *seq_for_testlist(struct compiling *, const node *);
static expr_ty ast_for_expr(struct compiling *, const node *);
static stmt_ty ast_for_stmt(struct compiling *, const node *);
static asdl_seq *ast_for_suite(struct compiling *c, const node *n);
static asdl_seq *ast_for_exprlist(struct compiling *, const node *,
                                  expr_context_ty);
static expr_ty ast_for_testlist(struct compiling *, const node *);
static stmt_ty ast_for_classdef(struct compiling *, const node *, asdl_seq *);

static stmt_ty ast_for_with_stmt(struct compiling *, const node *, bool);
static stmt_ty ast_for_for_stmt(struct compiling *, const node *, bool);

/* Note different signature for ast_for_call */
static expr_ty ast_for_call(struct compiling *, const node *, expr_ty,
                            const node *, const node *, const node *);

static PyObject *parsenumber(struct compiling *, const char *);
static expr_ty parsestrplus(struct compiling *, const node *n);
static void get_last_end_pos(asdl_seq *, int *, int *);

#define COMP_GENEXP   0
#define COMP_LISTCOMP 1
#define COMP_SETCOMP  2

static int
init_normalization(struct compiling *c)
{
    PyObject *m = PyImport_ImportModuleNoBlock("unicodedata");
    if (!m)
        return 0;
    c->c_normalize = PyObject_GetAttrString(m, "normalize");
    Py_DECREF(m);
    if (!c->c_normalize)
        return 0;
    return 1;
}

static identifier
new_identifier(const char *n, struct compiling *c)
{
    PyObject *id = PyUnicode_DecodeUTF8(n, strlen(n), NULL);
    if (!id)
        return NULL;
    /* PyUnicode_DecodeUTF8 should always return a ready string. */
    assert(PyUnicode_IS_READY(id));
    /* Check whether there are non-ASCII characters in the
       identifier; if so, normalize to NFKC. */
    if (!PyUnicode_IS_ASCII(id)) {
        PyObject *id2;
        if (!c->c_normalize && !init_normalization(c)) {
            Py_DECREF(id);
            return NULL;
        }
        PyObject *form = PyUnicode_InternFromString("NFKC");
        if (form == NULL) {
            Py_DECREF(id);
            return NULL;
        }
        PyObject *args[2] = {form, id};
        id2 = _PyObject_FastCall(c->c_normalize, args, 2);
        Py_DECREF(id);
        Py_DECREF(form);
        if (!id2)
            return NULL;
        if (!PyUnicode_Check(id2)) {
            PyErr_Format(PyExc_TypeError,
                         "unicodedata.normalize() must return a string, not "
                         "%.200s",
                         _PyType_Name(Py_TYPE(id2)));
            Py_DECREF(id2);
            return NULL;
        }
        id = id2;
    }
    PyUnicode_InternInPlace(&id);
    if (PyArena_AddPyObject(c->c_arena, id) < 0) {
        Py_DECREF(id);
        return NULL;
    }
    return id;
}

#define NEW_IDENTIFIER(n) new_identifier(STR(n), c)

static int
ast_error(struct compiling *c, const node *n, const char *errmsg, ...)
{
    PyObject *value, *errstr, *loc, *tmp;
    va_list va;

    va_start(va, errmsg);
    errstr = PyUnicode_FromFormatV(errmsg, va);
    va_end(va);
    if (!errstr) {
        return 0;
    }
    loc = PyErr_ProgramTextObject(c->c_filename, LINENO(n));
    if (!loc) {
        Py_INCREF(Py_None);
        loc = Py_None;
    }
    tmp = Py_BuildValue("(OiiN)", c->c_filename, LINENO(n), n->n_col_offset + 1, loc);
    if (!tmp) {
        Py_DECREF(errstr);
        return 0;
    }
    value = PyTuple_Pack(2, errstr, tmp);
    Py_DECREF(errstr);
    Py_DECREF(tmp);
    if (value) {
        PyErr_SetObject(PyExc_SyntaxError, value);
        Py_DECREF(value);
    }
    return 0;
}

/* num_stmts() returns number of contained statements.

   Use this routine to determine how big a sequence is needed for
   the statements in a parse tree.  Its raison d'etre is this bit of
   grammar:

   stmt: simple_stmt | compound_stmt
   simple_stmt: small_stmt (';' small_stmt)* [';'] NEWLINE

   A simple_stmt can contain multiple small_stmt elements joined
   by semicolons.  If the arg is a simple_stmt, the number of
   small_stmt elements is returned.
*/

static string
new_type_comment(const char *s, struct compiling *c)
{
    PyObject *res = PyUnicode_DecodeUTF8(s, strlen(s), NULL);
    if (res == NULL)
        return NULL;
    if (PyArena_AddPyObject(c->c_arena, res) < 0) {
        Py_DECREF(res);
        return NULL;
    }
    return res;
}
#define NEW_TYPE_COMMENT(n) new_type_comment(STR(n), c)

static int
num_stmts(const node *n)
{
    int i, l;
    node *ch;

    switch (TYPE(n)) {
        case single_input:
            if (TYPE(CHILD(n, 0)) == NEWLINE)
                return 0;
            else
                return num_stmts(CHILD(n, 0));
        case file_input:
            l = 0;
            for (i = 0; i < NCH(n); i++) {
                ch = CHILD(n, i);
                if (TYPE(ch) == stmt)
                    l += num_stmts(ch);
            }
            return l;
        case stmt:
            return num_stmts(CHILD(n, 0));
        case compound_stmt:
            return 1;
        case simple_stmt:
            return NCH(n) / 2; /* Divide by 2 to remove count of semi-colons */
        case suite:
        case func_body_suite:
            /* func_body_suite: simple_stmt | NEWLINE [TYPE_COMMENT NEWLINE] INDENT stmt+ DEDENT */
            /* suite: simple_stmt | NEWLINE INDENT stmt+ DEDENT */
            if (NCH(n) == 1)
                return num_stmts(CHILD(n, 0));
            else {
                i = 2;
                l = 0;
                if (TYPE(CHILD(n, 1)) == TYPE_COMMENT)
                    i += 2;
                for (; i < (NCH(n) - 1); i++)
                    l += num_stmts(CHILD(n, i));
                return l;
            }
        default: {
            _Py_FatalErrorFormat(__func__, "Non-statement found: %d %d",
                                 TYPE(n), NCH(n));
        }
    }
    Py_UNREACHABLE();
}

/* Transform the CST rooted at node * to the appropriate AST
*/

mod_ty
PyAST_FromNodeObject(const node *n, PyCompilerFlags *flags,
                     PyObject *filename, PyArena *arena)
{
    int i, j, k, num;
    asdl_seq *stmts = NULL;
    asdl_seq *type_ignores = NULL;
    stmt_ty s;
    node *ch;
    struct compiling c;
    mod_ty res = NULL;
    asdl_seq *argtypes = NULL;
    expr_ty ret, arg;

    c.c_arena = arena;
    /* borrowed reference */
    c.c_filename = filename;
    c.c_normalize = NULL;
    c.c_feature_version = flags ? flags->cf_feature_version : PY_MINOR_VERSION;

    if (TYPE(n) == encoding_decl)
        n = CHILD(n, 0);

    k = 0;
    switch (TYPE(n)) {
        case file_input:
            stmts = _Py_asdl_seq_new(num_stmts(n), arena);
            if (!stmts)
                goto out;
            for (i = 0; i < NCH(n) - 1; i++) {
                ch = CHILD(n, i);
                if (TYPE(ch) == NEWLINE)
                    continue;
                REQ(ch, stmt);
                num = num_stmts(ch);
                if (num == 1) {
                    s = ast_for_stmt(&c, ch);
                    if (!s)
                        goto out;
                    asdl_seq_SET(stmts, k++, s);
                }
                else {
                    ch = CHILD(ch, 0);
                    REQ(ch, simple_stmt);
                    for (j = 0; j < num; j++) {
                        s = ast_for_stmt(&c, CHILD(ch, j * 2));
                        if (!s)
                            goto out;
                        asdl_seq_SET(stmts, k++, s);
                    }
                }
            }

            /* Type ignores are stored under the ENDMARKER in file_input. */
            ch = CHILD(n, NCH(n) - 1);
            REQ(ch, ENDMARKER);
            num = NCH(ch);
            type_ignores = _Py_asdl_seq_new(num, arena);
            if (!type_ignores)
                goto out;

            for (i = 0; i < num; i++) {
                string type_comment = new_type_comment(STR(CHILD(ch, i)), &c);
                if (!type_comment)
                    goto out;
                type_ignore_ty ti = TypeIgnore(LINENO(CHILD(ch, i)), type_comment, arena);
                if (!ti)
                   goto out;
               asdl_seq_SET(type_ignores, i, ti);
            }

            res = Module(stmts, type_ignores, arena);
            break;
        case eval_input: {
            expr_ty testlist_ast;

            /* XXX Why not comp_for here? */
            testlist_ast = ast_for_testlist(&c, CHILD(n, 0));
            if (!testlist_ast)
                goto out;
            res = Expression(testlist_ast, arena);
            break;
        }
        case single_input:
            if (TYPE(CHILD(n, 0)) == NEWLINE) {
                stmts = _Py_asdl_seq_new(1, arena);
                if (!stmts)
                    goto out;
                asdl_seq_SET(stmts, 0, Pass(n->n_lineno, n->n_col_offset,
                                            n->n_end_lineno, n->n_end_col_offset,
                                            arena));
                if (!asdl_seq_GET(stmts, 0))
                    goto out;
                res = Interactive(stmts, arena);
            }
            else {
                n = CHILD(n, 0);
                num = num_stmts(n);
                stmts = _Py_asdl_seq_new(num, arena);
                if (!stmts)
                    goto out;
                if (num == 1) {
                    s = ast_for_stmt(&c, n);
                    if (!s)
                        goto out;
                    asdl_seq_SET(stmts, 0, s);
                }
                else {
                    /* Only a simple_stmt can contain multiple statements. */
                    REQ(n, simple_stmt);
                    for (i = 0; i < NCH(n); i += 2) {
                        if (TYPE(CHILD(n, i)) == NEWLINE)
                            break;
                        s = ast_for_stmt(&c, CHILD(n, i));
                        if (!s)
                            goto out;
                        asdl_seq_SET(stmts, i / 2, s);
                    }
                }

                res = Interactive(stmts, arena);
            }
            break;
        case func_type_input:
            n = CHILD(n, 0);
            REQ(n, func_type);

            if (TYPE(CHILD(n, 1)) == typelist) {
                ch = CHILD(n, 1);
                /* this is overly permissive -- we don't pay any attention to
                 * stars on the args -- just parse them into an ordered list */
                num = 0;
                for (i = 0; i < NCH(ch); i++) {
                    if (TYPE(CHILD(ch, i)) == test) {
                        num++;
                    }
                }

                argtypes = _Py_asdl_seq_new(num, arena);
                if (!argtypes)
                    goto out;

                j = 0;
                for (i = 0; i < NCH(ch); i++) {
                    if (TYPE(CHILD(ch, i)) == test) {
                        arg = ast_for_expr(&c, CHILD(ch, i));
                        if (!arg)
                            goto out;
                        asdl_seq_SET(argtypes, j++, arg);
                    }
                }
            }
            else {
                argtypes = _Py_asdl_seq_new(0, arena);
                if (!argtypes)
                    goto out;
            }

            ret = ast_for_expr(&c, CHILD(n, NCH(n) - 1));
            if (!ret)
                goto out;
            res = FunctionType(argtypes, ret, arena);
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "invalid node %d for PyAST_FromNode", TYPE(n));
            goto out;
    }
 out:
    if (c.c_normalize) {
        Py_DECREF(c.c_normalize);
    }
    return res;
}

mod_ty
PyAST_FromNode(const node *n, PyCompilerFlags *flags, const char *filename_str,
               PyArena *arena)
{
    mod_ty mod;
    PyObject *filename;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    mod = PyAST_FromNodeObject(n, flags, filename, arena);
    Py_DECREF(filename);
    return mod;

}

/* Return the AST repr. of the operator represented as syntax (|, ^, etc.)
*/

static operator_ty
get_operator(struct compiling *c, const node *n)
{
    switch (TYPE(n)) {
        case VBAR:
            return BitOr;
        case CIRCUMFLEX:
            return BitXor;
        case AMPER:
            return BitAnd;
        case LEFTSHIFT:
            return LShift;
        case RIGHTSHIFT:
            return RShift;
        case PLUS:
            return Add;
        case MINUS:
            return Sub;
        case STAR:
            return Mult;
        case AT:
            if (c->c_feature_version < 5) {
                ast_error(c, n,
                          "The '@' operator is only supported in Python 3.5 and greater");
                return (operator_ty)0;
            }
            return MatMult;
        case SLASH:
            return Div;
        case DOUBLESLASH:
            return FloorDiv;
        case PERCENT:
            return Mod;
        default:
            return (operator_ty)0;
    }
}

static const char * const FORBIDDEN[] = {
    "None",
    "True",
    "False",
    "__debug__",
    NULL,
};

static int
forbidden_name(struct compiling *c, identifier name, const node *n,
               int full_checks)
{
    assert(PyUnicode_Check(name));
    const char * const *p = FORBIDDEN;
    if (!full_checks) {
        /* In most cases, the parser will protect True, False, and None
           from being assign to. */
        p += 3;
    }
    for (; *p; p++) {
        if (_PyUnicode_EqualToASCIIString(name, *p)) {
            ast_error(c, n, "cannot assign to %U", name);
            return 1;
        }
    }
    return 0;
}

static expr_ty
copy_location(expr_ty e, const node *n, const node *end)
{
    if (e) {
        e->lineno = LINENO(n);
        e->col_offset = n->n_col_offset;
        e->end_lineno = end->n_end_lineno;
        e->end_col_offset = end->n_end_col_offset;
    }
    return e;
}

static const char *
get_expr_name(expr_ty e)
{
    switch (e->kind) {
        case Attribute_kind:
            return "attribute";
        case Subscript_kind:
            return "subscript";
        case Starred_kind:
            return "starred";
        case Name_kind:
            return "name";
        case List_kind:
            return "list";
        case Tuple_kind:
            return "tuple";
        case Lambda_kind:
            return "lambda";
        case Call_kind:
            return "function call";
        case BoolOp_kind:
        case BinOp_kind:
        case UnaryOp_kind:
            return "operator";
        case GeneratorExp_kind:
            return "generator expression";
        case Yield_kind:
        case YieldFrom_kind:
            return "yield expression";
        case Await_kind:
            return "await expression";
        case ListComp_kind:
            return "list comprehension";
        case SetComp_kind:
            return "set comprehension";
        case DictComp_kind:
            return "dict comprehension";
        case Dict_kind:
            return "dict display";
        case Set_kind:
            return "set display";
        case JoinedStr_kind:
        case FormattedValue_kind:
            return "f-string expression";
        case Constant_kind: {
            PyObject *value = e->v.Constant.value;
            if (value == Py_None) {
                return "None";
            }
            if (value == Py_False) {
                return "False";
            }
            if (value == Py_True) {
                return "True";
            }
            if (value == Py_Ellipsis) {
                return "Ellipsis";
            }
            return "literal";
        }
        case Compare_kind:
            return "comparison";
        case IfExp_kind:
            return "conditional expression";
        case NamedExpr_kind:
            return "named expression";
        default:
            PyErr_Format(PyExc_SystemError,
                         "unexpected expression in assignment %d (line %d)",
                         e->kind, e->lineno);
            return NULL;
    }
}

/* Set the context ctx for expr_ty e, recursively traversing e.

   Only sets context for expr kinds that "can appear in assignment context"
   (according to ../Parser/Python.asdl).  For other expr kinds, it sets
   an appropriate syntax error and returns false.
*/

static int
set_context(struct compiling *c, expr_ty e, expr_context_ty ctx, const node *n)
{
    asdl_seq *s = NULL;

    /* Expressions in an augmented assignment have a Store context. */

    switch (e->kind) {
        case Attribute_kind:
            e->v.Attribute.ctx = ctx;
            if (ctx == Store && forbidden_name(c, e->v.Attribute.attr, n, 1))
                return 0;
            break;
        case Subscript_kind:
            e->v.Subscript.ctx = ctx;
            break;
        case Starred_kind:
            e->v.Starred.ctx = ctx;
            if (!set_context(c, e->v.Starred.value, ctx, n))
                return 0;
            break;
        case Name_kind:
            if (ctx == Store) {
                if (forbidden_name(c, e->v.Name.id, n, 0))
                    return 0; /* forbidden_name() calls ast_error() */
            }
            e->v.Name.ctx = ctx;
            break;
        case List_kind:
            e->v.List.ctx = ctx;
            s = e->v.List.elts;
            break;
        case Tuple_kind:
            e->v.Tuple.ctx = ctx;
            s = e->v.Tuple.elts;
            break;
        default: {
            const char *expr_name = get_expr_name(e);
            if (expr_name != NULL) {
                ast_error(c, n, "cannot %s %s",
                          ctx == Store ? "assign to" : "delete",
                          expr_name);
            }
            return 0;
        }
    }

    /* If the LHS is a list or tuple, we need to set the assignment
       context for all the contained elements.
    */
    if (s) {
        Py_ssize_t i;

        for (i = 0; i < asdl_seq_LEN(s); i++) {
            if (!set_context(c, (expr_ty)asdl_seq_GET(s, i), ctx, n))
                return 0;
        }
    }
    return 1;
}

static operator_ty
ast_for_augassign(struct compiling *c, const node *n)
{
    REQ(n, augassign);
    n = CHILD(n, 0);
    switch (STR(n)[0]) {
        case '+':
            return Add;
        case '-':
            return Sub;
        case '/':
            if (STR(n)[1] == '/')
                return FloorDiv;
            else
                return Div;
        case '%':
            return Mod;
        case '<':
            return LShift;
        case '>':
            return RShift;
        case '&':
            return BitAnd;
        case '^':
            return BitXor;
        case '|':
            return BitOr;
        case '*':
            if (STR(n)[1] == '*')
                return Pow;
            else
                return Mult;
        case '@':
            if (c->c_feature_version < 5) {
                ast_error(c, n,
                          "The '@' operator is only supported in Python 3.5 and greater");
                return (operator_ty)0;
            }
            return MatMult;
        default:
            PyErr_Format(PyExc_SystemError, "invalid augassign: %s", STR(n));
            return (operator_ty)0;
    }
}

static cmpop_ty
ast_for_comp_op(struct compiling *c, const node *n)
{
    /* comp_op: '<'|'>'|'=='|'>='|'<='|'!='|'in'|'not' 'in'|'is'
               |'is' 'not'
    */
    REQ(n, comp_op);
    if (NCH(n) == 1) {
        n = CHILD(n, 0);
        switch (TYPE(n)) {
            case LESS:
                return Lt;
            case GREATER:
                return Gt;
            case EQEQUAL:                       /* == */
                return Eq;
            case LESSEQUAL:
                return LtE;
            case GREATEREQUAL:
                return GtE;
            case NOTEQUAL:
                return NotEq;
            case NAME:
                if (strcmp(STR(n), "in") == 0)
                    return In;
                if (strcmp(STR(n), "is") == 0)
                    return Is;
                /* fall through */
            default:
                PyErr_Format(PyExc_SystemError, "invalid comp_op: %s",
                             STR(n));
                return (cmpop_ty)0;
        }
    }
    else if (NCH(n) == 2) {
        /* handle "not in" and "is not" */
        switch (TYPE(CHILD(n, 0))) {
            case NAME:
                if (strcmp(STR(CHILD(n, 1)), "in") == 0)
                    return NotIn;
                if (strcmp(STR(CHILD(n, 0)), "is") == 0)
                    return IsNot;
                /* fall through */
            default:
                PyErr_Format(PyExc_SystemError, "invalid comp_op: %s %s",
                             STR(CHILD(n, 0)), STR(CHILD(n, 1)));
                return (cmpop_ty)0;
        }
    }
    PyErr_Format(PyExc_SystemError, "invalid comp_op: has %d children",
                 NCH(n));
    return (cmpop_ty)0;
}

static asdl_seq *
seq_for_testlist(struct compiling *c, const node *n)
{
    /* testlist: test (',' test)* [',']
       testlist_star_expr: test|star_expr (',' test|star_expr)* [',']
    */
    asdl_seq *seq;
    expr_ty expression;
    int i;
    assert(TYPE(n) == testlist || TYPE(n) == testlist_star_expr || TYPE(n) == testlist_comp);

    seq = _Py_asdl_seq_new((NCH(n) + 1) / 2, c->c_arena);
    if (!seq)
        return NULL;

    for (i = 0; i < NCH(n); i += 2) {
        const node *ch = CHILD(n, i);
        assert(TYPE(ch) == test || TYPE(ch) == test_nocond || TYPE(ch) == star_expr || TYPE(ch) == namedexpr_test);

        expression = ast_for_expr(c, ch);
        if (!expression)
            return NULL;

        assert(i / 2 < seq->size);
        asdl_seq_SET(seq, i / 2, expression);
    }
    return seq;
}

static arg_ty
ast_for_arg(struct compiling *c, const node *n)
{
    identifier name;
    expr_ty annotation = NULL;
    node *ch;
    arg_ty ret;

    assert(TYPE(n) == tfpdef || TYPE(n) == vfpdef);
    ch = CHILD(n, 0);
    name = NEW_IDENTIFIER(ch);
    if (!name)
        return NULL;
    if (forbidden_name(c, name, ch, 0))
        return NULL;

    if (NCH(n) == 3 && TYPE(CHILD(n, 1)) == COLON) {
        annotation = ast_for_expr(c, CHILD(n, 2));
        if (!annotation)
            return NULL;
    }

    ret = arg(name, annotation, NULL, LINENO(n), n->n_col_offset,
              n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    if (!ret)
        return NULL;
    return ret;
}

/* returns -1 if failed to handle keyword only arguments
   returns new position to keep processing if successful
               (',' tfpdef ['=' test])*
                     ^^^
   start pointing here
 */
static int
handle_keywordonly_args(struct compiling *c, const node *n, int start,
                        asdl_seq *kwonlyargs, asdl_seq *kwdefaults)
{
    PyObject *argname;
    node *ch;
    expr_ty expression, annotation;
    arg_ty arg = NULL;
    int i = start;
    int j = 0; /* index for kwdefaults and kwonlyargs */

    if (kwonlyargs == NULL) {
        ast_error(c, CHILD(n, start), "named arguments must follow bare *");
        return -1;
    }
    assert(kwdefaults != NULL);
    while (i < NCH(n)) {
        ch = CHILD(n, i);
        switch (TYPE(ch)) {
            case vfpdef:
            case tfpdef:
                if (i + 1 < NCH(n) && TYPE(CHILD(n, i + 1)) == EQUAL) {
                    expression = ast_for_expr(c, CHILD(n, i + 2));
                    if (!expression)
                        goto error;
                    asdl_seq_SET(kwdefaults, j, expression);
                    i += 2; /* '=' and test */
                }
                else { /* setting NULL if no default value exists */
                    asdl_seq_SET(kwdefaults, j, NULL);
                }
                if (NCH(ch) == 3) {
                    /* ch is NAME ':' test */
                    annotation = ast_for_expr(c, CHILD(ch, 2));
                    if (!annotation)
                        goto error;
                }
                else {
                    annotation = NULL;
                }
                ch = CHILD(ch, 0);
                argname = NEW_IDENTIFIER(ch);
                if (!argname)
                    goto error;
                if (forbidden_name(c, argname, ch, 0))
                    goto error;
                arg = arg(argname, annotation, NULL, LINENO(ch), ch->n_col_offset,
                          ch->n_end_lineno, ch->n_end_col_offset,
                          c->c_arena);
                if (!arg)
                    goto error;
                asdl_seq_SET(kwonlyargs, j++, arg);
                i += 1; /* the name */
                if (i < NCH(n) && TYPE(CHILD(n, i)) == COMMA)
                    i += 1; /* the comma, if present */
                break;
            case TYPE_COMMENT:
                /* arg will be equal to the last argument processed */
                arg->type_comment = NEW_TYPE_COMMENT(ch);
                if (!arg->type_comment)
                    goto error;
                i += 1;
                break;
            case DOUBLESTAR:
                return i;
            default:
                ast_error(c, ch, "unexpected node");
                goto error;
        }
    }
    return i;
 error:
    return -1;
}

/* Create AST for argument list. */

static arguments_ty
ast_for_arguments(struct compiling *c, const node *n)
{
    /* This function handles both typedargslist (function definition)
       and varargslist (lambda definition).

       parameters: '(' [typedargslist] ')'

       The following definition for typedarglist is equivalent to this set of rules:

         arguments = argument (',' [TYPE_COMMENT] argument)*
         argument = tfpdef ['=' test]
         kwargs = '**' tfpdef [','] [TYPE_COMMENT]
         args = '*' [tfpdef]
         kwonly_kwargs = (',' [TYPE_COMMENT] argument)* (TYPE_COMMENT | [','
                         [TYPE_COMMENT] [kwargs]])
         args_kwonly_kwargs = args kwonly_kwargs | kwargs
         poskeyword_args_kwonly_kwargs = arguments ( TYPE_COMMENT | [','
                                         [TYPE_COMMENT] [args_kwonly_kwargs]])
         typedargslist_no_posonly  = poskeyword_args_kwonly_kwargs | args_kwonly_kwargs
         typedarglist = (arguments ',' [TYPE_COMMENT] '/' [',' [[TYPE_COMMENT]
                        typedargslist_no_posonly]])|(typedargslist_no_posonly)"

       typedargslist: ( (tfpdef ['=' test] (',' [TYPE_COMMENT] tfpdef ['=' test])*
           ',' [TYPE_COMMENT] '/' [',' [ [TYPE_COMMENT] tfpdef ['=' test] ( ','
           [TYPE_COMMENT] tfpdef ['=' test])* (TYPE_COMMENT | [',' [TYPE_COMMENT] [ '*'
           [tfpdef] (',' [TYPE_COMMENT] tfpdef ['=' test])* (TYPE_COMMENT | [','
           [TYPE_COMMENT] ['**' tfpdef [','] [TYPE_COMMENT]]]) | '**' tfpdef [',']
           [TYPE_COMMENT]]]) | '*' [tfpdef] (',' [TYPE_COMMENT] tfpdef ['=' test])*
           (TYPE_COMMENT | [',' [TYPE_COMMENT] ['**' tfpdef [','] [TYPE_COMMENT]]]) |
           '**' tfpdef [','] [TYPE_COMMENT]]] ) |  (tfpdef ['=' test] (','
           [TYPE_COMMENT] tfpdef ['=' test])* (TYPE_COMMENT | [',' [TYPE_COMMENT] [ '*'
           [tfpdef] (',' [TYPE_COMMENT] tfpdef ['=' test])* (TYPE_COMMENT | [','
           [TYPE_COMMENT] ['**' tfpdef [','] [TYPE_COMMENT]]]) | '**' tfpdef [',']
           [TYPE_COMMENT]]]) | '*' [tfpdef] (',' [TYPE_COMMENT] tfpdef ['=' test])*
           (TYPE_COMMENT | [',' [TYPE_COMMENT] ['**' tfpdef [','] [TYPE_COMMENT]]]) |
           '**' tfpdef [','] [TYPE_COMMENT]))

       tfpdef: NAME [':' test]

       The following definition for varargslist is equivalent to this set of rules:

         arguments = argument (',' argument )*
         argument = vfpdef ['=' test]
         kwargs = '**' vfpdef [',']
         args = '*' [vfpdef]
         kwonly_kwargs = (',' argument )* [',' [kwargs]]
         args_kwonly_kwargs = args kwonly_kwargs | kwargs
         poskeyword_args_kwonly_kwargs = arguments [',' [args_kwonly_kwargs]]
         vararglist_no_posonly = poskeyword_args_kwonly_kwargs | args_kwonly_kwargs
         varargslist = arguments ',' '/' [','[(vararglist_no_posonly)]] |
                       (vararglist_no_posonly)

       varargslist: vfpdef ['=' test ](',' vfpdef ['=' test])* ',' '/' [',' [ (vfpdef ['='
           test] (',' vfpdef ['=' test])* [',' [ '*' [vfpdef] (',' vfpdef ['=' test])* [','
           ['**' vfpdef [',']]] | '**' vfpdef [',']]] | '*' [vfpdef] (',' vfpdef ['=' test])*
           [',' ['**' vfpdef [',']]] | '**' vfpdef [',']) ]] | (vfpdef ['=' test] (',' vfpdef
           ['=' test])* [',' [ '*' [vfpdef] (',' vfpdef ['=' test])* [',' ['**' vfpdef [',']]]
           | '**' vfpdef [',']]] | '*' [vfpdef] (',' vfpdef ['=' test])* [',' ['**' vfpdef
           [',']]] | '**' vfpdef [','])

       vfpdef: NAME

    */
    int i, j, k, l, nposonlyargs=0, nposargs = 0, nkwonlyargs = 0;
    int nposdefaults = 0, found_default = 0;
    asdl_seq *posonlyargs, *posargs, *posdefaults, *kwonlyargs, *kwdefaults;
    arg_ty vararg = NULL, kwarg = NULL;
    arg_ty arg = NULL;
    node *ch;

    if (TYPE(n) == parameters) {
        if (NCH(n) == 2) /* () as argument list */
            return arguments(NULL, NULL, NULL, NULL, NULL, NULL, NULL, c->c_arena);
        n = CHILD(n, 1);
    }
    assert(TYPE(n) == typedargslist || TYPE(n) == varargslist);

    /* First count the number of positional args & defaults.  The
       variable i is the loop index for this for loop and the next.
       The next loop picks up where the first leaves off.
    */
    for (i = 0; i < NCH(n); i++) {
        ch = CHILD(n, i);
        if (TYPE(ch) == STAR) {
            /* skip star */
            i++;
            if (i < NCH(n) && /* skip argument following star */
                (TYPE(CHILD(n, i)) == tfpdef ||
                 TYPE(CHILD(n, i)) == vfpdef)) {
                i++;
            }
            break;
        }
        if (TYPE(ch) == DOUBLESTAR) break;
        if (TYPE(ch) == vfpdef || TYPE(ch) == tfpdef) nposargs++;
        if (TYPE(ch) == EQUAL) nposdefaults++;
        if (TYPE(ch) == SLASH ) {
            nposonlyargs = nposargs;
            nposargs = 0;
        }
    }
    /* count the number of keyword only args &
       defaults for keyword only args */
    for ( ; i < NCH(n); ++i) {
        ch = CHILD(n, i);
        if (TYPE(ch) == DOUBLESTAR) break;
        if (TYPE(ch) == tfpdef || TYPE(ch) == vfpdef) nkwonlyargs++;
    }
    posonlyargs = (nposonlyargs ? _Py_asdl_seq_new(nposonlyargs, c->c_arena) : NULL);
    if (!posonlyargs && nposonlyargs) {
        return NULL;
    }
    posargs = (nposargs ? _Py_asdl_seq_new(nposargs, c->c_arena) : NULL);
    if (!posargs && nposargs)
        return NULL;
    kwonlyargs = (nkwonlyargs ?
                   _Py_asdl_seq_new(nkwonlyargs, c->c_arena) : NULL);
    if (!kwonlyargs && nkwonlyargs)
        return NULL;
    posdefaults = (nposdefaults ?
                    _Py_asdl_seq_new(nposdefaults, c->c_arena) : NULL);
    if (!posdefaults && nposdefaults)
        return NULL;
    /* The length of kwonlyargs and kwdefaults are same
       since we set NULL as default for keyword only argument w/o default
       - we have sequence data structure, but no dictionary */
    kwdefaults = (nkwonlyargs ?
                   _Py_asdl_seq_new(nkwonlyargs, c->c_arena) : NULL);
    if (!kwdefaults && nkwonlyargs)
        return NULL;

    /* tfpdef: NAME [':' test]
       vfpdef: NAME
    */
    i = 0;
    j = 0;  /* index for defaults */
    k = 0;  /* index for args */
    l = 0;  /* index for posonlyargs */
    while (i < NCH(n)) {
        ch = CHILD(n, i);
        switch (TYPE(ch)) {
            case tfpdef:
            case vfpdef:
                /* XXX Need to worry about checking if TYPE(CHILD(n, i+1)) is
                   anything other than EQUAL or a comma? */
                /* XXX Should NCH(n) check be made a separate check? */
                if (i + 1 < NCH(n) && TYPE(CHILD(n, i + 1)) == EQUAL) {
                    expr_ty expression = ast_for_expr(c, CHILD(n, i + 2));
                    if (!expression)
                        return NULL;
                    assert(posdefaults != NULL);
                    asdl_seq_SET(posdefaults, j++, expression);
                    i += 2;
                    found_default = 1;
                }
                else if (found_default) {
                    ast_error(c, n,
                              "non-default argument follows default argument");
                    return NULL;
                }
                arg = ast_for_arg(c, ch);
                if (!arg)
                    return NULL;
                if (l < nposonlyargs) {
                    asdl_seq_SET(posonlyargs, l++, arg);
                } else {
                    asdl_seq_SET(posargs, k++, arg);
                }
                i += 1; /* the name */
                if (i < NCH(n) && TYPE(CHILD(n, i)) == COMMA)
                    i += 1; /* the comma, if present */
                break;
             case SLASH:
                /* Advance the slash and the comma. If there are more names
                 * after the slash there will be a comma so we are advancing
                 * the correct number of nodes. If the slash is the last item,
                 * we will be advancing an extra token but then * i > NCH(n)
                 * and the enclosing while will finish correctly. */
                i += 2;
                break;
            case STAR:
                if (i+1 >= NCH(n) ||
                    (i+2 == NCH(n) && (TYPE(CHILD(n, i+1)) == COMMA
                                       || TYPE(CHILD(n, i+1)) == TYPE_COMMENT))) {
                    ast_error(c, CHILD(n, i),
                              "named arguments must follow bare *");
                    return NULL;
                }
                ch = CHILD(n, i+1);  /* tfpdef or COMMA */
                if (TYPE(ch) == COMMA) {
                    int res = 0;
                    i += 2; /* now follows keyword only arguments */

                    if (i < NCH(n) && TYPE(CHILD(n, i)) == TYPE_COMMENT) {
                        ast_error(c, CHILD(n, i),
                                  "bare * has associated type comment");
                        return NULL;
                    }

                    res = handle_keywordonly_args(c, n, i,
                                                  kwonlyargs, kwdefaults);
                    if (res == -1) return NULL;
                    i = res; /* res has new position to process */
                }
                else {
                    vararg = ast_for_arg(c, ch);
                    if (!vararg)
                        return NULL;

                i += 2; /* the star and the name */
                if (i < NCH(n) && TYPE(CHILD(n, i)) == COMMA)
                    i += 1; /* the comma, if present */

                if (i < NCH(n) && TYPE(CHILD(n, i)) == TYPE_COMMENT) {
                        vararg->type_comment = NEW_TYPE_COMMENT(CHILD(n, i));
                        if (!vararg->type_comment)
                            return NULL;
                        i += 1;
                    }

                    if (i < NCH(n) && (TYPE(CHILD(n, i)) == tfpdef
                                    || TYPE(CHILD(n, i)) == vfpdef)) {
                        int res = 0;
                        res = handle_keywordonly_args(c, n, i,
                                                      kwonlyargs, kwdefaults);
                        if (res == -1) return NULL;
                        i = res; /* res has new position to process */
                    }
                }
                break;
            case DOUBLESTAR:
                ch = CHILD(n, i+1);  /* tfpdef */
                assert(TYPE(ch) == tfpdef || TYPE(ch) == vfpdef);
                kwarg = ast_for_arg(c, ch);
                if (!kwarg)
                    return NULL;
                i += 2; /* the double star and the name */
                if (i < NCH(n) && TYPE(CHILD(n, i)) == COMMA)
                    i += 1; /* the comma, if present */
                break;
            case TYPE_COMMENT:
                assert(i);

                if (kwarg)
                    arg = kwarg;

                /* arg will be equal to the last argument processed */
                arg->type_comment = NEW_TYPE_COMMENT(ch);
                if (!arg->type_comment)
                    return NULL;
                i += 1;
                break;
            default:
                PyErr_Format(PyExc_SystemError,
                             "unexpected node in varargslist: %d @ %d",
                             TYPE(ch), i);
                return NULL;
        }
    }
    return arguments(posonlyargs, posargs, vararg, kwonlyargs, kwdefaults, kwarg, posdefaults, c->c_arena);
}

static expr_ty
ast_for_decorator(struct compiling *c, const node *n)
{
    /* decorator: '@' namedexpr_test NEWLINE */

    REQ(n, decorator);
    REQ(CHILD(n, 0), AT);
    REQ(CHILD(n, 2), NEWLINE);

    return ast_for_expr(c, CHILD(n, 1));
}

static asdl_seq*
ast_for_decorators(struct compiling *c, const node *n)
{
    asdl_seq* decorator_seq;
    expr_ty d;
    int i;

    REQ(n, decorators);
    decorator_seq = _Py_asdl_seq_new(NCH(n), c->c_arena);
    if (!decorator_seq)
        return NULL;

    for (i = 0; i < NCH(n); i++) {
        d = ast_for_decorator(c, CHILD(n, i));
        if (!d)
            return NULL;
        asdl_seq_SET(decorator_seq, i, d);
    }
    return decorator_seq;
}

static stmt_ty
ast_for_funcdef_impl(struct compiling *c, const node *n0,
                     asdl_seq *decorator_seq, bool is_async)
{
    /* funcdef: 'def' NAME parameters ['->' test] ':' [TYPE_COMMENT] suite */
    const node * const n = is_async ? CHILD(n0, 1) : n0;
    identifier name;
    arguments_ty args;
    asdl_seq *body;
    expr_ty returns = NULL;
    int name_i = 1;
    int end_lineno, end_col_offset;
    node *tc;
    string type_comment = NULL;

    if (is_async && c->c_feature_version < 5) {
        ast_error(c, n,
                  "Async functions are only supported in Python 3.5 and greater");
        return NULL;
    }

    REQ(n, funcdef);

    name = NEW_IDENTIFIER(CHILD(n, name_i));
    if (!name)
        return NULL;
    if (forbidden_name(c, name, CHILD(n, name_i), 0))
        return NULL;
    args = ast_for_arguments(c, CHILD(n, name_i + 1));
    if (!args)
        return NULL;
    if (TYPE(CHILD(n, name_i+2)) == RARROW) {
        returns = ast_for_expr(c, CHILD(n, name_i + 3));
        if (!returns)
            return NULL;
        name_i += 2;
    }
    if (TYPE(CHILD(n, name_i + 3)) == TYPE_COMMENT) {
        type_comment = NEW_TYPE_COMMENT(CHILD(n, name_i + 3));
        if (!type_comment)
            return NULL;
        name_i += 1;
    }
    body = ast_for_suite(c, CHILD(n, name_i + 3));
    if (!body)
        return NULL;
    get_last_end_pos(body, &end_lineno, &end_col_offset);

    if (NCH(CHILD(n, name_i + 3)) > 1) {
        /* Check if the suite has a type comment in it. */
        tc = CHILD(CHILD(n, name_i + 3), 1);

        if (TYPE(tc) == TYPE_COMMENT) {
            if (type_comment != NULL) {
                ast_error(c, n, "Cannot have two type comments on def");
                return NULL;
            }
            type_comment = NEW_TYPE_COMMENT(tc);
            if (!type_comment)
                return NULL;
        }
    }

    if (is_async)
        return AsyncFunctionDef(name, args, body, decorator_seq, returns, type_comment,
                                LINENO(n0), n0->n_col_offset, end_lineno, end_col_offset, c->c_arena);
    else
        return FunctionDef(name, args, body, decorator_seq, returns, type_comment,
                           LINENO(n), n->n_col_offset, end_lineno, end_col_offset, c->c_arena);
}

static stmt_ty
ast_for_async_funcdef(struct compiling *c, const node *n, asdl_seq *decorator_seq)
{
    /* async_funcdef: ASYNC funcdef */
    REQ(n, async_funcdef);
    REQ(CHILD(n, 0), ASYNC);
    REQ(CHILD(n, 1), funcdef);

    return ast_for_funcdef_impl(c, n, decorator_seq,
                                true /* is_async */);
}

static stmt_ty
ast_for_funcdef(struct compiling *c, const node *n, asdl_seq *decorator_seq)
{
    /* funcdef: 'def' NAME parameters ['->' test] ':' suite */
    return ast_for_funcdef_impl(c, n, decorator_seq,
                                false /* is_async */);
}


static stmt_ty
ast_for_async_stmt(struct compiling *c, const node *n)
{
    /* async_stmt: ASYNC (funcdef | with_stmt | for_stmt) */
    REQ(n, async_stmt);
    REQ(CHILD(n, 0), ASYNC);

    switch (TYPE(CHILD(n, 1))) {
        case funcdef:
            return ast_for_funcdef_impl(c, n, NULL,
                                        true /* is_async */);
        case with_stmt:
            return ast_for_with_stmt(c, n,
                                     true /* is_async */);

        case for_stmt:
            return ast_for_for_stmt(c, n,
                                    true /* is_async */);

        default:
            PyErr_Format(PyExc_SystemError,
                         "invalid async stament: %s",
                         STR(CHILD(n, 1)));
            return NULL;
    }
}

static stmt_ty
ast_for_decorated(struct compiling *c, const node *n)
{
    /* decorated: decorators (classdef | funcdef | async_funcdef) */
    stmt_ty thing = NULL;
    asdl_seq *decorator_seq = NULL;

    REQ(n, decorated);

    decorator_seq = ast_for_decorators(c, CHILD(n, 0));
    if (!decorator_seq)
      return NULL;

    assert(TYPE(CHILD(n, 1)) == funcdef ||
           TYPE(CHILD(n, 1)) == async_funcdef ||
           TYPE(CHILD(n, 1)) == classdef);

    if (TYPE(CHILD(n, 1)) == funcdef) {
      thing = ast_for_funcdef(c, CHILD(n, 1), decorator_seq);
    } else if (TYPE(CHILD(n, 1)) == classdef) {
      thing = ast_for_classdef(c, CHILD(n, 1), decorator_seq);
    } else if (TYPE(CHILD(n, 1)) == async_funcdef) {
      thing = ast_for_async_funcdef(c, CHILD(n, 1), decorator_seq);
    }
    return thing;
}

static expr_ty
ast_for_namedexpr(struct compiling *c, const node *n)
{
    /* namedexpr_test: test [':=' test]
       argument: ( test [comp_for] |
            test ':=' test |
            test '=' test |
            '**' test |
            '*' test )
    */
    expr_ty target, value;

    target = ast_for_expr(c, CHILD(n, 0));
    if (!target)
        return NULL;

    value = ast_for_expr(c, CHILD(n, 2));
    if (!value)
        return NULL;

    if (target->kind != Name_kind) {
        const char *expr_name = get_expr_name(target);
        if (expr_name != NULL) {
            ast_error(c, n, "cannot use assignment expressions with %s", expr_name);
        }
        return NULL;
    }

    if (!set_context(c, target, Store, n))
        return NULL;

    return NamedExpr(target, value, LINENO(n), n->n_col_offset, n->n_end_lineno,
                     n->n_end_col_offset, c->c_arena);
}

static expr_ty
ast_for_lambdef(struct compiling *c, const node *n)
{
    /* lambdef: 'lambda' [varargslist] ':' test
       lambdef_nocond: 'lambda' [varargslist] ':' test_nocond */
    arguments_ty args;
    expr_ty expression;

    if (NCH(n) == 3) {
        args = arguments(NULL, NULL, NULL, NULL, NULL, NULL, NULL, c->c_arena);
        if (!args)
            return NULL;
        expression = ast_for_expr(c, CHILD(n, 2));
        if (!expression)
            return NULL;
    }
    else {
        args = ast_for_arguments(c, CHILD(n, 1));
        if (!args)
            return NULL;
        expression = ast_for_expr(c, CHILD(n, 3));
        if (!expression)
            return NULL;
    }

    return Lambda(args, expression, LINENO(n), n->n_col_offset,
                  n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

static expr_ty
ast_for_ifexpr(struct compiling *c, const node *n)
{
    /* test: or_test 'if' or_test 'else' test */
    expr_ty expression, body, orelse;

    assert(NCH(n) == 5);
    body = ast_for_expr(c, CHILD(n, 0));
    if (!body)
        return NULL;
    expression = ast_for_expr(c, CHILD(n, 2));
    if (!expression)
        return NULL;
    orelse = ast_for_expr(c, CHILD(n, 4));
    if (!orelse)
        return NULL;
    return IfExp(expression, body, orelse, LINENO(n), n->n_col_offset,
                 n->n_end_lineno, n->n_end_col_offset,
                 c->c_arena);
}

/*
   Count the number of 'for' loops in a comprehension.

   Helper for ast_for_comprehension().
*/

static int
count_comp_fors(struct compiling *c, const node *n)
{
    int n_fors = 0;

  count_comp_for:
    n_fors++;
    REQ(n, comp_for);
    if (NCH(n) == 2) {
        REQ(CHILD(n, 0), ASYNC);
        n = CHILD(n, 1);
    }
    else if (NCH(n) == 1) {
        n = CHILD(n, 0);
    }
    else {
        goto error;
    }
    if (NCH(n) == (5)) {
        n = CHILD(n, 4);
    }
    else {
        return n_fors;
    }
  count_comp_iter:
    REQ(n, comp_iter);
    n = CHILD(n, 0);
    if (TYPE(n) == comp_for)
        goto count_comp_for;
    else if (TYPE(n) == comp_if) {
        if (NCH(n) == 3) {
            n = CHILD(n, 2);
            goto count_comp_iter;
        }
        else
            return n_fors;
    }

  error:
    /* Should never be reached */
    PyErr_SetString(PyExc_SystemError,
                    "logic error in count_comp_fors");
    return -1;
}

/* Count the number of 'if' statements in a comprehension.

   Helper for ast_for_comprehension().
*/

static int
count_comp_ifs(struct compiling *c, const node *n)
{
    int n_ifs = 0;

    while (1) {
        REQ(n, comp_iter);
        if (TYPE(CHILD(n, 0)) == comp_for)
            return n_ifs;
        n = CHILD(n, 0);
        REQ(n, comp_if);
        n_ifs++;
        if (NCH(n) == 2)
            return n_ifs;
        n = CHILD(n, 2);
    }
}

static asdl_seq *
ast_for_comprehension(struct compiling *c, const node *n)
{
    int i, n_fors;
    asdl_seq *comps;

    n_fors = count_comp_fors(c, n);
    if (n_fors == -1)
        return NULL;

    comps = _Py_asdl_seq_new(n_fors, c->c_arena);
    if (!comps)
        return NULL;

    for (i = 0; i < n_fors; i++) {
        comprehension_ty comp;
        asdl_seq *t;
        expr_ty expression, first;
        node *for_ch;
        node *sync_n;
        int is_async = 0;

        REQ(n, comp_for);

        if (NCH(n) == 2) {
            is_async = 1;
            REQ(CHILD(n, 0), ASYNC);
            sync_n = CHILD(n, 1);
        }
        else {
            sync_n = CHILD(n, 0);
        }
        REQ(sync_n, sync_comp_for);

        /* Async comprehensions only allowed in Python 3.6 and greater */
        if (is_async && c->c_feature_version < 6) {
            ast_error(c, n,
                      "Async comprehensions are only supported in Python 3.6 and greater");
            return NULL;
        }

        for_ch = CHILD(sync_n, 1);
        t = ast_for_exprlist(c, for_ch, Store);
        if (!t)
            return NULL;
        expression = ast_for_expr(c, CHILD(sync_n, 3));
        if (!expression)
            return NULL;

        /* Check the # of children rather than the length of t, since
           (x for x, in ...) has 1 element in t, but still requires a Tuple. */
        first = (expr_ty)asdl_seq_GET(t, 0);
        if (NCH(for_ch) == 1)
            comp = comprehension(first, expression, NULL,
                                 is_async, c->c_arena);
        else
            comp = comprehension(Tuple(t, Store, first->lineno, first->col_offset,
                                       for_ch->n_end_lineno, for_ch->n_end_col_offset,
                                       c->c_arena),
                                 expression, NULL, is_async, c->c_arena);
        if (!comp)
            return NULL;

        if (NCH(sync_n) == 5) {
            int j, n_ifs;
            asdl_seq *ifs;

            n = CHILD(sync_n, 4);
            n_ifs = count_comp_ifs(c, n);
            if (n_ifs == -1)
                return NULL;

            ifs = _Py_asdl_seq_new(n_ifs, c->c_arena);
            if (!ifs)
                return NULL;

            for (j = 0; j < n_ifs; j++) {
                REQ(n, comp_iter);
                n = CHILD(n, 0);
                REQ(n, comp_if);

                expression = ast_for_expr(c, CHILD(n, 1));
                if (!expression)
                    return NULL;
                asdl_seq_SET(ifs, j, expression);
                if (NCH(n) == 3)
                    n = CHILD(n, 2);
            }
            /* on exit, must guarantee that n is a comp_for */
            if (TYPE(n) == comp_iter)
                n = CHILD(n, 0);
            comp->ifs = ifs;
        }
        asdl_seq_SET(comps, i, comp);
    }
    return comps;
}

static expr_ty
ast_for_itercomp(struct compiling *c, const node *n, int type)
{
    /* testlist_comp: (test|star_expr)
     *                ( comp_for | (',' (test|star_expr))* [','] ) */
    expr_ty elt;
    asdl_seq *comps;
    node *ch;

    assert(NCH(n) > 1);

    ch = CHILD(n, 0);
    elt = ast_for_expr(c, ch);
    if (!elt)
        return NULL;
    if (elt->kind == Starred_kind) {
        ast_error(c, ch, "iterable unpacking cannot be used in comprehension");
        return NULL;
    }

    comps = ast_for_comprehension(c, CHILD(n, 1));
    if (!comps)
        return NULL;

    if (type == COMP_GENEXP)
        return GeneratorExp(elt, comps, LINENO(n), n->n_col_offset,
                            n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    else if (type == COMP_LISTCOMP)
        return ListComp(elt, comps, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    else if (type == COMP_SETCOMP)
        return SetComp(elt, comps, LINENO(n), n->n_col_offset,
                       n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    else
        /* Should never happen */
        return NULL;
}

/* Fills in the key, value pair corresponding to the dict element.  In case
 * of an unpacking, key is NULL.  *i is advanced by the number of ast
 * elements.  Iff successful, nonzero is returned.
 */
static int
ast_for_dictelement(struct compiling *c, const node *n, int *i,
                    expr_ty *key, expr_ty *value)
{
    expr_ty expression;
    if (TYPE(CHILD(n, *i)) == DOUBLESTAR) {
        assert(NCH(n) - *i >= 2);

        expression = ast_for_expr(c, CHILD(n, *i + 1));
        if (!expression)
            return 0;
        *key = NULL;
        *value = expression;

        *i += 2;
    }
    else {
        assert(NCH(n) - *i >= 3);

        expression = ast_for_expr(c, CHILD(n, *i));
        if (!expression)
            return 0;
        *key = expression;

        REQ(CHILD(n, *i + 1), COLON);

        expression = ast_for_expr(c, CHILD(n, *i + 2));
        if (!expression)
            return 0;
        *value = expression;

        *i += 3;
    }
    return 1;
}

static expr_ty
ast_for_dictcomp(struct compiling *c, const node *n)
{
    expr_ty key, value;
    asdl_seq *comps;
    int i = 0;

    if (!ast_for_dictelement(c, n, &i, &key, &value))
        return NULL;
    assert(key);
    assert(NCH(n) - i >= 1);

    comps = ast_for_comprehension(c, CHILD(n, i));
    if (!comps)
        return NULL;

    return DictComp(key, value, comps, LINENO(n), n->n_col_offset,
                    n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

static expr_ty
ast_for_dictdisplay(struct compiling *c, const node *n)
{
    int i;
    int j;
    int size;
    asdl_seq *keys, *values;

    size = (NCH(n) + 1) / 3; /* +1 in case no trailing comma */
    keys = _Py_asdl_seq_new(size, c->c_arena);
    if (!keys)
        return NULL;

    values = _Py_asdl_seq_new(size, c->c_arena);
    if (!values)
        return NULL;

    j = 0;
    for (i = 0; i < NCH(n); i++) {
        expr_ty key, value;

        if (!ast_for_dictelement(c, n, &i, &key, &value))
            return NULL;
        asdl_seq_SET(keys, j, key);
        asdl_seq_SET(values, j, value);

        j++;
    }
    keys->size = j;
    values->size = j;
    return Dict(keys, values, LINENO(n), n->n_col_offset,
                n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

static expr_ty
ast_for_genexp(struct compiling *c, const node *n)
{
    assert(TYPE(n) == (testlist_comp) || TYPE(n) == (argument));
    return ast_for_itercomp(c, n, COMP_GENEXP);
}

static expr_ty
ast_for_listcomp(struct compiling *c, const node *n)
{
    assert(TYPE(n) == (testlist_comp));
    return ast_for_itercomp(c, n, COMP_LISTCOMP);
}

static expr_ty
ast_for_setcomp(struct compiling *c, const node *n)
{
    assert(TYPE(n) == (dictorsetmaker));
    return ast_for_itercomp(c, n, COMP_SETCOMP);
}

static expr_ty
ast_for_setdisplay(struct compiling *c, const node *n)
{
    int i;
    int size;
    asdl_seq *elts;

    assert(TYPE(n) == (dictorsetmaker));
    size = (NCH(n) + 1) / 2; /* +1 in case no trailing comma */
    elts = _Py_asdl_seq_new(size, c->c_arena);
    if (!elts)
        return NULL;
    for (i = 0; i < NCH(n); i += 2) {
        expr_ty expression;
        expression = ast_for_expr(c, CHILD(n, i));
        if (!expression)
            return NULL;
        asdl_seq_SET(elts, i / 2, expression);
    }
    return Set(elts, LINENO(n), n->n_col_offset,
               n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

static expr_ty
ast_for_atom(struct compiling *c, const node *n)
{
    /* atom: '(' [yield_expr|testlist_comp] ')' | '[' [testlist_comp] ']'
       | '{' [dictmaker|testlist_comp] '}' | NAME | NUMBER | STRING+
       | '...' | 'None' | 'True' | 'False'
    */
    node *ch = CHILD(n, 0);

    switch (TYPE(ch)) {
    case NAME: {
        PyObject *name;
        const char *s = STR(ch);
        size_t len = strlen(s);
        if (len >= 4 && len <= 5) {
            if (!strcmp(s, "None"))
                return Constant(Py_None, NULL, LINENO(n), n->n_col_offset,
                                n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            if (!strcmp(s, "True"))
                return Constant(Py_True, NULL, LINENO(n), n->n_col_offset,
                                n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            if (!strcmp(s, "False"))
                return Constant(Py_False, NULL, LINENO(n), n->n_col_offset,
                                n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        name = new_identifier(s, c);
        if (!name)
            return NULL;
        /* All names start in Load context, but may later be changed. */
        return Name(name, Load, LINENO(n), n->n_col_offset,
                    n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    case STRING: {
        expr_ty str = parsestrplus(c, n);
        if (!str) {
            const char *errtype = NULL;
            if (PyErr_ExceptionMatches(PyExc_UnicodeError))
                errtype = "unicode error";
            else if (PyErr_ExceptionMatches(PyExc_ValueError))
                errtype = "value error";
            if (errtype) {
                PyObject *type, *value, *tback, *errstr;
                PyErr_Fetch(&type, &value, &tback);
                errstr = PyObject_Str(value);
                if (errstr) {
                    ast_error(c, n, "(%s) %U", errtype, errstr);
                    Py_DECREF(errstr);
                }
                else {
                    PyErr_Clear();
                    ast_error(c, n, "(%s) unknown error", errtype);
                }
                Py_DECREF(type);
                Py_XDECREF(value);
                Py_XDECREF(tback);
            }
            return NULL;
        }
        return str;
    }
    case NUMBER: {
        PyObject *pynum;
        /* Underscores in numeric literals are only allowed in Python 3.6 or greater */
        /* Check for underscores here rather than in parse_number so we can report a line number on error */
        if (c->c_feature_version < 6 && strchr(STR(ch), '_') != NULL) {
            ast_error(c, ch,
                      "Underscores in numeric literals are only supported in Python 3.6 and greater");
            return NULL;
        }
        pynum = parsenumber(c, STR(ch));
        if (!pynum)
            return NULL;

        if (PyArena_AddPyObject(c->c_arena, pynum) < 0) {
            Py_DECREF(pynum);
            return NULL;
        }
        return Constant(pynum, NULL, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    case ELLIPSIS: /* Ellipsis */
        return Constant(Py_Ellipsis, NULL, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    case LPAR: /* some parenthesized expressions */
        ch = CHILD(n, 1);

        if (TYPE(ch) == RPAR)
            return Tuple(NULL, Load, LINENO(n), n->n_col_offset,
                         n->n_end_lineno, n->n_end_col_offset, c->c_arena);

        if (TYPE(ch) == yield_expr)
            return ast_for_expr(c, ch);

        /* testlist_comp: test ( comp_for | (',' test)* [','] ) */
        if (NCH(ch) == 1) {
            return ast_for_testlist(c, ch);
        }

        if (TYPE(CHILD(ch, 1)) == comp_for) {
            return copy_location(ast_for_genexp(c, ch), n, n);
        }
        else {
            return copy_location(ast_for_testlist(c, ch), n, n);
        }
    case LSQB: /* list (or list comprehension) */
        ch = CHILD(n, 1);

        if (TYPE(ch) == RSQB)
            return List(NULL, Load, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);

        REQ(ch, testlist_comp);
        if (NCH(ch) == 1 || TYPE(CHILD(ch, 1)) == COMMA) {
            asdl_seq *elts = seq_for_testlist(c, ch);
            if (!elts)
                return NULL;

            return List(elts, Load, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        else {
            return copy_location(ast_for_listcomp(c, ch), n, n);
        }
    case LBRACE: {
        /* dictorsetmaker: ( ((test ':' test | '**' test)
         *                    (comp_for | (',' (test ':' test | '**' test))* [','])) |
         *                   ((test | '*' test)
         *                    (comp_for | (',' (test | '*' test))* [','])) ) */
        expr_ty res;
        ch = CHILD(n, 1);
        if (TYPE(ch) == RBRACE) {
            /* It's an empty dict. */
            return Dict(NULL, NULL, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        else {
            int is_dict = (TYPE(CHILD(ch, 0)) == DOUBLESTAR);
            if (NCH(ch) == 1 ||
                    (NCH(ch) > 1 &&
                     TYPE(CHILD(ch, 1)) == COMMA)) {
                /* It's a set display. */
                res = ast_for_setdisplay(c, ch);
            }
            else if (NCH(ch) > 1 &&
                    TYPE(CHILD(ch, 1)) == comp_for) {
                /* It's a set comprehension. */
                res = ast_for_setcomp(c, ch);
            }
            else if (NCH(ch) > 3 - is_dict &&
                    TYPE(CHILD(ch, 3 - is_dict)) == comp_for) {
                /* It's a dictionary comprehension. */
                if (is_dict) {
                    ast_error(c, n,
                              "dict unpacking cannot be used in dict comprehension");
                    return NULL;
                }
                res = ast_for_dictcomp(c, ch);
            }
            else {
                /* It's a dictionary display. */
                res = ast_for_dictdisplay(c, ch);
            }
            return copy_location(res, n, n);
        }
    }
    default:
        PyErr_Format(PyExc_SystemError, "unhandled atom %d", TYPE(ch));
        return NULL;
    }
}

static expr_ty
ast_for_slice(struct compiling *c, const node *n)
{
    node *ch;
    expr_ty lower = NULL, upper = NULL, step = NULL;

    REQ(n, subscript);

    /*
       subscript: test | [test] ':' [test] [sliceop]
       sliceop: ':' [test]
    */
    ch = CHILD(n, 0);
    if (NCH(n) == 1 && TYPE(ch) == test) {
        return ast_for_expr(c, ch);
    }

    if (TYPE(ch) == test) {
        lower = ast_for_expr(c, ch);
        if (!lower)
            return NULL;
    }

    /* If there's an upper bound it's in the second or third position. */
    if (TYPE(ch) == COLON) {
        if (NCH(n) > 1) {
            node *n2 = CHILD(n, 1);

            if (TYPE(n2) == test) {
                upper = ast_for_expr(c, n2);
                if (!upper)
                    return NULL;
            }
        }
    } else if (NCH(n) > 2) {
        node *n2 = CHILD(n, 2);

        if (TYPE(n2) == test) {
            upper = ast_for_expr(c, n2);
            if (!upper)
                return NULL;
        }
    }

    ch = CHILD(n, NCH(n) - 1);
    if (TYPE(ch) == sliceop) {
        if (NCH(ch) != 1) {
            ch = CHILD(ch, 1);
            if (TYPE(ch) == test) {
                step = ast_for_expr(c, ch);
                if (!step)
                    return NULL;
            }
        }
    }

    return Slice(lower, upper, step, LINENO(n), n->n_col_offset,
                 n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

static expr_ty
ast_for_binop(struct compiling *c, const node *n)
{
    /* Must account for a sequence of expressions.
       How should A op B op C by represented?
       BinOp(BinOp(A, op, B), op, C).
    */

    int i, nops;
    expr_ty expr1, expr2, result;
    operator_ty newoperator;

    expr1 = ast_for_expr(c, CHILD(n, 0));
    if (!expr1)
        return NULL;

    expr2 = ast_for_expr(c, CHILD(n, 2));
    if (!expr2)
        return NULL;

    newoperator = get_operator(c, CHILD(n, 1));
    if (!newoperator)
        return NULL;

    result = BinOp(expr1, newoperator, expr2, LINENO(n), n->n_col_offset,
                   CHILD(n, 2)->n_end_lineno, CHILD(n, 2)->n_end_col_offset,
                   c->c_arena);
    if (!result)
        return NULL;

    nops = (NCH(n) - 1) / 2;
    for (i = 1; i < nops; i++) {
        expr_ty tmp_result, tmp;
        const node* next_oper = CHILD(n, i * 2 + 1);

        newoperator = get_operator(c, next_oper);
        if (!newoperator)
            return NULL;

        tmp = ast_for_expr(c, CHILD(n, i * 2 + 2));
        if (!tmp)
            return NULL;

        tmp_result = BinOp(result, newoperator, tmp,
                           LINENO(n), n->n_col_offset,
                           CHILD(n, i * 2 + 2)->n_end_lineno,
                           CHILD(n, i * 2 + 2)->n_end_col_offset,
                           c->c_arena);
        if (!tmp_result)
            return NULL;
        result = tmp_result;
    }
    return result;
}

static expr_ty
ast_for_trailer(struct compiling *c, const node *n, expr_ty left_expr, const node *start)
{
    /* trailer: '(' [arglist] ')' | '[' subscriptlist ']' | '.' NAME
       subscriptlist: subscript (',' subscript)* [',']
       subscript: '.' '.' '.' | test | [test] ':' [test] [sliceop]
     */
    const node *n_copy = n;
    REQ(n, trailer);
    if (TYPE(CHILD(n, 0)) == LPAR) {
        if (NCH(n) == 2)
            return Call(left_expr, NULL, NULL, LINENO(start), start->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        else
            return ast_for_call(c, CHILD(n, 1), left_expr,
                                start, CHILD(n, 0), CHILD(n, 2));
    }
    else if (TYPE(CHILD(n, 0)) == DOT) {
        PyObject *attr_id = NEW_IDENTIFIER(CHILD(n, 1));
        if (!attr_id)
            return NULL;
        return Attribute(left_expr, attr_id, Load,
                         LINENO(start), start->n_col_offset,
                         n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else {
        REQ(CHILD(n, 0), LSQB);
        REQ(CHILD(n, 2), RSQB);
        n = CHILD(n, 1);
        if (NCH(n) == 1) {
            expr_ty slc = ast_for_slice(c, CHILD(n, 0));
            if (!slc)
                return NULL;
            return Subscript(left_expr, slc, Load, LINENO(start), start->n_col_offset,
                             n_copy->n_end_lineno, n_copy->n_end_col_offset,
                             c->c_arena);
        }
        else {
            int j;
            expr_ty slc, e;
            asdl_seq *elts;
            elts = _Py_asdl_seq_new((NCH(n) + 1) / 2, c->c_arena);
            if (!elts)
                return NULL;
            for (j = 0; j < NCH(n); j += 2) {
                slc = ast_for_slice(c, CHILD(n, j));
                if (!slc)
                    return NULL;
                asdl_seq_SET(elts, j / 2, slc);
            }
            e = Tuple(elts, Load, LINENO(n), n->n_col_offset,
                      n->n_end_lineno, n->n_end_col_offset,
                      c->c_arena);
            if (!e)
                return NULL;
            return Subscript(left_expr, e,
                             Load, LINENO(start), start->n_col_offset,
                             n_copy->n_end_lineno, n_copy->n_end_col_offset,
                             c->c_arena);
        }
    }
}

static expr_ty
ast_for_factor(struct compiling *c, const node *n)
{
    expr_ty expression;

    expression = ast_for_expr(c, CHILD(n, 1));
    if (!expression)
        return NULL;

    switch (TYPE(CHILD(n, 0))) {
        case PLUS:
            return UnaryOp(UAdd, expression, LINENO(n), n->n_col_offset,
                           n->n_end_lineno, n->n_end_col_offset,
                           c->c_arena);
        case MINUS:
            return UnaryOp(USub, expression, LINENO(n), n->n_col_offset,
                           n->n_end_lineno, n->n_end_col_offset,
                           c->c_arena);
        case TILDE:
            return UnaryOp(Invert, expression, LINENO(n), n->n_col_offset,
                           n->n_end_lineno, n->n_end_col_offset,
                           c->c_arena);
    }
    PyErr_Format(PyExc_SystemError, "unhandled factor: %d",
                 TYPE(CHILD(n, 0)));
    return NULL;
}

static expr_ty
ast_for_atom_expr(struct compiling *c, const node *n)
{
    int i, nch, start = 0;
    expr_ty e;

    REQ(n, atom_expr);
    nch = NCH(n);

    if (TYPE(CHILD(n, 0)) == AWAIT) {
        if (c->c_feature_version < 5) {
            ast_error(c, n,
                      "Await expressions are only supported in Python 3.5 and greater");
            return NULL;
        }
        start = 1;
        assert(nch > 1);
    }

    e = ast_for_atom(c, CHILD(n, start));
    if (!e)
        return NULL;
    if (nch == 1)
        return e;
    if (start && nch == 2) {
        return Await(e, LINENO(n), n->n_col_offset,
                     n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }

    for (i = start + 1; i < nch; i++) {
        node *ch = CHILD(n, i);
        if (TYPE(ch) != trailer)
            break;
        e = ast_for_trailer(c, ch, e, CHILD(n, start));
        if (!e)
            return NULL;
    }

    if (start) {
        /* there was an 'await' */
        return Await(e, LINENO(n), n->n_col_offset,
                     n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else {
        return e;
    }
}

static expr_ty
ast_for_power(struct compiling *c, const node *n)
{
    /* power: atom trailer* ('**' factor)*
     */
    expr_ty e;
    REQ(n, power);
    e = ast_for_atom_expr(c, CHILD(n, 0));
    if (!e)
        return NULL;
    if (NCH(n) == 1)
        return e;
    if (TYPE(CHILD(n, NCH(n) - 1)) == factor) {
        expr_ty f = ast_for_expr(c, CHILD(n, NCH(n) - 1));
        if (!f)
            return NULL;
        e = BinOp(e, Pow, f, LINENO(n), n->n_col_offset,
                  n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    return e;
}

static expr_ty
ast_for_starred(struct compiling *c, const node *n)
{
    expr_ty tmp;
    REQ(n, star_expr);

    tmp = ast_for_expr(c, CHILD(n, 1));
    if (!tmp)
        return NULL;

    /* The Load context is changed later. */
    return Starred(tmp, Load, LINENO(n), n->n_col_offset,
                   n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}


/* Do not name a variable 'expr'!  Will cause a compile error.
*/

static expr_ty
ast_for_expr(struct compiling *c, const node *n)
{
    /* handle the full range of simple expressions
       namedexpr_test: test [':=' test]
       test: or_test ['if' or_test 'else' test] | lambdef
       test_nocond: or_test | lambdef_nocond
       or_test: and_test ('or' and_test)*
       and_test: not_test ('and' not_test)*
       not_test: 'not' not_test | comparison
       comparison: expr (comp_op expr)*
       expr: xor_expr ('|' xor_expr)*
       xor_expr: and_expr ('^' and_expr)*
       and_expr: shift_expr ('&' shift_expr)*
       shift_expr: arith_expr (('<<'|'>>') arith_expr)*
       arith_expr: term (('+'|'-') term)*
       term: factor (('*'|'@'|'/'|'%'|'//') factor)*
       factor: ('+'|'-'|'~') factor | power
       power: atom_expr ['**' factor]
       atom_expr: [AWAIT] atom trailer*
       yield_expr: 'yield' [yield_arg]
    */

    asdl_seq *seq;
    int i;

 loop:
    switch (TYPE(n)) {
        case namedexpr_test:
            if (NCH(n) == 3)
                return ast_for_namedexpr(c, n);
            /* Fallthrough */
        case test:
        case test_nocond:
            if (TYPE(CHILD(n, 0)) == lambdef ||
                TYPE(CHILD(n, 0)) == lambdef_nocond)
                return ast_for_lambdef(c, CHILD(n, 0));
            else if (NCH(n) > 1)
                return ast_for_ifexpr(c, n);
            /* Fallthrough */
        case or_test:
        case and_test:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            seq = _Py_asdl_seq_new((NCH(n) + 1) / 2, c->c_arena);
            if (!seq)
                return NULL;
            for (i = 0; i < NCH(n); i += 2) {
                expr_ty e = ast_for_expr(c, CHILD(n, i));
                if (!e)
                    return NULL;
                asdl_seq_SET(seq, i / 2, e);
            }
            if (!strcmp(STR(CHILD(n, 1)), "and"))
                return BoolOp(And, seq, LINENO(n), n->n_col_offset,
                              n->n_end_lineno, n->n_end_col_offset,
                              c->c_arena);
            assert(!strcmp(STR(CHILD(n, 1)), "or"));
            return BoolOp(Or, seq, LINENO(n), n->n_col_offset,
                          n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        case not_test:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            else {
                expr_ty expression = ast_for_expr(c, CHILD(n, 1));
                if (!expression)
                    return NULL;

                return UnaryOp(Not, expression, LINENO(n), n->n_col_offset,
                               n->n_end_lineno, n->n_end_col_offset,
                               c->c_arena);
            }
        case comparison:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            else {
                expr_ty expression;
                asdl_int_seq *ops;
                asdl_seq *cmps;
                ops = _Py_asdl_int_seq_new(NCH(n) / 2, c->c_arena);
                if (!ops)
                    return NULL;
                cmps = _Py_asdl_seq_new(NCH(n) / 2, c->c_arena);
                if (!cmps) {
                    return NULL;
                }
                for (i = 1; i < NCH(n); i += 2) {
                    cmpop_ty newoperator;

                    newoperator = ast_for_comp_op(c, CHILD(n, i));
                    if (!newoperator) {
                        return NULL;
                    }

                    expression = ast_for_expr(c, CHILD(n, i + 1));
                    if (!expression) {
                        return NULL;
                    }

                    asdl_seq_SET(ops, i / 2, newoperator);
                    asdl_seq_SET(cmps, i / 2, expression);
                }
                expression = ast_for_expr(c, CHILD(n, 0));
                if (!expression) {
                    return NULL;
                }

                return Compare(expression, ops, cmps, LINENO(n), n->n_col_offset,
                               n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            }

        case star_expr:
            return ast_for_starred(c, n);
        /* The next five cases all handle BinOps.  The main body of code
           is the same in each case, but the switch turned inside out to
           reuse the code for each type of operator.
         */
        case expr:
        case xor_expr:
        case and_expr:
        case shift_expr:
        case arith_expr:
        case term:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            return ast_for_binop(c, n);
        case yield_expr: {
            node *an = NULL;
            node *en = NULL;
            int is_from = 0;
            expr_ty exp = NULL;
            if (NCH(n) > 1)
                an = CHILD(n, 1); /* yield_arg */
            if (an) {
                en = CHILD(an, NCH(an) - 1);
                if (NCH(an) == 2) {
                    is_from = 1;
                    exp = ast_for_expr(c, en);
                }
                else
                    exp = ast_for_testlist(c, en);
                if (!exp)
                    return NULL;
            }
            if (is_from)
                return YieldFrom(exp, LINENO(n), n->n_col_offset,
                                 n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            return Yield(exp, LINENO(n), n->n_col_offset,
                         n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        case factor:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            return ast_for_factor(c, n);
        case power:
            return ast_for_power(c, n);
        default:
            PyErr_Format(PyExc_SystemError, "unhandled expr: %d", TYPE(n));
            return NULL;
    }
    /* should never get here unless if error is set */
    return NULL;
}

static expr_ty
ast_for_call(struct compiling *c, const node *n, expr_ty func,
             const node *start, const node *maybegenbeg, const node *closepar)
{
    /*
      arglist: argument (',' argument)*  [',']
      argument: ( test [comp_for] | '*' test | test '=' test | '**' test )
    */

    int i, nargs, nkeywords;
    int ndoublestars;
    asdl_seq *args;
    asdl_seq *keywords;

    REQ(n, arglist);

    nargs = 0;
    nkeywords = 0;
    for (i = 0; i < NCH(n); i++) {
        node *ch = CHILD(n, i);
        if (TYPE(ch) == argument) {
            if (NCH(ch) == 1)
                nargs++;
            else if (TYPE(CHILD(ch, 1)) == comp_for) {
                nargs++;
                if (!maybegenbeg) {
                    ast_error(c, ch, "invalid syntax");
                    return NULL;
                }
                if (NCH(n) > 1) {
                    ast_error(c, ch, "Generator expression must be parenthesized");
                    return NULL;
                }
            }
            else if (TYPE(CHILD(ch, 0)) == STAR)
                nargs++;
            else if (TYPE(CHILD(ch, 1)) == COLONEQUAL) {
                nargs++;
            }
            else
                /* TYPE(CHILD(ch, 0)) == DOUBLESTAR or keyword argument */
                nkeywords++;
        }
    }

    args = _Py_asdl_seq_new(nargs, c->c_arena);
    if (!args)
        return NULL;
    keywords = _Py_asdl_seq_new(nkeywords, c->c_arena);
    if (!keywords)
        return NULL;

    nargs = 0;  /* positional arguments + iterable argument unpackings */
    nkeywords = 0;  /* keyword arguments + keyword argument unpackings */
    ndoublestars = 0;  /* just keyword argument unpackings */
    for (i = 0; i < NCH(n); i++) {
        node *ch = CHILD(n, i);
        if (TYPE(ch) == argument) {
            expr_ty e;
            node *chch = CHILD(ch, 0);
            if (NCH(ch) == 1) {
                /* a positional argument */
                if (nkeywords) {
                    if (ndoublestars) {
                        ast_error(c, chch,
                                  "positional argument follows "
                                  "keyword argument unpacking");
                    }
                    else {
                        ast_error(c, chch,
                                  "positional argument follows "
                                  "keyword argument");
                    }
                    return NULL;
                }
                e = ast_for_expr(c, chch);
                if (!e)
                    return NULL;
                asdl_seq_SET(args, nargs++, e);
            }
            else if (TYPE(chch) == STAR) {
                /* an iterable argument unpacking */
                expr_ty starred;
                if (ndoublestars) {
                    ast_error(c, chch,
                              "iterable argument unpacking follows "
                              "keyword argument unpacking");
                    return NULL;
                }
                e = ast_for_expr(c, CHILD(ch, 1));
                if (!e)
                    return NULL;
                starred = Starred(e, Load, LINENO(chch),
                        chch->n_col_offset,
                        e->end_lineno, e->end_col_offset,
                        c->c_arena);
                if (!starred)
                    return NULL;
                asdl_seq_SET(args, nargs++, starred);

            }
            else if (TYPE(chch) == DOUBLESTAR) {
                /* a keyword argument unpacking */
                keyword_ty kw;
                i++;
                e = ast_for_expr(c, CHILD(ch, 1));
                if (!e)
                    return NULL;
                kw = keyword(NULL, e, c->c_arena);
                asdl_seq_SET(keywords, nkeywords++, kw);
                ndoublestars++;
            }
            else if (TYPE(CHILD(ch, 1)) == comp_for) {
                /* the lone generator expression */
                e = copy_location(ast_for_genexp(c, ch), maybegenbeg, closepar);
                if (!e)
                    return NULL;
                asdl_seq_SET(args, nargs++, e);
            }
            else if (TYPE(CHILD(ch, 1)) == COLONEQUAL) {
                /* treat colon equal as positional argument */
                if (nkeywords) {
                    if (ndoublestars) {
                        ast_error(c, chch,
                                  "positional argument follows "
                                  "keyword argument unpacking");
                    }
                    else {
                        ast_error(c, chch,
                                  "positional argument follows "
                                  "keyword argument");
                    }
                    return NULL;
                }
                e = ast_for_namedexpr(c, ch);
                if (!e)
                    return NULL;
                asdl_seq_SET(args, nargs++, e);
            }
            else {
                /* a keyword argument */
                keyword_ty kw;
                identifier key, tmp;
                int k;

                // To remain LL(1), the grammar accepts any test (basically, any
                // expression) in the keyword slot of a call site.  So, we need
                // to manually enforce that the keyword is a NAME here.
                static const int name_tree[] = {
                    test,
                    or_test,
                    and_test,
                    not_test,
                    comparison,
                    expr,
                    xor_expr,
                    and_expr,
                    shift_expr,
                    arith_expr,
                    term,
                    factor,
                    power,
                    atom_expr,
                    atom,
                    0,
                };
                node *expr_node = chch;
                for (int i = 0; name_tree[i]; i++) {
                    if (TYPE(expr_node) != name_tree[i])
                        break;
                    if (NCH(expr_node) != 1)
                        break;
                    expr_node = CHILD(expr_node, 0);
                }
                if (TYPE(expr_node) != NAME) {
                    ast_error(c, chch,
                              "expression cannot contain assignment, "
                              "perhaps you meant \"==\"?");
                    return NULL;
                }
                key = new_identifier(STR(expr_node), c);
                if (key == NULL) {
                    return NULL;
                }
                if (forbidden_name(c, key, chch, 1)) {
                    return NULL;
                }
                for (k = 0; k < nkeywords; k++) {
                    tmp = ((keyword_ty)asdl_seq_GET(keywords, k))->arg;
                    if (tmp && !PyUnicode_Compare(tmp, key)) {
                        ast_error(c, chch,
                                  "keyword argument repeated");
                        return NULL;
                    }
                }
                e = ast_for_expr(c, CHILD(ch, 2));
                if (!e)
                    return NULL;
                kw = keyword(key, e, c->c_arena);
                if (!kw)
                    return NULL;
                asdl_seq_SET(keywords, nkeywords++, kw);
            }
        }
    }

    return Call(func, args, keywords, LINENO(start), start->n_col_offset,
                closepar->n_end_lineno, closepar->n_end_col_offset, c->c_arena);
}

static expr_ty
ast_for_testlist(struct compiling *c, const node* n)
{
    /* testlist_comp: test (comp_for | (',' test)* [',']) */
    /* testlist: test (',' test)* [','] */
    assert(NCH(n) > 0);
    if (TYPE(n) == testlist_comp) {
        if (NCH(n) > 1)
            assert(TYPE(CHILD(n, 1)) != comp_for);
    }
    else {
        assert(TYPE(n) == testlist ||
               TYPE(n) == testlist_star_expr);
    }
    if (NCH(n) == 1)
        return ast_for_expr(c, CHILD(n, 0));
    else {
        asdl_seq *tmp = seq_for_testlist(c, n);
        if (!tmp)
            return NULL;
        return Tuple(tmp, Load, LINENO(n), n->n_col_offset,
                     n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
}

static stmt_ty
ast_for_expr_stmt(struct compiling *c, const node *n)
{
    REQ(n, expr_stmt);
    /* expr_stmt: testlist_star_expr (annassign | augassign (yield_expr|testlist) |
                     [('=' (yield_expr|testlist_star_expr))+ [TYPE_COMMENT]] )
       annassign: ':' test ['=' (yield_expr|testlist)]
       testlist_star_expr: (test|star_expr) (',' (test|star_expr))* [',']
       augassign: ('+=' | '-=' | '*=' | '@=' | '/=' | '%=' | '&=' | '|=' | '^=' |
                   '<<=' | '>>=' | '**=' | '//=')
       test: ... here starts the operator precedence dance
     */
    int num = NCH(n);

    if (num == 1) {
        expr_ty e = ast_for_testlist(c, CHILD(n, 0));
        if (!e)
            return NULL;

        return Expr(e, LINENO(n), n->n_col_offset,
                    n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else if (TYPE(CHILD(n, 1)) == augassign) {
        expr_ty expr1, expr2;
        operator_ty newoperator;
        node *ch = CHILD(n, 0);

        expr1 = ast_for_testlist(c, ch);
        if (!expr1)
            return NULL;
        if(!set_context(c, expr1, Store, ch))
            return NULL;
        /* set_context checks that most expressions are not the left side.
          Augmented assignments can only have a name, a subscript, or an
          attribute on the left, though, so we have to explicitly check for
          those. */
        switch (expr1->kind) {
            case Name_kind:
            case Attribute_kind:
            case Subscript_kind:
                break;
            default:
                ast_error(c, ch, "illegal expression for augmented assignment");
                return NULL;
        }

        ch = CHILD(n, 2);
        if (TYPE(ch) == testlist)
            expr2 = ast_for_testlist(c, ch);
        else
            expr2 = ast_for_expr(c, ch);
        if (!expr2)
            return NULL;

        newoperator = ast_for_augassign(c, CHILD(n, 1));
        if (!newoperator)
            return NULL;

        return AugAssign(expr1, newoperator, expr2, LINENO(n), n->n_col_offset,
                         n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else if (TYPE(CHILD(n, 1)) == annassign) {
        expr_ty expr1, expr2, expr3;
        node *ch = CHILD(n, 0);
        node *deep, *ann = CHILD(n, 1);
        int simple = 1;

        /* AnnAssigns are only allowed in Python 3.6 or greater */
        if (c->c_feature_version < 6) {
            ast_error(c, ch,
                      "Variable annotation syntax is only supported in Python 3.6 and greater");
            return NULL;
        }

        /* we keep track of parens to qualify (x) as expression not name */
        deep = ch;
        while (NCH(deep) == 1) {
            deep = CHILD(deep, 0);
        }
        if (NCH(deep) > 0 && TYPE(CHILD(deep, 0)) == LPAR) {
            simple = 0;
        }
        expr1 = ast_for_testlist(c, ch);
        if (!expr1) {
            return NULL;
        }
        switch (expr1->kind) {
            case Name_kind:
                if (forbidden_name(c, expr1->v.Name.id, n, 0)) {
                    return NULL;
                }
                expr1->v.Name.ctx = Store;
                break;
            case Attribute_kind:
                if (forbidden_name(c, expr1->v.Attribute.attr, n, 1)) {
                    return NULL;
                }
                expr1->v.Attribute.ctx = Store;
                break;
            case Subscript_kind:
                expr1->v.Subscript.ctx = Store;
                break;
            case List_kind:
                ast_error(c, ch,
                          "only single target (not list) can be annotated");
                return NULL;
            case Tuple_kind:
                ast_error(c, ch,
                          "only single target (not tuple) can be annotated");
                return NULL;
            default:
                ast_error(c, ch,
                          "illegal target for annotation");
                return NULL;
        }

        if (expr1->kind != Name_kind) {
            simple = 0;
        }
        ch = CHILD(ann, 1);
        expr2 = ast_for_expr(c, ch);
        if (!expr2) {
            return NULL;
        }
        if (NCH(ann) == 2) {
            return AnnAssign(expr1, expr2, NULL, simple,
                             LINENO(n), n->n_col_offset,
                             n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        else {
            ch = CHILD(ann, 3);
            if (TYPE(ch) == testlist_star_expr) {
                expr3 = ast_for_testlist(c, ch);
            }
            else {
                expr3 = ast_for_expr(c, ch);
            }
            if (!expr3) {
                return NULL;
            }
            return AnnAssign(expr1, expr2, expr3, simple,
                             LINENO(n), n->n_col_offset,
                             n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
    }
    else {
        int i, nch_minus_type, has_type_comment;
        asdl_seq *targets;
        node *value;
        expr_ty expression;
        string type_comment;

        /* a normal assignment */
        REQ(CHILD(n, 1), EQUAL);

        has_type_comment = TYPE(CHILD(n, num - 1)) == TYPE_COMMENT;
        nch_minus_type = num - has_type_comment;

        targets = _Py_asdl_seq_new(nch_minus_type / 2, c->c_arena);
        if (!targets)
            return NULL;
        for (i = 0; i < nch_minus_type - 2; i += 2) {
            expr_ty e;
            node *ch = CHILD(n, i);
            if (TYPE(ch) == yield_expr) {
                ast_error(c, ch, "assignment to yield expression not possible");
                return NULL;
            }
            e = ast_for_testlist(c, ch);
            if (!e)
              return NULL;

            /* set context to assign */
            if (!set_context(c, e, Store, CHILD(n, i)))
              return NULL;

            asdl_seq_SET(targets, i / 2, e);
        }
        value = CHILD(n, nch_minus_type - 1);
        if (TYPE(value) == testlist_star_expr)
            expression = ast_for_testlist(c, value);
        else
            expression = ast_for_expr(c, value);
        if (!expression)
            return NULL;
        if (has_type_comment) {
            type_comment = NEW_TYPE_COMMENT(CHILD(n, nch_minus_type));
            if (!type_comment)
                return NULL;
        }
        else
            type_comment = NULL;
        return Assign(targets, expression, type_comment, LINENO(n), n->n_col_offset,
                      n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
}


static asdl_seq *
ast_for_exprlist(struct compiling *c, const node *n, expr_context_ty context)
{
    asdl_seq *seq;
    int i;
    expr_ty e;

    REQ(n, exprlist);

    seq = _Py_asdl_seq_new((NCH(n) + 1) / 2, c->c_arena);
    if (!seq)
        return NULL;
    for (i = 0; i < NCH(n); i += 2) {
        e = ast_for_expr(c, CHILD(n, i));
        if (!e)
            return NULL;
        asdl_seq_SET(seq, i / 2, e);
        if (context && !set_context(c, e, context, CHILD(n, i)))
            return NULL;
    }
    return seq;
}

static stmt_ty
ast_for_del_stmt(struct compiling *c, const node *n)
{
    asdl_seq *expr_list;

    /* del_stmt: 'del' exprlist */
    REQ(n, del_stmt);

    expr_list = ast_for_exprlist(c, CHILD(n, 1), Del);
    if (!expr_list)
        return NULL;
    return Delete(expr_list, LINENO(n), n->n_col_offset,
                  n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

static stmt_ty
ast_for_flow_stmt(struct compiling *c, const node *n)
{
    /*
      flow_stmt: break_stmt | continue_stmt | return_stmt | raise_stmt
                 | yield_stmt
      break_stmt: 'break'
      continue_stmt: 'continue'
      return_stmt: 'return' [testlist]
      yield_stmt: yield_expr
      yield_expr: 'yield' testlist | 'yield' 'from' test
      raise_stmt: 'raise' [test [',' test [',' test]]]
    */
    node *ch;

    REQ(n, flow_stmt);
    ch = CHILD(n, 0);
    switch (TYPE(ch)) {
        case break_stmt:
            return Break(LINENO(n), n->n_col_offset,
                         n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        case continue_stmt:
            return Continue(LINENO(n), n->n_col_offset,
                            n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        case yield_stmt: { /* will reduce to yield_expr */
            expr_ty exp = ast_for_expr(c, CHILD(ch, 0));
            if (!exp)
                return NULL;
            return Expr(exp, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        case return_stmt:
            if (NCH(ch) == 1)
                return Return(NULL, LINENO(n), n->n_col_offset,
                              n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            else {
                expr_ty expression = ast_for_testlist(c, CHILD(ch, 1));
                if (!expression)
                    return NULL;
                return Return(expression, LINENO(n), n->n_col_offset,
                              n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            }
        case raise_stmt:
            if (NCH(ch) == 1)
                return Raise(NULL, NULL, LINENO(n), n->n_col_offset,
                             n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            else if (NCH(ch) >= 2) {
                expr_ty cause = NULL;
                expr_ty expression = ast_for_expr(c, CHILD(ch, 1));
                if (!expression)
                    return NULL;
                if (NCH(ch) == 4) {
                    cause = ast_for_expr(c, CHILD(ch, 3));
                    if (!cause)
                        return NULL;
                }
                return Raise(expression, cause, LINENO(n), n->n_col_offset,
                             n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            }
            /* fall through */
        default:
            PyErr_Format(PyExc_SystemError,
                         "unexpected flow_stmt: %d", TYPE(ch));
            return NULL;
    }
}

static alias_ty
alias_for_import_name(struct compiling *c, const node *n, int store)
{
    /*
      import_as_name: NAME ['as' NAME]
      dotted_as_name: dotted_name ['as' NAME]
      dotted_name: NAME ('.' NAME)*
    */
    identifier str, name;

 loop:
    switch (TYPE(n)) {
        case import_as_name: {
            node *name_node = CHILD(n, 0);
            str = NULL;
            name = NEW_IDENTIFIER(name_node);
            if (!name)
                return NULL;
            if (NCH(n) == 3) {
                node *str_node = CHILD(n, 2);
                str = NEW_IDENTIFIER(str_node);
                if (!str)
                    return NULL;
                if (store && forbidden_name(c, str, str_node, 0))
                    return NULL;
            }
            else {
                if (forbidden_name(c, name, name_node, 0))
                    return NULL;
            }
            return alias(name, str, c->c_arena);
        }
        case dotted_as_name:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            else {
                node *asname_node = CHILD(n, 2);
                alias_ty a = alias_for_import_name(c, CHILD(n, 0), 0);
                if (!a)
                    return NULL;
                assert(!a->asname);
                a->asname = NEW_IDENTIFIER(asname_node);
                if (!a->asname)
                    return NULL;
                if (forbidden_name(c, a->asname, asname_node, 0))
                    return NULL;
                return a;
            }
        case dotted_name:
            if (NCH(n) == 1) {
                node *name_node = CHILD(n, 0);
                name = NEW_IDENTIFIER(name_node);
                if (!name)
                    return NULL;
                if (store && forbidden_name(c, name, name_node, 0))
                    return NULL;
                return alias(name, NULL, c->c_arena);
            }
            else {
                /* Create a string of the form "a.b.c" */
                int i;
                size_t len;
                char *s;
                PyObject *uni;

                len = 0;
                for (i = 0; i < NCH(n); i += 2)
                    /* length of string plus one for the dot */
                    len += strlen(STR(CHILD(n, i))) + 1;
                len--; /* the last name doesn't have a dot */
                str = PyBytes_FromStringAndSize(NULL, len);
                if (!str)
                    return NULL;
                s = PyBytes_AS_STRING(str);
                if (!s)
                    return NULL;
                for (i = 0; i < NCH(n); i += 2) {
                    char *sch = STR(CHILD(n, i));
                    strcpy(s, STR(CHILD(n, i)));
                    s += strlen(sch);
                    *s++ = '.';
                }
                --s;
                *s = '\0';
                uni = PyUnicode_DecodeUTF8(PyBytes_AS_STRING(str),
                                           PyBytes_GET_SIZE(str),
                                           NULL);
                Py_DECREF(str);
                if (!uni)
                    return NULL;
                str = uni;
                PyUnicode_InternInPlace(&str);
                if (PyArena_AddPyObject(c->c_arena, str) < 0) {
                    Py_DECREF(str);
                    return NULL;
                }
                return alias(str, NULL, c->c_arena);
            }
        case STAR:
            str = PyUnicode_InternFromString("*");
            if (!str)
                return NULL;
            if (PyArena_AddPyObject(c->c_arena, str) < 0) {
                Py_DECREF(str);
                return NULL;
            }
            return alias(str, NULL, c->c_arena);
        default:
            PyErr_Format(PyExc_SystemError,
                         "unexpected import name: %d", TYPE(n));
            return NULL;
    }
}

static stmt_ty
ast_for_import_stmt(struct compiling *c, const node *n)
{
    /*
      import_stmt: import_name | import_from
      import_name: 'import' dotted_as_names
      import_from: 'from' (('.' | '...')* dotted_name | ('.' | '...')+)
                   'import' ('*' | '(' import_as_names ')' | import_as_names)
    */
    int lineno;
    int col_offset;
    int i;
    asdl_seq *aliases;

    REQ(n, import_stmt);
    lineno = LINENO(n);
    col_offset = n->n_col_offset;
    n = CHILD(n, 0);
    if (TYPE(n) == import_name) {
        n = CHILD(n, 1);
        REQ(n, dotted_as_names);
        aliases = _Py_asdl_seq_new((NCH(n) + 1) / 2, c->c_arena);
        if (!aliases)
                return NULL;
        for (i = 0; i < NCH(n); i += 2) {
            alias_ty import_alias = alias_for_import_name(c, CHILD(n, i), 1);
            if (!import_alias)
                return NULL;
            asdl_seq_SET(aliases, i / 2, import_alias);
        }
        // Even though n is modified above, the end position is not changed
        return Import(aliases, lineno, col_offset,
                      n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else if (TYPE(n) == import_from) {
        int n_children;
        int idx, ndots = 0;
        const node *n_copy = n;
        alias_ty mod = NULL;
        identifier modname = NULL;

       /* Count the number of dots (for relative imports) and check for the
          optional module name */
        for (idx = 1; idx < NCH(n); idx++) {
            if (TYPE(CHILD(n, idx)) == dotted_name) {
                mod = alias_for_import_name(c, CHILD(n, idx), 0);
                if (!mod)
                    return NULL;
                idx++;
                break;
            } else if (TYPE(CHILD(n, idx)) == ELLIPSIS) {
                /* three consecutive dots are tokenized as one ELLIPSIS */
                ndots += 3;
                continue;
            } else if (TYPE(CHILD(n, idx)) != DOT) {
                break;
            }
            ndots++;
        }
        idx++; /* skip over the 'import' keyword */
        switch (TYPE(CHILD(n, idx))) {
        case STAR:
            /* from ... import * */
            n = CHILD(n, idx);
            n_children = 1;
            break;
        case LPAR:
            /* from ... import (x, y, z) */
            n = CHILD(n, idx + 1);
            n_children = NCH(n);
            break;
        case import_as_names:
            /* from ... import x, y, z */
            n = CHILD(n, idx);
            n_children = NCH(n);
            if (n_children % 2 == 0) {
                ast_error(c, n,
                          "trailing comma not allowed without"
                          " surrounding parentheses");
                return NULL;
            }
            break;
        default:
            ast_error(c, n, "Unexpected node-type in from-import");
            return NULL;
        }

        aliases = _Py_asdl_seq_new((n_children + 1) / 2, c->c_arena);
        if (!aliases)
            return NULL;

        /* handle "from ... import *" special b/c there's no children */
        if (TYPE(n) == STAR) {
            alias_ty import_alias = alias_for_import_name(c, n, 1);
            if (!import_alias)
                return NULL;
            asdl_seq_SET(aliases, 0, import_alias);
        }
        else {
            for (i = 0; i < NCH(n); i += 2) {
                alias_ty import_alias = alias_for_import_name(c, CHILD(n, i), 1);
                if (!import_alias)
                    return NULL;
                asdl_seq_SET(aliases, i / 2, import_alias);
            }
        }
        if (mod != NULL)
            modname = mod->name;
        return ImportFrom(modname, aliases, ndots, lineno, col_offset,
                          n_copy->n_end_lineno, n_copy->n_end_col_offset,
                          c->c_arena);
    }
    PyErr_Format(PyExc_SystemError,
                 "unknown import statement: starts with command '%s'",
                 STR(CHILD(n, 0)));
    return NULL;
}

static stmt_ty
ast_for_global_stmt(struct compiling *c, const node *n)
{
    /* global_stmt: 'global' NAME (',' NAME)* */
    identifier name;
    asdl_seq *s;
    int i;

    REQ(n, global_stmt);
    s = _Py_asdl_seq_new(NCH(n) / 2, c->c_arena);
    if (!s)
        return NULL;
    for (i = 1; i < NCH(n); i += 2) {
        name = NEW_IDENTIFIER(CHILD(n, i));
        if (!name)
            return NULL;
        asdl_seq_SET(s, i / 2, name);
    }
    return Global(s, LINENO(n), n->n_col_offset,
                  n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

static stmt_ty
ast_for_nonlocal_stmt(struct compiling *c, const node *n)
{
    /* nonlocal_stmt: 'nonlocal' NAME (',' NAME)* */
    identifier name;
    asdl_seq *s;
    int i;

    REQ(n, nonlocal_stmt);
    s = _Py_asdl_seq_new(NCH(n) / 2, c->c_arena);
    if (!s)
        return NULL;
    for (i = 1; i < NCH(n); i += 2) {
        name = NEW_IDENTIFIER(CHILD(n, i));
        if (!name)
            return NULL;
        asdl_seq_SET(s, i / 2, name);
    }
    return Nonlocal(s, LINENO(n), n->n_col_offset,
                    n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

static stmt_ty
ast_for_assert_stmt(struct compiling *c, const node *n)
{
    /* assert_stmt: 'assert' test [',' test] */
    REQ(n, assert_stmt);
    if (NCH(n) == 2) {
        expr_ty expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        return Assert(expression, NULL, LINENO(n), n->n_col_offset,
                      n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else if (NCH(n) == 4) {
        expr_ty expr1, expr2;

        expr1 = ast_for_expr(c, CHILD(n, 1));
        if (!expr1)
            return NULL;
        expr2 = ast_for_expr(c, CHILD(n, 3));
        if (!expr2)
            return NULL;

        return Assert(expr1, expr2, LINENO(n), n->n_col_offset,
                      n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    PyErr_Format(PyExc_SystemError,
                 "improper number of parts to 'assert' statement: %d",
                 NCH(n));
    return NULL;
}

static asdl_seq *
ast_for_suite(struct compiling *c, const node *n)
{
    /* suite: simple_stmt | NEWLINE [TYPE_COMMENT NEWLINE] INDENT stmt+ DEDENT */
    asdl_seq *seq;
    stmt_ty s;
    int i, total, num, end, pos = 0;
    node *ch;

    if (TYPE(n) != func_body_suite) {
        REQ(n, suite);
    }

    total = num_stmts(n);
    seq = _Py_asdl_seq_new(total, c->c_arena);
    if (!seq)
        return NULL;
    if (TYPE(CHILD(n, 0)) == simple_stmt) {
        n = CHILD(n, 0);
        /* simple_stmt always ends with a NEWLINE,
           and may have a trailing SEMI
        */
        end = NCH(n) - 1;
        if (TYPE(CHILD(n, end - 1)) == SEMI)
            end--;
        /* loop by 2 to skip semi-colons */
        for (i = 0; i < end; i += 2) {
            ch = CHILD(n, i);
            s = ast_for_stmt(c, ch);
            if (!s)
                return NULL;
            asdl_seq_SET(seq, pos++, s);
        }
    }
    else {
        i = 2;
        if (TYPE(CHILD(n, 1)) == TYPE_COMMENT) {
            i += 2;
            REQ(CHILD(n, 2), NEWLINE);
        }

        for (; i < (NCH(n) - 1); i++) {
            ch = CHILD(n, i);
            REQ(ch, stmt);
            num = num_stmts(ch);
            if (num == 1) {
                /* small_stmt or compound_stmt with only one child */
                s = ast_for_stmt(c, ch);
                if (!s)
                    return NULL;
                asdl_seq_SET(seq, pos++, s);
            }
            else {
                int j;
                ch = CHILD(ch, 0);
                REQ(ch, simple_stmt);
                for (j = 0; j < NCH(ch); j += 2) {
                    /* statement terminates with a semi-colon ';' */
                    if (NCH(CHILD(ch, j)) == 0) {
                        assert((j + 1) == NCH(ch));
                        break;
                    }
                    s = ast_for_stmt(c, CHILD(ch, j));
                    if (!s)
                        return NULL;
                    asdl_seq_SET(seq, pos++, s);
                }
            }
        }
    }
    assert(pos == seq->size);
    return seq;
}

static void
get_last_end_pos(asdl_seq *s, int *end_lineno, int *end_col_offset)
{
    Py_ssize_t tot = asdl_seq_LEN(s);
    // There must be no empty suites.
    assert(tot > 0);
    stmt_ty last = asdl_seq_GET(s, tot - 1);
    *end_lineno = last->end_lineno;
    *end_col_offset = last->end_col_offset;
}

static stmt_ty
ast_for_if_stmt(struct compiling *c, const node *n)
{
    /* if_stmt: 'if' test ':' suite ('elif' test ':' suite)*
       ['else' ':' suite]
    */
    char *s;
    int end_lineno, end_col_offset;

    REQ(n, if_stmt);

    if (NCH(n) == 4) {
        expr_ty expression;
        asdl_seq *suite_seq;

        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, CHILD(n, 3));
        if (!suite_seq)
            return NULL;
        get_last_end_pos(suite_seq, &end_lineno, &end_col_offset);

        return If(expression, suite_seq, NULL, LINENO(n), n->n_col_offset,
                  end_lineno, end_col_offset, c->c_arena);
    }

    s = STR(CHILD(n, 4));
    /* s[2], the third character in the string, will be
       's' for el_s_e, or
       'i' for el_i_f
    */
    if (s[2] == 's') {
        expr_ty expression;
        asdl_seq *seq1, *seq2;

        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        seq1 = ast_for_suite(c, CHILD(n, 3));
        if (!seq1)
            return NULL;
        seq2 = ast_for_suite(c, CHILD(n, 6));
        if (!seq2)
            return NULL;
        get_last_end_pos(seq2, &end_lineno, &end_col_offset);

        return If(expression, seq1, seq2, LINENO(n), n->n_col_offset,
                  end_lineno, end_col_offset, c->c_arena);
    }
    else if (s[2] == 'i') {
        int i, n_elif, has_else = 0;
        expr_ty expression;
        asdl_seq *suite_seq;
        asdl_seq *orelse = NULL;
        n_elif = NCH(n) - 4;
        /* must reference the child n_elif+1 since 'else' token is third,
           not fourth, child from the end. */
        if (TYPE(CHILD(n, (n_elif + 1))) == NAME
            && STR(CHILD(n, (n_elif + 1)))[2] == 's') {
            has_else = 1;
            n_elif -= 3;
        }
        n_elif /= 4;

        if (has_else) {
            asdl_seq *suite_seq2;

            orelse = _Py_asdl_seq_new(1, c->c_arena);
            if (!orelse)
                return NULL;
            expression = ast_for_expr(c, CHILD(n, NCH(n) - 6));
            if (!expression)
                return NULL;
            suite_seq = ast_for_suite(c, CHILD(n, NCH(n) - 4));
            if (!suite_seq)
                return NULL;
            suite_seq2 = ast_for_suite(c, CHILD(n, NCH(n) - 1));
            if (!suite_seq2)
                return NULL;
            get_last_end_pos(suite_seq2, &end_lineno, &end_col_offset);

            asdl_seq_SET(orelse, 0,
                         If(expression, suite_seq, suite_seq2,
                            LINENO(CHILD(n, NCH(n) - 7)),
                            CHILD(n, NCH(n) - 7)->n_col_offset,
                            end_lineno, end_col_offset, c->c_arena));
            /* the just-created orelse handled the last elif */
            n_elif--;
        }

        for (i = 0; i < n_elif; i++) {
            int off = 5 + (n_elif - i - 1) * 4;
            asdl_seq *newobj = _Py_asdl_seq_new(1, c->c_arena);
            if (!newobj)
                return NULL;
            expression = ast_for_expr(c, CHILD(n, off));
            if (!expression)
                return NULL;
            suite_seq = ast_for_suite(c, CHILD(n, off + 2));
            if (!suite_seq)
                return NULL;

            if (orelse != NULL) {
                get_last_end_pos(orelse, &end_lineno, &end_col_offset);
            } else {
                get_last_end_pos(suite_seq, &end_lineno, &end_col_offset);
            }
            asdl_seq_SET(newobj, 0,
                         If(expression, suite_seq, orelse,
                            LINENO(CHILD(n, off - 1)),
                            CHILD(n, off - 1)->n_col_offset,
                            end_lineno, end_col_offset, c->c_arena));
            orelse = newobj;
        }
        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, CHILD(n, 3));
        if (!suite_seq)
            return NULL;
        get_last_end_pos(orelse, &end_lineno, &end_col_offset);
        return If(expression, suite_seq, orelse,
                  LINENO(n), n->n_col_offset,
                  end_lineno, end_col_offset, c->c_arena);
    }

    PyErr_Format(PyExc_SystemError,
                 "unexpected token in 'if' statement: %s", s);
    return NULL;
}

static stmt_ty
ast_for_while_stmt(struct compiling *c, const node *n)
{
    /* while_stmt: 'while' test ':' suite ['else' ':' suite] */
    REQ(n, while_stmt);
    int end_lineno, end_col_offset;

    if (NCH(n) == 4) {
        expr_ty expression;
        asdl_seq *suite_seq;

        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, CHILD(n, 3));
        if (!suite_seq)
            return NULL;
        get_last_end_pos(suite_seq, &end_lineno, &end_col_offset);
        return While(expression, suite_seq, NULL, LINENO(n), n->n_col_offset,
                     end_lineno, end_col_offset, c->c_arena);
    }
    else if (NCH(n) == 7) {
        expr_ty expression;
        asdl_seq *seq1, *seq2;

        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        seq1 = ast_for_suite(c, CHILD(n, 3));
        if (!seq1)
            return NULL;
        seq2 = ast_for_suite(c, CHILD(n, 6));
        if (!seq2)
            return NULL;
        get_last_end_pos(seq2, &end_lineno, &end_col_offset);

        return While(expression, seq1, seq2, LINENO(n), n->n_col_offset,
                     end_lineno, end_col_offset, c->c_arena);
    }

    PyErr_Format(PyExc_SystemError,
                 "wrong number of tokens for 'while' statement: %d",
                 NCH(n));
    return NULL;
}

static stmt_ty
ast_for_for_stmt(struct compiling *c, const node *n0, bool is_async)
{
    const node * const n = is_async ? CHILD(n0, 1) : n0;
    asdl_seq *_target, *seq = NULL, *suite_seq;
    expr_ty expression;
    expr_ty target, first;
    const node *node_target;
    int end_lineno, end_col_offset;
    int has_type_comment;
    string type_comment;

    if (is_async && c->c_feature_version < 5) {
        ast_error(c, n,
                  "Async for loops are only supported in Python 3.5 and greater");
        return NULL;
    }

    /* for_stmt: 'for' exprlist 'in' testlist ':' [TYPE_COMMENT] suite ['else' ':' suite] */
    REQ(n, for_stmt);

    has_type_comment = TYPE(CHILD(n, 5)) == TYPE_COMMENT;

    if (NCH(n) == 9 + has_type_comment) {
        seq = ast_for_suite(c, CHILD(n, 8 + has_type_comment));
        if (!seq)
            return NULL;
    }

    node_target = CHILD(n, 1);
    _target = ast_for_exprlist(c, node_target, Store);
    if (!_target)
        return NULL;
    /* Check the # of children rather than the length of _target, since
       for x, in ... has 1 element in _target, but still requires a Tuple. */
    first = (expr_ty)asdl_seq_GET(_target, 0);
    if (NCH(node_target) == 1)
        target = first;
    else
        target = Tuple(_target, Store, first->lineno, first->col_offset,
                       node_target->n_end_lineno, node_target->n_end_col_offset,
                       c->c_arena);

    expression = ast_for_testlist(c, CHILD(n, 3));
    if (!expression)
        return NULL;
    suite_seq = ast_for_suite(c, CHILD(n, 5 + has_type_comment));
    if (!suite_seq)
        return NULL;

    if (seq != NULL) {
        get_last_end_pos(seq, &end_lineno, &end_col_offset);
    } else {
        get_last_end_pos(suite_seq, &end_lineno, &end_col_offset);
    }

    if (has_type_comment) {
        type_comment = NEW_TYPE_COMMENT(CHILD(n, 5));
        if (!type_comment)
            return NULL;
    }
    else
        type_comment = NULL;

    if (is_async)
        return AsyncFor(target, expression, suite_seq, seq, type_comment,
                        LINENO(n0), n0->n_col_offset,
                        end_lineno, end_col_offset, c->c_arena);
    else
        return For(target, expression, suite_seq, seq, type_comment,
                   LINENO(n), n->n_col_offset,
                   end_lineno, end_col_offset, c->c_arena);
}

static excepthandler_ty
ast_for_except_clause(struct compiling *c, const node *exc, node *body)
{
    /* except_clause: 'except' [test ['as' test]] */
    int end_lineno, end_col_offset;
    REQ(exc, except_clause);
    REQ(body, suite);

    if (NCH(exc) == 1) {
        asdl_seq *suite_seq = ast_for_suite(c, body);
        if (!suite_seq)
            return NULL;
        get_last_end_pos(suite_seq, &end_lineno, &end_col_offset);

        return ExceptHandler(NULL, NULL, suite_seq, LINENO(exc),
                             exc->n_col_offset,
                             end_lineno, end_col_offset, c->c_arena);
    }
    else if (NCH(exc) == 2) {
        expr_ty expression;
        asdl_seq *suite_seq;

        expression = ast_for_expr(c, CHILD(exc, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, body);
        if (!suite_seq)
            return NULL;
        get_last_end_pos(suite_seq, &end_lineno, &end_col_offset);

        return ExceptHandler(expression, NULL, suite_seq, LINENO(exc),
                             exc->n_col_offset,
                             end_lineno, end_col_offset, c->c_arena);
    }
    else if (NCH(exc) == 4) {
        asdl_seq *suite_seq;
        expr_ty expression;
        identifier e = NEW_IDENTIFIER(CHILD(exc, 3));
        if (!e)
            return NULL;
        if (forbidden_name(c, e, CHILD(exc, 3), 0))
            return NULL;
        expression = ast_for_expr(c, CHILD(exc, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, body);
        if (!suite_seq)
            return NULL;
        get_last_end_pos(suite_seq, &end_lineno, &end_col_offset);

        return ExceptHandler(expression, e, suite_seq, LINENO(exc),
                             exc->n_col_offset,
                             end_lineno, end_col_offset, c->c_arena);
    }

    PyErr_Format(PyExc_SystemError,
                 "wrong number of children for 'except' clause: %d",
                 NCH(exc));
    return NULL;
}

static stmt_ty
ast_for_try_stmt(struct compiling *c, const node *n)
{
    const int nch = NCH(n);
    int end_lineno, end_col_offset, n_except = (nch - 3)/3;
    asdl_seq *body, *handlers = NULL, *orelse = NULL, *finally = NULL;
    excepthandler_ty last_handler;

    REQ(n, try_stmt);

    body = ast_for_suite(c, CHILD(n, 2));
    if (body == NULL)
        return NULL;

    if (TYPE(CHILD(n, nch - 3)) == NAME) {
        if (strcmp(STR(CHILD(n, nch - 3)), "finally") == 0) {
            if (nch >= 9 && TYPE(CHILD(n, nch - 6)) == NAME) {
                /* we can assume it's an "else",
                   because nch >= 9 for try-else-finally and
                   it would otherwise have a type of except_clause */
                orelse = ast_for_suite(c, CHILD(n, nch - 4));
                if (orelse == NULL)
                    return NULL;
                n_except--;
            }

            finally = ast_for_suite(c, CHILD(n, nch - 1));
            if (finally == NULL)
                return NULL;
            n_except--;
        }
        else {
            /* we can assume it's an "else",
               otherwise it would have a type of except_clause */
            orelse = ast_for_suite(c, CHILD(n, nch - 1));
            if (orelse == NULL)
                return NULL;
            n_except--;
        }
    }
    else if (TYPE(CHILD(n, nch - 3)) != except_clause) {
        ast_error(c, n, "malformed 'try' statement");
        return NULL;
    }

    if (n_except > 0) {
        int i;
        /* process except statements to create a try ... except */
        handlers = _Py_asdl_seq_new(n_except, c->c_arena);
        if (handlers == NULL)
            return NULL;

        for (i = 0; i < n_except; i++) {
            excepthandler_ty e = ast_for_except_clause(c, CHILD(n, 3 + i * 3),
                                                       CHILD(n, 5 + i * 3));
            if (!e)
                return NULL;
            asdl_seq_SET(handlers, i, e);
        }
    }

    assert(finally != NULL || asdl_seq_LEN(handlers));
        if (finally != NULL) {
        // finally is always last
        get_last_end_pos(finally, &end_lineno, &end_col_offset);
    } else if (orelse != NULL) {
        // otherwise else is last
        get_last_end_pos(orelse, &end_lineno, &end_col_offset);
    } else {
        // inline the get_last_end_pos logic due to layout mismatch
        last_handler = (excepthandler_ty) asdl_seq_GET(handlers, n_except - 1);
        end_lineno = last_handler->end_lineno;
        end_col_offset = last_handler->end_col_offset;
    }
    return Try(body, handlers, orelse, finally, LINENO(n), n->n_col_offset,
               end_lineno, end_col_offset, c->c_arena);
}

/* with_item: test ['as' expr] */
static withitem_ty
ast_for_with_item(struct compiling *c, const node *n)
{
    expr_ty context_expr, optional_vars = NULL;

    REQ(n, with_item);
    context_expr = ast_for_expr(c, CHILD(n, 0));
    if (!context_expr)
        return NULL;
    if (NCH(n) == 3) {
        optional_vars = ast_for_expr(c, CHILD(n, 2));

        if (!optional_vars) {
            return NULL;
        }
        if (!set_context(c, optional_vars, Store, n)) {
            return NULL;
        }
    }

    return withitem(context_expr, optional_vars, c->c_arena);
}

/* with_stmt: 'with' with_item (',' with_item)*  ':' [TYPE_COMMENT] suite */
static stmt_ty
ast_for_with_stmt(struct compiling *c, const node *n0, bool is_async)
{
    const node * const n = is_async ? CHILD(n0, 1) : n0;
    int i, n_items, nch_minus_type, has_type_comment, end_lineno, end_col_offset;
    asdl_seq *items, *body;
    string type_comment;

    if (is_async && c->c_feature_version < 5) {
        ast_error(c, n,
                  "Async with statements are only supported in Python 3.5 and greater");
        return NULL;
    }

    REQ(n, with_stmt);

    has_type_comment = TYPE(CHILD(n, NCH(n) - 2)) == TYPE_COMMENT;
    nch_minus_type = NCH(n) - has_type_comment;

    n_items = (nch_minus_type - 2) / 2;
    items = _Py_asdl_seq_new(n_items, c->c_arena);
    if (!items)
        return NULL;
    for (i = 1; i < nch_minus_type - 2; i += 2) {
        withitem_ty item = ast_for_with_item(c, CHILD(n, i));
        if (!item)
            return NULL;
        asdl_seq_SET(items, (i - 1) / 2, item);
    }

    body = ast_for_suite(c, CHILD(n, NCH(n) - 1));
    if (!body)
        return NULL;
    get_last_end_pos(body, &end_lineno, &end_col_offset);

    if (has_type_comment) {
        type_comment = NEW_TYPE_COMMENT(CHILD(n, NCH(n) - 2));
        if (!type_comment)
            return NULL;
    }
    else
        type_comment = NULL;

    if (is_async)
        return AsyncWith(items, body, type_comment, LINENO(n0), n0->n_col_offset,
                         end_lineno, end_col_offset, c->c_arena);
    else
        return With(items, body, type_comment, LINENO(n), n->n_col_offset,
                    end_lineno, end_col_offset, c->c_arena);
}

static stmt_ty
ast_for_classdef(struct compiling *c, const node *n, asdl_seq *decorator_seq)
{
    /* classdef: 'class' NAME ['(' arglist ')'] ':' suite */
    PyObject *classname;
    asdl_seq *s;
    expr_ty call;
    int end_lineno, end_col_offset;

    REQ(n, classdef);

    if (NCH(n) == 4) { /* class NAME ':' suite */
        s = ast_for_suite(c, CHILD(n, 3));
        if (!s)
            return NULL;
        get_last_end_pos(s, &end_lineno, &end_col_offset);

        classname = NEW_IDENTIFIER(CHILD(n, 1));
        if (!classname)
            return NULL;
        if (forbidden_name(c, classname, CHILD(n, 3), 0))
            return NULL;
        return ClassDef(classname, NULL, NULL, s, decorator_seq,
                        LINENO(n), n->n_col_offset,
                        end_lineno, end_col_offset, c->c_arena);
    }

    if (TYPE(CHILD(n, 3)) == RPAR) { /* class NAME '(' ')' ':' suite */
        s = ast_for_suite(c, CHILD(n, 5));
        if (!s)
            return NULL;
        get_last_end_pos(s, &end_lineno, &end_col_offset);

        classname = NEW_IDENTIFIER(CHILD(n, 1));
        if (!classname)
            return NULL;
        if (forbidden_name(c, classname, CHILD(n, 3), 0))
            return NULL;
        return ClassDef(classname, NULL, NULL, s, decorator_seq,
                        LINENO(n), n->n_col_offset,
                        end_lineno, end_col_offset, c->c_arena);
    }

    /* class NAME '(' arglist ')' ':' suite */
    /* build up a fake Call node so we can extract its pieces */
    {
        PyObject *dummy_name;
        expr_ty dummy;
        dummy_name = NEW_IDENTIFIER(CHILD(n, 1));
        if (!dummy_name)
            return NULL;
        dummy = Name(dummy_name, Load, LINENO(n), n->n_col_offset,
                     CHILD(n, 1)->n_end_lineno, CHILD(n, 1)->n_end_col_offset,
                     c->c_arena);
        call = ast_for_call(c, CHILD(n, 3), dummy,
                            CHILD(n, 1), NULL, CHILD(n, 4));
        if (!call)
            return NULL;
    }
    s = ast_for_suite(c, CHILD(n, 6));
    if (!s)
        return NULL;
    get_last_end_pos(s, &end_lineno, &end_col_offset);

    classname = NEW_IDENTIFIER(CHILD(n, 1));
    if (!classname)
        return NULL;
    if (forbidden_name(c, classname, CHILD(n, 1), 0))
        return NULL;

    return ClassDef(classname, call->v.Call.args, call->v.Call.keywords, s,
                    decorator_seq, LINENO(n), n->n_col_offset,
                    end_lineno, end_col_offset, c->c_arena);
}

static stmt_ty
ast_for_stmt(struct compiling *c, const node *n)
{
    if (TYPE(n) == stmt) {
        assert(NCH(n) == 1);
        n = CHILD(n, 0);
    }
    if (TYPE(n) == simple_stmt) {
        assert(num_stmts(n) == 1);
        n = CHILD(n, 0);
    }
    if (TYPE(n) == small_stmt) {
        n = CHILD(n, 0);
        /* small_stmt: expr_stmt | del_stmt | pass_stmt | flow_stmt
                  | import_stmt | global_stmt | nonlocal_stmt | assert_stmt
        */
        switch (TYPE(n)) {
            case expr_stmt:
                return ast_for_expr_stmt(c, n);
            case del_stmt:
                return ast_for_del_stmt(c, n);
            case pass_stmt:
                return Pass(LINENO(n), n->n_col_offset,
                            n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            case flow_stmt:
                return ast_for_flow_stmt(c, n);
            case import_stmt:
                return ast_for_import_stmt(c, n);
            case global_stmt:
                return ast_for_global_stmt(c, n);
            case nonlocal_stmt:
                return ast_for_nonlocal_stmt(c, n);
            case assert_stmt:
                return ast_for_assert_stmt(c, n);
            default:
                PyErr_Format(PyExc_SystemError,
                             "unhandled small_stmt: TYPE=%d NCH=%d\n",
                             TYPE(n), NCH(n));
                return NULL;
        }
    }
    else {
        /* compound_stmt: if_stmt | while_stmt | for_stmt | try_stmt
                        | funcdef | classdef | decorated | async_stmt
        */
        node *ch = CHILD(n, 0);
        REQ(n, compound_stmt);
        switch (TYPE(ch)) {
            case if_stmt:
                return ast_for_if_stmt(c, ch);
            case while_stmt:
                return ast_for_while_stmt(c, ch);
            case for_stmt:
                return ast_for_for_stmt(c, ch, 0);
            case try_stmt:
                return ast_for_try_stmt(c, ch);
            case with_stmt:
                return ast_for_with_stmt(c, ch, 0);
            case funcdef:
                return ast_for_funcdef(c, ch, NULL);
            case classdef:
                return ast_for_classdef(c, ch, NULL);
            case decorated:
                return ast_for_decorated(c, ch);
            case async_stmt:
                return ast_for_async_stmt(c, ch);
            default:
                PyErr_Format(PyExc_SystemError,
                             "unhandled compound_stmt: TYPE=%d NCH=%d\n",
                             TYPE(n), NCH(n));
                return NULL;
        }
    }
}

static PyObject *
parsenumber_raw(struct compiling *c, const char *s)
{
    const char *end;
    long x;
    double dx;
    Py_complex compl;
    int imflag;

    assert(s != NULL);
    errno = 0;
    end = s + strlen(s) - 1;
    imflag = *end == 'j' || *end == 'J';
    if (s[0] == '0') {
        x = (long) PyOS_strtoul(s, (char **)&end, 0);
        if (x < 0 && errno == 0) {
            return PyLong_FromString(s, (char **)0, 0);
        }
    }
    else
        x = PyOS_strtol(s, (char **)&end, 0);
    if (*end == '\0') {
        if (errno != 0)
            return PyLong_FromString(s, (char **)0, 0);
        return PyLong_FromLong(x);
    }
    /* XXX Huge floats may silently fail */
    if (imflag) {
        compl.real = 0.;
        compl.imag = PyOS_string_to_double(s, (char **)&end, NULL);
        if (compl.imag == -1.0 && PyErr_Occurred())
            return NULL;
        return PyComplex_FromCComplex(compl);
    }
    else
    {
        dx = PyOS_string_to_double(s, NULL, NULL);
        if (dx == -1.0 && PyErr_Occurred())
            return NULL;
        return PyFloat_FromDouble(dx);
    }
}

static PyObject *
parsenumber(struct compiling *c, const char *s)
{
    char *dup, *end;
    PyObject *res = NULL;

    assert(s != NULL);

    if (strchr(s, '_') == NULL) {
        return parsenumber_raw(c, s);
    }
    /* Create a duplicate without underscores. */
    dup = PyMem_Malloc(strlen(s) + 1);
    if (dup == NULL) {
        return PyErr_NoMemory();
    }
    end = dup;
    for (; *s; s++) {
        if (*s != '_') {
            *end++ = *s;
        }
    }
    *end = '\0';
    res = parsenumber_raw(c, dup);
    PyMem_Free(dup);
    return res;
}

static PyObject *
decode_utf8(struct compiling *c, const char **sPtr, const char *end)
{
    const char *s, *t;
    t = s = *sPtr;
    /* while (s < end && *s != '\\') s++; */ /* inefficient for u".." */
    while (s < end && (*s & 0x80)) s++;
    *sPtr = s;
    return PyUnicode_DecodeUTF8(t, s - t, NULL);
}

static int
warn_invalid_escape_sequence(struct compiling *c, const node *n,
                             unsigned char first_invalid_escape_char)
{
    PyObject *msg = PyUnicode_FromFormat("invalid escape sequence \\%c",
                                         first_invalid_escape_char);
    if (msg == NULL) {
        return -1;
    }
    if (PyErr_WarnExplicitObject(PyExc_DeprecationWarning, msg,
                                   c->c_filename, LINENO(n),
                                   NULL, NULL) < 0)
    {
        if (PyErr_ExceptionMatches(PyExc_DeprecationWarning)) {
            /* Replace the DeprecationWarning exception with a SyntaxError
               to get a more accurate error report */
            PyErr_Clear();
            ast_error(c, n, "%U", msg);
        }
        Py_DECREF(msg);
        return -1;
    }
    Py_DECREF(msg);
    return 0;
}

static PyObject *
decode_unicode_with_escapes(struct compiling *c, const node *n, const char *s,
                            size_t len)
{
    PyObject *v, *u;
    char *buf;
    char *p;
    const char *end;

    /* check for integer overflow */
    if (len > SIZE_MAX / 6)
        return NULL;
    /* "ä" (2 bytes) may become "\U000000E4" (10 bytes), or 1:5
       "\ä" (3 bytes) may become "\u005c\U000000E4" (16 bytes), or ~1:6 */
    u = PyBytes_FromStringAndSize((char *)NULL, len * 6);
    if (u == NULL)
        return NULL;
    p = buf = PyBytes_AsString(u);
    end = s + len;
    while (s < end) {
        if (*s == '\\') {
            *p++ = *s++;
            if (s >= end || *s & 0x80) {
                strcpy(p, "u005c");
                p += 5;
                if (s >= end)
                    break;
            }
        }
        if (*s & 0x80) { /* XXX inefficient */
            PyObject *w;
            int kind;
            void *data;
            Py_ssize_t len, i;
            w = decode_utf8(c, &s, end);
            if (w == NULL) {
                Py_DECREF(u);
                return NULL;
            }
            kind = PyUnicode_KIND(w);
            data = PyUnicode_DATA(w);
            len = PyUnicode_GET_LENGTH(w);
            for (i = 0; i < len; i++) {
                Py_UCS4 chr = PyUnicode_READ(kind, data, i);
                sprintf(p, "\\U%08x", chr);
                p += 10;
            }
            /* Should be impossible to overflow */
            assert(p - buf <= PyBytes_GET_SIZE(u));
            Py_DECREF(w);
        } else {
            *p++ = *s++;
        }
    }
    len = p - buf;
    s = buf;

    const char *first_invalid_escape;
    v = _PyUnicode_DecodeUnicodeEscape(s, len, NULL, &first_invalid_escape);

    if (v != NULL && first_invalid_escape != NULL) {
        if (warn_invalid_escape_sequence(c, n, *first_invalid_escape) < 0) {
            /* We have not decref u before because first_invalid_escape points
               inside u. */
            Py_XDECREF(u);
            Py_DECREF(v);
            return NULL;
        }
    }
    Py_XDECREF(u);
    return v;
}

static PyObject *
decode_bytes_with_escapes(struct compiling *c, const node *n, const char *s,
                          size_t len)
{
    const char *first_invalid_escape;
    PyObject *result = _PyBytes_DecodeEscape(s, len, NULL,
                                             &first_invalid_escape);
    if (result == NULL)
        return NULL;

    if (first_invalid_escape != NULL) {
        if (warn_invalid_escape_sequence(c, n, *first_invalid_escape) < 0) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

/* Shift locations for the given node and all its children by adding `lineno`
   and `col_offset` to existing locations. */
static void fstring_shift_node_locations(node *n, int lineno, int col_offset)
{
    n->n_col_offset = n->n_col_offset + col_offset;
    n->n_end_col_offset = n->n_end_col_offset + col_offset;
    for (int i = 0; i < NCH(n); ++i) {
        if (n->n_lineno && n->n_lineno < CHILD(n, i)->n_lineno) {
            /* Shifting column offsets unnecessary if there's been newlines. */
            col_offset = 0;
        }
        fstring_shift_node_locations(CHILD(n, i), lineno, col_offset);
    }
    n->n_lineno = n->n_lineno + lineno;
    n->n_end_lineno = n->n_end_lineno + lineno;
}

/* Fix locations for the given node and its children.

   `parent` is the enclosing node.
   `n` is the node which locations are going to be fixed relative to parent.
   `expr_str` is the child node's string representation, including braces.
*/
static void
fstring_fix_node_location(const node *parent, node *n, char *expr_str)
{
    char *substr = NULL;
    char *start;
    int lines = LINENO(parent) - 1;
    int cols = parent->n_col_offset;
    /* Find the full fstring to fix location information in `n`. */
    while (parent && parent->n_type != STRING)
        parent = parent->n_child;
    if (parent && parent->n_str) {
        substr = strstr(parent->n_str, expr_str);
        if (substr) {
            start = substr;
            while (start > parent->n_str) {
                if (start[0] == '\n')
                    break;
                start--;
            }
            cols += (int)(substr - start);
            /* adjust the start based on the number of newlines encountered
               before the f-string expression */
            for (char* p = parent->n_str; p < substr; p++) {
                if (*p == '\n') {
                    lines++;
                }
            }
        }
    }
    fstring_shift_node_locations(n, lines, cols);
}

/* Compile this expression in to an expr_ty.  Add parens around the
   expression, in order to allow leading spaces in the expression. */
static expr_ty
fstring_compile_expr(const char *expr_start, const char *expr_end,
                     struct compiling *c, const node *n)

{
    node *mod_n;
    mod_ty mod;
    char *str;
    Py_ssize_t len;
    const char *s;

    assert(expr_end >= expr_start);
    assert(*(expr_start-1) == '{');
    assert(*expr_end == '}' || *expr_end == '!' || *expr_end == ':' ||
           *expr_end == '=');

    /* If the substring is all whitespace, it's an error.  We need to catch this
       here, and not when we call PyParser_SimpleParseStringFlagsFilename,
       because turning the expression '' in to '()' would go from being invalid
       to valid. */
    for (s = expr_start; s != expr_end; s++) {
        char c = *s;
        /* The Python parser ignores only the following whitespace
           characters (\r already is converted to \n). */
        if (!(c == ' ' || c == '\t' || c == '\n' || c == '\f')) {
            break;
        }
    }
    if (s == expr_end) {
        ast_error(c, n, "f-string: empty expression not allowed");
        return NULL;
    }

    len = expr_end - expr_start;
    /* Allocate 3 extra bytes: open paren, close paren, null byte. */
    str = PyMem_RawMalloc(len + 3);
    if (str == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    str[0] = '(';
    memcpy(str+1, expr_start, len);
    str[len+1] = ')';
    str[len+2] = 0;

    PyCompilerFlags cf = _PyCompilerFlags_INIT;
    cf.cf_flags = PyCF_ONLY_AST;
    mod_n = PyParser_SimpleParseStringFlagsFilename(str, "<fstring>",
                                                    Py_eval_input, 0);
    if (!mod_n) {
        PyMem_RawFree(str);
        return NULL;
    }
    /* Reuse str to find the correct column offset. */
    str[0] = '{';
    str[len+1] = '}';
    fstring_fix_node_location(n, mod_n, str);
    mod = PyAST_FromNode(mod_n, &cf, "<fstring>", c->c_arena);
    PyMem_RawFree(str);
    PyNode_Free(mod_n);
    if (!mod)
        return NULL;
    return mod->v.Expression.body;
}

/* Return -1 on error.

   Return 0 if we reached the end of the literal.

   Return 1 if we haven't reached the end of the literal, but we want
   the caller to process the literal up to this point. Used for
   doubled braces.
*/
static int
fstring_find_literal(const char **str, const char *end, int raw,
                     PyObject **literal, int recurse_lvl,
                     struct compiling *c, const node *n)
{
    /* Get any literal string. It ends when we hit an un-doubled left
       brace (which isn't part of a unicode name escape such as
       "\N{EULER CONSTANT}"), or the end of the string. */

    const char *s = *str;
    const char *literal_start = s;
    int result = 0;

    assert(*literal == NULL);
    while (s < end) {
        char ch = *s++;
        if (!raw && ch == '\\' && s < end) {
            ch = *s++;
            if (ch == 'N') {
                if (s < end && *s++ == '{') {
                    while (s < end && *s++ != '}') {
                    }
                    continue;
                }
                break;
            }
            if (ch == '{' && warn_invalid_escape_sequence(c, n, ch) < 0) {
                return -1;
            }
        }
        if (ch == '{' || ch == '}') {
            /* Check for doubled braces, but only at the top level. If
               we checked at every level, then f'{0:{3}}' would fail
               with the two closing braces. */
            if (recurse_lvl == 0) {
                if (s < end && *s == ch) {
                    /* We're going to tell the caller that the literal ends
                       here, but that they should continue scanning. But also
                       skip over the second brace when we resume scanning. */
                    *str = s + 1;
                    result = 1;
                    goto done;
                }

                /* Where a single '{' is the start of a new expression, a
                   single '}' is not allowed. */
                if (ch == '}') {
                    *str = s - 1;
                    ast_error(c, n, "f-string: single '}' is not allowed");
                    return -1;
                }
            }
            /* We're either at a '{', which means we're starting another
               expression; or a '}', which means we're at the end of this
               f-string (for a nested format_spec). */
            s--;
            break;
        }
    }
    *str = s;
    assert(s <= end);
    assert(s == end || *s == '{' || *s == '}');
done:
    if (literal_start != s) {
        if (raw)
            *literal = PyUnicode_DecodeUTF8Stateful(literal_start,
                                                    s - literal_start,
                                                    NULL, NULL);
        else
            *literal = decode_unicode_with_escapes(c, n, literal_start,
                                                   s - literal_start);
        if (!*literal)
            return -1;
    }
    return result;
}

/* Forward declaration because parsing is recursive. */
static expr_ty
fstring_parse(const char **str, const char *end, int raw, int recurse_lvl,
              struct compiling *c, const node *n);

/* Parse the f-string at *str, ending at end.  We know *str starts an
   expression (so it must be a '{'). Returns the FormattedValue node, which
   includes the expression, conversion character, format_spec expression, and
   optionally the text of the expression (if = is used).

   Note that I don't do a perfect job here: I don't make sure that a
   closing brace doesn't match an opening paren, for example. It
   doesn't need to error on all invalid expressions, just correctly
   find the end of all valid ones. Any errors inside the expression
   will be caught when we parse it later.

   *expression is set to the expression.  For an '=' "debug" expression,
   *expr_text is set to the debug text (the original text of the expression,
   including the '=' and any whitespace around it, as a string object).  If
   not a debug expression, *expr_text set to NULL. */
static int
fstring_find_expr(const char **str, const char *end, int raw, int recurse_lvl,
                  PyObject **expr_text, expr_ty *expression,
                  struct compiling *c, const node *n)
{
    /* Return -1 on error, else 0. */

    const char *expr_start;
    const char *expr_end;
    expr_ty simple_expression;
    expr_ty format_spec = NULL; /* Optional format specifier. */
    int conversion = -1; /* The conversion char.  Use default if not
                            specified, or !r if using = and no format
                            spec. */

    /* 0 if we're not in a string, else the quote char we're trying to
       match (single or double quote). */
    char quote_char = 0;

    /* If we're inside a string, 1=normal, 3=triple-quoted. */
    int string_type = 0;

    /* Keep track of nesting level for braces/parens/brackets in
       expressions. */
    Py_ssize_t nested_depth = 0;
    char parenstack[MAXLEVEL];

    *expr_text = NULL;

    /* Can only nest one level deep. */
    if (recurse_lvl >= 2) {
        ast_error(c, n, "f-string: expressions nested too deeply");
        goto error;
    }

    /* The first char must be a left brace, or we wouldn't have gotten
       here. Skip over it. */
    assert(**str == '{');
    *str += 1;

    expr_start = *str;
    for (; *str < end; (*str)++) {
        char ch;

        /* Loop invariants. */
        assert(nested_depth >= 0);
        assert(*str >= expr_start && *str < end);
        if (quote_char)
            assert(string_type == 1 || string_type == 3);
        else
            assert(string_type == 0);

        ch = **str;
        /* Nowhere inside an expression is a backslash allowed. */
        if (ch == '\\') {
            /* Error: can't include a backslash character, inside
               parens or strings or not. */
            ast_error(c, n,
                      "f-string expression part "
                      "cannot include a backslash");
            goto error;
        }
        if (quote_char) {
            /* We're inside a string. See if we're at the end. */
            /* This code needs to implement the same non-error logic
               as tok_get from tokenizer.c, at the letter_quote
               label. To actually share that code would be a
               nightmare. But, it's unlikely to change and is small,
               so duplicate it here. Note we don't need to catch all
               of the errors, since they'll be caught when parsing the
               expression. We just need to match the non-error
               cases. Thus we can ignore \n in single-quoted strings,
               for example. Or non-terminated strings. */
            if (ch == quote_char) {
                /* Does this match the string_type (single or triple
                   quoted)? */
                if (string_type == 3) {
                    if (*str+2 < end && *(*str+1) == ch && *(*str+2) == ch) {
                        /* We're at the end of a triple quoted string. */
                        *str += 2;
                        string_type = 0;
                        quote_char = 0;
                        continue;
                    }
                } else {
                    /* We're at the end of a normal string. */
                    quote_char = 0;
                    string_type = 0;
                    continue;
                }
            }
        } else if (ch == '\'' || ch == '"') {
            /* Is this a triple quoted string? */
            if (*str+2 < end && *(*str+1) == ch && *(*str+2) == ch) {
                string_type = 3;
                *str += 2;
            } else {
                /* Start of a normal string. */
                string_type = 1;
            }
            /* Start looking for the end of the string. */
            quote_char = ch;
        } else if (ch == '[' || ch == '{' || ch == '(') {
            if (nested_depth >= MAXLEVEL) {
                ast_error(c, n, "f-string: too many nested parenthesis");
                goto error;
            }
            parenstack[nested_depth] = ch;
            nested_depth++;
        } else if (ch == '#') {
            /* Error: can't include a comment character, inside parens
               or not. */
            ast_error(c, n, "f-string expression part cannot include '#'");
            goto error;
        } else if (nested_depth == 0 &&
                   (ch == '!' || ch == ':' || ch == '}' ||
                    ch == '=' || ch == '>' || ch == '<')) {
            /* See if there's a next character. */
            if (*str+1 < end) {
                char next = *(*str+1);

                /* For "!=". since '=' is not an allowed conversion character,
                   nothing is lost in this test. */
                if ((ch == '!' && next == '=') ||   /* != */
                    (ch == '=' && next == '=') ||   /* == */
                    (ch == '<' && next == '=') ||   /* <= */
                    (ch == '>' && next == '=')      /* >= */
                    ) {
                    *str += 1;
                    continue;
                }
                /* Don't get out of the loop for these, if they're single
                   chars (not part of 2-char tokens). If by themselves, they
                   don't end an expression (unlike say '!'). */
                if (ch == '>' || ch == '<') {
                    continue;
                }
            }

            /* Normal way out of this loop. */
            break;
        } else if (ch == ']' || ch == '}' || ch == ')') {
            if (!nested_depth) {
                ast_error(c, n, "f-string: unmatched '%c'", ch);
                goto error;
            }
            nested_depth--;
            int opening = parenstack[nested_depth];
            if (!((opening == '(' && ch == ')') ||
                  (opening == '[' && ch == ']') ||
                  (opening == '{' && ch == '}')))
            {
                ast_error(c, n,
                          "f-string: closing parenthesis '%c' "
                          "does not match opening parenthesis '%c'",
                          ch, opening);
                goto error;
            }
        } else {
            /* Just consume this char and loop around. */
        }
    }
    expr_end = *str;
    /* If we leave this loop in a string or with mismatched parens, we
       don't care. We'll get a syntax error when compiling the
       expression. But, we can produce a better error message, so
       let's just do that.*/
    if (quote_char) {
        ast_error(c, n, "f-string: unterminated string");
        goto error;
    }
    if (nested_depth) {
        int opening = parenstack[nested_depth - 1];
        ast_error(c, n, "f-string: unmatched '%c'", opening);
        goto error;
    }

    if (*str >= end)
        goto unexpected_end_of_string;

    /* Compile the expression as soon as possible, so we show errors
       related to the expression before errors related to the
       conversion or format_spec. */
    simple_expression = fstring_compile_expr(expr_start, expr_end, c, n);
    if (!simple_expression)
        goto error;

    /* Check for =, which puts the text value of the expression in
       expr_text. */
    if (**str == '=') {
        *str += 1;

        /* Skip over ASCII whitespace.  No need to test for end of string
           here, since we know there's at least a trailing quote somewhere
           ahead. */
        while (Py_ISSPACE(**str)) {
            *str += 1;
        }

        /* Set *expr_text to the text of the expression. */
        *expr_text = PyUnicode_FromStringAndSize(expr_start, *str-expr_start);
        if (!*expr_text) {
            goto error;
        }
    }

    /* Check for a conversion char, if present. */
    if (**str == '!') {
        *str += 1;
        if (*str >= end)
            goto unexpected_end_of_string;

        conversion = **str;
        *str += 1;

        /* Validate the conversion. */
        if (!(conversion == 's' || conversion == 'r' || conversion == 'a')) {
            ast_error(c, n,
                      "f-string: invalid conversion character: "
                      "expected 's', 'r', or 'a'");
            goto error;
        }

    }

    /* Check for the format spec, if present. */
    if (*str >= end)
        goto unexpected_end_of_string;
    if (**str == ':') {
        *str += 1;
        if (*str >= end)
            goto unexpected_end_of_string;

        /* Parse the format spec. */
        format_spec = fstring_parse(str, end, raw, recurse_lvl+1, c, n);
        if (!format_spec)
            goto error;
    }

    if (*str >= end || **str != '}')
        goto unexpected_end_of_string;

    /* We're at a right brace. Consume it. */
    assert(*str < end);
    assert(**str == '}');
    *str += 1;

    /* If we're in = mode (detected by non-NULL expr_text), and have no format
       spec and no explicit conversion, set the conversion to 'r'. */
    if (*expr_text && format_spec == NULL && conversion == -1) {
        conversion = 'r';
    }

    /* And now create the FormattedValue node that represents this
       entire expression with the conversion and format spec. */
    *expression = FormattedValue(simple_expression, conversion,
                                 format_spec, LINENO(n),
                                 n->n_col_offset, n->n_end_lineno,
                                 n->n_end_col_offset, c->c_arena);
    if (!*expression)
        goto error;

    return 0;

unexpected_end_of_string:
    ast_error(c, n, "f-string: expecting '}'");
    /* Falls through to error. */

error:
    Py_XDECREF(*expr_text);
    return -1;

}

/* Return -1 on error.

   Return 0 if we have a literal (possible zero length) and an
   expression (zero length if at the end of the string.

   Return 1 if we have a literal, but no expression, and we want the
   caller to call us again. This is used to deal with doubled
   braces.

   When called multiple times on the string 'a{{b{0}c', this function
   will return:

   1. the literal 'a{' with no expression, and a return value
      of 1. Despite the fact that there's no expression, the return
      value of 1 means we're not finished yet.

   2. the literal 'b' and the expression '0', with a return value of
      0. The fact that there's an expression means we're not finished.

   3. literal 'c' with no expression and a return value of 0. The
      combination of the return value of 0 with no expression means
      we're finished.
*/
static int
fstring_find_literal_and_expr(const char **str, const char *end, int raw,
                              int recurse_lvl, PyObject **literal,
                              PyObject **expr_text, expr_ty *expression,
                              struct compiling *c, const node *n)
{
    int result;

    assert(*literal == NULL && *expression == NULL);

    /* Get any literal string. */
    result = fstring_find_literal(str, end, raw, literal, recurse_lvl, c, n);
    if (result < 0)
        goto error;

    assert(result == 0 || result == 1);

    if (result == 1)
        /* We have a literal, but don't look at the expression. */
        return 1;

    if (*str >= end || **str == '}')
        /* We're at the end of the string or the end of a nested
           f-string: no expression. The top-level error case where we
           expect to be at the end of the string but we're at a '}' is
           handled later. */
        return 0;

    /* We must now be the start of an expression, on a '{'. */
    assert(**str == '{');

    if (fstring_find_expr(str, end, raw, recurse_lvl, expr_text,
                          expression, c, n) < 0)
        goto error;

    return 0;

error:
    Py_CLEAR(*literal);
    return -1;
}

#define EXPRLIST_N_CACHED  64

typedef struct {
    /* Incrementally build an array of expr_ty, so be used in an
       asdl_seq. Cache some small but reasonably sized number of
       expr_ty's, and then after that start dynamically allocating,
       doubling the number allocated each time. Note that the f-string
       f'{0}a{1}' contains 3 expr_ty's: 2 FormattedValue's, and one
       Constant for the literal 'a'. So you add expr_ty's about twice as
       fast as you add expressions in an f-string. */

    Py_ssize_t allocated;  /* Number we've allocated. */
    Py_ssize_t size;       /* Number we've used. */
    expr_ty    *p;         /* Pointer to the memory we're actually
                              using. Will point to 'data' until we
                              start dynamically allocating. */
    expr_ty    data[EXPRLIST_N_CACHED];
} ExprList;

#ifdef NDEBUG
#define ExprList_check_invariants(l)
#else
static void
ExprList_check_invariants(ExprList *l)
{
    /* Check our invariants. Make sure this object is "live", and
       hasn't been deallocated. */
    assert(l->size >= 0);
    assert(l->p != NULL);
    if (l->size <= EXPRLIST_N_CACHED)
        assert(l->data == l->p);
}
#endif

static void
ExprList_Init(ExprList *l)
{
    l->allocated = EXPRLIST_N_CACHED;
    l->size = 0;

    /* Until we start allocating dynamically, p points to data. */
    l->p = l->data;

    ExprList_check_invariants(l);
}

static int
ExprList_Append(ExprList *l, expr_ty exp)
{
    ExprList_check_invariants(l);
    if (l->size >= l->allocated) {
        /* We need to alloc (or realloc) the memory. */
        Py_ssize_t new_size = l->allocated * 2;

        /* See if we've ever allocated anything dynamically. */
        if (l->p == l->data) {
            Py_ssize_t i;
            /* We're still using the cached data. Switch to
               alloc-ing. */
            l->p = PyMem_RawMalloc(sizeof(expr_ty) * new_size);
            if (!l->p)
                return -1;
            /* Copy the cached data into the new buffer. */
            for (i = 0; i < l->size; i++)
                l->p[i] = l->data[i];
        } else {
            /* Just realloc. */
            expr_ty *tmp = PyMem_RawRealloc(l->p, sizeof(expr_ty) * new_size);
            if (!tmp) {
                PyMem_RawFree(l->p);
                l->p = NULL;
                return -1;
            }
            l->p = tmp;
        }

        l->allocated = new_size;
        assert(l->allocated == 2 * l->size);
    }

    l->p[l->size++] = exp;

    ExprList_check_invariants(l);
    return 0;
}

static void
ExprList_Dealloc(ExprList *l)
{
    ExprList_check_invariants(l);

    /* If there's been an error, or we've never dynamically allocated,
       do nothing. */
    if (!l->p || l->p == l->data) {
        /* Do nothing. */
    } else {
        /* We have dynamically allocated. Free the memory. */
        PyMem_RawFree(l->p);
    }
    l->p = NULL;
    l->size = -1;
}

static asdl_seq *
ExprList_Finish(ExprList *l, PyArena *arena)
{
    asdl_seq *seq;

    ExprList_check_invariants(l);

    /* Allocate the asdl_seq and copy the expressions in to it. */
    seq = _Py_asdl_seq_new(l->size, arena);
    if (seq) {
        Py_ssize_t i;
        for (i = 0; i < l->size; i++)
            asdl_seq_SET(seq, i, l->p[i]);
    }
    ExprList_Dealloc(l);
    return seq;
}

/* The FstringParser is designed to add a mix of strings and
   f-strings, and concat them together as needed. Ultimately, it
   generates an expr_ty. */
typedef struct {
    PyObject *last_str;
    ExprList expr_list;
    int fmode;
} FstringParser;

#ifdef NDEBUG
#define FstringParser_check_invariants(state)
#else
static void
FstringParser_check_invariants(FstringParser *state)
{
    if (state->last_str)
        assert(PyUnicode_CheckExact(state->last_str));
    ExprList_check_invariants(&state->expr_list);
}
#endif

static void
FstringParser_Init(FstringParser *state)
{
    state->last_str = NULL;
    state->fmode = 0;
    ExprList_Init(&state->expr_list);
    FstringParser_check_invariants(state);
}

static void
FstringParser_Dealloc(FstringParser *state)
{
    FstringParser_check_invariants(state);

    Py_XDECREF(state->last_str);
    ExprList_Dealloc(&state->expr_list);
}

/* Constants for the following */
static PyObject *u_kind;

/* Compute 'kind' field for string Constant (either 'u' or None) */
static PyObject *
make_kind(struct compiling *c, const node *n)
{
    char *s = NULL;
    PyObject *kind = NULL;

    /* Find the first string literal, if any */
    while (TYPE(n) != STRING) {
        if (NCH(n) == 0)
            return NULL;
        n = CHILD(n, 0);
    }
    REQ(n, STRING);

    /* If it starts with 'u', return a PyUnicode "u" string */
    s = STR(n);
    if (s && *s == 'u') {
        if (!u_kind) {
            u_kind = PyUnicode_InternFromString("u");
            if (!u_kind)
                return NULL;
        }
        kind = u_kind;
        if (PyArena_AddPyObject(c->c_arena, kind) < 0) {
            return NULL;
        }
        Py_INCREF(kind);
    }
    return kind;
}

/* Make a Constant node, but decref the PyUnicode object being added. */
static expr_ty
make_str_node_and_del(PyObject **str, struct compiling *c, const node* n)
{
    PyObject *s = *str;
    PyObject *kind = NULL;
    *str = NULL;
    assert(PyUnicode_CheckExact(s));
    if (PyArena_AddPyObject(c->c_arena, s) < 0) {
        Py_DECREF(s);
        return NULL;
    }
    kind = make_kind(c, n);
    if (kind == NULL && PyErr_Occurred())
        return NULL;
    return Constant(s, kind, LINENO(n), n->n_col_offset,
                    n->n_end_lineno, n->n_end_col_offset, c->c_arena);
}

/* Add a non-f-string (that is, a regular literal string). str is
   decref'd. */
static int
FstringParser_ConcatAndDel(FstringParser *state, PyObject *str)
{
    FstringParser_check_invariants(state);

    assert(PyUnicode_CheckExact(str));

    if (PyUnicode_GET_LENGTH(str) == 0) {
        Py_DECREF(str);
        return 0;
    }

    if (!state->last_str) {
        /* We didn't have a string before, so just remember this one. */
        state->last_str = str;
    } else {
        /* Concatenate this with the previous string. */
        PyUnicode_AppendAndDel(&state->last_str, str);
        if (!state->last_str)
            return -1;
    }
    FstringParser_check_invariants(state);
    return 0;
}

/* Parse an f-string. The f-string is in *str to end, with no
   'f' or quotes. */
static int
FstringParser_ConcatFstring(FstringParser *state, const char **str,
                            const char *end, int raw, int recurse_lvl,
                            struct compiling *c, const node *n)
{
    FstringParser_check_invariants(state);
    state->fmode = 1;

    /* Parse the f-string. */
    while (1) {
        PyObject *literal = NULL;
        PyObject *expr_text = NULL;
        expr_ty expression = NULL;

        /* If there's a zero length literal in front of the
           expression, literal will be NULL. If we're at the end of
           the f-string, expression will be NULL (unless result == 1,
           see below). */
        int result = fstring_find_literal_and_expr(str, end, raw, recurse_lvl,
                                                   &literal, &expr_text,
                                                   &expression, c, n);
        if (result < 0)
            return -1;

        /* Add the literal, if any. */
        if (literal && FstringParser_ConcatAndDel(state, literal) < 0) {
            Py_XDECREF(expr_text);
            return -1;
        }
        /* Add the expr_text, if any. */
        if (expr_text && FstringParser_ConcatAndDel(state, expr_text) < 0) {
            return -1;
        }

        /* We've dealt with the literal and expr_text, their ownership has
           been transferred to the state object.  Don't look at them again. */

        /* See if we should just loop around to get the next literal
           and expression, while ignoring the expression this
           time. This is used for un-doubling braces, as an
           optimization. */
        if (result == 1)
            continue;

        if (!expression)
            /* We're done with this f-string. */
            break;

        /* We know we have an expression. Convert any existing string
           to a Constant node. */
        if (!state->last_str) {
            /* Do nothing. No previous literal. */
        } else {
            /* Convert the existing last_str literal to a Constant node. */
            expr_ty str = make_str_node_and_del(&state->last_str, c, n);
            if (!str || ExprList_Append(&state->expr_list, str) < 0)
                return -1;
        }

        if (ExprList_Append(&state->expr_list, expression) < 0)
            return -1;
    }

    /* If recurse_lvl is zero, then we must be at the end of the
       string. Otherwise, we must be at a right brace. */

    if (recurse_lvl == 0 && *str < end-1) {
        ast_error(c, n, "f-string: unexpected end of string");
        return -1;
    }
    if (recurse_lvl != 0 && **str != '}') {
        ast_error(c, n, "f-string: expecting '}'");
        return -1;
    }

    FstringParser_check_invariants(state);
    return 0;
}

/* Convert the partial state reflected in last_str and expr_list to an
   expr_ty. The expr_ty can be a Constant, or a JoinedStr. */
static expr_ty
FstringParser_Finish(FstringParser *state, struct compiling *c,
                     const node *n)
{
    asdl_seq *seq;

    FstringParser_check_invariants(state);

    /* If we're just a constant string with no expressions, return
       that. */
    if (!state->fmode) {
        assert(!state->expr_list.size);
        if (!state->last_str) {
            /* Create a zero length string. */
            state->last_str = PyUnicode_FromStringAndSize(NULL, 0);
            if (!state->last_str)
                goto error;
        }
        return make_str_node_and_del(&state->last_str, c, n);
    }

    /* Create a Constant node out of last_str, if needed. It will be the
       last node in our expression list. */
    if (state->last_str) {
        expr_ty str = make_str_node_and_del(&state->last_str, c, n);
        if (!str || ExprList_Append(&state->expr_list, str) < 0)
            goto error;
    }
    /* This has already been freed. */
    assert(state->last_str == NULL);

    seq = ExprList_Finish(&state->expr_list, c->c_arena);
    if (!seq)
        goto error;

    return JoinedStr(seq, LINENO(n), n->n_col_offset,
                     n->n_end_lineno, n->n_end_col_offset, c->c_arena);

error:
    FstringParser_Dealloc(state);
    return NULL;
}

/* Given an f-string (with no 'f' or quotes) that's in *str and ends
   at end, parse it into an expr_ty.  Return NULL on error.  Adjust
   str to point past the parsed portion. */
static expr_ty
fstring_parse(const char **str, const char *end, int raw, int recurse_lvl,
              struct compiling *c, const node *n)
{
    FstringParser state;

    FstringParser_Init(&state);
    if (FstringParser_ConcatFstring(&state, str, end, raw, recurse_lvl,
                                    c, n) < 0) {
        FstringParser_Dealloc(&state);
        return NULL;
    }

    return FstringParser_Finish(&state, c, n);
}

/* n is a Python string literal, including the bracketing quote
   characters, and r, b, u, &/or f prefixes (if any), and embedded
   escape sequences (if any). parsestr parses it, and sets *result to
   decoded Python string object.  If the string is an f-string, set
   *fstr and *fstrlen to the unparsed string object.  Return 0 if no
   errors occurred.
*/
static int
parsestr(struct compiling *c, const node *n, int *bytesmode, int *rawmode,
         PyObject **result, const char **fstr, Py_ssize_t *fstrlen)
{
    size_t len;
    const char *s = STR(n);
    int quote = Py_CHARMASK(*s);
    int fmode = 0;
    *bytesmode = 0;
    *rawmode = 0;
    *result = NULL;
    *fstr = NULL;
    if (Py_ISALPHA(quote)) {
        while (!*bytesmode || !*rawmode) {
            if (quote == 'b' || quote == 'B') {
                quote = *++s;
                *bytesmode = 1;
            }
            else if (quote == 'u' || quote == 'U') {
                quote = *++s;
            }
            else if (quote == 'r' || quote == 'R') {
                quote = *++s;
                *rawmode = 1;
            }
            else if (quote == 'f' || quote == 'F') {
                quote = *++s;
                fmode = 1;
            }
            else {
                break;
            }
        }
    }

    /* fstrings are only allowed in Python 3.6 and greater */
    if (fmode && c->c_feature_version < 6) {
        ast_error(c, n, "Format strings are only supported in Python 3.6 and greater");
        return -1;
    }

    if (fmode && *bytesmode) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (quote != '\'' && quote != '\"') {
        PyErr_BadInternalCall();
        return -1;
    }
    /* Skip the leading quote char. */
    s++;
    len = strlen(s);
    if (len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "string to parse is too long");
        return -1;
    }
    if (s[--len] != quote) {
        /* Last quote char must match the first. */
        PyErr_BadInternalCall();
        return -1;
    }
    if (len >= 4 && s[0] == quote && s[1] == quote) {
        /* A triple quoted string. We've already skipped one quote at
           the start and one at the end of the string. Now skip the
           two at the start. */
        s += 2;
        len -= 2;
        /* And check that the last two match. */
        if (s[--len] != quote || s[--len] != quote) {
            PyErr_BadInternalCall();
            return -1;
        }
    }

    if (fmode) {
        /* Just return the bytes. The caller will parse the resulting
           string. */
        *fstr = s;
        *fstrlen = len;
        return 0;
    }

    /* Not an f-string. */
    /* Avoid invoking escape decoding routines if possible. */
    *rawmode = *rawmode || strchr(s, '\\') == NULL;
    if (*bytesmode) {
        /* Disallow non-ASCII characters. */
        const char *ch;
        for (ch = s; *ch; ch++) {
            if (Py_CHARMASK(*ch) >= 0x80) {
                ast_error(c, n,
                          "bytes can only contain ASCII "
                          "literal characters.");
                return -1;
            }
        }
        if (*rawmode)
            *result = PyBytes_FromStringAndSize(s, len);
        else
            *result = decode_bytes_with_escapes(c, n, s, len);
    } else {
        if (*rawmode)
            *result = PyUnicode_DecodeUTF8Stateful(s, len, NULL, NULL);
        else
            *result = decode_unicode_with_escapes(c, n, s, len);
    }
    return *result == NULL ? -1 : 0;
}

/* Accepts a STRING+ atom, and produces an expr_ty node. Run through
   each STRING atom, and process it as needed. For bytes, just
   concatenate them together, and the result will be a Constant node. For
   normal strings and f-strings, concatenate them together. The result
   will be a Constant node if there were no f-strings; a FormattedValue
   node if there's just an f-string (with no leading or trailing
   literals), or a JoinedStr node if there are multiple f-strings or
   any literals involved. */
static expr_ty
parsestrplus(struct compiling *c, const node *n)
{
    int bytesmode = 0;
    PyObject *bytes_str = NULL;
    int i;

    FstringParser state;
    FstringParser_Init(&state);

    for (i = 0; i < NCH(n); i++) {
        int this_bytesmode;
        int this_rawmode;
        PyObject *s;
        const char *fstr;
        Py_ssize_t fstrlen = -1;  /* Silence a compiler warning. */

        REQ(CHILD(n, i), STRING);
        if (parsestr(c, CHILD(n, i), &this_bytesmode, &this_rawmode, &s,
                     &fstr, &fstrlen) != 0)
            goto error;

        /* Check that we're not mixing bytes with unicode. */
        if (i != 0 && bytesmode != this_bytesmode) {
            ast_error(c, n, "cannot mix bytes and nonbytes literals");
            /* s is NULL if the current string part is an f-string. */
            Py_XDECREF(s);
            goto error;
        }
        bytesmode = this_bytesmode;

        if (fstr != NULL) {
            int result;
            assert(s == NULL && !bytesmode);
            /* This is an f-string. Parse and concatenate it. */
            result = FstringParser_ConcatFstring(&state, &fstr, fstr+fstrlen,
                                                 this_rawmode, 0, c, n);
            if (result < 0)
                goto error;
        } else {
            /* A string or byte string. */
            assert(s != NULL && fstr == NULL);

            assert(bytesmode ? PyBytes_CheckExact(s) :
                   PyUnicode_CheckExact(s));

            if (bytesmode) {
                /* For bytes, concat as we go. */
                if (i == 0) {
                    /* First time, just remember this value. */
                    bytes_str = s;
                } else {
                    PyBytes_ConcatAndDel(&bytes_str, s);
                    if (!bytes_str)
                        goto error;
                }
            } else {
                /* This is a regular string. Concatenate it. */
                if (FstringParser_ConcatAndDel(&state, s) < 0)
                    goto error;
            }
        }
    }
    if (bytesmode) {
        /* Just return the bytes object and we're done. */
        if (PyArena_AddPyObject(c->c_arena, bytes_str) < 0)
            goto error;
        return Constant(bytes_str, NULL, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }

    /* We're not a bytes string, bytes_str should never have been set. */
    assert(bytes_str == NULL);

    return FstringParser_Finish(&state, c, n);

error:
    Py_XDECREF(bytes_str);
    FstringParser_Dealloc(&state);
    return NULL;
}

PyObject *
_PyAST_GetDocString(asdl_seq *body)
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
