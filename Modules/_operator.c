
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
  return PyLong_FromLong(r); }

#define spamn2(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; Py_ssize_t r; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  if(-1 == (r=AOP(a1,a2))) return NULL; \
  return PyLong_FromSsize_t(r); }

#define spami2b(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; long r; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  if(-1 == (r=AOP(a1,a2))) return NULL; \
  return PyBool_FromLong(r); }

#define spamrc(OP,A) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; \
  if(! PyArg_UnpackTuple(a,#OP,2,2,&a1,&a2)) return NULL; \
  return PyObject_RichCompare(a1,a2,A); }

spami(truth            , PyObject_IsTrue)
spam2(op_add           , PyNumber_Add)
spam2(op_sub           , PyNumber_Subtract)
spam2(op_mul           , PyNumber_Multiply)
spam2(op_matmul        , PyNumber_MatrixMultiply)
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
spam2(op_imatmul       , PyNumber_InPlaceMatrixMultiply)
spam2(op_ifloordiv     , PyNumber_InPlaceFloorDivide)
spam2(op_itruediv      , PyNumber_InPlaceTrueDivide)
spam2(op_imod          , PyNumber_InPlaceRemainder)
spam2(op_ilshift       , PyNumber_InPlaceLshift)
spam2(op_irshift       , PyNumber_InPlaceRshift)
spam2(op_iand          , PyNumber_InPlaceAnd)
spam2(op_ixor          , PyNumber_InPlaceXor)
spam2(op_ior           , PyNumber_InPlaceOr)
spam2(op_concat        , PySequence_Concat)
spam2(op_iconcat       , PySequence_InPlaceConcat)
spami2b(op_contains     , PySequence_Contains)
spamn2(indexOf         , PySequence_Index)
spamn2(countOf         , PySequence_Count)
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

#undef spam1
#undef spam2
#undef spam1o
#undef spam1o

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

PyDoc_STRVAR(length_hint__doc__,
"length_hint(obj, default=0) -> int\n"
"Return an estimate of the number of items in obj.\n"
"This is useful for presizing containers when building from an\n"
"iterable.\n"
"\n"
"If the object supports len(), the result will be\n"
"exact. Otherwise, it may over- or under-estimate by an\n"
"arbitrary amount. The result will be an integer >= 0.");

static PyObject *length_hint(PyObject *self, PyObject *args)
{
    PyObject *obj;
    Py_ssize_t defaultvalue = 0, res;
    if (!PyArg_ParseTuple(args, "O|n:length_hint", &obj, &defaultvalue)) {
        return NULL;
    }
    res = PyObject_LengthHint(obj, defaultvalue);
    if (res == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromSsize_t(res);
}


PyDoc_STRVAR(compare_digest__doc__,
"compare_digest(a, b) -> bool\n"
"\n"
"Return 'a == b'.  This function uses an approach designed to prevent\n"
"timing analysis, making it appropriate for cryptography.\n"
"a and b must both be of the same type: either str (ASCII only),\n"
"or any bytes-like object.\n"
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

    /* ASCII unicode string */
    if(PyUnicode_Check(a) && PyUnicode_Check(b)) {
        if (PyUnicode_READY(a) == -1 || PyUnicode_READY(b) == -1) {
            return NULL;
        }
        if (!PyUnicode_IS_ASCII(a) || !PyUnicode_IS_ASCII(b)) {
            PyErr_SetString(PyExc_TypeError,
                            "comparing strings with non-ASCII characters is "
                            "not supported");
            return NULL;
        }

        rc = _tscmp(PyUnicode_DATA(a),
                    PyUnicode_DATA(b),
                    PyUnicode_GET_LENGTH(a),
                    PyUnicode_GET_LENGTH(b));
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

/* operator methods **********************************************************/

#define spam1(OP,DOC) {#OP, OP, METH_VARARGS, PyDoc_STR(DOC)},
#define spam2(OP,DOC) {#OP, op_##OP, METH_VARARGS, PyDoc_STR(DOC)},
#define spam1o(OP,DOC) {#OP, OP, METH_O, PyDoc_STR(DOC)},
#define spam2o(OP,DOC) {#OP, op_##OP, METH_O, PyDoc_STR(DOC)},

static struct PyMethodDef operator_methods[] = {

spam1o(truth,
 "truth(a) -- Return True if a is true, False otherwise.")
spam2(contains,
 "contains(a, b) -- Same as b in a (note reversed operands).")
spam1(indexOf,
 "indexOf(a, b) -- Return the first index of b in a.")
spam1(countOf,
 "countOf(a, b) -- Return the number of times b occurs in a.")

spam1(is_, "is_(a, b) -- Same as a is b.")
spam1(is_not, "is_not(a, b) -- Same as a is not b.")
spam2o(index, "index(a) -- Same as a.__index__()")
spam2(add, "add(a, b) -- Same as a + b.")
spam2(sub, "sub(a, b) -- Same as a - b.")
spam2(mul, "mul(a, b) -- Same as a * b.")
spam2(matmul, "matmul(a, b) -- Same as a @ b.")
spam2(floordiv, "floordiv(a, b) -- Same as a // b.")
spam2(truediv, "truediv(a, b) -- Same as a / b.")
spam2(mod, "mod(a, b) -- Same as a % b.")
spam2o(neg, "neg(a) -- Same as -a.")
spam2o(pos, "pos(a) -- Same as +a.")
spam2o(abs, "abs(a) -- Same as abs(a).")
spam2o(inv, "inv(a) -- Same as ~a.")
spam2o(invert, "invert(a) -- Same as ~a.")
spam2(lshift, "lshift(a, b) -- Same as a << b.")
spam2(rshift, "rshift(a, b) -- Same as a >> b.")
spam2o(not_, "not_(a) -- Same as not a.")
spam2(and_, "and_(a, b) -- Same as a & b.")
spam2(xor, "xor(a, b) -- Same as a ^ b.")
spam2(or_, "or_(a, b) -- Same as a | b.")
spam2(iadd, "a = iadd(a, b) -- Same as a += b.")
spam2(isub, "a = isub(a, b) -- Same as a -= b.")
spam2(imul, "a = imul(a, b) -- Same as a *= b.")
spam2(imatmul, "a = imatmul(a, b) -- Same as a @= b.")
spam2(ifloordiv, "a = ifloordiv(a, b) -- Same as a //= b.")
spam2(itruediv, "a = itruediv(a, b) -- Same as a /= b")
spam2(imod, "a = imod(a, b) -- Same as a %= b.")
spam2(ilshift, "a = ilshift(a, b) -- Same as a <<= b.")
spam2(irshift, "a = irshift(a, b) -- Same as a >>= b.")
spam2(iand, "a = iand(a, b) -- Same as a &= b.")
spam2(ixor, "a = ixor(a, b) -- Same as a ^= b.")
spam2(ior, "a = ior(a, b) -- Same as a |= b.")
spam2(concat,
 "concat(a, b) -- Same as a + b, for a and b sequences.")
spam2(iconcat,
 "a = iconcat(a, b) -- Same as a += b, for a and b sequences.")
spam2(getitem,
 "getitem(a, b) -- Same as a[b].")
spam2(setitem,
 "setitem(a, b, c) -- Same as a[b] = c.")
spam2(delitem,
 "delitem(a, b) -- Same as del a[b].")
spam2(pow, "pow(a, b) -- Same as a ** b.")
spam2(ipow, "a = ipow(a, b) -- Same as a **= b.")
spam2(lt, "lt(a, b) -- Same as a<b.")
spam2(le, "le(a, b) -- Same as a<=b.")
spam2(eq, "eq(a, b) -- Same as a==b.")
spam2(ne, "ne(a, b) -- Same as a!=b.")
spam2(gt, "gt(a, b) -- Same as a>b.")
spam2(ge, "ge(a, b) -- Same as a>=b.")

    {"_compare_digest", (PyCFunction)compare_digest, METH_VARARGS,
     compare_digest__doc__},
     {"length_hint", (PyCFunction)length_hint, METH_VARARGS,
     length_hint__doc__},
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

    if (kw != NULL && !_PyArg_NoKeywords("itemgetter", kw))
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

static PyObject *
itemgetter_repr(itemgetterobject *ig)
{
    PyObject *repr;
    const char *reprfmt;

    int status = Py_ReprEnter((PyObject *)ig);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(ig)->tp_name);
    }

    reprfmt = ig->nitems == 1 ? "%s(%R)" : "%s%R";
    repr = PyUnicode_FromFormat(reprfmt, Py_TYPE(ig)->tp_name, ig->item);
    Py_ReprLeave((PyObject *)ig);
    return repr;
}

static PyObject *
itemgetter_reduce(itemgetterobject *ig)
{
    if (ig->nitems == 1)
        return Py_BuildValue("O(O)", Py_TYPE(ig), ig->item);
    return PyTuple_Pack(2, Py_TYPE(ig), ig->item);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling");

static PyMethodDef itemgetter_methods[] = {
    {"__reduce__", (PyCFunction)itemgetter_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};

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
    0,                                  /* tp_reserved */
    (reprfunc)itemgetter_repr,          /* tp_repr */
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
    itemgetter_methods,                 /* tp_methods */
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
    Py_ssize_t nattrs, idx, char_idx;

    if (!_PyArg_NoKeywords("attrgetter()", kwds))
        return NULL;

    nattrs = PyTuple_GET_SIZE(args);
    if (nattrs <= 1) {
        if (!PyArg_UnpackTuple(args, "attrgetter", 1, 1, &attr))
            return NULL;
    }

    attr = PyTuple_New(nattrs);
    if (attr == NULL)
        return NULL;

    /* prepare attr while checking args */
    for (idx = 0; idx < nattrs; ++idx) {
        PyObject *item = PyTuple_GET_ITEM(args, idx);
        Py_ssize_t item_len;
        void *data;
        unsigned int kind;
        int dot_count;

        if (!PyUnicode_Check(item)) {
            PyErr_SetString(PyExc_TypeError,
                            "attribute name must be a string");
            Py_DECREF(attr);
            return NULL;
        }
        if (PyUnicode_READY(item)) {
            Py_DECREF(attr);
            return NULL;
        }
        item_len = PyUnicode_GET_LENGTH(item);
        kind = PyUnicode_KIND(item);
        data = PyUnicode_DATA(item);

        /* check whethere the string is dotted */
        dot_count = 0;
        for (char_idx = 0; char_idx < item_len; ++char_idx) {
            if (PyUnicode_READ(kind, data, char_idx) == '.')
                ++dot_count;
        }

        if (dot_count == 0) {
            Py_INCREF(item);
            PyUnicode_InternInPlace(&item);
            PyTuple_SET_ITEM(attr, idx, item);
        } else { /* make it a tuple of non-dotted attrnames */
            PyObject *attr_chain = PyTuple_New(dot_count + 1);
            PyObject *attr_chain_item;
            Py_ssize_t unibuff_from = 0;
            Py_ssize_t unibuff_till = 0;
            Py_ssize_t attr_chain_idx = 0;

            if (attr_chain == NULL) {
                Py_DECREF(attr);
                return NULL;
            }

            for (; dot_count > 0; --dot_count) {
                while (PyUnicode_READ(kind, data, unibuff_till) != '.') {
                    ++unibuff_till;
                }
                attr_chain_item = PyUnicode_Substring(item,
                                      unibuff_from,
                                      unibuff_till);
                if (attr_chain_item == NULL) {
                    Py_DECREF(attr_chain);
                    Py_DECREF(attr);
                    return NULL;
                }
                PyUnicode_InternInPlace(&attr_chain_item);
                PyTuple_SET_ITEM(attr_chain, attr_chain_idx, attr_chain_item);
                ++attr_chain_idx;
                unibuff_till = unibuff_from = unibuff_till + 1;
            }

            /* now add the last dotless name */
            attr_chain_item = PyUnicode_Substring(item,
                                                  unibuff_from, item_len);
            if (attr_chain_item == NULL) {
                Py_DECREF(attr_chain);
                Py_DECREF(attr);
                return NULL;
            }
            PyUnicode_InternInPlace(&attr_chain_item);
            PyTuple_SET_ITEM(attr_chain, attr_chain_idx, attr_chain_item);

            PyTuple_SET_ITEM(attr, idx, attr_chain);
        }
    }

    /* create attrgetterobject structure */
    ag = PyObject_GC_New(attrgetterobject, &attrgetter_type);
    if (ag == NULL) {
        Py_DECREF(attr);
        return NULL;
    }

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
    PyObject *newobj;

    /* attr is either a tuple or instance of str.
       Ensured by the setup code of attrgetter_new */
    if (PyTuple_CheckExact(attr)) { /* chained getattr */
        Py_ssize_t name_idx = 0, name_count;
        PyObject *attr_name;

        name_count = PyTuple_GET_SIZE(attr);
        Py_INCREF(obj);
        for (name_idx = 0; name_idx < name_count; ++name_idx) {
            attr_name = PyTuple_GET_ITEM(attr, name_idx);
            newobj = PyObject_GetAttr(obj, attr_name);
            Py_DECREF(obj);
            if (newobj == NULL) {
                return NULL;
            }
            /* here */
            obj = newobj;
        }
    } else { /* single getattr */
        newobj = PyObject_GetAttr(obj, attr);
        if (newobj == NULL)
            return NULL;
        obj = newobj;
    }

    return obj;
}

static PyObject *
attrgetter_call(attrgetterobject *ag, PyObject *args, PyObject *kw)
{
    PyObject *obj, *result;
    Py_ssize_t i, nattrs=ag->nattrs;

    if (kw != NULL && !_PyArg_NoKeywords("attrgetter", kw))
        return NULL;
    if (!PyArg_UnpackTuple(args, "attrgetter", 1, 1, &obj))
        return NULL;
    if (ag->nattrs == 1) /* ag->attr is always a tuple */
        return dotted_getattr(obj, PyTuple_GET_ITEM(ag->attr, 0));

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

static PyObject *
dotjoinattr(PyObject *attr, PyObject **attrsep)
{
    if (PyTuple_CheckExact(attr)) {
        if (*attrsep == NULL) {
            *attrsep = PyUnicode_FromString(".");
            if (*attrsep == NULL)
                return NULL;
        }
        return PyUnicode_Join(*attrsep, attr);
    } else {
        Py_INCREF(attr);
        return attr;
    }
}

static PyObject *
attrgetter_args(attrgetterobject *ag)
{
    Py_ssize_t i;
    PyObject *attrsep = NULL;
    PyObject *attrstrings = PyTuple_New(ag->nattrs);
    if (attrstrings == NULL)
        return NULL;

    for (i = 0; i < ag->nattrs; ++i) {
        PyObject *attr = PyTuple_GET_ITEM(ag->attr, i);
        PyObject *attrstr = dotjoinattr(attr, &attrsep);
        if (attrstr == NULL) {
            Py_XDECREF(attrsep);
            Py_DECREF(attrstrings);
            return NULL;
        }
        PyTuple_SET_ITEM(attrstrings, i, attrstr);
    }
    Py_XDECREF(attrsep);
    return attrstrings;
}

static PyObject *
attrgetter_repr(attrgetterobject *ag)
{
    PyObject *repr = NULL;
    int status = Py_ReprEnter((PyObject *)ag);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(ag)->tp_name);
    }

    if (ag->nattrs == 1) {
        PyObject *attrsep = NULL;
        PyObject *attr = dotjoinattr(PyTuple_GET_ITEM(ag->attr, 0), &attrsep);
        if (attr != NULL) {
            repr = PyUnicode_FromFormat("%s(%R)", Py_TYPE(ag)->tp_name, attr);
            Py_DECREF(attr);
        }
        Py_XDECREF(attrsep);
    }
    else {
        PyObject *attrstrings = attrgetter_args(ag);
        if (attrstrings != NULL) {
            repr = PyUnicode_FromFormat("%s%R",
                                        Py_TYPE(ag)->tp_name, attrstrings);
            Py_DECREF(attrstrings);
        }
    }
    Py_ReprLeave((PyObject *)ag);
    return repr;
}

static PyObject *
attrgetter_reduce(attrgetterobject *ag)
{
    PyObject *attrstrings = attrgetter_args(ag);
    if (attrstrings == NULL)
        return NULL;

    return Py_BuildValue("ON", Py_TYPE(ag), attrstrings);
}

static PyMethodDef attrgetter_methods[] = {
    {"__reduce__", (PyCFunction)attrgetter_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};

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
    0,                                  /* tp_reserved */
    (reprfunc)attrgetter_repr,          /* tp_repr */
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
    attrgetter_methods,                 /* tp_methods */
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
    PyObject *name;

    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "methodcaller needs at least "
                        "one argument, the method name");
        return NULL;
    }

    name = PyTuple_GET_ITEM(args, 0);
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "method name must be a string");
        return NULL;
    }

    /* create methodcallerobject structure */
    mc = PyObject_GC_New(methodcallerobject, &methodcaller_type);
    if (mc == NULL)
        return NULL;

    name = PyTuple_GET_ITEM(args, 0);
    Py_INCREF(name);
    PyUnicode_InternInPlace(&name);
    mc->name = name;

    Py_XINCREF(kwds);
    mc->kwds = kwds;

    mc->args = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    if (mc->args == NULL) {
        Py_DECREF(mc);
        return NULL;
    }

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

    if (kw != NULL && !_PyArg_NoKeywords("methodcaller", kw))
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

