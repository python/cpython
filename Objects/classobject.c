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

/* Class object implementation */

#include "Python.h"
#include "structmember.h"

/* Forward */
static PyObject *class_lookup
	Py_PROTO((PyClassObject *, PyObject *, PyClassObject **));
static PyObject *instance_getattr1 Py_PROTO((PyInstanceObject *, PyObject *));

PyObject *
PyClass_New(bases, dict, name)
	PyObject *bases; /* NULL or tuple of classobjects! */
	PyObject *dict;
	PyObject *name; /* String; NULL if unknown */
{
#ifdef SUPPORT_OBSOLETE_ACCESS
	int pos;
	PyObject *key, *value;
#endif
	PyClassObject *op, *dummy;
	static PyObject *getattrstr, *setattrstr, *delattrstr;
	static PyObject *docstr;
	if (docstr == NULL) {
		docstr= PyString_InternFromString("__doc__");
		if (docstr == NULL)
			return NULL;
	}
	if (PyDict_GetItem(dict, docstr) == NULL) {
		if (PyDict_SetItem(dict, docstr, Py_None) < 0)
			return NULL;
	}
	if (bases == NULL) {
		bases = PyTuple_New(0);
		if (bases == NULL)
			return NULL;
	}
	else
		Py_INCREF(bases);
	op = PyObject_NEW(PyClassObject, &PyClass_Type);
	if (op == NULL) {
		Py_DECREF(bases);
		return NULL;
	}
	op->cl_bases = bases;
	Py_INCREF(dict);
	op->cl_dict = dict;
	Py_XINCREF(name);
	op->cl_name = name;
	if (getattrstr == NULL) {
		getattrstr = PyString_InternFromString("__getattr__");
		setattrstr = PyString_InternFromString("__setattr__");
		delattrstr = PyString_InternFromString("__delattr__");
	}
	op->cl_getattr = class_lookup(op, getattrstr, &dummy);
	op->cl_setattr = class_lookup(op, setattrstr, &dummy);
	op->cl_delattr = class_lookup(op, delattrstr, &dummy);
	Py_XINCREF(op->cl_getattr);
	Py_XINCREF(op->cl_setattr);
	Py_XINCREF(op->cl_delattr);
#ifdef SUPPORT_OBSOLETE_ACCESS
	pos = 0;
	while (PyDict_Next(dict, &pos, &key, &value)) {
		if (PyAccess_Check(value))
			PyAccess_SetOwner(value, (PyObject *)op);
	}
#endif
	return (PyObject *) op;
}

/* Class methods */

static void
class_dealloc(op)
	PyClassObject *op;
{
	Py_DECREF(op->cl_bases);
	Py_DECREF(op->cl_dict);
	Py_XDECREF(op->cl_name);
	free((ANY *)op);
}

static PyObject *
class_lookup(cp, name, pclass)
	PyClassObject *cp;
	PyObject *name;
	PyClassObject **pclass;
{
	int i, n;
	PyObject *value = PyDict_GetItem(cp->cl_dict, name);
	if (value != NULL) {
		*pclass = cp;
		return value;
	}
	n = PyTuple_Size(cp->cl_bases);
	for (i = 0; i < n; i++) {
		PyObject *v = class_lookup(
			(PyClassObject *)
			PyTuple_GetItem(cp->cl_bases, i), name, pclass);
		if (v != NULL)
			return v;
	}
	return NULL;
}

static PyObject *
class_getattr(op, name)
	register PyClassObject *op;
	PyObject *name;
{
	register PyObject *v;
	register char *sname = PyString_AsString(name);
	PyClassObject *class;
	if (sname[0] == '_' && sname[1] == '_') {
		if (strcmp(sname, "__dict__") == 0) {
			if (PyEval_GetRestricted()) {
				PyErr_SetString(PyExc_RuntimeError,
			   "class.__dict__ not accessible in restricted mode");
				return NULL;
			}
			Py_INCREF(op->cl_dict);
			return op->cl_dict;
		}
		if (strcmp(sname, "__bases__") == 0) {
			Py_INCREF(op->cl_bases);
			return op->cl_bases;
		}
		if (strcmp(sname, "__name__") == 0) {
			if (op->cl_name == NULL)
				v = Py_None;
			else
				v = op->cl_name;
			Py_INCREF(v);
			return v;
		}
	}
	v = class_lookup(op, name, &class);
	if (v == NULL) {
		PyErr_SetObject(PyExc_AttributeError, name);
		return NULL;
	}
#ifdef SUPPORT_OBSOLETE_ACCESS
	if (PyAccess_Check(v)) {
		v = PyAccess_AsValue(v, PyEval_GetOwner());
		if (v == NULL)
			return NULL;
	}
	else
#endif
		Py_INCREF(v);
	if (PyFunction_Check(v)) {
		PyObject *w = PyMethod_New(v, (PyObject *)NULL,
						    (PyObject *)class);
		Py_DECREF(v);
		v = w;
	}
	return v;
}

