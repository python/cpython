/* strop module */

#define PY_SSIZE_T_CLEAN
#include "Python.h"

PyDoc_STRVAR(strop_module__doc__,
"Common string manipulations, optimized for speed.\n"
"\n"
"Always use \"import string\" rather than referencing\n"
"this module directly.");

PyDoc_STRVAR(maketrans__doc__,
"maketrans(frm, to) -> string\n"
"\n"
"Return a translation table (a string of 256 bytes long)\n"
"suitable for use in string.translate.  The strings frm and to\n"
"must be of the same length.");

static PyObject *
strop_maketrans(PyObject *self, PyObject *args)
{
	unsigned char *c, *from=NULL, *to=NULL;
	Py_ssize_t i, fromlen=0, tolen=0;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "t#t#:maketrans", &from, &fromlen, &to, &tolen))
		return NULL;

	if (fromlen != tolen) {
		PyErr_SetString(PyExc_ValueError,
				"maketrans arguments must have same length");
		return NULL;
	}

	result = PyString_FromStringAndSize((char *)NULL, 256);
	if (result == NULL)
		return NULL;
	c = (unsigned char *) PyString_AS_STRING((PyStringObject *)result);
	for (i = 0; i < 256; i++)
		c[i]=(unsigned char)i;
	for (i = 0; i < fromlen; i++)
		c[from[i]]=to[i];

	return result;
}

/* List of functions defined in the module */

static PyMethodDef
strop_methods[] = {
	{"maketrans",	strop_maketrans,   METH_VARARGS, maketrans__doc__},
	{NULL,		NULL}	/* sentinel */
};


PyMODINIT_FUNC
initstrop(void)
{
	PyObject *m;
	m = Py_InitModule4("strop", strop_methods, strop_module__doc__,
			   (PyObject*)NULL, PYTHON_API_VERSION);
	if (m == NULL)
		return;
}
