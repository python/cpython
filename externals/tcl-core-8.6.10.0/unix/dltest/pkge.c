/*
 * pkge.c --
 *
 *	This file contains a simple Tcl package "pkge" that is intended for
 *	testing the Tcl dynamic loading facilities. Its Init procedure returns
 *	an error in order to test how this is handled.
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
 * Pkge_Init declaration is in the source file itself, which is only
 * accessed when we are building a library.
 */
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT


/*
 *----------------------------------------------------------------------
 *
 * Pkge_Init --
 *
 *	This is a package initialization procedure, which is called by Tcl
 *	when this package is to be added to an interpreter.
 *
 * Results:
 *	Returns TCL_ERROR and leaves an error message in interp->result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

EXTERN int
Pkge_Init(
    Tcl_Interp *interp)		/* Interpreter in which the package is to be
				 * made available. */
{
    static const char script[] = "if 44 {open non_existent}";

    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    return Tcl_EvalEx(interp, script, -1, 0);
}
