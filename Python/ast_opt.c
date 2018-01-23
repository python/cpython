/* AST Optimizer */
#include "Python.h"
#include "Python-ast.h"


/* TODO: is_const and get_const_value are copied from Python/compile.c.
   It should be deduped in the future.  Maybe, we can include this file
   from compile.c?
*/
static int
is_const(expr_ty e)
{
    switch (e->kind) {
    case Constant_kind:
    case Num_kind:
    case Str_kind:
    case Bytes_kind:
    case Ellipsis_kind:
    case NameConstant_kind:
        return 1;
    default:
        return 0;
    }
}

static PyObject *
get_const_value(expr_ty e)
{
    switch (e->kind) {
    case Constant_kind:
        return e->v.Constant.value;
    case Num_kind:
        return e->v.Num.n;
    case Str_kind:
        return e->v.Str.s;
    case Bytes_kind:
        return e->v.Bytes.s;
    case Ellipsis_kind:
        return Py_Ellipsis;
    case NameConstant_kind:
        return e->v.NameConstant.value;
    default:
        Py_UNREACHABLE();
    }
}

static int
make_const(expr_ty node, PyObject *val, PyArena *arena)
{
    if (val == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            return 0;
        }
        PyErr_Clear();
        return 1;
    }
    if (PyArena_AddPyObject(arena, val) < 0) {
        Py_DECREF(val);
        return 0;
    }
    node->kind = Constant_kind;
    node->v.Constant.value = val;
    return 1;
}

#define COPY_NODE(TO, FROM) (memcpy((TO), (FROM), sizeof(struct _expr)))

static PyObject*
unary_not(PyObject *v)
{
    int r = PyObject_IsTrue(v);
    if (r < 0)
        return NULL;
    return PyBool_FromLong(!r);
}

static int
fold_unaryop(expr_ty node, PyArena *arena, int optimize)
{
    expr_ty arg = node->v.UnaryOp.operand;

    if (!is_const(arg)) {
        /* Fold not into comparison */
        if (node->v.UnaryOp.op == Not && arg->kind == Compare_kind &&
                asdl_seq_LEN(arg->v.Compare.ops) == 1) {
            /* Eq and NotEq are often implemented in terms of one another, so
               folding not (self == other) into self != other breaks implementation
               of !=. Detecting such cases doesn't seem worthwhile.
               Python uses </> for 'is subset'/'is superset' operations on sets.
               They don't satisfy not folding laws. */
            int op = asdl_seq_GET(arg->v.Compare.ops, 0);
            switch (op) {
            case Is:
                op = IsNot;
                break;
            case IsNot:
                op = Is;
                break;
            case In:
                op = NotIn;
                break;
            case NotIn:
                op = In;
                break;
            default:
                op = 0;
            }
            if (op) {
                asdl_seq_SET(arg->v.Compare.ops, 0, op);
                COPY_NODE(node, arg);
                return 1;
            }
        }
        return 1;
    }

    typedef PyObject *(*unary_op)(PyObject*);
    static const unary_op ops[] = {
        [Invert] = PyNumber_Invert,
        [Not] = unary_not,
        [UAdd] = PyNumber_Positive,
        [USub] = PyNumber_Negative,
    };
    PyObject *newval = ops[node->v.UnaryOp.op](get_const_value(arg));
    return make_const(node, newval, arena);
}

/* Check whether a collection doesn't containing too much items (including
   subcollections).  This protects from creating a constant that needs
   too much time for calculating a hash.
   "limit" is the maximal number of items.
   Returns the negative number if the total number of items exceeds the
   limit.  Otherwise returns the limit minus the total number of items.
*/

static Py_ssize_t
check_complexity(PyObject *obj, Py_ssize_t limit)
{
    if (PyTuple_Check(obj)) {
        Py_ssize_t i;
        limit -= PyTuple_GET_SIZE(obj);
        for (i = 0; limit >= 0 && i < PyTuple_GET_SIZE(obj); i++) {
            limit = check_complexity(PyTuple_GET_ITEM(obj, i), limit);
        }
        return limit;
    }
    else if (PyFrozenSet_Check(obj)) {
        Py_ssize_t i = 0;
        PyObject *item;
        Py_hash_t hash;
        limit -= PySet_GET_SIZE(obj);
        while (limit >= 0 && _PySet_NextEntry(obj, &i, &item, &hash)) {
            limit = check_complexity(item, limit);
        }
    }
    return limit;
}

