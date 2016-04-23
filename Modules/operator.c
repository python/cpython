
#include "Python.h"

PyDoc_STRVAR(operator_doc,
"Operator interface.\n\
\n\
This module exports a set of functions implemented in C corresponding\n\
to the intrinsic operators of Python.  For example, operator.add(x, y)\n\
is equivalent to the expression x+y.  The function names are those\n\
used for special methods; variants without leading and trailing\n\
'__' are also provided for convenience.");

#define spam1(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a1) { \
  return AOP(a1); }

#define spam2(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  return AOP(a1,a2); }

#define spamoi(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1; int a2; \
  if(! PyArg_ParseTuple(a,"Oi:" #OP,&a1,&a2)) return NULL; \
  return AOP(a1,a2); }

#define spam2n(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  if(-1 == AOP(a1,a2)) return NULL; \
  Py_INCREF(Py_None); \
  return Py_None; }

#define spam3n(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2, *a3; \
  if(! PyArg_UnpackTuple(a,#OP,3,3,&a1,&a2,&a3)) return NULL; \
  if(-1 == AOP(a1,a2,a3)) return NULL; \
  Py_INCREF(Py_None); \
  return Py_None; }

#define spami(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a1) { \
  long r; \
  if(-1 == (r=AOP(a1))) return NULL; \
  return PyBool_FromLong(r); }

#define spami2(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; long r; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  if(-1 == (r=AOP(a1,a2))) return NULL; \
  return PyInt_FromLong(r); }

#define spamn2(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; Py_ssize_t r; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  if(-1 == (r=AOP(a1,a2))) return NULL; \
  return PyInt_FromSsize_t(r); }

#define spami2b(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; long r; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  if(-1 == (r=AOP(a1,a2))) return NULL; \
  return PyBool_FromLong(r); }

#define spamrc(OP,A) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  return PyObject_RichCompare(a1,a2,A); }

/* Deprecated operators that need warnings. */
static int
op_isCallable(PyObject *x)
{
    if (PyErr_WarnPy3k("operator.isCallable() is not supported in 3.x. "
                       "Use hasattr(obj, '__call__').", 1) < 0)
        return -1;
    return PyCallable_Check(x);
}

static int
op_sequenceIncludes(PyObject *seq, PyObject* ob)
{
    if (PyErr_WarnPy3k("operator.sequenceIncludes() is not supported "
                       "in 3.x. Use operator.contains().", 1) < 0)
        return -1;
    return PySequence_Contains(seq, ob);
}

spami(isCallable       , op_isCallable)
spami(isNumberType     , PyNumber_Check)
spami(truth            , PyObject_IsTrue)
spam2(op_add           , PyNumber_Add)
spam2(op_sub           , PyNumber_Subtract)
spam2(op_mul           , PyNumber_Multiply)
spam2(op_div           , PyNumber_Divide)
spam2(op_floordiv      , PyNumber_FloorDivide)
spam2(op_truediv       , PyNumber_TrueDivide)
spam2(op_mod           , PyNumber_Remainder)
spam1(op_neg           , PyNumber_Negative)
spam1(op_pos           , PyNumber_Positive)
spam1(op_abs           , PyNumber_Absolute)
spam1(op_inv           , PyNumber_Invert)
spam1(op_invert        , PyNumber_Invert)
spam2(op_lshift        , PyNumber_Lshift)
spam2(op_rshift        , PyNumber_Rshift)
spami(op_not_          , PyObject_Not)
spam2(op_and_          , PyNumber_And)
spam2(op_xor           , PyNumber_Xor)
spam2(op_or_           , PyNumber_Or)
spam2(op_iadd          , PyNumber_InPlaceAdd)
spam2(op_isub          , PyNumber_InPlaceSubtract)
spam2(op_imul          , PyNumber_InPlaceMultiply)
spam2(op_idiv          , PyNumber_InPlaceDivide)
spam2(op_ifloordiv     , PyNumber_InPlaceFloorDivide)
spam2(op_itruediv      , PyNumber_InPlaceTrueDivide)
spam2(op_imod          , PyNumber_InPlaceRemainder)
spam2(op_ilshift       , PyNumber_InPlaceLshift)
spam2(op_irshift       , PyNumber_InPlaceRshift)
spam2(op_iand          , PyNumber_InPlaceAnd)
spam2(op_ixor          , PyNumber_InPlaceXor)
spam2(op_ior           , PyNumber_InPlaceOr)
spami(isSequenceType   , PySequence_Check)
spam2(op_concat        , PySequence_Concat)
spamoi(op_repeat       , PySequence_Repeat)
spam2(op_iconcat       , PySequence_InPlaceConcat)
spamoi(op_irepeat      , PySequence_InPlaceRepeat)
spami2b(op_contains     , PySequence_Contains)
spami2b(sequenceIncludes, op_sequenceIncludes)
spamn2(indexOf         , PySequence_Index)
spamn2(countOf         , PySequence_Count)
spami(isMappingType    , PyMapping_Check)
spam2(op_getitem       , PyObject_GetItem)
spam2n(op_delitem       , PyObject_DelItem)
spam3n(op_setitem      , PyObject_SetItem)
spamrc(op_lt           , Py_LT)
spamrc(op_le           , Py_LE)
spamrc(op_eq           , Py_EQ)
spamrc(op_ne           , Py_NE)
spamrc(op_gt           , Py_GT)
spamrc(op_ge           , Py_GE)

