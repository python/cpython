/*
 * winMain.c --
 *
 *	Provides a default version of the main program and Tcl_AppInit
 *	procedure for wish and other Tk-based applications.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tk.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <locale.h>
#include <stdlib.h>
#include <tchar.h>

#if defined(__GNUC__)
int _CRT_glob = 0;
#endif /* __GNUC__ */

#ifdef TK_TEST
extern Tcl_PackageInitProc Tktest_Init;
#endif /* TK_TEST */

#if defined(STATIC_BUILD) && TCL_USE_STATIC_PACKAGES
extern Tcl_PackageInitProc Registry_Init;
extern Tcl_PackageInitProc Dde_Init;
extern Tcl_PackageInitProc Dde_SafeInit;
#endif

#ifdef TCL_BROKEN_MAINARGS
static void setargv(int *argcPtr, TCHAR ***argvPtr);
#endif

/*
 * Forward declarations for procedures defined later in this file:
 */

static BOOL consoleRequired = TRUE;

/*
 * The following #if block allows you to change the AppInit function by using
 * a #define of TCL_LOCAL_APPINIT instead of rewriting this entire file. The
 * #if checks for that #define and uses Tcl_AppInit if it doesn't exist.
 */

#ifndef TK_LOCAL_APPINIT
#define TK_LOCAL_APPINIT Tcl_AppInit
#endif
#ifndef MODULE_SCOPE
#   define MODULE_SCOPE extern
#endif
MODULE_SCOPE int TK_LOCAL_APPINIT(Tcl_Interp *interp);

/*
 * The following #if block allows you to change how Tcl finds the startup
 * script, prime the library or encoding paths, fiddle with the argv, etc.,
 * without needing to rewrite Tk_Main()
 */

#ifdef TK_LOCAL_MAIN_HOOK
MODULE_SCOPE int TK_LOCAL_MAIN_HOOK(int *argc, TCHAR ***argv);
#endif

/* Make sure the stubbed variants of those are never used. */
#undef Tcl_ObjSetVar2
#undef Tcl_NewStringObj

/*
 *----------------------------------------------------------------------
 *
 * _tWinMain --
 *
 *	Main entry point from Windows.
 *
 * Results:
 *	Returns false if initialization fails, otherwise it never returns.
 *
 * Side effects:
 *	Just about anything, since from here we call arbitrary Tcl code.
 *
 *----------------------------------------------------------------------
 */

int APIENTRY
#ifdef TCL_BROKEN_MAINARGS
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpszCmdLine,
    int nCmdShow)
#else
_tWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR lpszCmdLine,
    int nCmdShow)
#endif
{
    TCHAR **argv;
    int argc;
    TCHAR *p;

    /*
     * Create the console channels and install them as the standard channels.
     * All I/O will be discarded until Tk_CreateConsoleWindow is called to
     * attach the console to a text widget.
     */

    consoleRequired = TRUE;

    /*
     * Set up the default locale to be standard "C" locale so parsing is
     * performed correctly.
     */

    setlocale(LC_ALL, "C");

    /*
     * Get our args from the c-runtime. Ignore lpszCmdLine.
     */

#if defined(TCL_BROKEN_MAINARGS)
    setargv(&argc, &argv);
#else
    argc = __argc;
    argv = __targv;
#endif

    /*
     * Forward slashes substituted for backslashes.
     */

    for (p = argv[0]; *p != '\0'; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }

#ifdef TK_LOCAL_MAIN_HOOK
    TK_LOCAL_MAIN_HOOK(&argc, &argv);
#endif

    Tk_Main(argc, argv, TK_LOCAL_APPINIT);
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
    if (Tk_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);

    /*
     * Initialize the console only if we are running as an interactive
     * application.
     */

    if (consoleRequired) {
	if (Tk_CreateConsoleWindow(interp) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    }
#if defined(STATIC_BUILD) && TCL_USE_STATIC_PACKAGES
    if (Registry_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "registry", Registry_Init, 0);

    if (Dde_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "dde", Dde_Init, Dde_SafeInit);
#endif

#ifdef TK_TEST
    if (Tktest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tktest", Tktest_Init, 0);
#endif /* TK_TEST */

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
     * Call Tcl_CreateObjCommand for application-specific commands, if they
     * weren't already created by the init procedures called above.
     */

    /*
     * Specify a user-specific startup file to invoke if the application is
     * run interactively. Typically the startup file is "~/.apprc" where "app"
     * is the name of the application. If this line is deleted then no user-
     * specific startup file will be run under any conditions.
     */

    Tcl_ObjSetVar2(interp, Tcl_NewStringObj("tcl_rcFileName", -1), NULL,
	    Tcl_NewStringObj("~/wishrc.tcl", -1), TCL_GLOBAL_ONLY);
    return TCL_OK;
}

#if defined(TK_TEST)
/*
 *----------------------------------------------------------------------
 *
 * _tmain --
 *
 *	Main entry point from the console.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never returns
 *	either.
 *
 * Side effects:
 *	Whatever the applications does.
 *
 *----------------------------------------------------------------------
 */

#ifdef TCL_BROKEN_MAINARGS
int
main(
    int argc,
    char **dummy)
{
    TCHAR **argv;
#else
int
_tmain(
    int argc,
    TCHAR **argv)
{
#endif
    /*
     * Set up the default locale to be standard "C" locale so parsing is
     * performed correctly.
     */

    setlocale(LC_ALL, "C");

#ifdef TCL_BROKEN_MAINARGS
    /*
     * Get our args from the c-runtime. Ignore argc/argv.
     */

    setargv(&argc, &argv);
#endif
    /*
     * Console emulation widget not required as this entry is from the
     * console subsystem, thus stdin,out,err already have end-points.
     */

    consoleRequired = FALSE;

#ifdef TK_LOCAL_MAIN_HOOK
    TK_LOCAL_MAIN_HOOK(&argc, &argv);
#endif

    Tk_Main(argc, argv, Tcl_AppInit);
    return 0;
}
#endif /* !__GNUC__ || TK_TEST */


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
