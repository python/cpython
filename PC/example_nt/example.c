#include "Python.h"

static PyObject *
ex_foo(PyObject *self, PyObject *args)
{
	printf("Hello, world\n");
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef example_methods[] = {
	{"foo", ex_foo, METH_VARARGS, "foo() doc string"},
	{NULL, NULL}
};

PyMODINIT_FUNC
initexample(void)
{
	Py_InitModule("example", example_methods);
}