#define MAX_INT_SIZE           128  /* bits */
#define MAX_COLLECTION_SIZE    256  /* items */
#define MAX_STR_SIZE          4096  /* characters */
#define MAX_TOTAL_ITEMS       1024  /* including nested collections */

static PyObject *
safe_multiply(PyObject *v, PyObject *w)
{
    if (PyLong_Check(v) && PyLong_Check(w) && Py_SIZE(v) && Py_SIZE(w)) {
        size_t vbits = _PyLong_NumBits(v);
        size_t wbits = _PyLong_NumBits(w);
        if (vbits == (size_t)-1 || wbits == (size_t)-1) {
            return NULL;
        }
        if (vbits + wbits > MAX_INT_SIZE) {
            return NULL;
        }
    }
    else if (PyLong_Check(v) && (PyTuple_Check(w) || PyFrozenSet_Check(w))) {
        Py_ssize_t size = PyTuple_Check(w) ? PyTuple_GET_SIZE(w) :
                                             PySet_GET_SIZE(w);
        if (size) {
            long n = PyLong_AsLong(v);
            if (n < 0 || n > MAX_COLLECTION_SIZE / size) {
                return NULL;
            }
            if (n && check_complexity(w, MAX_TOTAL_ITEMS / n) < 0) {
                return NULL;
            }
        }
    }
    else if (PyLong_Check(v) && (PyUnicode_Check(w) || PyBytes_Check(w))) {
        Py_ssize_t size = PyUnicode_Check(w) ? PyUnicode_GET_LENGTH(w) :
                                               PyBytes_GET_SIZE(w);
        if (size) {
            long n = PyLong_AsLong(v);
            if (n < 0 || n > MAX_STR_SIZE / size) {
                return NULL;
            }
        }
    }
    else if (PyLong_Check(w) &&
             (PyTuple_Check(v) || PyFrozenSet_Check(v) ||
              PyUnicode_Check(v) || PyBytes_Check(v)))
    {
        return safe_multiply(w, v);
    }

    return PyNumber_Multiply(v, w);
}

static PyObject *
safe_power(PyObject *v, PyObject *w)
{
    if (PyLong_Check(v) && PyLong_Check(w) && Py_SIZE(v) && Py_SIZE(w) > 0) {
        size_t vbits = _PyLong_NumBits(v);
        size_t wbits = PyLong_AsSize_t(w);
        if (vbits == (size_t)-1 || wbits == (size_t)-1) {
            return NULL;
        }
        if (vbits > MAX_INT_SIZE / wbits) {
            return NULL;
        }
    }

    return PyNumber_Power(v, w, Py_None);
}

static PyObject *
safe_lshift(PyObject *v, PyObject *w)
{
    if (PyLong_Check(v) && PyLong_Check(w) && Py_SIZE(v) && Py_SIZE(w)) {
        size_t vbits = _PyLong_NumBits(v);
        size_t wbits = PyLong_AsSize_t(w);
        if (vbits == (size_t)-1 || wbits == (size_t)-1) {
            return NULL;
        }
        if (wbits > MAX_INT_SIZE || vbits > MAX_INT_SIZE - wbits) {
            return NULL;
        }
    }

    return PyNumber_Lshift(v, w);
}

static PyObject *
safe_mod(PyObject *v, PyObject *w)
{
    if (PyUnicode_Check(v) || PyBytes_Check(v)) {
        return NULL;
    }

    return PyNumber_Remainder(v, w);
}

