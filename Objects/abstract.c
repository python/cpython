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
#include <ctype.h>

/* Shorthands to return certain errors */

static PyObject *
type_error(msg)
	char *msg;
{
	PyErr_SetString(PyExc_TypeError, msg);
	return NULL;
}

static PyObject *
null_error()
{
	if (!PyErr_Occurred())
		PyErr_SetString(PyExc_SystemError,
				"null argument to internal routine");
	return NULL;
}

/* Copied with modifications from stropmodule.c: atoi, atof, atol */

static PyObject *
int_from_string(v)
	PyObject *v;
{
	extern long PyOS_strtol Py_PROTO((const char *, char **, int));
	char *s, *end;
	long x;
	char buffer[256]; /* For errors */

	s = PyString_AS_STRING(v);
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	errno = 0;
	x = PyOS_strtol(s, &end, 10);
	if (end == s || !isdigit(end[-1]))
		goto bad;
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
  bad:
		sprintf(buffer, "invalid literal for int(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (end != PyString_AS_STRING(v) + PyString_GET_SIZE(v)) {
		PyErr_SetString(PyExc_ValueError,
				"null byte in argument for int()");
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "int() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	return PyInt_FromLong(x);
}

static PyObject *
long_from_string(v)
	PyObject *v;
{
	char *s, *end;
	PyObject *x;
	char buffer[256]; /* For errors */

	s = PyString_AS_STRING(v);
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	x = PyLong_FromString(s, &end, 10);
	if (x == NULL) {
		if (PyErr_ExceptionMatches(PyExc_ValueError))
			goto bad;
		return NULL;
	}
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
  bad:
		sprintf(buffer, "invalid literal for long(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		Py_XDECREF(x);
		return NULL;
	}
	else if (end != PyString_AS_STRING(v) + PyString_GET_SIZE(v)) {
		PyErr_SetString(PyExc_ValueError,
				"null byte in argument for long()");
		return NULL;
	}
	return x;
}

static PyObject *
float_from_string(v)
	PyObject *v;
{
	extern double strtod Py_PROTO((const char *, char **));
	char *s, *last, *end;
	double x;
	char buffer[256]; /* For errors */

	s = PyString_AS_STRING(v);
	last = s + PyString_GET_SIZE(v);
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for float()");
		return NULL;
	}
	errno = 0;
	PyFPE_START_PROTECT("float_from_string", return 0)
	x = strtod(s, &end);
	PyFPE_END_PROTECT(x)
	/* Believe it or not, Solaris 2.6 can move end *beyond* the null
	   byte at the end of the string, when the input is inf(inity) */
	if (end > last)
		end = last;
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for float(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (end != PyString_AS_STRING(v) + PyString_GET_SIZE(v)) {
		PyErr_SetString(PyExc_ValueError,
				"null byte in argument for float()");
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "float() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	return PyFloat_FromDouble(x);
}

/* Operations on any object */

int
PyObject_Cmp(o1, o2, result)
	PyObject *o1;
	PyObject *o2;
	int *result;
{
	int r;

	if (o1 == NULL || o2 == NULL) {
		null_error();
		return -1;
	}
	r = PyObject_Compare(o1, o2);
	if (PyErr_Occurred())
		return -1;
	*result = r;
	return 0;
}

PyObject *
PyObject_Type(o)
	PyObject *o;
{
	PyObject *v;

	if (o == NULL)
		return null_error();
	v = (PyObject *)o->ob_type;
	Py_INCREF(v);
	return v;
}

int
PyObject_Length(o)
	PyObject *o;
{
	PySequenceMethods *m;

	if (o == NULL) {
		null_error();
		return -1;
	}

	m = o->ob_type->tp_as_sequence;
	if (m && m->sq_length)
		return m->sq_length(o);

	return PyMapping_Length(o);
}

PyObject *
PyObject_GetItem(o, key)
	PyObject *o;
	PyObject *key;
{
	PyMappingMethods *m;

	if (o == NULL || key == NULL)
		return null_error();

	m = o->ob_type->tp_as_mapping;
	if (m && m->mp_subscript)
		return m->mp_subscript(o, key);

	if (o->ob_type->tp_as_sequence) {
		if (PyInt_Check(key))
			return PySequence_GetItem(o, PyInt_AsLong(key));
		return type_error("sequence index must be integer");
	}

	return type_error("unsubscriptable object");
}

int
PyObject_SetItem(o, key, value)
	PyObject *o;
	PyObject *key;
	PyObject *value;
{
	PyMappingMethods *m;

	if (o == NULL || key == NULL || value == NULL) {
		null_error();
		return -1;
	}
	m = o->ob_type->tp_as_mapping;
	if (m && m->mp_ass_subscript)
		return m->mp_ass_subscript(o, key, value);

	if (o->ob_type->tp_as_sequence) {
		if (PyInt_Check(key))
			return PySequence_SetItem(o, PyInt_AsLong(key), value);
		type_error("sequence index must be integer");
		return -1;
	}

	type_error("object does not support item assignment");
	return -1;
}

int
PyObject_DelItem(o, key)
	PyObject *o;
	PyObject *key;
{
	PyMappingMethods *m;

	if (o == NULL || key == NULL) {
		null_error();
		return -1;
	}
	m = o->ob_type->tp_as_mapping;
	if (m && m->mp_ass_subscript)
		return m->mp_ass_subscript(o, key, (PyObject*)NULL);

	if (o->ob_type->tp_as_sequence) {
		if (PyInt_Check(key))
			return PySequence_DelItem(o, PyInt_AsLong(key));
		type_error("sequence index must be integer");
		return -1;
	}

	type_error("object does not support item deletion");
	return -1;
}

/* Operations on numbers */

int
PyNumber_Check(o)
	PyObject *o;
{
	return o && o->ob_type->tp_as_number;
}

/* Binary operators */

#define BINOP(v, w, opname, ropname, thisfunc) \
	if (PyInstance_Check(v) || PyInstance_Check(w)) \
		return PyInstance_DoBinOp(v, w, opname, ropname, thisfunc)

PyObject *
PyNumber_Or(v, w)
	PyObject *v, *w;
{
        extern int PyNumber_Coerce();

	BINOP(v, w, "__or__", "__ror__", PyNumber_Or);
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
	return type_error("bad operand type(s) for |");
}

PyObject *
PyNumber_Xor(v, w)
	PyObject *v, *w;
{
        extern int PyNumber_Coerce();

	BINOP(v, w, "__xor__", "__rxor__", PyNumber_Xor);
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
	return type_error("bad operand type(s) for ^");
}

PyObject *
PyNumber_And(v, w)
	PyObject *v, *w;
{
	BINOP(v, w, "__and__", "__rand__", PyNumber_And);
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
	return type_error("bad operand type(s) for &");
}

PyObject *
PyNumber_Lshift(v, w)
	PyObject *v, *w;
{
	BINOP(v, w, "__lshift__", "__rlshift__", PyNumber_Lshift);
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
	return type_error("bad operand type(s) for <<");
}

PyObject *
PyNumber_Rshift(v, w)
	PyObject *v, *w;
{
	BINOP(v, w, "__rshift__", "__rrshift__", PyNumber_Rshift);
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
	return type_error("bad operand type(s) for >>");
}

PyObject *
PyNumber_Add(v, w)
	PyObject *v, *w;
{
	PySequenceMethods *m;

	BINOP(v, w, "__add__", "__radd__", PyNumber_Add);
	m = v->ob_type->tp_as_sequence;
	if (m && m->sq_concat)
		return (*m->sq_concat)(v, w);
	else if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_add) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for +");
}

PyObject *
PyNumber_Subtract(v, w)
	PyObject *v, *w;
{
	BINOP(v, w, "__sub__", "__rsub__", PyNumber_Subtract);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_subtract) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for -");
}

