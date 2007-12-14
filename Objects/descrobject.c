/* Descriptors -- a new, flexible way to describe attributes */

#include "Python.h"
#include "structmember.h" /* Why is this not included in Python.h? */

static void
descr_dealloc(PyDescrObject *descr)
{
	_PyObject_GC_UNTRACK(descr);
	Py_XDECREF(descr->d_type);
	Py_XDECREF(descr->d_name);
	PyObject_GC_Del(descr);
}

static char *
descr_name(PyDescrObject *descr)
{
	if (descr->d_name != NULL && PyString_Check(descr->d_name))
		return PyString_AS_STRING(descr->d_name);
	else
		return "?";
}

static PyObject *
descr_repr(PyDescrObject *descr, char *format)
{
	return PyString_FromFormat(format, descr_name(descr),
				   descr->d_type->tp_name);
}

static PyObject *
method_repr(PyMethodDescrObject *descr)
{
	return descr_repr((PyDescrObject *)descr, 
			  "<method '%s' of '%s' objects>");
}

static PyObject *
member_repr(PyMemberDescrObject *descr)
{
	return descr_repr((PyDescrObject *)descr, 
			  "<member '%s' of '%s' objects>");
}

static PyObject *
getset_repr(PyGetSetDescrObject *descr)
{
	return descr_repr((PyDescrObject *)descr, 
			  "<attribute '%s' of '%s' objects>");
}

static PyObject *
wrapperdescr_repr(PyWrapperDescrObject *descr)
{
	return descr_repr((PyDescrObject *)descr, 
			  "<slot wrapper '%s' of '%s' objects>");
}

static int
descr_check(PyDescrObject *descr, PyObject *obj, PyObject **pres)
{
	if (obj == NULL) {
		Py_INCREF(descr);
		*pres = (PyObject *)descr;
		return 1;
	}
	if (!PyObject_TypeCheck(obj, descr->d_type)) {
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%s' for '%s' objects "
			     "doesn't apply to '%s' object",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name,
			     obj->ob_type->tp_name);
		*pres = NULL;
		return 1;
	}
	return 0;
}

static PyObject *
classmethod_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
{
	/* Ensure a valid type.  Class methods ignore obj. */
	if (type == NULL) {
		if (obj != NULL)
			type = (PyObject *)obj->ob_type;
		else {
			/* Wot - no type?! */
			PyErr_Format(PyExc_TypeError,
				     "descriptor '%s' for type '%s' "
				     "needs either an object or a type",
				     descr_name((PyDescrObject *)descr),
				     descr->d_type->tp_name);
			return NULL;
		}
	}
	if (!PyType_Check(type)) {
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%s' for type '%s' "
			     "needs a type, not a '%s' as arg 2",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name,
			     type->ob_type->tp_name);
		return NULL;
	}
	if (!PyType_IsSubtype((PyTypeObject *)type, descr->d_type)) {
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%s' for type '%s' "
			     "doesn't apply to type '%s'",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name,
			     ((PyTypeObject *)type)->tp_name);
		return NULL;
	}
	return PyCFunction_New(descr->d_method, type);
}

static PyObject *
method_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
{
	PyObject *res;

	if (descr_check((PyDescrObject *)descr, obj, &res))
		return res;
	return PyCFunction_New(descr->d_method, obj);
}

static PyObject *
member_get(PyMemberDescrObject *descr, PyObject *obj, PyObject *type)
{
	PyObject *res;

	if (descr_check((PyDescrObject *)descr, obj, &res))
		return res;
	return PyMember_GetOne((char *)obj, descr->d_member);
}

static PyObject *
getset_get(PyGetSetDescrObject *descr, PyObject *obj, PyObject *type)
{
	PyObject *res;

	if (descr_check((PyDescrObject *)descr, obj, &res))
		return res;
	if (descr->d_getset->get != NULL)
		return descr->d_getset->get(obj, descr->d_getset->closure);
	PyErr_Format(PyExc_AttributeError,
		     "attribute '%.300s' of '%.100s' objects is not readable",
		     descr_name((PyDescrObject *)descr),
		     descr->d_type->tp_name);
	return NULL;
}

