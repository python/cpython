
/* Type object implementation */

#include "Python.h"
#include "structmember.h"

static PyMemberDef type_members[] = {
	{"__basicsize__", T_INT, offsetof(PyTypeObject,tp_basicsize),READONLY},
	{"__itemsize__", T_INT, offsetof(PyTypeObject, tp_itemsize), READONLY},
	{"__flags__", T_LONG, offsetof(PyTypeObject, tp_flags), READONLY},
	{"__doc__", T_STRING, offsetof(PyTypeObject, tp_doc), READONLY},
	{"__weakrefoffset__", T_LONG,
	 offsetof(PyTypeObject, tp_weaklistoffset), READONLY},
	{"__base__", T_OBJECT, offsetof(PyTypeObject, tp_base), READONLY},
	{"__dictoffset__", T_LONG,
	 offsetof(PyTypeObject, tp_dictoffset), READONLY},
	{"__bases__", T_OBJECT, offsetof(PyTypeObject, tp_bases), READONLY},
	{"__mro__", T_OBJECT, offsetof(PyTypeObject, tp_mro), READONLY},
	{0}
};

static PyObject *
type_name(PyTypeObject *type, void *context)
{
	char *s;

	s = strrchr(type->tp_name, '.');
	if (s == NULL)
		s = type->tp_name;
	else
		s++;
	return PyString_FromString(s);
}

static PyObject *
type_module(PyTypeObject *type, void *context)
{
	PyObject *mod;
	char *s;

	s = strrchr(type->tp_name, '.');
	if (s != NULL)
		return PyString_FromStringAndSize(type->tp_name,
						  (int)(s - type->tp_name));
	if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE))
		return PyString_FromString("__builtin__");
	mod = PyDict_GetItemString(type->tp_defined, "__module__");
	if (mod != NULL && PyString_Check(mod)) {
		Py_INCREF(mod);
		return mod;
	}
	PyErr_SetString(PyExc_AttributeError, "__module__");
	return NULL;
}

static int
type_set_module(PyTypeObject *type, PyObject *value, void *context)
{
	if (!(type->tp_flags & Py_TPFLAGS_DYNAMICTYPE) ||
	    strrchr(type->tp_name, '.')) {
		PyErr_Format(PyExc_TypeError,
			     "can't set %s.__module__", type->tp_name);
		return -1;
	}
	if (!value) {
		PyErr_Format(PyExc_TypeError,
			     "can't delete %s.__module__", type->tp_name);
		return -1;
	}
	return PyDict_SetItemString(type->tp_dict, "__module__", value);
}

static PyObject *
type_dict(PyTypeObject *type, void *context)
{
	if (type->tp_dict == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
 	if (type->tp_flags & Py_TPFLAGS_DYNAMICTYPE) {
		Py_INCREF(type->tp_dict);
		return type->tp_dict;
	}
	return PyDictProxy_New(type->tp_dict);
}

static PyObject *
type_defined(PyTypeObject *type, void *context)
{
	if (type->tp_defined == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (type->tp_flags & Py_TPFLAGS_DYNAMICTYPE) {
		Py_INCREF(type->tp_defined);
		return type->tp_defined;
	}
	return PyDictProxy_New(type->tp_defined);
}

static PyObject *
type_dynamic(PyTypeObject *type, void *context)
{
	PyObject *res;

	res = (type->tp_flags & Py_TPFLAGS_DYNAMICTYPE) ? Py_True : Py_False;
	Py_INCREF(res);
	return res;
}

PyGetSetDef type_getsets[] = {
	{"__name__", (getter)type_name, NULL, NULL},
	{"__module__", (getter)type_module, (setter)type_set_module, NULL},
	{"__dict__",  (getter)type_dict,  NULL, NULL},
	{"__defined__",  (getter)type_defined,  NULL, NULL},
	{"__dynamic__", (getter)type_dynamic, NULL, NULL},
	{0}
};

static int
type_compare(PyObject *v, PyObject *w)
{
	/* This is called with type objects only. So we
	   can just compare the addresses. */
	Py_uintptr_t vv = (Py_uintptr_t)v;
	Py_uintptr_t ww = (Py_uintptr_t)w;
	return (vv < ww) ? -1 : (vv > ww) ? 1 : 0;
}

static PyObject *
type_repr(PyTypeObject *type)
{
	PyObject *mod, *name, *rtn;
	char *kind;

	mod = type_module(type, NULL);
	if (mod == NULL)
		PyErr_Clear();
	else if (!PyString_Check(mod)) {
		Py_DECREF(mod);
		mod = NULL;
	}
	name = type_name(type, NULL);
	if (name == NULL)
		return NULL;

	if (type->tp_flags & Py_TPFLAGS_HEAPTYPE)
		kind = "class";
	else
		kind = "type";

	if (mod != NULL && strcmp(PyString_AS_STRING(mod), "__builtin__")) {
		rtn = PyString_FromFormat("<%s '%s.%s'>",
					  kind,
					  PyString_AS_STRING(mod),
					  PyString_AS_STRING(name));
	}
	else
		rtn = PyString_FromFormat("<%s '%s'>", kind, type->tp_name);

	Py_XDECREF(mod);
	Py_DECREF(name);
	return rtn;
}

static PyObject *
type_call(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *obj;

	if (type->tp_new == NULL) {
		PyErr_Format(PyExc_TypeError,
			     "cannot create '%.100s' instances",
			     type->tp_name);
		return NULL;
	}

	obj = type->tp_new(type, args, kwds);
	if (obj != NULL) {
		type = obj->ob_type;
		if (type->tp_init != NULL &&
		    type->tp_init(obj, args, kwds) < 0) {
			Py_DECREF(obj);
			obj = NULL;
		}
	}
	return obj;
}

PyObject *
PyType_GenericAlloc(PyTypeObject *type, int nitems)
{
#define PTRSIZE (sizeof(PyObject *))

	int size;
	PyObject *obj;

	/* Inline PyObject_New() so we can zero the memory */
	size = _PyObject_VAR_SIZE(type, nitems);
	/* Round up size, if necessary, so we fully zero out __dict__ */
	if (type->tp_itemsize % PTRSIZE != 0) {
		size += PTRSIZE - 1;
		size /= PTRSIZE;
		size *= PTRSIZE;
	}
	if (PyType_IS_GC(type)) {
		obj = _PyObject_GC_Malloc(type, nitems);
	}
	else {
		obj = PyObject_MALLOC(size);
	}
	if (obj == NULL)
		return PyErr_NoMemory();
	memset(obj, '\0', size);
	if (type->tp_flags & Py_TPFLAGS_HEAPTYPE)
		Py_INCREF(type);
	if (type->tp_itemsize == 0)
		PyObject_INIT(obj, type);
	else
		(void) PyObject_INIT_VAR((PyVarObject *)obj, type, nitems);
	if (PyType_IS_GC(type))
		_PyObject_GC_TRACK(obj);
	return obj;
}

PyObject *
PyType_GenericNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	return type->tp_alloc(type, 0);
}

/* Helper for subtyping */

static void
subtype_dealloc(PyObject *self)
{
	PyTypeObject *type, *base;
	destructor f;

	/* This exists so we can DECREF self->ob_type */

	/* Find the nearest base with a different tp_dealloc */
	type = self->ob_type;
	base = type->tp_base;
	while ((f = base->tp_dealloc) == subtype_dealloc) {
		base = base->tp_base;
		assert(base);
	}

	/* If we added a dict, DECREF it */
	if (type->tp_dictoffset && !base->tp_dictoffset) {
		PyObject **dictptr = _PyObject_GetDictPtr(self);
		if (dictptr != NULL) {
			PyObject *dict = *dictptr;
			if (dict != NULL) {
				Py_DECREF(dict);
				*dictptr = NULL;
			}
		}
	}

	/* If we added weaklist, we clear it */
	if (type->tp_weaklistoffset && !base->tp_weaklistoffset)
		PyObject_ClearWeakRefs(self);

	/* Finalize GC if the base doesn't do GC and we do */
	if (PyType_IS_GC(type) && !PyType_IS_GC(base))
		PyObject_GC_Fini(self);

	/* Call the base tp_dealloc() */
	assert(f);
	f(self);

	/* Can't reference self beyond this point */
	if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
		Py_DECREF(type);
	}
}

staticforward void override_slots(PyTypeObject *type, PyObject *dict);
staticforward PyTypeObject *solid_base(PyTypeObject *type);

typedef struct {
	PyTypeObject type;
	PyNumberMethods as_number;
	PySequenceMethods as_sequence;
	PyMappingMethods as_mapping;
	PyBufferProcs as_buffer;
	PyObject *name, *slots;
	PyMemberDef members[1];
} etype;

/* type test with subclassing support */

int
PyType_IsSubtype(PyTypeObject *a, PyTypeObject *b)
{
	PyObject *mro;

	if (!(a->tp_flags & Py_TPFLAGS_HAVE_CLASS))
		return b == a || b == &PyBaseObject_Type;

	mro = a->tp_mro;
	if (mro != NULL) {
		/* Deal with multiple inheritance without recursion
		   by walking the MRO tuple */
		int i, n;
		assert(PyTuple_Check(mro));
		n = PyTuple_GET_SIZE(mro);
		for (i = 0; i < n; i++) {
			if (PyTuple_GET_ITEM(mro, i) == (PyObject *)b)
				return 1;
		}
		return 0;
	}
	else {
		/* a is not completely initilized yet; follow tp_base */
		do {
			if (a == b)
				return 1;
			a = a->tp_base;
		} while (a != NULL);
		return b == &PyBaseObject_Type;
	}
}

/* Internal routines to do a method lookup in the type
   without looking in the instance dictionary
   (so we can't use PyObject_GetAttr) but still binding
   it to the instance.  The arguments are the object,
   the method name as a C string, and the address of a
   static variable used to cache the interned Python string.

   Two variants:

   - lookup_maybe() returns NULL without raising an exception
     when the _PyType_Lookup() call fails;

   - lookup_method() always raises an exception upon errors.
*/

static PyObject *
lookup_maybe(PyObject *self, char *attrstr, PyObject **attrobj)
{
	PyObject *res;

	if (*attrobj == NULL) {
		*attrobj = PyString_InternFromString(attrstr);
		if (*attrobj == NULL)
			return NULL;
	}
	res = _PyType_Lookup(self->ob_type, *attrobj);
	if (res != NULL) {
		descrgetfunc f;
		if ((f = res->ob_type->tp_descr_get) == NULL)
			Py_INCREF(res);
		else
			res = f(res, self, (PyObject *)(self->ob_type));
	}
	return res;
}

static PyObject *
lookup_method(PyObject *self, char *attrstr, PyObject **attrobj)
{
	PyObject *res = lookup_maybe(self, attrstr, attrobj);
	if (res == NULL && !PyErr_Occurred())
		PyErr_SetObject(PyExc_AttributeError, *attrobj);
	return res;
}

/* A variation of PyObject_CallMethod that uses lookup_method()
   instead of PyObject_GetAttrString().  This uses the same convention
   as lookup_method to cache the interned name string object. */

PyObject *
call_method(PyObject *o, char *name, PyObject **nameobj, char *format, ...)
{
	va_list va;
	PyObject *args, *func = 0, *retval;
	PyObject *dummy_str = NULL;
	va_start(va, format);

	func = lookup_maybe(o, name, &dummy_str);
	if (func == NULL) {
		va_end(va);
		if (!PyErr_Occurred())
			PyErr_SetObject(PyExc_AttributeError, dummy_str);
		Py_XDECREF(dummy_str);
		return NULL;
	}
	Py_DECREF(dummy_str);

	if (format && *format)
		args = Py_VaBuildValue(format, va);
	else
		args = PyTuple_New(0);

	va_end(va);

	if (args == NULL)
		return NULL;

	assert(PyTuple_Check(args));
	retval = PyObject_Call(func, args, NULL);

	Py_DECREF(args);
	Py_DECREF(func);

	return retval;
}

/* Clone of call_method() that returns NotImplemented when the lookup fails. */

PyObject *
call_maybe(PyObject *o, char *name, PyObject **nameobj, char *format, ...)
{
	va_list va;
	PyObject *args, *func = 0, *retval;
	PyObject *dummy_str = NULL;
	va_start(va, format);

	func = lookup_maybe(o, name, &dummy_str);
	Py_XDECREF(dummy_str);
	if (func == NULL) {
		va_end(va);
		if (!PyErr_Occurred()) {
			Py_INCREF(Py_NotImplemented);
			return Py_NotImplemented;
		}
		return NULL;
	}

	if (format && *format)
		args = Py_VaBuildValue(format, va);
	else
		args = PyTuple_New(0);

	va_end(va);

	if (args == NULL)
		return NULL;

	assert(PyTuple_Check(args));
	retval = PyObject_Call(func, args, NULL);

	Py_DECREF(args);
	Py_DECREF(func);

	return retval;
}

/* Method resolution order algorithm from "Putting Metaclasses to Work"
   by Forman and Danforth (Addison-Wesley 1999). */

static int
conservative_merge(PyObject *left, PyObject *right)
{
	int left_size;
	int right_size;
	int i, j, r, ok;
	PyObject *temp, *rr;

	assert(PyList_Check(left));
	assert(PyList_Check(right));

  again:
	left_size = PyList_GET_SIZE(left);
	right_size = PyList_GET_SIZE(right);
	for (i = 0; i < left_size; i++) {
		for (j = 0; j < right_size; j++) {
			if (PyList_GET_ITEM(left, i) ==
			    PyList_GET_ITEM(right, j)) {
				/* found a merge point */
				temp = PyList_New(0);
				if (temp == NULL)
					return -1;
				for (r = 0; r < j; r++) {
					rr = PyList_GET_ITEM(right, r);
					ok = PySequence_Contains(left, rr);
					if (ok < 0) {
						Py_DECREF(temp);
						return -1;
					}
					if (!ok) {
						ok = PyList_Append(temp, rr);
						if (ok < 0) {
							Py_DECREF(temp);
							return -1;
						}
					}
				}
				ok = PyList_SetSlice(left, i, i, temp);
				Py_DECREF(temp);
				if (ok < 0)
					return -1;
				ok = PyList_SetSlice(right, 0, j+1, NULL);
				if (ok < 0)
					return -1;
				goto again;
			}
		}
	}
	return PyList_SetSlice(left, left_size, left_size, right);
}