static int
fold_binop(expr_ty node, PyArena *arena, int optimize)
{
    expr_ty lhs, rhs;
    lhs = node->v.BinOp.left;
    rhs = node->v.BinOp.right;
    if (!is_const(lhs) || !is_const(rhs)) {
        return 1;
    }

    PyObject *lv = get_const_value(lhs);
    PyObject *rv = get_const_value(rhs);
    PyObject *newval;

    switch (node->v.BinOp.op) {
    case Add:
        newval = PyNumber_Add(lv, rv);
        break;
    case Sub:
        newval = PyNumber_Subtract(lv, rv);
        break;
    case Mult:
        newval = safe_multiply(lv, rv);
        break;
    case Div:
        newval = PyNumber_TrueDivide(lv, rv);
        break;
    case FloorDiv:
        newval = PyNumber_FloorDivide(lv, rv);
        break;
    case Mod:
        newval = safe_mod(lv, rv);
        break;
    case Pow:
        newval = safe_power(lv, rv);
        break;
    case LShift:
        newval = safe_lshift(lv, rv);
        break;
    case RShift:
        newval = PyNumber_Rshift(lv, rv);
        break;
    case BitOr:
        newval = PyNumber_Or(lv, rv);
        break;
    case BitXor:
        newval = PyNumber_Xor(lv, rv);
        break;
    case BitAnd:
        newval = PyNumber_And(lv, rv);
        break;
    default: // Unknown operator
        return 1;
    }

    return make_const(node, newval, arena);
}

static PyObject*
make_const_tuple(asdl_seq *elts)
{
    for (int i = 0; i < asdl_seq_LEN(elts); i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(elts, i);
        if (!is_const(e)) {
            return NULL;
        }
    }

    PyObject *newval = PyTuple_New(asdl_seq_LEN(elts));
    if (newval == NULL) {
        return NULL;
    }

    for (int i = 0; i < asdl_seq_LEN(elts); i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(elts, i);
        PyObject *v = get_const_value(e);
        Py_INCREF(v);
        PyTuple_SET_ITEM(newval, i, v);
    }
    return newval;
}

static int
fold_tuple(expr_ty node, PyArena *arena, int optimize)
{
    PyObject *newval;

    if (node->v.Tuple.ctx != Load)
        return 1;

    newval = make_const_tuple(node->v.Tuple.elts);
    return make_const(node, newval, arena);
}

static int
fold_subscr(expr_ty node, PyArena *arena, int optimize)
{
    PyObject *newval;
    expr_ty arg, idx;
    slice_ty slice;

    arg = node->v.Subscript.value;
    slice = node->v.Subscript.slice;
    if (node->v.Subscript.ctx != Load ||
            !is_const(arg) ||
            /* TODO: handle other types of slices */
            slice->kind != Index_kind ||
            !is_const(slice->v.Index.value))
    {
        return 1;
    }

    idx = slice->v.Index.value;
    newval = PyObject_GetItem(get_const_value(arg), get_const_value(idx));
    return make_const(node, newval, arena);
}

/* Change literal list or set of constants into constant
   tuple or frozenset respectively.
   Used for right operand of "in" and "not in" tests and for iterable
   in "for" loop and comprehensions.
*/
static int
fold_iter(expr_ty arg, PyArena *arena, int optimize)
{
    PyObject *newval;
    if (arg->kind == List_kind) {
        newval = make_const_tuple(arg->v.List.elts);
    }
    else if (arg->kind == Set_kind) {
        newval = make_const_tuple(arg->v.Set.elts);
        if (newval) {
            Py_SETREF(newval, PyFrozenSet_New(newval));
        }
    }
    else {
        return 1;
    }
    return make_const(arg, newval, arena);
}

static int
fold_compare(expr_ty node, PyArena *arena, int optimize)
{
    asdl_int_seq *ops;
    asdl_seq *args;
    Py_ssize_t i;

    ops = node->v.Compare.ops;
    args = node->v.Compare.comparators;
    /* TODO: optimize cases with literal arguments. */
    /* Change literal list or set in 'in' or 'not in' into
       tuple or frozenset respectively. */
    i = asdl_seq_LEN(ops) - 1;
    int op = asdl_seq_GET(ops, i);
    if (op == In || op == NotIn) {
        if (!fold_iter((expr_ty)asdl_seq_GET(args, i), arena, optimize)) {
            return 0;
        }
    }
    return 1;
}