static PyObject *
methodcaller_repr(methodcallerobject *mc)
{
    PyObject *argreprs, *repr = NULL, *sep, *joinedargreprs;
    Py_ssize_t numtotalargs, numposargs, numkwdargs, i;
    int status = Py_ReprEnter((PyObject *)mc);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(mc)->tp_name);
    }

    if (mc->kwds != NULL) {
        numkwdargs = PyDict_Size(mc->kwds);
        if (numkwdargs < 0) {
            Py_ReprLeave((PyObject *)mc);
            return NULL;
        }
    } else {
        numkwdargs = 0;
    }

    numposargs = PyTuple_GET_SIZE(mc->args);
    numtotalargs = numposargs + numkwdargs;

    if (numtotalargs == 0) {
        repr = PyUnicode_FromFormat("%s(%R)", Py_TYPE(mc)->tp_name, mc->name);
        Py_ReprLeave((PyObject *)mc);
        return repr;
    }

    argreprs = PyTuple_New(numtotalargs);
    if (argreprs == NULL) {
        Py_ReprLeave((PyObject *)mc);
        return NULL;
    }

    for (i = 0; i < numposargs; ++i) {
        PyObject *onerepr = PyObject_Repr(PyTuple_GET_ITEM(mc->args, i));
        if (onerepr == NULL)
            goto done;
        PyTuple_SET_ITEM(argreprs, i, onerepr);
    }

    if (numkwdargs != 0) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(mc->kwds, &pos, &key, &value)) {
            PyObject *onerepr = PyUnicode_FromFormat("%U=%R", key, value);
            if (onerepr == NULL)
                goto done;
            if (i >= numtotalargs) {
                i = -1;
                break;
            }
            PyTuple_SET_ITEM(argreprs, i, onerepr);
            ++i;
        }
        if (i != numtotalargs) {
            PyErr_SetString(PyExc_RuntimeError,
                            "keywords dict changed size during iteration");
            goto done;
        }
    }

    sep = PyUnicode_FromString(", ");
    if (sep == NULL)
        goto done;

    joinedargreprs = PyUnicode_Join(sep, argreprs);
    Py_DECREF(sep);
    if (joinedargreprs == NULL)
        goto done;

    repr = PyUnicode_FromFormat("%s(%R, %U)", Py_TYPE(mc)->tp_name,
                                mc->name, joinedargreprs);
    Py_DECREF(joinedargreprs);

