/* Example of embedding Python in another program */

#include "allobjects.h"

static char *argv0;

main(argc, argv)
	int argc;
	char **argv;
{
	/* Save a copy of argv0 */
	argv0 = argv[0];

	/* Initialize the Python interpreter.  Required. */
	initall();

	/* Define sys.argv.  It is up to the application if you
	   want this; you can also let it undefined (since the Python 
	   code is generally not a main program it has no business
	   touching sys.argv...) */
	setpythonargv(argc, argv);

	/* Do some application specific code */
	printf("Hello, brave new world\n\n");

	/* Execute some Python statements (in module __main__) */
	run_command("import sys\n");
	run_command("print sys.builtin_module_names\n");
	run_command("print sys.argv\n");

	/* Note that you can call any public function of the Python
	   interpreter here, e.g. call_object(). */

	/* Some more application specific code */
	printf("\nGoodbye, cruel world\n");

	/* Exit, cleaning up the interpreter */
	goaway(0);
	/*NOTREACHED*/
}

char *
getprogramname()
{
	return argv0;
}
