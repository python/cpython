/* Abstract Object Interface (many thanks to Jim Fulton) */

#include "Python.h"
#include <ctype.h>

/* Shorthands to return certain errors */

static PyObject *
type_error(const char *msg)
{
	PyErr_SetString(PyExc_TypeError, msg);
	return NULL;
}

static PyObject *
null_error(void)
{
	if (!PyErr_Occurred())
		PyErr_SetString(PyExc_SystemError,
				"null argument to internal routine");
	return NULL;
}

/* Operations on any object */

int
PyObject_Cmp(PyObject *o1, PyObject *o2, int *result)
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
PyObject_Type(PyObject *o)
{
	PyObject *v;

	if (o == NULL)
		return null_error();
	v = (PyObject *)o->ob_type;
	Py_INCREF(v);
	return v;
}

int
PyObject_Size(PyObject *o)
{
	PySequenceMethods *m;

	if (o == NULL) {
		null_error();
		return -1;
	}

	m = o->ob_type->tp_as_sequence;
	if (m && m->sq_length)
		return m->sq_length(o);

	return PyMapping_Size(o);
}

#undef PyObject_Length
int
PyObject_Length(PyObject *o)
{
	return PyObject_Size(o);
}
#define PyObject_Length PyObject_Size

PyObject *
PyObject_GetItem(PyObject *o, PyObject *key)
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
		else if (PyLong_Check(key)) {
			long key_value = PyLong_AsLong(key);
			if (key_value == -1 && PyErr_Occurred())
				return NULL;
			return PySequence_GetItem(o, key_value);
		}
		return type_error("sequence index must be integer");
	}

	return type_error("unsubscriptable object");
}

int
PyObject_SetItem(PyObject *o, PyObject *key, PyObject *value)
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
		else if (PyLong_Check(key)) {
			long key_value = PyLong_AsLong(key);
			if (key_value == -1 && PyErr_Occurred())
				return -1;
			return PySequence_SetItem(o, key_value, value);
		}
		type_error("sequence index must be integer");
		return -1;
	}

	type_error("object does not support item assignment");
	return -1;
}

int
PyObject_DelItem(PyObject *o, PyObject *key)
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
		else if (PyLong_Check(key)) {
			long key_value = PyLong_AsLong(key);
			if (key_value == -1 && PyErr_Occurred())
				return -1;
			return PySequence_DelItem(o, key_value);
		}
		type_error("sequence index must be integer");
		return -1;
	}

	type_error("object does not support item deletion");
	return -1;
}

int PyObject_AsCharBuffer(PyObject *obj,
			  const char **buffer,
			  int *buffer_len)
{
	PyBufferProcs *pb;
	const char *pp;
	int len;

	if (obj == NULL || buffer == NULL || buffer_len == NULL) {
		null_error();
		return -1;
	}
	pb = obj->ob_type->tp_as_buffer;
	if ( pb == NULL ||
	     pb->bf_getcharbuffer == NULL ||
	     pb->bf_getsegcount == NULL ) {
		PyErr_SetString(PyExc_TypeError,
				"expected a character buffer object");
		goto onError;
	}
	if ( (*pb->bf_getsegcount)(obj,NULL) != 1 ) {
		PyErr_SetString(PyExc_TypeError,
				"expected a single-segment buffer object");
		goto onError;
	}
	len = (*pb->bf_getcharbuffer)(obj,0,&pp);
	if (len < 0)
		goto onError;
	*buffer = pp;
	*buffer_len = len;
	return 0;

 onError:
	return -1;
}

int PyObject_AsReadBuffer(PyObject *obj,
			  const void **buffer,
			  int *buffer_len)
{
	PyBufferProcs *pb;
	void *pp;
	int len;

	if (obj == NULL || buffer == NULL || buffer_len == NULL) {
		null_error();
		return -1;
	}
	pb = obj->ob_type->tp_as_buffer;
	if ( pb == NULL ||
	     pb->bf_getreadbuffer == NULL ||
	     pb->bf_getsegcount == NULL ) {
		PyErr_SetString(PyExc_TypeError,
				"expected a readable buffer object");
		goto onError;
	}
	if ( (*pb->bf_getsegcount)(obj,NULL) != 1 ) {
		PyErr_SetString(PyExc_TypeError,
				"expected a single-segment buffer object");
		goto onError;
	}
	len = (*pb->bf_getreadbuffer)(obj,0,&pp);
	if (len < 0)
		goto onError;
	*buffer = pp;
	*buffer_len = len;
	return 0;

 onError:
	return -1;
}

