/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
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
#include "macglue.h"

#include <Memory.h>
#include <Resources.h>
#include <stdio.h>
#include <Events.h>
#include <Windows.h>
#include <Fonts.h>

#ifdef __MWERKS__
#include <SIOUX.h>
#define USE_SIOUX
#if __profile__ == 1
#include <profiler.h>
#endif
#endif

#ifdef THINK_C
#include <console.h>
#endif

#define STARTUP "PythonStartup"

extern int Py_DebugFlag; /* For parser.c, declared in pythonrun.c */
extern int Py_VerboseFlag; /* For import.c, declared in pythonrun.c */
short PyMac_AppRefNum;	/* RefNum of application resource fork */

/* For Py_GetArgcArgv(); set by main() */
static char **orig_argv;
static int  orig_argc;

PyMac_PrefRecord options;

static void Py_Main Py_PROTO((int, char **)); /* Forward */
void PyMac_Exit Py_PROTO((int)); /* Forward */

/* Initialize the Mac toolbox world */

static void
init_mac_world()
{
#ifdef THINK_C
	printf("\n");
#else
	MaxApplZone();
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	TEInit();
	InitDialogs((long)0);
	InitMenus();
	InitCursor();
#endif
}

/*
** PyMac_InteractiveOptions - Allow user to set options if option key is pressed
*/
static void
PyMac_InteractiveOptions(PyMac_PrefRecord *p, int *argcp, char ***argvp)
{
	KeyMap rmap;
	unsigned char *map;
	short item, type;
	ControlHandle handle;
	DialogPtr dialog;
	Rect rect;
	int old_argc = *argcp;
	int i;

	/*
	** If the preferences disallows interactive options we return,
	** similarly of <option> isn't pressed.
	*/
	if (p->nointopt) return;
	
	GetKeys(rmap);
	map = (unsigned char *)rmap;
	if ( ( map[0x3a>>3] & (1<<(0x3a&7)) ) == 0 )	/* option key is 3a */
		return;

	dialog = GetNewDialog(OPT_DIALOG, NULL, (WindowPtr)-1);
	if ( dialog == NULL ) {
		printf("Option dialog not found - cannot set options\n");
		return;
	}
	SetDialogDefaultItem(dialog, OPT_OK);
	SetDialogCancelItem(dialog, OPT_CANCEL);
	
	/* Set default values */
#define SET_OPT_ITEM(num, var) \
		GetDialogItem(dialog, (num), &type, (Handle *)&handle, &rect); \
		SetControlValue(handle, (short)p->var);

	SET_OPT_ITEM(OPT_INSPECT, inspect);
	SET_OPT_ITEM(OPT_VERBOSE, verbose);
/*	SET_OPT_ITEM(OPT_SUPPRESS, suppress_print); */
	SET_OPT_ITEM(OPT_UNBUFFERED, unbuffered);
	SET_OPT_ITEM(OPT_DEBUGGING, debugging);
	SET_OPT_ITEM(OPT_KEEPNORMAL, keep_normal);
	SET_OPT_ITEM(OPT_KEEPERROR, keep_error);
	/* The rest are not settable interactively */

#undef SET_OPT_ITEM
	
	while (1) {
		handle = NULL;
		ModalDialog(NULL, &item);
		if ( item == OPT_OK )
			break;
		if ( item == OPT_CANCEL ) {
			DisposeDialog(dialog);
			exit(0);
		}
		if ( item == OPT_CMDLINE ) {
			int new_argc, newer_argc;
			char **new_argv, **newer_argv;
			
			new_argc = ccommand(&new_argv);
			newer_argc = (new_argc-1) + old_argc;
			newer_argv = malloc((newer_argc+1)*sizeof(char *));
			if( !newer_argv )
				Py_FatalError("Cannot malloc argv\n");
			for(i=0; i<old_argc; i++)
				newer_argv[i] = (*argvp)[i];
			for(i=old_argc; i<=newer_argc; i++) /* Copy the NULL too */
				newer_argv[i] = new_argv[i-old_argc+1];
			*argvp = newer_argv;
			*argcp = newer_argc;
			
			/* XXXX Is it not safe to use free() here, apparently */
		}
#define OPT_ITEM(num, var) \
		if ( item == (num) ) { \
			p->var = !p->var; \
			GetDialogItem(dialog, (num), &type, (Handle *)&handle, &rect); \
			SetControlValue(handle, (short)p->var); \
		}
		
		OPT_ITEM(OPT_INSPECT, inspect);
		OPT_ITEM(OPT_VERBOSE, verbose);
/*		OPT_ITEM(OPT_SUPPRESS, suppress_print); */
		OPT_ITEM(OPT_UNBUFFERED, unbuffered);
		OPT_ITEM(OPT_DEBUGGING, debugging);
		OPT_ITEM(OPT_KEEPNORMAL, keep_normal);
		OPT_ITEM(OPT_KEEPERROR, keep_error);
		
#undef OPT_ITEM
	}
	DisposeDialog(dialog);
}