PyObject *
PyNumber_Multiply(v, w)
	PyObject *v, *w;
{
	PyTypeObject *tp = v->ob_type;
	PySequenceMethods *m;

	BINOP(v, w, "__mul__", "__rmul__", PyNumber_Multiply);
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
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
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
		if ((f = v->ob_type->tp_as_number->nb_multiply) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	m = tp->tp_as_sequence;
	if (m && m->sq_repeat) {
		if (!PyInt_Check(w))
			return type_error(
				"can't multiply sequence with non-int");
		return (*m->sq_repeat)(v, (int)PyInt_AsLong(w));
	}
	return type_error("bad operand type(s) for *");
}

PyObject *
PyNumber_Divide(v, w)
	PyObject *v, *w;
{
	BINOP(v, w, "__div__", "__rdiv__", PyNumber_Divide);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_divide) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for /");
}

PyObject *
PyNumber_Remainder(v, w)
	PyObject *v, *w;
{
	if (PyString_Check(v))
		return PyString_Format(v, w);
	BINOP(v, w, "__mod__", "__rmod__", PyNumber_Remainder);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_remainder) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for %");
}

PyObject *
PyNumber_Divmod(v, w)
	PyObject *v, *w;
{
	BINOP(v, w, "__divmod__", "__rdivmod__", PyNumber_Divmod);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_divmod) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for divmod()");
}