int PyObject_AsWriteBuffer(PyObject *obj,
			   void **buffer,
			   int *buffer_len)
{
	PyBufferProcs *pb;
	void*pp;
	int len;

	if (obj == NULL || buffer == NULL || buffer_len == NULL) {
		null_error();
		return -1;
	}
	pb = obj->ob_type->tp_as_buffer;
	if ( pb == NULL ||
	     pb->bf_getwritebuffer == NULL ||
	     pb->bf_getsegcount == NULL ) {
		PyErr_SetString(PyExc_TypeError,
				"expected a writeable buffer object");
		goto onError;
	}
	if ( (*pb->bf_getsegcount)(obj,NULL) != 1 ) {
		PyErr_SetString(PyExc_TypeError,
				"expected a single-segment buffer object");
		goto onError;
	}
	len = (*pb->bf_getwritebuffer)(obj,0,&pp);
	if (len < 0)
		goto onError;
	*buffer = pp;
	*buffer_len = len;
	return 0;

 onError:
	return -1;
}

/* Operations on numbers */

int
PyNumber_Check(PyObject *o)
{
	return o && o->ob_type->tp_as_number;
}

/* Binary operators */

#define BINOP(v, w, opname, ropname, thisfunc) \
	if (PyInstance_Check(v) || PyInstance_Check(w)) \
		return PyInstance_DoBinOp(v, w, opname, ropname, thisfunc)

PyObject *
PyNumber_Or(PyObject *v, PyObject *w)
{
	BINOP(v, w, "__or__", "__ror__", PyNumber_Or);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_or) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for |");
}

PyObject *
PyNumber_Xor(PyObject *v, PyObject *w)
{
	BINOP(v, w, "__xor__", "__rxor__", PyNumber_Xor);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_xor) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for ^");
}

PyObject *
PyNumber_And(PyObject *v, PyObject *w)
{
	BINOP(v, w, "__and__", "__rand__", PyNumber_And);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_and) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for &");
}

PyObject *
PyNumber_Lshift(PyObject *v, PyObject *w)
{
	BINOP(v, w, "__lshift__", "__rlshift__", PyNumber_Lshift);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_lshift) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for <<");
}

PyObject *
PyNumber_Rshift(PyObject *v, PyObject *w)
{
	BINOP(v, w, "__rshift__", "__rrshift__", PyNumber_Rshift);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_rshift) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for >>");
}

PyObject *
PyNumber_Add(PyObject *v, PyObject *w)
{
	PySequenceMethods *m;

	BINOP(v, w, "__add__", "__radd__", PyNumber_Add);
	m = v->ob_type->tp_as_sequence;
	if (m && m->sq_concat)
		return (*m->sq_concat)(v, w);
	else if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_add) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for +");
}

PyObject *
PyNumber_Subtract(PyObject *v, PyObject *w)
{
	BINOP(v, w, "__sub__", "__rsub__", PyNumber_Subtract);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_subtract) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for -");
}

PyObject *
PyNumber_Multiply(PyObject *v, PyObject *w)
{
	PyTypeObject *tp = v->ob_type;
	PySequenceMethods *m;

	BINOP(v, w, "__mul__", "__rmul__", PyNumber_Multiply);
	if (tp->tp_as_number != NULL &&
	    w->ob_type->tp_as_sequence != NULL) {
		/* number*sequence -- swap v and w */
		PyObject *tmp = v;
		v = w;
		w = tmp;
		tp = v->ob_type;
	}
	if (tp->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_multiply) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	m = tp->tp_as_sequence;
	if (m && m->sq_repeat) {
		long mul_value;

		if (PyInt_Check(w)) {
			mul_value = PyInt_AsLong(w);
		}
		else if (PyLong_Check(w)) {
			mul_value = PyLong_AsLong(w);
			if (mul_value == -1 && PyErr_Occurred())
                                return NULL; 
		}
		else {
			return type_error(
				"can't multiply sequence with non-int");
		}
		return (*m->sq_repeat)(v, (int)mul_value);
	}
	return type_error("bad operand type(s) for *");
}

