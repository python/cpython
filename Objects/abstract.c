/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Abstract Object Interface (many thanks to Jim Fulton) */
 
#include "Python.h"

#define Py_TRY(E) if(!(E)) return NULL
#define Py_ASSERT(EXP,E,V) if(!(EXP)) return PyErr_SetString(E,V), (void*)NULL
#define SPAM printf("line %d\n",__LINE__)

static PyObject *
Py_ReturnMethodError(name)
  char *name;
{
  if(! name) name = "Unknown Error";
  PyErr_SetString(PyExc_AttributeError,name);
  return 0;
}

static PyObject *
Py_ReturnNullError()
{
  if(! PyErr_Occurred())
    PyErr_SetString(PyExc_SystemError,
		    "null argument to internal routine");
  return 0;
}

int 
PyObject_Cmp(o1, o2, result)
  PyObject *o1;
  PyObject *o2;
  int *result;
{
  int r;

  if(! o1 || ! o2) return Py_ReturnNullError(),-1;
  r=PyObject_Compare(o1,o2);
  if(PyErr_Occurred()) return -1;
  *result=r;
  return 0;
}

#if 0 /* Already in object.c */
int
PyCallable_Check(x)
  PyObject *x;
{
	if (x == NULL)
		return 0;
	if (x->ob_type->tp_call != NULL ||
	    PyFunction_Check(x) ||
	    PyMethod_Check(x) ||
	    PyCFunction_Check(x) ||
	    PyClass_Check(x))
		return 1;
	if (PyInstance_Check(x)) {
		PyObject *call = PyObject_GetAttrString(x, "__call__");
		if (call == NULL) {
			PyErr_Clear();
			return 0;
		}
		/* Could test recursively but don't, for fear of endless
		   recursion if some joker sets self.__call__ = self */
		Py_DECREF(call);
		return 1;
	}
	return 0;
}
#endif

PyObject *
PyObject_Type(o)
	PyObject *o;
{
	PyObject *v;

	if(! o) return Py_ReturnNullError();
	v = (PyObject *)o->ob_type;
	Py_INCREF(v);
	return v;
}

int
PyObject_Length(o)
  PyObject *o;
{
  PySequenceMethods *m;

  if(! o) return Py_ReturnNullError(),-1;

  if((m=o->ob_type->tp_as_sequence) && m->sq_length)
    return m->sq_length(o);

  return PyMapping_Length(o);
}

PyObject *
PyObject_GetItem(o, key)
  PyObject *o;
  PyObject *key;
{
  PyMappingMethods *m;

  if(! o || ! key) return Py_ReturnNullError();

  if((m=o->ob_type->tp_as_mapping) && m->mp_subscript)
    return m->mp_subscript(o,key);
  
  if(PyInt_Check(key))
    return PySequence_GetItem(o,PyInt_AsLong(key));

  PyErr_SetString(PyExc_TypeError,"expected integer index");
  return NULL;
}

int
PyObject_SetItem(o, key, value)
  PyObject *o;
  PyObject *key;
  PyObject *value;
{
  PyMappingMethods *m;

  if(! o || ! key || ! value) return Py_ReturnNullError(),-1;
  if((m=o->ob_type->tp_as_mapping) && m->mp_ass_subscript)
    return m->mp_ass_subscript(o,key,value);
  
  if(PyInt_Check(key))
    return PySequence_SetItem(o,PyInt_AsLong(key),value);

  PyErr_SetString(PyExc_TypeError,"expected integer index");
  return -1;
}

int
PyObject_DelItem(o, key)
  PyObject *o;
  PyObject *key;
{
  PyMappingMethods *m;

  if(! o || ! key) return Py_ReturnNullError(),-1;
  if((m=o->ob_type->tp_as_mapping) && m->mp_ass_subscript)
    return m->mp_ass_subscript(o,key,(PyObject*)NULL);
  
  if(PyInt_Check(key))
    return PySequence_SetItem(o,PyInt_AsLong(key),(PyObject*)NULL);

  PyErr_SetString(PyExc_TypeError,"expected integer index");
  return -1;
}

