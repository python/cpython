/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Python interpreter main program for frozen scripts */

#include "Python.h"

#ifdef MS_WIN32
extern void PyWinFreeze_ExeInit();
extern void PyWinFreeze_ExeTerm();
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* For isatty() */
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
