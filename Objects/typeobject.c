/* Type object implementation */

#include "Python.h"
#include "structmember.h"

#include <ctype.h>

/* The *real* layout of a type object when allocated on the heap */
/* XXX Should we publish this in a header file? */
typedef struct {
	/* Note: there's a dependency on the order of these members
	   in slotptr() below. */
	PyTypeObject type;
	PyNumberMethods as_number;
	PyMappingMethods as_mapping;
	PySequenceMethods as_sequence; /* as_sequence comes after as_mapping,
					  so that the mapping wins when both
					  the mapping and the sequence define
					  a given operator (e.g. __getitem__).
					  see add_operators() below. */
	PyBufferProcs as_buffer;
	PyObject *name, *slots;
	PyMemberDef members[1];
} etype;

static PyMemberDef type_members[] = {
	{"__basicsize__", T_INT, offsetof(PyTypeObject,tp_basicsize),READONLY},
	{"__itemsize__", T_INT, offsetof(PyTypeObject, tp_itemsize), READONLY},
	{"__flags__", T_LONG, offsetof(PyTypeObject, tp_flags), READONLY},
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
	mod = PyDict_GetItemString(type->tp_dict, "__module__");
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
	if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE) ||
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
	return PyDictProxy_New(type->tp_dict);
}

static PyObject *
type_get_doc(PyTypeObject *type, void *context)
{
	PyObject *result;
	if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE) && type->tp_doc != NULL)
		return PyString_FromString(type->tp_doc);
	result = PyDict_GetItemString(type->tp_dict, "__doc__");
	if (result == NULL) {
		result = Py_None;
		Py_INCREF(result);
	}
	else if (result->ob_type->tp_descr_get) {
		result = result->ob_type->tp_descr_get(result, NULL,
						       (PyObject *)type);
	}
	else {
		Py_INCREF(result);
	}
	return result;
}