PyObject *
PyNumber_Divide(PyObject *v, PyObject *w)
{
	BINOP(v, w, "__div__", "__rdiv__", PyNumber_Divide);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_divide) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for /");
}

PyObject *
PyNumber_Remainder(PyObject *v, PyObject *w)
{
	if (PyString_Check(v))
		return PyString_Format(v, w);
	else if (PyUnicode_Check(v))
		return PyUnicode_Format(v, w);
	BINOP(v, w, "__mod__", "__rmod__", PyNumber_Remainder);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_remainder) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	return type_error("bad operand type(s) for %");
}

PyObject *
PyNumber_Divmod(PyObject *v, PyObject *w)
{
	BINOP(v, w, "__divmod__", "__rdivmod__", PyNumber_Divmod);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f)(PyObject *, PyObject *) = NULL;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_divmod) != NULL)
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
do_pow(PyObject *v, PyObject *w)
{
	PyObject *res;
	PyObject * (*f)(PyObject *, PyObject *, PyObject *);
	BINOP(v, w, "__pow__", "__rpow__", do_pow);
	if (v->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"pow(x, y) requires numeric arguments");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	if (v->ob_type->tp_as_number != NULL &&
	    (f = v->ob_type->tp_as_number->nb_power) != NULL)
		res = (*f)(v, w, Py_None);
	else
		res = type_error("pow(x, y) not defined for these operands");
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

PyObject *
PyNumber_Power(PyObject *v, PyObject *w, PyObject *z)
{
	PyObject *res;
	PyObject *v1, *z1, *w2, *z2;
	PyObject * (*f)(PyObject *, PyObject *, PyObject *);

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
	if (v->ob_type->tp_as_number != NULL &&
	    (f = v1->ob_type->tp_as_number->nb_power) != NULL)
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

/* Binary in-place operators */

/* The in-place operators are defined to fall back to the 'normal',
   non in-place operations, if the in-place methods are not in place, and to
   take class instances into account. This is how it is supposed to work:

   - If the left-hand-side object (the first argument) is an
     instance object, try to let PyInstance_HalfBinOp() handle it.  Pass the
     non in-place variant of the function as callback, because it will only
     be used if the left-hand object is changed by coercion.

   - Otherwise, if the left hand object is not an instance object, it has
     the appropriate struct members, and they are filled, call the
     appropriate function and return the result. No coercion is done on the
     arguments; the left-hand object is the one the operation is performed
     on, and it's up to the function to deal with the right-hand object.
     
   - Otherwise, in-place modification is not supported. Handle it exactly as
     a non in-place operation of the same kind:

     - If either object is an instance, let PyInstance_DoBinOp() handle it.
     
     - Otherwise, both arguments are C types. If the left-hand object has
       the appropriate struct members filled, coerce, call the
       appropriate function, and return the result.
  
     - Otherwise, we are out of options: raise a type error specific to
       augmented assignment.

   */

#define HASINPLACE(t) PyType_HasFeature((t)->ob_type, Py_TPFLAGS_HAVE_INPLACEOPS)

PyObject *
PyNumber_InPlaceOr(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__ior__", &x,
					 PyNumber_Or, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_or) != NULL)
		return (*f)(v, w);

	BINOP(v, w, "__or__", "__ror__", PyNumber_Or);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_or) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for |=");
}

PyObject *
PyNumber_InPlaceXor(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__ixor__", &x,
					 PyNumber_Xor, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_xor) != NULL)
		return (*f)(v, w);

	BINOP(v, w, "__xor__", "__rxor__", PyNumber_Xor);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_xor) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for ^=");
}

PyObject *
PyNumber_InPlaceAnd(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__iand__", &x,
					 PyNumber_And, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_and) != NULL)
		return (*f)(v, w);

	BINOP(v, w, "__and__", "__rand__", PyNumber_And);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_and) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for &=");
}

PyObject *
PyNumber_InPlaceLshift(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__ilshift__", &x,
					PyNumber_Lshift, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_lshift) != NULL)
		return (*f)(v, w);

	BINOP(v, w, "__lshift__", "__rlshift__", PyNumber_Lshift);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_lshift) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for <<=");
}