static int
class_setattr(op, name, v)
	PyClassObject *op;
	PyObject *name;
	PyObject *v;
{
#ifdef SUPPORT_OBSOLETE_ACCESS
	PyObject *ac;
#endif
	char *sname = PyString_AsString(name);
	if (sname[0] == '_' && sname[1] == '_') {
		int n = PyString_Size(name);
		if (sname[n-1] == '_' && sname[n-2] == '_') {
			PyErr_SetString(PyExc_TypeError,
					"read-only special attribute");
			return -1;
		}
	}
	if (PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
			   "classes are read-only in restricted mode");
		return -1;
	}
#ifdef SUPPORT_OBSOLETE_ACCESS
	ac = PyDict_GetItem(op->cl_dict, name);
	if (ac != NULL && PyAccess_Check(ac))
		return PyAccess_SetValue(ac, PyEval_GetOwner(), v);
#endif
	if (v == NULL) {
		int rv = PyDict_DelItem(op->cl_dict, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
				   "delete non-existing class attribute");
		return rv;
	}
	else
		return PyDict_SetItem(op->cl_dict, name, v);
}

static PyObject *
class_repr(op)
	PyClassObject *op;
{
	char buf[140];
	char *name;
	if (op->cl_name == NULL || !PyString_Check(op->cl_name))
		name = "?";
	else
		name = PyString_AsString(op->cl_name);
	sprintf(buf, "<class %.100s at %lx>", name, (long)op);
	return PyString_FromString(buf);
}

PyTypeObject PyClass_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"class",
	sizeof(PyClassObject),
	0,
	(destructor)class_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	(reprfunc)class_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	0,		/*tp_hash*/
	0,		/*tp_call*/
	0,		/*tp_str*/
	(getattrofunc)class_getattr, /*tp_getattro*/
	(setattrofunc)class_setattr, /*tp_setattro*/
};

int
PyClass_IsSubclass(class, base)
	PyObject *class;
	PyObject *base;
{
	int i, n;
	PyClassObject *cp;
	if (class == base)
		return 1;
	if (class == NULL || !PyClass_Check(class))
		return 0;
	cp = (PyClassObject *)class;
	n = PyTuple_Size(cp->cl_bases);
	for (i = 0; i < n; i++) {
		if (PyClass_IsSubclass(PyTuple_GetItem(cp->cl_bases, i), base))
			return 1;
	}
	return 0;
}


/* Instance objects */

#ifdef SUPPORT_OBSOLETE_ACCESS
static int
addaccess(class, inst)
	PyClassObject *class;
	PyInstanceObject *inst;
{
	int i, n, pos, ret;
	PyObject *key, *value, *ac;
	
	n = PyTuple_Size(class->cl_bases);
	for (i = 0; i < n; i++) {
		if (addaccess((PyClassObject *)PyTuple_GetItem(
			      class->cl_bases, i), inst) < 0)
			return -1;
	}
	
	pos = 0;
	while (PyDict_Next(class->cl_dict, &pos, &key, &value)) {
		if (!PyAccess_Check(value))
			continue;
		if (PyAccess_HasValue(value))
			continue;
		ac = PyDict_GetItem(inst->in_dict, key);
		if (ac != NULL && PyAccess_Check(ac)) {
			PyErr_SetObject(PyExc_ConflictError, key);
			return -1;
		}
		ac = PyAccess_Clone(value);
		if (ac == NULL)
			return -1;
		ret = PyDict_SetItem(inst->in_dict, key, ac);
		Py_DECREF(ac);
		if (ret != 0)
			return -1;
    	}
	return 0;
}
#endif