int 
PyNumber_Check(o)
  PyObject *o;
{
  return o && o->ob_type->tp_as_number;
}


#define BINOP(opname, ropname, thisfunc) \
	if (!PyInstance_Check(v) && !PyInstance_Check(w)) \
		; \
	else \
		return PyInstance_DoBinOp(v, w, opname, ropname, thisfunc)

PyObject *
PyNumber_Or(v, w)
	PyObject *v, *w;
{
        extern int PyNumber_Coerce();

	BINOP("__or__", "__ror__", PyNumber_Or);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_or) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for |");
	return NULL;
}

PyObject *
PyNumber_Xor(v, w)
	PyObject *v, *w;
{
        extern int PyNumber_Coerce();

	BINOP("__xor__", "__rxor__", PyNumber_Xor);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_xor) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for ^");
	return NULL;
}

PyObject *
PyNumber_And(v, w)
	PyObject *v, *w;
{
	BINOP("__and__", "__rand__", PyNumber_And);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_and) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for &");
	return NULL;
}

PyObject *
PyNumber_Lshift(v, w)
	PyObject *v, *w;
{
	BINOP("__lshift__", "__rlshift__", PyNumber_Lshift);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_lshift) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for <<");
	return NULL;
}

PyObject *
PyNumber_Rshift(v, w)
	PyObject *v, *w;
{
	BINOP("__rshift__", "__rrshift__", PyNumber_Rshift);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_rshift) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for >>");
	return NULL;
}

PyObject *
PyNumber_Add(v, w)
	PyObject *v, *w;
{
	BINOP("__add__", "__radd__", PyNumber_Add);
	if (v->ob_type->tp_as_sequence != NULL)
		return (*v->ob_type->tp_as_sequence->sq_concat)(v, w);
	else if (v->ob_type->tp_as_number != NULL) {
		PyObject *x;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_add)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for +");
	return NULL;
}

PyObject *
PyNumber_Subtract(v, w)
	PyObject *v, *w;
{
	BINOP("__sub__", "__rsub__", PyNumber_Subtract);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_subtract)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for -");
	return NULL;
}

PyObject *
PyNumber_Multiply(v, w)
	PyObject *v, *w;
{
	PyTypeObject *tp;
	tp = v->ob_type;
	BINOP("__mul__", "__rmul__", PyNumber_Multiply);
	if (tp->tp_as_number != NULL &&
	    w->ob_type->tp_as_sequence != NULL &&
	    !PyInstance_Check(v)) {
		/* number*sequence -- swap v and w */
		PyObject *tmp = v;
		v = w;
		w = tmp;
		tp = v->ob_type;
	}
	if (tp->tp_as_number != NULL) {
		PyObject *x;
		if (PyInstance_Check(v)) {
			/* Instances of user-defined classes get their
			   other argument uncoerced, so they may
			   implement sequence*number as well as
			   number*number. */
			Py_INCREF(v);
			Py_INCREF(w);
		}
		else if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_multiply)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	if (tp->tp_as_sequence != NULL) {
		if (!PyInt_Check(w)) {
			PyErr_SetString(PyExc_TypeError,
				"can't multiply sequence with non-int");
			return NULL;
		}
		return (*tp->tp_as_sequence->sq_repeat)
						(v, (int)PyInt_AsLong(w));
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for *");
	return NULL;
}

PyObject *
PyNumber_Divide(v, w)
	PyObject *v, *w;
{
	BINOP("__div__", "__rdiv__", PyNumber_Divide);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_divide)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for /");
	return NULL;
}

PyObject *
PyNumber_Remainder(v, w)
	PyObject *v, *w;
{
	if (PyString_Check(v)) {
		return PyString_Format(v, w);
	}
	BINOP("__mod__", "__rmod__", PyNumber_Remainder);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_remainder)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for %");
	return NULL;
}

PyObject *
PyNumber_Divmod(v, w)
	PyObject *v, *w;
{
	PyObject *res;

	if (PyInstance_Check(v) || PyInstance_Check(w))
		return PyInstance_DoBinOp(v, w, "__divmod__", "__rdivmod__",
				     PyNumber_Divmod);
	if (v->ob_type->tp_as_number == NULL ||
				w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError,
		    "divmod() requires numeric or class instance arguments");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	res = (*v->ob_type->tp_as_number->nb_divmod)(v, w);
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}


