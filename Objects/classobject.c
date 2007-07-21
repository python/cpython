/* Class object implementation (dead now except for methods) */

#include "Python.h"
#include "structmember.h"

#define TP_DESCR_GET(t) ((t)->tp_descr_get)


PyObject *
PyMethod_Function(PyObject *im)
{
	if (!PyMethod_Check(im)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyMethodObject *)im)->im_func;
}

PyObject *
PyMethod_Self(PyObject *im)
{
	if (!PyMethod_Check(im)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyMethodObject *)im)->im_self;
}

PyObject *
PyMethod_Class(PyObject *im)
{
	if (!PyMethod_Check(im)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyMethodObject *)im)->im_class;
}


/* Method objects are used for two purposes:
   (a) as bound instance methods (returned by instancename.methodname)
   (b) as unbound methods (returned by ClassName.methodname)
   In case (b), im_self is NULL
*/

static PyMethodObject *free_list;

PyObject *
PyMethod_New(PyObject *func, PyObject *self, PyObject *klass)
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
		im = PyObject_GC_New(PyMethodObject, &PyMethod_Type);
		if (im == NULL)
			return NULL;
	}
	im->im_weakreflist = NULL;
	Py_INCREF(func);
	im->im_func = func;
	Py_XINCREF(self);
	im->im_self = self;
	Py_XINCREF(klass);
	im->im_class = klass;
	_PyObject_GC_TRACK(im);
	return (PyObject *)im;
}

/* Descriptors for PyMethod attributes */

/* im_class, im_func and im_self are stored in the PyMethod object */

#define OFF(x) offsetof(PyMethodObject, x)

static PyMemberDef method_memberlist[] = {
	{"im_class",	T_OBJECT,	OFF(im_class),	READONLY|RESTRICTED,
	 "the class associated with a method"},
	{"im_func",	T_OBJECT,	OFF(im_func),	READONLY|RESTRICTED,
	 "the function (or other callable) implementing a method"},
	{"im_self",	T_OBJECT,	OFF(im_self),	READONLY|RESTRICTED,
	 "the instance to which a method is bound; None for unbound methods"},
	{NULL}	/* Sentinel */
};

/* Christian Tismer argued convincingly that method attributes should
   (nearly) always override function attributes.
   The one exception is __doc__; there's a default __doc__ which
   should only be used for the class, not for instances */

static PyObject *
method_get_doc(PyMethodObject *im, void *context)
{
	static PyObject *docstr;
	if (docstr == NULL) {
		docstr= PyUnicode_InternFromString("__doc__");
		if (docstr == NULL)
			return NULL;
	}
	return PyObject_GetAttr(im->im_func, docstr);
}

static PyGetSetDef method_getset[] = {
	{"__doc__", (getter)method_get_doc, NULL, NULL},
	{0}
};

static PyObject *
method_getattro(PyObject *obj, PyObject *name)
{
	PyMethodObject *im = (PyMethodObject *)obj;
	PyTypeObject *tp = obj->ob_type;
	PyObject *descr = NULL;

	{
		if (tp->tp_dict == NULL) {
			if (PyType_Ready(tp) < 0)
				return NULL;
		}
		descr = _PyType_Lookup(tp, name);
	}

	if (descr != NULL) {
		descrgetfunc f = TP_DESCR_GET(descr->ob_type);
		if (f != NULL)
			return f(descr, obj, (PyObject *)obj->ob_type);
		else {
			Py_INCREF(descr);
			return descr;
		}
	}

	return PyObject_GetAttr(im->im_func, name);
}

PyDoc_STRVAR(method_doc,
"method(function, instance, class)\n\
\n\
Create an instance method object.");

static PyObject *
method_new(PyTypeObject* type, PyObject* args, PyObject *kw)
{
	PyObject *func;
	PyObject *self;
	PyObject *classObj = NULL;

	if (!_PyArg_NoKeywords("instancemethod", kw))
		return NULL;
	if (!PyArg_UnpackTuple(args, "method", 2, 3,
			      &func, &self, &classObj))
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError,
				"first argument must be callable");
		return NULL;
	}
	if (self == Py_None)
		self = NULL;
	if (self == NULL && classObj == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"unbound methods must have non-NULL im_class");
		return NULL;
	}

	return PyMethod_New(func, self, classObj);
}