PyObject *
PyNumber_InPlaceRshift(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__irshift__", &x,
					PyNumber_Rshift, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_rshift) != NULL)
		return (*f)(v, w);

	BINOP(v, w, "__rshift__", "__rrshift__", PyNumber_Rshift);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_rshift) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for >>=");
}

PyObject *
PyNumber_InPlaceAdd(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__iadd__", &x,
					 PyNumber_Add, 0) <= 0)
			return x;
	}
	else if (HASINPLACE(v)) {
		if (v->ob_type->tp_as_sequence != NULL)
			f = v->ob_type->tp_as_sequence->sq_inplace_concat;
		if (f == NULL && v->ob_type->tp_as_number != NULL)
			f = v->ob_type->tp_as_number->nb_inplace_add;
		if (f != NULL)
			return (*f)(v, w);
	}

	BINOP(v, w, "__add__", "__radd__", PyNumber_Add);

	if (v->ob_type->tp_as_sequence != NULL) {
		f = v->ob_type->tp_as_sequence->sq_concat;
		if (f != NULL)
			return (*f)(v, w);
	}
	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL) {
			f = v->ob_type->tp_as_number->nb_add;
			if (f != NULL)
				x = (*f)(v, w);
		}
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for +=");
}

PyObject *
PyNumber_InPlaceSubtract(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__isub__", &x,
					PyNumber_Subtract, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_subtract) != NULL)
		return (*f)(v, w);

	BINOP(v, w, "__sub__", "__rsub__", PyNumber_Subtract);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_subtract) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for -=");
}

PyObject *
PyNumber_InPlaceMultiply(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject * (*g)(PyObject *, int) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__imul__", &x,
					PyNumber_Multiply, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_multiply) != NULL)
		return (*f)(v, w);
	else if (v->ob_type->tp_as_sequence != NULL && HASINPLACE(v) &&
		 (g = v->ob_type->tp_as_sequence->sq_inplace_repeat) != NULL) {
		long mul_value;

		if (PyInt_Check(w)) {
			mul_value = PyInt_AsLong(w);
		}
		else if (PyLong_Check(w)) {
			mul_value = PyLong_AsLong(w);
			if (mul_value == -1 && PyErr_Occurred())
                                return NULL; 
		}
		else {
			return type_error(
				"can't multiply sequence with non-int");
		}
		return (*g)(v, (int)mul_value);
	}

	BINOP(v, w, "__mul__", "__rmul__", PyNumber_Multiply);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_multiply) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	else if (v->ob_type->tp_as_sequence != NULL &&
		 (g = v->ob_type->tp_as_sequence->sq_repeat) != NULL) {
		long mul_value;

		if (PyInt_Check(w)) {
			mul_value = PyInt_AsLong(w);
		}
		else if (PyLong_Check(w)) {
			mul_value = PyLong_AsLong(w);
			if (mul_value == -1 && PyErr_Occurred())
                                return NULL; 
		}
		else {
			return type_error(
				"can't multiply sequence with non-int");
		}
		return (*g)(v, (int)mul_value);
	}
	return type_error("bad operand type(s) for *=");
}

PyObject *
PyNumber_InPlaceDivide(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__idiv__", &x,
					PyNumber_Divide, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_divide) != NULL)
		return (*f)(v, w);

	BINOP(v, w, "__div__", "__rdiv__", PyNumber_Divide);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if (v->ob_type->tp_as_number != NULL &&
		    (f = v->ob_type->tp_as_number->nb_divide) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for /=");
}

PyObject *
PyNumber_InPlaceRemainder(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__imod__", &x,
					PyNumber_Remainder, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_remainder) != NULL)
		return (*f)(v, w);

	if (PyString_Check(v))
		return PyString_Format(v, w);
	else if (PyUnicode_Check(v))
		return PyUnicode_Format(v, w);

	BINOP(v, w, "__mod__", "__rmod__", PyNumber_Remainder);

	if (v->ob_type->tp_as_number != NULL) {
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_remainder) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}

	return type_error("bad operand type(s) for %=");
}


/* In-place Power (binary or ternary, for API consistency) */

