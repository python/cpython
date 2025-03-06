/* AST Optimizer */
#include "Python.h"
#include "pycore_ast.h"           // _PyAST_GetDocString()
#include "pycore_format.h"        // F_LJUST
#include "pycore_long.h"          // _PyLong
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_setobject.h"     // _PySet_NextEntry()


typedef struct {
    int optimize;
    int ff_features;
} _PyASTOptimizeState;

#define ENTER_RECURSIVE() \
if (Py_EnterRecursiveCall(" during compilation")) { \
    return 0; \
}

#define LEAVE_RECURSIVE() Py_LeaveRecursiveCall();

static int
make_const(expr_ty node, PyObject *val, PyArena *arena)
{
    // Even if no new value was calculated, make_const may still
    // need to clear an error (e.g. for division by zero)
    if (val == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            return 0;
        }
        PyErr_Clear();
        return 1;
    }
    if (_PyArena_AddPyObject(arena, val) < 0) {
        Py_DECREF(val);
        return 0;
    }
    node->kind = Constant_kind;
    node->v.Constant.kind = NULL;
    node->v.Constant.value = val;
    return 1;
}

#define COPY_NODE(TO, FROM) (memcpy((TO), (FROM), sizeof(struct _expr)))

static int
has_starred(asdl_expr_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(elts, i);
        if (e->kind == Starred_kind) {
            return 1;
        }
    }
    return 0;
}

static expr_ty
parse_literal(PyObject *fmt, Py_ssize_t *ppos, PyArena *arena)
{
    const void *data = PyUnicode_DATA(fmt);
    int kind = PyUnicode_KIND(fmt);
    Py_ssize_t size = PyUnicode_GET_LENGTH(fmt);
    Py_ssize_t start, pos;
    int has_percents = 0;
    start = pos = *ppos;
    while (pos < size) {
        if (PyUnicode_READ(kind, data, pos) != '%') {
            pos++;
        }
        else if (pos+1 < size && PyUnicode_READ(kind, data, pos+1) == '%') {
            has_percents = 1;
            pos += 2;
        }
        else {
            break;
        }
    }
    *ppos = pos;
    if (pos == start) {
        return NULL;
    }
    PyObject *str = PyUnicode_Substring(fmt, start, pos);
    /* str = str.replace('%%', '%') */
    if (str && has_percents) {
        _Py_DECLARE_STR(dbl_percent, "%%");
        Py_SETREF(str, PyUnicode_Replace(str, &_Py_STR(dbl_percent),
                                         _Py_LATIN1_CHR('%'), -1));
    }
    if (!str) {
        return NULL;
    }

    if (_PyArena_AddPyObject(arena, str) < 0) {
        Py_DECREF(str);
        return NULL;
    }
    return _PyAST_Constant(str, NULL, -1, -1, -1, -1, arena);
}

#define MAXDIGITS 3

static int
simple_format_arg_parse(PyObject *fmt, Py_ssize_t *ppos,
                        int *spec, int *flags, int *width, int *prec)
{
    Py_ssize_t pos = *ppos, len = PyUnicode_GET_LENGTH(fmt);
    Py_UCS4 ch;

#define NEXTC do {                      \
    if (pos >= len) {                   \
        return 0;                       \
    }                                   \
    ch = PyUnicode_READ_CHAR(fmt, pos); \
    pos++;                              \
} while (0)

    *flags = 0;
    while (1) {
        NEXTC;
        switch (ch) {
            case '-': *flags |= F_LJUST; continue;
            case '+': *flags |= F_SIGN; continue;
            case ' ': *flags |= F_BLANK; continue;
            case '#': *flags |= F_ALT; continue;
            case '0': *flags |= F_ZERO; continue;
        }
        break;
    }
    if ('0' <= ch && ch <= '9') {
        *width = 0;
        int digits = 0;
        while ('0' <= ch && ch <= '9') {
            *width = *width * 10 + (ch - '0');
            NEXTC;
            if (++digits >= MAXDIGITS) {
                return 0;
            }
        }
    }

    if (ch == '.') {
        NEXTC;
        *prec = 0;
        if ('0' <= ch && ch <= '9') {
            int digits = 0;
            while ('0' <= ch && ch <= '9') {
                *prec = *prec * 10 + (ch - '0');
                NEXTC;
                if (++digits >= MAXDIGITS) {
                    return 0;
                }
            }
        }
    }
    *spec = ch;
    *ppos = pos;
    return 1;

