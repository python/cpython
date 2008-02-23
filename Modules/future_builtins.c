
/* future_builtins module */

/* This module provides functions that will be builtins in Python 3.0,
   but that conflict with builtins that already exist in Python
   2.x. */


#include "Python.h"

PyDoc_STRVAR(module_doc,
"This module provides functions that will be builtins in Python 3.0,\n\
but that conflict with builtins that already exist in Python 2.x.\n\
\n\
Functions:\n\
\n\
hex(arg) -- Returns the hexidecimal representation of an integer\n\
oct(arg) -- Returns the octal representation of an integer\n\
\n\
The typical usage of this module is to replace existing builtins in a\n\
module's namespace:\n \n\
from future_builtins import hex, oct\n");

static PyObject *
builtin_hex(PyObject *self, PyObject *v)
{
	return PyNumber_ToBase(v, 16);
}

PyDoc_STRVAR(hex_doc,
"hex(number) -> string\n\
\n\
Return the hexadecimal representation of an integer or long integer.");


static PyObject *
builtin_oct(PyObject *self, PyObject *v)
{
	return PyNumber_ToBase(v, 8);
}

PyDoc_STRVAR(oct_doc,
"oct(number) -> string\n\
\n\
Return the octal representation of an integer or long integer.");


/* List of functions exported by this module */

static PyMethodDef module_functions[] = {
 	{"hex",		builtin_hex,        METH_O, hex_doc},
 	{"oct",		builtin_oct,        METH_O, oct_doc},
	{NULL,		NULL}	/* Sentinel */
};


/* Initialize this module. */

PyMODINIT_FUNC
initfuture_builtins(void)
{
	PyObject *m;

	m = Py_InitModule3("future_builtins", module_functions, module_doc);
	if (m == NULL)
		return;

	/* any other initialization needed */
}
