/* Simple program that repeatedly calls Py_Initialize(), does something, and
   then calls Py_Finalize().  This should help finding leaks related to
   initialization. */

#include "Python.h"

main(int argc, char **argv)
{
	char *command;

	if (argc != 2) {
		fprintf(stderr, "usage: loop <python-command>\n");
		exit(2);
	}

	command = argv[1];

	Py_SetProgramName(argv[0]);

	while (1) {
		Py_Initialize();
		PyRun_SimpleString(command);
		Py_Finalize();
	}
	/*NOTREACHED*/
}
