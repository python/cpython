#include "Python.h"
#include "structmember.h"

/* set object implementation 
   written and maintained by Raymond D. Hettinger <python@rcn.com>
   derived from sets.py written by Greg V. Wilson, Alex Martelli, 
   Guido van Rossum, Raymond Hettinger, and Tim Peters.

   Copyright (c) 2003 Python Software Foundation.
   All rights reserved.
*/

static PyObject *
set_update(PySetObject *so, PyObject *other)
{
	PyObject *item, *data, *it;

	if (PyAnySet_Check(other)) {
		if (PyDict_Merge(so->data, ((PySetObject *)other)->data, 1) == -1) 
			return NULL;
		Py_RETURN_NONE;
	}

	it = PyObject_GetIter(other);
	if (it == NULL)
		return NULL;
	data = so->data;

	while ((item = PyIter_Next(it)) != NULL) {
                if (PyDict_SetItem(data, item, Py_True) == -1) {
			Py_DECREF(it);
			Py_DECREF(item);
			return NULL;
                } 
		Py_DECREF(item);
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(update_doc, 
"Update a set with the union of itself and another.");

static PyObject *
make_new_set(PyTypeObject *type, PyObject *iterable)
{
	PyObject *data = NULL;
	PyObject *tmp;
	PySetObject *so = NULL;

	data = PyDict_New();
	if (data == NULL) 
		return NULL;

	/* create PySetObject structure */
	so = (PySetObject *)type->tp_alloc(type, 0);
	if (so == NULL) {
		Py_DECREF(data);
		return NULL;
	}
	so->data = data;
	so->hash = -1;
	so->weakreflist = NULL;

	if (iterable != NULL) {
		tmp = set_update(so, iterable);
		if (tmp == NULL) {
			Py_DECREF(so);
			return NULL;
		}
		Py_DECREF(tmp);
	}

	return (PyObject *)so;
}

static PyObject *
frozenset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *iterable = NULL;

	if (!PyArg_UnpackTuple(args, type->tp_name, 0, 1, &iterable))
		return NULL;
	if (iterable != NULL && PyFrozenSet_CheckExact(iterable)) {
		Py_INCREF(iterable);
		return iterable;
	}
	return make_new_set(type, iterable);
}

static PyObject *
set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	return make_new_set(type, NULL);
}

static PyObject *
frozenset_dict_wrapper(PyObject *d)
{
	PySetObject *w;

	assert(PyDict_Check(d));
	w = (PySetObject *)make_new_set(&PyFrozenSet_Type, NULL);
	if (w == NULL)
		return NULL;
	Py_CLEAR(w->data);
	Py_INCREF(d);
	w->data = d;
	return (PyObject *)w;
}

static void
set_dealloc(PySetObject *so)
{
	if (so->weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *) so);
	Py_XDECREF(so->data);
	so->ob_type->tp_free(so);
}

static PyObject *
set_iter(PySetObject *so)
{
	return PyObject_GetIter(so->data);
}

static int
set_len(PySetObject *so)
{
	return PyDict_Size(so->data);
}

static int
set_contains(PySetObject *so, PyObject *key)
{
	PyObject *tmp;
	int result;

	result = PyDict_Contains(so->data, key);
	if (result == -1 && PyAnySet_Check(key)) {
		PyErr_Clear();
		tmp = frozenset_dict_wrapper(((PySetObject *)(key))->data);
		if (tmp == NULL)
			return -1;
		result = PyDict_Contains(so->data, tmp);
		Py_DECREF(tmp);
	}
	return result;
}

static PyObject *
set_direct_contains(PySetObject *so, PyObject *key)
{
	long result;

	result = set_contains(so, key);
	if (result == -1)
		return NULL;
	return PyBool_FromLong(result);
}

PyDoc_STRVAR(contains_doc, "x.__contains__(y) <==> y in x.");

static PyObject *
set_copy(PySetObject *so)
{
	return make_new_set(so->ob_type, (PyObject *)so);
}

static PyObject *
frozenset_copy(PySetObject *so)
{
	if (PyFrozenSet_CheckExact(so)) {
		Py_INCREF(so);
		return (PyObject *)so;
	}
	return set_copy(so);
}