static PyObject *
wrapperdescr_get(PyWrapperDescrObject *descr, PyObject *obj, PyObject *type)
{
	PyObject *res;

	if (descr_check((PyDescrObject *)descr, obj, &res))
		return res;
	return PyWrapper_New((PyObject *)descr, obj);
}

static int
descr_setcheck(PyDescrObject *descr, PyObject *obj, PyObject *value,
	       int *pres)
{
	assert(obj != NULL);
	if (!PyObject_IsInstance(obj, (PyObject *)(descr->d_type))) {
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.200s' for '%.100s' objects "
			     "doesn't apply to '%.100s' object",
			     descr_name(descr),
			     descr->d_type->tp_name,
			     obj->ob_type->tp_name);
		*pres = -1;
		return 1;
	}
	return 0;
}

static int
member_set(PyMemberDescrObject *descr, PyObject *obj, PyObject *value)
{
	int res;

	if (descr_setcheck((PyDescrObject *)descr, obj, value, &res))
		return res;
	return PyMember_SetOne((char *)obj, descr->d_member, value);
}

static int
getset_set(PyGetSetDescrObject *descr, PyObject *obj, PyObject *value)
{
	int res;

	if (descr_setcheck((PyDescrObject *)descr, obj, value, &res))
		return res;
	if (descr->d_getset->set != NULL)
		return descr->d_getset->set(obj, value,
					    descr->d_getset->closure);
	PyErr_Format(PyExc_AttributeError,
		     "attribute '%.300s' of '%.100s' objects is not writable",
		     descr_name((PyDescrObject *)descr),
		     descr->d_type->tp_name);
	return -1;
}

static PyObject *
methoddescr_call(PyMethodDescrObject *descr, PyObject *args, PyObject *kwds)
{
	Py_ssize_t argc;
	PyObject *self, *func, *result;

	/* Make sure that the first argument is acceptable as 'self' */
	assert(PyTuple_Check(args));
	argc = PyTuple_GET_SIZE(args);
	if (argc < 1) {
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.300s' of '%.100s' "
			     "object needs an argument",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name);
		return NULL;
	}
	self = PyTuple_GET_ITEM(args, 0);
	if (!PyObject_IsInstance(self, (PyObject *)(descr->d_type))) {
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.200s' "
			     "requires a '%.100s' object "
			     "but received a '%.100s'",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name,
			     self->ob_type->tp_name);
		return NULL;
	}

	func = PyCFunction_New(descr->d_method, self);
	if (func == NULL)
		return NULL;
	args = PyTuple_GetSlice(args, 1, argc);
	if (args == NULL) {
		Py_DECREF(func);
		return NULL;
	}
	result = PyEval_CallObjectWithKeywords(func, args, kwds);
	Py_DECREF(args);
	Py_DECREF(func);
	return result;
}

static PyObject *
classmethoddescr_call(PyMethodDescrObject *descr, PyObject *args,
		      PyObject *kwds)
{
	PyObject *func, *result;

	func = PyCFunction_New(descr->d_method, (PyObject *)descr->d_type);
	if (func == NULL)
		return NULL;

	result = PyEval_CallObjectWithKeywords(func, args, kwds);
	Py_DECREF(func);
	return result;
}

static PyObject *
wrapperdescr_call(PyWrapperDescrObject *descr, PyObject *args, PyObject *kwds)
{
	Py_ssize_t argc;
	PyObject *self, *func, *result;

	/* Make sure that the first argument is acceptable as 'self' */
	assert(PyTuple_Check(args));
	argc = PyTuple_GET_SIZE(args);
	if (argc < 1) {
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.300s' of '%.100s' "
			     "object needs an argument",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name);
		return NULL;
	}
	self = PyTuple_GET_ITEM(args, 0);
	if (!PyObject_IsInstance(self, (PyObject *)(descr->d_type))) {
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.200s' "
			     "requires a '%.100s' object "
			     "but received a '%.100s'",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name,
			     self->ob_type->tp_name);
		return NULL;
	}

	func = PyWrapper_New((PyObject *)descr, self);
	if (func == NULL)
		return NULL;
	args = PyTuple_GetSlice(args, 1, argc);
	if (args == NULL) {
		Py_DECREF(func);
		return NULL;
	}
	result = PyEval_CallObjectWithKeywords(func, args, kwds);
	Py_DECREF(args);
	Py_DECREF(func);
	return result;
}

