/*
 * tdbcStubLib.c --
 *
 *	Stubs table initialization wrapper for Tcl DataBase Connectivity
 *	(TDBC).
 *
 * Copyright (c) 2008 by Kevin B. Kenny.
 *
 * Please refer to the file, 'license.terms' for the conditions on
 * redistribution of this file and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 *
 *-----------------------------------------------------------------------------
 */

#include <tcl.h>

#define USE_TDBC_STUBS 1
#include "tdbc.h"

MODULE_SCOPE const TdbcStubs *tdbcStubsPtr;

const TdbcStubs *tdbcStubsPtr = NULL;

/*
 *-----------------------------------------------------------------------------
 *
 * TdbcInitializeStubs --
 *
 *	Loads the Tdbc package and initializes its Stubs table pointer.
 *
 * Client code should not call this function directly; instead, it should
 * use the Tdbc_InitStubs macro.
 *
 * Results:
 *	Returns the actual version of the Tdbc package that has been
 *	loaded, or NULL if an error occurs.
 *
 * Side effects:
 *	Sets the Stubs table pointer, or stores an error message in the
 *	interpreter's result.
 *
 *-----------------------------------------------------------------------------
 */

const char*
TdbcInitializeStubs(
    Tcl_Interp* interp,		/* Tcl interpreter */
    const char* version,	/* Version of TDBC requested */
    int epoch,			/* Epoch number of the Stubs table */
    int revision		/* Revision number within the epoch */
) {
    const int exact = 0;	/* Set this to 1 to require exact version */
    const char* packageName = "tdbc";
				/* Name of the package */
    const char* errorMsg = NULL;
				/* Error message if an error occurs */
    ClientData clientData = NULL;
				/* Client data for the package */
    const char* actualVersion;  /* Actual version of the package */
    const TdbcStubs* stubsPtr;	/* Stubs table for the public API */

    /* Load the package */

    actualVersion =
	Tcl_PkgRequireEx(interp, packageName, version, exact, &clientData);

    if (clientData == NULL) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "Error loading ", packageName, " package: "
			 "package not present, incomplete or misconfigured.",
			 (char*) NULL);
	return NULL;
    }

    /* Test that all version information matches the request */

    if (actualVersion == NULL) {
	return NULL;
    } else {
	stubsPtr = (const TdbcStubs*) clientData;
	if (stubsPtr->epoch != epoch) {
	    errorMsg = "mismatched epoch number";
	} else if (stubsPtr->revision < revision) {
	    errorMsg = "Stubs table provides too early a revision";
	} else {

	    /* Everything is ok. Return the package information */

	    tdbcStubsPtr = stubsPtr;
	    return actualVersion;
	}
    }

    /* Try to explain what went wrong when a mismatched version is found. */

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "Error loading ", packageName, " package "
		     "(requested version \"", version, "\", loaded version \"",
		     actualVersion, "\"): ", errorMsg, (char*) NULL);
    return NULL;

}
