/* Example of embedding Python in another program */

#include "Python.h"

void initxyzzy(void); /* Forward */

main(int argc, char **argv)
{
	/* Pass argv[0] to the Python interpreter */
	Py_SetProgramName(argv[0]);

	/* Initialize the Python interpreter.  Required. */
	Py_Initialize();

	/* Add a static module */
	initxyzzy();

	/* Define sys.argv.  It is up to the application if you
	   want this; you can also let it undefined (since the Python 
	   code is generally not a main program it has no business
	   touching sys.argv...) */
	PySys_SetArgv(argc, argv);

	/* Do some application specific code */
	printf("Hello, brave new world\n\n");

	/* Execute some Python statements (in module __main__) */
	PyRun_SimpleString("import sys\n");
	PyRun_SimpleString("print sys.builtin_module_names\n");
	PyRun_SimpleString("print sys.modules.keys()\n");
	PyRun_SimpleString("print sys.executable\n");
	PyRun_SimpleString("print sys.argv\n");

	/* Note that you can call any public function of the Python
	   interpreter here, e.g. call_object(). */

	/* Some more application specific code */
	printf("\nGoodbye, cruel world\n");

	/* Exit, cleaning up the interpreter */
	Py_Exit(0);
	/*NOTREACHED*/
}

/* A static module */

/* 'self' is not used */
static PyObject *
xyzzy_foo(PyObject *self, PyObjecT *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return PyInt_FromLong(42L);
}

static PyMethodDef xyzzy_methods[] = {
	{"foo",		xyzzy_foo,	1},
	{NULL,		NULL}		/* sentinel */
};

void
initxyzzy(void)
{
	PyImport_AddModule("xyzzy");
	Py_InitModule("xyzzy", xyzzy_methods);
}