static int astfold_mod(mod_ty node_, PyArena *ctx_, int optimize_);
static int astfold_stmt(stmt_ty node_, PyArena *ctx_, int optimize_);
static int astfold_expr(expr_ty node_, PyArena *ctx_, int optimize_);
static int astfold_arguments(arguments_ty node_, PyArena *ctx_, int optimize_);
static int astfold_comprehension(comprehension_ty node_, PyArena *ctx_, int optimize_);
static int astfold_keyword(keyword_ty node_, PyArena *ctx_, int optimize_);
static int astfold_slice(slice_ty node_, PyArena *ctx_, int optimize_);
static int astfold_arg(arg_ty node_, PyArena *ctx_, int optimize_);
static int astfold_withitem(withitem_ty node_, PyArena *ctx_, int optimize_);
static int astfold_excepthandler(excepthandler_ty node_, PyArena *ctx_, int optimize_);
#define CALL(FUNC, TYPE, ARG) \
    if (!FUNC((ARG), ctx_, optimize_)) \
        return 0;

#define CALL_OPT(FUNC, TYPE, ARG) \
    if ((ARG) != NULL && !FUNC((ARG), ctx_, optimize_)) \
        return 0;

#define CALL_SEQ(FUNC, TYPE, ARG) { \
    int i; \
    asdl_seq *seq = (ARG); /* avoid variable capture */ \
    for (i = 0; i < asdl_seq_LEN(seq); i++) { \
        TYPE elt = (TYPE)asdl_seq_GET(seq, i); \
        if (elt != NULL && !FUNC(elt, ctx_, optimize_)) \
            return 0; \
    } \
}

#define CALL_INT_SEQ(FUNC, TYPE, ARG) { \
    int i; \
    asdl_int_seq *seq = (ARG); /* avoid variable capture */ \
    for (i = 0; i < asdl_seq_LEN(seq); i++) { \
        TYPE elt = (TYPE)asdl_seq_GET(seq, i); \
        if (!FUNC(elt, ctx_, optimize_)) \
            return 0; \
    } \
}

static int
astfold_mod(mod_ty node_, PyArena *ctx_, int optimize_)
{
    switch (node_->kind) {
    case Module_kind:
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.Module.body);
        break;
    case Interactive_kind:
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.Interactive.body);
        break;
    case Expression_kind:
        CALL(astfold_expr, expr_ty, node_->v.Expression.body);
        break;
    case Suite_kind:
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.Suite.body);
        break;
    default:
        break;
    }
    return 1;
}

static int
astfold_expr(expr_ty node_, PyArena *ctx_, int optimize_)
{
    switch (node_->kind) {
    case BoolOp_kind:
        CALL_SEQ(astfold_expr, expr_ty, node_->v.BoolOp.values);
        break;
    case BinOp_kind:
        CALL(astfold_expr, expr_ty, node_->v.BinOp.left);
        CALL(astfold_expr, expr_ty, node_->v.BinOp.right);
        CALL(fold_binop, expr_ty, node_);
        break;
    case UnaryOp_kind:
        CALL(astfold_expr, expr_ty, node_->v.UnaryOp.operand);
        CALL(fold_unaryop, expr_ty, node_);
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
        CALL_SEQ(astfold_expr, expr_ty, node_->v.Dict.keys);
        CALL_SEQ(astfold_expr, expr_ty, node_->v.Dict.values);
        break;
    case Set_kind:
        CALL_SEQ(astfold_expr, expr_ty, node_->v.Set.elts);
        break;
    case ListComp_kind:
        CALL(astfold_expr, expr_ty, node_->v.ListComp.elt);
        CALL_SEQ(astfold_comprehension, comprehension_ty, node_->v.ListComp.generators);
        break;
    case SetComp_kind:
        CALL(astfold_expr, expr_ty, node_->v.SetComp.elt);
        CALL_SEQ(astfold_comprehension, comprehension_ty, node_->v.SetComp.generators);
        break;
    case DictComp_kind:
        CALL(astfold_expr, expr_ty, node_->v.DictComp.key);
        CALL(astfold_expr, expr_ty, node_->v.DictComp.value);
        CALL_SEQ(astfold_comprehension, comprehension_ty, node_->v.DictComp.generators);
        break;
    case GeneratorExp_kind:
        CALL(astfold_expr, expr_ty, node_->v.GeneratorExp.elt);
        CALL_SEQ(astfold_comprehension, comprehension_ty, node_->v.GeneratorExp.generators);
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
        CALL_SEQ(astfold_expr, expr_ty, node_->v.Compare.comparators);
        CALL(fold_compare, expr_ty, node_);
        break;
    case Call_kind:
        CALL(astfold_expr, expr_ty, node_->v.Call.func);
        CALL_SEQ(astfold_expr, expr_ty, node_->v.Call.args);
        CALL_SEQ(astfold_keyword, keyword_ty, node_->v.Call.keywords);
        break;
    case FormattedValue_kind:
        CALL(astfold_expr, expr_ty, node_->v.FormattedValue.value);
        CALL_OPT(astfold_expr, expr_ty, node_->v.FormattedValue.format_spec);
        break;
    case JoinedStr_kind:
        CALL_SEQ(astfold_expr, expr_ty, node_->v.JoinedStr.values);
        break;
    case Attribute_kind:
        CALL(astfold_expr, expr_ty, node_->v.Attribute.value);
        break;
    case Subscript_kind:
        CALL(astfold_expr, expr_ty, node_->v.Subscript.value);
        CALL(astfold_slice, slice_ty, node_->v.Subscript.slice);
        CALL(fold_subscr, expr_ty, node_);
        break;
    case Starred_kind:
        CALL(astfold_expr, expr_ty, node_->v.Starred.value);
        break;
    case List_kind:
        CALL_SEQ(astfold_expr, expr_ty, node_->v.List.elts);
        break;
    case Tuple_kind:
        CALL_SEQ(astfold_expr, expr_ty, node_->v.Tuple.elts);
        CALL(fold_tuple, expr_ty, node_);
        break;
    case Name_kind:
        if (_PyUnicode_EqualToASCIIString(node_->v.Name.id, "__debug__")) {
            return make_const(node_, PyBool_FromLong(!optimize_), ctx_);
        }
        break;
    default:
        break;
    }
    return 1;
}