static PyObject *
do_inplace_pow(PyObject *v, PyObject *w)
{
	PyObject * (*f)(PyObject *, PyObject *, PyObject *) = NULL;
	PyObject *x = NULL;

	if (PyInstance_Check(v)) {
		if (PyInstance_HalfBinOp(v, w, "__ipow__", &x, do_pow, 0) <= 0)
			return x;
	}
	else if (v->ob_type->tp_as_number != NULL && HASINPLACE(v) &&
		 (f = v->ob_type->tp_as_number->nb_inplace_power) != NULL)
		return (*f)(v, w, Py_None);

	BINOP(v, w, "__pow__", "__rpow__", do_pow);

	if (v->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		return type_error("bad operand type(s) for **=");
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	if ((f = v->ob_type->tp_as_number->nb_power) != NULL)
		x = (*f)(v, w, Py_None);
	else
		x = type_error("bad operand type(s) for **=");
	Py_DECREF(v);
	Py_DECREF(w);
	return x;
}

PyObject *
PyNumber_InPlacePower(PyObject *v, PyObject *w, PyObject *z)
{
	PyObject *res;
	PyObject *v1, *z1, *w2, *z2, *oldv;
	PyObject * (*f)(PyObject *, PyObject *, PyObject *);

	if (z == Py_None)
		return do_inplace_pow(v, w);
	/* XXX The ternary version doesn't do class instance coercions */
	if (PyInstance_Check(v))
		return v->ob_type->tp_as_number->nb_inplace_power(v, w, z);
	if (v->ob_type->tp_as_number == NULL ||
	    z->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		return type_error(
			"(inplace) pow(x, y, z) requires numeric arguments");
	}
	oldv = v;
	Py_INCREF(oldv);
	res = NULL;
	if (PyNumber_Coerce(&v, &w) != 0)
		goto error3;
	v1 = v;
	z1 = z;
	if (PyNumber_Coerce(&v1, &z1) != 0)
		goto error2;
	w2 = w;
	z2 = z1;
 	if (PyNumber_Coerce(&w2, &z2) != 0)
		goto error1;
	if (oldv == v1 && HASINPLACE(v1) &&
	    v->ob_type->tp_as_number != NULL &&
	    (f = v1->ob_type->tp_as_number->nb_inplace_power) != NULL)
		res = (*f)(v1, w2, z2);
	else if (v1->ob_type->tp_as_number != NULL &&
		 (f = v1->ob_type->tp_as_number->nb_power) != NULL)
		res = (*f)(v1, w2, z2);
	else
		res = type_error(
		     "(inplace) pow(x, y, z) not defined for these operands");
	Py_DECREF(w2);
	Py_DECREF(z2);
  error1:
	Py_DECREF(v1);
	Py_DECREF(z1);
  error2:
	Py_DECREF(v);
	Py_DECREF(w);
  error3:
	Py_DECREF(oldv);
	return res;
}


/* Unary operators and functions */

PyObject *
PyNumber_Negative(PyObject *o)
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
PyNumber_Positive(PyObject *o)
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
PyNumber_Invert(PyObject *o)
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
PyNumber_Absolute(PyObject *o)
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	m = o->ob_type->tp_as_number;
	if (m && m->nb_absolute)
		return m->nb_absolute(o);

	return type_error("bad operand type for abs()");
}

/* Add a check for embedded NULL-bytes in the argument. */
static PyObject *
int_from_string(const char *s, int len)
{
	char *end;
	PyObject *x;

	x = PyInt_FromString((char*)s, &end, 10);
	if (x == NULL)
		return NULL;
	if (end != s + len) {
		PyErr_SetString(PyExc_ValueError,
				"null byte in argument for int()");
		Py_DECREF(x);
		return NULL;
	}
	return x;
}