done:
    Py_DECREF(argreprs);
    Py_ReprLeave((PyObject *)mc);
    return repr;
}

static PyObject *
methodcaller_reduce(methodcallerobject *mc)
{
    PyObject *newargs;
    if (!mc->kwds || PyDict_Size(mc->kwds) == 0) {
        Py_ssize_t i;
        Py_ssize_t callargcount = PyTuple_GET_SIZE(mc->args);
        newargs = PyTuple_New(1 + callargcount);
        if (newargs == NULL)
            return NULL;
        Py_INCREF(mc->name);
        PyTuple_SET_ITEM(newargs, 0, mc->name);
        for (i = 0; i < callargcount; ++i) {
            PyObject *arg = PyTuple_GET_ITEM(mc->args, i);
            Py_INCREF(arg);
            PyTuple_SET_ITEM(newargs, i + 1, arg);
        }
        return Py_BuildValue("ON", Py_TYPE(mc), newargs);
    }
    else {
        PyObject *functools;
        PyObject *partial;
        PyObject *constructor;
        PyObject *newargs[2];

        _Py_IDENTIFIER(partial);
        functools = PyImport_ImportModule("functools");
        if (!functools)
            return NULL;
        partial = _PyObject_GetAttrId(functools, &PyId_partial);
        Py_DECREF(functools);
        if (!partial)
            return NULL;

        newargs[0] = (PyObject *)Py_TYPE(mc);
        newargs[1] = mc->name;
        constructor = _PyObject_FastCallDict(partial, newargs, 2, mc->kwds);

        Py_DECREF(partial);
        return Py_BuildValue("NO", constructor, mc->args);
    }
}