static int
astfold_slice(slice_ty node_, PyArena *ctx_, int optimize_)
{
    switch (node_->kind) {
    case Slice_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.Slice.lower);
        CALL_OPT(astfold_expr, expr_ty, node_->v.Slice.upper);
        CALL_OPT(astfold_expr, expr_ty, node_->v.Slice.step);
        break;
    case ExtSlice_kind:
        CALL_SEQ(astfold_slice, slice_ty, node_->v.ExtSlice.dims);
        break;
    case Index_kind:
        CALL(astfold_expr, expr_ty, node_->v.Index.value);
        break;
    default:
        break;
    }
    return 1;
}

static int
astfold_keyword(keyword_ty node_, PyArena *ctx_, int optimize_)
{
    CALL(astfold_expr, expr_ty, node_->value);
    return 1;
}

static int
astfold_comprehension(comprehension_ty node_, PyArena *ctx_, int optimize_)
{
    CALL(astfold_expr, expr_ty, node_->target);
    CALL(astfold_expr, expr_ty, node_->iter);
    CALL_SEQ(astfold_expr, expr_ty, node_->ifs);

    CALL(fold_iter, expr_ty, node_->iter);
    return 1;
}

static int
astfold_arguments(arguments_ty node_, PyArena *ctx_, int optimize_)
{
    CALL_SEQ(astfold_arg, arg_ty, node_->args);
    CALL_OPT(astfold_arg, arg_ty, node_->vararg);
    CALL_SEQ(astfold_arg, arg_ty, node_->kwonlyargs);
    CALL_SEQ(astfold_expr, expr_ty, node_->kw_defaults);
    CALL_OPT(astfold_arg, arg_ty, node_->kwarg);
    CALL_SEQ(astfold_expr, expr_ty, node_->defaults);
    return 1;
}

static int
astfold_arg(arg_ty node_, PyArena *ctx_, int optimize_)
{
    CALL_OPT(astfold_expr, expr_ty, node_->annotation);
    return 1;
}

