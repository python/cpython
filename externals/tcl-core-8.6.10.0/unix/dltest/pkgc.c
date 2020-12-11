/*
 * pkgc.c --
 *
 *	This file contains a simple Tcl package "pkgc" that is intended for
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
 * TCL_STORAGE_CLASS is set unconditionally to DLLEXPORT because the
 * Pkgc_Init declaration is in the source file itself, which is only
 * accessed when we are building a library.
 */
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

/*
 * Prototypes for procedures defined later in this file:
 */

static int    Pkgc_SubObjCmd(ClientData clientData,
		Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
static int    Pkgc_UnsafeObjCmd(ClientData clientData,
		Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);

/*
 *----------------------------------------------------------------------
 *
 * Pkgc_SubObjCmd --
 *
 *	This procedure is invoked to process the "pkgc_sub" Tcl command. It
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

static int
Pkgc_SubObjCmd(
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
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(first - second));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgc_UnsafeCmd --
 *
 *	This procedure is invoked to process the "pkgc_unsafe" Tcl command. It
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
Pkgc_UnsafeObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_SetObjResult(interp, Tcl_NewStringObj("unsafe command invoked", -1));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgc_Init --
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

EXTERN int
Pkgc_Init(
    Tcl_Interp *interp)		/* Interpreter in which the package is to be
				 * made available. */
{
    int code;

    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    code = Tcl_PkgProvide(interp, "Pkgc", "1.7.2");
    if (code != TCL_OK) {
	return code;
    }
    Tcl_CreateObjCommand(interp, "pkgc_sub", Pkgc_SubObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "pkgc_unsafe", Pkgc_UnsafeObjCmd, NULL,
	    NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgc_SafeInit --
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

EXTERN int
Pkgc_SafeInit(
    Tcl_Interp *interp)		/* Interpreter in which the package is to be
				 * made available. */
{
    int code;

    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    code = Tcl_PkgProvide(interp, "Pkgc", "1.7.2");
    if (code != TCL_OK) {
	return code;
    }
    Tcl_CreateObjCommand(interp, "pkgc_sub", Pkgc_SubObjCmd, NULL, NULL);
    return TCL_OK;
}
