/* 
 * tixAppInit.c --
 *
 *	Provides a default version of the Tcl_AppInit procedure for
 *	use in wish and similar Tk-based applications.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 2000 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixAppInit.c,v 1.2 2008/02/28 04:05:29 hobbs Exp $
 */

#ifdef USE_TCL_STUBS
#undef USE_TCL_STUBS
#endif

#ifdef USE_TK_STUBS
#undef USE_TK_STUBS
#endif

#include <tk.h>
#include <tix.h>

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

#if !defined(__WIN32__) && !defined(MAC_TCL) /* UNIX */

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

extern int matherr();
int *tclDummyMathPtr = (int *) matherr;

#endif /* UNIX */

#ifndef NO_MAIN


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;			/* Number of command-line arguments. */
    char **argv;		/* Values of command-line arguments. */
{
    Tk_Main(argc, argv, Tcl_AppInit);
    return 0;			/* Needed only to prevent compiler warning. */
}

#endif /* NO_MAIN */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp's result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    if (Tcl_Init(interp) == TCL_ERROR) {
	goto error;
    }
    if (Tk_Init(interp) == TCL_ERROR) {
	goto error;
    }
    Tcl_StaticPackage(interp, "Tk",  Tk_Init,  (Tcl_PackageInitProc *) NULL);

    if (Tix_Init(interp) == TCL_ERROR) {
	goto error;
    }
    Tcl_StaticPackage(interp, "Tix", Tix_Init, (Tcl_PackageInitProc *) NULL);

#if defined(__WIN32__) && defined(GUI_MODE_APP)
    /*
     * Initialize the console only if we are running as an interactive
     * application.
     */

    if (Tk_CreateConsoleWindow(interp) == TCL_ERROR) {
        goto error;
    }
#endif /* __WIN32__ */

    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     goto error;
     * }
     *
     * where "Mod" is the name of the module.
     */

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */
    Tix_SetRcFileName(interp, "~/.tixwishrc");

    return TCL_OK;

error:

#if defined(__WIN32__) && defined(GUI_MODE_APP)

    MessageBeep(MB_ICONEXCLAMATION);
    MessageBox(NULL, Tcl_GetStringResult(interp), "Error in Wish",
	    MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
    ExitProcess(1);
    /* we won't reach this, but we need the return */

#endif /* __WIN32__ */

    return TCL_ERROR;
}