/* Power (binary or ternary) */

static PyObject *
do_pow(v, w)
	PyObject *v, *w;
{
	PyObject *res;
	PyObject * (*f) Py_FPROTO((PyObject *, PyObject *, PyObject *));
	BINOP(v, w, "__pow__", "__rpow__", do_pow);
	if (v->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"pow(x, y) requires numeric arguments");
		return NULL;
	}
	if (
#ifndef WITHOUT_COMPLEX
            !PyComplex_Check(v) && 
#endif
            PyFloat_Check(w) && PyFloat_AsDouble(v) < 0.0) {
		if (!PyErr_Occurred())
		    PyErr_SetString(PyExc_ValueError,
				    "negative number to float power");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	if ((f = v->ob_type->tp_as_number->nb_power) != NULL)
		res = (*f)(v, w, Py_None);
	else
		res = type_error("pow(x, y) not defined for these operands");
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

PyObject *
PyNumber_Power(v, w, z)
	PyObject *v, *w, *z;
{
	PyObject *res;
	PyObject *v1, *z1, *w2, *z2;
	PyObject * (*f) Py_FPROTO((PyObject *, PyObject *, PyObject *));

	if (z == Py_None)
		return do_pow(v, w);
	/* XXX The ternary version doesn't do class instance coercions */
	if (PyInstance_Check(v))
		return v->ob_type->tp_as_number->nb_power(v, w, z);
	if (v->ob_type->tp_as_number == NULL ||
	    z->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		return type_error("pow(x, y, z) requires numeric arguments");
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
	if ((f = v1->ob_type->tp_as_number->nb_power) != NULL)
		res = (*f)(v1, w2, z2);
	else
		res = type_error(
			"pow(x, y, z) not defined for these operands");
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

/* Unary operators and functions */

PyObject *
PyNumber_Negative(o)
	PyObject *o;
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	m = o->ob_type->tp_as_number;
	if (m && m->nb_negative)
		return (*m->nb_negative)(o);

	return type_error("bad operand type for unary -");
}

PyObject *
PyNumber_Positive(o)
	PyObject *o;
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	m = o->ob_type->tp_as_number;
	if (m && m->nb_positive)
		return (*m->nb_positive)(o);

	return type_error("bad operand type for unary +");
}

PyObject *
PyNumber_Invert(o)
	PyObject *o;
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	m = o->ob_type->tp_as_number;
	if (m && m->nb_invert)
		return (*m->nb_invert)(o);

	return type_error("bad operand type for unary ~");
}