static PyObject*
op_pow(PyObject *s, PyObject *a)
{
    PyObject *a1, *a2;
    if (PyArg_UnpackTuple(a,"pow", 2, 2, &a1, &a2))
        return PyNumber_Power(a1, a2, Py_None);
    return NULL;
}

static PyObject*
op_ipow(PyObject *s, PyObject *a)
{
    PyObject *a1, *a2;
    if (PyArg_UnpackTuple(a,"ipow", 2, 2, &a1, &a2))
        return PyNumber_InPlacePower(a1, a2, Py_None);
    return NULL;
}

static PyObject *
op_index(PyObject *s, PyObject *a)
{
    return PyNumber_Index(a);
}

static PyObject*
is_(PyObject *s, PyObject *a)
{
    PyObject *a1, *a2, *result = NULL;
    if (PyArg_UnpackTuple(a,"is_", 2, 2, &a1, &a2)) {
        result = (a1 == a2) ? Py_True : Py_False;
        Py_INCREF(result);
    }
    return result;
}

static PyObject*
is_not(PyObject *s, PyObject *a)
{
    PyObject *a1, *a2, *result = NULL;
    if (PyArg_UnpackTuple(a,"is_not", 2, 2, &a1, &a2)) {
        result = (a1 != a2) ? Py_True : Py_False;
        Py_INCREF(result);
    }
    return result;
}

static PyObject*
op_getslice(PyObject *s, PyObject *a)
{
    PyObject *a1;
    Py_ssize_t a2, a3;

    if (!PyArg_ParseTuple(a, "Onn:getslice", &a1, &a2, &a3))
        return NULL;
    return PySequence_GetSlice(a1, a2, a3);
}

static PyObject*
op_setslice(PyObject *s, PyObject *a)
{
    PyObject *a1, *a4;
    Py_ssize_t a2, a3;

    if (!PyArg_ParseTuple(a, "OnnO:setslice", &a1, &a2, &a3, &a4))
        return NULL;

    if (-1 == PySequence_SetSlice(a1, a2, a3, a4))
        return NULL;

    Py_RETURN_NONE;
}

static PyObject*
op_delslice(PyObject *s, PyObject *a)
{
    PyObject *a1;
    Py_ssize_t a2, a3;

    if (!PyArg_ParseTuple(a, "Onn:delslice", &a1, &a2, &a3))
        return NULL;

    if (-1 == PySequence_DelSlice(a1, a2, a3))
        return NULL;

    Py_RETURN_NONE;
}