static PyObject *
do_pow(v, w)
	PyObject *v, *w;
{
	PyObject *res;
	if (PyInstance_Check(v) || PyInstance_Check(w))
		return PyInstance_DoBinOp(v, w, "__pow__", "__rpow__", do_pow);
	if (v->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"pow() requires numeric arguments");
		return NULL;
	}
	if (PyFloat_Check(w) && PyFloat_AsDouble(v) < 0.0) {
		if (!PyErr_Occurred())
		    PyErr_SetString(PyExc_ValueError,
				    "negative number to float power");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	res = (*v->ob_type->tp_as_number->nb_power)(v, w, Py_None);
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

PyObject *
PyNumber_Power(v,w,z)
	PyObject *v, *w, *z;
{
	PyObject *res;
	PyObject *v1, *z1, *w2, *z2;

	if (z == Py_None)
		return do_pow(v, w);
	/* XXX The ternary version doesn't do class instance coercions */
	if (PyInstance_Check(v))
		return v->ob_type->tp_as_number->nb_power(v, w, z);
	if (v->ob_type->tp_as_number == NULL ||
	    z->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError, "pow() requires numeric arguments");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	res = NULL;
	v1 = v;
	z1 = z;
	if (PyNumber_Coerce(&v1, &z1) != 0)
		goto error2;
	w2 = w;
	z2 = z1;
 	if (PyNumber_Coerce(&w2, &z2) != 0)
		goto error1;
	res = (*v1->ob_type->tp_as_number->nb_power)(v1, w2, z2);
	Py_DECREF(w2);
	Py_DECREF(z2);
 error1:
	Py_DECREF(v1);
	Py_DECREF(z1);
 error2:
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}


PyObject *
PyNumber_Negative(v)
	PyObject *v;
{
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_negative)(v);
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for unary -");
	return NULL;
}

PyObject *
PyNumber_Positive(v)
	PyObject *v;
{
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_positive)(v);
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for unary +");
	return NULL;
}

PyObject *
PyNumber_Invert(v)
	PyObject *v;
{
	PyObject * (*f) Py_FPROTO((PyObject *));
	if (v->ob_type->tp_as_number != NULL &&
		(f = v->ob_type->tp_as_number->nb_invert) != NULL)
		return (*f)(v);
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for unary ~");
	return NULL;
}

PyObject *
PyNumber_Absolute(o)
  PyObject *o;
{
  PyNumberMethods *m;

  if(! o) return Py_ReturnNullError();
  if((m=o->ob_type->tp_as_number) && m->nb_absolute)
    return m->nb_absolute(o);

  return Py_ReturnMethodError("__abs__");  
}

PyObject *
PyNumber_Int(o)
  PyObject *o;
{
  PyNumberMethods *m;

  if(! o) return Py_ReturnNullError();
  if((m=o->ob_type->tp_as_number) && m->nb_int)
    return m->nb_int(o);

  return Py_ReturnMethodError("__int__");  
}

PyObject *
PyNumber_Long(o)
  PyObject *o;
{
  PyNumberMethods *m;

  if(! o) return Py_ReturnNullError();
  if((m=o->ob_type->tp_as_number) && m->nb_long)
    return m->nb_long(o);

  return Py_ReturnMethodError("__long__");  
}

PyObject *
PyNumber_Float(o)
  PyObject *o;
{
  PyNumberMethods *m;

  if(! o) return Py_ReturnNullError();
  if((m=o->ob_type->tp_as_number) && m->nb_float)
    return m->nb_float(o);

  return Py_ReturnMethodError("__float__");  
}


int 
PySequence_Check(o)
  PyObject *o;
{
  return o && o->ob_type->tp_as_sequence;
}

int 
PySequence_Length(s)
  PyObject *s;
{
  PySequenceMethods *m;

  if(! s) return Py_ReturnNullError(),-1;

  if((m=s->ob_type->tp_as_sequence) && m->sq_length)
    return m->sq_length(s);

  Py_ReturnMethodError("__len__");
  return -1;
}