static PyObject *
method_get_doc(PyMethodDescrObject *descr, void *closure)
{
	if (descr->d_method->ml_doc == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(descr->d_method->ml_doc);
}

static PyMemberDef descr_members[] = {
	{"__objclass__", T_OBJECT, offsetof(PyDescrObject, d_type), READONLY},
	{"__name__", T_OBJECT, offsetof(PyDescrObject, d_name), READONLY},
	{0}
};

static PyGetSetDef method_getset[] = {
	{"__doc__", (getter)method_get_doc},
	{0}
};

static PyObject *
member_get_doc(PyMemberDescrObject *descr, void *closure)
{
	if (descr->d_member->doc == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(descr->d_member->doc);
}

static PyGetSetDef member_getset[] = {
	{"__doc__", (getter)member_get_doc},
	{0}
};

static PyObject *
getset_get_doc(PyGetSetDescrObject *descr, void *closure)
{
	if (descr->d_getset->doc == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(descr->d_getset->doc);
}

static PyGetSetDef getset_getset[] = {
	{"__doc__", (getter)getset_get_doc},
	{0}
};

static PyObject *
wrapperdescr_get_doc(PyWrapperDescrObject *descr, void *closure)
{
	if (descr->d_base->doc == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(descr->d_base->doc);
}

static PyGetSetDef wrapperdescr_getset[] = {
	{"__doc__", (getter)wrapperdescr_get_doc},
	{0}
};

static int
descr_traverse(PyObject *self, visitproc visit, void *arg)
{
	PyDescrObject *descr = (PyDescrObject *)self;
	Py_VISIT(descr->d_type);
	return 0;
}

static PyTypeObject PyMethodDescr_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"method_descriptor",
	sizeof(PyMethodDescrObject),
	0,
	(destructor)descr_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)method_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	(ternaryfunc)methoddescr_call,		/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	0,					/* tp_doc */
	descr_traverse,				/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	descr_members,				/* tp_members */
	method_getset,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	(descrgetfunc)method_get,		/* tp_descr_get */
	0,					/* tp_descr_set */
};

/* This is for METH_CLASS in C, not for "f = classmethod(f)" in Python! */
static PyTypeObject PyClassMethodDescr_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"classmethod_descriptor",
	sizeof(PyMethodDescrObject),
	0,
	(destructor)descr_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)method_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	(ternaryfunc)classmethoddescr_call,	/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	0,					/* tp_doc */
	descr_traverse,				/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	descr_members,				/* tp_members */
	method_getset,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	(descrgetfunc)classmethod_get,		/* tp_descr_get */
	0,					/* tp_descr_set */
};

static PyTypeObject PyMemberDescr_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"member_descriptor",
	sizeof(PyMemberDescrObject),
	0,
	(destructor)descr_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)member_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	0,					/* tp_doc */
	descr_traverse,				/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	descr_members,				/* tp_members */
	member_getset,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	(descrgetfunc)member_get,		/* tp_descr_get */
	(descrsetfunc)member_set,		/* tp_descr_set */
};

static PyTypeObject PyGetSetDescr_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"getset_descriptor",
	sizeof(PyGetSetDescrObject),
	0,
	(destructor)descr_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)getset_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	0,					/* tp_doc */
	descr_traverse,				/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	descr_members,				/* tp_members */
	getset_getset,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	(descrgetfunc)getset_get,		/* tp_descr_get */
	(descrsetfunc)getset_set,		/* tp_descr_set */
};

PyTypeObject PyWrapperDescr_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"wrapper_descriptor",
	sizeof(PyWrapperDescrObject),
	0,
	(destructor)descr_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)wrapperdescr_repr,		/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	(ternaryfunc)wrapperdescr_call,		/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	0,					/* tp_doc */
	descr_traverse,				/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	descr_members,				/* tp_members */
	wrapperdescr_getset,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	(descrgetfunc)wrapperdescr_get,		/* tp_descr_get */
	0,					/* tp_descr_set */
};

static PyDescrObject *
descr_new(PyTypeObject *descrtype, PyTypeObject *type, const char *name)
{
	PyDescrObject *descr;

	descr = (PyDescrObject *)PyType_GenericAlloc(descrtype, 0);
	if (descr != NULL) {
		Py_XINCREF(type);
		descr->d_type = type;
		descr->d_name = PyString_InternFromString(name);
		if (descr->d_name == NULL) {
			Py_DECREF(descr);
			descr = NULL;
		}
	}
	return descr;
}