static int
astfold_stmt(stmt_ty node_, PyArena *ctx_, int optimize_)
{
    switch (node_->kind) {
    case FunctionDef_kind:
        CALL(astfold_arguments, arguments_ty, node_->v.FunctionDef.args);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.FunctionDef.body);
        CALL_SEQ(astfold_expr, expr_ty, node_->v.FunctionDef.decorator_list);
        CALL_OPT(astfold_expr, expr_ty, node_->v.FunctionDef.returns);
        break;
    case AsyncFunctionDef_kind:
        CALL(astfold_arguments, arguments_ty, node_->v.AsyncFunctionDef.args);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.AsyncFunctionDef.body);
        CALL_SEQ(astfold_expr, expr_ty, node_->v.AsyncFunctionDef.decorator_list);
        CALL_OPT(astfold_expr, expr_ty, node_->v.AsyncFunctionDef.returns);
        break;
    case ClassDef_kind:
        CALL_SEQ(astfold_expr, expr_ty, node_->v.ClassDef.bases);
        CALL_SEQ(astfold_keyword, keyword_ty, node_->v.ClassDef.keywords);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.ClassDef.body);
        CALL_SEQ(astfold_expr, expr_ty, node_->v.ClassDef.decorator_list);
        break;
    case Return_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.Return.value);
        break;
    case Delete_kind:
        CALL_SEQ(astfold_expr, expr_ty, node_->v.Delete.targets);
        break;
    case Assign_kind:
        CALL_SEQ(astfold_expr, expr_ty, node_->v.Assign.targets);
        CALL(astfold_expr, expr_ty, node_->v.Assign.value);
        break;
    case AugAssign_kind:
        CALL(astfold_expr, expr_ty, node_->v.AugAssign.target);
        CALL(astfold_expr, expr_ty, node_->v.AugAssign.value);
        break;
    case AnnAssign_kind:
        CALL(astfold_expr, expr_ty, node_->v.AnnAssign.target);
        CALL(astfold_expr, expr_ty, node_->v.AnnAssign.annotation);
        CALL_OPT(astfold_expr, expr_ty, node_->v.AnnAssign.value);
        break;
    case For_kind:
        CALL(astfold_expr, expr_ty, node_->v.For.target);
        CALL(astfold_expr, expr_ty, node_->v.For.iter);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.For.body);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.For.orelse);

        CALL(fold_iter, expr_ty, node_->v.For.iter);
        break;
    case AsyncFor_kind:
        CALL(astfold_expr, expr_ty, node_->v.AsyncFor.target);
        CALL(astfold_expr, expr_ty, node_->v.AsyncFor.iter);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.AsyncFor.body);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.AsyncFor.orelse);
        break;
    case While_kind:
        CALL(astfold_expr, expr_ty, node_->v.While.test);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.While.body);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.While.orelse);
        break;
    case If_kind:
        CALL(astfold_expr, expr_ty, node_->v.If.test);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.If.body);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.If.orelse);
        break;
    case With_kind:
        CALL_SEQ(astfold_withitem, withitem_ty, node_->v.With.items);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.With.body);
        break;
    case AsyncWith_kind:
        CALL_SEQ(astfold_withitem, withitem_ty, node_->v.AsyncWith.items);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.AsyncWith.body);
        break;
    case Raise_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.Raise.exc);
        CALL_OPT(astfold_expr, expr_ty, node_->v.Raise.cause);
        break;
    case Try_kind:
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.Try.body);
        CALL_SEQ(astfold_excepthandler, excepthandler_ty, node_->v.Try.handlers);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.Try.orelse);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.Try.finalbody);
        break;
    case Assert_kind:
        CALL(astfold_expr, expr_ty, node_->v.Assert.test);
        CALL_OPT(astfold_expr, expr_ty, node_->v.Assert.msg);
        break;
    case Expr_kind:
        CALL(astfold_expr, expr_ty, node_->v.Expr.value);
        break;
    default:
        break;
    }
    return 1;
}

static int
astfold_excepthandler(excepthandler_ty node_, PyArena *ctx_, int optimize_)
{
    switch (node_->kind) {
    case ExceptHandler_kind:
        CALL_OPT(astfold_expr, expr_ty, node_->v.ExceptHandler.type);
        CALL_SEQ(astfold_stmt, stmt_ty, node_->v.ExceptHandler.body);
        break;
    default:
        break;
    }
    return 1;
}

static int
astfold_withitem(withitem_ty node_, PyArena *ctx_, int optimize_)
{
    CALL(astfold_expr, expr_ty, node_->context_expr);
    CALL_OPT(astfold_expr, expr_ty, node_->optional_vars);
    return 1;
}

#undef CALL
#undef CALL_OPT
#undef CALL_SEQ
#undef CALL_INT_SEQ

int
_PyAST_Optimize(mod_ty mod, PyArena *arena, int optimize)
{
    int ret = astfold_mod(mod, arena, optimize);
    assert(ret || PyErr_Occurred());
    return ret;
}