PyObject *
PyInstance_New(class, arg, kw)
	PyObject *class;
	PyObject *arg;
	PyObject *kw;
{
	register PyInstanceObject *inst;
	PyObject *init;
	static PyObject *initstr;
	if (!PyClass_Check(class)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	inst = PyObject_NEW(PyInstanceObject, &PyInstance_Type);
	if (inst == NULL)
		return NULL;
	Py_INCREF(class);
	inst->in_class = (PyClassObject *)class;
	inst->in_dict = PyDict_New();
	if (inst->in_dict == NULL
#ifdef SUPPORT_OBSOLETE_ACCESS
	    || addaccess((PyClassObject *)class, inst) != 0
#endif
		) {
		Py_DECREF(inst);
		return NULL;
	}
	if (initstr == NULL)
		initstr = PyString_InternFromString("__init__");
	init = instance_getattr1(inst, initstr);
	if (init == NULL) {
		PyErr_Clear();
		if ((arg != NULL && (!PyTuple_Check(arg) ||
				     PyTuple_Size(arg) != 0))
		    || (kw != NULL && (!PyDict_Check(kw) ||
				      PyDict_Size(kw) != 0))) {
			PyErr_SetString(PyExc_TypeError,
				   "this constructor takes no arguments");
			Py_DECREF(inst);
			inst = NULL;
		}
	}
	else {
		PyObject *res = PyEval_CallObjectWithKeywords(init, arg, kw);
		Py_DECREF(init);
		if (res == NULL) {
			Py_DECREF(inst);
			inst = NULL;
		}
		else {
			if (res != Py_None) {
				PyErr_SetString(PyExc_TypeError,
					   "__init__() should return None");
				Py_DECREF(inst);
				inst = NULL;
			}
			Py_DECREF(res);
		}
	}
	return (PyObject *)inst;
}

/* Instance methods */

static void
instance_dealloc(inst)
	register PyInstanceObject *inst;
{
	PyObject *error_type, *error_value, *error_traceback;
	PyObject *del;
	static PyObject *delstr;
	/* Call the __del__ method if it exists.  First temporarily
	   revive the object and save the current exception, if any. */
#ifdef Py_TRACE_REFS
	/* much too complicated if Py_TRACE_REFS defined */
	extern long _Py_RefTotal;
	inst->ob_type = &PyInstance_Type;
	_Py_NewReference(inst);
	_Py_RefTotal--;		/* compensate for increment in NEWREF */
#ifdef COUNT_ALLOCS
	inst->ob_type->tp_alloc--; /* ditto */
#endif
#else /* !Py_TRACE_REFS */
	Py_INCREF(inst);
#endif /* !Py_TRACE_REFS */
	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	if (delstr == NULL)
		delstr = PyString_InternFromString("__del__");
	if ((del = instance_getattr1(inst, delstr)) != NULL) {
		PyObject *res = PyEval_CallObject(del, (PyObject *)NULL);
		if (res == NULL) {
			PyObject *f, *t, *v, *tb;
 			PyErr_Fetch(&t, &v, &tb);
			f = PySys_GetObject("stderr");
			PyErr_Clear();
			if (f != NULL) {
				PyFile_WriteString("Exception ", f);
				if (t) {
					PyFile_WriteObject(t, f, Py_PRINT_RAW);
					if (v && v != Py_None) {
						PyFile_WriteString(": ", f);
						PyFile_WriteObject(v, f, 0);
					}
				}
				PyFile_WriteString(" in ", f);
				PyFile_WriteObject(del, f, 0);
				PyFile_WriteString(" ignored\n", f);
				PyErr_Clear(); /* Just in case */
			}
			Py_XDECREF(t);
			Py_XDECREF(v);
			Py_XDECREF(tb);
		}
		else
			Py_DECREF(res);
		Py_DECREF(del);
	}
	/* Restore the saved exception and undo the temporary revival */
	PyErr_Restore(error_type, error_value, error_traceback);
	/* Can't use DECREF here, it would cause a recursive call */
	if (--inst->ob_refcnt > 0) {
#ifdef COUNT_ALLOCS
		inst->ob_type->tp_free--;
#endif
		return; /* __del__ added a reference; don't delete now */
	}
#ifdef Py_TRACE_REFS
#ifdef COUNT_ALLOCS
	inst->ob_type->tp_free--;	/* compensate for increment in UNREF */
#endif
	_Py_ForgetReference(inst);
	inst->ob_type = NULL;
#endif /* Py_TRACE_REFS */
	Py_DECREF(inst->in_class);
	Py_XDECREF(inst->in_dict);
	free((ANY *)inst);
}

static PyObject *
instance_getattr1(inst, name)
	register PyInstanceObject *inst;
	PyObject *name;
{
	register PyObject *v;
	register char *sname = PyString_AsString(name);
	PyClassObject *class;
	if (sname[0] == '_' && sname[1] == '_') {
		if (strcmp(sname, "__dict__") == 0) {
			if (PyEval_GetRestricted()) {
				PyErr_SetString(PyExc_RuntimeError,
			"instance.__dict__ not accessible in restricted mode");
				return NULL;
			}
			Py_INCREF(inst->in_dict);
			return inst->in_dict;
		}
		if (strcmp(sname, "__class__") == 0) {
			Py_INCREF(inst->in_class);
			return (PyObject *)inst->in_class;
		}
	}
	class = NULL;
	v = PyDict_GetItem(inst->in_dict, name);
	if (v == NULL) {
		v = class_lookup(inst->in_class, name, &class);
		if (v == NULL) {
			PyErr_SetObject(PyExc_AttributeError, name);
			return NULL;
		}
	}
#ifdef SUPPORT_OBSOLETE_ACCESS
	if (PyAccess_Check(v)) {
		v = PyAccess_AsValue(v, PyEval_GetOwner());
		if (v == NULL)
			return NULL;
	}
	else
#endif
		Py_INCREF(v);
	if (class != NULL) {
		if (PyFunction_Check(v)) {
			PyObject *w = PyMethod_New(v, (PyObject *)inst,
							    (PyObject *)class);
			Py_DECREF(v);
			v = w;
		}
		else if (PyMethod_Check(v)) {
			PyObject *im_class = PyMethod_Class(v);
			/* Only if classes are compatible */
			if (PyClass_IsSubclass((PyObject *)class, im_class)) {
				PyObject *im_func = PyMethod_Function(v);
				PyObject *w = PyMethod_New(im_func,
						(PyObject *)inst, im_class);
				Py_DECREF(v);
				v = w;
			}
		}
	}
	return v;
}

static PyObject *
instance_getattr(inst, name)
	register PyInstanceObject *inst;
	PyObject *name;
{
	register PyObject *func, *res;
	res = instance_getattr1(inst, name);
	if (res == NULL && (func = inst->in_class->cl_getattr) != NULL) {
		PyObject *args;
		PyErr_Clear();
		args = Py_BuildValue("(OO)", inst, name);
		if (args == NULL)
			return NULL;
		res = PyEval_CallObject(func, args);
		Py_DECREF(args);
	}
	return res;
}

static int
instance_setattr1(inst, name, v)
	PyInstanceObject *inst;
	PyObject *name;
	PyObject *v;
{
#ifdef SUPPORT_OBSOLETE_ACCESS
	PyObject *ac;
	ac = PyDict_GetItem(inst->in_dict, name);
	if (ac != NULL && PyAccess_Check(ac))
		return PyAccess_SetValue(ac, PyEval_GetOwner(), v);
#endif
	if (v == NULL) {
		int rv = PyDict_DelItem(inst->in_dict, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
				   "delete non-existing instance attribute");
		return rv;
	}
	else
		return PyDict_SetItem(inst->in_dict, name, v);
}

static int
instance_setattr(inst, name, v)
	PyInstanceObject *inst;
	PyObject *name;
	PyObject *v;
{
	PyObject *func, *args, *res;
	char *sname = PyString_AsString(name);
	if (sname[0] == '_' && sname[1] == '_'
	    && (strcmp(sname, "__dict__") == 0 ||
		strcmp(sname, "__class__") == 0)) {
	        int n = PyString_Size(name);
		if (sname[n-1] == '_' && sname[n-2] == '_') {
			PyErr_SetString(PyExc_TypeError,
					"read-only special attribute");
			return -1;
		}
	}
	if (v == NULL)
		func = inst->in_class->cl_delattr;
	else
		func = inst->in_class->cl_setattr;
	if (func == NULL)
		return instance_setattr1(inst, name, v);
	if (v == NULL)
		args = Py_BuildValue("(OO)", inst, name);
	else
		args = Py_BuildValue("(OOO)", inst, name, v);
	if (args == NULL)
		return -1;
	res = PyEval_CallObject(func, args);
	Py_DECREF(args);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static PyObject *
instance_repr(inst)
	PyInstanceObject *inst;
{
	PyObject *func;
	PyObject *res;
	static PyObject *reprstr;

	if (reprstr == NULL)
		reprstr = PyString_InternFromString("__repr__");
	func = instance_getattr(inst, reprstr);
	if (func == NULL) {
		char buf[140];
		PyObject *classname = inst->in_class->cl_name;
		char *cname;
		if (classname != NULL && PyString_Check(classname))
			cname = PyString_AsString(classname);
		else
			cname = "?";
		PyErr_Clear();
		sprintf(buf, "<%.100s instance at %lx>", cname, (long)inst);
		return PyString_FromString(buf);
	}
	res = PyEval_CallObject(func, (PyObject *)NULL);
	Py_DECREF(func);
	return res;
}

static PyObject *
instance_compare1(inst, other)
	PyObject *inst, *other;
{
	return PyInstance_DoBinOp(inst, other, "__cmp__", "__rcmp__",
			     instance_compare1);
}

static int
instance_compare(inst, other)
	PyObject *inst, *other;
{
	PyObject *result;
	long outcome;
	result = instance_compare1(inst, other);
	if (result == NULL || !PyInt_Check(result)) {
		PyErr_Clear();
		return (inst < other) ? -1 : 1;
	}
	outcome = PyInt_AsLong(result);
	Py_DECREF(result);
	if (outcome < 0)
		return -1;
	else if (outcome > 0)
		return 1;
	return 0;
}

static long
instance_hash(inst)
	PyInstanceObject *inst;
{
	PyObject *func;
	PyObject *res;
	long outcome;
	static PyObject *hashstr, *cmpstr;

	if (hashstr == NULL)
		hashstr = PyString_InternFromString("__hash__");
	func = instance_getattr(inst, hashstr);
	if (func == NULL) {
		/* If there is no __cmp__ method, we hash on the address.
		   If a __cmp__ method exists, there must be a __hash__. */
		PyErr_Clear();
		if (cmpstr == NULL)
			cmpstr = PyString_InternFromString("__cmp__");
		func = instance_getattr(inst, cmpstr);
		if (func == NULL) {
			PyErr_Clear();
			outcome = (long)inst;
			if (outcome == -1)
				outcome = -2;
			return outcome;
		}
		PyErr_SetString(PyExc_TypeError, "unhashable instance");
		return -1;
	}
	res = PyEval_CallObject(func, (PyObject *)NULL);
	Py_DECREF(func);
	if (res == NULL)
		return -1;
	if (PyInt_Check(res)) {
		outcome = PyInt_AsLong(res);
		if (outcome == -1)
			outcome = -2;
	}
	else {
		PyErr_SetString(PyExc_TypeError,
				"__hash__() should return an int");
		outcome = -1;
	}
	Py_DECREF(res);
	return outcome;
}

static PyObject *getitemstr, *setitemstr, *delitemstr, *lenstr;

static int
instance_length(inst)
	PyInstanceObject *inst;
{
	PyObject *func;
	PyObject *res;
	int outcome;

	if (lenstr == NULL)
		lenstr = PyString_InternFromString("__len__");
	func = instance_getattr(inst, lenstr);
	if (func == NULL)
		return -1;
	res = PyEval_CallObject(func, (PyObject *)NULL);
	Py_DECREF(func);
	if (res == NULL)
		return -1;
	if (PyInt_Check(res)) {
		outcome = PyInt_AsLong(res);
		if (outcome < 0)
			PyErr_SetString(PyExc_ValueError,
					"__len__() should return >= 0");
	}
	else {
		PyErr_SetString(PyExc_TypeError,
				"__len__() should return an int");
		outcome = -1;
	}
	Py_DECREF(res);
	return outcome;
}

static PyObject *
instance_subscript(inst, key)
	PyInstanceObject *inst;
	PyObject *key;
{
	PyObject *func;
	PyObject *arg;
	PyObject *res;

	if (getitemstr == NULL)
		getitemstr = PyString_InternFromString("__getitem__");
	func = instance_getattr(inst, getitemstr);
	if (func == NULL)
		return NULL;
	arg = Py_BuildValue("(O)", key);
	if (arg == NULL) {
		Py_DECREF(func);
		return NULL;
	}
	res = PyEval_CallObject(func, arg);
	Py_DECREF(func);
	Py_DECREF(arg);
	return res;
}

static int
instance_ass_subscript(inst, key, value)
	PyInstanceObject*inst;
	PyObject *key;
	PyObject *value;
{
	PyObject *func;
	PyObject *arg;
	PyObject *res;

	if (value == NULL) {
		if (delitemstr == NULL)
			delitemstr = PyString_InternFromString("__delitem__");
		func = instance_getattr(inst, delitemstr);
	}
	else {
		if (setitemstr == NULL)
			setitemstr = PyString_InternFromString("__setitem__");
		func = instance_getattr(inst, setitemstr);
	}
	if (func == NULL)
		return -1;
	if (value == NULL)
		arg = Py_BuildValue("(O)", key);
	else
		arg = Py_BuildValue("(OO)", key, value);
	if (arg == NULL) {
		Py_DECREF(func);
		return -1;
	}
	res = PyEval_CallObject(func, arg);
	Py_DECREF(func);
	Py_DECREF(arg);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static PyMappingMethods instance_as_mapping = {
	(inquiry)instance_length, /*mp_length*/
	(binaryfunc)instance_subscript, /*mp_subscript*/
	(objobjargproc)instance_ass_subscript, /*mp_ass_subscript*/
};

static PyObject *
instance_item(inst, i)
	PyInstanceObject *inst;
	int i;
{
	PyObject *func, *arg, *res;

	if (getitemstr == NULL)
		getitemstr = PyString_InternFromString("__getitem__");
	func = instance_getattr(inst, getitemstr);
	if (func == NULL)
		return NULL;
	arg = Py_BuildValue("(i)", i);
	if (arg == NULL) {
		Py_DECREF(func);
		return NULL;
	}
	res = PyEval_CallObject(func, arg);
	Py_DECREF(func);
	Py_DECREF(arg);
	return res;
}

static PyObject *
instance_slice(inst, i, j)
	PyInstanceObject *inst;
	int i, j;
{
	PyObject *func, *arg, *res;
	static PyObject *getslicestr;

	if (getslicestr == NULL)
		getslicestr = PyString_InternFromString("__getslice__");
	func = instance_getattr(inst, getslicestr);
	if (func == NULL)
		return NULL;
	arg = Py_BuildValue("(ii)", i, j);
	if (arg == NULL) {
		Py_DECREF(func);
		return NULL;
	}
	res = PyEval_CallObject(func, arg);
	Py_DECREF(func);
	Py_DECREF(arg);
	return res;
}

static int
instance_ass_item(inst, i, item)
	PyInstanceObject *inst;
	int i;
	PyObject *item;
{
	PyObject *func, *arg, *res;

	if (item == NULL) {
		if (delitemstr == NULL)
			delitemstr = PyString_InternFromString("__delitem__");
		func = instance_getattr(inst, delitemstr);
	}
	else {
		if (setitemstr == NULL)
			setitemstr = PyString_InternFromString("__setitem__");
		func = instance_getattr(inst, setitemstr);
	}
	if (func == NULL)
		return -1;
	if (item == NULL)
		arg = Py_BuildValue("i", i);
	else
		arg = Py_BuildValue("(iO)", i, item);
	if (arg == NULL) {
		Py_DECREF(func);
		return -1;
	}
	res = PyEval_CallObject(func, arg);
	Py_DECREF(func);
	Py_DECREF(arg);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static int
instance_ass_slice(inst, i, j, value)
	PyInstanceObject *inst;
	int i, j;
	PyObject *value;
{
	PyObject *func, *arg, *res;
	static PyObject *setslicestr, *delslicestr;

	if (value == NULL) {
		if (delslicestr == NULL)
			delslicestr =
				PyString_InternFromString("__delslice__");
		func = instance_getattr(inst, delslicestr);
	}
	else {
		if (setslicestr == NULL)
			setslicestr =
				PyString_InternFromString("__setslice__");
		func = instance_getattr(inst, setslicestr);
	}
	if (func == NULL)
		return -1;
	if (value == NULL)
		arg = Py_BuildValue("(ii)", i, j);
	else
		arg = Py_BuildValue("(iiO)", i, j, value);
	if (arg == NULL) {
		Py_DECREF(func);
		return -1;
	}
	res = PyEval_CallObject(func, arg);
	Py_DECREF(func);
	Py_DECREF(arg);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static PySequenceMethods instance_as_sequence = {
	(inquiry)instance_length, /*sq_length*/
	0, /*sq_concat*/
	0, /*sq_repeat*/
	(intargfunc)instance_item, /*sq_item*/
	(intintargfunc)instance_slice, /*sq_slice*/
	(intobjargproc)instance_ass_item, /*sq_ass_item*/
	(intintobjargproc)instance_ass_slice, /*sq_ass_slice*/
};

static PyObject *
generic_unary_op(self, methodname)
	PyInstanceObject *self;
	PyObject *methodname;
{
	PyObject *func, *res;

	if ((func = instance_getattr(self, methodname)) == NULL)
		return NULL;
	res = PyEval_CallObject(func, (PyObject *)NULL);
	Py_DECREF(func);
	return res;
}


/* Forward */
static int halfbinop Py_PROTO((PyObject *, PyObject *, char *, PyObject **,
		PyObject * (*) Py_PROTO((PyObject *, PyObject *)), int ));


/* Implement a binary operator involving at least one class instance. */

PyObject *
PyInstance_DoBinOp(v, w, opname, ropname, thisfunc)
	PyObject *v;
	PyObject *w;
	char *opname;
	char *ropname;
	PyObject * (*thisfunc) Py_PROTO((PyObject *, PyObject *));
{
	char buf[256];
	PyObject *result = NULL;
	if (halfbinop(v, w, opname, &result, thisfunc, 0) <= 0)
		return result;
	if (halfbinop(w, v, ropname, &result, thisfunc, 1) <= 0)
		return result;
	sprintf(buf, "%s nor %s defined for these operands", opname, ropname);
	PyErr_SetString(PyExc_TypeError, buf);
	return NULL;
}


/* Try one half of a binary operator involving a class instance.
   Return value:
   -1 if an exception is to be reported right away
   0  if we have a valid result
   1  if we could try another operation
*/

static PyObject *coerce_obj;

static int
halfbinop(v, w, opname, r_result, thisfunc, swapped)
	PyObject *v;
	PyObject *w;
	char *opname;
	PyObject **r_result;
	PyObject * (*thisfunc) Py_PROTO((PyObject *, PyObject *));
	int swapped;
{
	PyObject *func;
	PyObject *args;
	PyObject *PyNumber_Coerce;
	PyObject *coerced = NULL;
	PyObject *v1;
	
	if (!PyInstance_Check(v))
		return 1;
	if (coerce_obj == NULL) {
		coerce_obj = PyString_InternFromString("__coerce__");
		if (coerce_obj == NULL)
			return -1;
	}
	PyNumber_Coerce = PyObject_GetAttr(v, coerce_obj);
	if (PyNumber_Coerce == NULL) {
		PyErr_Clear();
	}
	else {
		args = Py_BuildValue("(O)", w);
		if (args == NULL) {
			return -1;
		}
		coerced = PyEval_CallObject(PyNumber_Coerce, args);
		Py_DECREF(args);
		Py_DECREF(PyNumber_Coerce);
		if (coerced == NULL) {
			return -1;
		}
		if (coerced == Py_None) {
			Py_DECREF(coerced);
			return 1;
		}
		if (!PyTuple_Check(coerced) || PyTuple_Size(coerced) != 2) {
			Py_DECREF(coerced);
			PyErr_SetString(PyExc_TypeError,
				   "coercion should return None or 2-tuple");
			return -1;
		}
		v1 = PyTuple_GetItem(coerced, 0);
		w = PyTuple_GetItem(coerced, 1);
		if (v1 != v) {
			v = v1;
			if (!PyInstance_Check(v) && !PyInstance_Check(w)) {
				if (swapped)
					*r_result = (*thisfunc)(w, v);
				else
					*r_result = (*thisfunc)(v, w);
				Py_DECREF(coerced);
				return *r_result == NULL ? -1 : 0;
			}
		}
		w = PyTuple_GetItem(coerced, 1);
	}
	func = PyObject_GetAttrString(v, opname);
	if (func == NULL) {
		Py_XDECREF(coerced);
		if (PyErr_Occurred() != PyExc_AttributeError)
			return -1;
		PyErr_Clear();
		return 1;
	}
	args = Py_BuildValue("(O)", w);
	if (args == NULL) {
		Py_DECREF(func);
		Py_XDECREF(coerced);
		return -1;
	}
	*r_result = PyEval_CallObject(func, args);
	Py_DECREF(args);
	Py_DECREF(func);
	Py_XDECREF(coerced);
	return *r_result == NULL ? -1 : 0;
}

static int
instance_coerce(pv, pw)
	PyObject **pv;
	PyObject **pw;
{
	PyObject *v = *pv;
	PyObject *w = *pw;
	PyObject *PyNumber_Coerce;
	PyObject *args;
	PyObject *coerced;

	if (coerce_obj == NULL) {
		coerce_obj = PyString_InternFromString("__coerce__");
		if (coerce_obj == NULL)
			return -1;
	}
	PyNumber_Coerce = PyObject_GetAttr(v, coerce_obj);
	if (PyNumber_Coerce == NULL) {
		/* No __coerce__ method: always OK */
		PyErr_Clear();
		Py_INCREF(v);
		Py_INCREF(w);
		return 0;
	}
	/* Has __coerce__ method: call it */
	args = Py_BuildValue("(O)", w);
	if (args == NULL) {
		return -1;
	}
	coerced = PyEval_CallObject(PyNumber_Coerce, args);
	Py_DECREF(args);
	Py_DECREF(PyNumber_Coerce);
	if (coerced == NULL) {
		/* __coerce__ call raised an exception */
		return -1;
	}
	if (coerced == Py_None) {
		/* __coerce__ says "I can't do it" */
		Py_DECREF(coerced);
		return 1;
	}
	if (!PyTuple_Check(coerced) || PyTuple_Size(coerced) != 2) {
		/* __coerce__ return value is malformed */
		Py_DECREF(coerced);
		PyErr_SetString(PyExc_TypeError,
			   "coercion should return None or 2-tuple");
		return -1;
	}
	/* __coerce__ returned two new values */
	*pv = PyTuple_GetItem(coerced, 0);
	*pw = PyTuple_GetItem(coerced, 1);
	Py_INCREF(*pv);
	Py_INCREF(*pw);
	Py_DECREF(coerced);
	return 0;
}


#define UNARY(funcname, methodname) \
static PyObject *funcname(self) PyInstanceObject *self; { \
	static PyObject *o; \
	if (o == NULL) o = PyString_InternFromString(methodname); \
	return generic_unary_op(self, o); \
}

UNARY(instance_neg, "__neg__")
UNARY(instance_pos, "__pos__")
UNARY(instance_abs, "__abs__")

static int
instance_nonzero(self)
	PyInstanceObject *self;
{
	PyObject *func, *res;
	long outcome;
	static PyObject *nonzerostr;

	if (nonzerostr == NULL)
		nonzerostr = PyString_InternFromString("__nonzero__");
	if ((func = instance_getattr(self, nonzerostr)) == NULL) {
		PyErr_Clear();
		if (lenstr == NULL)
			lenstr = PyString_InternFromString("__len__");
		if ((func = instance_getattr(self, lenstr)) == NULL) {
			PyErr_Clear();
			/* Fall back to the default behavior:
			   all instances are nonzero */
			return 1;
		}
	}
	res = PyEval_CallObject(func, (PyObject *)NULL);
	Py_DECREF(func);
	if (res == NULL)
		return -1;
	if (!PyInt_Check(res)) {
		Py_DECREF(res);
		PyErr_SetString(PyExc_TypeError,
				"__nonzero__ should return an int");
		return -1;
	}
	outcome = PyInt_AsLong(res);
	Py_DECREF(res);
	if (outcome < 0) {
		PyErr_SetString(PyExc_ValueError,
				"__nonzero__ should return >= 0");
		return -1;
	}
	return outcome > 0;
}

UNARY(instance_invert, "__invert__")
UNARY(instance_int, "__int__")
UNARY(instance_long, "__long__")
UNARY(instance_float, "__float__")
UNARY(instance_oct, "__oct__")
UNARY(instance_hex, "__hex__")

/* This version is for ternary calls only (z != None) */
static PyObject *
instance_pow(v, w, z)
	PyObject *v;
	PyObject *w;
	PyObject *z;
{
	/* XXX Doesn't do coercions... */
	PyObject *func;
	PyObject *args;
	PyObject *result;
	static PyObject *powstr;

	if (powstr == NULL)
		powstr = PyString_InternFromString("__pow__");
	func = PyObject_GetAttr(v, powstr);
	if (func == NULL)
		return NULL;
	args = Py_BuildValue("(OO)", w, z);
	if (args == NULL) {
		Py_DECREF(func);
		return NULL;
	}
	result = PyEval_CallObject(func, args);
	Py_DECREF(func);
	Py_DECREF(args);
	return result;
}

static PyNumberMethods instance_as_number = {
	0, /*nb_add*/
	0, /*nb_subtract*/
	0, /*nb_multiply*/
	0, /*nb_divide*/
	0, /*nb_remainder*/
	0, /*nb_divmod*/
	(ternaryfunc)instance_pow, /*nb_power*/
	(unaryfunc)instance_neg, /*nb_negative*/
	(unaryfunc)instance_pos, /*nb_positive*/
	(unaryfunc)instance_abs, /*nb_absolute*/
	(inquiry)instance_nonzero, /*nb_nonzero*/
	(unaryfunc)instance_invert, /*nb_invert*/
	0, /*nb_lshift*/
	0, /*nb_rshift*/
	0, /*nb_and*/
	0, /*nb_xor*/
	0, /*nb_or*/
	(coercion)instance_coerce, /*nb_coerce*/
	(unaryfunc)instance_int, /*nb_int*/
	(unaryfunc)instance_long, /*nb_long*/
	(unaryfunc)instance_float, /*nb_float*/
	(unaryfunc)instance_oct, /*nb_oct*/
	(unaryfunc)instance_hex, /*nb_hex*/
};

PyTypeObject PyInstance_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"instance",
	sizeof(PyInstanceObject),
	0,
	(destructor)instance_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	instance_compare,	/*tp_compare*/
	(reprfunc)instance_repr, /*tp_repr*/
	&instance_as_number,	/*tp_as_number*/
	&instance_as_sequence,	/*tp_as_sequence*/
	&instance_as_mapping,	/*tp_as_mapping*/
	(hashfunc)instance_hash, /*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	(getattrofunc)instance_getattr, /*tp_getattro*/
	(setattrofunc)instance_setattr, /*tp_setattro*/
};


/* Instance method objects are used for two purposes:
   (a) as bound instance methods (returned by instancename.methodname)
   (b) as unbound methods (returned by ClassName.methodname)
   In case (b), im_self is NULL
*/

typedef struct {
	PyObject_HEAD
	PyObject *im_func;	/* The function implementing the method */
	PyObject *im_self;	/* The instance it is bound to, or NULL */
	PyObject *im_class;	/* The class that defined the method */
} PyMethodObject;

PyObject *
PyMethod_New(func, self, class)
	PyObject *func;
	PyObject *self;
	PyObject *class;
{
	register PyMethodObject *im;
	if (!PyFunction_Check(func)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	im = PyObject_NEW(PyMethodObject, &PyMethod_Type);
	if (im == NULL)
		return NULL;
	Py_INCREF(func);
	im->im_func = func;
	Py_XINCREF(self);
	im->im_self = self;
	Py_INCREF(class);
	im->im_class = class;
	return (PyObject *)im;
}

PyObject *
PyMethod_Function(im)
	register PyObject *im;
{
	if (!PyMethod_Check(im)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyMethodObject *)im)->im_func;
}

PyObject *
PyMethod_Self(im)
	register PyObject *im;
{
	if (!PyMethod_Check(im)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyMethodObject *)im)->im_self;
}

PyObject *
PyMethod_Class(im)
	register PyObject *im;
{
	if (!PyMethod_Check(im)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyMethodObject *)im)->im_class;
}

/* Class method methods */

#define OFF(x) offsetof(PyMethodObject, x)

static struct memberlist instancemethod_memberlist[] = {
	{"im_func",	T_OBJECT,	OFF(im_func)},
	{"im_self",	T_OBJECT,	OFF(im_self)},
	{"im_class",	T_OBJECT,	OFF(im_class)},
	/* Dummies that are not handled by getattr() except for __members__ */
	{"__doc__",	T_INT,		0},
	{"__name__",	T_INT,		0},
	{NULL}	/* Sentinel */
};

static PyObject *
instancemethod_getattr(im, name)
	register PyMethodObject *im;
	PyObject *name;
{
	char *sname = PyString_AsString(name);
	if (sname[0] == '_') {
		PyFunctionObject *func = (PyFunctionObject *)(im->im_func);
		if (strcmp(sname, "__name__") == 0) {
			Py_INCREF(func->func_name);
			return func->func_name;
		}
		if (strcmp(sname, "__doc__") == 0) {
			Py_INCREF(func->func_doc);
			return func->func_doc;
		}
	}
	if (PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
	    "instance-method attributes not accessible in restricted mode");
		return NULL;
	}
	return PyMember_Get((char *)im, instancemethod_memberlist, sname);
}

static void
instancemethod_dealloc(im)
	register PyMethodObject *im;
{
	Py_DECREF(im->im_func);
	Py_XDECREF(im->im_self);
	Py_DECREF(im->im_class);
	free((ANY *)im);
}

static int
instancemethod_compare(a, b)
	PyMethodObject *a, *b;
{
	if (a->im_self != b->im_self)
		return (a->im_self < b->im_self) ? -1 : 1;
	return PyObject_Compare(a->im_func, b->im_func);
}

static PyObject *
instancemethod_repr(a)
	PyMethodObject *a;
{
	char buf[240];
	PyInstanceObject *self = (PyInstanceObject *)(a->im_self);
	PyFunctionObject *func = (PyFunctionObject *)(a->im_func);
	PyClassObject *class = (PyClassObject *)(a->im_class);
	PyObject *fclassname, *iclassname, *funcname;
	char *fcname, *icname, *fname;
	fclassname = class->cl_name;
	funcname = func->func_name;
	if (fclassname != NULL && PyString_Check(fclassname))
		fcname = PyString_AsString(fclassname);
	else
		fcname = "?";
	if (funcname != NULL && PyString_Check(funcname))
		fname = PyString_AsString(funcname);
	else
		fname = "?";
	if (self == NULL)
		sprintf(buf, "<unbound method %.100s.%.100s>", fcname, fname);
	else {
		iclassname = self->in_class->cl_name;
		if (iclassname != NULL && PyString_Check(iclassname))
			icname = PyString_AsString(iclassname);
		else
			icname = "?";
		sprintf(buf, "<method %.60s.%.60s of %.60s instance at %lx>",
			fcname, fname, icname, (long)self);
	}
	return PyString_FromString(buf);
}

static long
instancemethod_hash(a)
	PyMethodObject *a;
{
	long x, y;
	if (a->im_self == NULL)
		x = PyObject_Hash(Py_None);
	else
		x = PyObject_Hash(a->im_self);
	if (x == -1)
		return -1;
	y = PyObject_Hash(a->im_func);
	if (y == -1)
		return -1;
	return x ^ y;
}

PyTypeObject PyMethod_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"instance method",
	sizeof(PyMethodObject),
	0,
	(destructor)instancemethod_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)instancemethod_compare, /*tp_compare*/
	(reprfunc)instancemethod_repr, /*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	(hashfunc)instancemethod_hash, /*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	(getattrofunc)instancemethod_getattr, /*tp_getattro*/
	0,			/*tp_setattro*/
};
