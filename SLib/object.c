#include <Python.h> // TODO: Replace with appropriate INTERNAL ONLY header.

Py_SLIB_LOCAL(Py_ssize_t) PyObject_GetRefCount(const PyObject *ob) {
	return ob->ob_refcnt;
}

Py_SLIB_LOCAL(PyTypeObject *) PyObject_GetType(const PyObject *ob) {
	return ob->ob_type;
}

Py_SLIB_LOCAL(void) PyObject_SetRefCount(PyObject *ob, const Py_ssize_t refcnt) {
	ob->ob_refcnt = (Py_ssize_t)refcnt;
}

Py_SLIB_LOCAL(void) PyObject_SetType(PyObject *ob, const PyTypeObject *type) {
	ob->ob_type = (PyTypeObject *)type;
}


Py_SLIB_LOCAL(Py_ssize_t) PyVarObject_GetSize(const PyVarObject *ob) {
	return ob->ob_size;
}

Py_SLIB_LOCAL(void) PyVarObject_SetSize(PyVarObject *ob, const Py_ssize_t size) {
	ob->ob_size = (Py_ssize_t)size;
}


Py_SLIB_LOCAL(int) PyObject_IsType(const PyObject *ob, const PyTypeObject *type) {
	return Py_TYPE(ob) == type;
}


Py_SLIB_LOCAL(void *) PyObject_GetStructure(const PyObject *ob, const PyTypeObject *type)
{
	if (!PyType_HasFeature((PyTypeObject *)type, Py_TPFLAGS_OMIT_PYOBJECT_SIZE) ||
		type->tp_obj_offset == type->tp_obj_size) // This checks to see if tp_basicsize was 0 (IE: It has no internal structure)
		return (void *)ob;

	return (void *)(((unsigned char *)ob) + type->tp_obj_offset);
}


Py_SLIB_LOCAL(void) PyObject_IncRef(PyObject *ob)
{
#ifdef Py_REF_DEBUG
	++_Py_RefTotal;
#endif
	++ob->ob_refcnt;
}

Py_SLIB_LOCAL(void) PyObject_DecRef(
#ifdef Py_REF_DEBUG
	const char *filename, int lineno,
#endif
	PyObject *ob)
{
#ifdef Py_REF_DEBUG
	--_Py_RefTotal;
#endif
	if (--ob->ob_refcnt != 0) {
#ifdef Py_REF_DEBUG
		if (ob->ob_refcnt < 0) {
			_Py_NegativeRefcount(filename, lineno, ob);
		}
#endif
	}
	else {
		_Py_Dealloc(ob);
	}
}
