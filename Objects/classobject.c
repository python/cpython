
/* Class object implementation */

#include "Python.h"
#include "structmember.h"

/* Forward */
static PyObject *class_lookup(PyClassObject *, PyObject *,
			      PyClassObject **);
static PyObject *instance_getattr1(PyInstanceObject *, PyObject *);
static PyObject *instance_getattr2(PyInstanceObject *, PyObject *);

static PyObject *getattrstr, *setattrstr, *delattrstr;


PyObject *
PyClass_New(PyObject *bases, PyObject *dict, PyObject *name)
     /* bases is NULL or tuple of classobjects! */
{
	PyClassObject *op, *dummy;
	static PyObject *docstr, *modstr, *namestr;
	if (docstr == NULL) {
		docstr= PyString_InternFromString("__doc__");
		if (docstr == NULL)
			return NULL;
	}
	if (modstr == NULL) {
		modstr= PyString_InternFromString("__module__");
		if (modstr == NULL)
			return NULL;
	}
	if (namestr == NULL) {
		namestr= PyString_InternFromString("__name__");
		if (namestr == NULL)
			return NULL;
	}
	if (name == NULL || !PyString_Check(name)) {
		PyErr_SetString(PyExc_SystemError,
				"PyClass_New: name must be a string");
		return NULL;
	}
	if (dict == NULL || !PyDict_Check(dict)) {
		PyErr_SetString(PyExc_SystemError,
				"PyClass_New: dict must be a dictionary");
		return NULL;
	}
	if (PyDict_GetItem(dict, docstr) == NULL) {
		if (PyDict_SetItem(dict, docstr, Py_None) < 0)
			return NULL;
	}
	if (PyDict_GetItem(dict, modstr) == NULL) {
		PyObject *globals = PyEval_GetGlobals();
		if (globals != NULL) {
			PyObject *modname = PyDict_GetItem(globals, namestr);
			if (modname != NULL) {
				if (PyDict_SetItem(dict, modstr, modname) < 0)
					return NULL;
			}
		}
	}
	if (bases == NULL) {
		bases = PyTuple_New(0);
		if (bases == NULL)
			return NULL;
	}
	else {
		int i;
		if (!PyTuple_Check(bases)) {
			PyErr_SetString(PyExc_SystemError,
					"PyClass_New: bases must be a tuple");
			return NULL;
		}
		i = PyTuple_Size(bases);
		while (--i >= 0) {
			if (!PyClass_Check(PyTuple_GetItem(bases, i))) {
				PyErr_SetString(PyExc_SystemError,
					"PyClass_New: base must be a class");
				return NULL;
			}
		}
		Py_INCREF(bases);
	}
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
	PyObject_GC_Init(op);
	return (PyObject *) op;
}

/* Class methods */

static void
class_dealloc(PyClassObject *op)
{
	PyObject_GC_Fini(op);
	Py_DECREF(op->cl_bases);
	Py_DECREF(op->cl_dict);
	Py_XDECREF(op->cl_name);
	Py_XDECREF(op->cl_getattr);
	Py_XDECREF(op->cl_setattr);
	Py_XDECREF(op->cl_delattr);
	op = (PyClassObject *) PyObject_AS_GC(op);
	PyObject_DEL(op);
}

static PyObject *
class_lookup(PyClassObject *cp, PyObject *name, PyClassObject **pclass)
{
	int i, n;
	PyObject *value = PyDict_GetItem(cp->cl_dict, name);
	if (value != NULL) {
		*pclass = cp;
		return value;
	}
	n = PyTuple_Size(cp->cl_bases);
	for (i = 0; i < n; i++) {
		/* XXX What if one of the bases is not a class? */
		PyObject *v = class_lookup(
			(PyClassObject *)
			PyTuple_GetItem(cp->cl_bases, i), name, pclass);
		if (v != NULL)
			return v;
	}
	return NULL;
}

static PyObject *
class_getattr(register PyClassObject *op, PyObject *name)
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
	Py_INCREF(v);
	if (PyFunction_Check(v)) {
		PyObject *w = PyMethod_New(v, (PyObject *)NULL,
						    (PyObject *)class);
		Py_DECREF(v);
		v = w;
	}
	return v;
}