PyObject *
PySequence_Concat(s, o)
  PyObject *s;
  PyObject *o;
{
  PySequenceMethods *m;

  if(! s || ! o) return Py_ReturnNullError();
      
  if((m=s->ob_type->tp_as_sequence) && m->sq_concat)
    return m->sq_concat(s,o);

  return Py_ReturnMethodError("__concat__");
}

PyObject *
PySequence_Repeat(o, count)
  PyObject *o;
  int count;
{
  PySequenceMethods *m;

  if(! o) return Py_ReturnNullError();
      
  if((m=o->ob_type->tp_as_sequence) && m->sq_repeat)
    return m->sq_repeat(o,count);

  return Py_ReturnMethodError("__repeat__");
}

PyObject *
PySequence_GetItem(s, i)
  PyObject *s;
  int i;
{
  PySequenceMethods *m;
  int l;

  if(! s) return Py_ReturnNullError();

  if(! ((m=s->ob_type->tp_as_sequence) && m->sq_item))
    return Py_ReturnMethodError("__getitem__");  

  if(i < 0)
    {
      if(! m->sq_length || 0 > (l=m->sq_length(s))) return NULL;
      i += l;
    }
      
  return m->sq_item(s,i);
}

PyObject *
PySequence_GetSlice(s, i1, i2)
  PyObject *s;
  int i1;
  int i2;
{
  PySequenceMethods *m;
  int l;

  if(! s) return Py_ReturnNullError();

  if(! ((m=s->ob_type->tp_as_sequence) && m->sq_slice))
    return Py_ReturnMethodError("__getslice__");  

  if(i1 < 0 || i2 < 0)
    {

      if(! m->sq_length || 0 > (l=m->sq_length(s))) return NULL;

      if(i1 < 0) i1 += l;
      if(i2 < 0) i2 += l;
    }
      
  return m->sq_slice(s,i1,i2);
}

int
PySequence_SetItem(s, i, o)
  PyObject *s;
  int i;
  PyObject *o;
{
  PySequenceMethods *m;
  int l;
  if(! s) return Py_ReturnNullError(),-1;

  if(! ((m=s->ob_type->tp_as_sequence) && m->sq_length && m->sq_ass_item))
    return Py_ReturnMethodError("__setitem__"),-1;  

  if(i < 0)
    {
      if(0 > (l=m->sq_length(s))) return -1;
      i += l;
    }
      
  return m->sq_ass_item(s,i,o);
}

int
PySequence_DelItem(s, i)
  PyObject *s;
  int i;
{
  PySequenceMethods *m;
  int l;
  if(! s) return Py_ReturnNullError(),-1;

  if(! ((m=s->ob_type->tp_as_sequence) && m->sq_length && m->sq_ass_item))
    return Py_ReturnMethodError("__delitem__"),-1;  

  if(i < 0)
    {
      if(0 > (l=m->sq_length(s))) return -1;
      i += l;
    }
      
  return m->sq_ass_item(s,i,(PyObject*)NULL);
}

int 
PySequence_SetSlice(s, i1, i2, o)
  PyObject *s;
  int i1;
  int i2;
  PyObject *o;
{
  PySequenceMethods *m;
  int l;

  if(! s) return Py_ReturnNullError(),-1;

  if(! ((m=s->ob_type->tp_as_sequence) && m->sq_length && m->sq_ass_slice))
    return Py_ReturnMethodError("__setslice__"),-1;  

  if(0 > (l=m->sq_length(s))) return -1;

  if(i1 < 0) i1 += l;
  if(i2 < 0) i2 += l;
      
  return m->sq_ass_slice(s,i1,i2,o);
}

int 
PySequence_DelSlice(s, i1, i2)
  PyObject *s;
  int i1;
  int i2;
{
  PySequenceMethods *m;
  int l;

  if(! s) return Py_ReturnNullError(),-1;

  if(! ((m=s->ob_type->tp_as_sequence) && m->sq_length && m->sq_ass_slice))
    return Py_ReturnMethodError("__delslice__"),-1;  

  if(0 > (l=m->sq_length(s))) return -1;

  if(i1 < 0) i1 += l;
  if(i2 < 0) i2 += l;
      
  return m->sq_ass_slice(s,i1,i2,(PyObject*)NULL);
}

