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

/* Generic object operations; and implementation of None (NoObject) */

#include "Python.h"

#if defined( Py_TRACE_REFS ) || defined( Py_REF_DEBUG )
long _Py_RefTotal;
#endif

/* Object allocation routines used by NEWOBJ and NEWVAROBJ macros.
   These are used by the individual routines for object creation.
   Do not call them otherwise, they do not initialize the object! */

#ifdef COUNT_ALLOCS
static PyTypeObject *type_list;
extern int tuple_zero_allocs, fast_tuple_allocs;
extern int quick_int_allocs, quick_neg_int_allocs;
extern int null_strings, one_strings;
void
dump_counts()
{
	PyTypeObject *tp;

	for (tp = type_list; tp; tp = tp->tp_next)
		fprintf(stderr, "%s alloc'd: %d, freed: %d, max in use: %d\n",
			tp->tp_name, tp->tp_alloc, tp->tp_free,
			tp->tp_maxalloc);
	fprintf(stderr, "fast tuple allocs: %d, empty: %d\n",
		fast_tuple_allocs, tuple_zero_allocs);
	fprintf(stderr, "fast int allocs: pos: %d, neg: %d\n",
		quick_int_allocs, quick_neg_int_allocs);
	fprintf(stderr, "null strings: %d, 1-strings: %d\n",
		null_strings, one_strings);
}

PyObject *
get_counts()
{
	PyTypeObject *tp;
	PyObject *result;
	PyObject *v;

	result = PyList_New(0);
	if (result == NULL)
		return NULL;
	for (tp = type_list; tp; tp = tp->tp_next) {
		v = Py_BuildValue("(siii)", tp->tp_name, tp->tp_alloc,
				  tp->tp_free, tp->tp_maxalloc);
		if (v == NULL) {
			Py_DECREF(result);
			return NULL;
		}
		if (PyList_Append(result, v) < 0) {
			Py_DECREF(v);
			Py_DECREF(result);
			return NULL;
		}
		Py_DECREF(v);
	}
	return result;
}

void
inc_count(tp)
	PyTypeObject *tp;
{
	if (tp->tp_alloc == 0) {
		/* first time; insert in linked list */
		if (tp->tp_next != NULL) /* sanity check */
			Py_FatalError("XXX inc_count sanity check");
		tp->tp_next = type_list;
		type_list = tp;
	}
	tp->tp_alloc++;
	if (tp->tp_alloc - tp->tp_free > tp->tp_maxalloc)
		tp->tp_maxalloc = tp->tp_alloc - tp->tp_free;
}
#endif

#ifndef MS_COREDLL
PyObject *
_PyObject_New(tp)
	PyTypeObject *tp;
#else
PyObject *
_PyObject_New(tp,op)
	PyTypeObject *tp;
	PyObject *op;
#endif
{
#ifndef MS_COREDLL
	PyObject *op = (PyObject *) malloc(tp->tp_basicsize);
#endif
	if (op == NULL)
		return PyErr_NoMemory();
	op->ob_type = tp;
	_Py_NewReference(op);
	return op;
}

#ifndef MS_COREDLL
PyVarObject *
_PyObject_NewVar(tp, size)
	PyTypeObject *tp;
	int size;
#else
PyVarObject *
_PyObject_NewVar(tp, size, op)
	PyTypeObject *tp;
	int size;
	PyVarObject *op;
#endif
{
#ifndef MS_COREDLL
	PyVarObject *op = (PyVarObject *)
		malloc(tp->tp_basicsize + size * tp->tp_itemsize);
#endif
	if (op == NULL)
		return (PyVarObject *)PyErr_NoMemory();
	op->ob_type = tp;
	op->ob_size = size;
	_Py_NewReference(op);
	return op;
}

int
PyObject_Print(op, fp, flags)
	PyObject *op;
	FILE *fp;
	int flags;
{
	int ret = 0;
	if (PyErr_CheckSignals())
		return -1;
	if (op == NULL) {
		fprintf(fp, "<nil>");
	}
	else {
		if (op->ob_refcnt <= 0)
			fprintf(fp, "<refcnt %u at %lx>",
				op->ob_refcnt, (long)op);
		else if (op->ob_type->tp_print == NULL) {
			if (op->ob_type->tp_repr == NULL) {
				fprintf(fp, "<%s object at %lx>",
					op->ob_type->tp_name, (long)op);
			}
			else {
				PyObject *s;
				if (flags & Py_PRINT_RAW)
					s = PyObject_Str(op);
				else
					s = PyObject_Repr(op);
				if (s == NULL)
					ret = -1;
				else if (!PyString_Check(s)) {
					PyErr_SetString(PyExc_TypeError,
						   "repr not string");
					ret = -1;
				}
				else {
					fprintf(fp, "%s",
						PyString_AsString(s));
				}
				Py_XDECREF(s);
			}
		}
		else
			ret = (*op->ob_type->tp_print)(op, fp, flags);
	}
	if (ret == 0) {
		if (ferror(fp)) {
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(fp);
			ret = -1;
		}
	}
	return ret;
}