static void
method_dealloc(register PyMethodObject *im)
{
	_PyObject_GC_UNTRACK(im);
	if (im->im_weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *)im);
	Py_DECREF(im->im_func);
	Py_XDECREF(im->im_self);
	Py_XDECREF(im->im_class);
	im->im_self = (PyObject *)free_list;
	free_list = im;
}

static PyObject *
method_richcompare(PyObject *self, PyObject *other, int op)
{
	PyMethodObject *a, *b;
	PyObject *res;
	int eq;

	if ((op != Py_EQ && op != Py_NE) ||
	    !PyMethod_Check(self) ||
	    !PyMethod_Check(other))
	{
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	a = (PyMethodObject *)self;
	b = (PyMethodObject *)other;
	eq = PyObject_RichCompareBool(a->im_func, b->im_func, Py_EQ);
	if (eq == 1) {
		if (a->im_self == NULL || b->im_self == NULL)
			eq = a->im_self == b->im_self;
		else
			eq = PyObject_RichCompareBool(a->im_self, b->im_self,
						      Py_EQ);
	}
	if (eq < 0)
		return NULL;
	if (op == Py_EQ)
		res = eq ? Py_True : Py_False;
	else
		res = eq ? Py_False : Py_True;
	Py_INCREF(res);
	return res;
}

static PyObject *
method_repr(PyMethodObject *a)
{
	PyObject *self = a->im_self;
	PyObject *func = a->im_func;
	PyObject *klass = a->im_class;
	PyObject *funcname = NULL, *klassname = NULL, *result = NULL;
	char *defname = "?";

	funcname = PyObject_GetAttrString(func, "__name__");
	if (funcname == NULL) {
		if (!PyErr_ExceptionMatches(PyExc_AttributeError))
			return NULL;
		PyErr_Clear();
	}
	else if (!PyUnicode_Check(funcname)) {
		Py_DECREF(funcname);
		funcname = NULL;
	}
	if (klass == NULL)
		klassname = NULL;
	else {
		klassname = PyObject_GetAttrString(klass, "__name__");
		if (klassname == NULL) {
			if (!PyErr_ExceptionMatches(PyExc_AttributeError))
				return NULL;
			PyErr_Clear();
		}
		else if (!PyUnicode_Check(klassname)) {
			Py_DECREF(klassname);
			klassname = NULL;
		}
	}
	if (self == NULL)
		result = PyUnicode_FromFormat("<unbound method %V.%V>",
		                              klassname, defname,
		                              funcname, defname);
	else {
		/* XXX Shouldn't use repr()/%R here! */
		result = PyUnicode_FromFormat("<bound method %V.%V of %R>",
		                              klassname, defname,
		                              funcname, defname, self);
	}
	Py_XDECREF(funcname);
	Py_XDECREF(klassname);
	return result;
}

static long
method_hash(PyMethodObject *a)
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
	x = x ^ y;
	if (x == -1)
		x = -2;
	return x;
}

static int
method_traverse(PyMethodObject *im, visitproc visit, void *arg)
{
	Py_VISIT(im->im_func);
	Py_VISIT(im->im_self);
	Py_VISIT(im->im_class);
	return 0;
}

static void
getclassname(PyObject *klass, char *buf, int bufsize)
{
	PyObject *name;

	assert(bufsize > 1);
	strcpy(buf, "?"); /* Default outcome */
	if (klass == NULL)
		return;
	name = PyObject_GetAttrString(klass, "__name__");
	if (name == NULL) {
		/* This function cannot return an exception */
		PyErr_Clear();
		return;
	}
	if (PyUnicode_Check(name)) {
		strncpy(buf, PyUnicode_AsString(name), bufsize);
		buf[bufsize-1] = '\0';
	}
	Py_DECREF(name);
}

