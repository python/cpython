
#include "Python.h"

PyDoc_STRVAR(operator_doc,
"Operator interface.\n\
\n\
This module exports a set of functions implemented in C corresponding\n\
to the intrinsic operators of Python.  For example, operator.add(x, y)\n\
is equivalent to the expression x+y.  The function names are those\n\
used for special class methods; variants without leading and trailing\n\
'__' are also provided for convenience.");

#define spam1(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1; \
  if(! PyArg_ParseTuple(a,"O:" #OP,&a1)) return NULL; \
  return AOP(a1); }

#define spam2(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; \
  if(! PyArg_ParseTuple(a,"OO:" #OP,&a1,&a2)) return NULL; \
  return AOP(a1,a2); }

#define spamoi(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1; int a2; \
  if(! PyArg_ParseTuple(a,"Oi:" #OP,&a1,&a2)) return NULL; \
  return AOP(a1,a2); }

#define spam2n(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; \
  if(! PyArg_ParseTuple(a,"OO:" #OP,&a1,&a2)) return NULL; \
  if(-1 == AOP(a1,a2)) return NULL; \
  Py_INCREF(Py_None); \
  return Py_None; }

#define spam3n(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2, *a3; \
  if(! PyArg_ParseTuple(a,"OOO:" #OP,&a1,&a2,&a3)) return NULL; \
  if(-1 == AOP(a1,a2,a3)) return NULL; \
  Py_INCREF(Py_None); \
  return Py_None; }

#define spami(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1; long r; \
  if(! PyArg_ParseTuple(a,"O:" #OP,&a1)) return NULL; \
  if(-1 == (r=AOP(a1))) return NULL; \
  return PyBool_FromLong(r); }

#define spami2(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; long r; \
  if(! PyArg_ParseTuple(a,"OO:" #OP,&a1,&a2)) return NULL; \
  if(-1 == (r=AOP(a1,a2))) return NULL; \
  return PyInt_FromLong(r); }

#define spami2b(OP,AOP) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; long r; \
  if(! PyArg_ParseTuple(a,"OO:" #OP,&a1,&a2)) return NULL; \
  if(-1 == (r=AOP(a1,a2))) return NULL; \
  return PyBool_FromLong(r); }

#define spamrc(OP,A) static PyObject *OP(PyObject *s, PyObject *a) { \
  PyObject *a1, *a2; \
  if(! PyArg_ParseTuple(a,"OO:" #OP,&a1,&a2)) return NULL; \
  return PyObject_RichCompare(a1,a2,A); }

spami(isCallable       , PyCallable_Check)
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
spami(isSequenceType   , PySequence_Check)
spam2(op_concat        , PySequence_Concat)
spamoi(op_repeat       , PySequence_Repeat)
spami2b(op_contains     , PySequence_Contains)
spami2b(sequenceIncludes, PySequence_Contains)
spami2(indexOf         , PySequence_Index)
spami2(countOf         , PySequence_Count)
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
op_getslice(PyObject *s, PyObject *a)
{
        PyObject *a1;
        int a2,a3;

        if (!PyArg_ParseTuple(a,"Oii:getslice",&a1,&a2,&a3))
                return NULL;
        return PySequence_GetSlice(a1,a2,a3);
}

static PyObject*
op_setslice(PyObject *s, PyObject *a)
{
        PyObject *a1, *a4;
        int a2,a3;

        if (!PyArg_ParseTuple(a,"OiiO:setslice",&a1,&a2,&a3,&a4))
                return NULL;

        if (-1 == PySequence_SetSlice(a1,a2,a3,a4))
                return NULL;

        Py_INCREF(Py_None);
        return Py_None;
}

static PyObject*
op_delslice(PyObject *s, PyObject *a)
{
        PyObject *a1;
        int a2,a3;

        if(! PyArg_ParseTuple(a,"Oii:delslice",&a1,&a2,&a3))
                return NULL;

        if (-1 == PySequence_DelSlice(a1,a2,a3))
                return NULL;

        Py_INCREF(Py_None);
        return Py_None;
}

#undef spam1
#undef spam2
#define spam1(OP,DOC) {#OP, OP, METH_VARARGS, DOC},
#define spam2(OP,ALTOP,DOC) {#OP, op_##OP, METH_VARARGS, DOC}, \
			   {#ALTOP, op_##OP, METH_VARARGS, DOC}, 

static struct PyMethodDef operator_methods[] = {

spam1(isCallable,
 "isCallable(a) -- Same as callable(a).")
spam1(isNumberType,
 "isNumberType(a) -- Return True if a has a numeric type, False otherwise.")
spam1(isSequenceType,
 "isSequenceType(a) -- Return True if a has a sequence type, False otherwise.")
spam1(truth,
 "truth(a) -- Return True if a is true, False otherwise.")
spam2(contains,__contains__,
 "contains(a, b) -- Same as b in a (note reversed operands).")
spam1(sequenceIncludes,
 "sequenceIncludes(a, b) -- Same as b in a (note reversed operands; deprecated).")
spam1(indexOf,
 "indexOf(a, b) -- Return the first index of b in a.")
spam1(countOf,
 "countOf(a, b) -- Return the number of times b occurs in a.")
spam1(isMappingType,
 "isMappingType(a) -- Return True if a has a mapping type, False otherwise.")

spam2(add,__add__, "add(a, b) -- Same as a + b.")
spam2(sub,__sub__, "sub(a, b) -- Same as a - b.")
spam2(mul,__mul__, "mul(a, b) -- Same as a * b.")
spam2(div,__div__, "div(a, b) -- Same as a / b when __future__.division is not in effect.")
spam2(floordiv,__floordiv__, "floordiv(a, b) -- Same as a // b.")
spam2(truediv,__truediv__, "truediv(a, b) -- Same as a / b when __future__.division is in effect.")
spam2(mod,__mod__, "mod(a, b) -- Same as a % b.")
spam2(neg,__neg__, "neg(a) -- Same as -a.")
spam2(pos,__pos__, "pos(a) -- Same as +a.")
spam2(abs,__abs__, "abs(a) -- Same as abs(a).")
spam2(inv,__inv__, "inv(a) -- Same as ~a.")
spam2(invert,__invert__, "invert(a) -- Same as ~a.")
spam2(lshift,__lshift__, "lshift(a, b) -- Same as a << b.")
spam2(rshift,__rshift__, "rshift(a, b) -- Same as a >> b.")
spam2(not_,__not__, "not_(a) -- Same as not a.")
spam2(and_,__and__, "and_(a, b) -- Same as a & b.")
spam2(xor,__xor__, "xor(a, b) -- Same as a ^ b.")
spam2(or_,__or__, "or_(a, b) -- Same as a | b.")
spam2(concat,__concat__,
 "concat(a, b) -- Same as a + b, for a and b sequences.")
spam2(repeat,__repeat__,
 "repeat(a, b) -- Return a * b, where a is a sequence, and b is an integer.")
spam2(getitem,__getitem__,
 "getitem(a, b) -- Same as a[b].")
spam2(setitem,__setitem__,
 "setitem(a, b, c) -- Same as a[b] = c.")
spam2(delitem,__delitem__,
 "delitem(a, b) -- Same as del a[b].")
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

	{NULL,		NULL}		/* sentinel */

};


/* Initialization function for the module (*must* be called initoperator) */

PyMODINIT_FUNC
initoperator(void)
{
        /* Create the module and add the functions */
        Py_InitModule4("operator", operator_methods, operator_doc,
		       (PyObject*)NULL, PYTHON_API_VERSION);
}
