/* Example of embedding Python in another program */

#include "Python.h"
#include "macglue.h"

static char *argv0;

long my_writehandler(char *buf, long count)
{
	long mycount;
	unsigned char mybuf[255];
	
	mycount = count;
	if (mycount > 255 ) mycount = 255;
	mybuf[0] = (unsigned char)mycount;
	strncpy((char *)mybuf+1, buf, mycount);
	DebugStr(mybuf);
	return count;
}

main(argc, argv)
	int argc;
	char **argv;
{
	/* So the user can set argc/argv to something interesting */
	argc = ccommand(&argv);
	/* Save a copy of argv0 */
	argv0 = argv[0];

	/* If the first option is "-q" we don't open a console */
	if ( argc > 1 && strcmp(argv[1], "-q") == 0 ) {
		PyMac_SetConsoleHandler(PyMac_DummyReadHandler, PyMac_DummyWriteHandler,
			PyMac_DummyWriteHandler);
	} else
	if ( argc > 1 && strcmp(argv[1], "-d") == 0 )  {
		PyMac_SetConsoleHandler(PyMac_DummyReadHandler, my_writehandler,
			my_writehandler);
	} 
	/* Initialize the Python interpreter.  Required. */
	PyMac_Initialize();

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