PyObject *
PyDescr_NewMethod(PyTypeObject *type, PyMethodDef *method)
{
	PyMethodDescrObject *descr;

	descr = (PyMethodDescrObject *)descr_new(&PyMethodDescr_Type,
						 type, method->ml_name);
	if (descr != NULL)
		descr->d_method = method;
	return (PyObject *)descr;
}

PyObject *
PyDescr_NewClassMethod(PyTypeObject *type, PyMethodDef *method)
{
	PyMethodDescrObject *descr;

	descr = (PyMethodDescrObject *)descr_new(&PyClassMethodDescr_Type,
						 type, method->ml_name);
	if (descr != NULL)
		descr->d_method = method;
	return (PyObject *)descr;
}

PyObject *
PyDescr_NewMember(PyTypeObject *type, PyMemberDef *member)
{
	PyMemberDescrObject *descr;

	descr = (PyMemberDescrObject *)descr_new(&PyMemberDescr_Type,
						 type, member->name);
	if (descr != NULL)
		descr->d_member = member;
	return (PyObject *)descr;
}

PyObject *
PyDescr_NewGetSet(PyTypeObject *type, PyGetSetDef *getset)
{
	PyGetSetDescrObject *descr;

	descr = (PyGetSetDescrObject *)descr_new(&PyGetSetDescr_Type,
						 type, getset->name);
	if (descr != NULL)
		descr->d_getset = getset;
	return (PyObject *)descr;
}

PyObject *
PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *base, void *wrapped)
{
	PyWrapperDescrObject *descr;

	descr = (PyWrapperDescrObject *)descr_new(&PyWrapperDescr_Type,
						 type, base->name);
	if (descr != NULL) {
		descr->d_base = base;
		descr->d_wrapped = wrapped;
	}
	return (PyObject *)descr;
}


/* --- Readonly proxy for dictionaries (actually any mapping) --- */

/* This has no reason to be in this file except that adding new files is a
   bit of a pain */

typedef struct {
	PyObject_HEAD
	PyObject *dict;
} proxyobject;

static Py_ssize_t
proxy_len(proxyobject *pp)
{
	return PyObject_Size(pp->dict);
}

static PyObject *
proxy_getitem(proxyobject *pp, PyObject *key)
{
	return PyObject_GetItem(pp->dict, key);
}

static PyMappingMethods proxy_as_mapping = {
	(lenfunc)proxy_len,			/* mp_length */
	(binaryfunc)proxy_getitem,		/* mp_subscript */
	0,					/* mp_ass_subscript */
};

static int
proxy_contains(proxyobject *pp, PyObject *key)
{
	return PyDict_Contains(pp->dict, key);
}

static PySequenceMethods proxy_as_sequence = {
	0,					/* sq_length */
	0,					/* sq_concat */
	0,					/* sq_repeat */
	0,					/* sq_item */
	0,					/* sq_slice */
	0,					/* sq_ass_item */
	0,					/* sq_ass_slice */
	(objobjproc)proxy_contains,		/* sq_contains */
	0,					/* sq_inplace_concat */
	0,					/* sq_inplace_repeat */
};

static PyObject *
proxy_has_key(proxyobject *pp, PyObject *key)
{
	int res = PyDict_Contains(pp->dict, key);
	if (res < 0)
		return NULL;
	return PyBool_FromLong(res);
}

static PyObject *
proxy_get(proxyobject *pp, PyObject *args)
{
	PyObject *key, *def = Py_None;

	if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &def))
		return NULL;
	return PyObject_CallMethod(pp->dict, "get", "(OO)", key, def);
}

static PyObject *
proxy_keys(proxyobject *pp)
{
	return PyMapping_Keys(pp->dict);
}

static PyObject *
proxy_values(proxyobject *pp)
{
	return PyMapping_Values(pp->dict);
}

static PyObject *
proxy_items(proxyobject *pp)
{
	return PyMapping_Items(pp->dict);
}

static PyObject *
proxy_iterkeys(proxyobject *pp)
{
	return PyObject_CallMethod(pp->dict, "iterkeys", NULL);
}