#undef spam1
#undef spam2
#undef spam1o
#undef spam1o
#define spam1(OP,DOC) {#OP, OP, METH_VARARGS, PyDoc_STR(DOC)},
#define spam2(OP,ALTOP,DOC) {#OP, op_##OP, METH_VARARGS, PyDoc_STR(DOC)}, \
                           {#ALTOP, op_##OP, METH_VARARGS, PyDoc_STR(DOC)},
#define spam1o(OP,DOC) {#OP, OP, METH_O, PyDoc_STR(DOC)},
#define spam2o(OP,ALTOP,DOC) {#OP, op_##OP, METH_O, PyDoc_STR(DOC)}, \
                           {#ALTOP, op_##OP, METH_O, PyDoc_STR(DOC)},



/* compare_digest **********************************************************/

/*
 * timing safe compare
 *
 * Returns 1 of the strings are equal.
 * In case of len(a) != len(b) the function tries to keep the timing
 * dependent on the length of b. CPU cache locally may still alter timing
 * a bit.
 */
static int
_tscmp(const unsigned char *a, const unsigned char *b,
        Py_ssize_t len_a, Py_ssize_t len_b)
{
    /* The volatile type declarations make sure that the compiler has no
     * chance to optimize and fold the code in any way that may change
     * the timing.
     */
    volatile Py_ssize_t length;
    volatile const unsigned char *left;
    volatile const unsigned char *right;
    Py_ssize_t i;
    unsigned char result;

    /* loop count depends on length of b */
    length = len_b;
    left = NULL;
    right = b;

    /* don't use else here to keep the amount of CPU instructions constant,
     * volatile forces re-evaluation
     *  */
    if (len_a == length) {
        left = *((volatile const unsigned char**)&a);
        result = 0;
    }
    if (len_a != length) {
        left = b;
        result = 1;
    }

    for (i=0; i < length; i++) {
        result |= *left++ ^ *right++;
    }

    return (result == 0);
}

PyDoc_STRVAR(compare_digest__doc__,
"compare_digest(a, b) -> bool\n"
"\n"
"Return 'a == b'.  This function uses an approach designed to prevent\n"
"timing analysis, making it appropriate for cryptography.\n"
"a and b must both be of the same type: either str (ASCII only),\n"
"or any type that supports the buffer protocol (e.g. bytes).\n"
"\n"
"Note: If a and b are of different lengths, or if an error occurs,\n"
"a timing attack could theoretically reveal information about the\n"
"types and lengths of a and b--but not their values.\n");

static PyObject*
compare_digest(PyObject *self, PyObject *args)
{
    PyObject *a, *b;
    int rc;

    if (!PyArg_ParseTuple(args, "OO:compare_digest", &a, &b)) {
        return NULL;
    }

    /* Unicode string */
    if (PyUnicode_Check(a) && PyUnicode_Check(b)) {
        rc = _tscmp((const unsigned char *)PyUnicode_AS_DATA(a),
                    (const unsigned char *)PyUnicode_AS_DATA(b),
                    PyUnicode_GET_DATA_SIZE(a),
                    PyUnicode_GET_DATA_SIZE(b));
    }
    /* fallback to buffer interface for bytes, bytesarray and other */
    else {
        Py_buffer view_a;
        Py_buffer view_b;

        if (PyObject_CheckBuffer(a) == 0 && PyObject_CheckBuffer(b) == 0) {
            PyErr_Format(PyExc_TypeError,
                         "unsupported operand types(s) or combination of types: "
                         "'%.100s' and '%.100s'",
                         Py_TYPE(a)->tp_name, Py_TYPE(b)->tp_name);
            return NULL;
        }

        if (PyObject_GetBuffer(a, &view_a, PyBUF_SIMPLE) == -1) {
            return NULL;
        }
        if (view_a.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            return NULL;
        }

        if (PyObject_GetBuffer(b, &view_b, PyBUF_SIMPLE) == -1) {
            PyBuffer_Release(&view_a);
            return NULL;
        }
        if (view_b.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            PyBuffer_Release(&view_b);
            return NULL;
        }

        rc = _tscmp((const unsigned char*)view_a.buf,
                    (const unsigned char*)view_b.buf,
                    view_a.len,
                    view_b.len);

        PyBuffer_Release(&view_a);
        PyBuffer_Release(&view_b);
    }

    return PyBool_FromLong(rc);
}

static struct PyMethodDef operator_methods[] = {

spam1o(isCallable,
 "isCallable(a) -- Same as callable(a).")
spam1o(isNumberType,
 "isNumberType(a) -- Return True if a has a numeric type, False otherwise.")
spam1o(isSequenceType,
 "isSequenceType(a) -- Return True if a has a sequence type, False otherwise.")
spam1o(truth,
 "truth(a) -- Return True if a is true, False otherwise.")
spam2(contains,__contains__,
 "contains(a, b) -- Same as b in a (note reversed operands).")
spam1(sequenceIncludes,
 "sequenceIncludes(a, b) -- Same as b in a (note reversed operands; deprecated).")
spam1(indexOf,
 "indexOf(a, b) -- Return the first index of b in a.")
spam1(countOf,
 "countOf(a, b) -- Return the number of times b occurs in a.")
spam1o(isMappingType,
 "isMappingType(a) -- Return True if a has a mapping type, False otherwise.")

spam1(is_, "is_(a, b) -- Same as a is b.")
spam1(is_not, "is_not(a, b) -- Same as a is not b.")
spam2o(index, __index__, "index(a) -- Same as a.__index__()")
spam2(add,__add__, "add(a, b) -- Same as a + b.")
spam2(sub,__sub__, "sub(a, b) -- Same as a - b.")
spam2(mul,__mul__, "mul(a, b) -- Same as a * b.")
spam2(div,__div__, "div(a, b) -- Same as a / b when __future__.division is not in effect.")
spam2(floordiv,__floordiv__, "floordiv(a, b) -- Same as a // b.")
spam2(truediv,__truediv__, "truediv(a, b) -- Same as a / b when __future__.division is in effect.")
spam2(mod,__mod__, "mod(a, b) -- Same as a % b.")
spam2o(neg,__neg__, "neg(a) -- Same as -a.")
spam2o(pos,__pos__, "pos(a) -- Same as +a.")
spam2o(abs,__abs__, "abs(a) -- Same as abs(a).")
spam2o(inv,__inv__, "inv(a) -- Same as ~a.")
spam2o(invert,__invert__, "invert(a) -- Same as ~a.")
spam2(lshift,__lshift__, "lshift(a, b) -- Same as a << b.")
spam2(rshift,__rshift__, "rshift(a, b) -- Same as a >> b.")
spam2o(not_,__not__, "not_(a) -- Same as not a.")
spam2(and_,__and__, "and_(a, b) -- Same as a & b.")
spam2(xor,__xor__, "xor(a, b) -- Same as a ^ b.")
spam2(or_,__or__, "or_(a, b) -- Same as a | b.")
spam2(iadd,__iadd__, "a = iadd(a, b) -- Same as a += b.")
spam2(isub,__isub__, "a = isub(a, b) -- Same as a -= b.")
spam2(imul,__imul__, "a = imul(a, b) -- Same as a *= b.")
spam2(idiv,__idiv__, "a = idiv(a, b) -- Same as a /= b when __future__.division is not in effect.")
spam2(ifloordiv,__ifloordiv__, "a = ifloordiv(a, b) -- Same as a //= b.")
spam2(itruediv,__itruediv__, "a = itruediv(a, b) -- Same as a /= b when __future__.division is in effect.")
spam2(imod,__imod__, "a = imod(a, b) -- Same as a %= b.")
spam2(ilshift,__ilshift__, "a = ilshift(a, b) -- Same as a <<= b.")
spam2(irshift,__irshift__, "a = irshift(a, b) -- Same as a >>= b.")
spam2(iand,__iand__, "a = iand(a, b) -- Same as a &= b.")
spam2(ixor,__ixor__, "a = ixor(a, b) -- Same as a ^= b.")
spam2(ior,__ior__, "a = ior(a, b) -- Same as a |= b.")
spam2(concat,__concat__,
 "concat(a, b) -- Same as a + b, for a and b sequences.")
spam2(repeat,__repeat__,
 "repeat(a, b) -- Return a * b, where a is a sequence, and b is an integer.")
spam2(iconcat,__iconcat__,
 "a = iconcat(a, b) -- Same as a += b, for a and b sequences.")
spam2(irepeat,__irepeat__,
 "a = irepeat(a, b) -- Same as a *= b, where a is a sequence, and b is an integer.")
spam2(getitem,__getitem__,
 "getitem(a, b) -- Same as a[b].")
spam2(setitem,__setitem__,
 "setitem(a, b, c) -- Same as a[b] = c.")
spam2(delitem,__delitem__,
 "delitem(a, b) -- Same as del a[b].")
spam2(pow,__pow__, "pow(a, b) -- Same as a ** b.")
spam2(ipow,__ipow__, "a = ipow(a, b) -- Same as a **= b.")
spam2(getslice,__getslice__,
 "getslice(a, b, c) -- Same as a[b:c].")
spam2(setslice,__setslice__,
"setslice(a, b, c, d) -- Same as a[b:c] = d.")
spam2(delslice,__delslice__,
"delslice(a, b, c) -- Same as del a[b:c].")
spam2(lt,__lt__, "lt(a, b) -- Same as a<b.")
spam2(le,__le__, "le(a, b) -- Same as a<=b.")
spam2(eq,__eq__, "eq(a, b) -- Same as a==b.")
spam2(ne,__ne__, "ne(a, b) -- Same as a!=b.")
spam2(gt,__gt__, "gt(a, b) -- Same as a>b.")
spam2(ge,__ge__, "ge(a, b) -- Same as a>=b.")

    {"_compare_digest", (PyCFunction)compare_digest, METH_VARARGS,
     compare_digest__doc__},
    {NULL,              NULL}           /* sentinel */

};

/* itemgetter object **********************************************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t nitems;
    PyObject *item;
} itemgetterobject;

static PyTypeObject itemgetter_type;

static PyObject *
itemgetter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    itemgetterobject *ig;
    PyObject *item;
    Py_ssize_t nitems;

    if (!_PyArg_NoKeywords("itemgetter()", kwds))
        return NULL;

    nitems = PyTuple_GET_SIZE(args);
    if (nitems <= 1) {
        if (!PyArg_UnpackTuple(args, "itemgetter", 1, 1, &item))
            return NULL;
    } else
        item = args;

    /* create itemgetterobject structure */
    ig = PyObject_GC_New(itemgetterobject, &itemgetter_type);
    if (ig == NULL)
        return NULL;

    Py_INCREF(item);
    ig->item = item;
    ig->nitems = nitems;

    PyObject_GC_Track(ig);
    return (PyObject *)ig;
}

static void
itemgetter_dealloc(itemgetterobject *ig)
{
    PyObject_GC_UnTrack(ig);
    Py_XDECREF(ig->item);
    PyObject_GC_Del(ig);
}

static int
itemgetter_traverse(itemgetterobject *ig, visitproc visit, void *arg)
{
    Py_VISIT(ig->item);
    return 0;
}

static PyObject *
itemgetter_call(itemgetterobject *ig, PyObject *args, PyObject *kw)
{
    PyObject *obj, *result;
    Py_ssize_t i, nitems=ig->nitems;

    if (!_PyArg_NoKeywords("itemgetter", kw))
        return NULL;
    if (!PyArg_UnpackTuple(args, "itemgetter", 1, 1, &obj))
        return NULL;
    if (nitems == 1)
        return PyObject_GetItem(obj, ig->item);

    assert(PyTuple_Check(ig->item));
    assert(PyTuple_GET_SIZE(ig->item) == nitems);

    result = PyTuple_New(nitems);
    if (result == NULL)
        return NULL;

    for (i=0 ; i < nitems ; i++) {
        PyObject *item, *val;
        item = PyTuple_GET_ITEM(ig->item, i);
        val = PyObject_GetItem(obj, item);
        if (val == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, i, val);
    }
    return result;
}

PyDoc_STRVAR(itemgetter_doc,
"itemgetter(item, ...) --> itemgetter object\n\
\n\
Return a callable object that fetches the given item(s) from its operand.\n\
After f = itemgetter(2), the call f(r) returns r[2].\n\
After g = itemgetter(2, 5, 3), the call g(r) returns (r[2], r[5], r[3])");

static PyTypeObject itemgetter_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "operator.itemgetter",              /* tp_name */
    sizeof(itemgetterobject),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)itemgetter_dealloc,     /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)itemgetter_call,       /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,            /* tp_flags */
    itemgetter_doc,                     /* tp_doc */
    (traverseproc)itemgetter_traverse,          /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    itemgetter_new,                     /* tp_new */
    0,                                  /* tp_free */
};