static void
set_slot(PyObject **slot, PyObject *v)
{
	PyObject *temp = *slot;
	Py_XINCREF(v);
	*slot = v;
	Py_XDECREF(temp);
}

static void
set_attr_slots(PyClassObject *c)
{
	PyClassObject *dummy;

	set_slot(&c->cl_getattr, class_lookup(c, getattrstr, &dummy));
	set_slot(&c->cl_setattr, class_lookup(c, setattrstr, &dummy));
	set_slot(&c->cl_delattr, class_lookup(c, delattrstr, &dummy));
}

static char *
set_dict(PyClassObject *c, PyObject *v)
{
	if (v == NULL || !PyDict_Check(v))
		return "__dict__ must be a dictionary object";
	set_slot(&c->cl_dict, v);
	set_attr_slots(c);
	return "";
}

static char *
set_bases(PyClassObject *c, PyObject *v)
{
	int i, n;

	if (v == NULL || !PyTuple_Check(v))
		return "__bases__ must be a tuple object";
	n = PyTuple_Size(v);
	for (i = 0; i < n; i++) {
		PyObject *x = PyTuple_GET_ITEM(v, i);
		if (!PyClass_Check(x))
			return "__bases__ items must be classes";
		if (PyClass_IsSubclass(x, (PyObject *)c))
			return "a __bases__ item causes an inheritance cycle";
	}
	set_slot(&c->cl_bases, v);
	set_attr_slots(c);
	return "";
}

static char *
set_name(PyClassObject *c, PyObject *v)
{
	if (v == NULL || !PyString_Check(v))
		return "__name__ must be a string object";
	if (strlen(PyString_AS_STRING(v)) != (size_t)PyString_GET_SIZE(v))
		return "__name__ must not contain null bytes";
	set_slot(&c->cl_name, v);
	return "";
}

static int
class_setattr(PyClassObject *op, PyObject *name, PyObject *v)
{
	char *sname;
	if (PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
			   "classes are read-only in restricted mode");
		return -1;
	}
	sname = PyString_AsString(name);
	if (sname[0] == '_' && sname[1] == '_') {
		int n = PyString_Size(name);
		if (sname[n-1] == '_' && sname[n-2] == '_') {
			char *err = NULL;
			if (strcmp(sname, "__dict__") == 0)
				err = set_dict(op, v);
			else if (strcmp(sname, "__bases__") == 0)
				err = set_bases(op, v);
			else if (strcmp(sname, "__name__") == 0)
				err = set_name(op, v);
			else if (strcmp(sname, "__getattr__") == 0)
				set_slot(&op->cl_getattr, v);
			else if (strcmp(sname, "__setattr__") == 0)
				set_slot(&op->cl_setattr, v);
			else if (strcmp(sname, "__delattr__") == 0)
				set_slot(&op->cl_delattr, v);
			/* For the last three, we fall through to update the
			   dictionary as well. */
			if (err != NULL) {
				if (*err == '\0')
					return 0;
				PyErr_SetString(PyExc_TypeError, err);
				return -1;
			}
		}
	}
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
class_repr(PyClassObject *op)
{
	PyObject *mod = PyDict_GetItemString(op->cl_dict, "__module__");
	char buf[140];
	char *name;
	if (op->cl_name == NULL || !PyString_Check(op->cl_name))
		name = "?";
	else
		name = PyString_AsString(op->cl_name);
	if (mod == NULL || !PyString_Check(mod))
		sprintf(buf, "<class ?.%.100s at %p>", name, op);
	else
		sprintf(buf, "<class %.50s.%.50s at %p>",
			PyString_AsString(mod),
			name, op);
	return PyString_FromString(buf);
}