PyObject *
PyObject_Repr(v)
	PyObject *v;
{
	if (PyErr_CheckSignals())
		return NULL;
	if (v == NULL)
		return PyString_FromString("<NULL>");
	else if (v->ob_type->tp_repr == NULL) {
		char buf[120];
		sprintf(buf, "<%.80s object at %lx>",
			v->ob_type->tp_name, (long)v);
		return PyString_FromString(buf);
	}
	else
		return (*v->ob_type->tp_repr)(v);
}

PyObject *
PyObject_Str(v)
	PyObject *v;
{
	if (v == NULL)
		return PyString_FromString("<NULL>");
	else if (PyString_Check(v)) {
		Py_INCREF(v);
		return v;
	}
	else if (v->ob_type->tp_str != NULL)
		return (*v->ob_type->tp_str)(v);
	else {
		PyObject *func;
		PyObject *res;
		if (!PyInstance_Check(v) ||
		    (func = PyObject_GetAttrString(v, "__str__")) == NULL) {
			PyErr_Clear();
			return PyObject_Repr(v);
		}
		res = PyEval_CallObject(func, (PyObject *)NULL);
		Py_DECREF(func);
		return res;
	}
}

static PyObject *
do_cmp(v, w)
	PyObject *v, *w;
{
	long c;
	/* __rcmp__ actually won't be called unless __cmp__ isn't defined,
	   because the check in cmpobject() reverses the objects first.
	   This is intentional -- it makes no sense to define cmp(x,y)
	   different than -cmp(y,x). */
	if (PyInstance_Check(v) || PyInstance_Check(w))
		return PyInstance_DoBinOp(v, w, "__cmp__", "__rcmp__", do_cmp);
	c = PyObject_Compare(v, w);
	if (c && PyErr_Occurred())
		return NULL;
	return PyInt_FromLong(c);
}

int
PyObject_Compare(v, w)
	PyObject *v, *w;
{
	PyTypeObject *tp;
	if (v == NULL || w == NULL) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (v == w)
		return 0;
	if (PyInstance_Check(v) || PyInstance_Check(w)) {
		PyObject *res;
		int c;
		if (!PyInstance_Check(v))
			return -PyObject_Compare(w, v);
		res = do_cmp(v, w);
		if (res == NULL)
			return -1;
		if (!PyInt_Check(res)) {
			Py_DECREF(res);
			PyErr_SetString(PyExc_TypeError,
					"comparison did not return an int");
			return -1;
		}
		c = PyInt_AsLong(res);
		Py_DECREF(res);
		return (c < 0) ? -1 : (c > 0) ? 1 : 0;	
	}
	if ((tp = v->ob_type) != w->ob_type) {
		if (tp->tp_as_number != NULL &&
				w->ob_type->tp_as_number != NULL) {
			if (PyNumber_Coerce(&v, &w) != 0)
				return -1;
			else {
				int cmp = (*v->ob_type->tp_compare)(v, w);
				Py_DECREF(v);
				Py_DECREF(w);
				return cmp;
			}
		}
		return strcmp(tp->tp_name, w->ob_type->tp_name);
	}
	if (tp->tp_compare == NULL)
		return (v < w) ? -1 : 1;
	return (*tp->tp_compare)(v, w);
}

long
PyObject_Hash(v)
	PyObject *v;
{
	PyTypeObject *tp = v->ob_type;
	if (tp->tp_hash != NULL)
		return (*tp->tp_hash)(v);
	if (tp->tp_compare == NULL)
		return (long) v; /* Use address as hash value */
	/* If there's a cmp but no hash defined, the object can't be hashed */
	PyErr_SetString(PyExc_TypeError, "unhashable type");
	return -1;
}

PyObject *
PyObject_GetAttrString(v, name)
	PyObject *v;
	char *name;
{
	if (v->ob_type->tp_getattro != NULL) {
		PyObject *w, *res;
		w = PyString_InternFromString(name);
		if (w == NULL)
			return NULL;
		res = (*v->ob_type->tp_getattro)(v, w);
		Py_XDECREF(w);
		return res;
	}

	if (v->ob_type->tp_getattr == NULL) {
		PyErr_SetString(PyExc_AttributeError, "attribute-less object");
		return NULL;
	}
	else {
		return (*v->ob_type->tp_getattr)(v, name);
	}
}