PyObject *
PySequence_Tuple(s)
  PyObject *s;
{
  int l, i;
  PyObject *t, *item;

  if(! s) return Py_ReturnNullError();

  Py_TRY((l=PySequence_Length(s)) != -1);
  Py_TRY(t=PyTuple_New(l));

  for(i=0; i < l; i++)
    {
      if(((item=PySequence_GetItem(s,i))) ||
	 PyTuple_SetItem(t,i,item) == -1)
	{
	  Py_DECREF(t);
	  return NULL;
	}
    }
  return t;
}

PyObject *
PySequence_List(s)
  PyObject *s;
{
  int l, i;
  PyObject *t, *item;

  if(! s) return Py_ReturnNullError();

  Py_TRY((l=PySequence_Length(s)) != -1);
  Py_TRY(t=PyList_New(l));

  for(i=0; i < l; i++)
    {
      if((item=PySequence_GetItem(s,i)) ||
	 PyList_SetItem(t,i,item) == -1)
	{
	  Py_DECREF(t);
	  return NULL;
	}
    }
  return t;
}

int 
PySequence_Count(s, o)
  PyObject *s;
  PyObject *o;
{
  int l, i, n=0, not_equal, err;
  PyObject *item;

  if(! s || ! o) return Py_ReturnNullError(), -1;
  if((l=PySequence_Length(s)) == -1) return -1;

  for(i=0; i < l; i++)
    {
      if((item=PySequence_GetItem(s,i)) == NULL) return -1;
      err=PyObject_Cmp(item,o,&not_equal) == -1;
      Py_DECREF(item);
      if(err) return -1;
      n += ! not_equal;
    }
  return n;
}

int 
PySequence_In(s, o)
  PyObject *s;
  PyObject *o;
{
  int l, i, not_equal, err;
  PyObject *item;

  if(! o || ! s) return Py_ReturnNullError(), -1;
  if((l=PySequence_Length(s)) == -1) return -1;

  for(i=0; i < l; i++)
    {
      if((item=PySequence_GetItem(s,i)) == NULL) return -1;
      err=PyObject_Cmp(item,o,&not_equal) == -1;
      Py_DECREF(item);
      if(err) return -1;
      if(! not_equal) return 1;
    }
  return 0;
}

int 
PySequence_Index(s, o)
  PyObject *s;
  PyObject *o;
{
  int l, i, not_equal, err;
  PyObject *item;

  if(! s || ! o) return Py_ReturnNullError(), -1;
  if((l=PySequence_Length(s)) == -1) return -1;

  for(i=0; i < l; i++)
    {
      if((item=PySequence_GetItem(s,i)) == NULL) return -1;
      err=PyObject_Cmp(item,o,&not_equal) == -1;
      Py_DECREF(item);
      if(err) return -1;
      if(! not_equal) return i;
    }
  PyErr_SetString(PyExc_ValueError, "list.index(x): x not in list");
  return -1;
}

int 
PyMapping_Check(o)
  PyObject *o;
{
  return o && o->ob_type->tp_as_mapping;
}

int 
PyMapping_Length(s)
  PyObject *s;
{
  PyMappingMethods *m;

  if(! s) return Py_ReturnNullError(),-1;

  if((m=s->ob_type->tp_as_mapping) && m->mp_length)
    return m->mp_length(s);

  Py_ReturnMethodError("__len__");
  return -1;
}

int 
PyMapping_HasKeyString(o, key)
  PyObject *o;
  char *key;
{
  PyObject *v;

  v=PyMapping_GetItemString(o,key);
  if(v) {
    Py_DECREF(v);
    return 1;
  }
  PyErr_Clear();
  return 0;
}

int 
PyMapping_HasKey(o, key)
  PyObject *o;
  PyObject *key;
{
  PyObject *v;

  v=PyObject_GetItem(o,key);
  if(v) {
    Py_DECREF(v);
    return 1;
  }
  PyErr_Clear();
  return 0;
}