static PyObject *
proxy_itervalues(proxyobject *pp)
{
	return PyObject_CallMethod(pp->dict, "itervalues", NULL);
}

static PyObject *
proxy_iteritems(proxyobject *pp)
{
	return PyObject_CallMethod(pp->dict, "iteritems", NULL);
}
static PyObject *
proxy_copy(proxyobject *pp)
{
	return PyObject_CallMethod(pp->dict, "copy", NULL);
}

static PyMethodDef proxy_methods[] = {
	{"has_key",   (PyCFunction)proxy_has_key,    METH_O,
	 PyDoc_STR("D.has_key(k) -> True if D has a key k, else False")},
	{"get",       (PyCFunction)proxy_get,        METH_VARARGS,
	 PyDoc_STR("D.get(k[,d]) -> D[k] if D.has_key(k), else d."
	 				"  d defaults to None.")},
	{"keys",      (PyCFunction)proxy_keys,       METH_NOARGS,
	 PyDoc_STR("D.keys() -> list of D's keys")},
	{"values",    (PyCFunction)proxy_values,     METH_NOARGS,
	 PyDoc_STR("D.values() -> list of D's values")},
	{"items",     (PyCFunction)proxy_items,      METH_NOARGS,
	 PyDoc_STR("D.items() -> list of D's (key, value) pairs, as 2-tuples")},
	{"iterkeys",  (PyCFunction)proxy_iterkeys,   METH_NOARGS,
	 PyDoc_STR("D.iterkeys() -> an iterator over the keys of D")},
	{"itervalues",(PyCFunction)proxy_itervalues, METH_NOARGS,
	 PyDoc_STR("D.itervalues() -> an iterator over the values of D")},
	{"iteritems", (PyCFunction)proxy_iteritems,  METH_NOARGS,
	 PyDoc_STR("D.iteritems() ->"
	 	   " an iterator over the (key, value) items of D")},
	{"copy",      (PyCFunction)proxy_copy,       METH_NOARGS,
	 PyDoc_STR("D.copy() -> a shallow copy of D")},
	{0}
};

static void
proxy_dealloc(proxyobject *pp)
{
	_PyObject_GC_UNTRACK(pp);
	Py_DECREF(pp->dict);
	PyObject_GC_Del(pp);
}

static PyObject *
proxy_getiter(proxyobject *pp)
{
	return PyObject_GetIter(pp->dict);
}

static PyObject *
proxy_str(proxyobject *pp)
{
	return PyObject_Str(pp->dict);
}

static int
proxy_traverse(PyObject *self, visitproc visit, void *arg)
{
	proxyobject *pp = (proxyobject *)self;
	Py_VISIT(pp->dict);
	return 0;
}

static int
proxy_compare(proxyobject *v, PyObject *w)
{
	return PyObject_Compare(v->dict, w);
}

static PyObject *
proxy_richcompare(proxyobject *v, PyObject *w, int op)
{
	return PyObject_RichCompare(v->dict, w, op);
}

static PyTypeObject proxytype = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"dictproxy",				/* tp_name */
	sizeof(proxyobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)proxy_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	(cmpfunc)proxy_compare,			/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	&proxy_as_sequence,			/* tp_as_sequence */
	&proxy_as_mapping,			/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	(reprfunc)proxy_str,			/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
 	0,					/* tp_doc */
	proxy_traverse,				/* tp_traverse */
 	0,					/* tp_clear */
	(richcmpfunc)proxy_richcompare,		/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	(getiterfunc)proxy_getiter,		/* tp_iter */
	0,					/* tp_iternext */
	proxy_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
};

PyObject *
PyDictProxy_New(PyObject *dict)
{
	proxyobject *pp;

	pp = PyObject_GC_New(proxyobject, &proxytype);
	if (pp != NULL) {
		Py_INCREF(dict);
		pp->dict = dict;
		_PyObject_GC_TRACK(pp);
	}
	return (PyObject *)pp;
}


/* --- Wrapper object for "slot" methods --- */

/* This has no reason to be in this file except that adding new files is a
   bit of a pain */

typedef struct {
	PyObject_HEAD
	PyWrapperDescrObject *descr;
	PyObject *self;
} wrapperobject;