/*
** Initialization code, shared by interpreter and applets
*/
static void
init_common(int *argcp, char ***argvp, int embedded)
{
	/* Remember resource fork refnum, for later */
	PyMac_AppRefNum = CurResFile();
	
	/* Initialize toolboxes */
	init_mac_world();
	
#ifdef USE_MAC_SHARED_LIBRARY
	/* Add the shared library to the stack of resource files */
	PyMac_AddLibResources();
#endif

#if defined(USE_GUSI)
	/* Setup GUSI */
	GUSIDefaultSetup();
	PyMac_SetGUSISpin();
	PyMac_SetGUSIOptions();
	atexit(PyMac_StopGUSISpin);
#endif

#ifdef USE_SIOUX
	/* Set various SIOUX flags. Some are changed later based on options */
/*	SIOUXSettings.standalone = 0;	/* XXXX Attempting to keep sioux from eating events */
	SIOUXSettings.asktosaveonclose = 0;
	SIOUXSettings.showstatusline = 0;
	SIOUXSettings.tabspaces = 4;
#endif

	/* Get options from preference file (or from applet resource fork) */
	options.keep_error = 1;		/* default-default */
	PyMac_PreferenceOptions(&options);
	
	if ( embedded ) {
		static char *emb_argv[] = {"embedded-python", 0};
		
		*argcp = 1;
		*argvp = emb_argv;
	} else {
		/* Create argc/argv. Do it before we go into the options event loop. */
		*argcp = PyMac_GetArgv(argvp, options.noargs);
		
		/* Do interactive option setting, if allowed and <option> depressed */
		PyMac_InteractiveOptions(&options, argcp, argvp);
	}
	
	/* Copy selected options to where the machine-independent stuff wants it */
	Py_VerboseFlag = options.verbose;
/*	Py_SuppressPrintingFlag = options.suppress_print; */
	Py_DebugFlag = options.debugging;
	if ( options.noargs ) {
		/* don't process events at all without the scripts permission */
		PyMacSchedParams scp;
		
		PyMac_GetSchedParams(&scp);
		scp.process_events = 0;
		/* Should we disable command-dot as well? */
		PyMac_SetSchedParams(&scp);
	}

	/* Set buffering */
	if (options.unbuffered) {
#ifndef MPW
		setbuf(stdout, (char *)NULL);
		setbuf(stderr, (char *)NULL);
#else
		/* On MPW (3.2) unbuffered seems to hang */
		setvbuf(stdout, (char *)NULL, _IOLBF, BUFSIZ);
		setvbuf(stderr, (char *)NULL, _IOLBF, BUFSIZ);
#endif
	}
#if __profile__ == 1
	/* collectSummary or collectDetailed, timebase, #routines, max stack depth */
	ProfilerInit(collectSummary, bestTimeBase, 2000, 150);
#endif

	/* Tell the rest of python about our argc/argv */
	orig_argc = *argcp;	/* For Py_GetArgcArgv() */
	orig_argv = *argvp;
	Py_SetProgramName((*argvp)[0]);
}