PyDoc_STRVAR(copy_doc, "Return a shallow copy of a set.");

static PyObject *
set_union(PySetObject *so, PyObject *other)
{
	PySetObject *result;
	PyObject *rv;

	result = (PySetObject *)set_copy(so);
	if (result == NULL)
		return NULL;
	rv = set_update(result, other);
	if (rv == NULL) {
		Py_DECREF(result);
		return NULL;
	}
	Py_DECREF(rv);
	return (PyObject *)result;
}

PyDoc_STRVAR(union_doc,
 "Return the union of two sets as a new set.\n\
\n\
(i.e. all elements that are in either set.)");

static PyObject *
set_or(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(so) || !PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return set_union(so, other);
}

static PyObject *
set_ior(PySetObject *so, PyObject *other)
{
	PyObject *result;

	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	result = set_update(so, other);
	if (result == NULL)
		return NULL;
	Py_DECREF(result);
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_intersection(PySetObject *so, PyObject *other)
{
	PySetObject *result;
	PyObject *item, *selfdata, *tgtdata, *it, *tmp;

	result = (PySetObject *)make_new_set(so->ob_type, NULL);
	if (result == NULL)
		return NULL;
	tgtdata = result->data;
	selfdata = so->data;

	if (PyAnySet_Check(other))
		other = ((PySetObject *)other)->data;

	if (PyDict_Check(other) && PyDict_Size(other) > PyDict_Size(selfdata)) {
		tmp = selfdata;
		selfdata = other;
		other = tmp;
	}

	if (PyDict_CheckExact(other)) {
		PyObject *value;
		int pos = 0;
		while (PyDict_Next(other, &pos, &item, &value)) {
			if (PyDict_Contains(selfdata, item)) {
				if (PyDict_SetItem(tgtdata, item, Py_True) == -1) {
					Py_DECREF(result);
					return NULL;
				}
			}
		}
		return (PyObject *)result;
	}

	it = PyObject_GetIter(other);
	if (it == NULL) {
		Py_DECREF(result);
		return NULL;
	}

	while ((item = PyIter_Next(it)) != NULL) {
		if (PyDict_Contains(selfdata, item)) {
			if (PyDict_SetItem(tgtdata, item, Py_True) == -1) {
				Py_DECREF(it);
				Py_DECREF(result);
				Py_DECREF(item);
				return NULL;
			}
		}
		Py_DECREF(item);
	}
	Py_DECREF(it);
	if (PyErr_Occurred()) {
		Py_DECREF(result);
		return NULL;
	}
	return (PyObject *)result;
}

PyDoc_STRVAR(intersection_doc,
"Return the intersection of two sets as a new set.\n\
\n\
(i.e. all elements that are in both sets.)");

static PyObject *
set_intersection_update(PySetObject *so, PyObject *other)
{
	PyObject *item, *selfdata, *it, *newdict, *tmp;

	newdict = PyDict_New();
	if (newdict == NULL)
		return newdict;

	it = PyObject_GetIter(other);
	if (it == NULL) {
		Py_DECREF(newdict);
		return NULL;
	}

	selfdata = so->data;
	while ((item = PyIter_Next(it)) != NULL) {
		if (PyDict_Contains(selfdata, item)) {
			if (PyDict_SetItem(newdict, item, Py_True) == -1) {
				Py_DECREF(newdict);
				Py_DECREF(it);
				Py_DECREF(item);
				return NULL;
			}
		}
		Py_DECREF(item);
	}
	Py_DECREF(it);
	if (PyErr_Occurred()) {
		Py_DECREF(newdict);
		return NULL;
	}
	tmp = so->data;
	so->data = newdict;
	Py_DECREF(tmp);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(intersection_update_doc,
"Update a set with the intersection of itself and another.");

static PyObject *
set_and(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(so) || !PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return set_intersection(so, other);
}

static PyObject *
set_iand(PySetObject *so, PyObject *other)
{
	PyObject *result;

	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	result = set_intersection_update(so, other);
	if (result == NULL)
		return NULL;
	Py_DECREF(result);
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_difference_update(PySetObject *so, PyObject *other)
{
	PyObject *item, *tgtdata, *it;
	
	it = PyObject_GetIter(other);
	if (it == NULL)
		return NULL;

	tgtdata = so->data;
	while ((item = PyIter_Next(it)) != NULL) {
		if (PyDict_DelItem(tgtdata, item) == -1) {
			if (PyErr_ExceptionMatches(PyExc_KeyError))
				PyErr_Clear();
			else {
				Py_DECREF(it);
				Py_DECREF(item);
				return NULL;
			}
		}
		Py_DECREF(item);
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(difference_update_doc,
"Remove all elements of another set from this set.");

static PyObject *
set_difference(PySetObject *so, PyObject *other)
{
	PyObject *result, *tmp;
	PyObject *otherdata, *tgtdata;
	PyObject *key, *value;
	int pos = 0;

	if (PyDict_Check(other))
		otherdata = other;
	else if (PyAnySet_Check(other))
		otherdata = ((PySetObject *)other)->data;
	else {
		result = set_copy(so);
		if (result == NULL)
			return result;
		tmp = set_difference_update((PySetObject *)result, other);
		if (tmp != NULL) {
			Py_DECREF(tmp);
			return result;
		}
		Py_DECREF(result);
		return NULL;
	}
	
	result = make_new_set(so->ob_type, NULL);
	if (result == NULL)
		return NULL;
	tgtdata = ((PySetObject *)result)->data;

	while (PyDict_Next(so->data, &pos, &key, &value)) {
		if (!PyDict_Contains(otherdata, key)) {
			if (PyDict_SetItem(tgtdata, key, Py_True) == -1)
				return NULL;
		}
	}
	return result;
}

PyDoc_STRVAR(difference_doc,
"Return the difference of two sets as a new set.\n\
\n\
(i.e. all elements that are in this set but not the other.)");
static PyObject *
set_sub(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(so) || !PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return set_difference(so, other);
}

static PyObject *
set_isub(PySetObject *so, PyObject *other)
{
	PyObject *result;

	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	result = set_difference_update(so, other);
	if (result == NULL)
		return NULL;
	Py_DECREF(result);
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_symmetric_difference_update(PySetObject *so, PyObject *other)
{
	PyObject *selfdata, *otherdata;
	PySetObject *otherset = NULL;
	PyObject *key, *value;
	int pos = 0;

	selfdata = so->data;
	if (PyDict_Check(other))
		otherdata = other;
	else if (PyAnySet_Check(other))
		otherdata = ((PySetObject *)other)->data;
	else {
		otherset = (PySetObject *)make_new_set(so->ob_type, other);
		if (otherset == NULL)
			return NULL;
		otherdata = otherset->data;
	}

	while (PyDict_Next(otherdata, &pos, &key, &value)) {
		if (PyDict_Contains(selfdata, key)) {
			if (PyDict_DelItem(selfdata, key) == -1) {
				Py_XDECREF(otherset);
				return NULL;
			}
		} else {
			if (PyDict_SetItem(selfdata, key, Py_True) == -1) {
				Py_XDECREF(otherset);
				return NULL;
			}
		}
	}
	Py_XDECREF(otherset);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(symmetric_difference_update_doc,
"Update a set with the symmetric difference of itself and another.");

static PyObject *
set_symmetric_difference(PySetObject *so, PyObject *other)
{
	PySetObject *result;
	PyObject *selfdata, *otherdata, *tgtdata, *rv, *otherset;
	PyObject *key, *value;
	int pos = 0;

	if (PyDict_Check(other))
		otherdata = other;
	else if (PyAnySet_Check(other))
		otherdata = ((PySetObject *)other)->data;
	else {
		otherset = make_new_set(so->ob_type, other);
		if (otherset == NULL)
			return NULL;
		rv = set_symmetric_difference_update((PySetObject *)otherset, (PyObject *)so);
		if (rv == NULL)
			return NULL;
		Py_DECREF(rv);
		return otherset;
	}	

	result = (PySetObject *)make_new_set(so->ob_type, NULL);
	if (result == NULL)
		return NULL;
	tgtdata = result->data;
	selfdata = so->data;

	while (PyDict_Next(otherdata, &pos, &key, &value)) {
		if (!PyDict_Contains(selfdata, key)) {
			if (PyDict_SetItem(tgtdata, key, Py_True) == -1) {
				Py_DECREF(result);
				return NULL;
			}
		}
	}

	pos = 0;
	while (PyDict_Next(selfdata, &pos, &key, &value)) {
		if (!PyDict_Contains(otherdata, key)) {
			if (PyDict_SetItem(tgtdata, key, Py_True) == -1) {
				Py_DECREF(result);
				return NULL;
			}
		}
	}

	return (PyObject *)result;
}

PyDoc_STRVAR(symmetric_difference_doc,
"Return the symmetric difference of two sets as a new set.\n\
\n\
(i.e. all elements that are in exactly one of the sets.)");

static PyObject *
set_xor(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(so) || !PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return set_symmetric_difference(so, other);
}

static PyObject *
set_ixor(PySetObject *so, PyObject *other)
{
	PyObject *result;

	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	result = set_symmetric_difference_update(so, other);
	if (result == NULL)
		return NULL;
	Py_DECREF(result);
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_issubset(PySetObject *so, PyObject *other)
{
	PyObject *otherdata, *tmp, *result;
	PyObject *key, *value;
	int pos = 0;

	if (!PyAnySet_Check(other)) {
		tmp = make_new_set(&PySet_Type, other);
		if (tmp == NULL)
			return NULL;
		result = set_issubset(so, tmp);
		Py_DECREF(tmp);
		return result;
	}
	if (set_len(so) > set_len((PySetObject *)other)) 
		Py_RETURN_FALSE;

	otherdata = ((PySetObject *)other)->data;
	while (PyDict_Next(((PySetObject *)so)->data, &pos, &key, &value)) {
		if (!PyDict_Contains(otherdata, key))
			Py_RETURN_FALSE;
	}
	Py_RETURN_TRUE;
}

PyDoc_STRVAR(issubset_doc, "Report whether another set contains this set.");

static PyObject *
set_issuperset(PySetObject *so, PyObject *other)
{
	PyObject *tmp, *result;

	if (!PyAnySet_Check(other)) {
		tmp = make_new_set(&PySet_Type, other);
		if (tmp == NULL)
			return NULL;
		result = set_issuperset(so, tmp);
		Py_DECREF(tmp);
		return result;
	}
	return set_issubset((PySetObject *)other, (PyObject *)so);
}

PyDoc_STRVAR(issuperset_doc, "Report whether this set contains another set.");

static long
set_nohash(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "set objects are unhashable");
	return -1;
}

static int
set_nocmp(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "cannot compare sets using cmp()");
	return -1;
}

static long
frozenset_hash(PyObject *self)
{
	PySetObject *so = (PySetObject *)self;
	PyObject *key, *value;
	int pos = 0;
	long hash = 1927868237L;

	if (so->hash != -1)
		return so->hash;

	hash *= (PyDict_Size(so->data) + 1);
	while (PyDict_Next(so->data, &pos, &key, &value)) {
		/* Work to increase the bit dispersion for closely spaced hash
		   values.  The is important because some use cases have many 
		   combinations of a small number of elements with nearby 
		   hashes so that many distinct combinations collapse to only 
		   a handful of distinct hash values. */
		long h = PyObject_Hash(key);
		hash ^= (h ^ (h << 16) ^ 89869747L)  * 3644798167u;
	}
	hash = hash * 69069L + 907133923L;
	if (hash == -1)
		hash = 590923713L;
	so->hash = hash;
	return hash;
}

static PyObject *
set_richcompare(PySetObject *v, PyObject *w, int op)
{
	if(!PyAnySet_Check(w)) {
		if (op == Py_EQ)
			Py_RETURN_FALSE;
		if (op == Py_NE)
			Py_RETURN_TRUE;
		PyErr_SetString(PyExc_TypeError, "can only compare to a set");
		return NULL;
	}
	switch (op) {
	case Py_EQ:
	case Py_NE:
		return PyObject_RichCompare(((PySetObject *)v)->data, 
			((PySetObject *)w)->data, op);
	case Py_LE:
		return set_issubset((PySetObject *)v, w);
	case Py_GE:
		return set_issuperset((PySetObject *)v, w);
	case Py_LT:
		if (set_len(v) >= set_len((PySetObject *)w))
			Py_RETURN_FALSE;		
		return set_issubset((PySetObject *)v, w);
	case Py_GT:
		if (set_len(v) <= set_len((PySetObject *)w))
			Py_RETURN_FALSE;
		return set_issuperset((PySetObject *)v, w);
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static PyObject *
set_repr(PySetObject *so)
{
	PyObject *keys, *result, *listrepr;

	keys = PyDict_Keys(so->data);
	if (keys == NULL)
		return NULL;
	listrepr = PyObject_Repr(keys);
	Py_DECREF(keys);
	if (listrepr == NULL)
		return NULL;

	result = PyString_FromFormat("%s(%s)", so->ob_type->tp_name,
		PyString_AS_STRING(listrepr));
	Py_DECREF(listrepr);
	return result;
}

static int
set_tp_print(PySetObject *so, FILE *fp, int flags)
{
	PyObject *key, *value;
	int pos=0;
	char *emit = "";	/* No separator emitted on first pass */
	char *separator = ", ";

	fprintf(fp, "%s([", so->ob_type->tp_name);
	while (PyDict_Next(so->data, &pos, &key, &value)) {
		fputs(emit, fp);
		emit = separator;
		if (PyObject_Print(key, fp, 0) != 0)
			return -1;
	}
	fputs("])", fp);
	return 0;
}

static PyObject *
set_clear(PySetObject *so)
{
	PyDict_Clear(so->data);
	so->hash = -1;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "Remove all elements from this set.");

static PyObject *
set_add(PySetObject *so, PyObject *item)
{
	if (PyDict_SetItem(so->data, item, Py_True) == -1)
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(add_doc, 
"Add an element to a set.\n\
\n\
This has no effect if the element is already present.");

static PyObject *
set_remove(PySetObject *so, PyObject *item)
{
	PyObject *tmp, *result;

	if (PyType_IsSubtype(item->ob_type, &PySet_Type)) {
		tmp = frozenset_dict_wrapper(((PySetObject *)(item))->data);
		if (tmp == NULL)
			return NULL;
		result = set_remove(so, tmp);
		Py_DECREF(tmp);
		return result;
	}

	if (PyDict_DelItem(so->data, item) == -1) 
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(remove_doc,
"Remove an element from a set; it must be a member.\n\
\n\
If the element is not a member, raise a KeyError.");

static PyObject *
set_discard(PySetObject *so, PyObject *item)
{
	PyObject *tmp, *result;

	if (PyType_IsSubtype(item->ob_type, &PySet_Type)) {
		tmp = frozenset_dict_wrapper(((PySetObject *)(item))->data);
		if (tmp == NULL)
			return NULL;
		result = set_discard(so, tmp);
		Py_DECREF(tmp);
		return result;
	}

	if (PyDict_DelItem(so->data, item) == -1) {
		if (!PyErr_ExceptionMatches(PyExc_KeyError))
			return NULL;
		PyErr_Clear();
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(discard_doc,
"Remove an element from a set if it is a member.\n\
\n\
If the element is not a member, do nothing."); 

static PyObject *
set_pop(PySetObject *so)
{
	PyObject *key, *value;
	int pos = 0;

	if (!PyDict_Next(so->data, &pos, &key, &value)) {
		PyErr_SetString(PyExc_KeyError, "pop from an empty set");
		return NULL;
	}
	Py_INCREF(key);
	if (PyDict_DelItem(so->data, key) == -1) {
		Py_DECREF(key);
		return NULL;
	}
	return key;
}

PyDoc_STRVAR(pop_doc, "Remove and return an arbitrary set element.");

static PyObject *
set_reduce(PySetObject *so)
{
	PyObject *keys=NULL, *args=NULL, *result=NULL, *dict=NULL;

	keys = PyDict_Keys(so->data);
	if (keys == NULL)
		goto done;
	args = PyTuple_Pack(1, keys);
	if (args == NULL)
		goto done;
	dict = PyObject_GetAttrString((PyObject *)so, "__dict__");
	if (dict == NULL) {
		PyErr_Clear();
		dict = Py_None;
		Py_INCREF(dict);
	}
	result = PyTuple_Pack(3, so->ob_type, args, dict);
done:
	Py_XDECREF(args);
	Py_XDECREF(keys);
	Py_XDECREF(dict);
	return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static int
set_init(PySetObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *iterable = NULL;
	PyObject *result;

	if (!PyAnySet_Check(self))
		return -1;
	if (!PyArg_UnpackTuple(args, self->ob_type->tp_name, 0, 1, &iterable))
		return -1;
	PyDict_Clear(self->data);
	self->hash = -1;
	if (iterable == NULL)
		return 0;
	result = set_update(self, iterable);
	if (result != NULL) {
		Py_DECREF(result);
		return 0;
	}
	return -1;
}

static PySequenceMethods set_as_sequence = {
	(inquiry)set_len,		/* sq_length */
	0,				/* sq_concat */
	0,				/* sq_repeat */
	0,				/* sq_item */
	0,				/* sq_slice */
	0,				/* sq_ass_item */
	0,				/* sq_ass_slice */
	(objobjproc)set_contains,	/* sq_contains */
};

/* set object ********************************************************/

static PyMethodDef set_methods[] = {
	{"add",		(PyCFunction)set_add,		METH_O,
	 add_doc},
	{"clear",	(PyCFunction)set_clear,		METH_NOARGS,
	 clear_doc},
	{"__contains__",(PyCFunction)set_direct_contains,	METH_O | METH_COEXIST,
	 contains_doc},
	{"copy",	(PyCFunction)set_copy,		METH_NOARGS,
	 copy_doc},
	{"discard",	(PyCFunction)set_discard,	METH_O,
	 discard_doc},
	{"difference",	(PyCFunction)set_difference,	METH_O,
	 difference_doc},
	{"difference_update",	(PyCFunction)set_difference_update,	METH_O,
	 difference_update_doc},
	{"intersection",(PyCFunction)set_intersection,	METH_O,
	 intersection_doc},
	{"intersection_update",(PyCFunction)set_intersection_update,	METH_O,
	 intersection_update_doc},
	{"issubset",	(PyCFunction)set_issubset,	METH_O,
	 issubset_doc},
	{"issuperset",	(PyCFunction)set_issuperset,	METH_O,
	 issuperset_doc},
	{"pop",		(PyCFunction)set_pop,		METH_NOARGS,
	 pop_doc},
	{"__reduce__",	(PyCFunction)set_reduce,	METH_NOARGS,
	 reduce_doc},
	{"remove",	(PyCFunction)set_remove,	METH_O,
	 remove_doc},
	{"symmetric_difference",(PyCFunction)set_symmetric_difference,	METH_O,
	 symmetric_difference_doc},
	{"symmetric_difference_update",(PyCFunction)set_symmetric_difference_update,	METH_O,
	 symmetric_difference_update_doc},
	{"union",	(PyCFunction)set_union,		METH_O,
	 union_doc},
	{"update",	(PyCFunction)set_update,	METH_O,
	 update_doc},
	{NULL,		NULL}	/* sentinel */
};

static PyNumberMethods set_as_number = {
	0,				/*nb_add*/
	(binaryfunc)set_sub,		/*nb_subtract*/
	0,				/*nb_multiply*/
	0,				/*nb_divide*/
	0,				/*nb_remainder*/
	0,				/*nb_divmod*/
	0,				/*nb_power*/
	0,				/*nb_negative*/
	0,				/*nb_positive*/
	0,				/*nb_absolute*/
	0,				/*nb_nonzero*/
	0,				/*nb_invert*/
	0,				/*nb_lshift*/
	0,				/*nb_rshift*/
	(binaryfunc)set_and,		/*nb_and*/
	(binaryfunc)set_xor,		/*nb_xor*/
	(binaryfunc)set_or,		/*nb_or*/
	0,				/*nb_coerce*/
	0,				/*nb_int*/
	0,				/*nb_long*/
	0,				/*nb_float*/
	0,				/*nb_oct*/
	0, 				/*nb_hex*/
	0,				/*nb_inplace_add*/
	(binaryfunc)set_isub,		/*nb_inplace_subtract*/
	0,				/*nb_inplace_multiply*/
	0,				/*nb_inplace_divide*/
	0,				/*nb_inplace_remainder*/
	0,				/*nb_inplace_power*/
	0,				/*nb_inplace_lshift*/
	0,				/*nb_inplace_rshift*/
	(binaryfunc)set_iand,		/*nb_inplace_and*/
	(binaryfunc)set_ixor,		/*nb_inplace_xor*/
	(binaryfunc)set_ior,		/*nb_inplace_or*/
};

PyDoc_STRVAR(set_doc,
"set(iterable) --> set object\n\
\n\
Build an unordered collection.");

PyTypeObject PySet_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/* ob_size */
	"set",				/* tp_name */
	sizeof(PySetObject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)set_dealloc,	/* tp_dealloc */
	(printfunc)set_tp_print,	/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	(cmpfunc)set_nocmp,		/* tp_compare */
	(reprfunc)set_repr,		/* tp_repr */
	&set_as_number,			/* tp_as_number */
	&set_as_sequence,		/* tp_as_sequence */
	0,				/* tp_as_mapping */
	set_nohash,			/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_WEAKREFS,	/* tp_flags */
	set_doc,			/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	(richcmpfunc)set_richcompare,	/* tp_richcompare */
	offsetof(PySetObject, weakreflist),	/* tp_weaklistoffset */
	(getiterfunc)set_iter,		/* tp_iter */
	0,				/* tp_iternext */
	set_methods,			/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	(initproc)set_init,		/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	set_new,			/* tp_new */
	PyObject_Del,			/* tp_free */
};

/* frozenset object ********************************************************/


static PyMethodDef frozenset_methods[] = {
	{"__contains__",(PyCFunction)set_direct_contains,	METH_O | METH_COEXIST,
	 contains_doc},
	{"copy",	(PyCFunction)frozenset_copy,	METH_NOARGS,
	 copy_doc},
	{"difference",	(PyCFunction)set_difference,	METH_O,
	 difference_doc},
	{"intersection",(PyCFunction)set_intersection,	METH_O,
	 intersection_doc},
	{"issubset",	(PyCFunction)set_issubset,	METH_O,
	 issubset_doc},
	{"issuperset",	(PyCFunction)set_issuperset,	METH_O,
	 issuperset_doc},
	{"__reduce__",	(PyCFunction)set_reduce,	METH_NOARGS,
	 reduce_doc},
	{"symmetric_difference",(PyCFunction)set_symmetric_difference,	METH_O,
	 symmetric_difference_doc},
	{"union",	(PyCFunction)set_union,		METH_O,
	 union_doc},
	{NULL,		NULL}	/* sentinel */
};

static PyNumberMethods frozenset_as_number = {
	0,				/*nb_add*/
	(binaryfunc)set_sub,		/*nb_subtract*/
	0,				/*nb_multiply*/
	0,				/*nb_divide*/
	0,				/*nb_remainder*/
	0,				/*nb_divmod*/
	0,				/*nb_power*/
	0,				/*nb_negative*/
	0,				/*nb_positive*/
	0,				/*nb_absolute*/
	0,				/*nb_nonzero*/
	0,				/*nb_invert*/
	0,				/*nb_lshift*/
	0,				/*nb_rshift*/
	(binaryfunc)set_and,		/*nb_and*/
	(binaryfunc)set_xor,		/*nb_xor*/
	(binaryfunc)set_or,		/*nb_or*/
};

PyDoc_STRVAR(frozenset_doc,
"frozenset(iterable) --> frozenset object\n\
\n\
Build an immutable unordered collection.");

PyTypeObject PyFrozenSet_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/* ob_size */
	"frozenset",			/* tp_name */
	sizeof(PySetObject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)set_dealloc,	/* tp_dealloc */
	(printfunc)set_tp_print,	/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	(cmpfunc)set_nocmp,		/* tp_compare */
	(reprfunc)set_repr,		/* tp_repr */
	&frozenset_as_number,		/* tp_as_number */
	&set_as_sequence,		/* tp_as_sequence */
	0,				/* tp_as_mapping */
	frozenset_hash,			/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_WEAKREFS,	/* tp_flags */
	frozenset_doc,			/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	(richcmpfunc)set_richcompare,	/* tp_richcompare */
	offsetof(PySetObject, weakreflist),	/* tp_weaklistoffset */
	(getiterfunc)set_iter,		/* tp_iter */
	0,				/* tp_iternext */
	frozenset_methods,		/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	frozenset_new,			/* tp_new */
	PyObject_Del,			/* tp_free */
};
