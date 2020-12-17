/*
 * ORIGINAL SOURCE: tk/generic/tkStubLib.c, version 1.9 2004/03/17
 */

#include "tclOOInt.h"

MODULE_SCOPE const TclOOStubs *tclOOStubsPtr;
MODULE_SCOPE const TclOOIntStubs *tclOOIntStubsPtr;

const TclOOStubs *tclOOStubsPtr = NULL;
const TclOOIntStubs *tclOOIntStubsPtr = NULL;

/*
 *----------------------------------------------------------------------
 *
 * TclOOInitializeStubs --
 *	Load the tclOO package, initialize stub table pointer. Do not call
 *	this function directly, use Tcl_OOInitStubs() macro instead.
 *
 * Results:
 *	The actual version of the package that satisfies the request, or NULL
 *	to indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */

#undef TclOOInitializeStubs

MODULE_SCOPE const char *
TclOOInitializeStubs(
    Tcl_Interp *interp,
    const char *version)
{
    int exact = 0;
    const char *packageName = "TclOO";
    const char *errMsg = NULL;
    TclOOStubs *stubsPtr = NULL;
    const char *actualVersion = tclStubsPtr->tcl_PkgRequireEx(interp,
	    packageName, version, exact, &stubsPtr);

    if (actualVersion == NULL) {
	return NULL;
    }
    if (stubsPtr == NULL) {
	errMsg = "missing stub table pointer";
    } else {
	tclOOStubsPtr = stubsPtr;
	if (stubsPtr->hooks) {
	    tclOOIntStubsPtr = stubsPtr->hooks->tclOOIntStubs;
	} else {
	    tclOOIntStubsPtr = NULL;
	}
	return actualVersion;
    }
    tclStubsPtr->tcl_ResetResult(interp);
    tclStubsPtr->tcl_AppendResult(interp, "Error loading ", packageName,
	    " (requested version ", version, ", actual version ",
	    actualVersion, "): ", errMsg, NULL);
    return NULL;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