PyObject *
PyObject_CallObject(o, a)
  PyObject *o, *a;
{
  PyObject *r;

  if(a) return PyEval_CallObject(o,a);

  if(! (a=PyTuple_New(0)))
    return NULL;
  r=PyEval_CallObject(o,a);
  Py_DECREF(a);
  return r;
} 

PyObject *
#ifdef HAVE_STDARG_PROTOTYPES
/* VARARGS 2 */
PyObject_CallFunction(PyObject *PyCallable_Check, char *format, ...)
#else
/* VARARGS */
PyObject_CallFunction(va_alist) va_dcl
#endif
{
  va_list va;
  PyObject *args, *retval;
#ifdef HAVE_STDARG_PROTOTYPES
  va_start(va, format);
#else
  PyObject *PyCallable_Check;
  char *format;
  va_start(va);
  PyCallable_Check = va_arg(va, PyObject *);
  format   = va_arg(va, char *);
#endif

  if( ! PyCallable_Check)
    {
      va_end(va);
      return Py_ReturnNullError();
    }

  if(format)
    args = Py_VaBuildValue(format, va);
  else
    args = PyTuple_New(0);
  
  va_end(va);
  if(! args) return NULL;

  if(! PyTuple_Check(args))
    {
      PyObject *a;
      
      Py_TRY(a=PyTuple_New(1));
      Py_TRY(PyTuple_SetItem(a,0,args) != -1);
      args=a;
    }
  retval = PyObject_CallObject(PyCallable_Check,args);
  Py_DECREF(args);
  return retval;
}

PyObject *
#ifdef HAVE_STDARG_PROTOTYPES
/* VARARGS 2 */
PyObject_CallMethod(PyObject *o, char *name, char *format, ...)
#else
/* VARARGS */
PyObject_CallMethod(va_alist) va_dcl
#endif
{
  va_list va;
  PyObject *args, *PyCFunction=0, *retval;
#ifdef HAVE_STDARG_PROTOTYPES
  va_start(va, format);
#else
  PyObject *o;
  char *name;
  char *format;
  va_start(va);
  o      = va_arg(va, PyObject *);
  name   = va_arg(va, char *);
  format = va_arg(va, char *);
#endif

  if( ! o || ! name)
    {
      va_end(va);
      return Py_ReturnNullError();
    }

  PyCFunction=PyObject_GetAttrString(o,name);
  if(! PyCFunction)
    {
      va_end(va);
      PyErr_SetString(PyExc_AttributeError,name);
      return 0;
    }
   
  if(! (PyCallable_Check(PyCFunction)))
    {
      va_end(va);
      PyErr_SetString(PyExc_TypeError,"call of non-callable attribute");
      return 0;
    }

  if(format && *format)
    args = Py_VaBuildValue(format, va);
  else
    args = PyTuple_New(0);
  
  va_end(va);

  if(! args) return NULL;

  if(! PyTuple_Check(args))
    {
      PyObject *a;
      
      Py_TRY(a=PyTuple_New(1));
      Py_TRY(PyTuple_SetItem(a,0,args) != -1);
      args=a;
    }

  retval = PyObject_CallObject(PyCFunction,args);
  Py_DECREF(args);
  Py_DECREF(PyCFunction);
  return retval;
}

PyObject *
PyMapping_GetItemString(o, key)
  PyObject *o;
  char *key;
{
  PyObject *okey, *r;

  if( ! key) return Py_ReturnNullError();
  Py_TRY(okey=PyString_FromString(key));
  r = PyObject_GetItem(o,okey);
  Py_DECREF(okey);
  return r;
}

int
PyMapping_SetItemString(o, key, value)
 PyObject *o;
 char *key;
 PyObject *value;
{
  PyObject *okey;
  int r;

  if( ! key) return Py_ReturnNullError(),-1;
  if (!(okey=PyString_FromString(key))) return -1;
  r = PyObject_SetItem(o,okey,value);
  Py_DECREF(okey);
  return r;
}