/*
** Inspection mode after script/applet termination
*/
static int
run_inspect()
{
	int sts = 0;
	
	if (options.inspect && isatty((int)fileno(stdin)))
		sts = PyRun_AnyFile(stdin, "<stdin>") != 0;
	return sts;
}

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

	init_common(&argc, &argv, 0);
	
	Py_Initialize();
	PySys_SetArgv(argc, argv);
	
	err = run_main_resource();
	
	err = (run_inspect() || err);
	
	fflush(stderr);
	fflush(stdout);
	PyMac_Exit(err);
	/* XXX Should we bother to Py_Exit(sts)? */
}

/*
** Hook for embedding python.
*/
void
PyMac_Initialize()
{
	int argc;
	char **argv;
	
	init_common(&argc, &argv, 1);
	Py_Initialize();
	PySys_SetArgv(argc, argv);
}

#endif /* USE_MAC_APPLET_SUPPORT */

/* For normal application */
void
PyMac_InitApplication()
{
	int argc;
	char **argv;
	
	init_common(&argc, &argv, 0);
	
	if ( argc > 1 ) {
		/* We're running a script. Attempt to change current directory */
		char curwd[256], *endp;
		
		strcpy(curwd, argv[1]);
		endp = strrchr(curwd, ':');
		if ( endp && endp > curwd ) {
			*endp = '\0';

			chdir(curwd);
#ifdef USE_GUSI
			/* Change MacOS's idea of wd too */
			PyMac_FixGUSIcd();
#endif
		}
	}
	Py_Main(argc, argv);
}

/* Main program */

static void
Py_Main(argc, argv)
	int argc;
	char **argv;
{
	int sts;
	char *command = NULL;
	char *filename = NULL;
	FILE *fp = stdin;

	filename = argv[1];

	if (Py_VerboseFlag ||
	    command == NULL && filename == NULL && isatty((int)fileno(fp)))
		fprintf(stderr, "Python %s\n%s\n",
			Py_GetVersion(), Py_GetCopyright());
	
	if (filename != NULL) {
		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "%s: can't open file '%s'\n",
				argv[0], filename);
			PyMac_Exit(2);
		}
	}
	
	/* We initialize the menubar here, hoping SIOUX is initialized by now */
	PyMac_InitMenuBar();
	
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
		
	if ( filename != NULL || command != NULL )
		sts = (run_inspect() || sts);

	Py_Exit(sts);
	/*NOTREACHED*/
}

/*
** Terminate application
*/
void
PyMac_Exit(status)
	int status;
{
	int keep;

#if __profile__ == 1
	ProfilerDump("\pPython Profiler Results");
	ProfilerTerm();
#endif	
	if ( status )
		keep = options.keep_error;
	else
		keep = options.keep_normal;
		
#ifdef USE_SIOUX
	if (keep) {
		SIOUXSettings.standalone = 1;
		SIOUXSettings.autocloseonquit = 0;
		SIOUXSetTitle("\p\307terminated\310");
		PyMac_RestoreMenuBar();
#ifdef USE_MSL
		/*
		** Temporary workaround: autocloseonquit clearing does not
		** currently work for the MSL/GUSI combo.
		*/
		while(getchar() > 0);
#endif
	}
	else
		SIOUXSettings.autocloseonquit = 1;
#endif /* USE_SIOUX */
#ifdef THINK_C
	console_options.pause_atexit = keep;
#endif

	exit(status);
}

/* Return the program name -- some code out there needs this. */
char *
Py_GetProgramFullPath()
{
	return orig_argv[0];
}


/* Make the *original* argc/argv available to other modules.
   This is rare, but it is needed by the secureware extension. */

void
Py_GetArgcArgv(argc,argv)
	int *argc;
	char ***argv;
{
	*argc = orig_argc;
	*argv = orig_argv;
}

/* More cruft that shouldn't really be here, used in sysmodule.c */

char *
Py_GetPrefix()
{
	return "";
}

char *
Py_GetExecPrefix()
{
	return "";
}