#undef NEXTC
}

static expr_ty
parse_format(PyObject *fmt, Py_ssize_t *ppos, expr_ty arg, PyArena *arena)
{
    int spec, flags, width = -1, prec = -1;
    if (!simple_format_arg_parse(fmt, ppos, &spec, &flags, &width, &prec)) {
        // Unsupported format.
        return NULL;
    }
    if (spec == 's' || spec == 'r' || spec == 'a') {
        char buf[1 + MAXDIGITS + 1 + MAXDIGITS + 1], *p = buf;
        if (!(flags & F_LJUST) && width > 0) {
            *p++ = '>';
        }
        if (width >= 0) {
            p += snprintf(p, MAXDIGITS + 1, "%d", width);
        }
        if (prec >= 0) {
            p += snprintf(p, MAXDIGITS + 2, ".%d", prec);
        }
        expr_ty format_spec = NULL;
        if (p != buf) {
            PyObject *str = PyUnicode_FromString(buf);
            if (str == NULL) {
                return NULL;
            }
            if (_PyArena_AddPyObject(arena, str) < 0) {
                Py_DECREF(str);
                return NULL;
            }
            format_spec = _PyAST_Constant(str, NULL, -1, -1, -1, -1, arena);
            if (format_spec == NULL) {
                return NULL;
            }
        }
        return _PyAST_FormattedValue(arg, spec, format_spec,
                                     arg->lineno, arg->col_offset,
                                     arg->end_lineno, arg->end_col_offset,
                                     arena);
    }
    // Unsupported format.
    return NULL;
}

static int
optimize_format(expr_ty node, PyObject *fmt, asdl_expr_seq *elts, PyArena *arena)
{
    Py_ssize_t pos = 0;
    Py_ssize_t cnt = 0;
    asdl_expr_seq *seq = _Py_asdl_expr_seq_new(asdl_seq_LEN(elts) * 2 + 1, arena);
    if (!seq) {
        return 0;
    }
    seq->size = 0;

    while (1) {
        expr_ty lit = parse_literal(fmt, &pos, arena);
        if (lit) {
            asdl_seq_SET(seq, seq->size++, lit);
        }
        else if (PyErr_Occurred()) {
            return 0;
        }

        if (pos >= PyUnicode_GET_LENGTH(fmt)) {
            break;
        }
        if (cnt >= asdl_seq_LEN(elts)) {
            // More format units than items.
            return 1;
        }
        assert(PyUnicode_READ_CHAR(fmt, pos) == '%');
        pos++;
        expr_ty expr = parse_format(fmt, &pos, asdl_seq_GET(elts, cnt), arena);
        cnt++;
        if (!expr) {
            return !PyErr_Occurred();
        }
        asdl_seq_SET(seq, seq->size++, expr);
    }
    if (cnt < asdl_seq_LEN(elts)) {
        // More items than format units.
        return 1;
    }
    expr_ty res = _PyAST_JoinedStr(seq,
                                   node->lineno, node->col_offset,
                                   node->end_lineno, node->end_col_offset,
                                   arena);
    if (!res) {
        return 0;
    }
    COPY_NODE(node, res);
//     PySys_FormatStderr("format = %R\n", fmt);
    return 1;
}

static int
fold_binop(expr_ty node, PyArena *arena, _PyASTOptimizeState *state)
{
    expr_ty lhs, rhs;
    lhs = node->v.BinOp.left;
    rhs = node->v.BinOp.right;
    if (lhs->kind != Constant_kind) {
        return 1;
    }
    PyObject *lv = lhs->v.Constant.value;

    if (node->v.BinOp.op == Mod &&
        rhs->kind == Tuple_kind &&
        PyUnicode_Check(lv) &&
        !has_starred(rhs->v.Tuple.elts))
    {
        return optimize_format(node, lv, rhs->v.Tuple.elts, arena);
    }

    return 1;
}

static PyObject*
make_const_tuple(asdl_expr_seq *elts)
{
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(elts); i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(elts, i);
        if (e->kind != Constant_kind) {
            return NULL;
        }
    }

    PyObject *newval = PyTuple_New(asdl_seq_LEN(elts));
    if (newval == NULL) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < asdl_seq_LEN(elts); i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(elts, i);
        PyObject *v = e->v.Constant.value;
        PyTuple_SET_ITEM(newval, i, Py_NewRef(v));
    }
    return newval;
}