PyObject *
PyNumber_Absolute(o)
	PyObject *o;
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	m = o->ob_type->tp_as_number;
	if (m && m->nb_absolute)
		return m->nb_absolute(o);

	return type_error("bad operand type for abs()");
}

PyObject *
PyNumber_Int(o)
	PyObject *o;
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	if (PyString_Check(o))
		return int_from_string(o);
	m = o->ob_type->tp_as_number;
	if (m && m->nb_int)
		return m->nb_int(o);

	return type_error("object can't be converted to int");
}

PyObject *
PyNumber_Long(o)
	PyObject *o;
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	if (PyString_Check(o))
		return long_from_string(o);
	m = o->ob_type->tp_as_number;
	if (m && m->nb_long)
		return m->nb_long(o);

	return type_error("object can't be converted to long");
}

PyObject *
PyNumber_Float(o)
	PyObject *o;
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	if (PyString_Check(o))
		return float_from_string(o);
	m = o->ob_type->tp_as_number;
	if (m && m->nb_float)
		return m->nb_float(o);

	return type_error("object can't be converted to float");
}

/* Operations on sequences */

int
PySequence_Check(s)
	PyObject *s;
{
	return s != NULL && s->ob_type->tp_as_sequence;
}

int
PySequence_Length(s)
	PyObject *s;
{
	PySequenceMethods *m;

	if (s == NULL) {
		null_error();
		return -1;
	}

	m = s->ob_type->tp_as_sequence;
	if (m && m->sq_length)
		return m->sq_length(s);

	type_error("len() of unsized object");
	return -1;
}

PyObject *
PySequence_Concat(s, o)
	PyObject *s;
	PyObject *o;
{
	PySequenceMethods *m;

	if (s == NULL || o == NULL)
		return null_error();

	m = s->ob_type->tp_as_sequence;
	if (m && m->sq_concat)
		return m->sq_concat(s, o);

	return type_error("object can't be concatenated");
}

PyObject *
PySequence_Repeat(o, count)
	PyObject *o;
	int count;
{
	PySequenceMethods *m;

	if (o == NULL)
		return null_error();

	m = o->ob_type->tp_as_sequence;
	if (m && m->sq_repeat)
		return m->sq_repeat(o, count);

	return type_error("object can't be repeated");
}

PyObject *
PySequence_GetItem(s, i)
	PyObject *s;
	int i;
{
	PySequenceMethods *m;

	if (s == NULL)
		return null_error();

	m = s->ob_type->tp_as_sequence;
	if (m && m->sq_item) {
		if (i < 0) {
			if (m->sq_length) {
				int l = (*m->sq_length)(s);
				if (l < 0)
					return NULL;
				i += l;
			}
		}
		return m->sq_item(s, i);
	}

	return type_error("unindexable object");
}

PyObject *
PySequence_GetSlice(s, i1, i2)
	PyObject *s;
	int i1;
	int i2;
{
	PySequenceMethods *m;

	if (!s) return null_error();

	m = s->ob_type->tp_as_sequence;
	if (m && m->sq_slice) {
		if (i1 < 0 || i2 < 0) {
			if (m->sq_length) {
				int l = (*m->sq_length)(s);
				if (l < 0)
					return NULL;
				if (i1 < 0)
					i1 += l;
				if (i2 < 0)
					i2 += l;
			}
		}
		return m->sq_slice(s, i1, i2);
	}

	return type_error("unsliceable object");
}

int
PySequence_SetItem(s, i, o)
	PyObject *s;
	int i;
	PyObject *o;
{
	PySequenceMethods *m;

	if (s == NULL) {
		null_error();
		return -1;
	}

	m = s->ob_type->tp_as_sequence;
	if (m && m->sq_ass_item) {
		if (i < 0) {
			if (m->sq_length) {
				int l = (*m->sq_length)(s);
				if (l < 0)
					return -1;
				i += l;
			}
		}
		return m->sq_ass_item(s, i, o);
	}

	type_error("object doesn't support item assignment");
	return -1;
}

