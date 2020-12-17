/*
 * We need to ensure that we use the tcl stub macros so that this file
 * contains no references to any of the tcl stub functions.
 */

#undef USE_TCL_STUBS
#define USE_TCL_STUBS

#include "tk.h"

#define USE_TTK_STUBS 1
#include "ttkTheme.h"

MODULE_SCOPE const TtkStubs *ttkStubsPtr;
const TtkStubs *ttkStubsPtr = NULL;

/*
 *----------------------------------------------------------------------
 *
 * TtkInitializeStubs --
 *	Load the Ttk package, initialize stub table pointer.
 *	Do not call this function directly, use Ttk_InitStubs() macro instead.
 *
 * Results:
 *	The actual version of the package that satisfies the request, or
 *	NULL to indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointer.
 *
 */
MODULE_SCOPE const char *
TtkInitializeStubs(
    Tcl_Interp *interp, const char *version, int epoch, int revision)
{
    int exact = 0;
    const char *packageName = "Ttk";
    const char *errMsg = NULL;
    ClientData pkgClientData = NULL;
    const char *actualVersion = Tcl_PkgRequireEx(
	interp, packageName, version, exact, &pkgClientData);
    const TtkStubs *stubsPtr = pkgClientData;

    if (!actualVersion) {
	return NULL;
    }

    if (!stubsPtr) {
	errMsg = "missing stub table pointer";
	goto error;
    }
    if (stubsPtr->epoch != epoch) {
	errMsg = "epoch number mismatch";
	goto error;
    }
    if (stubsPtr->revision < revision) {
	errMsg = "require later revision";
	goto error;
    }

    ttkStubsPtr = stubsPtr;
    return actualVersion;

error:
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp,
	"Error loading ", packageName, " package",
	" (requested version '", version,
	"', loaded version '", actualVersion, "'): ",
	errMsg, 
	NULL);
    return NULL;
}