static int
serious_order_disagreements(PyObject *left, PyObject *right)
{
	return 0; /* XXX later -- for now, we cheat: "don't do that" */
}

static PyObject *
mro_implementation(PyTypeObject *type)
{
	int i, n, ok;
	PyObject *bases, *result;

	bases = type->tp_bases;
	n = PyTuple_GET_SIZE(bases);
	result = Py_BuildValue("[O]", (PyObject *)type);
	if (result == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		PyTypeObject *base =
			(PyTypeObject *) PyTuple_GET_ITEM(bases, i);
		PyObject *parentMRO = PySequence_List(base->tp_mro);
		if (parentMRO == NULL) {
			Py_DECREF(result);
			return NULL;
		}
		if (serious_order_disagreements(result, parentMRO)) {
			Py_DECREF(result);
			return NULL;
		}
		ok = conservative_merge(result, parentMRO);
		Py_DECREF(parentMRO);
		if (ok < 0) {
			Py_DECREF(result);
			return NULL;
		}
	}
	return result;
}

static PyObject *
mro_external(PyObject *self)
{
	PyTypeObject *type = (PyTypeObject *)self;

	return mro_implementation(type);
}

static int
mro_internal(PyTypeObject *type)
{
	PyObject *mro, *result, *tuple;

	if (type->ob_type == &PyType_Type) {
		result = mro_implementation(type);
	}
	else {
		static PyObject *mro_str;
		mro = lookup_method((PyObject *)type, "mro", &mro_str);
		if (mro == NULL)
			return -1;
		result = PyObject_CallObject(mro, NULL);
		Py_DECREF(mro);
	}
	if (result == NULL)
		return -1;
	tuple = PySequence_Tuple(result);
	Py_DECREF(result);
	type->tp_mro = tuple;
	return 0;
}


/* Calculate the best base amongst multiple base classes.
   This is the first one that's on the path to the "solid base". */

static PyTypeObject *
best_base(PyObject *bases)
{
	int i, n;
	PyTypeObject *base, *winner, *candidate, *base_i;

	assert(PyTuple_Check(bases));
	n = PyTuple_GET_SIZE(bases);
	assert(n > 0);
	base = (PyTypeObject *)PyTuple_GET_ITEM(bases, 0);
	winner = &PyBaseObject_Type;
	for (i = 0; i < n; i++) {
		base_i = (PyTypeObject *)PyTuple_GET_ITEM(bases, i);
		if (!PyType_Check((PyObject *)base_i)) {
			PyErr_SetString(
				PyExc_TypeError,
				"bases must be types");
			return NULL;
		}
		if (base_i->tp_dict == NULL) {
			if (PyType_Ready(base_i) < 0)
				return NULL;
		}
		candidate = solid_base(base_i);
		if (PyType_IsSubtype(winner, candidate))
			;
		else if (PyType_IsSubtype(candidate, winner)) {
			winner = candidate;
			base = base_i;
		}
		else {
			PyErr_SetString(
				PyExc_TypeError,
				"multiple bases have "
				"instance lay-out conflict");
			return NULL;
		}
	}
	assert(base != NULL);
	return base;
}

static int
extra_ivars(PyTypeObject *type, PyTypeObject *base)
{
	size_t t_size = type->tp_basicsize;
	size_t b_size = base->tp_basicsize;

	assert(t_size >= b_size); /* Else type smaller than base! */
	if (type->tp_itemsize || base->tp_itemsize) {
		/* If itemsize is involved, stricter rules */
		return t_size != b_size ||
			type->tp_itemsize != base->tp_itemsize;
	}
	if (type->tp_weaklistoffset && base->tp_weaklistoffset == 0 &&
	    type->tp_weaklistoffset + sizeof(PyObject *) == t_size)
		t_size -= sizeof(PyObject *);
	if (type->tp_dictoffset && base->tp_dictoffset == 0 &&
	    type->tp_dictoffset + sizeof(PyObject *) == t_size)
		t_size -= sizeof(PyObject *);

	return t_size != b_size;
}

static PyTypeObject *
solid_base(PyTypeObject *type)
{
	PyTypeObject *base;

	if (type->tp_base)
		base = solid_base(type->tp_base);
	else
		base = &PyBaseObject_Type;
	if (extra_ivars(type, base))
		return type;
	else
		return base;
}

staticforward void object_dealloc(PyObject *);
staticforward int object_init(PyObject *, PyObject *, PyObject *);

static PyObject *
subtype_dict(PyObject *obj, void *context)
{
	PyObject **dictptr = _PyObject_GetDictPtr(obj);
	PyObject *dict;

	if (dictptr == NULL) {
		PyErr_SetString(PyExc_AttributeError,
				"This object has no __dict__");
		return NULL;
	}
	dict = *dictptr;
	if (dict == NULL)
		*dictptr = dict = PyDict_New();
	Py_XINCREF(dict);
	return dict;
}

PyGetSetDef subtype_getsets[] = {
	{"__dict__", subtype_dict, NULL, NULL},
	{0},
};

static PyObject *
type_new(PyTypeObject *metatype, PyObject *args, PyObject *kwds)
{
	PyObject *name, *bases, *dict;
	static char *kwlist[] = {"name", "bases", "dict", 0};
	PyObject *slots, *tmp;
	PyTypeObject *type, *base, *tmptype, *winner;
	etype *et;
	PyMemberDef *mp;
	int i, nbases, nslots, slotoffset, dynamic, add_dict, add_weak;

	/* Special case: type(x) should return x->ob_type */
	if (metatype == &PyType_Type &&
	    PyTuple_Check(args) && PyTuple_GET_SIZE(args) == 1 &&
	    (kwds == NULL || (PyDict_Check(kwds) && PyDict_Size(kwds) == 0))) {
		PyObject *x = PyTuple_GET_ITEM(args, 0);
		Py_INCREF(x->ob_type);
		return (PyObject *) x->ob_type;
	}

	/* Check arguments: (name, bases, dict) */
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "SO!O!:type", kwlist,
					 &name,
					 &PyTuple_Type, &bases,
					 &PyDict_Type, &dict))
		return NULL;

	/* Determine the proper metatype to deal with this,
	   and check for metatype conflicts while we're at it.
	   Note that if some other metatype wins to contract,
	   it's possible that its instances are not types. */
	nbases = PyTuple_GET_SIZE(bases);
	winner = metatype;
	for (i = 0; i < nbases; i++) {
		tmp = PyTuple_GET_ITEM(bases, i);
		tmptype = tmp->ob_type;
		if (PyType_IsSubtype(winner, tmptype))
			continue;
		if (PyType_IsSubtype(tmptype, winner)) {
			winner = tmptype;
			continue;
		}
		PyErr_SetString(PyExc_TypeError,
				"metatype conflict among bases");
		return NULL;
	}
	if (winner != metatype) {
		if (winner->tp_new != type_new) /* Pass it to the winner */
			return winner->tp_new(winner, args, kwds);
		metatype = winner;
	}

	/* Adjust for empty tuple bases */
	if (nbases == 0) {
		bases = Py_BuildValue("(O)", &PyBaseObject_Type);
		if (bases == NULL)
			return NULL;
		nbases = 1;
	}
	else
		Py_INCREF(bases);

	/* XXX From here until type is allocated, "return NULL" leaks bases! */

	/* Calculate best base, and check that all bases are type objects */
	base = best_base(bases);
	if (base == NULL)
		return NULL;
	if (!PyType_HasFeature(base, Py_TPFLAGS_BASETYPE)) {
		PyErr_Format(PyExc_TypeError,
			     "type '%.100s' is not an acceptable base type",
			     base->tp_name);
		return NULL;
	}

	/* Should this be a dynamic class (i.e. modifiable __dict__)?
	   Look in two places for a variable named __dynamic__:
	   1) in the class dict
	   2) in the module dict (globals)
	   The first variable that is an int >= 0 is used.
	   Otherwise, a default is calculated from the base classes:
	   if any base class is dynamic, this class is dynamic; otherwise
	   it is static. */
	dynamic = -1; /* Not yet determined */
	/* Look in the class */
	tmp = PyDict_GetItemString(dict, "__dynamic__");
	if (tmp != NULL) {
		dynamic = PyInt_AsLong(tmp);
		if (dynamic < 0)
			PyErr_Clear();
	}
	if (dynamic < 0) {
		/* Look in the module globals */
		tmp = PyEval_GetGlobals();
		if (tmp != NULL) {
			tmp = PyDict_GetItemString(tmp, "__dynamic__");
			if (tmp != NULL) {
				dynamic = PyInt_AsLong(tmp);
				if (dynamic < 0)
					PyErr_Clear();
			}
		}
	}
	if (dynamic < 0) {
		/* Make a new class dynamic if any of its bases is
		   dynamic.  This is not always the same as inheriting
		   the __dynamic__ class attribute! */
		dynamic = 0;
		for (i = 0; i < nbases; i++) {
			tmptype = (PyTypeObject *)
				PyTuple_GET_ITEM(bases, i);
			if (tmptype->tp_flags &
			    Py_TPFLAGS_DYNAMICTYPE) {
				dynamic = 1;
				break;
			}
		}
	}

	/* Check for a __slots__ sequence variable in dict, and count it */
	slots = PyDict_GetItemString(dict, "__slots__");
	nslots = 0;
	add_dict = 0;
	add_weak = 0;
	if (slots != NULL) {
		/* Make it into a tuple */
		if (PyString_Check(slots))
			slots = Py_BuildValue("(O)", slots);
		else
			slots = PySequence_Tuple(slots);
		if (slots == NULL)
			return NULL;
		nslots = PyTuple_GET_SIZE(slots);
		if (nslots > 0 && base->tp_itemsize != 0) {
			PyErr_Format(PyExc_TypeError,
				     "nonempty __slots__ "
				     "not supported for subtype of '%s'",
				     base->tp_name);
			return NULL;
		}
		for (i = 0; i < nslots; i++) {
			if (!PyString_Check(PyTuple_GET_ITEM(slots, i))) {
				PyErr_SetString(PyExc_TypeError,
				"__slots__ must be a sequence of strings");
				Py_DECREF(slots);
				return NULL;
			}
			/* XXX Check against null bytes in name */
		}
	}
	if (slots == NULL && base->tp_dictoffset == 0 &&
	    (base->tp_setattro == PyObject_GenericSetAttr ||
	     base->tp_setattro == NULL)) {
		add_dict++;
	}
	if (slots == NULL && base->tp_weaklistoffset == 0 &&
	    base->tp_itemsize == 0) {
		nslots++;
		add_weak++;
	}

	/* XXX From here until type is safely allocated,
	   "return NULL" may leak slots! */

	/* Allocate the type object */
	type = (PyTypeObject *)metatype->tp_alloc(metatype, nslots);
	if (type == NULL)
		return NULL;

	/* Keep name and slots alive in the extended type object */
	et = (etype *)type;
	Py_INCREF(name);
	et->name = name;
	et->slots = slots;

	/* Initialize tp_flags */
	type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE |
		Py_TPFLAGS_BASETYPE;
	if (dynamic)
		type->tp_flags |= Py_TPFLAGS_DYNAMICTYPE;

	/* It's a new-style number unless it specifically inherits any
	   old-style numeric behavior */
	if ((base->tp_flags & Py_TPFLAGS_CHECKTYPES) ||
	    (base->tp_as_number == NULL))
		type->tp_flags |= Py_TPFLAGS_CHECKTYPES;

	/* Initialize essential fields */
	type->tp_as_number = &et->as_number;
	type->tp_as_sequence = &et->as_sequence;
	type->tp_as_mapping = &et->as_mapping;
	type->tp_as_buffer = &et->as_buffer;
	type->tp_name = PyString_AS_STRING(name);

	/* Set tp_base and tp_bases */
	type->tp_bases = bases;
	Py_INCREF(base);
	type->tp_base = base;

	/* Initialize tp_defined from passed-in dict */
	type->tp_defined = dict = PyDict_Copy(dict);
	if (dict == NULL) {
		Py_DECREF(type);
		return NULL;
	}

	/* Set __module__ in the dict */
	if (PyDict_GetItemString(dict, "__module__") == NULL) {
		tmp = PyEval_GetGlobals();
		if (tmp != NULL) {
			tmp = PyDict_GetItemString(tmp, "__name__");
			if (tmp != NULL) {
				if (PyDict_SetItemString(dict, "__module__",
							 tmp) < 0)
					return NULL;
			}
		}
	}

	/* Special-case __new__: if it's a plain function,
	   make it a static function */
	tmp = PyDict_GetItemString(dict, "__new__");
	if (tmp != NULL && PyFunction_Check(tmp)) {
		tmp = PyStaticMethod_New(tmp);
		if (tmp == NULL) {
			Py_DECREF(type);
			return NULL;
		}
		PyDict_SetItemString(dict, "__new__", tmp);
		Py_DECREF(tmp);
	}

	/* Add descriptors for custom slots from __slots__, or for __dict__ */
	mp = et->members;
	slotoffset = base->tp_basicsize;
	if (slots != NULL) {
		for (i = 0; i < nslots; i++, mp++) {
			mp->name = PyString_AS_STRING(
				PyTuple_GET_ITEM(slots, i));
			mp->type = T_OBJECT;
			mp->offset = slotoffset;
			if (base->tp_weaklistoffset == 0 &&
			    strcmp(mp->name, "__weakref__") == 0)
				type->tp_weaklistoffset = slotoffset;
			slotoffset += sizeof(PyObject *);
		}
	}
	else {
		if (add_dict) {
			if (base->tp_itemsize)
				type->tp_dictoffset = -(long)sizeof(PyObject *);
			else
				type->tp_dictoffset = slotoffset;
			slotoffset += sizeof(PyObject *);
			type->tp_getset = subtype_getsets;
		}
		if (add_weak) {
			assert(!base->tp_itemsize);
			type->tp_weaklistoffset = slotoffset;
			mp->name = "__weakref__";
			mp->type = T_OBJECT;
			mp->offset = slotoffset;
			mp->flags = READONLY;
			mp++;
			slotoffset += sizeof(PyObject *);
		}
	}
	type->tp_basicsize = slotoffset;
	type->tp_itemsize = base->tp_itemsize;
	type->tp_members = et->members;

	/* Special case some slots */
	if (type->tp_dictoffset != 0 || nslots > 0) {
		if (base->tp_getattr == NULL && base->tp_getattro == NULL)
			type->tp_getattro = PyObject_GenericGetAttr;
		if (base->tp_setattr == NULL && base->tp_setattro == NULL)
			type->tp_setattro = PyObject_GenericSetAttr;
	}
	type->tp_dealloc = subtype_dealloc;

	/* Always override allocation strategy to use regular heap */
	type->tp_alloc = PyType_GenericAlloc;
	type->tp_free = _PyObject_Del;

	/* Initialize the rest */
	if (PyType_Ready(type) < 0) {
		Py_DECREF(type);
		return NULL;
	}

	/* Override slots that deserve it */
	if (!PyType_HasFeature(type, Py_TPFLAGS_DYNAMICTYPE))
		override_slots(type, type->tp_defined);

	return (PyObject *)type;
}