static PyObject *
class_str(PyClassObject *op)
{
	PyObject *mod = PyDict_GetItemString(op->cl_dict, "__module__");
	PyObject *name = op->cl_name;
	PyObject *res;
	int m, n;

	if (name == NULL || !PyString_Check(name))
		return class_repr(op);
	if (mod == NULL || !PyString_Check(mod)) {
		Py_INCREF(name);
		return name;
	}
	m = PyString_Size(mod);
	n = PyString_Size(name);
	res = PyString_FromStringAndSize((char *)NULL, m+1+n);
	if (res != NULL) {
		char *s = PyString_AsString(res);
		memcpy(s, PyString_AsString(mod), m);
		s += m;
		*s++ = '.';
		memcpy(s, PyString_AsString(name), n);
	}
	return res;
}

static int
class_traverse(PyClassObject *o, visitproc visit, void *arg)
{
	int err;
	if (o->cl_bases) {
		err = visit(o->cl_bases, arg);
		if (err)
			return err;
	}
	if (o->cl_dict) {
		err = visit(o->cl_dict, arg);
		if (err)
			return err;
	}
	if (o->cl_name) {
		err = visit(o->cl_name, arg);
		if (err)
			return err;
	}
	if (o->cl_getattr) {
		err = visit(o->cl_getattr, arg);
		if (err)
			return err;
	}
	if (o->cl_setattr) {
		err = visit(o->cl_setattr, arg);
		if (err)
			return err;
	}
	if (o->cl_delattr) {
		err = visit(o->cl_delattr, arg);
		if (err)
			return err;
	}
	return 0;
}

PyTypeObject PyClass_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"class",
	sizeof(PyClassObject) + PyGC_HEAD_SIZE,
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
	(reprfunc)class_str, /*tp_str*/
	(getattrofunc)class_getattr, /*tp_getattro*/
	(setattrofunc)class_setattr, /*tp_setattro*/
	0,		/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC, /*tp_flags*/
	0,		/* tp_doc */
	(traverseproc)class_traverse,	/* tp_traverse */
};

int
PyClass_IsSubclass(PyObject *class, PyObject *base)
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

