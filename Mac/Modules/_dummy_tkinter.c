
/* Dummy _tkinter module for use with Carbon. Gives (slightly) better error
 * message when you try to use Tkinter.
 */

/* Xxo objects */

#include "Python.h"


/* List of functions defined in the module */

static PyMethodDef xx_methods[] = {
	{NULL,		NULL}		/* sentinel */
};



DL_EXPORT(void)
init_tkinter(void)
{
	PyObject *m;

	/* Create the module and add the functions */
	m = Py_InitModule("_tkinter", xx_methods);

	PyErr_SetString(PyExc_ImportError, "Tkinter not supported under Carbon (yet).");
}