static PyMethodDef methodcaller_methods[] = {
    {"__reduce__", (PyCFunction)methodcaller_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};
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
    0,                                  /* tp_reserved */
    (reprfunc)methodcaller_repr,        /* tp_repr */
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
    methodcaller_methods,               /* tp_methods */
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


/* Initialization function for the module (*must* be called PyInit__operator) */


static struct PyModuleDef operatormodule = {
    PyModuleDef_HEAD_INIT,
    "_operator",
    operator_doc,
    -1,
    operator_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__operator(void)
{
    PyObject *m;

    /* Create the module and add the functions */
    m = PyModule_Create(&operatormodule);
    if (m == NULL)
        return NULL;

    if (PyType_Ready(&itemgetter_type) < 0)
        return NULL;
    Py_INCREF(&itemgetter_type);
    PyModule_AddObject(m, "itemgetter", (PyObject *)&itemgetter_type);

    if (PyType_Ready(&attrgetter_type) < 0)
        return NULL;
    Py_INCREF(&attrgetter_type);
    PyModule_AddObject(m, "attrgetter", (PyObject *)&attrgetter_type);

    if (PyType_Ready(&methodcaller_type) < 0)
        return NULL;
    Py_INCREF(&methodcaller_type);
    PyModule_AddObject(m, "methodcaller", (PyObject *)&methodcaller_type);
    return m;
}