/* attrgetter object **********************************************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t nattrs;
    PyObject *attr;
} attrgetterobject;

static PyTypeObject attrgetter_type;

static PyObject *
attrgetter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    attrgetterobject *ag;
    PyObject *attr;
    Py_ssize_t nattrs;

    if (!_PyArg_NoKeywords("attrgetter()", kwds))
        return NULL;

    nattrs = PyTuple_GET_SIZE(args);
    if (nattrs <= 1) {
        if (!PyArg_UnpackTuple(args, "attrgetter", 1, 1, &attr))
            return NULL;
    } else
        attr = args;

    /* create attrgetterobject structure */
    ag = PyObject_GC_New(attrgetterobject, &attrgetter_type);
    if (ag == NULL)
        return NULL;

    Py_INCREF(attr);
    ag->attr = attr;
    ag->nattrs = nattrs;

    PyObject_GC_Track(ag);
    return (PyObject *)ag;
}

static void
attrgetter_dealloc(attrgetterobject *ag)
{
    PyObject_GC_UnTrack(ag);
    Py_XDECREF(ag->attr);
    PyObject_GC_Del(ag);
}

static int
attrgetter_traverse(attrgetterobject *ag, visitproc visit, void *arg)
{
    Py_VISIT(ag->attr);
    return 0;
}

