/*
 * tclAppInit.c --
 *
 *	Provides a default version of the main program and Tcl_AppInit
 *	procedure for tclsh and other Tcl-based applications (without Tk).
 *	Note that this program must be built in Win32 console mode to work
 *	properly.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tcl.h"
#define WIN32_LEAN_AND_MEAN
#define STRICT			/* See MSDN Article Q83456 */
#include <windows.h>
#undef STRICT
#undef WIN32_LEAN_AND_MEAN
#include <locale.h>
#include <stdlib.h>
#include <tchar.h>

#ifdef TCL_TEST
extern Tcl_PackageInitProc Tcltest_Init;
extern Tcl_PackageInitProc Tcltest_SafeInit;
#endif /* TCL_TEST */

#if defined(STATIC_BUILD) && TCL_USE_STATIC_PACKAGES
extern Tcl_PackageInitProc Registry_Init;
extern Tcl_PackageInitProc Dde_Init;
extern Tcl_PackageInitProc Dde_SafeInit;
#endif

#if defined(__GNUC__) || defined(TCL_BROKEN_MAINARGS)
int _CRT_glob = 0;
#endif /* __GNUC__ || TCL_BROKEN_MAINARGS */
#ifdef TCL_BROKEN_MAINARGS
static void setargv(int *argcPtr, TCHAR ***argvPtr);
#endif /* TCL_BROKEN_MAINARGS */

/*
 * The following #if block allows you to change the AppInit function by using
 * a #define of TCL_LOCAL_APPINIT instead of rewriting this entire file. The
 * #if checks for that #define and uses Tcl_AppInit if it does not exist.
 */

#ifndef TCL_LOCAL_APPINIT
#define TCL_LOCAL_APPINIT Tcl_AppInit
#endif
#ifndef MODULE_SCOPE
#   define MODULE_SCOPE extern
#endif
MODULE_SCOPE int TCL_LOCAL_APPINIT(Tcl_Interp *);

/*
 * The following #if block allows you to change how Tcl finds the startup
 * script, prime the library or encoding paths, fiddle with the argv, etc.,
 * without needing to rewrite Tcl_Main()
 */

#ifdef TCL_LOCAL_MAIN_HOOK
MODULE_SCOPE int TCL_LOCAL_MAIN_HOOK(int *argc, TCHAR ***argv);
#endif

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tcl_Main never returns here, so this procedure never returns
 *	either.
 *
 * Side effects:
 *	Just about anything, since from here we call arbitrary Tcl code.
 *
 *----------------------------------------------------------------------
 */