PyObject *
PyInstance_New(PyObject *class, PyObject *arg, PyObject *kw)
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
	inst->in_dict = PyDict_New();
	if (inst->in_dict == NULL) {
		inst = (PyInstanceObject *) PyObject_AS_GC(inst);
		PyObject_DEL(inst);
		return NULL;
	}
	Py_INCREF(class);
	inst->in_class = (PyClassObject *)class;
	PyObject_GC_Init(inst);
	if (initstr == NULL)
		initstr = PyString_InternFromString("__init__");
	init = instance_getattr2(inst, initstr);
	if (init == NULL) {
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
instance_dealloc(register PyInstanceObject *inst)
{
	PyObject *error_type, *error_value, *error_traceback;
	PyObject *del;
	static PyObject *delstr;
#ifdef Py_REF_DEBUG
	extern long _Py_RefTotal;
#endif
	/* Temporarily resurrect the object. */
#ifdef Py_TRACE_REFS
#ifndef Py_REF_DEBUG
#   error "Py_TRACE_REFS defined but Py_REF_DEBUG not."
#endif
	/* much too complicated if Py_TRACE_REFS defined */
	inst->ob_type = &PyInstance_Type;
	_Py_NewReference((PyObject *)inst);
#ifdef COUNT_ALLOCS
	/* compensate for boost in _Py_NewReference; note that
	 * _Py_RefTotal was also boosted; we'll knock that down later.
	 */
	inst->ob_type->tp_alloc--;
#endif
#else /* !Py_TRACE_REFS */
	/* Py_INCREF boosts _Py_RefTotal if Py_REF_DEBUG is defined */
	Py_INCREF(inst);
#endif /* !Py_TRACE_REFS */

	/* Save the current exception, if any. */
	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	/* Execute __del__ method, if any. */
	if (delstr == NULL)
		delstr = PyString_InternFromString("__del__");
	if ((del = instance_getattr2(inst, delstr)) != NULL) {
		PyObject *res = PyEval_CallObject(del, (PyObject *)NULL);
		if (res == NULL)
			PyErr_WriteUnraisable(del);
		else
			Py_DECREF(res);
		Py_DECREF(del);
	}
	/* Restore the saved exception. */
	PyErr_Restore(error_type, error_value, error_traceback);
	/* Undo the temporary resurrection; can't use DECREF here, it would
	 * cause a recursive call.
	 */
#ifdef Py_REF_DEBUG
	/* _Py_RefTotal was boosted either by _Py_NewReference or
	 * Py_INCREF above.
	 */
	_Py_RefTotal--;
#endif
	if (--inst->ob_refcnt > 0) {
#ifdef COUNT_ALLOCS
		inst->ob_type->tp_free--;
#endif
		return; /* __del__ added a reference; don't delete now */
	}
#ifdef Py_TRACE_REFS
	_Py_ForgetReference((PyObject *)inst);
#ifdef COUNT_ALLOCS
	/* compensate for increment in _Py_ForgetReference */
	inst->ob_type->tp_free--;
#endif
#ifndef WITH_CYCLE_GC
	inst->ob_type = NULL;
#endif
#endif
	PyObject_GC_Fini(inst);
	Py_DECREF(inst->in_class);
	Py_XDECREF(inst->in_dict);
	inst = (PyInstanceObject *) PyObject_AS_GC(inst);
	PyObject_DEL(inst);
}

static PyObject *
instance_getattr1(register PyInstanceObject *inst, PyObject *name)
{
	register PyObject *v;
	register char *sname = PyString_AsString(name);
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
	v = instance_getattr2(inst, name);
	if (v == NULL) {
		PyErr_Format(PyExc_AttributeError,"'%.50s' instance has no attribute '%.400s'",
			     PyString_AS_STRING(inst->in_class->cl_name), sname);
	}
	return v;
}

static PyObject *
instance_getattr2(register PyInstanceObject *inst, PyObject *name)
{
	register PyObject *v;
	PyClassObject *class;
	class = NULL;
	v = PyDict_GetItem(inst->in_dict, name);
	if (v == NULL) {
		v = class_lookup(inst->in_class, name, &class);
		if (v == NULL)
			return v;
	}
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
instance_getattr(register PyInstanceObject *inst, PyObject *name)
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
instance_setattr1(PyInstanceObject *inst, PyObject *name, PyObject *v)
{
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
instance_setattr(PyInstanceObject *inst, PyObject *name, PyObject *v)
{
	PyObject *func, *args, *res, *tmp;
	char *sname = PyString_AsString(name);
	if (sname[0] == '_' && sname[1] == '_') {
		int n = PyString_Size(name);
		if (sname[n-1] == '_' && sname[n-2] == '_') {
			if (strcmp(sname, "__dict__") == 0) {
				if (PyEval_GetRestricted()) {
					PyErr_SetString(PyExc_RuntimeError,
				 "__dict__ not accessible in restricted mode");
					return -1;
				}
				if (v == NULL || !PyDict_Check(v)) {
				    PyErr_SetString(PyExc_TypeError,
				       "__dict__ must be set to a dictionary");
				    return -1;
				}
				tmp = inst->in_dict;
				Py_INCREF(v);
				inst->in_dict = v;
				Py_DECREF(tmp);
				return 0;
			}
			if (strcmp(sname, "__class__") == 0) {
				if (PyEval_GetRestricted()) {
					PyErr_SetString(PyExc_RuntimeError,
				"__class__ not accessible in restricted mode");
					return -1;
				}
				if (v == NULL || !PyClass_Check(v)) {
					PyErr_SetString(PyExc_TypeError,
					   "__class__ must be set to a class");
					return -1;
				}
				tmp = (PyObject *)(inst->in_class);
				Py_INCREF(v);
				inst->in_class = (PyClassObject *)v;
				Py_DECREF(tmp);
				return 0;
			}
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
instance_repr(PyInstanceObject *inst)
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
		PyObject *mod = PyDict_GetItemString(
			inst->in_class->cl_dict, "__module__");
		char *cname;
		if (classname != NULL && PyString_Check(classname))
			cname = PyString_AsString(classname);
		else
			cname = "?";
		PyErr_Clear();
		if (mod == NULL || !PyString_Check(mod))
			sprintf(buf, "<?.%.100s instance at %p>",
				cname, inst);
		else
			sprintf(buf, "<%.50s.%.50s instance at %p>",
				PyString_AsString(mod),
				cname, inst);
		return PyString_FromString(buf);
	}
	res = PyEval_CallObject(func, (PyObject *)NULL);
	Py_DECREF(func);
	return res;
}

static PyObject *
instance_compare1(PyObject *inst, PyObject *other)
{
	return PyInstance_DoBinOp(inst, other, "__cmp__", "__rcmp__",
			     instance_compare1);
}

static int
instance_compare(PyObject *inst, PyObject *other)
{
	PyObject *result;
	long outcome;
	result = instance_compare1(inst, other);
	if (result == NULL)
		return -1;
	if (!PyInt_Check(result)) {
		Py_DECREF(result);
		PyErr_SetString(PyExc_TypeError,
				"comparison did not return an int");
		return -1;
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
instance_hash(PyInstanceObject *inst)
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
			return _Py_HashPointer(inst);
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

static int
instance_traverse(PyInstanceObject *o, visitproc visit, void *arg)
{
	int err;
	if (o->in_class) {
		err = visit((PyObject *)(o->in_class), arg);
		if (err)
			return err;
	}
	if (o->in_dict) {
		err = visit(o->in_dict, arg);
		if (err)
			return err;
	}
	return 0;
}

static PyObject *getitemstr, *setitemstr, *delitemstr, *lenstr;

static int
instance_length(PyInstanceObject *inst)
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
instance_subscript(PyInstanceObject *inst, PyObject *key)
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
instance_ass_subscript(PyInstanceObject *inst, PyObject *key, PyObject *value)
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
instance_item(PyInstanceObject *inst, int i)
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
sliceobj_from_intint(int i, int j)
{
	PyObject *start, *end, *res;

	start = PyInt_FromLong((long)i);
	if (!start)
		return NULL;
	
	end = PyInt_FromLong((long)j);
	if (!end) {
		Py_DECREF(start);
		return NULL;
	}
	res = PySlice_New(start, end, NULL);
	Py_DECREF(start);
	Py_DECREF(end);
	return res;
}


static PyObject *
instance_slice(PyInstanceObject *inst, int i, int j)
{
	PyObject *func, *arg, *res;
	static PyObject *getslicestr;

	if (getslicestr == NULL)
		getslicestr = PyString_InternFromString("__getslice__");
	func = instance_getattr(inst, getslicestr);

	if (func == NULL) {
		PyErr_Clear();

		if (getitemstr == NULL)
			getitemstr = PyString_InternFromString("__getitem__");
		func = instance_getattr(inst, getitemstr);
		if (func == NULL)
			return NULL;
		arg = Py_BuildValue("(N)", sliceobj_from_intint(i, j));
	} else 
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
instance_ass_item(PyInstanceObject *inst, int i, PyObject *item)
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
instance_ass_slice(PyInstanceObject *inst, int i, int j, PyObject *value)
{
	PyObject *func, *arg, *res;
	static PyObject *setslicestr, *delslicestr;

	if (value == NULL) {
		if (delslicestr == NULL)
			delslicestr =
				PyString_InternFromString("__delslice__");
		func = instance_getattr(inst, delslicestr);
		if (func == NULL) {
			PyErr_Clear();
			if (delitemstr == NULL)
				delitemstr =
				    PyString_InternFromString("__delitem__");
			func = instance_getattr(inst, delitemstr);
			if (func == NULL)
				return -1;

			arg = Py_BuildValue("(N)",
					    sliceobj_from_intint(i, j));
		} else
			arg = Py_BuildValue("(ii)", i, j);
	}
	else {
		if (setslicestr == NULL)
			setslicestr =
				PyString_InternFromString("__setslice__");
		func = instance_getattr(inst, setslicestr);
		if (func == NULL) {
			PyErr_Clear();
			if (setitemstr == NULL)
				setitemstr =
				    PyString_InternFromString("__setitem__");
			func = instance_getattr(inst, setitemstr);
			if (func == NULL)
				return -1;

			arg = Py_BuildValue("(NO)",
					    sliceobj_from_intint(i, j), value);
		} else
			arg = Py_BuildValue("(iiO)", i, j, value);
	}
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

static int instance_contains(PyInstanceObject *inst, PyObject *member)
{
	static PyObject *__contains__;
	PyObject *func, *arg, *res;
	int ret;

	if(__contains__ == NULL) {
		__contains__ = PyString_InternFromString("__contains__");
		if(__contains__ == NULL)
			return -1;
	}
	func = instance_getattr(inst, __contains__);
	if(func == NULL) {
		/* fall back to previous behavior */
		int i, cmp_res;

		if(!PyErr_ExceptionMatches(PyExc_AttributeError))
			return -1;
		PyErr_Clear();
		for(i=0;;i++) {
			PyObject *obj = instance_item(inst, i);
			int ret = 0;

			if(obj == NULL) {
				if(!PyErr_ExceptionMatches(PyExc_IndexError))
					return -1;
				PyErr_Clear();
				return 0;
			}
			if(PyObject_Cmp(obj, member, &cmp_res) == -1)
				ret = -1;
			if(cmp_res == 0) 
				ret = 1;
			Py_DECREF(obj);
			if(ret)
				return ret;
		}
	}
	arg = Py_BuildValue("(O)", member);
	if(arg == NULL) {
		Py_DECREF(func);
		return -1;
	}
	res = PyEval_CallObject(func, arg);
	Py_DECREF(func);
	Py_DECREF(arg);
	if(res == NULL) 
		return -1;
	ret = PyObject_IsTrue(res);
	Py_DECREF(res);
	return ret;
}

static PySequenceMethods
instance_as_sequence = {
	(inquiry)instance_length, /*sq_length*/
	0, /*sq_concat*/
	0, /*sq_repeat*/
	(intargfunc)instance_item, /*sq_item*/
	(intintargfunc)instance_slice, /*sq_slice*/
	(intobjargproc)instance_ass_item, /*sq_ass_item*/
	(intintobjargproc)instance_ass_slice, /*sq_ass_slice*/
	(objobjproc)instance_contains, /* sq_contains */
};

static PyObject *
generic_unary_op(PyInstanceObject *self, PyObject *methodname)
{
	PyObject *func, *res;

	if ((func = instance_getattr(self, methodname)) == NULL)
		return NULL;
	res = PyEval_CallObject(func, (PyObject *)NULL);
	Py_DECREF(func);
	return res;
}


/* Implement a binary operator involving at least one class instance. */

PyObject *
PyInstance_DoBinOp(PyObject *v, PyObject *w, char *opname, char *ropname,
                   PyObject * (*thisfunc)(PyObject *, PyObject *))
{
	char buf[256];
	PyObject *result = NULL;

	if (PyInstance_HalfBinOp(v, w, opname, &result, thisfunc, 0) <= 0)
		return result;
	if (PyInstance_HalfBinOp(w, v, ropname, &result, thisfunc, 1) <= 0)
		return result;
	/* Sigh -- special case for comparisons */
	if (strcmp(opname, "__cmp__") == 0) {
		Py_uintptr_t iv = (Py_uintptr_t)v;
		Py_uintptr_t iw = (Py_uintptr_t)w;
		long c = (iv < iw) ? -1 : (iv > iw) ? 1 : 0;
		return PyInt_FromLong(c);
	}
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

int
PyInstance_HalfBinOp(PyObject *v, PyObject *w, char *opname, PyObject **r_result,
		     PyObject * (*thisfunc)(PyObject *, PyObject *), int swapped)
{
	PyObject *func;
	PyObject *args;
	PyObject *coercefunc;
	PyObject *coerced = NULL;
	PyObject *v1;
	
	if (!PyInstance_Check(v))
		return 1;
	if (coerce_obj == NULL) {
		coerce_obj = PyString_InternFromString("__coerce__");
		if (coerce_obj == NULL)
			return -1;
	}
	coercefunc = PyObject_GetAttr(v, coerce_obj);
	if (coercefunc == NULL) {
		PyErr_Clear();
	}
	else {
		args = Py_BuildValue("(O)", w);
		if (args == NULL) {
			return -1;
		}
		coerced = PyEval_CallObject(coercefunc, args);
		Py_DECREF(args);
		Py_DECREF(coercefunc);
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
	}
	func = PyObject_GetAttrString(v, opname);
	if (func == NULL) {
		Py_XDECREF(coerced);
		if (!PyErr_ExceptionMatches(PyExc_AttributeError))
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
instance_coerce(PyObject **pv, PyObject **pw)
{
	PyObject *v = *pv;
	PyObject *w = *pw;
	PyObject *coercefunc;
	PyObject *args;
	PyObject *coerced;

	if (coerce_obj == NULL) {
		coerce_obj = PyString_InternFromString("__coerce__");
		if (coerce_obj == NULL)
			return -1;
	}
	coercefunc = PyObject_GetAttr(v, coerce_obj);
	if (coercefunc == NULL) {
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
	coerced = PyEval_CallObject(coercefunc, args);
	Py_DECREF(args);
	Py_DECREF(coercefunc);
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
static PyObject *funcname(PyInstanceObject *self) { \
	static PyObject *o; \
	if (o == NULL) o = PyString_InternFromString(methodname); \
	return generic_unary_op(self, o); \
}

UNARY(instance_neg, "__neg__")
UNARY(instance_pos, "__pos__")
UNARY(instance_abs, "__abs__")

static int
instance_nonzero(PyInstanceObject *self)
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
instance_pow(PyObject *v, PyObject *w, PyObject *z)
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

static PyObject *
instance_inplace_pow(PyObject *v, PyObject *w, PyObject *z)
{
	/* XXX Doesn't do coercions... */
	PyObject *func;
	PyObject *args;
	PyObject *result;
	static PyObject *ipowstr;

	if (ipowstr == NULL)
		ipowstr = PyString_InternFromString("__ipow__");
	func = PyObject_GetAttr(v, ipowstr);
	if (func == NULL) {
		if (!PyErr_ExceptionMatches(PyExc_AttributeError))
			return NULL;
		PyErr_Clear();
		return instance_pow(v, w, z);
	}
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
	0, /*nb_inplace_add*/
	0, /*nb_inplace_subtract*/
	0, /*nb_inplace_multiply*/
	0, /*nb_inplace_divide*/
	0, /*nb_inplace_remainder*/
	(ternaryfunc)instance_inplace_pow, /*nb_inplace_power*/
	0, /*nb_inplace_lshift*/
	0, /*nb_inplace_rshift*/
	0, /*nb_inplace_and*/
	0, /*nb_inplace_xor*/
	0, /*nb_inplace_or*/
};

PyTypeObject PyInstance_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"instance",
	sizeof(PyInstanceObject) + PyGC_HEAD_SIZE,
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
        0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC, /*tp_flags*/
	0,		/* tp_doc */
	(traverseproc)instance_traverse,	/* tp_traverse */
};


/* Instance method objects are used for two purposes:
   (a) as bound instance methods (returned by instancename.methodname)
   (b) as unbound methods (returned by ClassName.methodname)
   In case (b), im_self is NULL
*/

static PyMethodObject *free_list;

PyObject *
PyMethod_New(PyObject *func, PyObject *self, PyObject *class)
{
	register PyMethodObject *im;
	if (!PyCallable_Check(func)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	im = free_list;
	if (im != NULL) {
		free_list = (PyMethodObject *)(im->im_self);
		PyObject_INIT(im, &PyMethod_Type);
	}
	else {
		im = PyObject_NEW(PyMethodObject, &PyMethod_Type);
		if (im == NULL)
			return NULL;
	}
	Py_INCREF(func);
	im->im_func = func;
	Py_XINCREF(self);
	im->im_self = self;
	Py_INCREF(class);
	im->im_class = class;
	PyObject_GC_Init(im);
	return (PyObject *)im;
}

PyObject *
PyMethod_Function(register PyObject *im)
{
	if (!PyMethod_Check(im)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyMethodObject *)im)->im_func;
}

PyObject *
PyMethod_Self(register PyObject *im)
{
	if (!PyMethod_Check(im)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyMethodObject *)im)->im_self;
}

PyObject *
PyMethod_Class(register PyObject *im)
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
instancemethod_getattr(register PyMethodObject *im, PyObject *name)
{
	char *sname = PyString_AsString(name);
	if (sname[0] == '_') {
		/* Inherit __name__ and __doc__ from the callable object
		   implementing the method */
	        if (strcmp(sname, "__name__") == 0 ||
		    strcmp(sname, "__doc__") == 0)
			return PyObject_GetAttr(im->im_func, name);
	}
	if (PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
	    "instance-method attributes not accessible in restricted mode");
		return NULL;
	}
	return PyMember_Get((char *)im, instancemethod_memberlist, sname);
}

static void
instancemethod_dealloc(register PyMethodObject *im)
{
	PyObject_GC_Fini(im);
	Py_DECREF(im->im_func);
	Py_XDECREF(im->im_self);
	Py_DECREF(im->im_class);
	im->im_self = (PyObject *)free_list;
	free_list = im;
}

static int
instancemethod_compare(PyMethodObject *a, PyMethodObject *b)
{
	if (a->im_self != b->im_self)
		return (a->im_self < b->im_self) ? -1 : 1;
	return PyObject_Compare(a->im_func, b->im_func);
}

static PyObject *
instancemethod_repr(PyMethodObject *a)
{
	char buf[240];
	PyInstanceObject *self = (PyInstanceObject *)(a->im_self);
	PyObject *func = a->im_func;
	PyClassObject *class = (PyClassObject *)(a->im_class);
	PyObject *fclassname, *iclassname, *funcname;
	char *fcname, *icname, *fname;
	fclassname = class->cl_name;
	if (PyFunction_Check(func)) {
		funcname = ((PyFunctionObject *)func)->func_name;
		Py_INCREF(funcname);
	}
	else {
		funcname = PyObject_GetAttrString(func,"__name__");
		if (funcname == NULL)
			PyErr_Clear();
	}
	if (funcname != NULL && PyString_Check(funcname))
		fname = PyString_AS_STRING(funcname);
	else
		fname = "?";
	if (fclassname != NULL && PyString_Check(fclassname))
		fcname = PyString_AsString(fclassname);
	else
		fcname = "?";
	if (self == NULL)
		sprintf(buf, "<unbound method %.100s.%.100s>", fcname, fname);
	else {
		iclassname = self->in_class->cl_name;
		if (iclassname != NULL && PyString_Check(iclassname))
			icname = PyString_AsString(iclassname);
		else
			icname = "?";
		sprintf(buf, "<method %.60s.%.60s of %.60s instance at %p>",
			fcname, fname, icname, self);
	}
	Py_XDECREF(funcname);
	return PyString_FromString(buf);
}

static long
instancemethod_hash(PyMethodObject *a)
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

static int
instancemethod_traverse(PyMethodObject *im, visitproc visit, void *arg)
{
	int err;
	if (im->im_func) {
		err = visit(im->im_func, arg);
		if (err)
			return err;
	}
	if (im->im_self) {
		err = visit(im->im_self, arg);
		if (err)
			return err;
	}
	if (im->im_class) {
		err = visit(im->im_class, arg);
		if (err)
			return err;
	}
	return 0;
}

PyTypeObject PyMethod_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"instance method",
	sizeof(PyMethodObject) + PyGC_HEAD_SIZE,
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
	0,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC, /*tp_flags*/
	0,			/* tp_doc */
	(traverseproc)instancemethod_traverse,	/* tp_traverse */
};

/* Clear out the free list */

void
PyMethod_Fini(void)
{
	while (free_list) {
		PyMethodObject *im = free_list;
		free_list = (PyMethodObject *)(im->im_self);
		im = (PyMethodObject *) PyObject_AS_GC(im);
		PyObject_DEL(im);
	}
}