static PyObject *
dotted_getattr(PyObject *obj, PyObject *attr)
{
    char *s, *p;

#ifdef Py_USING_UNICODE
    if (PyUnicode_Check(attr)) {
        attr = _PyUnicode_AsDefaultEncodedString(attr, NULL);
        if (attr == NULL)
            return NULL;
    }
#endif

    if (!PyString_Check(attr)) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute name must be a string");
        return NULL;
    }

    s = PyString_AS_STRING(attr);
    Py_INCREF(obj);
    for (;;) {
        PyObject *newobj, *str;
        p = strchr(s, '.');
        str = p ? PyString_FromStringAndSize(s, (p-s)) :
              PyString_FromString(s);
        if (str == NULL) {
            Py_DECREF(obj);
            return NULL;
        }
        newobj = PyObject_GetAttr(obj, str);
        Py_DECREF(str);
        Py_DECREF(obj);
        if (newobj == NULL)
            return NULL;
        obj = newobj;
        if (p == NULL) break;
        s = p+1;
    }

    return obj;
}

static PyObject *
attrgetter_call(attrgetterobject *ag, PyObject *args, PyObject *kw)
{
    PyObject *obj, *result;
    Py_ssize_t i, nattrs=ag->nattrs;

    if (!_PyArg_NoKeywords("attrgetter", kw))
        return NULL;
    if (!PyArg_UnpackTuple(args, "attrgetter", 1, 1, &obj))
        return NULL;
    if (ag->nattrs == 1)
        return dotted_getattr(obj, ag->attr);

    assert(PyTuple_Check(ag->attr));
    assert(PyTuple_GET_SIZE(ag->attr) == nattrs);

    result = PyTuple_New(nattrs);
    if (result == NULL)
        return NULL;

    for (i=0 ; i < nattrs ; i++) {
        PyObject *attr, *val;
        attr = PyTuple_GET_ITEM(ag->attr, i);
        val = dotted_getattr(obj, attr);
        if (val == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, i, val);
    }
    return result;
}