static void
wrapper_dealloc(wrapperobject *wp)
{
	PyObject_GC_UnTrack(wp);
	Py_TRASHCAN_SAFE_BEGIN(wp)
	Py_XDECREF(wp->descr);
	Py_XDECREF(wp->self);
	PyObject_GC_Del(wp);
	Py_TRASHCAN_SAFE_END(wp)
}

static int
wrapper_compare(wrapperobject *a, wrapperobject *b)
{
	if (a->descr == b->descr)
		return PyObject_Compare(a->self, b->self);
	else
		return (a->descr < b->descr) ? -1 : 1;
}

static long
wrapper_hash(wrapperobject *wp)
{
	int x, y;
	x = _Py_HashPointer(wp->descr);
	if (x == -1)
		return -1;
	y = PyObject_Hash(wp->self);
	if (y == -1)
		return -1;
	x = x ^ y;
	if (x == -1)
		x = -2;
	return x;
}

static PyObject *
wrapper_repr(wrapperobject *wp)
{
	return PyString_FromFormat("<method-wrapper '%s' of %s object at %p>",
				   wp->descr->d_base->name,
				   wp->self->ob_type->tp_name,
				   wp->self);
}

static PyMemberDef wrapper_members[] = {
	{"__self__", T_OBJECT, offsetof(wrapperobject, self), READONLY},
	{0}
};

static PyObject *
wrapper_objclass(wrapperobject *wp)
{
	PyObject *c = (PyObject *)wp->descr->d_type;

	Py_INCREF(c);
	return c;
}

static PyObject *
wrapper_name(wrapperobject *wp)
{
	char *s = wp->descr->d_base->name;

	return PyString_FromString(s);
}

static PyObject *
wrapper_doc(wrapperobject *wp)
{
	char *s = wp->descr->d_base->doc;

	if (s == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	else {
		return PyString_FromString(s);
	}
}

static PyGetSetDef wrapper_getsets[] = {
	{"__objclass__", (getter)wrapper_objclass},
	{"__name__", (getter)wrapper_name},
	{"__doc__", (getter)wrapper_doc},
	{0}
};

static PyObject *
wrapper_call(wrapperobject *wp, PyObject *args, PyObject *kwds)
{
	wrapperfunc wrapper = wp->descr->d_base->wrapper;
	PyObject *self = wp->self;

	if (wp->descr->d_base->flags & PyWrapperFlag_KEYWORDS) {
		wrapperfunc_kwds wk = (wrapperfunc_kwds)wrapper;
		return (*wk)(self, args, wp->descr->d_wrapped, kwds);
	}

	if (kwds != NULL && (!PyDict_Check(kwds) || PyDict_Size(kwds) != 0)) {
		PyErr_Format(PyExc_TypeError,
			     "wrapper %s doesn't take keyword arguments",
			     wp->descr->d_base->name);
		return NULL;
	}
	return (*wrapper)(self, args, wp->descr->d_wrapped);
}

static int
wrapper_traverse(PyObject *self, visitproc visit, void *arg)
{
	wrapperobject *wp = (wrapperobject *)self;
	Py_VISIT(wp->descr);
	Py_VISIT(wp->self);
	return 0;
}

static PyTypeObject wrappertype = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"method-wrapper",			/* tp_name */
	sizeof(wrapperobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)wrapper_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	(cmpfunc)wrapper_compare,		/* tp_compare */
	(reprfunc)wrapper_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,		       			/* tp_as_mapping */
	(hashfunc)wrapper_hash,			/* tp_hash */
	(ternaryfunc)wrapper_call,		/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
 	0,					/* tp_doc */
	wrapper_traverse,			/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	wrapper_members,			/* tp_members */
	wrapper_getsets,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
};

PyObject *
PyWrapper_New(PyObject *d, PyObject *self)
{
	wrapperobject *wp;
	PyWrapperDescrObject *descr;

	assert(PyObject_TypeCheck(d, &PyWrapperDescr_Type));
	descr = (PyWrapperDescrObject *)d;
	assert(PyObject_IsInstance(self, (PyObject *)(descr->d_type)));

	wp = PyObject_GC_New(wrapperobject, &wrappertype);
	if (wp != NULL) {
		Py_INCREF(descr);
		wp->descr = descr;
		Py_INCREF(self);
		wp->self = self;
		_PyObject_GC_TRACK(wp);
	}
	return (PyObject *)wp;
}