int
PySequence_DelItem(s, i)
	PyObject *s;
	int i;
{
	PySequenceMethods *m;

	if (s == NULL) {
		null_error();
		return -1;
	}

	m = s->ob_type->tp_as_sequence;
	if (m && m->sq_ass_item) {
		if (i < 0) {
			if (m->sq_length) {
				int l = (*m->sq_length)(s);
				if (l < 0)
					return -1;
				i += l;
			}
		}
		return m->sq_ass_item(s, i, (PyObject *)NULL);
	}

	type_error("object doesn't support item deletion");
	return -1;
}

int
PySequence_SetSlice(s, i1, i2, o)
	PyObject *s;
	int i1;
	int i2;
	PyObject *o;
{
	PySequenceMethods *m;

	if (s == NULL) {
		null_error();
		return -1;
	}

	m = s->ob_type->tp_as_sequence;
	if (m && m->sq_ass_slice) {
		if (i1 < 0 || i2 < 0) {
			if (m->sq_length) {
				int l = (*m->sq_length)(s);
				if (l < 0)
					return -1;
				if (i1 < 0)
					i1 += l;
				if (i2 < 0)
					i2 += l;
			}
		}
		return m->sq_ass_slice(s, i1, i2, o);
	}
	type_error("object doesn't support slice assignment");
	return -1;
}

int
PySequence_DelSlice(s, i1, i2)
	PyObject *s;
	int i1;
	int i2;
{
	PySequenceMethods *m;

	if (s == NULL) {
		null_error();
		return -1;
	}

	m = s->ob_type->tp_as_sequence;
	if (m && m->sq_ass_slice) {
		if (i1 < 0 || i2 < 0) {
			if (m->sq_length) {
				int l = (*m->sq_length)(s);
				if (l < 0)
					return -1;
				if (i1 < 0)
					i1 += l;
				if (i2 < 0)
					i2 += l;
			}
		}
		return m->sq_ass_slice(s, i1, i2, (PyObject *)NULL);
	}
	type_error("object doesn't support slice deletion");
	return -1;
}

PyObject *
PySequence_Tuple(v)
	PyObject *v;
{
	PySequenceMethods *m;

	if (v == NULL)
		return null_error();

	if (PyTuple_Check(v)) {
		Py_INCREF(v);
		return v;
	}

	if (PyList_Check(v))
		return PyList_AsTuple(v);

	/* There used to be code for strings here, but tuplifying strings is
	   not a common activity, so I nuked it.  Down with code bloat! */

	/* Generic sequence object */
	m = v->ob_type->tp_as_sequence;
	if (m && m->sq_item) {
		int i;
		PyObject *t;
		int n = PySequence_Length(v);
		if (n < 0)
			return NULL;
		t = PyTuple_New(n);
		if (t == NULL)
			return NULL;
		for (i = 0; ; i++) {
			PyObject *item = (*m->sq_item)(v, i);
			if (item == NULL) {
				if (PyErr_ExceptionMatches(PyExc_IndexError))
					PyErr_Clear();
				else {
					Py_DECREF(t);
					t = NULL;
				}
				break;
			}
			if (i >= n) {
				if (n < 500)
					n += 10;
				else
					n += 100;
				if (_PyTuple_Resize(&t, n, 0) != 0)
					break;
			}
			PyTuple_SET_ITEM(t, i, item);
		}
		if (i < n && t != NULL)
			_PyTuple_Resize(&t, i, 0);
		return t;
	}

	/* None of the above */
	return type_error("tuple() argument must be a sequence");
}

