/*
 * tclXtTest.c --
 *
 *	Contains commands for Xt notifier specific tests on Unix.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef USE_TCL_STUBS
#   define USE_TCL_STUBS
#endif
#include <X11/Intrinsic.h>
#include "tcl.h"

static Tcl_ObjCmdProc TesteventloopCmd;
extern DLLEXPORT Tcl_PackageInitProc Tclxttest_Init;

/*
 * Functions defined in tclXtNotify.c for use by users of the Xt Notifier:
 */

extern void	InitNotifier(void);
extern XtAppContext	TclSetAppContext(XtAppContext ctx);

/*
 *----------------------------------------------------------------------
 *
 * Tclxttest_Init --
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
Tclxttest_Init(
    Tcl_Interp *interp)		/* Interpreter for application. */
{
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }
    XtToolkitInitialize();
    InitNotifier();
    Tcl_CreateObjCommand(interp, "testeventloop", TesteventloopCmd,
	    NULL, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TesteventloopCmd --
 *
 *	This procedure implements the "testeventloop" command. It is used to
 *	test the Tcl notifier from an "external" event loop (i.e. not
 *	Tcl_DoOneEvent()).
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TesteventloopCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static int *framePtr = NULL;/* Pointer to integer on stack frame of
				 * innermost invocation of the "wait"
				 * subcommand. */

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ...");
	return TCL_ERROR;
    }
    if (strcmp(Tcl_GetString(objv[1]), "done") == 0) {
	*framePtr = 1;
    } else if (strcmp(Tcl_GetString(objv[1]), "wait") == 0) {
	int *oldFramePtr;
	int done;
	int oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);

	/*
	 * Save the old stack frame pointer and set up the current frame.
	 */

	oldFramePtr = framePtr;
	framePtr = &done;

	/*
	 * Enter an Xt event loop until the flag changes. Note that we do not
	 * explicitly call Tcl_ServiceEvent().
	 */

	done = 0;
	while (!done) {
	    XtAppProcessEvent(TclSetAppContext(NULL), XtIMAll);
	}
	(void) Tcl_SetServiceMode(oldMode);
	framePtr = oldFramePtr;
    } else {
	Tcl_AppendResult(interp, "bad option \"", Tcl_GetString(objv[1]),
		"\": must be done or wait", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