#ifdef TCL_BROKEN_MAINARGS
int
main(
    int argc,			/* Number of command-line arguments. */
    char *dummy[])		/* Not used. */
{
    TCHAR **argv;
#else
int
_tmain(
    int argc,			/* Number of command-line arguments. */
    TCHAR *argv[])		/* Values of command-line arguments. */
{
#endif
    TCHAR *p;

    /*
     * Set up the default locale to be standard "C" locale so parsing is
     * performed correctly.
     */

    setlocale(LC_ALL, "C");

#ifdef TCL_BROKEN_MAINARGS
    /*
     * Get our args from the c-runtime. Ignore command line.
     */

    setargv(&argc, &argv);
#endif

    /*
     * Forward slashes substituted for backslashes.
     */

    for (p = argv[0]; *p != '\0'; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }

#ifdef TCL_LOCAL_MAIN_HOOK
    TCL_LOCAL_MAIN_HOOK(&argc, &argv);
#endif

    Tcl_Main(argc, argv, TCL_LOCAL_APPINIT);
    return 0;			/* Needed only to prevent compiler warning. */
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization. Most
 *	applications, especially those that incorporate additional packages,
 *	will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error message in
 *	the interp's result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(
    Tcl_Interp *interp)		/* Interpreter for application. */
{
    if ((Tcl_Init)(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

#if defined(STATIC_BUILD) && TCL_USE_STATIC_PACKAGES
    if (Registry_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "registry", Registry_Init, NULL);

    if (Dde_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "dde", Dde_Init, Dde_SafeInit);
#endif

#ifdef TCL_TEST
    if (Tcltest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tcltest", Tcltest_Init, Tcltest_SafeInit);
#endif /* TCL_TEST */

    /*
     * Call the init procedures for included packages. Each call should look
     * like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module. (Dynamically-loadable packages
     * should have the same entry-point name.)
     */

    /*
     * Call Tcl_CreateCommand for application-specific commands, if they
     * weren't already created by the init procedures called above.
     */

    /*
     * Specify a user-specific startup file to invoke if the application is
     * run interactively. Typically the startup file is "~/.apprc" where "app"
     * is the name of the application. If this line is deleted then no
     * user-specific startup file will be run under any conditions.
     */

    (Tcl_ObjSetVar2)(interp, Tcl_NewStringObj("tcl_rcFileName", -1), NULL,
	    Tcl_NewStringObj("~/tclshrc.tcl", -1), TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 *-------------------------------------------------------------------------
 *
 * setargv --
 *
 *	Parse the Windows command line string into argc/argv. Done here
 *	because we don't trust the builtin argument parser in crt0. Windows
 *	applications are responsible for breaking their command line into
 *	arguments.
 *
 *	2N backslashes + quote -> N backslashes + begin quoted string
 *	2N + 1 backslashes + quote -> literal
 *	N backslashes + non-quote -> literal
 *	quote + quote in a quoted string -> single quote
 *	quote + quote not in quoted string -> empty string
 *	quote -> begin quoted string
 *
 * Results:
 *	Fills argcPtr with the number of arguments and argvPtr with the array
 *	of arguments.
 *
 * Side effects:
 *	Memory allocated.
 *
 *--------------------------------------------------------------------------
 */

#ifdef TCL_BROKEN_MAINARGS
static void
setargv(
    int *argcPtr,		/* Filled with number of argument strings. */
    TCHAR ***argvPtr)		/* Filled with argument strings (malloc'd). */
{
    TCHAR *cmdLine, *p, *arg, *argSpace;
    TCHAR **argv;
    int argc, size, inquote, copy, slashes;

    cmdLine = GetCommandLine();

    /*
     * Precompute an overly pessimistic guess at the number of arguments in
     * the command line by counting non-space spans.
     */

    size = 2;
    for (p = cmdLine; *p != '\0'; p++) {
	if ((*p == ' ') || (*p == '\t')) {	/* INTL: ISO space. */
	    size++;
	    while ((*p == ' ') || (*p == '\t')) { /* INTL: ISO space. */
		p++;
	    }
	    if (*p == '\0') {
		break;
	    }
	}
    }

    /* Make sure we don't call ckalloc through the (not yet initialized) stub table */
    #undef Tcl_Alloc
    #undef Tcl_DbCkalloc

    argSpace = ckalloc(size * sizeof(char *)
	    + (_tcslen(cmdLine) * sizeof(TCHAR)) + sizeof(TCHAR));
    argv = (TCHAR **) argSpace;
    argSpace += size * (sizeof(char *)/sizeof(TCHAR));
    size--;

    p = cmdLine;
    for (argc = 0; argc < size; argc++) {
	argv[argc] = arg = argSpace;
	while ((*p == ' ') || (*p == '\t')) {	/* INTL: ISO space. */
	    p++;
	}
	if (*p == '\0') {
	    break;
	}

	inquote = 0;
	slashes = 0;
	while (1) {
	    copy = 1;
	    while (*p == '\\') {
		slashes++;
		p++;
	    }
	    if (*p == '"') {
		if ((slashes & 1) == 0) {
		    copy = 0;
		    if ((inquote) && (p[1] == '"')) {
			p++;
			copy = 1;
		    } else {
			inquote = !inquote;
		    }
		}
		slashes >>= 1;
	    }

	    while (slashes) {
		*arg = '\\';
		arg++;
		slashes--;
	    }

	    if ((*p == '\0') || (!inquote &&
		    ((*p == ' ') || (*p == '\t')))) {	/* INTL: ISO space. */
		break;
	    }
	    if (copy != 0) {
		*arg = *p;
		arg++;
	    }
	    p++;
	}
	*arg = '\0';
	argSpace = arg + 1;
    }
    argv[argc] = NULL;

    *argcPtr = argc;
    *argvPtr = argv;
}
#endif /* TCL_BROKEN_MAINARGS */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