/* Internal API to look for a name through the MRO.
   This returns a borrowed reference, and doesn't set an exception! */
PyObject *
_PyType_Lookup(PyTypeObject *type, PyObject *name)
{
	int i, n;
	PyObject *mro, *res, *dict;

	/* For static types, look in tp_dict */
	if (!(type->tp_flags & Py_TPFLAGS_DYNAMICTYPE)) {
		dict = type->tp_dict;
		assert(dict && PyDict_Check(dict));
		return PyDict_GetItem(dict, name);
	}

	/* For dynamic types, look in tp_defined of types in MRO */
	mro = type->tp_mro;
	assert(PyTuple_Check(mro));
	n = PyTuple_GET_SIZE(mro);
	for (i = 0; i < n; i++) {
		type = (PyTypeObject *) PyTuple_GET_ITEM(mro, i);
		assert(PyType_Check(type));
		dict = type->tp_defined;
		assert(dict && PyDict_Check(dict));
		res = PyDict_GetItem(dict, name);
		if (res != NULL)
			return res;
	}
	return NULL;
}

/* This is similar to PyObject_GenericGetAttr(),
   but uses _PyType_Lookup() instead of just looking in type->tp_dict. */
static PyObject *
type_getattro(PyTypeObject *type, PyObject *name)
{
	PyTypeObject *metatype = type->ob_type;
	PyObject *descr, *res;
	descrgetfunc f;

	/* Initialize this type (we'll assume the metatype is initialized) */
	if (type->tp_dict == NULL) {
		if (PyType_Ready(type) < 0)
			return NULL;
	}

	/* Get a descriptor from the metatype */
	descr = _PyType_Lookup(metatype, name);
	f = NULL;
	if (descr != NULL) {
		f = descr->ob_type->tp_descr_get;
		if (f != NULL && PyDescr_IsData(descr))
			return f(descr,
				 (PyObject *)type, (PyObject *)metatype);
	}

	/* Look in tp_defined of this type and its bases */
	res = _PyType_Lookup(type, name);
	if (res != NULL) {
		f = res->ob_type->tp_descr_get;
		if (f != NULL)
			return f(res, (PyObject *)NULL, (PyObject *)type);
		Py_INCREF(res);
		return res;
	}

	/* Use the descriptor from the metatype */
	if (f != NULL) {
		res = f(descr, (PyObject *)type, (PyObject *)metatype);
		return res;
	}
	if (descr != NULL) {
		Py_INCREF(descr);
		return descr;
	}

	/* Give up */
	PyErr_Format(PyExc_AttributeError,
		     "type object '%.50s' has no attribute '%.400s'",
		     type->tp_name, PyString_AS_STRING(name));
	return NULL;
}

static int
type_setattro(PyTypeObject *type, PyObject *name, PyObject *value)
{
	if (type->tp_flags & Py_TPFLAGS_DYNAMICTYPE)
		return PyObject_GenericSetAttr((PyObject *)type, name, value);
	PyErr_SetString(PyExc_TypeError, "can't set type attributes");
	return -1;
}

static void
type_dealloc(PyTypeObject *type)
{
	etype *et;

	/* Assert this is a heap-allocated type object */
	assert(type->tp_flags & Py_TPFLAGS_HEAPTYPE);
	et = (etype *)type;
	Py_XDECREF(type->tp_base);
	Py_XDECREF(type->tp_dict);
	Py_XDECREF(type->tp_bases);
	Py_XDECREF(type->tp_mro);
	Py_XDECREF(type->tp_defined);
	/* XXX more? */
	Py_XDECREF(et->name);
	Py_XDECREF(et->slots);
	type->ob_type->tp_free((PyObject *)type);
}

static PyMethodDef type_methods[] = {
	{"mro", (PyCFunction)mro_external, METH_NOARGS,
	 "mro() -> list\nreturn a type's method resolution order"},
	{0}
};

static char type_doc[] =
"type(object) -> the object's type\n"
"type(name, bases, dict) -> a new type";

PyTypeObject PyType_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"type",					/* tp_name */
	sizeof(etype),				/* tp_basicsize */
	sizeof(PyMemberDef),			/* tp_itemsize */
	(destructor)type_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,			 		/* tp_getattr */
	0,					/* tp_setattr */
	type_compare,				/* tp_compare */
	(reprfunc)type_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)_Py_HashPointer,		/* tp_hash */
	(ternaryfunc)type_call,			/* tp_call */
	0,					/* tp_str */
	(getattrofunc)type_getattro,		/* tp_getattro */
	(setattrofunc)type_setattro,		/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	type_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	type_methods,				/* tp_methods */
	type_members,				/* tp_members */
	type_getsets,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	offsetof(PyTypeObject, tp_dict),	/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	type_new,				/* tp_new */
};


/* The base type of all types (eventually)... except itself. */

static int
object_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	return 0;
}

static void
object_dealloc(PyObject *self)
{
	self->ob_type->tp_free(self);
}

static PyObject *
object_repr(PyObject *self)
{
	PyTypeObject *type;
	PyObject *mod, *name, *rtn;

	type = self->ob_type;
	mod = type_module(type, NULL);
	if (mod == NULL)
		PyErr_Clear();
	else if (!PyString_Check(mod)) {
		Py_DECREF(mod);
		mod = NULL;
	}
	name = type_name(type, NULL);
	if (name == NULL)
		return NULL;
	if (mod != NULL && strcmp(PyString_AS_STRING(mod), "__builtin__"))
		rtn = PyString_FromFormat("<%s.%s object at %p>",
					  PyString_AS_STRING(mod),
					  PyString_AS_STRING(name),
					  self);
	else
		rtn = PyString_FromFormat("<%s object at %p>",
					  type->tp_name, self);
	Py_XDECREF(mod);
	Py_DECREF(name);
	return rtn;
}

static PyObject *
object_str(PyObject *self)
{
	unaryfunc f;

	f = self->ob_type->tp_repr;
	if (f == NULL)
		f = object_repr;
	return f(self);
}

static long
object_hash(PyObject *self)
{
	return _Py_HashPointer(self);
}

static void
object_free(PyObject *self)
{
	PyObject_Del(self);
}

static PyObject *
object_get_class(PyObject *self, void *closure)
{
	Py_INCREF(self->ob_type);
	return (PyObject *)(self->ob_type);
}

static int
equiv_structs(PyTypeObject *a, PyTypeObject *b)
{
	return a == b ||
	       (a != NULL &&
		b != NULL &&
		a->tp_basicsize == b->tp_basicsize &&
		a->tp_itemsize == b->tp_itemsize &&
		a->tp_dictoffset == b->tp_dictoffset &&
		a->tp_weaklistoffset == b->tp_weaklistoffset &&
		((a->tp_flags & Py_TPFLAGS_HAVE_GC) ==
		 (b->tp_flags & Py_TPFLAGS_HAVE_GC)));
}

static int
same_slots_added(PyTypeObject *a, PyTypeObject *b)
{
	PyTypeObject *base = a->tp_base;
	int size;

	if (base != b->tp_base)
		return 0;
	if (equiv_structs(a, base) && equiv_structs(b, base))
		return 1;
	size = base->tp_basicsize;
	if (a->tp_dictoffset == size && b->tp_dictoffset == size)
		size += sizeof(PyObject *);
	if (a->tp_weaklistoffset == size && b->tp_weaklistoffset == size)
		size += sizeof(PyObject *);
	return size == a->tp_basicsize && size == b->tp_basicsize;
}

static int
object_set_class(PyObject *self, PyObject *value, void *closure)
{
	PyTypeObject *old = self->ob_type;
	PyTypeObject *new, *newbase, *oldbase;

	if (!PyType_Check(value)) {
		PyErr_Format(PyExc_TypeError,
		  "__class__ must be set to new-style class, not '%s' object",
		  value->ob_type->tp_name);
		return -1;
	}
	new = (PyTypeObject *)value;
	newbase = new;
	oldbase = old;
	while (equiv_structs(newbase, newbase->tp_base))
		newbase = newbase->tp_base;
	while (equiv_structs(oldbase, oldbase->tp_base))
		oldbase = oldbase->tp_base;
	if (newbase != oldbase &&
	    (newbase->tp_base != oldbase->tp_base ||
	     !same_slots_added(newbase, oldbase))) {
		PyErr_Format(PyExc_TypeError,
			     "__class__ assignment: "
			     "'%s' object layout differs from '%s'",
			     new->tp_name, 
			     old->tp_name);
		return -1;
	}
	if (new->tp_flags & Py_TPFLAGS_HEAPTYPE) {
		Py_INCREF(new);
	}
	self->ob_type = new;
	if (old->tp_flags & Py_TPFLAGS_HEAPTYPE) {
		Py_DECREF(old);
	}
	return 0;
}

static PyGetSetDef object_getsets[] = {
	{"__class__", object_get_class, object_set_class,
	 "the object's class"},
	{0}
};

static PyObject *
object_reduce(PyObject *self, PyObject *args)
{
	/* Call copy_reg._reduce(self) */
	static PyObject *copy_reg_str;
	PyObject *copy_reg, *res;

	if (!copy_reg_str) {
		copy_reg_str = PyString_InternFromString("copy_reg");
		if (copy_reg_str == NULL)
			return NULL;
	}
	copy_reg = PyImport_Import(copy_reg_str);
	if (!copy_reg)
		return NULL;
	res = PyEval_CallMethod(copy_reg, "_reduce", "(O)", self);
	Py_DECREF(copy_reg);
	return res;
}

static PyMethodDef object_methods[] = {
	{"__reduce__", object_reduce, METH_NOARGS, "helper for pickle"},
	{0}
};

PyTypeObject PyBaseObject_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
 	0,					/* ob_size */
	"object",				/* tp_name */
	sizeof(PyObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)object_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,			 		/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	object_repr,				/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	object_hash,				/* tp_hash */
	0,					/* tp_call */
	object_str,				/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	PyObject_GenericSetAttr,		/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"The most base type",			/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	object_methods,				/* tp_methods */
	0,					/* tp_members */
	object_getsets,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	object_init,				/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	object_free,				/* tp_free */
};


/* Initialize the __dict__ in a type object */

static int
add_methods(PyTypeObject *type, PyMethodDef *meth)
{
	PyObject *dict = type->tp_defined;

	for (; meth->ml_name != NULL; meth++) {
		PyObject *descr;
		if (PyDict_GetItemString(dict, meth->ml_name))
			continue;
		descr = PyDescr_NewMethod(type, meth);
		if (descr == NULL)
			return -1;
		if (PyDict_SetItemString(dict,meth->ml_name,descr) < 0)
			return -1;
		Py_DECREF(descr);
	}
	return 0;
}

static int
add_members(PyTypeObject *type, PyMemberDef *memb)
{
	PyObject *dict = type->tp_defined;

	for (; memb->name != NULL; memb++) {
		PyObject *descr;
		if (PyDict_GetItemString(dict, memb->name))
			continue;
		descr = PyDescr_NewMember(type, memb);
		if (descr == NULL)
			return -1;
		if (PyDict_SetItemString(dict, memb->name, descr) < 0)
			return -1;
		Py_DECREF(descr);
	}
	return 0;
}

static int
add_getset(PyTypeObject *type, PyGetSetDef *gsp)
{
	PyObject *dict = type->tp_defined;

	for (; gsp->name != NULL; gsp++) {
		PyObject *descr;
		if (PyDict_GetItemString(dict, gsp->name))
			continue;
		descr = PyDescr_NewGetSet(type, gsp);

		if (descr == NULL)
			return -1;
		if (PyDict_SetItemString(dict, gsp->name, descr) < 0)
			return -1;
		Py_DECREF(descr);
	}
	return 0;
}

