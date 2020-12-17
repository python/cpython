/*
 * tkUnixInit.c --
 *
 *	This file contains Unix-specific interpreter initialization functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"

#ifdef HAVE_COREFOUNDATION
static int		GetLibraryPath(Tcl_Interp *interp);
#else
#define GetLibraryPath(dummy)	(void)0
#endif /* HAVE_COREFOUNDATION */

/*
 *----------------------------------------------------------------------
 *
 * TkpInit --
 *
 *	Performs Unix-specific interpreter initialization related to the
 *      tk_library variable.
 *
 * Results:
 *	Returns a standard Tcl result. Leaves an error message or result in
 *	the interp's result.
 *
 * Side effects:
 *	Sets "tk_library" Tcl variable, runs "tk.tcl" script.
 *
 *----------------------------------------------------------------------
 */

int
TkpInit(
    Tcl_Interp *interp)
{
    TkCreateXEventSource();
    GetLibraryPath(interp);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetAppName --
 *
 *	Retrieves the name of the current application from a platform specific
 *	location. For Unix, the application name is the tail of the path
 *	contained in the tcl variable argv0.
 *
 * Results:
 *	Returns the application name in the given Tcl_DString.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkpGetAppName(
    Tcl_Interp *interp,
    Tcl_DString *namePtr)	/* A previously initialized Tcl_DString. */
{
    const char *p, *name;

    name = Tcl_GetVar2(interp, "argv0", NULL, TCL_GLOBAL_ONLY);
    if ((name == NULL) || (*name == 0)) {
	name = "tk";
    } else {
	p = strrchr(name, '/');
	if (p != NULL) {
	    name = p+1;
	}
    }
    Tcl_DStringAppend(namePtr, name, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayWarning --
 *
 *	This routines is called from Tk_Main to display warning messages that
 *	occur during startup.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates messages on stdout.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayWarning(
    const char *msg,		/* Message to be displayed. */
    const char *title)		/* Title of warning. */
{
    Tcl_Channel errChannel = Tcl_GetStdChannel(TCL_STDERR);

    if (errChannel) {
	Tcl_WriteChars(errChannel, title, -1);
	Tcl_WriteChars(errChannel, ": ", 2);
	Tcl_WriteChars(errChannel, msg, -1);
	Tcl_WriteChars(errChannel, "\n", 1);
    }
}

#ifdef HAVE_COREFOUNDATION

/*
 *----------------------------------------------------------------------
 *
 * GetLibraryPath --
 *
 *	If we have a bundle structure for the Tk installation, then check
 *	there first to see if we can find the libraries there.
 *
 * Results:
 *	TCL_OK if we have found the tk library; TCL_ERROR otherwise.
 *
 * Side effects:
 *	Same as for Tcl_MacOSXOpenVersionedBundleResources.
 *
 *----------------------------------------------------------------------
 */

static int
GetLibraryPath(
    Tcl_Interp *interp)
{
#ifdef TK_FRAMEWORK
    int foundInFramework = TCL_ERROR;
    char tkLibPath[PATH_MAX + 1];

    foundInFramework = Tcl_MacOSXOpenVersionedBundleResources(interp,
	    "com.tcltk.tklibrary", TK_FRAMEWORK_VERSION, 0, PATH_MAX,
	    tkLibPath);
    if (tkLibPath[0] != '\0') {
        Tcl_SetVar2(interp, "tk_library", NULL, tkLibPath, TCL_GLOBAL_ONLY);
    }
    return foundInFramework;
#else
    return TCL_ERROR;
#endif
}
#endif /* HAVE_COREFOUNDATION */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
