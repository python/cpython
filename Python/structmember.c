
/* Map C struct members to Python object attributes */

#include "Python.h"

#include "structmember.h"

static PyObject *
listmembers(struct memberlist *mlist)
{
	int i, n;
	PyObject *v;
	for (n = 0; mlist[n].name != NULL; n++)
		;
	v = PyList_New(n);
	if (v != NULL) {
		for (i = 0; i < n; i++)
			PyList_SetItem(v, i,
				       PyString_FromString(mlist[i].name));
		if (PyErr_Occurred()) {
			Py_DECREF(v);
			v = NULL;
		}
		else {
			PyList_Sort(v);
		}
	}
	return v;
}

PyObject *
PyMember_Get(char *addr, struct memberlist *mlist, char *name)
{
	struct memberlist *l;

	if (strcmp(name, "__members__") == 0)
		return listmembers(mlist);
	for (l = mlist; l->name != NULL; l++) {
		if (strcmp(l->name, name) == 0) {
			PyMemberDef copy;
			copy.name = l->name;
			copy.type = l->type;
			copy.offset = l->offset;
			copy.flags = l->flags;
			copy.doc = NULL;
			return PyMember_GetOne(addr, &copy);
		}
	}
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
}

PyObject *
PyMember_GetOne(char *addr, PyMemberDef *l)
{
	PyObject *v;
	if ((l->flags & READ_RESTRICTED) &&
	    PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError, "restricted attribute");
		return NULL;
	}
	addr += l->offset;
	switch (l->type) {
	case T_BYTE:
		v = PyInt_FromLong(
			(long) (((*(char*)addr & 0xff) ^ 0x80) - 0x80));
		break;
	case T_UBYTE:
		v = PyInt_FromLong((long) *(char*)addr & 0xff);
		break;
	case T_SHORT:
		v = PyInt_FromLong((long) *(short*)addr);
		break;
	case T_USHORT:
		v = PyInt_FromLong((long) *(unsigned short*)addr);
		break;
	case T_INT:
		v = PyInt_FromLong((long) *(int*)addr);
		break;
	case T_UINT:
		v = PyInt_FromLong((long) *(unsigned int*)addr);
		break;
	case T_LONG:
		v = PyInt_FromLong(*(long*)addr);
		break;
	case T_ULONG:
		v = PyLong_FromDouble((double) *(unsigned long*)addr);
		break;
	case T_FLOAT:
		v = PyFloat_FromDouble((double)*(float*)addr);
		break;
	case T_DOUBLE:
		v = PyFloat_FromDouble(*(double*)addr);
		break;
	case T_STRING:
		if (*(char**)addr == NULL) {
			Py_INCREF(Py_None);
			v = Py_None;
		}
		else
			v = PyString_FromString(*(char**)addr);
		break;
	case T_STRING_INPLACE:
		v = PyString_FromString((char*)addr);
		break;
	case T_CHAR:
		v = PyString_FromStringAndSize((char*)addr, 1);
		break;
	case T_OBJECT:
		v = *(PyObject **)addr;
		if (v == NULL)
			v = Py_None;
		Py_INCREF(v);
		break;
	case T_OBJECT_EX:
		v = *(PyObject **)addr;
		if (v == NULL)
			PyErr_SetString(PyExc_AttributeError, l->name);
		Py_XINCREF(v);
		break;
	default:
		PyErr_SetString(PyExc_SystemError, "bad memberdescr type");
		v = NULL;
	}
	return v;
}

int
PyMember_Set(char *addr, struct memberlist *mlist, char *name, PyObject *v)
{
	struct memberlist *l;

	for (l = mlist; l->name != NULL; l++) {
		if (strcmp(l->name, name) == 0) {
			PyMemberDef copy;
			copy.name = l->name;
			copy.type = l->type;
			copy.offset = l->offset;
			copy.flags = l->flags;
			copy.doc = NULL;
			return PyMember_SetOne(addr, &copy, v);
		}
	}

	PyErr_SetString(PyExc_AttributeError, name);
	return -1;
}

int
PyMember_SetOne(char *addr, PyMemberDef *l, PyObject *v)
{
	PyObject *oldv;

	if ((l->flags & READONLY) || l->type == T_STRING)
	{
		PyErr_SetString(PyExc_TypeError, "readonly attribute");
		return -1;
	}
	if ((l->flags & WRITE_RESTRICTED) && PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError, "restricted attribute");
		return -1;
	}
	if (v == NULL && l->type != T_OBJECT_EX && l->type != T_OBJECT) {
		PyErr_SetString(PyExc_TypeError,
				"can't delete numeric/char attribute");
		return -1;
	}
	addr += l->offset;
	switch (l->type) {
	case T_BYTE:
	case T_UBYTE:
		if (!PyInt_Check(v)) {
			PyErr_BadArgument();
			return -1;
		}
		*(char*)addr = (char) PyInt_AsLong(v);
		break;
	case T_SHORT:
	case T_USHORT:
		if (!PyInt_Check(v)) {
			PyErr_BadArgument();
			return -1;
		}
		*(short*)addr = (short) PyInt_AsLong(v);
		break;
	case T_UINT:
	case T_INT:
		if (!PyInt_Check(v)) {
			PyErr_BadArgument();
			return -1;
		}
		*(int*)addr = (int) PyInt_AsLong(v);
		break;
	case T_LONG:
		if (!PyInt_Check(v)) {
			PyErr_BadArgument();
			return -1;
		}
		*(long*)addr = PyInt_AsLong(v);
		break;
	case T_ULONG:
		if (PyInt_Check(v))
			*(long*)addr = PyInt_AsLong(v);
		else if (PyLong_Check(v))
			*(long*)addr = PyLong_AsLong(v);
		else {
			PyErr_BadArgument();
			return -1;
		}
		break;
	case T_FLOAT:
		if (PyInt_Check(v))
			*(float*)addr =
				(float) PyInt_AsLong(v);
		else if (PyFloat_Check(v))
			*(float*)addr =
				(float) PyFloat_AsDouble(v);
		else {
			PyErr_BadArgument();
			return -1;
		}
		break;
	case T_DOUBLE:
		if (PyInt_Check(v))
			*(double*)addr = (double) PyInt_AsLong(v);
		else if (PyFloat_Check(v))
			*(double*)addr = PyFloat_AsDouble(v);
		else {
			PyErr_BadArgument();
			return -1;
		}
		break;
	case T_OBJECT:
	case T_OBJECT_EX:
		Py_XINCREF(v);
		oldv = *(PyObject **)addr;
		*(PyObject **)addr = v;
		Py_XDECREF(oldv);
		break;
	case T_CHAR:
		if (PyString_Check(v) && PyString_Size(v) == 1) {
			*(char*)addr = PyString_AsString(v)[0];
		}
		else {
			PyErr_BadArgument();
			return -1;
		}
		break;
	default:
		PyErr_Format(PyExc_SystemError,
			     "bad memberdescr type for %s", l->name);
		return -1;
	}
	return 0;
}