static void
inherit_special(PyTypeObject *type, PyTypeObject *base)
{
	int oldsize, newsize;

	/* Special flag magic */
	if (!type->tp_as_buffer && base->tp_as_buffer) {
		type->tp_flags &= ~Py_TPFLAGS_HAVE_GETCHARBUFFER;
		type->tp_flags |=
			base->tp_flags & Py_TPFLAGS_HAVE_GETCHARBUFFER;
	}
	if (!type->tp_as_sequence && base->tp_as_sequence) {
		type->tp_flags &= ~Py_TPFLAGS_HAVE_SEQUENCE_IN;
		type->tp_flags |= base->tp_flags & Py_TPFLAGS_HAVE_SEQUENCE_IN;
	}
	if ((type->tp_flags & Py_TPFLAGS_HAVE_INPLACEOPS) !=
	    (base->tp_flags & Py_TPFLAGS_HAVE_INPLACEOPS)) {
		if ((!type->tp_as_number && base->tp_as_number) ||
		    (!type->tp_as_sequence && base->tp_as_sequence)) {
			type->tp_flags &= ~Py_TPFLAGS_HAVE_INPLACEOPS;
			if (!type->tp_as_number && !type->tp_as_sequence) {
				type->tp_flags |= base->tp_flags &
					Py_TPFLAGS_HAVE_INPLACEOPS;
			}
		}
		/* Wow */
	}
	if (!type->tp_as_number && base->tp_as_number) {
		type->tp_flags &= ~Py_TPFLAGS_CHECKTYPES;
		type->tp_flags |= base->tp_flags & Py_TPFLAGS_CHECKTYPES;
	}

	/* Copying basicsize is connected to the GC flags */
	oldsize = base->tp_basicsize;
	newsize = type->tp_basicsize ? type->tp_basicsize : oldsize;
	if (!(type->tp_flags & Py_TPFLAGS_HAVE_GC) &&
	    (base->tp_flags & Py_TPFLAGS_HAVE_GC) &&
	    (type->tp_flags & Py_TPFLAGS_HAVE_RICHCOMPARE/*GC slots exist*/) &&
	    (!type->tp_traverse && !type->tp_clear)) {
		type->tp_flags |= Py_TPFLAGS_HAVE_GC;
		if (type->tp_traverse == NULL)
			type->tp_traverse = base->tp_traverse;
		if (type->tp_clear == NULL)
			type->tp_clear = base->tp_clear;
	}
	if (type->tp_flags & base->tp_flags & Py_TPFLAGS_HAVE_CLASS) {
		if (base != &PyBaseObject_Type ||
		    (type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
			if (type->tp_new == NULL)
				type->tp_new = base->tp_new;
		}
	}
	type->tp_basicsize = newsize;

	/* Copy other non-function slots */

#undef COPYVAL
#define COPYVAL(SLOT) \
	if (type->SLOT == 0) type->SLOT = base->SLOT

	COPYVAL(tp_itemsize);
	if (type->tp_flags & base->tp_flags & Py_TPFLAGS_HAVE_WEAKREFS) {
		COPYVAL(tp_weaklistoffset);
	}
	if (type->tp_flags & base->tp_flags & Py_TPFLAGS_HAVE_CLASS) {
		COPYVAL(tp_dictoffset);
	}
}

static void
inherit_slots(PyTypeObject *type, PyTypeObject *base)
{
	PyTypeObject *basebase;

#undef SLOTDEFINED
#undef COPYSLOT
#undef COPYNUM
#undef COPYSEQ
#undef COPYMAP

#define SLOTDEFINED(SLOT) \
	(base->SLOT != 0 && \
	 (basebase == NULL || base->SLOT != basebase->SLOT))

#define COPYSLOT(SLOT) \
	if (!type->SLOT && SLOTDEFINED(SLOT)) type->SLOT = base->SLOT

#define COPYNUM(SLOT) COPYSLOT(tp_as_number->SLOT)
#define COPYSEQ(SLOT) COPYSLOT(tp_as_sequence->SLOT)
#define COPYMAP(SLOT) COPYSLOT(tp_as_mapping->SLOT)

	/* This won't inherit indirect slots (from tp_as_number etc.)
	   if type doesn't provide the space. */

	if (type->tp_as_number != NULL && base->tp_as_number != NULL) {
		basebase = base->tp_base;
		if (basebase->tp_as_number == NULL)
			basebase = NULL;
		COPYNUM(nb_add);
		COPYNUM(nb_subtract);
		COPYNUM(nb_multiply);
		COPYNUM(nb_divide);
		COPYNUM(nb_remainder);
		COPYNUM(nb_divmod);
		COPYNUM(nb_power);
		COPYNUM(nb_negative);
		COPYNUM(nb_positive);
		COPYNUM(nb_absolute);
		COPYNUM(nb_nonzero);
		COPYNUM(nb_invert);
		COPYNUM(nb_lshift);
		COPYNUM(nb_rshift);
		COPYNUM(nb_and);
		COPYNUM(nb_xor);
		COPYNUM(nb_or);
		COPYNUM(nb_coerce);
		COPYNUM(nb_int);
		COPYNUM(nb_long);
		COPYNUM(nb_float);
		COPYNUM(nb_oct);
		COPYNUM(nb_hex);
		COPYNUM(nb_inplace_add);
		COPYNUM(nb_inplace_subtract);
		COPYNUM(nb_inplace_multiply);
		COPYNUM(nb_inplace_divide);
		COPYNUM(nb_inplace_remainder);
		COPYNUM(nb_inplace_power);
		COPYNUM(nb_inplace_lshift);
		COPYNUM(nb_inplace_rshift);
		COPYNUM(nb_inplace_and);
		COPYNUM(nb_inplace_xor);
		COPYNUM(nb_inplace_or);
		if (base->tp_flags & Py_TPFLAGS_CHECKTYPES) {
			COPYNUM(nb_true_divide);
			COPYNUM(nb_floor_divide);
			COPYNUM(nb_inplace_true_divide);
			COPYNUM(nb_inplace_floor_divide);
		}
	}

	if (type->tp_as_sequence != NULL && base->tp_as_sequence != NULL) {
		basebase = base->tp_base;
		if (basebase->tp_as_sequence == NULL)
			basebase = NULL;
		COPYSEQ(sq_length);
		COPYSEQ(sq_concat);
		COPYSEQ(sq_repeat);
		COPYSEQ(sq_item);
		COPYSEQ(sq_slice);
		COPYSEQ(sq_ass_item);
		COPYSEQ(sq_ass_slice);
		COPYSEQ(sq_contains);
		COPYSEQ(sq_inplace_concat);
		COPYSEQ(sq_inplace_repeat);
	}

	if (type->tp_as_mapping != NULL && base->tp_as_mapping != NULL) {
		basebase = base->tp_base;
		if (basebase->tp_as_mapping == NULL)
			basebase = NULL;
		COPYMAP(mp_length);
		COPYMAP(mp_subscript);
		COPYMAP(mp_ass_subscript);
	}

	basebase = base->tp_base;

	COPYSLOT(tp_dealloc);
	COPYSLOT(tp_print);
	if (type->tp_getattr == NULL && type->tp_getattro == NULL) {
		type->tp_getattr = base->tp_getattr;
		type->tp_getattro = base->tp_getattro;
	}
	if (type->tp_setattr == NULL && type->tp_setattro == NULL) {
		type->tp_setattr = base->tp_setattr;
		type->tp_setattro = base->tp_setattro;
	}
	/* tp_compare see tp_richcompare */
	COPYSLOT(tp_repr);
	/* tp_hash see tp_richcompare */
	COPYSLOT(tp_call);
	COPYSLOT(tp_str);
	COPYSLOT(tp_as_buffer);
	if (type->tp_flags & base->tp_flags & Py_TPFLAGS_HAVE_RICHCOMPARE) {
		if (type->tp_compare == NULL &&
		    type->tp_richcompare == NULL &&
		    type->tp_hash == NULL)
		{
			type->tp_compare = base->tp_compare;
			type->tp_richcompare = base->tp_richcompare;
			type->tp_hash = base->tp_hash;
		}
	}
	else {
		COPYSLOT(tp_compare);
	}
	if (type->tp_flags & base->tp_flags & Py_TPFLAGS_HAVE_ITER) {
		COPYSLOT(tp_iter);
		COPYSLOT(tp_iternext);
	}
	if (type->tp_flags & base->tp_flags & Py_TPFLAGS_HAVE_CLASS) {
		COPYSLOT(tp_descr_get);
		COPYSLOT(tp_descr_set);
		COPYSLOT(tp_dictoffset);
		COPYSLOT(tp_init);
		COPYSLOT(tp_alloc);
		COPYSLOT(tp_free);
	}
}

staticforward int add_operators(PyTypeObject *);

int
PyType_Ready(PyTypeObject *type)
{
	PyObject *dict, *bases, *x;
	PyTypeObject *base;
	int i, n;

	if (type->tp_flags & Py_TPFLAGS_READY) {
		assert(type->tp_dict != NULL);
		return 0;
	}
	assert((type->tp_flags & Py_TPFLAGS_READYING) == 0);
	assert(type->tp_dict == NULL);

	type->tp_flags |= Py_TPFLAGS_READYING;

	/* Initialize tp_base (defaults to BaseObject unless that's us) */
	base = type->tp_base;
	if (base == NULL && type != &PyBaseObject_Type)
		base = type->tp_base = &PyBaseObject_Type;

	/* Initialize tp_bases */
	bases = type->tp_bases;
	if (bases == NULL) {
		if (base == NULL)
			bases = PyTuple_New(0);
		else
			bases = Py_BuildValue("(O)", base);
		if (bases == NULL)
			goto error;
		type->tp_bases = bases;
	}

	/* Initialize the base class */
	if (base && base->tp_dict == NULL) {
		if (PyType_Ready(base) < 0)
			goto error;
	}

	/* Initialize tp_defined */
	dict = type->tp_defined;
	if (dict == NULL) {
		dict = PyDict_New();
		if (dict == NULL)
			goto error;
		type->tp_defined = dict;
	}

	/* Add type-specific descriptors to tp_defined */
	if (add_operators(type) < 0)
		goto error;
	if (type->tp_methods != NULL) {
		if (add_methods(type, type->tp_methods) < 0)
			goto error;
	}
	if (type->tp_members != NULL) {
		if (add_members(type, type->tp_members) < 0)
			goto error;
	}
	if (type->tp_getset != NULL) {
		if (add_getset(type, type->tp_getset) < 0)
			goto error;
	}

	/* Temporarily make tp_dict the same object as tp_defined.
	   (This is needed to call mro(), and can stay this way for
	   dynamic types). */
	Py_INCREF(type->tp_defined);
	type->tp_dict = type->tp_defined;

	/* Calculate method resolution order */
	if (mro_internal(type) < 0) {
		goto error;
	}

	/* Inherit special flags from dominant base */
	if (type->tp_base != NULL)
		inherit_special(type, type->tp_base);

	/* Initialize tp_dict properly */
	if (PyType_HasFeature(type, Py_TPFLAGS_DYNAMICTYPE)) {
		/* For a dynamic type, all slots are overridden */
		override_slots(type, NULL);
	}
	else {
		/* For a static type, tp_dict is the consolidation
		   of the tp_defined of its bases in MRO. */
		Py_DECREF(type->tp_dict);
		type->tp_dict = PyDict_Copy(type->tp_defined);
		if (type->tp_dict == NULL)
			goto error;
		bases = type->tp_mro;
		assert(bases != NULL);
		assert(PyTuple_Check(bases));
		n = PyTuple_GET_SIZE(bases);
		for (i = 1; i < n; i++) {
			base = (PyTypeObject *)PyTuple_GET_ITEM(bases, i);
			assert(PyType_Check(base));
			x = base->tp_defined;
			if (x != NULL && PyDict_Merge(type->tp_dict, x, 0) < 0)
				goto error;
			inherit_slots(type, base);
		}
	}

	/* Some more special stuff */
	base = type->tp_base;
	if (base != NULL) {
		if (type->tp_as_number == NULL)
			type->tp_as_number = base->tp_as_number;
		if (type->tp_as_sequence == NULL)
			type->tp_as_sequence = base->tp_as_sequence;
		if (type->tp_as_mapping == NULL)
			type->tp_as_mapping = base->tp_as_mapping;
	}

	/* All done -- set the ready flag */
	assert(type->tp_dict != NULL);
	type->tp_flags =
		(type->tp_flags & ~Py_TPFLAGS_READYING) | Py_TPFLAGS_READY;
	return 0;

  error:
	type->tp_flags &= ~Py_TPFLAGS_READYING;
	return -1;
}


/* Generic wrappers for overloadable 'operators' such as __getitem__ */

/* There's a wrapper *function* for each distinct function typedef used
   for type object slots (e.g. binaryfunc, ternaryfunc, etc.).  There's a
   wrapper *table* for each distinct operation (e.g. __len__, __add__).
   Most tables have only one entry; the tables for binary operators have two
   entries, one regular and one with reversed arguments. */

static PyObject *
wrap_inquiry(PyObject *self, PyObject *args, void *wrapped)
{
	inquiry func = (inquiry)wrapped;
	int res;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	res = (*func)(self);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	return PyInt_FromLong((long)res);
}

static struct wrapperbase tab_len[] = {
	{"__len__", (wrapperfunc)wrap_inquiry, "x.__len__() <==> len(x)"},
	{0}
};

static PyObject *
wrap_binaryfunc(PyObject *self, PyObject *args, void *wrapped)
{
	binaryfunc func = (binaryfunc)wrapped;
	PyObject *other;

	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;
	return (*func)(self, other);
}

static PyObject *
wrap_binaryfunc_r(PyObject *self, PyObject *args, void *wrapped)
{
	binaryfunc func = (binaryfunc)wrapped;
	PyObject *other;

	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;
	return (*func)(other, self);
}

#undef BINARY
#define BINARY(NAME, OP) \
static struct wrapperbase tab_##NAME[] = { \
	{"__" #NAME "__", \
	 (wrapperfunc)wrap_binaryfunc, \
	 "x.__" #NAME "__(y) <==> " #OP}, \
	{"__r" #NAME "__", \
	 (wrapperfunc)wrap_binaryfunc_r, \
	 "y.__r" #NAME "__(x) <==> " #OP}, \
	{0} \
}

BINARY(add, "x+y");
BINARY(sub, "x-y");
BINARY(mul, "x*y");
BINARY(div, "x/y");
BINARY(mod, "x%y");
BINARY(divmod, "divmod(x,y)");
BINARY(lshift, "x<<y");
BINARY(rshift, "x>>y");
BINARY(and, "x&y");
BINARY(xor, "x^y");
BINARY(or, "x|y");

static PyObject *
wrap_coercefunc(PyObject *self, PyObject *args, void *wrapped)
{
	coercion func = (coercion)wrapped;
	PyObject *other, *res;
	int ok;

	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;
	ok = func(&self, &other);
	if (ok < 0)
		return NULL;
	if (ok > 0) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	res = PyTuple_New(2);
	if (res == NULL) {
		Py_DECREF(self);
		Py_DECREF(other);
		return NULL;
	}
	PyTuple_SET_ITEM(res, 0, self);
	PyTuple_SET_ITEM(res, 1, other);
	return res;
}

static struct wrapperbase tab_coerce[] = {
	{"__coerce__", (wrapperfunc)wrap_coercefunc,
	 "x.__coerce__(y) <==> coerce(x, y)"},
	{0}
};

BINARY(floordiv, "x//y");
BINARY(truediv, "x/y # true division");

static PyObject *
wrap_ternaryfunc(PyObject *self, PyObject *args, void *wrapped)
{
	ternaryfunc func = (ternaryfunc)wrapped;
	PyObject *other;
	PyObject *third = Py_None;

	/* Note: This wrapper only works for __pow__() */

	if (!PyArg_ParseTuple(args, "O|O", &other, &third))
		return NULL;
	return (*func)(self, other, third);
}

#undef TERNARY
#define TERNARY(NAME, OP) \
static struct wrapperbase tab_##NAME[] = { \
	{"__" #NAME "__", \
	 (wrapperfunc)wrap_ternaryfunc, \
	 "x.__" #NAME "__(y, z) <==> " #OP}, \
	{"__r" #NAME "__", \
	 (wrapperfunc)wrap_ternaryfunc, \
	 "y.__r" #NAME "__(x, z) <==> " #OP}, \
	{0} \
}

TERNARY(pow, "(x**y) % z");

#undef UNARY
#define UNARY(NAME, OP) \
static struct wrapperbase tab_##NAME[] = { \
	{"__" #NAME "__", \
	 (wrapperfunc)wrap_unaryfunc, \
	 "x.__" #NAME "__() <==> " #OP}, \
	{0} \
}

static PyObject *
wrap_unaryfunc(PyObject *self, PyObject *args, void *wrapped)
{
	unaryfunc func = (unaryfunc)wrapped;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return (*func)(self);
}

UNARY(neg, "-x");
UNARY(pos, "+x");
UNARY(abs, "abs(x)");
UNARY(nonzero, "x != 0");
UNARY(invert, "~x");
UNARY(int, "int(x)");
UNARY(long, "long(x)");
UNARY(float, "float(x)");
UNARY(oct, "oct(x)");
UNARY(hex, "hex(x)");

#undef IBINARY
#define IBINARY(NAME, OP) \
static struct wrapperbase tab_##NAME[] = { \
	{"__" #NAME "__", \
	 (wrapperfunc)wrap_binaryfunc, \
	 "x.__" #NAME "__(y) <==> " #OP}, \
	{0} \
}

IBINARY(iadd, "x+=y");
IBINARY(isub, "x-=y");
IBINARY(imul, "x*=y");
IBINARY(idiv, "x/=y");
IBINARY(imod, "x%=y");
IBINARY(ilshift, "x<<=y");
IBINARY(irshift, "x>>=y");
IBINARY(iand, "x&=y");
IBINARY(ixor, "x^=y");
IBINARY(ior, "x|=y");
IBINARY(ifloordiv, "x//=y");
IBINARY(itruediv, "x/=y # true division");

#undef ITERNARY
#define ITERNARY(NAME, OP) \
static struct wrapperbase tab_##NAME[] = { \
	{"__" #NAME "__", \
	 (wrapperfunc)wrap_ternaryfunc, \
	 "x.__" #NAME "__(y) <==> " #OP}, \
	{0} \
}

ITERNARY(ipow, "x = (x**y) % z");

static struct wrapperbase tab_getitem[] = {
	{"__getitem__", (wrapperfunc)wrap_binaryfunc,
	 "x.__getitem__(y) <==> x[y]"},
	{0}
};

static PyObject *
wrap_intargfunc(PyObject *self, PyObject *args, void *wrapped)
{
	intargfunc func = (intargfunc)wrapped;
	int i;

	if (!PyArg_ParseTuple(args, "i", &i))
		return NULL;
	return (*func)(self, i);
}

static struct wrapperbase tab_mul_int[] = {
	{"__mul__", (wrapperfunc)wrap_intargfunc, "x.__mul__(n) <==> x*n"},
	{"__rmul__", (wrapperfunc)wrap_intargfunc, "x.__rmul__(n) <==> n*x"},
	{0}
};

static struct wrapperbase tab_concat[] = {
	{"__add__", (wrapperfunc)wrap_binaryfunc, "x.__add__(y) <==> x+y"},
	{0}
};

static struct wrapperbase tab_imul_int[] = {
	{"__imul__", (wrapperfunc)wrap_intargfunc, "x.__imul__(n) <==> x*=n"},
	{0}
};

static int
getindex(PyObject *self, PyObject *arg)
{
	int i;

	i = PyInt_AsLong(arg);
	if (i == -1 && PyErr_Occurred())
		return -1;
	if (i < 0) {
		PySequenceMethods *sq = self->ob_type->tp_as_sequence;
		if (sq && sq->sq_length) {
			int n = (*sq->sq_length)(self);
			if (n < 0)
				return -1;
			i += n;
		}
	}
	return i;
}

static PyObject *
wrap_sq_item(PyObject *self, PyObject *args, void *wrapped)
{
	intargfunc func = (intargfunc)wrapped;
	PyObject *arg;
	int i;

	if (!PyArg_ParseTuple(args, "O", &arg))
		return NULL;
	i = getindex(self, arg);
	if (i == -1 && PyErr_Occurred())
		return NULL;
	return (*func)(self, i);
}

static struct wrapperbase tab_getitem_int[] = {
	{"__getitem__", (wrapperfunc)wrap_sq_item,
	 "x.__getitem__(i) <==> x[i]"},
	{0}
};

static PyObject *
wrap_intintargfunc(PyObject *self, PyObject *args, void *wrapped)
{
	intintargfunc func = (intintargfunc)wrapped;
	int i, j;

	if (!PyArg_ParseTuple(args, "ii", &i, &j))
		return NULL;
	return (*func)(self, i, j);
}

static struct wrapperbase tab_getslice[] = {
	{"__getslice__", (wrapperfunc)wrap_intintargfunc,
	 "x.__getslice__(i, j) <==> x[i:j]"},
	{0}
};

static PyObject *
wrap_sq_setitem(PyObject *self, PyObject *args, void *wrapped)
{
	intobjargproc func = (intobjargproc)wrapped;
	int i, res;
	PyObject *arg, *value;

	if (!PyArg_ParseTuple(args, "OO", &arg, &value))
		return NULL;
	i = getindex(self, arg);
	if (i == -1 && PyErr_Occurred())
		return NULL;
	res = (*func)(self, i, value);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
wrap_sq_delitem(PyObject *self, PyObject *args, void *wrapped)
{
	intobjargproc func = (intobjargproc)wrapped;
	int i, res;
	PyObject *arg;

	if (!PyArg_ParseTuple(args, "O", &arg))
		return NULL;
	i = getindex(self, arg);
	if (i == -1 && PyErr_Occurred())
		return NULL;
	res = (*func)(self, i, NULL);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static struct wrapperbase tab_setitem_int[] = {
	{"__setitem__", (wrapperfunc)wrap_sq_setitem,
	 "x.__setitem__(i, y) <==> x[i]=y"},
	{"__delitem__", (wrapperfunc)wrap_sq_delitem,
	 "x.__delitem__(y) <==> del x[y]"},
	{0}
};

static PyObject *
wrap_intintobjargproc(PyObject *self, PyObject *args, void *wrapped)
{
	intintobjargproc func = (intintobjargproc)wrapped;
	int i, j, res;
	PyObject *value;

	if (!PyArg_ParseTuple(args, "iiO", &i, &j, &value))
		return NULL;
	res = (*func)(self, i, j, value);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static struct wrapperbase tab_setslice[] = {
	{"__setslice__", (wrapperfunc)wrap_intintobjargproc,
	 "x.__setslice__(i, j, y) <==> x[i:j]=y"},
	{0}
};

/* XXX objobjproc is a misnomer; should be objargpred */
static PyObject *
wrap_objobjproc(PyObject *self, PyObject *args, void *wrapped)
{
	objobjproc func = (objobjproc)wrapped;
	int res;
	PyObject *value;

	if (!PyArg_ParseTuple(args, "O", &value))
		return NULL;
	res = (*func)(self, value);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	return PyInt_FromLong((long)res);
}

static struct wrapperbase tab_contains[] = {
	{"__contains__", (wrapperfunc)wrap_objobjproc,
	 "x.__contains__(y) <==> y in x"},
	{0}
};

static PyObject *
wrap_objobjargproc(PyObject *self, PyObject *args, void *wrapped)
{
	objobjargproc func = (objobjargproc)wrapped;
	int res;
	PyObject *key, *value;

	if (!PyArg_ParseTuple(args, "OO", &key, &value))
		return NULL;
	res = (*func)(self, key, value);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
wrap_delitem(PyObject *self, PyObject *args, void *wrapped)
{
	objobjargproc func = (objobjargproc)wrapped;
	int res;
	PyObject *key;

	if (!PyArg_ParseTuple(args, "O", &key))
		return NULL;
	res = (*func)(self, key, NULL);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static struct wrapperbase tab_setitem[] = {
	{"__setitem__", (wrapperfunc)wrap_objobjargproc,
	 "x.__setitem__(y, z) <==> x[y]=z"},
	{"__delitem__", (wrapperfunc)wrap_delitem,
	 "x.__delitem__(y) <==> del x[y]"},
	{0}
};

static PyObject *
wrap_cmpfunc(PyObject *self, PyObject *args, void *wrapped)
{
	cmpfunc func = (cmpfunc)wrapped;
	int res;
	PyObject *other;

	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;
	if (other->ob_type->tp_compare != func &&
	    !PyType_IsSubtype(other->ob_type, self->ob_type)) {
		PyErr_Format(
			PyExc_TypeError,
			"%s.__cmp__(x,y) requires y to be a '%s', not a '%s'",
			self->ob_type->tp_name,
			self->ob_type->tp_name,
			other->ob_type->tp_name);
		return NULL;
	}
	res = (*func)(self, other);
	if (PyErr_Occurred())
		return NULL;
	return PyInt_FromLong((long)res);
}

static struct wrapperbase tab_cmp[] = {
	{"__cmp__", (wrapperfunc)wrap_cmpfunc,
	 "x.__cmp__(y) <==> cmp(x,y)"},
	{0}
};

static struct wrapperbase tab_repr[] = {
	{"__repr__", (wrapperfunc)wrap_unaryfunc,
	 "x.__repr__() <==> repr(x)"},
	{0}
};

static struct wrapperbase tab_getattr[] = {
	{"__getattribute__", (wrapperfunc)wrap_binaryfunc,
	 "x.__getattribute__('name') <==> x.name"},
	{0}
};

static PyObject *
wrap_setattr(PyObject *self, PyObject *args, void *wrapped)
{
	setattrofunc func = (setattrofunc)wrapped;
	int res;
	PyObject *name, *value;

	if (!PyArg_ParseTuple(args, "OO", &name, &value))
		return NULL;
	res = (*func)(self, name, value);
	if (res < 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
wrap_delattr(PyObject *self, PyObject *args, void *wrapped)
{
	setattrofunc func = (setattrofunc)wrapped;
	int res;
	PyObject *name;

	if (!PyArg_ParseTuple(args, "O", &name))
		return NULL;
	res = (*func)(self, name, NULL);
	if (res < 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static struct wrapperbase tab_setattr[] = {
	{"__setattr__", (wrapperfunc)wrap_setattr,
	 "x.__setattr__('name', value) <==> x.name = value"},
	{"__delattr__", (wrapperfunc)wrap_delattr,
	 "x.__delattr__('name') <==> del x.name"},
	{0}
};

static PyObject *
wrap_hashfunc(PyObject *self, PyObject *args, void *wrapped)
{
	hashfunc func = (hashfunc)wrapped;
	long res;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	res = (*func)(self);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	return PyInt_FromLong(res);
}

static struct wrapperbase tab_hash[] = {
	{"__hash__", (wrapperfunc)wrap_hashfunc,
	 "x.__hash__() <==> hash(x)"},
	{0}
};

static PyObject *
wrap_call(PyObject *self, PyObject *args, void *wrapped)
{
	ternaryfunc func = (ternaryfunc)wrapped;

	/* XXX What about keyword arguments? */
	return (*func)(self, args, NULL);
}

static struct wrapperbase tab_call[] = {
	{"__call__", (wrapperfunc)wrap_call,
	 "x.__call__(...) <==> x(...)"},
	{0}
};

static struct wrapperbase tab_str[] = {
	{"__str__", (wrapperfunc)wrap_unaryfunc,
	 "x.__str__() <==> str(x)"},
	{0}
};

static PyObject *
wrap_richcmpfunc(PyObject *self, PyObject *args, void *wrapped, int op)
{
	richcmpfunc func = (richcmpfunc)wrapped;
	PyObject *other;

	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;
	return (*func)(self, other, op);
}

#undef RICHCMP_WRAPPER
#define RICHCMP_WRAPPER(NAME, OP) \
static PyObject * \
richcmp_##NAME(PyObject *self, PyObject *args, void *wrapped) \
{ \
	return wrap_richcmpfunc(self, args, wrapped, OP); \
}

RICHCMP_WRAPPER(lt, Py_LT)
RICHCMP_WRAPPER(le, Py_LE)
RICHCMP_WRAPPER(eq, Py_EQ)
RICHCMP_WRAPPER(ne, Py_NE)
RICHCMP_WRAPPER(gt, Py_GT)
RICHCMP_WRAPPER(ge, Py_GE)

#undef RICHCMP_ENTRY
#define RICHCMP_ENTRY(NAME, EXPR) \
	{"__" #NAME "__", (wrapperfunc)richcmp_##NAME, \
	 "x.__" #NAME "__(y) <==> " EXPR}

static struct wrapperbase tab_richcmp[] = {
	RICHCMP_ENTRY(lt, "x<y"),
	RICHCMP_ENTRY(le, "x<=y"),
	RICHCMP_ENTRY(eq, "x==y"),
	RICHCMP_ENTRY(ne, "x!=y"),
	RICHCMP_ENTRY(gt, "x>y"),
	RICHCMP_ENTRY(ge, "x>=y"),
	{0}
};

static struct wrapperbase tab_iter[] = {
	{"__iter__", (wrapperfunc)wrap_unaryfunc, "x.__iter__() <==> iter(x)"},
	{0}
};

static PyObject *
wrap_next(PyObject *self, PyObject *args, void *wrapped)
{
	unaryfunc func = (unaryfunc)wrapped;
	PyObject *res;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	res = (*func)(self);
	if (res == NULL && !PyErr_Occurred())
		PyErr_SetNone(PyExc_StopIteration);
	return res;
}

static struct wrapperbase tab_next[] = {
	{"next", (wrapperfunc)wrap_next,
		"x.next() -> the next value, or raise StopIteration"},
	{0}
};

static PyObject *
wrap_descr_get(PyObject *self, PyObject *args, void *wrapped)
{
	descrgetfunc func = (descrgetfunc)wrapped;
	PyObject *obj;
	PyObject *type = NULL;

	if (!PyArg_ParseTuple(args, "O|O", &obj, &type))
		return NULL;
	return (*func)(self, obj, type);
}

static struct wrapperbase tab_descr_get[] = {
	{"__get__", (wrapperfunc)wrap_descr_get,
	 "descr.__get__(obj, type) -> value"},
	{0}
};

static PyObject *
wrap_descrsetfunc(PyObject *self, PyObject *args, void *wrapped)
{
	descrsetfunc func = (descrsetfunc)wrapped;
	PyObject *obj, *value;
	int ret;

	if (!PyArg_ParseTuple(args, "OO", &obj, &value))
		return NULL;
	ret = (*func)(self, obj, value);
	if (ret < 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static struct wrapperbase tab_descr_set[] = {
	{"__set__", (wrapperfunc)wrap_descrsetfunc,
	 "descr.__set__(obj, value)"},
	{0}
};

static PyObject *
wrap_init(PyObject *self, PyObject *args, void *wrapped)
{
	initproc func = (initproc)wrapped;

	/* XXX What about keyword arguments? */
	if (func(self, args, NULL) < 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static struct wrapperbase tab_init[] = {
	{"__init__", (wrapperfunc)wrap_init,
	 "x.__init__(...) initializes x; "
	 "see x.__type__.__doc__ for signature"},
	{0}
};

static PyObject *
tp_new_wrapper(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyTypeObject *type, *subtype, *staticbase;
	PyObject *arg0, *res;

	if (self == NULL || !PyType_Check(self))
		Py_FatalError("__new__() called with non-type 'self'");
	type = (PyTypeObject *)self;
	if (!PyTuple_Check(args) || PyTuple_GET_SIZE(args) < 1) {
		PyErr_Format(PyExc_TypeError,
			     "%s.__new__(): not enough arguments",
			     type->tp_name);
		return NULL;
	}
	arg0 = PyTuple_GET_ITEM(args, 0);
	if (!PyType_Check(arg0)) {
		PyErr_Format(PyExc_TypeError,
			     "%s.__new__(X): X is not a type object (%s)",
			     type->tp_name,
			     arg0->ob_type->tp_name);
		return NULL;
	}
	subtype = (PyTypeObject *)arg0;
	if (!PyType_IsSubtype(subtype, type)) {
		PyErr_Format(PyExc_TypeError,
			     "%s.__new__(%s): %s is not a subtype of %s",
			     type->tp_name,
			     subtype->tp_name,
			     subtype->tp_name,
			     type->tp_name);
		return NULL;
	}

	/* Check that the use doesn't do something silly and unsafe like
	   object.__new__(dictionary).  To do this, we check that the
	   most derived base that's not a heap type is this type. */
	staticbase = subtype;
	while (staticbase && (staticbase->tp_flags & Py_TPFLAGS_HEAPTYPE))
		staticbase = staticbase->tp_base;
	if (staticbase->tp_new != type->tp_new) {
		PyErr_Format(PyExc_TypeError,
			     "%s.__new__(%s) is not safe, use %s.__new__()",
			     type->tp_name,
			     subtype->tp_name,
			     staticbase == NULL ? "?" : staticbase->tp_name);
		return NULL;
	}

	args = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
	if (args == NULL)
		return NULL;
	res = type->tp_new(subtype, args, kwds);
	Py_DECREF(args);
	return res;
}

static struct PyMethodDef tp_new_methoddef[] = {
	{"__new__", (PyCFunction)tp_new_wrapper, METH_KEYWORDS,
	 "T.__new__(S, ...) -> a new object with type S, a subtype of T"},
	{0}
};

static int
add_tp_new_wrapper(PyTypeObject *type)
{
	PyObject *func;

	if (PyDict_GetItemString(type->tp_defined, "__new__") != NULL)
		return 0;
	func = PyCFunction_New(tp_new_methoddef, (PyObject *)type);
	if (func == NULL)
		return -1;
	return PyDict_SetItemString(type->tp_defined, "__new__", func);
}

static int
add_wrappers(PyTypeObject *type, struct wrapperbase *wraps, void *wrapped)
{
	PyObject *dict = type->tp_defined;

	for (; wraps->name != NULL; wraps++) {
		PyObject *descr;
		if (PyDict_GetItemString(dict, wraps->name))
			continue;
		descr = PyDescr_NewWrapper(type, wraps, wrapped);
		if (descr == NULL)
			return -1;
		if (PyDict_SetItemString(dict, wraps->name, descr) < 0)
			return -1;
		Py_DECREF(descr);
	}
	return 0;
}

/* This function is called by PyType_Ready() to populate the type's
   dictionary with method descriptors for function slots.  For each
   function slot (like tp_repr) that's defined in the type, one or
   more corresponding descriptors are added in the type's tp_defined
   dictionary under the appropriate name (like __repr__).  Some
   function slots cause more than one descriptor to be added (for
   example, the nb_add slot adds both __add__ and __radd__
   descriptors) and some function slots compete for the same
   descriptor (for example both sq_item and mp_subscript generate a
   __getitem__ descriptor).  This only adds new descriptors and
   doesn't overwrite entries in tp_defined that were previously
   defined.  The descriptors contain a reference to the C function
   they must call, so that it's safe if they are copied into a
   subtype's __dict__ and the subtype has a different C function in
   its slot -- calling the method defined by the descriptor will call
   the C function that was used to create it, rather than the C
   function present in the slot when it is called.  (This is important
   because a subtype may have a C function in the slot that calls the
   method from the dictionary, and we want to avoid infinite recursion
   here.) */

static int
add_operators(PyTypeObject *type)
{
	PySequenceMethods *sq;
	PyMappingMethods *mp;
	PyNumberMethods *nb;

#undef ADD
#define ADD(SLOT, TABLE) \
		if (SLOT) { \
			if (add_wrappers(type, TABLE, (void *)(SLOT)) < 0) \
				return -1; \
		}

	if ((sq = type->tp_as_sequence) != NULL) {
		ADD(sq->sq_length, tab_len);
		ADD(sq->sq_concat, tab_concat);
		ADD(sq->sq_repeat, tab_mul_int);
		ADD(sq->sq_item, tab_getitem_int);
		ADD(sq->sq_slice, tab_getslice);
		ADD(sq->sq_ass_item, tab_setitem_int);
		ADD(sq->sq_ass_slice, tab_setslice);
		ADD(sq->sq_contains, tab_contains);
		ADD(sq->sq_inplace_concat, tab_iadd);
		ADD(sq->sq_inplace_repeat, tab_imul_int);
	}

	if ((mp = type->tp_as_mapping) != NULL) {
		if (sq->sq_length == NULL)
			ADD(mp->mp_length, tab_len);
		ADD(mp->mp_subscript, tab_getitem);
		ADD(mp->mp_ass_subscript, tab_setitem);
	}

	/* We don't support "old-style numbers" because their binary
	   operators require that both arguments have the same type;
	   the wrappers here only work for new-style numbers. */
	if ((type->tp_flags & Py_TPFLAGS_CHECKTYPES) &&
	    (nb = type->tp_as_number) != NULL) {
		ADD(nb->nb_add, tab_add);
		ADD(nb->nb_subtract, tab_sub);
		ADD(nb->nb_multiply, tab_mul);
		ADD(nb->nb_divide, tab_div);
		ADD(nb->nb_remainder, tab_mod);
		ADD(nb->nb_divmod, tab_divmod);
		ADD(nb->nb_power, tab_pow);
		ADD(nb->nb_negative, tab_neg);
		ADD(nb->nb_positive, tab_pos);
		ADD(nb->nb_absolute, tab_abs);
		ADD(nb->nb_nonzero, tab_nonzero);
		ADD(nb->nb_invert, tab_invert);
		ADD(nb->nb_lshift, tab_lshift);
		ADD(nb->nb_rshift, tab_rshift);
		ADD(nb->nb_and, tab_and);
		ADD(nb->nb_xor, tab_xor);
		ADD(nb->nb_or, tab_or);
		ADD(nb->nb_coerce, tab_coerce);
		ADD(nb->nb_int, tab_int);
		ADD(nb->nb_long, tab_long);
		ADD(nb->nb_float, tab_float);
		ADD(nb->nb_oct, tab_oct);
		ADD(nb->nb_hex, tab_hex);
		ADD(nb->nb_inplace_add, tab_iadd);
		ADD(nb->nb_inplace_subtract, tab_isub);
		ADD(nb->nb_inplace_multiply, tab_imul);
		ADD(nb->nb_inplace_divide, tab_idiv);
		ADD(nb->nb_inplace_remainder, tab_imod);
		ADD(nb->nb_inplace_power, tab_ipow);
		ADD(nb->nb_inplace_lshift, tab_ilshift);
		ADD(nb->nb_inplace_rshift, tab_irshift);
		ADD(nb->nb_inplace_and, tab_iand);
		ADD(nb->nb_inplace_xor, tab_ixor);
		ADD(nb->nb_inplace_or, tab_ior);
		if (type->tp_flags & Py_TPFLAGS_CHECKTYPES) {
			ADD(nb->nb_floor_divide, tab_floordiv);
			ADD(nb->nb_true_divide, tab_truediv);
			ADD(nb->nb_inplace_floor_divide, tab_ifloordiv);
			ADD(nb->nb_inplace_true_divide, tab_itruediv);
		}
	}

	ADD(type->tp_getattro, tab_getattr);
	ADD(type->tp_setattro, tab_setattr);
	ADD(type->tp_compare, tab_cmp);
	ADD(type->tp_repr, tab_repr);
	ADD(type->tp_hash, tab_hash);
	ADD(type->tp_call, tab_call);
	ADD(type->tp_str, tab_str);
	ADD(type->tp_richcompare, tab_richcmp);
	ADD(type->tp_iter, tab_iter);
	ADD(type->tp_iternext, tab_next);
	ADD(type->tp_descr_get, tab_descr_get);
	ADD(type->tp_descr_set, tab_descr_set);
	ADD(type->tp_init, tab_init);

	if (type->tp_new != NULL) {
		if (add_tp_new_wrapper(type) < 0)
			return -1;
	}

	return 0;
}

/* Slot wrappers that call the corresponding __foo__ slot.  See comments
   below at override_slots() for more explanation. */

#define SLOT0(FUNCNAME, OPSTR) \
static PyObject * \
FUNCNAME(PyObject *self) \
{ \
	static PyObject *cache_str; \
	return call_method(self, OPSTR, &cache_str, "()"); \
}

#define SLOT1(FUNCNAME, OPSTR, ARG1TYPE, ARGCODES) \
static PyObject * \
FUNCNAME(PyObject *self, ARG1TYPE arg1) \
{ \
	static PyObject *cache_str; \
	return call_method(self, OPSTR, &cache_str, "(" ARGCODES ")", arg1); \
}


#define SLOT1BINFULL(FUNCNAME, TESTFUNC, SLOTNAME, OPSTR, ROPSTR) \
static PyObject * \
FUNCNAME(PyObject *self, PyObject *other) \
{ \
	static PyObject *cache_str, *rcache_str; \
	if (self->ob_type->tp_as_number != NULL && \
	    self->ob_type->tp_as_number->SLOTNAME == TESTFUNC) { \
		PyObject *r; \
		r = call_maybe( \
			self, OPSTR, &cache_str, "(O)", other); \
		if (r != Py_NotImplemented || \
		    other->ob_type == self->ob_type) \
			return r; \
		Py_DECREF(r); \
	} \
	if (other->ob_type->tp_as_number != NULL && \
	    other->ob_type->tp_as_number->SLOTNAME == TESTFUNC) { \
		return call_maybe( \
			other, ROPSTR, &rcache_str, "(O)", self); \
	} \
	Py_INCREF(Py_NotImplemented); \
	return Py_NotImplemented; \
}

#define SLOT1BIN(FUNCNAME, SLOTNAME, OPSTR, ROPSTR) \
	SLOT1BINFULL(FUNCNAME, FUNCNAME, SLOTNAME, OPSTR, ROPSTR)

#define SLOT2(FUNCNAME, OPSTR, ARG1TYPE, ARG2TYPE, ARGCODES) \
static PyObject * \
FUNCNAME(PyObject *self, ARG1TYPE arg1, ARG2TYPE arg2) \
{ \
	static PyObject *cache_str; \
	return call_method(self, OPSTR, &cache_str, \
			   "(" ARGCODES ")", arg1, arg2); \
}

static int
slot_sq_length(PyObject *self)
{
	static PyObject *len_str;
	PyObject *res = call_method(self, "__len__", &len_str, "()");

	if (res == NULL)
		return -1;
	return (int)PyInt_AsLong(res);
}

SLOT1(slot_sq_concat, "__add__", PyObject *, "O")
SLOT1(slot_sq_repeat, "__mul__", int, "i")
SLOT1(slot_sq_item, "__getitem__", int, "i")
SLOT2(slot_sq_slice, "__getslice__", int, int, "ii")

static int
slot_sq_ass_item(PyObject *self, int index, PyObject *value)
{
	PyObject *res;
	static PyObject *delitem_str, *setitem_str;

	if (value == NULL)
		res = call_method(self, "__delitem__", &delitem_str,
				  "(i)", index);
	else
		res = call_method(self, "__setitem__", &setitem_str,
				  "(iO)", index, value);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static int
slot_sq_ass_slice(PyObject *self, int i, int j, PyObject *value)
{
	PyObject *res;
	static PyObject *delslice_str, *setslice_str;

	if (value == NULL)
		res = call_method(self, "__delslice__", &delslice_str,
				  "(ii)", i, j);
	else
		res = call_method(self, "__setslice__", &setslice_str,
				  "(iiO)", i, j, value);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static int
slot_sq_contains(PyObject *self, PyObject *value)
{
	PyObject *func, *res, *args;
	static PyObject *contains_str;

	func = lookup_method(self, "__contains__", &contains_str);

	if (func != NULL) {
		args = Py_BuildValue("(O)", value);
		if (args == NULL)
			res = NULL;
		else {
			res = PyObject_Call(func, args, NULL);
			Py_DECREF(args);
		}
		Py_DECREF(func);
		if (res == NULL)
			return -1;
		return PyObject_IsTrue(res);
	}
	else {
		PyErr_Clear();
		return _PySequence_IterSearch(self, value,
					      PY_ITERSEARCH_CONTAINS);
	}
}

SLOT1(slot_sq_inplace_concat, "__iadd__", PyObject *, "O")
SLOT1(slot_sq_inplace_repeat, "__imul__", int, "i")

#define slot_mp_length slot_sq_length

SLOT1(slot_mp_subscript, "__getitem__", PyObject *, "O")

static int
slot_mp_ass_subscript(PyObject *self, PyObject *key, PyObject *value)
{
	PyObject *res;
	static PyObject *delitem_str, *setitem_str;

	if (value == NULL)
		res = call_method(self, "__delitem__", &delitem_str,
				  "(O)", key);
	else
		res = call_method(self, "__setitem__", &setitem_str,
				 "(OO)", key, value);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

SLOT1BIN(slot_nb_add, nb_add, "__add__", "__radd__")
SLOT1BIN(slot_nb_subtract, nb_subtract, "__sub__", "__rsub__")
SLOT1BIN(slot_nb_multiply, nb_multiply, "__mul__", "__rmul__")
SLOT1BIN(slot_nb_divide, nb_divide, "__div__", "__rdiv__")
SLOT1BIN(slot_nb_remainder, nb_remainder, "__mod__", "__rmod__")
SLOT1BIN(slot_nb_divmod, nb_divmod, "__divmod__", "__rdivmod__")

staticforward PyObject *slot_nb_power(PyObject *, PyObject *, PyObject *);

SLOT1BINFULL(slot_nb_power_binary, slot_nb_power,
	     nb_power, "__pow__", "__rpow__")

static PyObject *
slot_nb_power(PyObject *self, PyObject *other, PyObject *modulus)
{
	static PyObject *pow_str;

	if (modulus == Py_None)
		return slot_nb_power_binary(self, other);
	/* Three-arg power doesn't use __rpow__ */
	return call_method(self, "__pow__", &pow_str,
			   "(OO)", other, modulus);
}

SLOT0(slot_nb_negative, "__neg__")
SLOT0(slot_nb_positive, "__pos__")
SLOT0(slot_nb_absolute, "__abs__")

static int
slot_nb_nonzero(PyObject *self)
{
	PyObject *func, *res;
	static PyObject *nonzero_str, *len_str;

	func = lookup_method(self, "__nonzero__", &nonzero_str);
	if (func == NULL) {
		PyErr_Clear();
		func = lookup_method(self, "__len__", &len_str);
	}

	if (func != NULL) {
		res = PyObject_CallObject(func, NULL);
		Py_DECREF(func);
		if (res == NULL)
			return -1;
		return PyObject_IsTrue(res);
	}
	else {
		PyErr_Clear();
		return 1;
	}
}

SLOT0(slot_nb_invert, "__invert__")
SLOT1BIN(slot_nb_lshift, nb_lshift, "__lshift__", "__rlshift__")
SLOT1BIN(slot_nb_rshift, nb_rshift, "__rshift__", "__rrshift__")
SLOT1BIN(slot_nb_and, nb_and, "__and__", "__rand__")
SLOT1BIN(slot_nb_xor, nb_xor, "__xor__", "__rxor__")
SLOT1BIN(slot_nb_or, nb_or, "__or__", "__ror__")

static int
slot_nb_coerce(PyObject **a, PyObject **b)
{
	static PyObject *coerce_str;
	PyObject *self = *a, *other = *b;

	if (self->ob_type->tp_as_number != NULL &&
	    self->ob_type->tp_as_number->nb_coerce == slot_nb_coerce) {
		PyObject *r;
		r = call_maybe(
			self, "__coerce__", &coerce_str, "(O)", other);
		if (r == NULL)
			return -1;
		if (r == Py_NotImplemented) {
			Py_DECREF(r);
			return 1;
		}
		if (!PyTuple_Check(r) || PyTuple_GET_SIZE(r) != 2) {
			PyErr_SetString(PyExc_TypeError,
					"__coerce__ didn't return a 2-tuple");
			Py_DECREF(r);
			return -1;
		}
		*a = PyTuple_GET_ITEM(r, 0);
		Py_INCREF(*a);
		*b = PyTuple_GET_ITEM(r, 1);
		Py_INCREF(*b);
		Py_DECREF(r);
		return 0;
	}
	if (other->ob_type->tp_as_number != NULL &&
	    other->ob_type->tp_as_number->nb_coerce == slot_nb_coerce) {
		PyObject *r;
		r = call_maybe(
			other, "__coerce__", &coerce_str, "(O)", self);
		if (r == NULL)
			return -1;
		if (r == Py_NotImplemented) {
			Py_DECREF(r);
			return 1;
		}
		if (!PyTuple_Check(r) || PyTuple_GET_SIZE(r) != 2) {
			PyErr_SetString(PyExc_TypeError,
					"__coerce__ didn't return a 2-tuple");
			Py_DECREF(r);
			return -1;
		}
		*a = PyTuple_GET_ITEM(r, 1);
		Py_INCREF(*a);
		*b = PyTuple_GET_ITEM(r, 0);
		Py_INCREF(*b);
		Py_DECREF(r);
		return 0;
	}
	return 1;
}

SLOT0(slot_nb_int, "__int__")
SLOT0(slot_nb_long, "__long__")
SLOT0(slot_nb_float, "__float__")
SLOT0(slot_nb_oct, "__oct__")
SLOT0(slot_nb_hex, "__hex__")
SLOT1(slot_nb_inplace_add, "__iadd__", PyObject *, "O")
SLOT1(slot_nb_inplace_subtract, "__isub__", PyObject *, "O")
SLOT1(slot_nb_inplace_multiply, "__imul__", PyObject *, "O")
SLOT1(slot_nb_inplace_divide, "__idiv__", PyObject *, "O")
SLOT1(slot_nb_inplace_remainder, "__imod__", PyObject *, "O")
SLOT2(slot_nb_inplace_power, "__ipow__", PyObject *, PyObject *, "OO")
SLOT1(slot_nb_inplace_lshift, "__ilshift__", PyObject *, "O")
SLOT1(slot_nb_inplace_rshift, "__irshift__", PyObject *, "O")
SLOT1(slot_nb_inplace_and, "__iand__", PyObject *, "O")
SLOT1(slot_nb_inplace_xor, "__ixor__", PyObject *, "O")
SLOT1(slot_nb_inplace_or, "__ior__", PyObject *, "O")
SLOT1BIN(slot_nb_floor_divide, nb_floor_divide,
	 "__floordiv__", "__rfloordiv__")
SLOT1BIN(slot_nb_true_divide, nb_true_divide, "__truediv__", "__rtruediv__")
SLOT1(slot_nb_inplace_floor_divide, "__ifloordiv__", PyObject *, "O")
SLOT1(slot_nb_inplace_true_divide, "__itruediv__", PyObject *, "O")

static int
half_compare(PyObject *self, PyObject *other)
{
	PyObject *func, *args, *res;
	static PyObject *cmp_str;
	int c;

	func = lookup_method(self, "__cmp__", &cmp_str);
	if (func == NULL) {
		PyErr_Clear();
	}
	else {
		args = Py_BuildValue("(O)", other);
		if (args == NULL)
			res = NULL;
		else {
			res = PyObject_Call(func, args, NULL);
			Py_DECREF(args);
		}
		if (res != Py_NotImplemented) {
			if (res == NULL)
				return -2;
			c = PyInt_AsLong(res);
			Py_DECREF(res);
			if (c == -1 && PyErr_Occurred())
				return -2;
			return (c < 0) ? -1 : (c > 0) ? 1 : 0;
		}
		Py_DECREF(res);
	}
	return 2;
}

/* This slot is published for the benefit of try_3way_compare in object.c */
int
_PyObject_SlotCompare(PyObject *self, PyObject *other)
{
	int c;

	if (self->ob_type->tp_compare == _PyObject_SlotCompare) {
		c = half_compare(self, other);
		if (c <= 1)
			return c;
	}
	if (other->ob_type->tp_compare == _PyObject_SlotCompare) {
		c = half_compare(other, self);
		if (c < -1)
			return -2;
		if (c <= 1)
			return -c;
	}
	return (void *)self < (void *)other ? -1 :
		(void *)self > (void *)other ? 1 : 0;
}

static PyObject *
slot_tp_repr(PyObject *self)
{
	PyObject *func, *res;
	static PyObject *repr_str;

	func = lookup_method(self, "__repr__", &repr_str);
	if (func != NULL) {
		res = PyEval_CallObject(func, NULL);
		Py_DECREF(func);
		return res;
	}
	PyErr_Clear();
	return PyString_FromFormat("<%s object at %p>",
				   self->ob_type->tp_name, self);
}

static PyObject *
slot_tp_str(PyObject *self)
{
	PyObject *func, *res;
	static PyObject *str_str;

	func = lookup_method(self, "__str__", &str_str);
	if (func != NULL) {
		res = PyEval_CallObject(func, NULL);
		Py_DECREF(func);
		return res;
	}
	else {
		PyErr_Clear();
		return slot_tp_repr(self);
	}
}

static long
slot_tp_hash(PyObject *self)
{
	PyObject *func, *res;
	static PyObject *hash_str, *eq_str, *cmp_str;

	long h;

	func = lookup_method(self, "__hash__", &hash_str);

	if (func != NULL) {
		res = PyEval_CallObject(func, NULL);
		Py_DECREF(func);
		if (res == NULL)
			return -1;
		h = PyInt_AsLong(res);
	}
	else {
		PyErr_Clear();
		func = lookup_method(self, "__eq__", &eq_str);
		if (func == NULL) {
			PyErr_Clear();
			func = lookup_method(self, "__cmp__", &cmp_str);
		}
		if (func != NULL) {
			Py_DECREF(func);
			PyErr_SetString(PyExc_TypeError, "unhashable type");
			return -1;
		}
		PyErr_Clear();
		h = _Py_HashPointer((void *)self);
	}
	if (h == -1 && !PyErr_Occurred())
		h = -2;
	return h;
}

static PyObject *
slot_tp_call(PyObject *self, PyObject *args, PyObject *kwds)
{
	static PyObject *call_str;
	PyObject *meth = lookup_method(self, "__call__", &call_str);
	PyObject *res;

	if (meth == NULL)
		return NULL;
	res = PyObject_Call(meth, args, kwds);
	Py_DECREF(meth);
	return res;
}

static PyObject *
slot_tp_getattro(PyObject *self, PyObject *name)
{
	PyTypeObject *tp = self->ob_type;
	PyObject *getattr;
	static PyObject *getattr_str = NULL;

	if (getattr_str == NULL) {
		getattr_str = PyString_InternFromString("__getattribute__");
		if (getattr_str == NULL)
			return NULL;
	}
	getattr = _PyType_Lookup(tp, getattr_str);
	if (getattr == NULL) {
		/* Avoid further slowdowns */
		if (tp->tp_getattro == slot_tp_getattro)
			tp->tp_getattro = PyObject_GenericGetAttr;
		return PyObject_GenericGetAttr(self, name);
	}
	return PyObject_CallFunction(getattr, "OO", self, name);
}

static PyObject *
slot_tp_getattr_hook(PyObject *self, PyObject *name)
{
	PyTypeObject *tp = self->ob_type;
	PyObject *getattr, *getattribute, *res;
	static PyObject *getattribute_str = NULL;
	static PyObject *getattr_str = NULL;

	if (getattr_str == NULL) {
		getattr_str = PyString_InternFromString("__getattr__");
		if (getattr_str == NULL)
			return NULL;
	}
	if (getattribute_str == NULL) {
		getattribute_str =
			PyString_InternFromString("__getattribute__");
		if (getattribute_str == NULL)
			return NULL;
	}
	getattr = _PyType_Lookup(tp, getattr_str);
	getattribute = _PyType_Lookup(tp, getattribute_str);
	if (getattr == NULL && getattribute == NULL) {
		/* Avoid further slowdowns */
		if (tp->tp_getattro == slot_tp_getattr_hook)
			tp->tp_getattro = PyObject_GenericGetAttr;
		return PyObject_GenericGetAttr(self, name);
	}
	if (getattribute == NULL)
		res = PyObject_GenericGetAttr(self, name);
	else
		res = PyObject_CallFunction(getattribute, "OO", self, name);
	if (getattr != NULL &&
	    res == NULL && PyErr_ExceptionMatches(PyExc_AttributeError)) {
		PyErr_Clear();
		res = PyObject_CallFunction(getattr, "OO", self, name);
	}
	return res;
}

static int
slot_tp_setattro(PyObject *self, PyObject *name, PyObject *value)
{
	PyObject *res;
	static PyObject *delattr_str, *setattr_str;

	if (value == NULL)
		res = call_method(self, "__delattr__", &delattr_str,
				  "(O)", name);
	else
		res = call_method(self, "__setattr__", &setattr_str,
				  "(OO)", name, value);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

/* Map rich comparison operators to their __xx__ namesakes */
static char *name_op[] = {
	"__lt__",
	"__le__",
	"__eq__",
	"__ne__",
	"__gt__",
	"__ge__",
};

static PyObject *
half_richcompare(PyObject *self, PyObject *other, int op)
{
	PyObject *func, *args, *res;
	static PyObject *op_str[6];

	func = lookup_method(self, name_op[op], &op_str[op]);
	if (func == NULL) {
		PyErr_Clear();
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	args = Py_BuildValue("(O)", other);
	if (args == NULL)
		res = NULL;
	else {
		res = PyObject_Call(func, args, NULL);
		Py_DECREF(args);
	}
	Py_DECREF(func);
	return res;
}

/* Map rich comparison operators to their swapped version, e.g. LT --> GT */
static int swapped_op[] = {Py_GT, Py_GE, Py_EQ, Py_NE, Py_LT, Py_LE};

static PyObject *
slot_tp_richcompare(PyObject *self, PyObject *other, int op)
{
	PyObject *res;

	if (self->ob_type->tp_richcompare == slot_tp_richcompare) {
		res = half_richcompare(self, other, op);
		if (res != Py_NotImplemented)
			return res;
		Py_DECREF(res);
	}
	if (other->ob_type->tp_richcompare == slot_tp_richcompare) {
		res = half_richcompare(other, self, swapped_op[op]);
		if (res != Py_NotImplemented) {
			return res;
		}
		Py_DECREF(res);
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static PyObject *
slot_tp_iter(PyObject *self)
{
	PyObject *func, *res;
	static PyObject *iter_str, *getitem_str;

	func = lookup_method(self, "__iter__", &iter_str);
	if (func != NULL) {
		 res = PyObject_CallObject(func, NULL);
		 Py_DECREF(func);
		 return res;
	}
	PyErr_Clear();
	func = lookup_method(self, "__getitem__", &getitem_str);
	if (func == NULL) {
		PyErr_SetString(PyExc_TypeError, "iter() of non-sequence");
		return NULL;
	}
	Py_DECREF(func);
	return PySeqIter_New(self);
}

static PyObject *
slot_tp_iternext(PyObject *self)
{
	static PyObject *next_str;
	return call_method(self, "next", &next_str, "()");
}

static PyObject *
slot_tp_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
	PyTypeObject *tp = self->ob_type;
	PyObject *get;
	static PyObject *get_str = NULL;

	if (get_str == NULL) {
		get_str = PyString_InternFromString("__get__");
		if (get_str == NULL)
			return NULL;
	}
	get = _PyType_Lookup(tp, get_str);
	if (get == NULL) {
		/* Avoid further slowdowns */
		if (tp->tp_descr_get == slot_tp_descr_get)
			tp->tp_descr_get = NULL;
		Py_INCREF(self);
		return self;
	}
	if (obj == NULL)
		obj = Py_None;
	if (type == NULL)
		type = Py_None;
	return PyObject_CallFunction(get, "OOO", self, obj, type);
}

static int
slot_tp_descr_set(PyObject *self, PyObject *target, PyObject *value)
{
	PyObject *res;
	static PyObject *del_str, *set_str;

	if (value == NULL)
		res = call_method(self, "__del__", &del_str,
				  "(O)", target);
	else
		res = call_method(self, "__set__", &set_str,
				  "(OO)", target, value);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static int
slot_tp_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	static PyObject *init_str;
	PyObject *meth = lookup_method(self, "__init__", &init_str);
	PyObject *res;

	if (meth == NULL)
		return -1;
	res = PyObject_Call(meth, args, kwds);
	Py_DECREF(meth);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static PyObject *
slot_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *func = PyObject_GetAttrString((PyObject *)type, "__new__");
	PyObject *newargs, *x;
	int i, n;

	if (func == NULL)
		return NULL;
	assert(PyTuple_Check(args));
	n = PyTuple_GET_SIZE(args);
	newargs = PyTuple_New(n+1);
	if (newargs == NULL)
		return NULL;
	Py_INCREF(type);
	PyTuple_SET_ITEM(newargs, 0, (PyObject *)type);
	for (i = 0; i < n; i++) {
		x = PyTuple_GET_ITEM(args, i);
		Py_INCREF(x);
		PyTuple_SET_ITEM(newargs, i+1, x);
	}
	x = PyObject_Call(func, newargs, kwds);
	Py_DECREF(func);
	return x;
}

/* This is called at the very end of type_new() (even after
   PyType_Ready()) to complete the initialization of dynamic types.
   The dict argument is the dictionary argument passed to type_new(),
   which is the local namespace of the class statement, in other
   words, it contains the methods.  For each special method (like
   __repr__) defined in the dictionary, the corresponding function
   slot in the type object (like tp_repr) is set to a special function
   whose name is 'slot_' followed by the slot name and whose signature
   is whatever is required for that slot.  These slot functions look
   up the corresponding method in the type's dictionary and call it.
   The slot functions have to take care of the various peculiarities
   of the mapping between slots and special methods, such as mapping
   one slot to multiple methods (tp_richcompare <--> __le__, __lt__
   etc.)  or mapping multiple slots to a single method (sq_item,
   mp_subscript <--> __getitem__). */

static void
override_slots(PyTypeObject *type, PyObject *dict)
{
	PySequenceMethods *sq = type->tp_as_sequence;
	PyMappingMethods *mp = type->tp_as_mapping;
	PyNumberMethods *nb = type->tp_as_number;

#define SQSLOT(OPNAME, SLOTNAME, FUNCNAME) \
	if (dict == NULL || PyDict_GetItemString(dict, OPNAME)) { \
		sq->SLOTNAME = FUNCNAME; \
	}

#define MPSLOT(OPNAME, SLOTNAME, FUNCNAME) \
	if (dict == NULL || PyDict_GetItemString(dict, OPNAME)) { \
		mp->SLOTNAME = FUNCNAME; \
	}

#define NBSLOT(OPNAME, SLOTNAME, FUNCNAME) \
	if (dict == NULL || PyDict_GetItemString(dict, OPNAME)) { \
		nb->SLOTNAME = FUNCNAME; \
	}

#define TPSLOT(OPNAME, SLOTNAME, FUNCNAME) \
	if (dict == NULL || PyDict_GetItemString(dict, OPNAME)) { \
		type->SLOTNAME = FUNCNAME; \
	}

	SQSLOT("__len__", sq_length, slot_sq_length);
	SQSLOT("__add__", sq_concat, slot_sq_concat);
	SQSLOT("__mul__", sq_repeat, slot_sq_repeat);
	SQSLOT("__getitem__", sq_item, slot_sq_item);
	SQSLOT("__getslice__", sq_slice, slot_sq_slice);
	SQSLOT("__setitem__", sq_ass_item, slot_sq_ass_item);
	SQSLOT("__delitem__", sq_ass_item, slot_sq_ass_item);
	SQSLOT("__setslice__", sq_ass_slice, slot_sq_ass_slice);
	SQSLOT("__delslice__", sq_ass_slice, slot_sq_ass_slice);
	SQSLOT("__contains__", sq_contains, slot_sq_contains);
	SQSLOT("__iadd__", sq_inplace_concat, slot_sq_inplace_concat);
	SQSLOT("__imul__", sq_inplace_repeat, slot_sq_inplace_repeat);

	MPSLOT("__len__", mp_length, slot_mp_length);
	MPSLOT("__getitem__", mp_subscript, slot_mp_subscript);
	MPSLOT("__setitem__", mp_ass_subscript, slot_mp_ass_subscript);
	MPSLOT("__delitem__", mp_ass_subscript, slot_mp_ass_subscript);

	NBSLOT("__add__", nb_add, slot_nb_add);
	NBSLOT("__sub__", nb_subtract, slot_nb_subtract);
	NBSLOT("__mul__", nb_multiply, slot_nb_multiply);
	NBSLOT("__div__", nb_divide, slot_nb_divide);
	NBSLOT("__mod__", nb_remainder, slot_nb_remainder);
	NBSLOT("__divmod__", nb_divmod, slot_nb_divmod);
	NBSLOT("__pow__", nb_power, slot_nb_power);
	NBSLOT("__neg__", nb_negative, slot_nb_negative);
	NBSLOT("__pos__", nb_positive, slot_nb_positive);
	NBSLOT("__abs__", nb_absolute, slot_nb_absolute);
	NBSLOT("__nonzero__", nb_nonzero, slot_nb_nonzero);
	NBSLOT("__invert__", nb_invert, slot_nb_invert);
	NBSLOT("__lshift__", nb_lshift, slot_nb_lshift);
	NBSLOT("__rshift__", nb_rshift, slot_nb_rshift);
	NBSLOT("__and__", nb_and, slot_nb_and);
	NBSLOT("__xor__", nb_xor, slot_nb_xor);
	NBSLOT("__or__", nb_or, slot_nb_or);
	NBSLOT("__coerce__", nb_coerce, slot_nb_coerce);
	NBSLOT("__int__", nb_int, slot_nb_int);
	NBSLOT("__long__", nb_long, slot_nb_long);
	NBSLOT("__float__", nb_float, slot_nb_float);
	NBSLOT("__oct__", nb_oct, slot_nb_oct);
	NBSLOT("__hex__", nb_hex, slot_nb_hex);
	NBSLOT("__iadd__", nb_inplace_add, slot_nb_inplace_add);
	NBSLOT("__isub__", nb_inplace_subtract, slot_nb_inplace_subtract);
	NBSLOT("__imul__", nb_inplace_multiply, slot_nb_inplace_multiply);
	NBSLOT("__idiv__", nb_inplace_divide, slot_nb_inplace_divide);
	NBSLOT("__imod__", nb_inplace_remainder, slot_nb_inplace_remainder);
	NBSLOT("__ipow__", nb_inplace_power, slot_nb_inplace_power);
	NBSLOT("__ilshift__", nb_inplace_lshift, slot_nb_inplace_lshift);
	NBSLOT("__irshift__", nb_inplace_rshift, slot_nb_inplace_rshift);
	NBSLOT("__iand__", nb_inplace_and, slot_nb_inplace_and);
	NBSLOT("__ixor__", nb_inplace_xor, slot_nb_inplace_xor);
	NBSLOT("__ior__", nb_inplace_or, slot_nb_inplace_or);
	NBSLOT("__floordiv__", nb_floor_divide, slot_nb_floor_divide);
	NBSLOT("__truediv__", nb_true_divide, slot_nb_true_divide);
	NBSLOT("__ifloordiv__", nb_inplace_floor_divide,
	       slot_nb_inplace_floor_divide);
	NBSLOT("__itruediv__", nb_inplace_true_divide,
	       slot_nb_inplace_true_divide);

	if (dict == NULL ||
	    PyDict_GetItemString(dict, "__str__") ||
	    PyDict_GetItemString(dict, "__repr__"))
		type->tp_print = NULL;

	TPSLOT("__cmp__", tp_compare, _PyObject_SlotCompare);
	TPSLOT("__repr__", tp_repr, slot_tp_repr);
	TPSLOT("__hash__", tp_hash, slot_tp_hash);
	TPSLOT("__call__", tp_call, slot_tp_call);
	TPSLOT("__str__", tp_str, slot_tp_str);
	TPSLOT("__getattribute__", tp_getattro, slot_tp_getattro);
	TPSLOT("__getattr__", tp_getattro, slot_tp_getattr_hook);
	TPSLOT("__setattr__", tp_setattro, slot_tp_setattro);
	TPSLOT("__lt__", tp_richcompare, slot_tp_richcompare);
	TPSLOT("__le__", tp_richcompare, slot_tp_richcompare);
	TPSLOT("__eq__", tp_richcompare, slot_tp_richcompare);
	TPSLOT("__ne__", tp_richcompare, slot_tp_richcompare);
	TPSLOT("__gt__", tp_richcompare, slot_tp_richcompare);
	TPSLOT("__ge__", tp_richcompare, slot_tp_richcompare);
	TPSLOT("__iter__", tp_iter, slot_tp_iter);
	TPSLOT("next", tp_iternext, slot_tp_iternext);
	TPSLOT("__get__", tp_descr_get, slot_tp_descr_get);
	TPSLOT("__set__", tp_descr_set, slot_tp_descr_set);
	TPSLOT("__init__", tp_init, slot_tp_init);
	TPSLOT("__new__", tp_new, slot_tp_new);
}


/* Cooperative 'super' */

typedef struct {
	PyObject_HEAD
	PyTypeObject *type;
	PyObject *obj;
} superobject;

static PyMemberDef super_members[] = {
	{"__thisclass__", T_OBJECT, offsetof(superobject, type), READONLY,
	 "the class invoking super()"},
	{"__self__",  T_OBJECT, offsetof(superobject, obj), READONLY,
	 "the instance invoking super(); may be None"},
	{0}
};

static void
super_dealloc(PyObject *self)
{
	superobject *su = (superobject *)self;

	Py_XDECREF(su->obj);
	Py_XDECREF(su->type);
	self->ob_type->tp_free(self);
}

static PyObject *
super_repr(PyObject *self)
{
	superobject *su = (superobject *)self;

	if (su->obj)
		return PyString_FromFormat(
			"<super: <class '%s'>, <%s object>>",
			su->type ? su->type->tp_name : "NULL",
			su->obj->ob_type->tp_name);
	else
		return PyString_FromFormat(
			"<super: <class '%s'>, NULL>",
			su->type ? su->type->tp_name : "NULL");
}

static PyObject *
super_getattro(PyObject *self, PyObject *name)
{
	superobject *su = (superobject *)self;

	if (su->obj != NULL) {
		PyObject *mro, *res, *tmp;
		descrgetfunc f;
		int i, n;

		mro = su->obj->ob_type->tp_mro;
		if (mro == NULL)
			n = 0;
		else {
			assert(PyTuple_Check(mro));
			n = PyTuple_GET_SIZE(mro);
		}
		for (i = 0; i < n; i++) {
			if ((PyObject *)(su->type) == PyTuple_GET_ITEM(mro, i))
				break;
		}
		if (i >= n && PyType_Check(su->obj)) {
			mro = ((PyTypeObject *)(su->obj))->tp_mro;
			if (mro == NULL)
				n = 0;
			else {
				assert(PyTuple_Check(mro));
				n = PyTuple_GET_SIZE(mro);
			}
			for (i = 0; i < n; i++) {
				if ((PyObject *)(su->type) ==
				    PyTuple_GET_ITEM(mro, i))
					break;
			}
		}
		i++;
		res = NULL;
		for (; i < n; i++) {
			tmp = PyTuple_GET_ITEM(mro, i);
			assert(PyType_Check(tmp));
			res = PyDict_GetItem(
				((PyTypeObject *)tmp)->tp_defined, name);
			if (res != NULL) {
				Py_INCREF(res);
				f = res->ob_type->tp_descr_get;
				if (f != NULL) {
					tmp = f(res, su->obj, res);
					Py_DECREF(res);
					res = tmp;
				}
				return res;
			}
		}
	}
	return PyObject_GenericGetAttr(self, name);
}

static PyObject *
super_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
	superobject *su = (superobject *)self;
	superobject *new;

	if (obj == NULL || obj == Py_None || su->obj != NULL) {
		/* Not binding to an object, or already bound */
		Py_INCREF(self);
		return self;
	}
	new = (superobject *)PySuper_Type.tp_new(&PySuper_Type, NULL, NULL);
	if (new == NULL)
		return NULL;
	Py_INCREF(su->type);
	Py_INCREF(obj);
	new->type = su->type;
	new->obj = obj;
	return (PyObject *)new;
}

static int
super_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	superobject *su = (superobject *)self;
	PyTypeObject *type;
	PyObject *obj = NULL;

	if (!PyArg_ParseTuple(args, "O!|O:super", &PyType_Type, &type, &obj))
		return -1;
	if (obj == Py_None)
		obj = NULL;
	if (obj != NULL &&
	    !PyType_IsSubtype(obj->ob_type, type) &&
	    !(PyType_Check(obj) &&
	      PyType_IsSubtype((PyTypeObject *)obj, type))) {
		PyErr_SetString(PyExc_TypeError,
			"super(type, obj): "
			"obj must be an instance or subtype of type");
		return -1;
	}
	Py_INCREF(type);
	Py_XINCREF(obj);
	su->type = type;
	su->obj = obj;
	return 0;
}

static char super_doc[] =
"super(type) -> unbound super object\n"
"super(type, obj) -> bound super object; requires isinstance(obj, type)\n"
"super(type, type2) -> bound super object; requires issubclass(type2, type)\n"
"Typical use to call a cooperative superclass method:\n"
"class C(B):\n"
"    def meth(self, arg):\n"
"        super(C, self).meth(arg)";

PyTypeObject PySuper_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"super",				/* tp_name */
	sizeof(superobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	super_dealloc,		 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	super_repr,				/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,		       			/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	super_getattro,				/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
 	super_doc,				/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	super_members,				/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	super_descr_get,			/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	super_init,				/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	_PyObject_Del,				/* tp_free */
};