PyObject *
PyNumber_Int(PyObject *o)
{
	PyNumberMethods *m;
	const char *buffer;
	int buffer_len;

	if (o == NULL)
		return null_error();
	if (PyInt_Check(o)) {
		Py_INCREF(o);
		return o;
	}
	if (PyString_Check(o))
		return int_from_string(PyString_AS_STRING(o), 
				       PyString_GET_SIZE(o));
	if (PyUnicode_Check(o))
		return PyInt_FromUnicode(PyUnicode_AS_UNICODE(o),
					 PyUnicode_GET_SIZE(o),
					 10);
	m = o->ob_type->tp_as_number;
	if (m && m->nb_int)
		return m->nb_int(o);
	if (!PyObject_AsCharBuffer(o, &buffer, &buffer_len))
		return int_from_string((char*)buffer, buffer_len);

	return type_error("object can't be converted to int");
}

/* Add a check for embedded NULL-bytes in the argument. */
static PyObject *
long_from_string(const char *s, int len)
{
	char *end;
	PyObject *x;

	x = PyLong_FromString((char*)s, &end, 10);
	if (x == NULL)
		return NULL;
	if (end != s + len) {
		PyErr_SetString(PyExc_ValueError,
				"null byte in argument for long()");
		Py_DECREF(x);
		return NULL;
	}
	return x;
}

PyObject *
PyNumber_Long(PyObject *o)
{
	PyNumberMethods *m;
	const char *buffer;
	int buffer_len;

	if (o == NULL)
		return null_error();
	if (PyLong_Check(o)) {
		Py_INCREF(o);
		return o;
	}
	if (PyString_Check(o))
		/* need to do extra error checking that PyLong_FromString() 
		 * doesn't do.  In particular long('9.5') must raise an
		 * exception, not truncate the float.
		 */
		return long_from_string(PyString_AS_STRING(o),
					PyString_GET_SIZE(o));
	if (PyUnicode_Check(o))
		/* The above check is done in PyLong_FromUnicode(). */
		return PyLong_FromUnicode(PyUnicode_AS_UNICODE(o),
					  PyUnicode_GET_SIZE(o),
					  10);
	m = o->ob_type->tp_as_number;
	if (m && m->nb_long)
		return m->nb_long(o);
	if (!PyObject_AsCharBuffer(o, &buffer, &buffer_len))
		return long_from_string(buffer, buffer_len);

	return type_error("object can't be converted to long");
}

PyObject *
PyNumber_Float(PyObject *o)
{
	PyNumberMethods *m;

	if (o == NULL)
		return null_error();
	if (PyFloat_Check(o)) {
		Py_INCREF(o);
		return o;
	}
	if (!PyString_Check(o)) {
		m = o->ob_type->tp_as_number;
		if (m && m->nb_float)
			return m->nb_float(o);
	}
	return PyFloat_FromString(o, NULL);
}

/* Operations on sequences */

int
PySequence_Check(PyObject *s)
{
	return s != NULL && s->ob_type->tp_as_sequence;
}

int
PySequence_Size(PyObject *s)
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

#undef PySequence_Length
int
PySequence_Length(PyObject *s)
{
	return PySequence_Size(s);
}
#define PySequence_Length PySequence_Size

PyObject *
PySequence_Concat(PyObject *s, PyObject *o)
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
PySequence_Repeat(PyObject *o, int count)
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
PySequence_InPlaceConcat(PyObject *s, PyObject *o)
{
	PySequenceMethods *m;

	if (s == NULL || o == NULL)
		return null_error();

	m = s->ob_type->tp_as_sequence;
	if (m && HASINPLACE(s) && m->sq_inplace_concat)
		return m->sq_inplace_concat(s, o);
	if (m && m->sq_concat)
		return m->sq_concat(s, o);

	return type_error("object can't be concatenated");
}

PyObject *
PySequence_InPlaceRepeat(PyObject *o, int count)
{
	PySequenceMethods *m;

	if (o == NULL)
		return null_error();

	m = o->ob_type->tp_as_sequence;
	if (m && HASINPLACE(o) && m->sq_inplace_repeat)
		return m->sq_inplace_repeat(o, count);
	if (m && m->sq_repeat)
		return m->sq_repeat(o, count);

	return type_error("object can't be repeated");
}

PyObject *
PySequence_GetItem(PyObject *s, int i)
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

static PyObject *
sliceobj_from_intint(int i, int j)
{
	PyObject *start, *end, *slice;
	start = PyInt_FromLong((long)i);
	if (!start)
		return NULL;
	end = PyInt_FromLong((long)j);
	if (!end) {
		Py_DECREF(start);
		return NULL;
	}
	slice = PySlice_New(start, end, NULL);
	Py_DECREF(start);
	Py_DECREF(end);
	return slice;
}