PyObject *
PySequence_List(v)
	PyObject *v;
{
	PySequenceMethods *m;

	if (v == NULL)
		return null_error();

	if (PyList_Check(v))
		return PyList_GetSlice(v, 0, PyList_GET_SIZE(v));

	m = v->ob_type->tp_as_sequence;
	if (m && m->sq_item) {
		int i;
		PyObject *l;
		int n = PySequence_Length(v);
		if (n < 0)
			return NULL;
		l = PyList_New(n);
		if (l == NULL)
			return NULL;
		for (i = 0; ; i++) {
			PyObject *item = (*m->sq_item)(v, i);
			if (item == NULL) {
				if (PyErr_ExceptionMatches(PyExc_IndexError))
					PyErr_Clear();
				else {
					Py_DECREF(l);
					l = NULL;
				}
				break;
			}
			if (i < n)
				PyList_SET_ITEM(l, i, item);
			else if (PyList_Append(l, item) < 0) {
				Py_DECREF(l);
				l = NULL;
				break;
			}
		}
		if (i < n && l != NULL) {
			if (PyList_SetSlice(l, i, n, (PyObject *)NULL) != 0) {
				Py_DECREF(l);
				l = NULL;
			}
		}
		return l;
	}
	return type_error("list() argument must be a sequence");
}

int
PySequence_Count(s, o)
	PyObject *s;
	PyObject *o;
{
	int l, i, n, cmp, err;
	PyObject *item;

	if (s == NULL || o == NULL) {
		null_error();
		return -1;
	}
	
	l = PySequence_Length(s);
	if (l < 0)
		return -1;

	n = 0;
	for (i = 0; i < l; i++) {
		item = PySequence_GetItem(s, i);
		if (item == NULL)
			return -1;
		err = PyObject_Cmp(item, o, &cmp);
		Py_DECREF(item);
		if (err < 0)
			return err;
		if (cmp == 0)
			n++;
	}
	return n;
}

int
PySequence_Contains(w, v) /* v in w */
	PyObject *w;
	PyObject *v;
{
	int i, cmp;
	PyObject *x;
	PySequenceMethods *sq;

	/* Special case for char in string */
	if (PyString_Check(w)) {
		register char *s, *end;
		register char c;
		if (!PyString_Check(v) || PyString_Size(v) != 1) {
			PyErr_SetString(PyExc_TypeError,
			    "string member test needs char left operand");
			return -1;
		}
		c = PyString_AsString(v)[0];
		s = PyString_AsString(w);
		end = s + PyString_Size(w);
		while (s < end) {
			if (c == *s++)
				return 1;
		}
		return 0;
	}

	sq = w->ob_type->tp_as_sequence;
	if (sq == NULL || sq->sq_item == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"'in' or 'not in' needs sequence right argument");
		return -1;
	}

	for (i = 0; ; i++) {
		x = (*sq->sq_item)(w, i);
		if (x == NULL) {
			if (PyErr_ExceptionMatches(PyExc_IndexError)) {
				PyErr_Clear();
				break;
			}
			return -1;
		}
		cmp = PyObject_Compare(v, x);
		Py_XDECREF(x);
		if (cmp == 0)
			return 1;
		if (PyErr_Occurred())
			return -1;
	}

	return 0;
}

/* Backwards compatibility */
#undef PySequence_In
int
PySequence_In(w, v)
	PyObject *w;
	PyObject *v;
{
	return PySequence_Contains(w, v);
}

int
PySequence_Index(s, o)
	PyObject *s;
	PyObject *o;
{
	int l, i, cmp, err;
	PyObject *item;

	if (s == NULL || o == NULL) {
		null_error();
		return -1;
	}
	
	l = PySequence_Length(s);
	if (l < 0)
		return -1;

	for (i = 0; i < l; i++) {
		item = PySequence_GetItem(s, i);
		if (item == NULL)
			return -1;
		err = PyObject_Cmp(item, o, &cmp);
		Py_DECREF(item);
		if (err < 0)
			return err;
		if (cmp == 0)
			return i;
	}

	PyErr_SetString(PyExc_ValueError, "sequence.index(x): x not in list");
	return -1;
}

