/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

******************************************************************/

/* Python interpreter main program for frozen scripts */

#include "Python.h"

#ifdef MS_WIN32
extern void PyWinFreeze_ExeInit();
extern void PyWinFreeze_ExeTerm();
extern int PyInitFrozenExtensions();
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* For isatty() */
#endif

/* For isatty()'s proto. - [cjh] */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Main program */

int
Py_FrozenMain(argc, argv)
	int argc;
	char **argv;
{
	char *p;
	int n, sts;
	int inspect = 0;
	int unbuffered = 0;

	Py_FrozenFlag = 1; /* Suppress errors from getpath.c */

	if ((p = getenv("PYTHONINSPECT")) && *p != '\0')
		inspect = 1;
	if ((p = getenv("PYTHONUNBUFFERED")) && *p != '\0')
		unbuffered = 1;

	if (unbuffered) {
		setbuf(stdin, (char *)NULL);
		setbuf(stdout, (char *)NULL);
		setbuf(stderr, (char *)NULL);
	}

#ifdef MS_WIN32
	PyInitFrozenExtensions();
#endif /* MS_WIN32 */
	Py_SetProgramName(argv[0]);
	Py_Initialize();
#ifdef MS_WIN32
	PyWinFreeze_ExeInit();
#endif

	if (Py_VerboseFlag)
		fprintf(stderr, "Python %s\n%s\n",
			Py_GetVersion(), Py_GetCopyright());

	PySys_SetArgv(argc, argv);

	n = PyImport_ImportFrozenModule("__main__");
	if (n == 0)
		Py_FatalError("__main__ not frozen");
	if (n < 0) {
		PyErr_Print();
		sts = 1;
	}
	else
		sts = 0;

	if (inspect && isatty((int)fileno(stdin)))
		sts = PyRun_AnyFile(stdin, "<stdin>") != 0;

#ifdef MS_WIN32
	PyWinFreeze_ExeTerm();
#endif
	Py_Finalize();
	return sts;
}