int
PyObject_HasAttrString(v, name)
	PyObject *v;
	char *name;
{
	PyObject *res = PyObject_GetAttrString(v, name);
	if (res != NULL) {
		Py_DECREF(res);
		return 1;
	}
	PyErr_Clear();
	return 0;
}

int
PyObject_SetAttrString(v, name, w)
	PyObject *v;
	char *name;
	PyObject *w;
{
	if (v->ob_type->tp_setattro != NULL) {
		PyObject *s;
		int res;
		s = PyString_InternFromString(name);
		if (s == NULL)
			return -1;
		res = (*v->ob_type->tp_setattro)(v, s, w);
		Py_XDECREF(s);
		return res;
	}

	if (v->ob_type->tp_setattr == NULL) {
		if (v->ob_type->tp_getattr == NULL)
			PyErr_SetString(PyExc_TypeError,
				   "attribute-less object (assign or del)");
		else
			PyErr_SetString(PyExc_TypeError,
				   "object has read-only attributes");
		return -1;
	}
	else {
		return (*v->ob_type->tp_setattr)(v, name, w);
	}
}

PyObject *
PyObject_GetAttr(v, name)
	PyObject *v;
	PyObject *name;
{
	if (v->ob_type->tp_getattro != NULL)
		return (*v->ob_type->tp_getattro)(v, name);
	else
		return PyObject_GetAttrString(v, PyString_AsString(name));
}

int
PyObject_HasAttr(v, name)
	PyObject *v;
	PyObject *name;
{
	PyObject *res = PyObject_GetAttr(v, name);
	if (res != NULL) {
		Py_DECREF(res);
		return 1;
	}
	PyErr_Clear();
	return 0;
}

int
PyObject_SetAttr(v, name, value)
	PyObject *v;
	PyObject *name;
	PyObject *value;
{
	int err;
	Py_INCREF(name);
	PyString_InternInPlace(&name);
	if (v->ob_type->tp_setattro != NULL)
		err = (*v->ob_type->tp_setattro)(v, name, value);
	else
		err = PyObject_SetAttrString(
			v, PyString_AsString(name), value);
	Py_DECREF(name);
	return err;
}

/* Test a value used as condition, e.g., in a for or if statement.
   Return -1 if an error occurred */

int
PyObject_IsTrue(v)
	PyObject *v;
{
	int res;
	if (v == Py_None)
		res = 0;
	else if (v->ob_type->tp_as_number != NULL)
		res = (*v->ob_type->tp_as_number->nb_nonzero)(v);
	else if (v->ob_type->tp_as_mapping != NULL)
		res = (*v->ob_type->tp_as_mapping->mp_length)(v);
	else if (v->ob_type->tp_as_sequence != NULL)
		res = (*v->ob_type->tp_as_sequence->sq_length)(v);
	else
		res = 1;
	if (res > 0)
		res = 1;
	return res;
}

/* Coerce two numeric types to the "larger" one.
   Increment the reference count on each argument.
   Return -1 and raise an exception if no coercion is possible
   (and then no reference count is incremented).
*/

int
PyNumber_Coerce(pv, pw)
	PyObject **pv, **pw;
{
	register PyObject *v = *pv;
	register PyObject *w = *pw;
	int res;

	if (v->ob_type == w->ob_type && !PyInstance_Check(v)) {
		Py_INCREF(v);
		Py_INCREF(w);
		return 0;
	}
	if (v->ob_type->tp_as_number && v->ob_type->tp_as_number->nb_coerce) {
		res = (*v->ob_type->tp_as_number->nb_coerce)(pv, pw);
		if (res <= 0)
			return res;
	}
	if (w->ob_type->tp_as_number && w->ob_type->tp_as_number->nb_coerce) {
		res = (*w->ob_type->tp_as_number->nb_coerce)(pw, pv);
		if (res <= 0)
			return res;
	}
	PyErr_SetString(PyExc_TypeError, "number coercion failed");
	return -1;
}


/* Test whether an object can be called */

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


/*
NoObject is usable as a non-NULL undefined value, used by the macro None.
There is (and should be!) no way to create other objects of this type,
so there is exactly one (which is indestructible, by the way).
*/

/* ARGSUSED */
static PyObject *
none_repr(op)
	PyObject *op;
{
	return PyString_FromString("None");
}

static PyTypeObject PyNothing_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"None",
	0,
	0,
	0,		/*tp_dealloc*/ /*never called*/
	0,		/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	(reprfunc)none_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	0,		/*tp_hash */
};

PyObject _Py_NoneStruct = {
	PyObject_HEAD_INIT(&PyNothing_Type)
};


#ifdef Py_TRACE_REFS

static PyObject refchain = {&refchain, &refchain};

void
_Py_ResetReferences()
{
	refchain._ob_prev = refchain._ob_next = &refchain;
	_Py_RefTotal = 0;
}