/* Operations on mappings */

int
PyMapping_Check(o)
	PyObject *o;
{
	return o && o->ob_type->tp_as_mapping;
}

int
PyMapping_Length(o)
	PyObject *o;
{
	PyMappingMethods *m;

	if (o == NULL) {
		null_error();
		return -1;
	}

	m = o->ob_type->tp_as_mapping;
	if (m && m->mp_length)
		return m->mp_length(o);

	type_error("len() of unsized object");
	return -1;
}

PyObject *
PyMapping_GetItemString(o, key)
	PyObject *o;
	char *key;
{
	PyObject *okey, *r;

	if (key == NULL)
		return null_error();

	okey = PyString_FromString(key);
	if (okey == NULL)
		return NULL;
	r = PyObject_GetItem(o, okey);
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

	if (key == NULL) {
		null_error();
		return -1;
	}

	okey = PyString_FromString(key);
	if (okey == NULL)
		return -1;
	r = PyObject_SetItem(o, okey, value);
	Py_DECREF(okey);
	return r;
}

int
PyMapping_HasKeyString(o, key)
	PyObject *o;
	char *key;
{
	PyObject *v;

	v = PyMapping_GetItemString(o, key);
	if (v) {
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

	v = PyObject_GetItem(o, key);
	if (v) {
		Py_DECREF(v);
		return 1;
	}
	PyErr_Clear();
	return 0;
}

/* Operations on callable objects */

/* XXX PyCallable_Check() is in object.c */

PyObject *
PyObject_CallObject(o, a)
	PyObject *o, *a;
{
	PyObject *r;
	PyObject *args = a;

	if (args == NULL) {
		args = PyTuple_New(0);
		if (args == NULL)
			return NULL;
	}

	r = PyEval_CallObject(o, args);

	if (args != a) {
		Py_DECREF(args);
	}

	return r;
}

PyObject *
#ifdef HAVE_STDARG_PROTOTYPES
/* VARARGS 2 */
PyObject_CallFunction(PyObject *callable, char *format, ...)
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
	PyObject *callable;
	char *format;
	va_start(va);
	callable = va_arg(va, PyObject *);
	format   = va_arg(va, char *);
#endif

	if (callable == NULL) {
		va_end(va);
		return null_error();
	}

	if (format)
		args = Py_VaBuildValue(format, va);
	else
		args = PyTuple_New(0);

	va_end(va);
	
	if (args == NULL)
		return NULL;

	if (!PyTuple_Check(args)) {
		PyObject *a;

		a = PyTuple_New(1);
		if (a == NULL)
			return NULL;
		if (PyTuple_SetItem(a, 0, args) < 0)
			return NULL;
		args = a;
	}
	retval = PyObject_CallObject(callable, args);

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
	PyObject *args, *func = 0, *retval;
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

	if (o == NULL || name == NULL) {
		va_end(va);
		return null_error();
	}

	func = PyObject_GetAttrString(o, name);
	if (func == NULL) {
		va_end(va);
		PyErr_SetString(PyExc_AttributeError, name);
		return 0;
	}

	if (!PyCallable_Check(func)) {
		va_end(va);
		return type_error("call of non-callable attribute");
	}

	if (format && *format)
		args = Py_VaBuildValue(format, va);
	else
		args = PyTuple_New(0);

	va_end(va);

	if (!args)
		return NULL;

	if (!PyTuple_Check(args)) {
		PyObject *a;

		a = PyTuple_New(1);
		if (a == NULL)
			return NULL;
		if (PyTuple_SetItem(a, 0, args) < 0)
			return NULL;
		args = a;
	}

	retval = PyObject_CallObject(func, args);

	Py_DECREF(args);
	Py_DECREF(func);

	return retval;
}
