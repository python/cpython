
/* Use this file as a template to start implementing a new object type.
   If your objects will be called foobar, start by copying this file to
   foobarobject.c, changing all occurrences of xx to foobar and all
   occurrences of Xx by Foobar.  You will probably want to delete all
   references to 'x_attr' and add your own types of attributes
   instead.  Maybe you want to name your local variables other than
   'xp'.  If your object type is needed in other files, you'll have to
   create a file "foobarobject.h"; see intobject.h for an example. */


/* Xx objects */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	PyObject	*x_attr;	/* Attributes dictionary */
} xxobject;

staticforward PyTypeObject Xxtype;

#define is_xxobject(v)		((v)->ob_type == &Xxtype)

static xxobject *
newxxobject(PyObject *arg)
{
	xxobject *xp;
	xp = PyObject_NEW(xxobject, &Xxtype);
	if (xp == NULL)
		return NULL;
	xp->x_attr = NULL;
	return xp;
}

/* Xx methods */

static void
xx_dealloc(xxobject *xp)
{
	Py_XDECREF(xp->x_attr);
	PyObject_DEL(xp);
}

static PyObject *
xx_demo(xxobject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":demo"))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef xx_methods[] = {
	{"demo",	(PyCFunction)xx_demo,	METH_VARARGS},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
xx_getattr(xxobject *xp, char *name)
{
	if (xp->x_attr != NULL) {
		PyObject *v = PyDict_GetItemString(xp->x_attr, name);
		if (v != NULL) {
			Py_INCREF(v);
			return v;
		}
	}
	return Py_FindMethod(xx_methods, (PyObject *)xp, name);
}

static int
xx_setattr(xxobject *xp, char *name, PyObject *v)
{
	if (xp->x_attr == NULL) {
		xp->x_attr = PyDict_New();
		if (xp->x_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(xp->x_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
                                        "delete non-existing xx attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(xp->x_attr, name, v);
}

static PyTypeObject Xxtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"xx",			/*tp_name*/
	sizeof(xxobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)xx_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)xx_getattr, /*tp_getattr*/
	(setattrfunc)xx_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};
