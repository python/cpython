/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Python interpreter main program */

#include "Python.h"
#include "pythonresources.h"
#include "import.h"
#include "marshal.h"

#include <Memory.h>
#include <Resources.h>
#include <stdio.h>
#include <Events.h>
#include <Windows.h>
#include <Desk.h>

#ifdef __MWERKS__
#include <SIOUX.h>
#endif

#define STARTUP "PythonStartup"

extern int Py_DebugFlag; /* For parser.c, declared in pythonrun.c */
extern int Py_VerboseFlag; /* For import.c, declared in pythonrun.c */
extern int Py_SuppressPrintingFlag; /* For ceval.c, declared in pythonrun.c */


/* Subroutines that live in their own file */
extern char *getversion();
extern char *getcopyright();


/* For getprogramname(); set by main() */
static char *argv0;

/* For getargcargv(); set by main() */
static char **orig_argv;
static int  orig_argc;

#ifdef USE_MAC_APPLET_SUPPORT
/* Applet support */

/* Run a compiled Python Python script from 'PYC ' resource __main__ */
static int
run_main_resource()
{
	Handle h;
	long size;
	PyObject *code;
	PyObject *result;
	
	h = GetNamedResource('PYC ', "\p__main__");
	if (h == NULL) {
		Alert(NOPYC_ALERT, NULL);
		return 1;
	}
	size = GetResourceSizeOnDisk(h);
	HLock(h);
	code = PyMarshal_ReadObjectFromString(*h + 8, (int)(size - 8));
	HUnlock(h);
	ReleaseResource(h);
	if (code == NULL) {
		PyErr_Print();
		return 1;
	}
	result = PyImport_ExecCodeModule("__main__", code);
	Py_DECREF(code);
	if (result == NULL) {
		PyErr_Print();
		return 1;
	}
	Py_DECREF(result);
	return 0;
}

/* Initialization sequence for applets */
void
PyMac_InitApplet()
{
	int argc;
	char **argv;
	int err;

#ifdef USE_MAC_SHARED_LIBRARY
	PyMac_AddLibResources();
#endif
#ifdef __MWERKS__
	SIOUXSettings.asktosaveonclose = 0;
	SIOUXSettings.showstatusline = 0;
	SIOUXSettings.tabspaces = 4;
#endif
	argc = PyMac_GetArgv(&argv);
	Py_Initialize();
	PySys_SetArgv(argc, argv);
	err = run_main_resource();
	fflush(stderr);
	fflush(stdout);
#ifdef __MWERKS__
	if (!err)
		SIOUXSettings.autocloseonquit = 1;
	else
		printf("\n[Terminated]\n");
#endif
	/* XXX Should we bother to Py_Exit(sts)? */
}

#endif /* USE_MAC_APPLET_SUPPORT */

/* For normal application */
void
PyMac_InitApplication()
{
	int argc;
	char **argv;
	
#ifdef USE_MAC_SHARED_LIBRARY
	PyMac_AddLibResources();
#endif
#ifdef __MWERKS__
	SIOUXSettings.asktosaveonclose = 0;
	SIOUXSettings.showstatusline = 0;
	SIOUXSettings.tabspaces = 4;
#endif
	argc = PyMac_GetArgv(&argv);
	if ( argc > 1 ) {
		/* We're running a script. Attempt to change current directory */
		char curwd[256], *endp;
		
		strcpy(curwd, argv[1]);
		endp = strrchr(curwd, ':');
		if ( endp && endp > curwd ) {
			*endp = '\0';

			chdir(curwd);
		}
	}
	Py_Main(argc, argv);
}

