/***********************************************************************
 *  interruptmodule.c
 *
 *  Python extension implementing the interrupt module.
 *  
 **********************************************************************/

#include "Python.h"

#ifndef PyDoc_STR
#define PyDoc_VAR(name) static char name[]
#define PyDoc_STR(str) str
#define PyDoc_STRVAR(name,str) PyDoc_VAR(name) = PyDoc_STR(str)
#endif

/* module documentation */

PyDoc_STRVAR(module_doc,
"Provide a way to interrupt the main thread from a subthread.\n\n\
In threaded Python code the KeyboardInterrupt is always directed to\n\
the thread which raised it.  This extension provides a method,\n\
interrupt_main, which a subthread can use to raise a KeyboardInterrupt\n\
in the main thread.");

/* module functions */

static PyObject *
setinterrupt(PyObject * self, PyObject * args)
{
	PyErr_SetInterrupt();
	Py_INCREF(Py_None);
	return Py_None;
}

/* registration table */

static struct PyMethodDef methods[] = {
	{"interrupt_main", setinterrupt, METH_VARARGS,
	 PyDoc_STR("Interrupt the main thread")},
	{NULL, NULL}
};

/* module initialization */

void
initinterrupt(void)
{
	(void) Py_InitModule3("interrupt", methods, module_doc);
}
