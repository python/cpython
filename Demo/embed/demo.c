/* Example of embedding Python in another program */

#include "Python.h"

static char *argv0;

main(argc, argv)
	int argc;
	char **argv;
{
	/* Save a copy of argv0 */
	argv0 = argv[0];

	/* Initialize the Python interpreter.  Required. */
	Py_Initialize();

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
	PyRun_SimpleString("print sys.argv\n");

	/* Note that you can call any public function of the Python
	   interpreter here, e.g. call_object(). */

	/* Some more application specific code */
	printf("\nGoodbye, cruel world\n");

	/* Exit, cleaning up the interpreter */
	Py_Exit(0);
	/*NOTREACHED*/
}

/* This function is called by the interpreter to get its own name */
char *
getprogramname()
{
	return argv0;
}