/*
** PyMac_InteractiveOptions - Allow user to set options if option key is pressed
*/
void
PyMac_InteractiveOptions(int *inspect, int *verbose, int *suppress_print, 
						 int *unbuffered, int *debugging)
{
	KeyMap rmap;
	unsigned char *map;
	short item, type;
	ControlHandle handle;
	DialogPtr dialog;
	Rect rect;
	
	GetKeys(rmap);
	map = (unsigned char *)rmap;
	if ( ( map[0x3a>>3] & (1<<(0x3a&7)) ) == 0 )	/* option key is 3a */
		return;

	dialog = GetNewDialog(OPT_DIALOG, NULL, (WindowPtr)-1);
	if ( dialog == NULL ) {
		printf("Option dialog not found - cannot set options\n");
		return;
	}
	while (1) {
		handle = NULL;
		ModalDialog(NULL, &item);
		if ( item == OPT_OK )
			break;
		if ( item == OPT_CANCEL ) {
			DisposDialog(dialog);
			exit(0);
		}
#define OPT_ITEM(num, var) \
		if ( item == (num) ) { \
			*(var) = !*(var); \
			GetDialogItem(dialog, (num), &type, (Handle *)&handle, &rect); \
			SetCtlValue(handle, (short)*(var)); \
		}
		
		OPT_ITEM(OPT_INSPECT, inspect);
		OPT_ITEM(OPT_VERBOSE, verbose);
		OPT_ITEM(OPT_SUPPRESS, suppress_print);
		OPT_ITEM(OPT_UNBUFFERED, unbuffered);
		OPT_ITEM(OPT_DEBUGGING, debugging);
		
#undef OPT_ITEM
	}
	DisposDialog(dialog);
}
/* Main program */

int
Py_Main(argc, argv)
	int argc;
	char **argv;
{
	int sts;
	char *command = NULL;
	char *filename = NULL;
	FILE *fp = stdin;
	int inspect = 0;
	int unbuffered = 0;

	orig_argc = argc;	/* For getargcargv() */
	orig_argv = argv;
	argv0 = argv[0];	/* For getprogramname() */
	
	PyMac_InteractiveOptions(&inspect, &Py_VerboseFlag, &Py_SuppressPrintingFlag,
			&unbuffered, &Py_DebugFlag);


	if (unbuffered) {
#ifndef MPW
		setbuf(stdout, (char *)NULL);
		setbuf(stderr, (char *)NULL);
#else
		/* On MPW (3.2) unbuffered seems to hang */
		setvbuf(stdout, (char *)NULL, _IOLBF, BUFSIZ);
		setvbuf(stderr, (char *)NULL, _IOLBF, BUFSIZ);
#endif
	}

	filename = argv[1];

	if (Py_VerboseFlag ||
	    command == NULL && filename == NULL && isatty((int)fileno(fp)))
		fprintf(stderr, "Python %s\n%s\n",
			getversion(), getcopyright());
	
	if (filename != NULL) {
		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "%s: can't open file '%s'\n",
				argv[0], filename);
			exit(2);
		}
	}
	
	Py_Initialize();
	
	PySys_SetArgv(argc-1, argv+1);

	if (filename == NULL && isatty((int)fileno(fp))) {
		FILE *fp = fopen(STARTUP, "r");
		if (fp != NULL) {
			(void) PyRun_SimpleFile(fp, STARTUP);
			PyErr_Clear();
			fclose(fp);
		}
	}
	sts = PyRun_AnyFile(
			fp, filename == NULL ? "<stdin>" : filename) != 0;
	if (filename != NULL)
		fclose(fp);

	if (inspect && isatty((int)fileno(stdin)) &&
	    (filename != NULL || command != NULL))
		sts = PyRun_AnyFile(stdin, "<stdin>") != 0;

	Py_Exit(sts);
	/*NOTREACHED*/
}


/* Return the program name -- some code out there needs this. */

char *
getprogramname()
{
	return argv0;
}


/* Make the *original* argc/argv available to other modules.
   This is rare, but it is needed by the secureware extension. */

void
getargcargv(argc,argv)
	int *argc;
	char ***argv;
{
	*argc = orig_argc;
	*argv = orig_argv;
}