PyObject *
PySequence_GetSlice(PyObject *s, int i1, int i2)
{
	PySequenceMethods *m;
	PyMappingMethods *mp;

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
	} else if ((mp = s->ob_type->tp_as_mapping) && mp->mp_subscript) {
		PyObject *res;
		PyObject *slice = sliceobj_from_intint(i1, i2);
		if (!slice)
			return NULL;
		res = mp->mp_subscript(s, slice);
		Py_DECREF(slice);
		return res;
	}

	return type_error("unsliceable object");
}

int
PySequence_SetItem(PyObject *s, int i, PyObject *o)
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
PySequence_DelItem(PyObject *s, int i)
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
PySequence_SetSlice(PyObject *s, int i1, int i2, PyObject *o)
{
	PySequenceMethods *m;
	PyMappingMethods *mp;

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
	} else if ((mp = s->ob_type->tp_as_mapping) && mp->mp_ass_subscript) {
		int res;
		PyObject *slice = sliceobj_from_intint(i1, i2);
		if (!slice)
			return -1;
		res = mp->mp_ass_subscript(s, slice, o);
		Py_DECREF(slice);
		return res;
	}

	type_error("object doesn't support slice assignment");
	return -1;
}

int
PySequence_DelSlice(PyObject *s, int i1, int i2)
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
PySequence_Tuple(PyObject *v)
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
		int n = PySequence_Size(v);
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
PySequence_List(PyObject *v)
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
		int n = PySequence_Size(v);
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

PyObject *
PySequence_Fast(PyObject *v, const char *m)
{
	if (v == NULL)
		return null_error();

	if (PyList_Check(v) || PyTuple_Check(v)) {
		Py_INCREF(v);
		return v;
	}

	v = PySequence_Tuple(v);
	if (v == NULL && PyErr_ExceptionMatches(PyExc_TypeError))
		return type_error(m);

	return v;
}

int
PySequence_Count(PyObject *s, PyObject *o)
{
	int l, i, n, cmp, err;
	PyObject *item;

	if (s == NULL || o == NULL) {
		null_error();
		return -1;
	}
	
	l = PySequence_Size(s);
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
PySequence_Contains(PyObject *w, PyObject *v) /* v in w */
{
	int i, cmp;
	PyObject *x;
	PySequenceMethods *sq;

	if(PyType_HasFeature(w->ob_type, Py_TPFLAGS_HAVE_SEQUENCE_IN)) {
		sq = w->ob_type->tp_as_sequence;
	        if(sq != NULL && sq->sq_contains != NULL)
			return (*sq->sq_contains)(w, v);
	}
	
	/* If there is no better way to check whether an item is is contained,
	   do it the hard way */
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
PySequence_In(PyObject *w, PyObject *v)
{
	return PySequence_Contains(w, v);
}

int
PySequence_Index(PyObject *s, PyObject *o)
{
	int l, i, cmp, err;
	PyObject *item;

	if (s == NULL || o == NULL) {
		null_error();
		return -1;
	}
	
	l = PySequence_Size(s);
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
PyMapping_Check(PyObject *o)
{
	return o && o->ob_type->tp_as_mapping;
}

int
PyMapping_Size(PyObject *o)
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

#undef PyMapping_Length
int
PyMapping_Length(PyObject *o)
{
	return PyMapping_Size(o);
}
#define PyMapping_Length PyMapping_Size

PyObject *
PyMapping_GetItemString(PyObject *o, char *key)
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
PyMapping_SetItemString(PyObject *o, char *key, PyObject *value)
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
PyMapping_HasKeyString(PyObject *o, char *key)
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
PyMapping_HasKey(PyObject *o, PyObject *key)
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
PyObject_CallObject(PyObject *o, PyObject *a)
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
PyObject_CallFunction(PyObject *callable, char *format, ...)
{
	va_list va;
	PyObject *args, *retval;
	va_start(va, format);

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
PyObject_CallMethod(PyObject *o, char *name, char *format, ...)
{
	va_list va;
	PyObject *args, *func = 0, *retval;
	va_start(va, format);

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