static PyGetSetDef type_getsets[] = {
	{"__name__", (getter)type_name, NULL, NULL},
	{"__module__", (getter)type_module, (setter)type_set_module, NULL},
	{"__dict__",  (getter)type_dict,  NULL, NULL},
	{"__doc__", (getter)type_get_doc, NULL, NULL},
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
		/* Ugly exception: when the call was type(something),
		   don't call tp_init on the result. */
		if (type == &PyType_Type &&
		    PyTuple_Check(args) && PyTuple_GET_SIZE(args) == 1 &&
		    (kwds == NULL ||
		     (PyDict_Check(kwds) && PyDict_Size(kwds) == 0)))
			return obj;
		/* If the returned object is not an instance of type,
		   it won't be initialized. */
		if (!PyType_IsSubtype(obj->ob_type, type))
			return obj;
		type = obj->ob_type;
		if (PyType_HasFeature(type, Py_TPFLAGS_HAVE_CLASS) &&
		    type->tp_init != NULL &&
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
	PyObject *obj;
	const size_t size = _PyObject_VAR_SIZE(type, nitems);

	if (PyType_IS_GC(type))
		obj = _PyObject_GC_Malloc(size);
	else
		obj = PyObject_MALLOC(size);

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

/* Helpers for subtyping */

static int
traverse_slots(PyTypeObject *type, PyObject *self, visitproc visit, void *arg)
{
	int i, n;
	PyMemberDef *mp;

	n = type->ob_size;
	mp = ((etype *)type)->members;
	for (i = 0; i < n; i++, mp++) {
		if (mp->type == T_OBJECT_EX) {
			char *addr = (char *)self + mp->offset;
			PyObject *obj = *(PyObject **)addr;
			if (obj != NULL) {
				int err = visit(obj, arg);
				if (err)
					return err;
			}
		}
	}
	return 0;
}

static int
subtype_traverse(PyObject *self, visitproc visit, void *arg)
{
	PyTypeObject *type, *base;
	traverseproc basetraverse;

	/* Find the nearest base with a different tp_traverse,
	   and traverse slots while we're at it */
	type = self->ob_type;
	base = type;
	while ((basetraverse = base->tp_traverse) == subtype_traverse) {
		if (base->ob_size) {
			int err = traverse_slots(base, self, visit, arg);
			if (err)
				return err;
		}
		base = base->tp_base;
		assert(base);
	}

	if (type->tp_dictoffset != base->tp_dictoffset) {
		PyObject **dictptr = _PyObject_GetDictPtr(self);
		if (dictptr && *dictptr) {
			int err = visit(*dictptr, arg);
			if (err)
				return err;
		}
	}

	if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
		/* For a heaptype, the instances count as references
		   to the type.  Traverse the type so the collector
		   can find cycles involving this link. */
		int err = visit((PyObject *)type, arg);
		if (err)
			return err;
	}

	if (basetraverse)
		return basetraverse(self, visit, arg);
	return 0;
}

static void
clear_slots(PyTypeObject *type, PyObject *self)
{
	int i, n;
	PyMemberDef *mp;

	n = type->ob_size;
	mp = ((etype *)type)->members;
	for (i = 0; i < n; i++, mp++) {
		if (mp->type == T_OBJECT_EX && !(mp->flags & READONLY)) {
			char *addr = (char *)self + mp->offset;
			PyObject *obj = *(PyObject **)addr;
			if (obj != NULL) {
				Py_DECREF(obj);
				*(PyObject **)addr = NULL;
			}
		}
	}
}

static int
subtype_clear(PyObject *self)
{
	PyTypeObject *type, *base;
	inquiry baseclear;

	/* Find the nearest base with a different tp_clear
	   and clear slots while we're at it */
	type = self->ob_type;
	base = type;
	while ((baseclear = base->tp_clear) == subtype_clear) {
		if (base->ob_size)
			clear_slots(base, self);
		base = base->tp_base;
		assert(base);
	}

	/* There's no need to clear the instance dict (if any);
	   the collector will call its tp_clear handler. */

	if (baseclear)
		return baseclear(self);
	return 0;
}

static PyObject *lookup_maybe(PyObject *, char *, PyObject **);

static int
call_finalizer(PyObject *self)
{
	static PyObject *del_str = NULL;
	PyObject *del, *res;
	PyObject *error_type, *error_value, *error_traceback;

	/* Temporarily resurrect the object. */
	assert(self->ob_refcnt == 0);
	self->ob_refcnt = 1;

	/* Save the current exception, if any. */
	PyErr_Fetch(&error_type, &error_value, &error_traceback);

	/* Execute __del__ method, if any. */
	del = lookup_maybe(self, "__del__", &del_str);
	if (del != NULL) {
		res = PyEval_CallObject(del, NULL);
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
	assert(self->ob_refcnt > 0);
	if (--self->ob_refcnt == 0)
		return 0;	/* this is the normal path out */

	/* __del__ resurrected it!  Make it look like the original Py_DECREF
	 * never happened.
	 */
	{
		int refcnt = self->ob_refcnt;
		_Py_NewReference(self);
		self->ob_refcnt = refcnt;
	}
	assert(!PyType_IS_GC(self->ob_type) ||
	       _Py_AS_GC(self)->gc.gc_refs != _PyGC_REFS_UNTRACKED);
	/* If Py_REF_DEBUG, the original decref dropped _Py_RefTotal, but
	 * _Py_NewReference bumped it again, so that's a wash.
	 * If Py_TRACE_REFS, _Py_NewReference re-added self to the object
	 * chain, so no more to do there either.
	 * If COUNT_ALLOCS, the original decref bumped tp_frees, and
	 * _Py_NewReference bumped tp_allocs:  both of those need to be
	 * undone.
	 */
#ifdef COUNT_ALLOCS
	--self->ob_type->tp_frees;
	--self->ob_type->tp_allocs;
#endif
	return -1; /* __del__ added a reference; don't delete now */
}

static void
subtype_dealloc(PyObject *self)
{
	PyTypeObject *type, *base;
	destructor basedealloc;

	/* Extract the type; we expect it to be a heap type */
	type = self->ob_type;
	assert(type->tp_flags & Py_TPFLAGS_HEAPTYPE);

	/* Test whether the type has GC exactly once */

	if (!PyType_IS_GC(type)) {
		/* It's really rare to find a dynamic type that doesn't have
		   GC; it can only happen when deriving from 'object' and not
		   adding any slots or instance variables.  This allows
		   certain simplifications: there's no need to call
		   clear_slots(), or DECREF the dict, or clear weakrefs. */

		/* Maybe call finalizer; exit early if resurrected */
		if (call_finalizer(self) < 0)
			return;

		/* Find the nearest base with a different tp_dealloc */
		base = type;
		while ((basedealloc = base->tp_dealloc) == subtype_dealloc) {
			assert(base->ob_size == 0);
			base = base->tp_base;
			assert(base);
		}

		/* Call the base tp_dealloc() */
		assert(basedealloc);
		basedealloc(self);

		/* Can't reference self beyond this point */
		Py_DECREF(type);

		/* Done */
		return;
	}

	/* We get here only if the type has GC */

	/* UnTrack and re-Track around the trashcan macro, alas */
	PyObject_GC_UnTrack(self);
	Py_TRASHCAN_SAFE_BEGIN(self);
	_PyObject_GC_TRACK(self); /* We'll untrack for real later */

	/* Maybe call finalizer; exit early if resurrected */
	if (call_finalizer(self) < 0)
		goto endlabel;

	/* Find the nearest base with a different tp_dealloc
	   and clear slots while we're at it */
	base = type;
	while ((basedealloc = base->tp_dealloc) == subtype_dealloc) {
		if (base->ob_size)
			clear_slots(base, self);
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
	if (!PyType_IS_GC(base))
		_PyObject_GC_UNTRACK(self);

	/* Call the base tp_dealloc() */
	assert(basedealloc);
	basedealloc(self);

	/* Can't reference self beyond this point */
	Py_DECREF(type);

  endlabel:
	Py_TRASHCAN_SAFE_END(self);
}

static PyTypeObject *solid_base(PyTypeObject *type);

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

static PyObject *
call_method(PyObject *o, char *name, PyObject **nameobj, char *format, ...)
{
	va_list va;
	PyObject *args, *func = 0, *retval;
	va_start(va, format);

	func = lookup_maybe(o, name, nameobj);
	if (func == NULL) {
		va_end(va);
		if (!PyErr_Occurred())
			PyErr_SetObject(PyExc_AttributeError, *nameobj);
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

/* Clone of call_method() that returns NotImplemented when the lookup fails. */

static PyObject *
call_maybe(PyObject *o, char *name, PyObject **nameobj, char *format, ...)
{
	va_list va;
	PyObject *args, *func = 0, *retval;
	va_start(va, format);

	func = lookup_maybe(o, name, nameobj);
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

static int
fill_classic_mro(PyObject *mro, PyObject *cls)
{
	PyObject *bases, *base;
	int i, n;

	assert(PyList_Check(mro));
	assert(PyClass_Check(cls));
	i = PySequence_Contains(mro, cls);
	if (i < 0)
		return -1;
	if (!i) {
		if (PyList_Append(mro, cls) < 0)
			return -1;
	}
	bases = ((PyClassObject *)cls)->cl_bases;
	assert(bases && PyTuple_Check(bases));
	n = PyTuple_GET_SIZE(bases);
	for (i = 0; i < n; i++) {
		base = PyTuple_GET_ITEM(bases, i);
		if (fill_classic_mro(mro, base) < 0)
			return -1;
	}
	return 0;
}

static PyObject *
classic_mro(PyObject *cls)
{
	PyObject *mro;

	assert(PyClass_Check(cls));
	mro = PyList_New(0);
	if (mro != NULL) {
		if (fill_classic_mro(mro, cls) == 0)
			return mro;
		Py_DECREF(mro);
	}
	return NULL;
}

static PyObject *
mro_implementation(PyTypeObject *type)
{
	int i, n, ok;
	PyObject *bases, *result;

	if(type->tp_dict == NULL) {
		if(PyType_Ready(type) < 0)
			return NULL;
	}

	bases = type->tp_bases;
	n = PyTuple_GET_SIZE(bases);
	result = Py_BuildValue("[O]", (PyObject *)type);
	if (result == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		PyObject *base = PyTuple_GET_ITEM(bases, i);
		PyObject *parentMRO;
		if (PyType_Check(base))
			parentMRO = PySequence_List(
				((PyTypeObject*)base)->tp_mro);
		else
			parentMRO = classic_mro(base);
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
	PyObject *base_proto;

	assert(PyTuple_Check(bases));
	n = PyTuple_GET_SIZE(bases);
	assert(n > 0);
	base = NULL;
	winner = NULL;
	for (i = 0; i < n; i++) {
		base_proto = PyTuple_GET_ITEM(bases, i);
		if (PyClass_Check(base_proto))
			continue;
		if (!PyType_Check(base_proto)) {
			PyErr_SetString(
				PyExc_TypeError,
				"bases must be types");
			return NULL;
		}
		base_i = (PyTypeObject *)base_proto;
		if (base_i->tp_dict == NULL) {
			if (PyType_Ready(base_i) < 0)
				return NULL;
		}
		candidate = solid_base(base_i);
		if (winner == NULL) {
			winner = candidate;
			base = base_i;
		}
		else if (PyType_IsSubtype(winner, candidate))
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
	if (base == NULL)
		PyErr_SetString(PyExc_TypeError,
			"a new-style class can't have only classic bases");
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

static void object_dealloc(PyObject *);
static int object_init(PyObject *, PyObject *, PyObject *);
static int update_slot(PyTypeObject *, PyObject *);
static void fixup_slot_dispatchers(PyTypeObject *);

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

static int
subtype_setdict(PyObject *obj, PyObject *value, void *context)
{
	PyObject **dictptr = _PyObject_GetDictPtr(obj);
	PyObject *dict;

	if (dictptr == NULL) {
		PyErr_SetString(PyExc_AttributeError,
				"This object has no __dict__");
		return -1;
	}
	if (value != NULL && !PyDict_Check(value)) {
		PyErr_SetString(PyExc_TypeError,
				"__dict__ must be set to a dictionary");
		return -1;
	}
	dict = *dictptr;
	Py_XINCREF(value);
	*dictptr = value;
	Py_XDECREF(dict);
	return 0;
}

static PyGetSetDef subtype_getsets[] = {
	{"__dict__", subtype_dict, subtype_setdict, NULL},
	{0},
};

/* bozo: __getstate__ that raises TypeError */

static PyObject *
bozo_func(PyObject *self, PyObject *args)
{
	PyErr_SetString(PyExc_TypeError,
			"a class that defines __slots__ without "
			"defining __getstate__ cannot be pickled");
	return NULL;
}

static PyMethodDef bozo_ml = {"__getstate__", bozo_func, METH_VARARGS};

static PyObject *bozo_obj = NULL;

static int
valid_identifier(PyObject *s)
{
	unsigned char *p;
	int i, n;

	if (!PyString_Check(s)) {
		PyErr_SetString(PyExc_TypeError,
				"__slots__ must be strings");
		return 0;
	}
	p = (unsigned char *) PyString_AS_STRING(s);
	n = PyString_GET_SIZE(s);
	/* We must reject an empty name.  As a hack, we bump the
	   length to 1 so that the loop will balk on the trailing \0. */
	if (n == 0)
		n = 1;
	for (i = 0; i < n; i++, p++) {
		if (!(i == 0 ? isalpha(*p) : isalnum(*p)) && *p != '_') {
			PyErr_SetString(PyExc_TypeError,
					"__slots__ must be identifiers");
			return 0;
		}
	}
	return 1;
}

static PyObject *
type_new(PyTypeObject *metatype, PyObject *args, PyObject *kwds)
{
	PyObject *name, *bases, *dict;
	static char *kwlist[] = {"name", "bases", "dict", 0};
	static char buffer[256];
	PyObject *slots, *tmp, *newslots;
	PyTypeObject *type, *base, *tmptype, *winner;
	etype *et;
	PyMemberDef *mp;
	int i, nbases, nslots, slotoffset, add_dict, add_weak;

	assert(args != NULL && PyTuple_Check(args));
	assert(kwds == NULL || PyDict_Check(kwds));

	/* Special case: type(x) should return x->ob_type */
	{
		const int nargs = PyTuple_GET_SIZE(args);
		const int nkwds = kwds == NULL ? 0 : PyDict_Size(kwds);

		if (PyType_CheckExact(metatype) && nargs == 1 && nkwds == 0) {
			PyObject *x = PyTuple_GET_ITEM(args, 0);
			Py_INCREF(x->ob_type);
			return (PyObject *) x->ob_type;
		}

		/* SF bug 475327 -- if that didn't trigger, we need 3
		   arguments. but PyArg_ParseTupleAndKeywords below may give
		   a msg saying type() needs exactly 3. */
		if (nargs + nkwds != 3) {
			PyErr_SetString(PyExc_TypeError,
					"type() takes 1 or 3 arguments");
			return NULL;
		}
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
		if (tmptype == &PyClass_Type)
			continue; /* Special case classic classes */
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
			if (!valid_identifier(PyTuple_GET_ITEM(slots, i))) {
				Py_DECREF(slots);
				return NULL;
			}
		}

		newslots = PyTuple_New(nslots);
		if (newslots == NULL)
			return NULL;
		for (i = 0; i < nslots; i++) {
			tmp = PyTuple_GET_ITEM(slots, i);
			if (_Py_Mangle(PyString_AS_STRING(name),
				PyString_AS_STRING(tmp),
				buffer, sizeof(buffer)))
			{
				tmp = PyString_FromString(buffer);
			} else {
				Py_INCREF(tmp);
			}
			PyTuple_SET_ITEM(newslots, i, tmp);
		}
		Py_DECREF(slots);
		slots = newslots;

	}
	if (slots != NULL) {
		/* See if *this* class defines __getstate__ */
		PyObject *getstate = PyDict_GetItemString(dict,
							  "__getstate__");
		if (getstate == NULL) {
			/* If not, provide a bozo that raises TypeError */
			if (bozo_obj == NULL) {
				bozo_obj = PyCFunction_New(&bozo_ml, NULL);
				if (bozo_obj == NULL) {
					/* XXX decref various things */
					return NULL;
				}
			}
			if (PyDict_SetItemString(dict,
						 "__getstate__",
						 bozo_obj) < 0) {
				/* XXX decref various things */
				return NULL;
			}
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
	if (base->tp_flags & Py_TPFLAGS_HAVE_GC)
		type->tp_flags |= Py_TPFLAGS_HAVE_GC;

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

	/* Initialize tp_dict from passed-in dict */
	type->tp_dict = dict = PyDict_Copy(dict);
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

	/* Set tp_doc to a copy of dict['__doc__'], if the latter is there
	   and is a string.  The __doc__ accessor will first look for tp_doc;
	   if that fails, it will still look into __dict__.
	*/
	{
		PyObject *doc = PyDict_GetItemString(dict, "__doc__");
		if (doc != NULL && PyString_Check(doc)) {
			const size_t n = (size_t)PyString_GET_SIZE(doc);
			type->tp_doc = (char *)PyObject_MALLOC(n+1);
			if (type->tp_doc == NULL) {
				Py_DECREF(type);
				return NULL;
			}
			memcpy(type->tp_doc, PyString_AS_STRING(doc), n+1);
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
			mp->type = T_OBJECT_EX;
			mp->offset = slotoffset;
			if (base->tp_weaklistoffset == 0 &&
			    strcmp(mp->name, "__weakref__") == 0) {
				mp->type = T_OBJECT;
				mp->flags = READONLY;
				type->tp_weaklistoffset = slotoffset;
			}
			slotoffset += sizeof(PyObject *);
		}
	}
	else {
		if (add_dict) {
			if (base->tp_itemsize)
				type->tp_dictoffset =
					-(long)sizeof(PyObject *);
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

	/* Enable GC unless there are really no instance variables possible */
	if (!(type->tp_basicsize == sizeof(PyObject) &&
	      type->tp_itemsize == 0))
		type->tp_flags |= Py_TPFLAGS_HAVE_GC;

	/* Always override allocation strategy to use regular heap */
	type->tp_alloc = PyType_GenericAlloc;
	if (type->tp_flags & Py_TPFLAGS_HAVE_GC) {
		type->tp_free = PyObject_GC_Del;
		type->tp_traverse = subtype_traverse;
		type->tp_clear = subtype_clear;
	}
	else
		type->tp_free = PyObject_Del;

	/* Initialize the rest */
	if (PyType_Ready(type) < 0) {
		Py_DECREF(type);
		return NULL;
	}

	/* Put the proper slots in place */
	fixup_slot_dispatchers(type);

	return (PyObject *)type;
}

/* Internal API to look for a name through the MRO.
   This returns a borrowed reference, and doesn't set an exception! */
PyObject *
_PyType_Lookup(PyTypeObject *type, PyObject *name)
{
	int i, n;
	PyObject *mro, *res, *base, *dict;

	/* Look in tp_dict of types in MRO */
	mro = type->tp_mro;

	/* If mro is NULL, the type is either not yet initialized
	   by PyType_Ready(), or already cleared by type_clear().
	   Either way the safest thing to do is to return NULL. */
	if (mro == NULL)
		return NULL;

	assert(PyTuple_Check(mro));
	n = PyTuple_GET_SIZE(mro);
	for (i = 0; i < n; i++) {
		base = PyTuple_GET_ITEM(mro, i);
		if (PyClass_Check(base))
			dict = ((PyClassObject *)base)->cl_dict;
		else {
			assert(PyType_Check(base));
			dict = ((PyTypeObject *)base)->tp_dict;
		}
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
	PyObject *meta_attribute, *attribute;
	descrgetfunc meta_get;

	/* Initialize this type (we'll assume the metatype is initialized) */
	if (type->tp_dict == NULL) {
		if (PyType_Ready(type) < 0)
			return NULL;
	}

	/* No readable descriptor found yet */
	meta_get = NULL;

	/* Look for the attribute in the metatype */
	meta_attribute = _PyType_Lookup(metatype, name);

	if (meta_attribute != NULL) {
		meta_get = meta_attribute->ob_type->tp_descr_get;

		if (meta_get != NULL && PyDescr_IsData(meta_attribute)) {
			/* Data descriptors implement tp_descr_set to intercept
			 * writes. Assume the attribute is not overridden in
			 * type's tp_dict (and bases): call the descriptor now.
			 */
			return meta_get(meta_attribute, (PyObject *)type,
					(PyObject *)metatype);
		}
	}

	/* No data descriptor found on metatype. Look in tp_dict of this
	 * type and its bases */
	attribute = _PyType_Lookup(type, name);
	if (attribute != NULL) {
		/* Implement descriptor functionality, if any */
		descrgetfunc local_get = attribute->ob_type->tp_descr_get;
		if (local_get != NULL) {
			/* NULL 2nd argument indicates the descriptor was
			 * found on the target object itself (or a base)  */
			return local_get(attribute, (PyObject *)NULL,
					 (PyObject *)type);
		}

		Py_INCREF(attribute);
		return attribute;
	}

	/* No attribute found in local __dict__ (or bases): use the
	 * descriptor from the metatype, if any */
	if (meta_get != NULL)
		return meta_get(meta_attribute, (PyObject *)type,
				(PyObject *)metatype);

	/* If an ordinary attribute was found on the metatype, return it now */
	if (meta_attribute != NULL) {
		Py_INCREF(meta_attribute);
		return meta_attribute;
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
	if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
		PyErr_Format(
			PyExc_TypeError,
			"can't set attributes of built-in/extension type '%s'",
			type->tp_name);
		return -1;
	}
	if (PyObject_GenericSetAttr((PyObject *)type, name, value) < 0)
		return -1;
	return update_slot(type, name);
}

static void
type_dealloc(PyTypeObject *type)
{
	etype *et;

	/* Assert this is a heap-allocated type object */
	assert(type->tp_flags & Py_TPFLAGS_HEAPTYPE);
	_PyObject_GC_UNTRACK(type);
	PyObject_ClearWeakRefs((PyObject *)type);
	et = (etype *)type;
	Py_XDECREF(type->tp_base);
	Py_XDECREF(type->tp_dict);
	Py_XDECREF(type->tp_bases);
	Py_XDECREF(type->tp_mro);
	Py_XDECREF(type->tp_cache);
	Py_XDECREF(type->tp_subclasses);
	PyObject_Free(type->tp_doc);
	Py_XDECREF(et->name);
	Py_XDECREF(et->slots);
	type->ob_type->tp_free((PyObject *)type);
}

static PyObject *
type_subclasses(PyTypeObject *type, PyObject *args_ignored)
{
	PyObject *list, *raw, *ref;
	int i, n;

	list = PyList_New(0);
	if (list == NULL)
		return NULL;
	raw = type->tp_subclasses;
	if (raw == NULL)
		return list;
	assert(PyList_Check(raw));
	n = PyList_GET_SIZE(raw);
	for (i = 0; i < n; i++) {
		ref = PyList_GET_ITEM(raw, i);
		assert(PyWeakref_CheckRef(ref));
		ref = PyWeakref_GET_OBJECT(ref);
		if (ref != Py_None) {
			if (PyList_Append(list, ref) < 0) {
				Py_DECREF(list);
				return NULL;
			}
		}
	}
	return list;
}

static PyMethodDef type_methods[] = {
	{"mro", (PyCFunction)mro_external, METH_NOARGS,
	 "mro() -> list\nreturn a type's method resolution order"},
	{"__subclasses__", (PyCFunction)type_subclasses, METH_NOARGS,
	 "__subclasses__() -> list of immediate subclasses"},
	{0}
};

PyDoc_STRVAR(type_doc,
"type(object) -> the object's type\n"
"type(name, bases, dict) -> a new type");

static int
type_traverse(PyTypeObject *type, visitproc visit, void *arg)
{
	int err;

	/* Because of type_is_gc(), the collector only calls this
	   for heaptypes. */
	assert(type->tp_flags & Py_TPFLAGS_HEAPTYPE);

#define VISIT(SLOT) \
	if (SLOT) { \
		err = visit((PyObject *)(SLOT), arg); \
		if (err) \
			return err; \
	}

	VISIT(type->tp_dict);
	VISIT(type->tp_cache);
	VISIT(type->tp_mro);
	VISIT(type->tp_bases);
	VISIT(type->tp_base);

	/* There's no need to visit type->tp_subclasses or
	   ((etype *)type)->slots, because they can't be involved
	   in cycles; tp_subclasses is a list of weak references,
	   and slots is a tuple of strings. */

#undef VISIT

	return 0;
}

static int
type_clear(PyTypeObject *type)
{
	PyObject *tmp;

	/* Because of type_is_gc(), the collector only calls this
	   for heaptypes. */
	assert(type->tp_flags & Py_TPFLAGS_HEAPTYPE);

#define CLEAR(SLOT) \
	if (SLOT) { \
		tmp = (PyObject *)(SLOT); \
		SLOT = NULL; \
		Py_DECREF(tmp); \
	}

	/* The only field we need to clear is tp_mro, which is part of a
	   hard cycle (its first element is the class itself) that won't
	   be broken otherwise (it's a tuple and tuples don't have a
	   tp_clear handler).  None of the other fields need to be
	   cleared, and here's why:

	   tp_dict:
	       It is a dict, so the collector will call its tp_clear.

	   tp_cache:
	       Not used; if it were, it would be a dict.

	   tp_bases, tp_base:
	       If these are involved in a cycle, there must be at least
	       one other, mutable object in the cycle, e.g. a base
	       class's dict; the cycle will be broken that way.

	   tp_subclasses:
	       A list of weak references can't be part of a cycle; and
	       lists have their own tp_clear.

	   slots (in etype):
	       A tuple of strings can't be part of a cycle.
	*/

	CLEAR(type->tp_mro);

#undef CLEAR

	return 0;
}

static int
type_is_gc(PyTypeObject *type)
{
	return type->tp_flags & Py_TPFLAGS_HEAPTYPE;
}

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
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,		/* tp_flags */
	type_doc,				/* tp_doc */
	(traverseproc)type_traverse,		/* tp_traverse */
	(inquiry)type_clear,			/* tp_clear */
	0,					/* tp_richcompare */
	offsetof(PyTypeObject, tp_weaklist),	/* tp_weaklistoffset */
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
	PyObject_GC_Del,        		/* tp_free */
	(inquiry)type_is_gc,			/* tp_is_gc */
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

	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"can't delete __class__ attribute");
		return -1;
	}
	if (!PyType_Check(value)) {
		PyErr_Format(PyExc_TypeError,
		  "__class__ must be set to new-style class, not '%s' object",
		  value->ob_type->tp_name);
		return -1;
	}
	new = (PyTypeObject *)value;
	if (new->tp_dealloc != old->tp_dealloc ||
	    new->tp_free != old->tp_free)
	{
		PyErr_Format(PyExc_TypeError,
			     "__class__ assignment: "
			     "'%s' deallocator differs from '%s'",
			     new->tp_name,
			     old->tp_name);
		return -1;
	}
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
	PyObject_Del,           		/* tp_free */
};


/* Initialize the __dict__ in a type object */

static PyObject *
create_specialmethod(PyMethodDef *meth, PyObject *(*func)(PyObject *))
{
	PyObject *cfunc;
	PyObject *result;

	cfunc = PyCFunction_New(meth, NULL);
	if (cfunc == NULL)
		return NULL;
	result = func(cfunc);
	Py_DECREF(cfunc);
	return result;
}

static int
add_methods(PyTypeObject *type, PyMethodDef *meth)
{
	PyObject *dict = type->tp_dict;

	for (; meth->ml_name != NULL; meth++) {
		PyObject *descr;
		if (PyDict_GetItemString(dict, meth->ml_name))
			continue;
		if (meth->ml_flags & METH_CLASS) {
			if (meth->ml_flags & METH_STATIC) {
				PyErr_SetString(PyExc_ValueError,
				     "method cannot be both class and static");
				return -1;
			}
			descr = create_specialmethod(meth, PyClassMethod_New);
		}
		else if (meth->ml_flags & METH_STATIC) {
			descr = create_specialmethod(meth, PyStaticMethod_New);
		}
		else {
			descr = PyDescr_NewMethod(type, meth);
		}
		if (descr == NULL)
			return -1;
		if (PyDict_SetItemString(dict, meth->ml_name, descr) < 0)
			return -1;
		Py_DECREF(descr);
	}
	return 0;
}

static int
add_members(PyTypeObject *type, PyMemberDef *memb)
{
	PyObject *dict = type->tp_dict;

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
	PyObject *dict = type->tp_dict;

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
		/* The condition below could use some explanation.
		   It appears that tp_new is not inherited for static types
		   whose base class is 'object'; this seems to be a precaution
		   so that old extension types don't suddenly become
		   callable (object.__new__ wouldn't insure the invariants
		   that the extension type's own factory function ensures).
		   Heap types, of course, are under our control, so they do
		   inherit tp_new; static extension types that specify some
		   other built-in type as the default are considered
		   new-style-aware so they also inherit object.__new__. */
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
#undef COPYBUF

#define SLOTDEFINED(SLOT) \
	(base->SLOT != 0 && \
	 (basebase == NULL || base->SLOT != basebase->SLOT))

#define COPYSLOT(SLOT) \
	if (!type->SLOT && SLOTDEFINED(SLOT)) type->SLOT = base->SLOT

#define COPYNUM(SLOT) COPYSLOT(tp_as_number->SLOT)
#define COPYSEQ(SLOT) COPYSLOT(tp_as_sequence->SLOT)
#define COPYMAP(SLOT) COPYSLOT(tp_as_mapping->SLOT)
#define COPYBUF(SLOT) COPYSLOT(tp_as_buffer->SLOT)

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

	if (type->tp_as_buffer != NULL && base->tp_as_buffer != NULL) {
		basebase = base->tp_base;
		if (basebase->tp_as_buffer == NULL)
			basebase = NULL;
		COPYBUF(bf_getreadbuffer);
		COPYBUF(bf_getwritebuffer);
		COPYBUF(bf_getsegcount);
		COPYBUF(bf_getcharbuffer);
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
		COPYSLOT(tp_is_gc);
	}
}

static int add_operators(PyTypeObject *);
static int add_subclass(PyTypeObject *base, PyTypeObject *type);

int
PyType_Ready(PyTypeObject *type)
{
	PyObject *dict, *bases;
	PyTypeObject *base;
	int i, n;

	if (type->tp_flags & Py_TPFLAGS_READY) {
		assert(type->tp_dict != NULL);
		return 0;
	}
	assert((type->tp_flags & Py_TPFLAGS_READYING) == 0);

	type->tp_flags |= Py_TPFLAGS_READYING;

	/* Initialize tp_base (defaults to BaseObject unless that's us) */
	base = type->tp_base;
	if (base == NULL && type != &PyBaseObject_Type)
		base = type->tp_base = &PyBaseObject_Type;

	/* Initialize ob_type if NULL.  This means extensions that want to be
	   compilable separately on Windows can call PyType_Ready() instead of
	   initializing the ob_type field of their type objects. */
	if (type->ob_type == NULL)
		type->ob_type = base->ob_type;

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

	/* Initialize tp_dict */
	dict = type->tp_dict;
	if (dict == NULL) {
		dict = PyDict_New();
		if (dict == NULL)
			goto error;
		type->tp_dict = dict;
	}

	/* Add type-specific descriptors to tp_dict */
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

	/* Calculate method resolution order */
	if (mro_internal(type) < 0) {
		goto error;
	}

	/* Inherit special flags from dominant base */
	if (type->tp_base != NULL)
		inherit_special(type, type->tp_base);

	/* Initialize tp_dict properly */
	bases = type->tp_mro;
	assert(bases != NULL);
	assert(PyTuple_Check(bases));
	n = PyTuple_GET_SIZE(bases);
	for (i = 1; i < n; i++) {
		PyObject *b = PyTuple_GET_ITEM(bases, i);
		if (PyType_Check(b))
			inherit_slots(type, (PyTypeObject *)b);
	}

	/* if the type dictionary doesn't contain a __doc__, set it from
	   the tp_doc slot.
	 */
	if (PyDict_GetItemString(type->tp_dict, "__doc__") == NULL) {
		if (type->tp_doc != NULL) {
			PyObject *doc = PyString_FromString(type->tp_doc);
			PyDict_SetItemString(type->tp_dict, "__doc__", doc);
			Py_DECREF(doc);
		} else {
			PyDict_SetItemString(type->tp_dict,
					     "__doc__", Py_None);
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

	/* Link into each base class's list of subclasses */
	bases = type->tp_bases;
	n = PyTuple_GET_SIZE(bases);
	for (i = 0; i < n; i++) {
		PyObject *b = PyTuple_GET_ITEM(bases, i);
		if (PyType_Check(b) &&
		    add_subclass((PyTypeObject *)b, type) < 0)
			goto error;
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

static int
add_subclass(PyTypeObject *base, PyTypeObject *type)
{
	int i;
	PyObject *list, *ref, *new;

	list = base->tp_subclasses;
	if (list == NULL) {
		base->tp_subclasses = list = PyList_New(0);
		if (list == NULL)
			return -1;
	}
	assert(PyList_Check(list));
	new = PyWeakref_NewRef((PyObject *)type, NULL);
	i = PyList_GET_SIZE(list);
	while (--i >= 0) {
		ref = PyList_GET_ITEM(list, i);
		assert(PyWeakref_CheckRef(ref));
		if (PyWeakref_GET_OBJECT(ref) == Py_None)
			return PyList_SetItem(list, i, new);
	}
	i = PyList_Append(list, new);
	Py_DECREF(new);
	return i;
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
wrap_binaryfunc_l(PyObject *self, PyObject *args, void *wrapped)
{
	binaryfunc func = (binaryfunc)wrapped;
	PyObject *other;

	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;
	if (!(self->ob_type->tp_flags & Py_TPFLAGS_CHECKTYPES) &&
	    !PyType_IsSubtype(other->ob_type, self->ob_type)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return (*func)(self, other);
}

static PyObject *
wrap_binaryfunc_r(PyObject *self, PyObject *args, void *wrapped)
{
	binaryfunc func = (binaryfunc)wrapped;
	PyObject *other;

	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;
	if (!(self->ob_type->tp_flags & Py_TPFLAGS_CHECKTYPES) &&
	    !PyType_IsSubtype(other->ob_type, self->ob_type)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return (*func)(other, self);
}

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

static PyObject *
wrap_ternaryfunc_r(PyObject *self, PyObject *args, void *wrapped)
{
	ternaryfunc func = (ternaryfunc)wrapped;
	PyObject *other;
	PyObject *third = Py_None;

	/* Note: This wrapper only works for __pow__() */

	if (!PyArg_ParseTuple(args, "O|O", &other, &third))
		return NULL;
	return (*func)(other, self, third);
}

static PyObject *
wrap_unaryfunc(PyObject *self, PyObject *args, void *wrapped)
{
	unaryfunc func = (unaryfunc)wrapped;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return (*func)(self);
}

static PyObject *
wrap_intargfunc(PyObject *self, PyObject *args, void *wrapped)
{
	intargfunc func = (intargfunc)wrapped;
	int i;

	if (!PyArg_ParseTuple(args, "i", &i))
		return NULL;
	return (*func)(self, i);
}

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

	if (PyTuple_GET_SIZE(args) == 1) {
		arg = PyTuple_GET_ITEM(args, 0);
		i = getindex(self, arg);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		return (*func)(self, i);
	}
	PyArg_ParseTuple(args, "O", &arg);
	assert(PyErr_Occurred());
	return NULL;
}

static PyObject *
wrap_intintargfunc(PyObject *self, PyObject *args, void *wrapped)
{
	intintargfunc func = (intintargfunc)wrapped;
	int i, j;

	if (!PyArg_ParseTuple(args, "ii", &i, &j))
		return NULL;
	return (*func)(self, i, j);
}

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

static PyObject *
wrap_delslice(PyObject *self, PyObject *args, void *wrapped)
{
	intintobjargproc func = (intintobjargproc)wrapped;
	int i, j, res;

	if (!PyArg_ParseTuple(args, "ii", &i, &j))
		return NULL;
	res = (*func)(self, i, j, NULL);
	if (res == -1 && PyErr_Occurred())
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

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

static PyObject *
wrap_call(PyObject *self, PyObject *args, void *wrapped, PyObject *kwds)
{
	ternaryfunc func = (ternaryfunc)wrapped;

	return (*func)(self, args, kwds);
}

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

static PyObject *
wrap_descr_set(PyObject *self, PyObject *args, void *wrapped)
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

static PyObject *
wrap_descr_delete(PyObject *self, PyObject *args, void *wrapped)
{
	descrsetfunc func = (descrsetfunc)wrapped;
	PyObject *obj;
	int ret;

	if (!PyArg_ParseTuple(args, "O", &obj))
		return NULL;
	ret = (*func)(self, obj, NULL);
	if (ret < 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
wrap_init(PyObject *self, PyObject *args, void *wrapped, PyObject *kwds)
{
	initproc func = (initproc)wrapped;

	if (func(self, args, kwds) < 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

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
	   object.__new__(dict).  To do this, we check that the
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

	if (PyDict_GetItemString(type->tp_dict, "__new__") != NULL)
		return 0;
	func = PyCFunction_New(tp_new_methoddef, (PyObject *)type);
	if (func == NULL)
		return -1;
	return PyDict_SetItemString(type->tp_dict, "__new__", func);
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
	int do_other = self->ob_type != other->ob_type && \
	    other->ob_type->tp_as_number != NULL && \
	    other->ob_type->tp_as_number->SLOTNAME == TESTFUNC; \
	if (self->ob_type->tp_as_number != NULL && \
	    self->ob_type->tp_as_number->SLOTNAME == TESTFUNC) { \
		PyObject *r; \
		if (do_other && \
		    PyType_IsSubtype(other->ob_type, self->ob_type)) { \
			r = call_maybe( \
				other, ROPSTR, &rcache_str, "(O)", self); \
			if (r != Py_NotImplemented) \
				return r; \
			Py_DECREF(r); \
			do_other = 0; \
		} \
		r = call_maybe( \
			self, OPSTR, &cache_str, "(O)", other); \
		if (r != Py_NotImplemented || \
		    other->ob_type == self->ob_type) \
			return r; \
		Py_DECREF(r); \
	} \
	if (do_other) { \
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
	int len;

	if (res == NULL)
		return -1;
	len = (int)PyInt_AsLong(res);
	Py_DECREF(res);
	if (len == -1 && PyErr_Occurred())
		return -1;
	if (len < 0) {
		PyErr_SetString(PyExc_ValueError,
				"__len__() should return >= 0");
		return -1;
	}
	return len;
}

SLOT1(slot_sq_concat, "__add__", PyObject *, "O")
SLOT1(slot_sq_repeat, "__mul__", int, "i")

/* Super-optimized version of slot_sq_item.
   Other slots could do the same... */
static PyObject *
slot_sq_item(PyObject *self, int i)
{
	static PyObject *getitem_str;
	PyObject *func, *args = NULL, *ival = NULL, *retval = NULL;
	descrgetfunc f;

	if (getitem_str == NULL) {
		getitem_str = PyString_InternFromString("__getitem__");
		if (getitem_str == NULL)
			return NULL;
	}
	func = _PyType_Lookup(self->ob_type, getitem_str);
	if (func != NULL) {
		if ((f = func->ob_type->tp_descr_get) == NULL)
			Py_INCREF(func);
		else
			func = f(func, self, (PyObject *)(self->ob_type));
		ival = PyInt_FromLong(i);
		if (ival != NULL) {
			args = PyTuple_New(1);
			if (args != NULL) {
				PyTuple_SET_ITEM(args, 0, ival);
				retval = PyObject_Call(func, args, NULL);
				Py_XDECREF(args);
				Py_XDECREF(func);
				return retval;
			}
		}
	}
	else {
		PyErr_SetObject(PyExc_AttributeError, getitem_str);
	}
	Py_XDECREF(args);
	Py_XDECREF(ival);
	Py_XDECREF(func);
	return NULL;
}

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

	func = lookup_maybe(self, "__contains__", &contains_str);

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
	else if (PyErr_Occurred())
		return -1;
	else {
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

static PyObject *slot_nb_power(PyObject *, PyObject *, PyObject *);

SLOT1BINFULL(slot_nb_power_binary, slot_nb_power,
	     nb_power, "__pow__", "__rpow__")

static PyObject *
slot_nb_power(PyObject *self, PyObject *other, PyObject *modulus)
{
	static PyObject *pow_str;

	if (modulus == Py_None)
		return slot_nb_power_binary(self, other);
	/* Three-arg power doesn't use __rpow__.  But ternary_op
	   can call this when the second argument's type uses
	   slot_nb_power, so check before calling self.__pow__. */
	if (self->ob_type->tp_as_number != NULL &&
	    self->ob_type->tp_as_number->nb_power == slot_nb_power) {
		return call_method(self, "__pow__", &pow_str,
				   "(OO)", other, modulus);
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

SLOT0(slot_nb_negative, "__neg__")
SLOT0(slot_nb_positive, "__pos__")
SLOT0(slot_nb_absolute, "__abs__")

static int
slot_nb_nonzero(PyObject *self)
{
	PyObject *func, *res;
	static PyObject *nonzero_str, *len_str;

	func = lookup_maybe(self, "__nonzero__", &nonzero_str);
	if (func == NULL) {
		if (PyErr_Occurred())
			return -1;
		func = lookup_maybe(self, "__len__", &len_str);
		if (func == NULL) {
			if (PyErr_Occurred())
				return -1;
			else
				return 1;
		}
	}
	res = PyObject_CallObject(func, NULL);
	Py_DECREF(func);
	if (res == NULL)
		return -1;
	return PyObject_IsTrue(res);
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
		}
		else {
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
		Py_DECREF(func);
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

/* There are two slot dispatch functions for tp_getattro.

   - slot_tp_getattro() is used when __getattribute__ is overridden
     but no __getattr__ hook is present;

   - slot_tp_getattr_hook() is used when a __getattr__ hook is present.

   The code in update_one_slot() always installs slot_tp_getattr_hook(); this
   detects the absence of __getattr__ and then installs the simpler slot if
   necessary. */

static PyObject *
slot_tp_getattro(PyObject *self, PyObject *name)
{
	static PyObject *getattribute_str = NULL;
	return call_method(self, "__getattribute__", &getattribute_str,
			   "(O)", name);
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
	if (getattr == NULL) {
		/* No __getattr__ hook: use a simpler dispatcher */
		tp->tp_getattro = slot_tp_getattro;
		return slot_tp_getattro(self, name);
	}
	getattribute = _PyType_Lookup(tp, getattribute_str);
	if (getattribute == NULL ||
	    (getattribute->ob_type == &PyWrapperDescr_Type &&
	     ((PyWrapperDescrObject *)getattribute)->d_wrapped ==
	     (void *)PyObject_GenericGetAttr))
		res = PyObject_GenericGetAttr(self, name);
	else
		res = PyObject_CallFunction(getattribute, "OO", self, name);
	if (res == NULL && PyErr_ExceptionMatches(PyExc_AttributeError)) {
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
		PyErr_SetString(PyExc_TypeError,
				"iteration over non-sequence");
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
		res = call_method(self, "__delete__", &del_str,
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
	Py_DECREF(newargs);
	Py_DECREF(func);
	return x;
}


/* Table mapping __foo__ names to tp_foo offsets and slot_tp_foo wrapper
   functions.  The offsets here are relative to the 'etype' structure, which
   incorporates the additional structures used for numbers, sequences and
   mappings.  Note that multiple names may map to the same slot (e.g. __eq__,
   __ne__ etc. all map to tp_richcompare) and one name may map to multiple
   slots (e.g. __str__ affects tp_str as well as tp_repr). The table is
   terminated with an all-zero entry.  (This table is further initialized and
   sorted in init_slotdefs() below.) */

typedef struct wrapperbase slotdef;

#undef TPSLOT
#undef FLSLOT
#undef ETSLOT
#undef SQSLOT
#undef MPSLOT
#undef NBSLOT
#undef UNSLOT
#undef IBSLOT
#undef BINSLOT
#undef RBINSLOT

#define TPSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
	{NAME, offsetof(PyTypeObject, SLOT), (void *)(FUNCTION), WRAPPER, DOC}
#define FLSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC, FLAGS) \
	{NAME, offsetof(PyTypeObject, SLOT), (void *)(FUNCTION), WRAPPER, \
	 DOC, FLAGS}
#define ETSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
	{NAME, offsetof(etype, SLOT), (void *)(FUNCTION), WRAPPER, DOC}
#define SQSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
	ETSLOT(NAME, as_sequence.SLOT, FUNCTION, WRAPPER, DOC)
#define MPSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
	ETSLOT(NAME, as_mapping.SLOT, FUNCTION, WRAPPER, DOC)
#define NBSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
	ETSLOT(NAME, as_number.SLOT, FUNCTION, WRAPPER, DOC)
#define UNSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
	ETSLOT(NAME, as_number.SLOT, FUNCTION, WRAPPER, \
	       "x." NAME "() <==> " DOC)
#define IBSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
	ETSLOT(NAME, as_number.SLOT, FUNCTION, WRAPPER, \
	       "x." NAME "(y) <==> x" DOC "y")
#define BINSLOT(NAME, SLOT, FUNCTION, DOC) \
	ETSLOT(NAME, as_number.SLOT, FUNCTION, wrap_binaryfunc_l, \
	       "x." NAME "(y) <==> x" DOC "y")
#define RBINSLOT(NAME, SLOT, FUNCTION, DOC) \
	ETSLOT(NAME, as_number.SLOT, FUNCTION, wrap_binaryfunc_r, \
	       "x." NAME "(y) <==> y" DOC "x")

static slotdef slotdefs[] = {
	SQSLOT("__len__", sq_length, slot_sq_length, wrap_inquiry,
	       "x.__len__() <==> len(x)"),
	SQSLOT("__add__", sq_concat, slot_sq_concat, wrap_binaryfunc,
	       "x.__add__(y) <==> x+y"),
	SQSLOT("__mul__", sq_repeat, slot_sq_repeat, wrap_intargfunc,
	       "x.__mul__(n) <==> x*n"),
	SQSLOT("__rmul__", sq_repeat, slot_sq_repeat, wrap_intargfunc,
	       "x.__rmul__(n) <==> n*x"),
	SQSLOT("__getitem__", sq_item, slot_sq_item, wrap_sq_item,
	       "x.__getitem__(y) <==> x[y]"),
	SQSLOT("__getslice__", sq_slice, slot_sq_slice, wrap_intintargfunc,
	       "x.__getslice__(i, j) <==> x[i:j]"),
	SQSLOT("__setitem__", sq_ass_item, slot_sq_ass_item, wrap_sq_setitem,
	       "x.__setitem__(i, y) <==> x[i]=y"),
	SQSLOT("__delitem__", sq_ass_item, slot_sq_ass_item, wrap_sq_delitem,
	       "x.__delitem__(y) <==> del x[y]"),
	SQSLOT("__setslice__", sq_ass_slice, slot_sq_ass_slice,
	       wrap_intintobjargproc,
	       "x.__setslice__(i, j, y) <==> x[i:j]=y"),
	SQSLOT("__delslice__", sq_ass_slice, slot_sq_ass_slice, wrap_delslice,
	       "x.__delslice__(i, j) <==> del x[i:j]"),
	SQSLOT("__contains__", sq_contains, slot_sq_contains, wrap_objobjproc,
	       "x.__contains__(y) <==> y in x"),
	SQSLOT("__iadd__", sq_inplace_concat, slot_sq_inplace_concat,
	       wrap_binaryfunc, "x.__iadd__(y) <==> x+=y"),
	SQSLOT("__imul__", sq_inplace_repeat, slot_sq_inplace_repeat,
	       wrap_intargfunc, "x.__imul__(y) <==> x*=y"),

	MPSLOT("__len__", mp_length, slot_mp_length, wrap_inquiry,
	       "x.__len__() <==> len(x)"),
	MPSLOT("__getitem__", mp_subscript, slot_mp_subscript,
	       wrap_binaryfunc,
	       "x.__getitem__(y) <==> x[y]"),
	MPSLOT("__setitem__", mp_ass_subscript, slot_mp_ass_subscript,
	       wrap_objobjargproc,
	       "x.__setitem__(i, y) <==> x[i]=y"),
	MPSLOT("__delitem__", mp_ass_subscript, slot_mp_ass_subscript,
	       wrap_delitem,
	       "x.__delitem__(y) <==> del x[y]"),

	BINSLOT("__add__", nb_add, slot_nb_add,
		"+"),
	RBINSLOT("__radd__", nb_add, slot_nb_add,
		 "+"),
	BINSLOT("__sub__", nb_subtract, slot_nb_subtract,
		"-"),
	RBINSLOT("__rsub__", nb_subtract, slot_nb_subtract,
		 "-"),
	BINSLOT("__mul__", nb_multiply, slot_nb_multiply,
		"*"),
	RBINSLOT("__rmul__", nb_multiply, slot_nb_multiply,
		 "*"),
	BINSLOT("__div__", nb_divide, slot_nb_divide,
		"/"),
	RBINSLOT("__rdiv__", nb_divide, slot_nb_divide,
		 "/"),
	BINSLOT("__mod__", nb_remainder, slot_nb_remainder,
		"%"),
	RBINSLOT("__rmod__", nb_remainder, slot_nb_remainder,
		 "%"),
	BINSLOT("__divmod__", nb_divmod, slot_nb_divmod,
		"divmod(x, y)"),
	RBINSLOT("__rdivmod__", nb_divmod, slot_nb_divmod,
		 "divmod(y, x)"),
	NBSLOT("__pow__", nb_power, slot_nb_power, wrap_ternaryfunc,
	       "x.__pow__(y[, z]) <==> pow(x, y[, z])"),
	NBSLOT("__rpow__", nb_power, slot_nb_power, wrap_ternaryfunc_r,
	       "y.__rpow__(x[, z]) <==> pow(x, y[, z])"),
	UNSLOT("__neg__", nb_negative, slot_nb_negative, wrap_unaryfunc, "-x"),
	UNSLOT("__pos__", nb_positive, slot_nb_positive, wrap_unaryfunc, "+x"),
	UNSLOT("__abs__", nb_absolute, slot_nb_absolute, wrap_unaryfunc,
	       "abs(x)"),
	UNSLOT("__nonzero__", nb_nonzero, slot_nb_nonzero, wrap_inquiry,
	       "x != 0"),
	UNSLOT("__invert__", nb_invert, slot_nb_invert, wrap_unaryfunc, "~x"),
	BINSLOT("__lshift__", nb_lshift, slot_nb_lshift, "<<"),
	RBINSLOT("__rlshift__", nb_lshift, slot_nb_lshift, "<<"),
	BINSLOT("__rshift__", nb_rshift, slot_nb_rshift, ">>"),
	RBINSLOT("__rrshift__", nb_rshift, slot_nb_rshift, ">>"),
	BINSLOT("__and__", nb_and, slot_nb_and, "&"),
	RBINSLOT("__rand__", nb_and, slot_nb_and, "&"),
	BINSLOT("__xor__", nb_xor, slot_nb_xor, "^"),
	RBINSLOT("__rxor__", nb_xor, slot_nb_xor, "^"),
	BINSLOT("__or__", nb_or, slot_nb_or, "|"),
	RBINSLOT("__ror__", nb_or, slot_nb_or, "|"),
	NBSLOT("__coerce__", nb_coerce, slot_nb_coerce, wrap_coercefunc,
	       "x.__coerce__(y) <==> coerce(x, y)"),
	UNSLOT("__int__", nb_int, slot_nb_int, wrap_unaryfunc,
	       "int(x)"),
	UNSLOT("__long__", nb_long, slot_nb_long, wrap_unaryfunc,
	       "long(x)"),
	UNSLOT("__float__", nb_float, slot_nb_float, wrap_unaryfunc,
	       "float(x)"),
	UNSLOT("__oct__", nb_oct, slot_nb_oct, wrap_unaryfunc,
	       "oct(x)"),
	UNSLOT("__hex__", nb_hex, slot_nb_hex, wrap_unaryfunc,
	       "hex(x)"),
	IBSLOT("__iadd__", nb_inplace_add, slot_nb_inplace_add,
	       wrap_binaryfunc, "+"),
	IBSLOT("__isub__", nb_inplace_subtract, slot_nb_inplace_subtract,
	       wrap_binaryfunc, "-"),
	IBSLOT("__imul__", nb_inplace_multiply, slot_nb_inplace_multiply,
	       wrap_binaryfunc, "*"),
	IBSLOT("__idiv__", nb_inplace_divide, slot_nb_inplace_divide,
	       wrap_binaryfunc, "/"),
	IBSLOT("__imod__", nb_inplace_remainder, slot_nb_inplace_remainder,
	       wrap_binaryfunc, "%"),
	IBSLOT("__ipow__", nb_inplace_power, slot_nb_inplace_power,
	       wrap_ternaryfunc, "**"),
	IBSLOT("__ilshift__", nb_inplace_lshift, slot_nb_inplace_lshift,
	       wrap_binaryfunc, "<<"),
	IBSLOT("__irshift__", nb_inplace_rshift, slot_nb_inplace_rshift,
	       wrap_binaryfunc, ">>"),
	IBSLOT("__iand__", nb_inplace_and, slot_nb_inplace_and,
	       wrap_binaryfunc, "&"),
	IBSLOT("__ixor__", nb_inplace_xor, slot_nb_inplace_xor,
	       wrap_binaryfunc, "^"),
	IBSLOT("__ior__", nb_inplace_or, slot_nb_inplace_or,
	       wrap_binaryfunc, "|"),
	BINSLOT("__floordiv__", nb_floor_divide, slot_nb_floor_divide, "//"),
	RBINSLOT("__rfloordiv__", nb_floor_divide, slot_nb_floor_divide, "//"),
	BINSLOT("__truediv__", nb_true_divide, slot_nb_true_divide, "/"),
	RBINSLOT("__rtruediv__", nb_true_divide, slot_nb_true_divide, "/"),
	IBSLOT("__ifloordiv__", nb_inplace_floor_divide,
	       slot_nb_inplace_floor_divide, wrap_binaryfunc, "//"),
	IBSLOT("__itruediv__", nb_inplace_true_divide,
	       slot_nb_inplace_true_divide, wrap_binaryfunc, "/"),

	TPSLOT("__str__", tp_str, slot_tp_str, wrap_unaryfunc,
	       "x.__str__() <==> str(x)"),
	TPSLOT("__str__", tp_print, NULL, NULL, ""),
	TPSLOT("__repr__", tp_repr, slot_tp_repr, wrap_unaryfunc,
	       "x.__repr__() <==> repr(x)"),
	TPSLOT("__repr__", tp_print, NULL, NULL, ""),
	TPSLOT("__cmp__", tp_compare, _PyObject_SlotCompare, wrap_cmpfunc,
	       "x.__cmp__(y) <==> cmp(x,y)"),
	TPSLOT("__hash__", tp_hash, slot_tp_hash, wrap_hashfunc,
	       "x.__hash__() <==> hash(x)"),
	FLSLOT("__call__", tp_call, slot_tp_call, (wrapperfunc)wrap_call,
	       "x.__call__(...) <==> x(...)", PyWrapperFlag_KEYWORDS),
	TPSLOT("__getattribute__", tp_getattro, slot_tp_getattr_hook,
	       wrap_binaryfunc, "x.__getattribute__('name') <==> x.name"),
	TPSLOT("__getattribute__", tp_getattr, NULL, NULL, ""),
	TPSLOT("__getattr__", tp_getattro, slot_tp_getattr_hook, NULL, ""),
	TPSLOT("__getattr__", tp_getattr, NULL, NULL, ""),
	TPSLOT("__setattr__", tp_setattro, slot_tp_setattro, wrap_setattr,
	       "x.__setattr__('name', value) <==> x.name = value"),
	TPSLOT("__setattr__", tp_setattr, NULL, NULL, ""),
	TPSLOT("__delattr__", tp_setattro, slot_tp_setattro, wrap_delattr,
	       "x.__delattr__('name') <==> del x.name"),
	TPSLOT("__delattr__", tp_setattr, NULL, NULL, ""),
	TPSLOT("__lt__", tp_richcompare, slot_tp_richcompare, richcmp_lt,
	       "x.__lt__(y) <==> x<y"),
	TPSLOT("__le__", tp_richcompare, slot_tp_richcompare, richcmp_le,
	       "x.__le__(y) <==> x<=y"),
	TPSLOT("__eq__", tp_richcompare, slot_tp_richcompare, richcmp_eq,
	       "x.__eq__(y) <==> x==y"),
	TPSLOT("__ne__", tp_richcompare, slot_tp_richcompare, richcmp_ne,
	       "x.__ne__(y) <==> x!=y"),
	TPSLOT("__gt__", tp_richcompare, slot_tp_richcompare, richcmp_gt,
	       "x.__gt__(y) <==> x>y"),
	TPSLOT("__ge__", tp_richcompare, slot_tp_richcompare, richcmp_ge,
	       "x.__ge__(y) <==> x>=y"),
	TPSLOT("__iter__", tp_iter, slot_tp_iter, wrap_unaryfunc,
	       "x.__iter__() <==> iter(x)"),
	TPSLOT("next", tp_iternext, slot_tp_iternext, wrap_next,
	       "x.next() -> the next value, or raise StopIteration"),
	TPSLOT("__get__", tp_descr_get, slot_tp_descr_get, wrap_descr_get,
	       "descr.__get__(obj[, type]) -> value"),
	TPSLOT("__set__", tp_descr_set, slot_tp_descr_set, wrap_descr_set,
	       "descr.__set__(obj, value)"),
	TPSLOT("__delete__", tp_descr_set, slot_tp_descr_set,
	       wrap_descr_delete, "descr.__delete__(obj)"),
	FLSLOT("__init__", tp_init, slot_tp_init, (wrapperfunc)wrap_init,
	       "x.__init__(...) initializes x; "
	       "see x.__class__.__doc__ for signature",
	       PyWrapperFlag_KEYWORDS),
	TPSLOT("__new__", tp_new, slot_tp_new, NULL, ""),
	{NULL}
};

/* Given a type pointer and an offset gotten from a slotdef entry, return a
   pointer to the actual slot.  This is not quite the same as simply adding
   the offset to the type pointer, since it takes care to indirect through the
   proper indirection pointer (as_buffer, etc.); it returns NULL if the
   indirection pointer is NULL. */
static void **
slotptr(PyTypeObject *type, int offset)
{
	char *ptr;

	/* Note: this depends on the order of the members of etype! */
	assert(offset >= 0);
	assert(offset < offsetof(etype, as_buffer));
	if (offset >= offsetof(etype, as_sequence)) {
		ptr = (void *)type->tp_as_sequence;
		offset -= offsetof(etype, as_sequence);
	}
	else if (offset >= offsetof(etype, as_mapping)) {
		ptr = (void *)type->tp_as_mapping;
		offset -= offsetof(etype, as_mapping);
	}
	else if (offset >= offsetof(etype, as_number)) {
		ptr = (void *)type->tp_as_number;
		offset -= offsetof(etype, as_number);
	}
	else {
		ptr = (void *)type;
	}
	if (ptr != NULL)
		ptr += offset;
	return (void **)ptr;
}

/* Length of array of slotdef pointers used to store slots with the
   same __name__.  There should be at most MAX_EQUIV-1 slotdef entries with
   the same __name__, for any __name__. Since that's a static property, it is
   appropriate to declare fixed-size arrays for this. */
#define MAX_EQUIV 10

/* Return a slot pointer for a given name, but ONLY if the attribute has
   exactly one slot function.  The name must be an interned string. */
static void **
resolve_slotdups(PyTypeObject *type, PyObject *name)
{
	/* XXX Maybe this could be optimized more -- but is it worth it? */

	/* pname and ptrs act as a little cache */
	static PyObject *pname;
	static slotdef *ptrs[MAX_EQUIV];
	slotdef *p, **pp;
	void **res, **ptr;

	if (pname != name) {
		/* Collect all slotdefs that match name into ptrs. */
		pname = name;
		pp = ptrs;
		for (p = slotdefs; p->name_strobj; p++) {
			if (p->name_strobj == name)
				*pp++ = p;
		}
		*pp = NULL;
	}

	/* Look in all matching slots of the type; if exactly one of these has
	   a filled-in slot, return its value.  Otherwise return NULL. */
	res = NULL;
	for (pp = ptrs; *pp; pp++) {
		ptr = slotptr(type, (*pp)->offset);
		if (ptr == NULL || *ptr == NULL)
			continue;
		if (res != NULL)
			return NULL;
		res = ptr;
	}
	return res;
}

/* Common code for update_these_slots() and fixup_slot_dispatchers().  This
   does some incredibly complex thinking and then sticks something into the
   slot.  (It sees if the adjacent slotdefs for the same slot have conflicting
   interests, and then stores a generic wrapper or a specific function into
   the slot.)  Return a pointer to the next slotdef with a different offset,
   because that's convenient  for fixup_slot_dispatchers(). */
static slotdef *
update_one_slot(PyTypeObject *type, slotdef *p)
{
	PyObject *descr;
	PyWrapperDescrObject *d;
	void *generic = NULL, *specific = NULL;
	int use_generic = 0;
	int offset = p->offset;
	void **ptr = slotptr(type, offset);

	if (ptr == NULL) {
		do {
			++p;
		} while (p->offset == offset);
		return p;
	}
	do {
		descr = _PyType_Lookup(type, p->name_strobj);
		if (descr == NULL)
			continue;
		if (descr->ob_type == &PyWrapperDescr_Type) {
			void **tptr = resolve_slotdups(type, p->name_strobj);
			if (tptr == NULL || tptr == ptr)
				generic = p->function;
			d = (PyWrapperDescrObject *)descr;
			if (d->d_base->wrapper == p->wrapper &&
			    PyType_IsSubtype(type, d->d_type))
			{
				if (specific == NULL ||
				    specific == d->d_wrapped)
					specific = d->d_wrapped;
				else
					use_generic = 1;
			}
		}
		else {
			use_generic = 1;
			generic = p->function;
		}
	} while ((++p)->offset == offset);
	if (specific && !use_generic)
		*ptr = specific;
	else
		*ptr = generic;
	return p;
}

static int recurse_down_subclasses(PyTypeObject *type, slotdef **pp,
				   PyObject *name);

/* In the type, update the slots whose slotdefs are gathered in the pp0 array,
   and then do the same for all this type's subtypes. */
static int
update_these_slots(PyTypeObject *type, slotdef **pp0, PyObject *name)
{
	slotdef **pp;

	for (pp = pp0; *pp; pp++)
		update_one_slot(type, *pp);
	return recurse_down_subclasses(type, pp0, name);
}

/* Update the slots whose slotdefs are gathered in the pp array in all (direct
   or indirect) subclasses of type. */
static int
recurse_down_subclasses(PyTypeObject *type, slotdef **pp, PyObject *name)
{
	PyTypeObject *subclass;
	PyObject *ref, *subclasses, *dict;
	int i, n;

	subclasses = type->tp_subclasses;
	if (subclasses == NULL)
		return 0;
	assert(PyList_Check(subclasses));
	n = PyList_GET_SIZE(subclasses);
	for (i = 0; i < n; i++) {
		ref = PyList_GET_ITEM(subclasses, i);
		assert(PyWeakref_CheckRef(ref));
		subclass = (PyTypeObject *)PyWeakref_GET_OBJECT(ref);
		assert(subclass != NULL);
		if ((PyObject *)subclass == Py_None)
			continue;
		assert(PyType_Check(subclass));
		/* Avoid recursing down into unaffected classes */
		dict = subclass->tp_dict;
		if (dict != NULL && PyDict_Check(dict) &&
		    PyDict_GetItem(dict, name) != NULL)
			continue;
		if (update_these_slots(subclass, pp, name) < 0)
			return -1;
	}
	return 0;
}

/* Comparison function for qsort() to compare slotdefs by their offset, and
   for equal offset by their address (to force a stable sort). */
static int
slotdef_cmp(const void *aa, const void *bb)
{
	const slotdef *a = (const slotdef *)aa, *b = (const slotdef *)bb;
	int c = a->offset - b->offset;
	if (c != 0)
		return c;
	else
		return a - b;
}

/* Initialize the slotdefs table by adding interned string objects for the
   names and sorting the entries. */
static void
init_slotdefs(void)
{
	slotdef *p;
	static int initialized = 0;

	if (initialized)
		return;
	for (p = slotdefs; p->name; p++) {
		p->name_strobj = PyString_InternFromString(p->name);
		if (!p->name_strobj)
			Py_FatalError("Out of memory interning slotdef names");
	}
	qsort((void *)slotdefs, (size_t)(p-slotdefs), sizeof(slotdef),
	      slotdef_cmp);
	initialized = 1;
}

/* Update the slots after assignment to a class (type) attribute. */
static int
update_slot(PyTypeObject *type, PyObject *name)
{
	slotdef *ptrs[MAX_EQUIV];
	slotdef *p;
	slotdef **pp;
	int offset;

	init_slotdefs();
	pp = ptrs;
	for (p = slotdefs; p->name; p++) {
		/* XXX assume name is interned! */
		if (p->name_strobj == name)
			*pp++ = p;
	}
	*pp = NULL;
	for (pp = ptrs; *pp; pp++) {
		p = *pp;
		offset = p->offset;
		while (p > slotdefs && (p-1)->offset == offset)
			--p;
		*pp = p;
	}
	if (ptrs[0] == NULL)
		return 0; /* Not an attribute that affects any slots */
	return update_these_slots(type, ptrs, name);
}

/* Store the proper functions in the slot dispatches at class (type)
   definition time, based upon which operations the class overrides in its
   dict. */
static void
fixup_slot_dispatchers(PyTypeObject *type)
{
	slotdef *p;

	init_slotdefs();
	for (p = slotdefs; p->name; )
		p = update_one_slot(type, p);
}

/* This function is called by PyType_Ready() to populate the type's
   dictionary with method descriptors for function slots.  For each
   function slot (like tp_repr) that's defined in the type, one or more
   corresponding descriptors are added in the type's tp_dict dictionary
   under the appropriate name (like __repr__).  Some function slots
   cause more than one descriptor to be added (for example, the nb_add
   slot adds both __add__ and __radd__ descriptors) and some function
   slots compete for the same descriptor (for example both sq_item and
   mp_subscript generate a __getitem__ descriptor).

   In the latter case, the first slotdef entry encoutered wins.  Since
   slotdef entries are sorted by the offset of the slot in the etype
   struct, this gives us some control over disambiguating between
   competing slots: the members of struct etype are listed from most
   general to least general, so the most general slot is preferred.  In
   particular, because as_mapping comes before as_sequence, for a type
   that defines both mp_subscript and sq_item, mp_subscript wins.

   This only adds new descriptors and doesn't overwrite entries in
   tp_dict that were previously defined.  The descriptors contain a
   reference to the C function they must call, so that it's safe if they
   are copied into a subtype's __dict__ and the subtype has a different
   C function in its slot -- calling the method defined by the
   descriptor will call the C function that was used to create it,
   rather than the C function present in the slot when it is called.
   (This is important because a subtype may have a C function in the
   slot that calls the method from the dictionary, and we want to avoid
   infinite recursion here.) */

static int
add_operators(PyTypeObject *type)
{
	PyObject *dict = type->tp_dict;
	slotdef *p;
	PyObject *descr;
	void **ptr;

	init_slotdefs();
	for (p = slotdefs; p->name; p++) {
		if (p->wrapper == NULL)
			continue;
		ptr = slotptr(type, p->offset);
		if (!ptr || !*ptr)
			continue;
		if (PyDict_GetItem(dict, p->name_strobj))
			continue;
		descr = PyDescr_NewWrapper(type, p, *ptr);
		if (descr == NULL)
			return -1;
		if (PyDict_SetItem(dict, p->name_strobj, descr) < 0)
			return -1;
		Py_DECREF(descr);
	}
	if (type->tp_new != NULL) {
		if (add_tp_new_wrapper(type) < 0)
			return -1;
	}
	return 0;
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

	_PyObject_GC_UNTRACK(self);
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
		PyObject *mro, *res, *tmp, *dict;
		PyTypeObject *starttype;
		descrgetfunc f;
		int i, n;

		starttype = su->obj->ob_type;
		mro = starttype->tp_mro;

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
			starttype = (PyTypeObject *)(su->obj);
			mro = starttype->tp_mro;
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
			if (PyType_Check(tmp))
				dict = ((PyTypeObject *)tmp)->tp_dict;
			else if (PyClass_Check(tmp))
				dict = ((PyClassObject *)tmp)->cl_dict;
			else
				continue;
			res = PyDict_GetItem(dict, name);
			if (res != NULL  && !PyDescr_IsData(res)) {
				Py_INCREF(res);
				f = res->ob_type->tp_descr_get;
				if (f != NULL) {
					tmp = f(res, su->obj,
						(PyObject *)starttype);
					Py_DECREF(res);
					res = tmp;
				}
				return res;
			}
		}
	}
	return PyObject_GenericGetAttr(self, name);
}

static int
supercheck(PyTypeObject *type, PyObject *obj)
{
	if (!PyType_IsSubtype(obj->ob_type, type) &&
	    !(PyType_Check(obj) &&
	      PyType_IsSubtype((PyTypeObject *)obj, type))) {
		PyErr_SetString(PyExc_TypeError,
			"super(type, obj): "
			"obj must be an instance or subtype of type");
		return -1;
	}
	else
		return 0;
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
	if (su->ob_type != &PySuper_Type)
		/* If su is an instance of a subclass of super,
		   call its type */
		return PyObject_CallFunction((PyObject *)su->ob_type,
					     "OO", su->type, obj);
	else {
		/* Inline the common case */
		if (supercheck(su->type, obj) < 0)
			return NULL;
		new = (superobject *)PySuper_Type.tp_new(&PySuper_Type,
							 NULL, NULL);
		if (new == NULL)
			return NULL;
		Py_INCREF(su->type);
		Py_INCREF(obj);
		new->type = su->type;
		new->obj = obj;
		return (PyObject *)new;
	}
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
	if (obj != NULL && supercheck(type, obj) < 0)
		return -1;
	Py_INCREF(type);
	Py_XINCREF(obj);
	su->type = type;
	su->obj = obj;
	return 0;
}

PyDoc_STRVAR(super_doc,
"super(type) -> unbound super object\n"
"super(type, obj) -> bound super object; requires isinstance(obj, type)\n"
"super(type, type2) -> bound super object; requires issubclass(type2, type)\n"
"Typical use to call a cooperative superclass method:\n"
"class C(B):\n"
"    def meth(self, arg):\n"
"        super(C, self).meth(arg)");

static int
super_traverse(PyObject *self, visitproc visit, void *arg)
{
	superobject *su = (superobject *)self;
	int err;

#define VISIT(SLOT) \
	if (SLOT) { \
		err = visit((PyObject *)(SLOT), arg); \
		if (err) \
			return err; \
	}

	VISIT(su->obj);
	VISIT(su->type);

#undef VISIT

	return 0;
}

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
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,		/* tp_flags */
 	super_doc,				/* tp_doc */
 	super_traverse,				/* tp_traverse */
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
	PyObject_GC_Del,        		/* tp_free */
};