/* A built-in 'property' type */

/*
    class property(object):

        def __init__(self, fget=None, fset=None, fdel=None, doc=None):
            if doc is None and fget is not None and hasattr(fget, "__doc__"):
                doc = fget.__doc__
            self.__get = fget
            self.__set = fset
            self.__del = fdel
            self.__doc__ = doc

        def __get__(self, inst, type=None):
            if inst is None:
                return self
            if self.__get is None:
                raise AttributeError, "unreadable attribute"
            return self.__get(inst)

        def __set__(self, inst, value):
            if self.__set is None:
                raise AttributeError, "can't set attribute"
            return self.__set(inst, value)

        def __delete__(self, inst):
            if self.__del is None:
                raise AttributeError, "can't delete attribute"
            return self.__del(inst)

*/

typedef struct {
	PyObject_HEAD
	PyObject *prop_get;
	PyObject *prop_set;
	PyObject *prop_del;
	PyObject *prop_doc;
	int getter_doc;
} propertyobject;

static PyObject * property_copy(PyObject *, PyObject *, PyObject *,
				  PyObject *, PyObject *);

static PyMemberDef property_members[] = {
	{"fget", T_OBJECT, offsetof(propertyobject, prop_get), READONLY},
	{"fset", T_OBJECT, offsetof(propertyobject, prop_set), READONLY},
	{"fdel", T_OBJECT, offsetof(propertyobject, prop_del), READONLY},
	{"__doc__",  T_OBJECT, offsetof(propertyobject, prop_doc), READONLY},
	{0}
};


PyDoc_STRVAR(getter_doc,
	     "Descriptor to change the getter on a property.");

PyObject *
property_getter(PyObject *self, PyObject *getter)
{
	return property_copy(self, getter, NULL, NULL, NULL);
}


PyDoc_STRVAR(setter_doc,
	     "Descriptor to change the setter on a property.");

PyObject *
property_setter(PyObject *self, PyObject *setter)
{
	return property_copy(self, NULL, setter, NULL, NULL);
}


PyDoc_STRVAR(deleter_doc,
	     "Descriptor to change the deleter on a property.");

PyObject *
property_deleter(PyObject *self, PyObject *deleter)
{
	return property_copy(self, NULL, NULL, deleter, NULL);
}


static PyMethodDef property_methods[] = {
	{"getter", property_getter, METH_O, getter_doc},
	{"setter", property_setter, METH_O, setter_doc},
	{"deleter", property_deleter, METH_O, deleter_doc},
	{0}
};


static void
property_dealloc(PyObject *self)
{
	propertyobject *gs = (propertyobject *)self;

	_PyObject_GC_UNTRACK(self);
	Py_XDECREF(gs->prop_get);
	Py_XDECREF(gs->prop_set);
	Py_XDECREF(gs->prop_del);
	Py_XDECREF(gs->prop_doc);
	self->ob_type->tp_free(self);
}

static PyObject *
property_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
	propertyobject *gs = (propertyobject *)self;

	if (obj == NULL || obj == Py_None) {
		Py_INCREF(self);
		return self;
	}
	if (gs->prop_get == NULL) {
		PyErr_SetString(PyExc_AttributeError, "unreadable attribute");
		return NULL;
	}
	return PyObject_CallFunction(gs->prop_get, "(O)", obj);
}

