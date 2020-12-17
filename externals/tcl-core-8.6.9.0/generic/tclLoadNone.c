/*
 * tclLoadNone.c --
 *
 *	This procedure provides a version of the TclpDlopen for use in
 *	systems that don't support dynamic loading; it just returns an error.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 *----------------------------------------------------------------------
 *
 * TclpDlopen --
 *
 *	This procedure is called to carry out dynamic loading of binary code;
 *	it is intended for use only on systems that don't support dynamic
 *	loading (it returns an error).
 *
 * Results:
 *	The result is TCL_ERROR, and an error message is left in the interp's
 *	result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpDlopen(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Obj *pathPtr,		/* Name of the file containing the desired
				 * code (UTF-8). */
    Tcl_LoadHandle *loadHandle,	/* Filled with token for dynamically loaded
				 * file which will be passed back to
				 * (*unloadProcPtr)() to unload the file. */
    Tcl_FSUnloadFileProc **unloadProcPtr,
				/* Filled with address of Tcl_FSUnloadFileProc
				 * function which should be used for this
				 * file. */
    int flags)
{
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"dynamic loading is not currently available on this system",
		-1));
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGuessPackageName --
 *
 *	If the "load" command is invoked without providing a package name,
 *	this procedure is invoked to try to figure it out.
 *
 * Results:
 *	Always returns 0 to indicate that we couldn't figure out a package
 *	name; generic code will then try to guess the package from the file
 *	name. A return value of 1 would have meant that we figured out the
 *	package name and put it in bufPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclGuessPackageName(
    const char *fileName,	/* Name of file containing package (already
				 * translated to local form if needed). */
    Tcl_DString *bufPtr)	/* Initialized empty dstring. Append package
				 * name to this if possible. */
{
    return 0;
}

/*
 * These functions are fallbacks if we somehow determine that the platform can
 * do loading from memory but the user wishes to disable it. They just report
 * (gracefully) that they fail.
 */

#ifdef TCL_LOAD_FROM_MEMORY

MODULE_SCOPE void *
TclpLoadMemoryGetBuffer(
    Tcl_Interp *interp,		/* Dummy: unused by this implementation */
    int size)			/* Dummy: unused by this implementation */
{
    return NULL;
}

MODULE_SCOPE int
TclpLoadMemory(
    Tcl_Interp *interp,		/* Used for error reporting. */
    void *buffer,		/* Dummy: unused by this implementation */
    int size,			/* Dummy: unused by this implementation */
    int codeSize,		/* Dummy: unused by this implementation */
    Tcl_LoadHandle *loadHandle,	/* Dummy: unused by this implementation */
    Tcl_FSUnloadFileProc **unloadProcPtr,
				/* Dummy: unused by this implementation */
    int flags)
				/* Dummy: unused by this implementation */
{
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("dynamic loading from memory "
		"is not available on this system", -1));
    }
    return TCL_ERROR;
}

#endif /* TCL_LOAD_FROM_MEMORY */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