static void
getinstclassname(PyObject *inst, char *buf, int bufsize)
{
	PyObject *klass;

	if (inst == NULL) {
		assert(bufsize > 0 && (size_t)bufsize > strlen("nothing"));
		strcpy(buf, "nothing");
		return;
	}

	klass = PyObject_GetAttrString(inst, "__class__");
	if (klass == NULL) {
		/* This function cannot return an exception */
		PyErr_Clear();
		klass = (PyObject *)(inst->ob_type);
		Py_INCREF(klass);
	}
	getclassname(klass, buf, bufsize);
	Py_XDECREF(klass);
}

static PyObject *
method_call(PyObject *func, PyObject *arg, PyObject *kw)
{
	PyObject *self = PyMethod_GET_SELF(func);
	PyObject *klass = PyMethod_GET_CLASS(func);
	PyObject *result;

	func = PyMethod_GET_FUNCTION(func);
	if (self == NULL) {
		/* Unbound methods must be called with an instance of
		   the class (or a derived class) as first argument */
		int ok;
		if (PyTuple_Size(arg) >= 1)
			self = PyTuple_GET_ITEM(arg, 0);
		if (self == NULL)
			ok = 0;
		else {
			ok = PyObject_IsInstance(self, klass);
			if (ok < 0)
				return NULL;
		}
		if (!ok) {
			char clsbuf[256];
			char instbuf[256];
			getclassname(klass, clsbuf, sizeof(clsbuf));
			getinstclassname(self, instbuf, sizeof(instbuf));
			PyErr_Format(PyExc_TypeError,
				     "unbound method %s%s must be called with "
				     "%s instance as first argument "
				     "(got %s%s instead)",
				     PyEval_GetFuncName(func),
				     PyEval_GetFuncDesc(func),
				     clsbuf,
				     instbuf,
				     self == NULL ? "" : " instance");
			return NULL;
		}
		Py_INCREF(arg);
	}
	else {
		Py_ssize_t argcount = PyTuple_Size(arg);
		PyObject *newarg = PyTuple_New(argcount + 1);
		int i;
		if (newarg == NULL)
			return NULL;
		Py_INCREF(self);
		PyTuple_SET_ITEM(newarg, 0, self);
		for (i = 0; i < argcount; i++) {
			PyObject *v = PyTuple_GET_ITEM(arg, i);
			Py_XINCREF(v);
			PyTuple_SET_ITEM(newarg, i+1, v);
		}
		arg = newarg;
	}
	result = PyObject_Call((PyObject *)func, arg, kw);
	Py_DECREF(arg);
	return result;
}

static PyObject *
method_descr_get(PyObject *meth, PyObject *obj, PyObject *cls)
{
	/* Don't rebind an already bound method, or an unbound method
	   of a class that's not a base class of cls. */

	if (PyMethod_GET_SELF(meth) != NULL) {
		/* Already bound */
		Py_INCREF(meth);
		return meth;
	}
	/* No, it is an unbound method */
	if (PyMethod_GET_CLASS(meth) != NULL && cls != NULL) {
		/* Do subclass test.  If it fails, return meth unchanged. */
		int ok = PyObject_IsSubclass(cls, PyMethod_GET_CLASS(meth));
		if (ok < 0)
			return NULL;
		if (!ok) {
			Py_INCREF(meth);
			return meth;
		}
	}
	/* Bind it to obj */
	return PyMethod_New(PyMethod_GET_FUNCTION(meth), obj, cls);
}

PyTypeObject PyMethod_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"method",
	sizeof(PyMethodObject),
	0,
	(destructor)method_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)method_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)method_hash,			/* tp_hash */
	method_call,				/* tp_call */
	0,					/* tp_str */
	method_getattro,			/* tp_getattro */
	PyObject_GenericSetAttr,		/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	method_doc,				/* tp_doc */
	(traverseproc)method_traverse,		/* tp_traverse */
	0,					/* tp_clear */
	method_richcompare,			/* tp_richcompare */
 	offsetof(PyMethodObject, im_weakreflist), /* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	method_memberlist,			/* tp_members */
	method_getset,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	method_descr_get,			/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	method_new,				/* tp_new */
};

/* Clear out the free list */

void
PyMethod_Fini(void)
{
	while (free_list) {
		PyMethodObject *im = free_list;
		free_list = (PyMethodObject *)(im->im_self);
		PyObject_GC_Del(im);
	}
}