void
_Py_NewReference(op)
	PyObject *op;
{
	_Py_RefTotal++;
	op->ob_refcnt = 1;
	op->_ob_next = refchain._ob_next;
	op->_ob_prev = &refchain;
	refchain._ob_next->_ob_prev = op;
	refchain._ob_next = op;
#ifdef COUNT_ALLOCS
	inc_count(op->ob_type);
#endif
}

void
_Py_ForgetReference(op)
	register PyObject *op;
{
	register PyObject *p;
	if (op->ob_refcnt < 0)
		Py_FatalError("UNREF negative refcnt");
	if (op == &refchain ||
	    op->_ob_prev->_ob_next != op || op->_ob_next->_ob_prev != op)
		Py_FatalError("UNREF invalid object");
#ifdef SLOW_UNREF_CHECK
	for (p = refchain._ob_next; p != &refchain; p = p->_ob_next) {
		if (p == op)
			break;
	}
	if (p == &refchain) /* Not found */
		Py_FatalError("UNREF unknown object");
#endif
	op->_ob_next->_ob_prev = op->_ob_prev;
	op->_ob_prev->_ob_next = op->_ob_next;
	op->_ob_next = op->_ob_prev = NULL;
#ifdef COUNT_ALLOCS
	op->ob_type->tp_free++;
#endif
}

void
_Py_Dealloc(op)
	PyObject *op;
{
	destructor dealloc = op->ob_type->tp_dealloc;
	_Py_ForgetReference(op);
	op->ob_type = NULL;
	(*dealloc)(op);
}

void
_Py_PrintReferences(fp)
	FILE *fp;
{
	PyObject *op;
	fprintf(fp, "Remaining objects:\n");
	for (op = refchain._ob_next; op != &refchain; op = op->_ob_next) {
		fprintf(fp, "[%d] ", op->ob_refcnt);
		if (PyObject_Print(op, fp, 0) != 0)
			PyErr_Clear();
		putc('\n', fp);
	}
}

PyObject *
_Py_GetObjects(self, args)
	PyObject *self;
	PyObject *args;
{
	int i, n;
	PyObject *t = NULL;
	PyObject *res, *op;

	if (!PyArg_ParseTuple(args, "i|O", &n, &t))
		return NULL;
	op = refchain._ob_next;
	res = PyList_New(0);
	if (res == NULL)
		return NULL;
	for (i = 0; (n == 0 || i < n) && op != &refchain; i++) {
		while (op == self || op == args || op == res || op == t ||
		       t != NULL && op->ob_type != (PyTypeObject *) t) {
			op = op->_ob_next;
			if (op == &refchain)
				return res;
		}
		if (PyList_Append(res, op) < 0) {
			Py_DECREF(res);
			return NULL;
		}
		op = op->_ob_next;
	}
	return res;
}

#endif


/* Hack to force loading of cobject.o */
PyTypeObject *_Py_cobject_hack = &PyCObject_Type;


/* Hack to force loading of abstract.o */
int (*_Py_abstract_hack) Py_FPROTO((PyObject *)) = &PyObject_Length;


/* Malloc wrappers (see mymalloc.h) */

/* The Py_{Malloc,Realloc} wrappers call PyErr_NoMemory() on failure */

ANY *
Py_Malloc(nbytes)
	size_t nbytes;
{
	ANY *p;
#if _PyMem_EXTRA > 0
	if (nbytes == 0)
		nbytes = _PyMem_EXTRA;
#endif
	p = malloc(nbytes);
	if (p != NULL)
		return p;
	else {
		PyErr_NoMemory();
		return NULL;
	}
}

ANY *
Py_Realloc(p, nbytes)
	ANY *p;
	size_t nbytes;
{
#if _PyMem_EXTRA > 0
	if (nbytes == 0)
		nbytes = _PyMem_EXTRA;
#endif
	p = realloc(p, nbytes);
	if (p != NULL)
		return p;
	else {
		PyErr_NoMemory();
		return NULL;
	}
}

void
Py_Free(p)
	ANY *p;
{
	free(p);
}

/* The PyMem_{Malloc,Realloc} wrappers don't call anything on failure */

ANY *
PyMem_Malloc(nbytes)
	size_t nbytes;
{
#if _PyMem_EXTRA > 0
	if (nbytes == 0)
		nbytes = _PyMem_EXTRA;
#endif
	return malloc(nbytes);
}

ANY *
PyMem_Realloc(p, nbytes)
	ANY *p;
	size_t nbytes;
{
#if _PyMem_EXTRA > 0
	if (nbytes == 0)
		nbytes = _PyMem_EXTRA;
#endif
	return realloc(p, nbytes);
}

void
PyMem_Free(p)
	ANY *p;
{
	free(p);
}
