
/* dl module */

#include "Python.h"

#include <dlfcn.h>

#ifndef RTLD_LAZY
#define RTLD_LAZY 1
#endif

typedef void *PyUnivPtr;
typedef struct {
	PyObject_HEAD
	PyUnivPtr *dl_handle;
} dlobject;

staticforward PyTypeObject Dltype;

static PyObject *Dlerror;

static PyObject *
newdlobject(PyUnivPtr *handle)
{
	dlobject *xp;
	xp = PyObject_New(dlobject, &Dltype);
	if (xp == NULL)
		return NULL;
	xp->dl_handle = handle;
	return (PyObject *)xp;
}

static void
dl_dealloc(dlobject *xp)
{
	if (xp->dl_handle != NULL)
		dlclose(xp->dl_handle);
	PyObject_Del(xp);
}

static PyObject *
dl_close(dlobject *xp, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;
	if (xp->dl_handle != NULL) {
		dlclose(xp->dl_handle);
		xp->dl_handle = NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
dl_sym(dlobject *xp, PyObject *args)
{
	char *name;
	PyUnivPtr *func;
	if (!PyArg_Parse(args, "s", &name))
		return NULL;
	func = dlsym(xp->dl_handle, name);
	if (func == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyInt_FromLong((long)func);
}

static PyObject *
dl_call(dlobject *xp, PyObject *args)
{
	PyObject *name;
	long (*func)(long, long, long, long, long,
                     long, long, long, long, long);
	long alist[10];
	long res;
	int i;
	int n = PyTuple_Size(args);
	if (n < 1) {
		PyErr_SetString(PyExc_TypeError, "at least a name is needed");
		return NULL;
	}
	name = PyTuple_GetItem(args, 0);
	if (!PyString_Check(name)) {
		PyErr_SetString(PyExc_TypeError,
				"function name must be a string");
		return NULL;
	}
	func = dlsym(xp->dl_handle, PyString_AsString(name));
	if (func == NULL) {
		PyErr_SetString(PyExc_ValueError, dlerror());
		return NULL;
	}
	if (n-1 > 10) {
		PyErr_SetString(PyExc_TypeError,
				"too many arguments (max 10)");
		return NULL;
	}
	for (i = 1; i < n; i++) {
		PyObject *v = PyTuple_GetItem(args, i);
		if (PyInt_Check(v))
			alist[i-1] = PyInt_AsLong(v);
		else if (PyString_Check(v))
			alist[i-1] = (long)PyString_AsString(v);
		else if (v == Py_None)
			alist[i-1] = (long) ((char *)NULL);
		else {
			PyErr_SetString(PyExc_TypeError,
				   "arguments must be int, string or None");
			return NULL;
		}
	}
	for (; i <= 10; i++)
		alist[i-1] = 0;
	res = (*func)(alist[0], alist[1], alist[2], alist[3], alist[4],
		      alist[5], alist[6], alist[7], alist[8], alist[9]);
	return PyInt_FromLong(res);
}

static PyMethodDef dlobject_methods[] = {
	{"call",	(PyCFunction)dl_call,	1 /* varargs */},
	{"sym", 	(PyCFunction)dl_sym},
	{"close",	(PyCFunction)dl_close},
	{NULL,  	NULL}			 /* Sentinel */
};

static PyObject *
dl_getattr(dlobject *xp, char *name)
{
	return Py_FindMethod(dlobject_methods, (PyObject *)xp, name);
}


static PyTypeObject Dltype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"dl",			/*tp_name*/
	sizeof(dlobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)dl_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)dl_getattr,/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

static PyObject *
dl_open(PyObject *self, PyObject *args)
{
	char *name;
	int mode;
	PyUnivPtr *handle;
	if (PyArg_Parse(args, "z", &name))
		mode = RTLD_LAZY;
	else {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(zi)", &name, &mode))
			return NULL;
#ifndef RTLD_NOW
		if (mode != RTLD_LAZY) {
			PyErr_SetString(PyExc_ValueError, "mode must be 1");
			return NULL;
		}
#endif
	}
	handle = dlopen(name, mode);
	if (handle == NULL) {
		PyErr_SetString(Dlerror, dlerror());
		return NULL;
	}
	return newdlobject(handle);
}

static PyMethodDef dl_methods[] = {
	{"open",	dl_open},
	{NULL,		NULL}		/* sentinel */
};

/* From socketmodule.c
 * Convenience routine to export an integer value.
 *
 * Errors are silently ignored, for better or for worse...
 */
static void
insint(PyObject *d, char *name, int value)
{
	PyObject *v = PyInt_FromLong((long) value);
	if (!v || PyDict_SetItemString(d, name, v))
		PyErr_Clear();

	Py_XDECREF(v);
}

void
initdl(void)
{
	PyObject *m, *d, *x;

	if (sizeof(int) != sizeof(long) ||
	    sizeof(long) != sizeof(char *)) {
		PyErr_SetString(PyExc_SystemError,
 "module dl requires sizeof(int) == sizeof(long) == sizeof(char*)");
		return;
	}

	/* Create the module and add the functions */
	m = Py_InitModule("dl", dl_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	Dlerror = x = PyErr_NewException("dl.error", NULL, NULL);
	PyDict_SetItemString(d, "error", x);
	x = PyInt_FromLong((long)RTLD_LAZY);
	PyDict_SetItemString(d, "RTLD_LAZY", x);
#define INSINT(X)    insint(d,#X,X)
#ifdef RTLD_NOW
        INSINT(RTLD_NOW);
#endif
#ifdef RTLD_NOLOAD
        INSINT(RTLD_NOLOAD);
#endif
#ifdef RTLD_GLOBAL
        INSINT(RTLD_GLOBAL);
#endif
#ifdef RTLD_LOCAL
        INSINT(RTLD_LOCAL);
#endif
#ifdef RTLD_PARENT
        INSINT(RTLD_PARENT);
#endif
#ifdef RTLD_GROUP
        INSINT(RTLD_GROUP);
#endif
#ifdef RTLD_WORLD
        INSINT(RTLD_WORLD);
#endif
#ifdef RTLD_NODELETE
        INSINT(RTLD_NODELETE);
#endif
}