PyDoc_STRVAR(attrgetter_doc,
"attrgetter(attr, ...) --> attrgetter object\n\
\n\
Return a callable object that fetches the given attribute(s) from its operand.\n\
After f = attrgetter('name'), the call f(r) returns r.name.\n\
After g = attrgetter('name', 'date'), the call g(r) returns (r.name, r.date).\n\
After h = attrgetter('name.first', 'name.last'), the call h(r) returns\n\
(r.name.first, r.name.last).");

static PyTypeObject attrgetter_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "operator.attrgetter",              /* tp_name */
    sizeof(attrgetterobject),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)attrgetter_dealloc,     /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)attrgetter_call,       /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,            /* tp_flags */
    attrgetter_doc,                     /* tp_doc */
    (traverseproc)attrgetter_traverse,          /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    attrgetter_new,                     /* tp_new */
    0,                                  /* tp_free */
};


/* methodcaller object **********************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *args;
    PyObject *kwds;
} methodcallerobject;

static PyTypeObject methodcaller_type;

static PyObject *
methodcaller_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    methodcallerobject *mc;
    PyObject *name, *newargs;

    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "methodcaller needs at least "
                        "one argument, the method name");
        return NULL;
    }

    /* create methodcallerobject structure */
    mc = PyObject_GC_New(methodcallerobject, &methodcaller_type);
    if (mc == NULL)
        return NULL;

    newargs = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    if (newargs == NULL) {
        Py_DECREF(mc);
        return NULL;
    }
    mc->args = newargs;

    name = PyTuple_GET_ITEM(args, 0);
    Py_INCREF(name);
    mc->name = name;

    Py_XINCREF(kwds);
    mc->kwds = kwds;

    PyObject_GC_Track(mc);
    return (PyObject *)mc;
}