static int
property_descr_set(PyObject *self, PyObject *obj, PyObject *value)
{
	propertyobject *gs = (propertyobject *)self;
	PyObject *func, *res;

	if (value == NULL)
		func = gs->prop_del;
	else
		func = gs->prop_set;
	if (func == NULL) {
		PyErr_SetString(PyExc_AttributeError,
				value == NULL ?
				"can't delete attribute" :
				"can't set attribute");
		return -1;
	}
	if (value == NULL)
		res = PyObject_CallFunction(func, "(O)", obj);
	else
		res = PyObject_CallFunction(func, "(OO)", obj, value);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static PyObject *
property_copy(PyObject *old, PyObject *get, PyObject *set, PyObject *del,
		PyObject *doc)
{
	propertyobject *pold = (propertyobject *)old;
	propertyobject *pnew = NULL;
	PyObject *new, *type;

	type = PyObject_Type(old);
	if (type == NULL)
		return NULL;

	if (get == NULL || get == Py_None) {
		Py_XDECREF(get);
		get = pold->prop_get ? pold->prop_get : Py_None;
	}
	if (set == NULL || set == Py_None) {
		Py_XDECREF(set);
		set = pold->prop_set ? pold->prop_set : Py_None;
	}
	if (del == NULL || del == Py_None) {
		Py_XDECREF(del);
		del = pold->prop_del ? pold->prop_del : Py_None;
	}
	if (doc == NULL || doc == Py_None) {
		Py_XDECREF(doc);
		doc = pold->prop_doc ? pold->prop_doc : Py_None;
	}
	
	new =  PyObject_CallFunction(type, "OOOO", get, set, del, doc);
	if (new == NULL)
		return NULL;
	pnew = (propertyobject *)new;
	
	if (pold->getter_doc && get != Py_None) {
		PyObject *get_doc = PyObject_GetAttrString(get, "__doc__");
		if (get_doc != NULL) {
			Py_XDECREF(pnew->prop_doc);
			pnew->prop_doc = get_doc;  /* get_doc already INCREF'd by GetAttr */
			pnew->getter_doc = 1;
		} else {
			PyErr_Clear();
		}
	}
	return new;
}

static int
property_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *get = NULL, *set = NULL, *del = NULL, *doc = NULL;
	static char *kwlist[] = {"fget", "fset", "fdel", "doc", 0};
	propertyobject *prop = (propertyobject *)self;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO:property",
					 kwlist, &get, &set, &del, &doc))
		return -1;

	if (get == Py_None)
		get = NULL;
	if (set == Py_None)
		set = NULL;
	if (del == Py_None)
		del = NULL;

	Py_XINCREF(get);
	Py_XINCREF(set);
	Py_XINCREF(del);
	Py_XINCREF(doc);

	prop->prop_get = get;
	prop->prop_set = set;
	prop->prop_del = del;
	prop->prop_doc = doc;
	prop->getter_doc = 0;

	/* if no docstring given and the getter has one, use that one */
	if ((doc == NULL || doc == Py_None) && get != NULL) {
		PyObject *get_doc = PyObject_GetAttrString(get, "__doc__");
		if (get_doc != NULL) {
			Py_XDECREF(prop->prop_doc);
			prop->prop_doc = get_doc;  /* get_doc already INCREF'd by GetAttr */
			prop->getter_doc = 1;
		} else {
			PyErr_Clear();
		}
	}

	return 0;
}

PyDoc_STRVAR(property_doc,
"property(fget=None, fset=None, fdel=None, doc=None) -> property attribute\n"
"\n"
"fget is a function to be used for getting an attribute value, and likewise\n"
"fset is a function for setting, and fdel a function for del'ing, an\n"
"attribute.  Typical use is to define a managed attribute x:\n"
"class C(object):\n"
"    def getx(self): return self._x\n"
"    def setx(self, value): self._x = value\n"
"    def delx(self): del self._x\n"
"    x = property(getx, setx, delx, \"I'm the 'x' property.\")\n"
"\n"
"Decorators make defining new properties or modifying existing ones easy:\n"
"class C(object):\n"
"    @property\n"
"    def x(self): return self._x\n"
"    @x.setter\n"
"    def x(self, value): self._x = value\n"
"    @x.deleter\n"
"    def x(self): del self._x\n"
);

static int
property_traverse(PyObject *self, visitproc visit, void *arg)
{
	propertyobject *pp = (propertyobject *)self;
	Py_VISIT(pp->prop_get);
	Py_VISIT(pp->prop_set);
	Py_VISIT(pp->prop_del);
	Py_VISIT(pp->prop_doc);
	return 0;
}

PyTypeObject PyProperty_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"property",				/* tp_name */
	sizeof(propertyobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	property_dealloc,	 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,		       			/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,		/* tp_flags */
 	property_doc,				/* tp_doc */
	property_traverse,			/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	property_methods,			/* tp_methods */
	property_members,			/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	property_descr_get,			/* tp_descr_get */
	property_descr_set,			/* tp_descr_set */
	0,					/* tp_dictoffset */
	property_init,				/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	PyObject_GC_Del,               		/* tp_free */
};
