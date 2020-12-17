/*
 * pkgb.c --
 *
 *	This file contains a simple Tcl package "pkgb" that is intended for
 *	testing the Tcl dynamic loading facilities. It can be used in both
 *	safe and unsafe interpreters.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#undef STATIC_BUILD
#include "tcl.h"

/*
 * Prototypes for procedures defined later in this file:
 */

static int    Pkgb_SubObjCmd(ClientData clientData,
		Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
static int    Pkgb_UnsafeObjCmd(ClientData clientData,
		Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
static int    Pkgb_DemoObjCmd(ClientData clientData,
		Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);

/*
 *----------------------------------------------------------------------
 *
 * Pkgb_SubObjCmd --
 *
 *	This procedure is invoked to process the "pkgb_sub" Tcl command. It
 *	expects two arguments and returns their difference.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

#ifndef Tcl_GetErrorLine
#   define Tcl_GetErrorLine(interp) ((interp)->errorLine)
#endif

static int
Pkgb_SubObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int first, second;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "num num");
	return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[1], &first) != TCL_OK)
	    || (Tcl_GetIntFromObj(interp, objv[2], &second) != TCL_OK)) {
	char buf[TCL_INTEGER_SPACE];
	sprintf(buf, "%d", Tcl_GetErrorLine(interp));
	Tcl_AppendResult(interp, " in line: ", buf, NULL);
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(first - second));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgb_UnsafeObjCmd --
 *
 *	This procedure is invoked to process the "pkgb_unsafe" Tcl command. It
 *	just returns a constant string.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
Pkgb_UnsafeObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_EvalEx(interp, "list unsafe command invoked", -1, TCL_EVAL_GLOBAL);
}

static int
Pkgb_DemoObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
#if (TCL_MAJOR_VERSION > 8) || (TCL_MINOR_VERSION > 4)
    Tcl_Obj *first;

    if (Tcl_ListObjIndex(NULL, Tcl_GetEncodingSearchPath(), 0, &first)
	    == TCL_OK) {
	Tcl_SetObjResult(interp, first);
    }
#else
    Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_GetDefaultEncodingDir(), -1));
#endif
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgb_Init --
 *
 *	This is a package initialization procedure, which is called by Tcl
 *	when this package is to be added to an interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

DLLEXPORT int
Pkgb_Init(
    Tcl_Interp *interp)		/* Interpreter in which the package is to be
				 * made available. */
{
    int code;

    if (Tcl_InitStubs(interp, "8.5-", 0) == NULL) {
	return TCL_ERROR;
    }
    code = Tcl_PkgProvide(interp, "Pkgb", "2.3");
    if (code != TCL_OK) {
	return code;
    }
    Tcl_CreateObjCommand(interp, "pkgb_sub", Pkgb_SubObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "pkgb_unsafe", Pkgb_UnsafeObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "pkgb_demo", Pkgb_DemoObjCmd, NULL, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgb_SafeInit --
 *
 *	This is a package initialization procedure, which is called by Tcl
 *	when this package is to be added to a safe interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

DLLEXPORT int
Pkgb_SafeInit(
    Tcl_Interp *interp)		/* Interpreter in which the package is to be
				 * made available. */
{
    int code;

    if (Tcl_InitStubs(interp, "8.5-", 0) == NULL) {
	return TCL_ERROR;
    }
    code = Tcl_PkgProvide(interp, "Pkgb", "2.3");
    if (code != TCL_OK) {
	return code;
    }
    Tcl_CreateObjCommand(interp, "pkgb_sub", Pkgb_SubObjCmd, NULL, NULL);
    return TCL_OK;
}