static int
fold_tuple(expr_ty node, PyArena *arena, _PyASTOptimizeState *state)
{
    PyObject *newval;

    if (node->v.Tuple.ctx != Load)
        return 1;

    newval = make_const_tuple(node->v.Tuple.elts);
    return make_const(node, newval, arena);
}

static int astfold_mod(mod_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_stmt(stmt_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_expr(expr_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_arguments(arguments_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_comprehension(comprehension_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_keyword(keyword_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_arg(arg_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_withitem(withitem_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_excepthandler(excepthandler_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_match_case(match_case_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_pattern(pattern_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);
static int astfold_type_param(type_param_ty node_, PyArena *ctx_, _PyASTOptimizeState *state);

#define CALL(FUNC, TYPE, ARG) \
    if (!FUNC((ARG), ctx_, state)) \
        return 0;

#define CALL_OPT(FUNC, TYPE, ARG) \
    if ((ARG) != NULL && !FUNC((ARG), ctx_, state)) \
        return 0;

#define CALL_SEQ(FUNC, TYPE, ARG) { \
    Py_ssize_t i; \
    asdl_ ## TYPE ## _seq *seq = (ARG); /* avoid variable capture */ \
    for (i = 0; i < asdl_seq_LEN(seq); i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, i); \
        if (elt != NULL && !FUNC(elt, ctx_, state)) \
            return 0; \
    } \
}


static int
stmt_seq_remove_item(asdl_stmt_seq *stmts, Py_ssize_t idx)
{
    if (idx >= asdl_seq_LEN(stmts)) {
        return 0;
    }
    for (Py_ssize_t i = idx; i < asdl_seq_LEN(stmts) - 1; i++) {
        stmt_ty st = (stmt_ty)asdl_seq_GET(stmts, i+1);
        asdl_seq_SET(stmts, i, st);
    }
    stmts->size--;
    return 1;
}

static int
astfold_body(asdl_stmt_seq *stmts, PyArena *ctx_, _PyASTOptimizeState *state)
{
    int docstring = _PyAST_GetDocString(stmts) != NULL;
    if (docstring && (state->optimize >= 2)) {
        /* remove the docstring */
        if (!stmt_seq_remove_item(stmts, 0)) {
            return 0;
        }
        docstring = 0;
    }
    CALL_SEQ(astfold_stmt, stmt, stmts);
    if (!docstring && _PyAST_GetDocString(stmts) != NULL) {
        stmt_ty st = (stmt_ty)asdl_seq_GET(stmts, 0);
        asdl_expr_seq *values = _Py_asdl_expr_seq_new(1, ctx_);
        if (!values) {
            return 0;
        }
        asdl_seq_SET(values, 0, st->v.Expr.value);
        expr_ty expr = _PyAST_JoinedStr(values, st->lineno, st->col_offset,
                                        st->end_lineno, st->end_col_offset,
                                        ctx_);
        if (!expr) {
            return 0;
        }
        st->v.Expr.value = expr;
    }
    return 1;
}

static int
astfold_mod(mod_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    switch (node_->kind) {
    case Module_kind:
        CALL(astfold_body, asdl_seq, node_->v.Module.body);
        break;
    case Interactive_kind:
        CALL_SEQ(astfold_stmt, stmt, node_->v.Interactive.body);
        break;
    case Expression_kind:
        CALL(astfold_expr, expr_ty, node_->v.Expression.body);
        break;
    // The following top level nodes don't participate in constant folding
    case FunctionType_kind:
        break;
    // No default case, so the compiler will emit a warning if new top level
    // compilation nodes are added without being handled here
    }
    return 1;
}

static int
astfold_expr(expr_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    ENTER_RECURSIVE();
    switch (node_->kind) {
    case BoolOp_kind:
        CALL_SEQ(astfold_expr, expr, node_->v.BoolOp.values);
        break;
    case BinOp_kind:
        CALL(astfold_expr, expr_ty, node_->v.BinOp.left);
        CALL(astfold_expr, expr_ty, node_->v.BinOp.right);
        CALL(fold_binop, expr_ty, node_);
        break;
    case UnaryOp_kind:
        CALL(astfold_expr, expr_ty, node_->v.UnaryOp.operand);
        break;
    case Lambda_kind:
        CALL(astfold_arguments, arguments_ty, node_->v.Lambda.args);
        CALL(astfold_expr, expr_ty, node_->v.Lambda.body);
        break;
    case IfExp_kind:
        CALL(astfold_expr, expr_ty, node_->v.IfExp.test);
        CALL(astfold_expr, expr_ty, node_->v.IfExp.body);
        CALL(astfold_expr, expr_ty, node_->v.IfExp.orelse);
        break;
    case Dict_kind:
        CALL_SEQ(astfold_expr, expr, node_->v.Dict.keys);
        CALL_SEQ(astfold_expr, expr, node_->v.Dict.values);
        break;
    case Set_kind:
        CALL_SEQ(astfold_expr, expr, node_->v.Set.elts);
        break;
    case ListComp_kind:
        CALL(astfold_expr, expr_ty, node_->v.ListComp.elt);
        CALL_SEQ(astfold_comprehension, comprehension, node_->v.ListComp.generators);
        break;
    case SetComp_kind:
        CALL(astfold_expr, expr_ty, node_->v.SetComp.elt);
        CALL_SEQ(astfold_comprehension, comprehension, node_->v.SetComp.generators);
        break;
    case DictComp_kind:
        CALL(astfold_expr, expr_ty, node_->v.DictComp.key);
        CALL(astfold_expr, expr_ty, node_->v.DictComp.value);
        CALL_SEQ(astfold_comprehension, comprehension, node_->v.DictComp.generators);
        break;
    case GeneratorExp_kind:
        CALL(astfold_expr, expr_ty, node_->v.GeneratorExp.elt);
        CALL_SEQ(astfold_comprehension, comprehension, node_->v.GeneratorExp.generators);
        break;
    case Await_kind:
        CALL(astfold_expr, expr_ty, node_->v.Await.value);
        break;
    case Yield_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.Yield.value);
        break;
    case YieldFrom_kind:
        CALL(astfold_expr, expr_ty, node_->v.YieldFrom.value);
        break;
    case Compare_kind:
        CALL(astfold_expr, expr_ty, node_->v.Compare.left);
        CALL_SEQ(astfold_expr, expr, node_->v.Compare.comparators);
        break;
    case Call_kind:
        CALL(astfold_expr, expr_ty, node_->v.Call.func);
        CALL_SEQ(astfold_expr, expr, node_->v.Call.args);
        CALL_SEQ(astfold_keyword, keyword, node_->v.Call.keywords);
        break;
    case FormattedValue_kind:
        CALL(astfold_expr, expr_ty, node_->v.FormattedValue.value);
        CALL_OPT(astfold_expr, expr_ty, node_->v.FormattedValue.format_spec);
        break;
    case JoinedStr_kind:
        CALL_SEQ(astfold_expr, expr, node_->v.JoinedStr.values);
        break;
    case Attribute_kind:
        CALL(astfold_expr, expr_ty, node_->v.Attribute.value);
        break;
    case Subscript_kind:
        CALL(astfold_expr, expr_ty, node_->v.Subscript.value);
        CALL(astfold_expr, expr_ty, node_->v.Subscript.slice);
        break;
    case Starred_kind:
        CALL(astfold_expr, expr_ty, node_->v.Starred.value);
        break;
    case Slice_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.Slice.lower);
        CALL_OPT(astfold_expr, expr_ty, node_->v.Slice.upper);
        CALL_OPT(astfold_expr, expr_ty, node_->v.Slice.step);
        break;
    case List_kind:
        CALL_SEQ(astfold_expr, expr, node_->v.List.elts);
        break;
    case Tuple_kind:
        CALL_SEQ(astfold_expr, expr, node_->v.Tuple.elts);
        CALL(fold_tuple, expr_ty, node_);
        break;
    case Name_kind:
        if (node_->v.Name.ctx == Load &&
                _PyUnicode_EqualToASCIIString(node_->v.Name.id, "__debug__")) {
            LEAVE_RECURSIVE();
            return make_const(node_, PyBool_FromLong(!state->optimize), ctx_);
        }
        break;
    case NamedExpr_kind:
        CALL(astfold_expr, expr_ty, node_->v.NamedExpr.value);
        break;
    case Constant_kind:
        // Already a constant, nothing further to do
        break;
    // No default case, so the compiler will emit a warning if new expression
    // kinds are added without being handled here
    }
    LEAVE_RECURSIVE();
    return 1;
}

static int
astfold_keyword(keyword_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    CALL(astfold_expr, expr_ty, node_->value);
    return 1;
}

static int
astfold_comprehension(comprehension_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    CALL(astfold_expr, expr_ty, node_->target);
    CALL(astfold_expr, expr_ty, node_->iter);
    CALL_SEQ(astfold_expr, expr, node_->ifs);
    return 1;
}

static int
astfold_arguments(arguments_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    CALL_SEQ(astfold_arg, arg, node_->posonlyargs);
    CALL_SEQ(astfold_arg, arg, node_->args);
    CALL_OPT(astfold_arg, arg_ty, node_->vararg);
    CALL_SEQ(astfold_arg, arg, node_->kwonlyargs);
    CALL_SEQ(astfold_expr, expr, node_->kw_defaults);
    CALL_OPT(astfold_arg, arg_ty, node_->kwarg);
    CALL_SEQ(astfold_expr, expr, node_->defaults);
    return 1;
}

static int
astfold_arg(arg_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    if (!(state->ff_features & CO_FUTURE_ANNOTATIONS)) {
        CALL_OPT(astfold_expr, expr_ty, node_->annotation);
    }
    return 1;
}

static int
astfold_stmt(stmt_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    ENTER_RECURSIVE();
    switch (node_->kind) {
    case FunctionDef_kind:
        CALL_SEQ(astfold_type_param, type_param, node_->v.FunctionDef.type_params);
        CALL(astfold_arguments, arguments_ty, node_->v.FunctionDef.args);
        CALL(astfold_body, asdl_seq, node_->v.FunctionDef.body);
        CALL_SEQ(astfold_expr, expr, node_->v.FunctionDef.decorator_list);
        if (!(state->ff_features & CO_FUTURE_ANNOTATIONS)) {
            CALL_OPT(astfold_expr, expr_ty, node_->v.FunctionDef.returns);
        }
        break;
    case AsyncFunctionDef_kind:
        CALL_SEQ(astfold_type_param, type_param, node_->v.AsyncFunctionDef.type_params);
        CALL(astfold_arguments, arguments_ty, node_->v.AsyncFunctionDef.args);
        CALL(astfold_body, asdl_seq, node_->v.AsyncFunctionDef.body);
        CALL_SEQ(astfold_expr, expr, node_->v.AsyncFunctionDef.decorator_list);
        if (!(state->ff_features & CO_FUTURE_ANNOTATIONS)) {
            CALL_OPT(astfold_expr, expr_ty, node_->v.AsyncFunctionDef.returns);
        }
        break;
    case ClassDef_kind:
        CALL_SEQ(astfold_type_param, type_param, node_->v.ClassDef.type_params);
        CALL_SEQ(astfold_expr, expr, node_->v.ClassDef.bases);
        CALL_SEQ(astfold_keyword, keyword, node_->v.ClassDef.keywords);
        CALL(astfold_body, asdl_seq, node_->v.ClassDef.body);
        CALL_SEQ(astfold_expr, expr, node_->v.ClassDef.decorator_list);
        break;
    case Return_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.Return.value);
        break;
    case Delete_kind:
        CALL_SEQ(astfold_expr, expr, node_->v.Delete.targets);
        break;
    case Assign_kind:
        CALL_SEQ(astfold_expr, expr, node_->v.Assign.targets);
        CALL(astfold_expr, expr_ty, node_->v.Assign.value);
        break;
    case AugAssign_kind:
        CALL(astfold_expr, expr_ty, node_->v.AugAssign.target);
        CALL(astfold_expr, expr_ty, node_->v.AugAssign.value);
        break;
    case AnnAssign_kind:
        CALL(astfold_expr, expr_ty, node_->v.AnnAssign.target);
        if (!(state->ff_features & CO_FUTURE_ANNOTATIONS)) {
            CALL(astfold_expr, expr_ty, node_->v.AnnAssign.annotation);
        }
        CALL_OPT(astfold_expr, expr_ty, node_->v.AnnAssign.value);
        break;
    case TypeAlias_kind:
        CALL(astfold_expr, expr_ty, node_->v.TypeAlias.name);
        CALL_SEQ(astfold_type_param, type_param, node_->v.TypeAlias.type_params);
        CALL(astfold_expr, expr_ty, node_->v.TypeAlias.value);
        break;
    case For_kind:
        CALL(astfold_expr, expr_ty, node_->v.For.target);
        CALL(astfold_expr, expr_ty, node_->v.For.iter);
        CALL_SEQ(astfold_stmt, stmt, node_->v.For.body);
        CALL_SEQ(astfold_stmt, stmt, node_->v.For.orelse);
        break;
    case AsyncFor_kind:
        CALL(astfold_expr, expr_ty, node_->v.AsyncFor.target);
        CALL(astfold_expr, expr_ty, node_->v.AsyncFor.iter);
        CALL_SEQ(astfold_stmt, stmt, node_->v.AsyncFor.body);
        CALL_SEQ(astfold_stmt, stmt, node_->v.AsyncFor.orelse);
        break;
    case While_kind:
        CALL(astfold_expr, expr_ty, node_->v.While.test);
        CALL_SEQ(astfold_stmt, stmt, node_->v.While.body);
        CALL_SEQ(astfold_stmt, stmt, node_->v.While.orelse);
        break;
    case If_kind:
        CALL(astfold_expr, expr_ty, node_->v.If.test);
        CALL_SEQ(astfold_stmt, stmt, node_->v.If.body);
        CALL_SEQ(astfold_stmt, stmt, node_->v.If.orelse);
        break;
    case With_kind:
        CALL_SEQ(astfold_withitem, withitem, node_->v.With.items);
        CALL_SEQ(astfold_stmt, stmt, node_->v.With.body);
        break;
    case AsyncWith_kind:
        CALL_SEQ(astfold_withitem, withitem, node_->v.AsyncWith.items);
        CALL_SEQ(astfold_stmt, stmt, node_->v.AsyncWith.body);
        break;
    case Raise_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.Raise.exc);
        CALL_OPT(astfold_expr, expr_ty, node_->v.Raise.cause);
        break;
    case Try_kind:
        CALL_SEQ(astfold_stmt, stmt, node_->v.Try.body);
        CALL_SEQ(astfold_excepthandler, excepthandler, node_->v.Try.handlers);
        CALL_SEQ(astfold_stmt, stmt, node_->v.Try.orelse);
        CALL_SEQ(astfold_stmt, stmt, node_->v.Try.finalbody);
        break;
    case TryStar_kind:
        CALL_SEQ(astfold_stmt, stmt, node_->v.TryStar.body);
        CALL_SEQ(astfold_excepthandler, excepthandler, node_->v.TryStar.handlers);
        CALL_SEQ(astfold_stmt, stmt, node_->v.TryStar.orelse);
        CALL_SEQ(astfold_stmt, stmt, node_->v.TryStar.finalbody);
        break;
    case Assert_kind:
        CALL(astfold_expr, expr_ty, node_->v.Assert.test);
        CALL_OPT(astfold_expr, expr_ty, node_->v.Assert.msg);
        break;
    case Expr_kind:
        CALL(astfold_expr, expr_ty, node_->v.Expr.value);
        break;
    case Match_kind:
        CALL(astfold_expr, expr_ty, node_->v.Match.subject);
        CALL_SEQ(astfold_match_case, match_case, node_->v.Match.cases);
        break;
    // The following statements don't contain any subexpressions to be folded
    case Import_kind:
    case ImportFrom_kind:
    case Global_kind:
    case Nonlocal_kind:
    case Pass_kind:
    case Break_kind:
    case Continue_kind:
        break;
    // No default case, so the compiler will emit a warning if new statement
    // kinds are added without being handled here
    }
    LEAVE_RECURSIVE();
    return 1;
}

static int
astfold_excepthandler(excepthandler_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    switch (node_->kind) {
    case ExceptHandler_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.ExceptHandler.type);
        CALL_SEQ(astfold_stmt, stmt, node_->v.ExceptHandler.body);
        break;
    // No default case, so the compiler will emit a warning if new handler
    // kinds are added without being handled here
    }
    return 1;
}

static int
astfold_withitem(withitem_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    CALL(astfold_expr, expr_ty, node_->context_expr);
    CALL_OPT(astfold_expr, expr_ty, node_->optional_vars);
    return 1;
}

static int
fold_const_match_patterns(expr_ty node, PyArena *ctx_, _PyASTOptimizeState *state)
{
    switch (node->kind)
    {
        case UnaryOp_kind:
        {
            if (node->v.UnaryOp.op == USub &&
                node->v.UnaryOp.operand->kind == Constant_kind)
            {
                PyObject *operand = node->v.UnaryOp.operand->v.Constant.value;
                PyObject *folded = PyNumber_Negative(operand);
                return make_const(node, folded, ctx_);
            }
            break;
        }
        case BinOp_kind:
        {
            operator_ty op = node->v.BinOp.op;
            if ((op == Add || op == Sub) &&
                node->v.BinOp.right->kind == Constant_kind)
            {
                CALL(fold_const_match_patterns, expr_ty, node->v.BinOp.left);
                if (node->v.BinOp.left->kind == Constant_kind) {
                    PyObject *left = node->v.BinOp.left->v.Constant.value;
                    PyObject *right = node->v.BinOp.right->v.Constant.value;
                    PyObject *folded = op == Add ? PyNumber_Add(left, right) : PyNumber_Subtract(left, right);
                    return make_const(node, folded, ctx_);
                }
            }
            break;
        }
        default:
            break;
    }
    return 1;
}

static int
astfold_pattern(pattern_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    // Currently, this is really only used to form complex/negative numeric
    // constants in MatchValue and MatchMapping nodes
    // We still recurse into all subexpressions and subpatterns anyway
    ENTER_RECURSIVE();
    switch (node_->kind) {
        case MatchValue_kind:
            CALL(fold_const_match_patterns, expr_ty, node_->v.MatchValue.value);
            break;
        case MatchSingleton_kind:
            break;
        case MatchSequence_kind:
            CALL_SEQ(astfold_pattern, pattern, node_->v.MatchSequence.patterns);
            break;
        case MatchMapping_kind:
            CALL_SEQ(fold_const_match_patterns, expr, node_->v.MatchMapping.keys);
            CALL_SEQ(astfold_pattern, pattern, node_->v.MatchMapping.patterns);
            break;
        case MatchClass_kind:
            CALL(astfold_expr, expr_ty, node_->v.MatchClass.cls);
            CALL_SEQ(astfold_pattern, pattern, node_->v.MatchClass.patterns);
            CALL_SEQ(astfold_pattern, pattern, node_->v.MatchClass.kwd_patterns);
            break;
        case MatchStar_kind:
            break;
        case MatchAs_kind:
            if (node_->v.MatchAs.pattern) {
                CALL(astfold_pattern, pattern_ty, node_->v.MatchAs.pattern);
            }
            break;
        case MatchOr_kind:
            CALL_SEQ(astfold_pattern, pattern, node_->v.MatchOr.patterns);
            break;
    // No default case, so the compiler will emit a warning if new pattern
    // kinds are added without being handled here
    }
    LEAVE_RECURSIVE();
    return 1;
}

static int
astfold_match_case(match_case_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    CALL(astfold_pattern, expr_ty, node_->pattern);
    CALL_OPT(astfold_expr, expr_ty, node_->guard);
    CALL_SEQ(astfold_stmt, stmt, node_->body);
    return 1;
}

static int
astfold_type_param(type_param_ty node_, PyArena *ctx_, _PyASTOptimizeState *state)
{
    switch (node_->kind) {
        case TypeVar_kind:
            CALL_OPT(astfold_expr, expr_ty, node_->v.TypeVar.bound);
            CALL_OPT(astfold_expr, expr_ty, node_->v.TypeVar.default_value);
            break;
        case ParamSpec_kind:
            CALL_OPT(astfold_expr, expr_ty, node_->v.ParamSpec.default_value);
            break;
        case TypeVarTuple_kind:
            CALL_OPT(astfold_expr, expr_ty, node_->v.TypeVarTuple.default_value);
            break;
    }
    return 1;
}

#undef CALL
#undef CALL_OPT
#undef CALL_SEQ

int
_PyAST_Optimize(mod_ty mod, PyArena *arena, int optimize, int ff_features)
{
    _PyASTOptimizeState state;
    state.optimize = optimize;
    state.ff_features = ff_features;

    int ret = astfold_mod(mod, arena, &state);
    assert(ret || PyErr_Occurred());

    return ret;
}