static void
methodcaller_dealloc(methodcallerobject *mc)
{
    PyObject_GC_UnTrack(mc);
    Py_XDECREF(mc->name);
    Py_XDECREF(mc->args);
    Py_XDECREF(mc->kwds);
    PyObject_GC_Del(mc);
}

static int
methodcaller_traverse(methodcallerobject *mc, visitproc visit, void *arg)
{
    Py_VISIT(mc->args);
    Py_VISIT(mc->kwds);
    return 0;
}

static PyObject *
methodcaller_call(methodcallerobject *mc, PyObject *args, PyObject *kw)
{
    PyObject *method, *obj, *result;

    if (!_PyArg_NoKeywords("methodcaller", kw))
        return NULL;
    if (!PyArg_UnpackTuple(args, "methodcaller", 1, 1, &obj))
        return NULL;
    method = PyObject_GetAttr(obj, mc->name);
    if (method == NULL)
        return NULL;
    result = PyObject_Call(method, mc->args, mc->kwds);
    Py_DECREF(method);
    return result;
}

PyDoc_STRVAR(methodcaller_doc,
"methodcaller(name, ...) --> methodcaller object\n\
\n\
Return a callable object that calls the given method on its operand.\n\
After f = methodcaller('name'), the call f(r) returns r.name().\n\
After g = methodcaller('name', 'date', foo=1), the call g(r) returns\n\
r.name('date', foo=1).");

static PyTypeObject methodcaller_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "operator.methodcaller",            /* tp_name */
    sizeof(methodcallerobject),         /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)methodcaller_dealloc, /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)methodcaller_call,     /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    methodcaller_doc,                           /* tp_doc */
    (traverseproc)methodcaller_traverse,        /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    methodcaller_new,                   /* tp_new */
    0,                                  /* tp_free */
};


/* Initialization function for the module (*must* be called initoperator) */

PyMODINIT_FUNC
initoperator(void)
{
    PyObject *m;

    /* Create the module and add the functions */
    m = Py_InitModule4("operator", operator_methods, operator_doc,
                   (PyObject*)NULL, PYTHON_API_VERSION);
    if (m == NULL)
        return;

    if (PyType_Ready(&itemgetter_type) < 0)
        return;
    Py_INCREF(&itemgetter_type);
    PyModule_AddObject(m, "itemgetter", (PyObject *)&itemgetter_type);

    if (PyType_Ready(&attrgetter_type) < 0)
        return;
    Py_INCREF(&attrgetter_type);
    PyModule_AddObject(m, "attrgetter", (PyObject *)&attrgetter_type);

    if (PyType_Ready(&methodcaller_type) < 0)
        return;
    Py_INCREF(&methodcaller_type);
    PyModule_AddObject(m, "methodcaller", (PyObject *)&methodcaller_type);
}
