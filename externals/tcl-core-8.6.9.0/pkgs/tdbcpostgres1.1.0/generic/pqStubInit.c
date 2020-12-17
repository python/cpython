/*
 * pqStubInit.c --
 *
 *	Stubs tables for the foreign PostgreSQL libraries so that
 *	Tcl extensions can use them without the linker's knowing about them.
 *
 * @CREATED@ 2015-06-26 12:55:15Z by genExtStubs.tcl from ../generic/pqStubDefs.txt
 *
 * Copyright (c) 2010 by Kevin B. Kenny.
 *
 * Please refer to the file, 'license.terms' for the conditions on
 * redistribution of this file and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *-----------------------------------------------------------------------------
 */

#include <tcl.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "fakepq.h"

/*
 * Static data used in this file
 */

/*
 * Names of the libraries that might contain the PostgreSQL API
 */

#if defined(__CYGWIN__) && !defined(LIBPREFIX)
# define LIBPREFIX "cyg"
#else
# define LIBPREFIX "lib"
#endif

static const char *const pqStubLibNames[] = {
    /* @LIBNAMES@: DO NOT EDIT THESE NAMES */
    "pq", NULL
    /* @END@ */
};

/* ABI Version numbers of the PostgreSQL API that we can cope with */

static const char pqSuffixes[][4] = {
    "", ".5"
};

/* Names of the functions that we need from PostgreSQL */

static const char *const pqSymbolNames[] = {
    /* @SYMNAMES@: DO NOT EDIT THESE NAMES */
    "pg_encoding_to_char",
    "PQclear",
    "PQclientEncoding",
    "PQcmdTuples",
    "PQconnectdb",
    "PQerrorMessage",
    "PQdescribePrepared",
    "PQexec",
    "PQexecPrepared",
    "PQdb",
    "PQfinish",
    "PQfname",
    "PQfnumber",
    "PQftype",
    "PQgetisnull",
    "PQgetlength",
    "PQgetvalue",
    "PQhost",
    "PQnfields",
    "PQnparams",
    "PQntuples",
    "PQoptions",
    "PQparamtype",
    "PQpass",
    "PQport",
    "PQprepare",
    "PQresultErrorField",
    "PQresultStatus",
    "PQsetClientEncoding",
    "PQsetNoticeProcessor",
    "PQstatus",
    "PQuser",
    "PQtty",
    NULL
    /* @END@ */
};

/*
 * Table containing pointers to the functions named above.
 */

static pqStubDefs pqStubsTable;
const pqStubDefs* pqStubs = &pqStubsTable;

/*
 *-----------------------------------------------------------------------------
 *
 * PQInitStubs --
 *
 *	Initialize the Stubs table for the PostgreSQL API
 *
 * Results:
 *	Returns the handle to the loaded PostgreSQL client library, or NULL
 *	if the load is unsuccessful. Leaves an error message in the
 *	interpreter.
 *
 *-----------------------------------------------------------------------------
 */

MODULE_SCOPE Tcl_LoadHandle
PostgresqlInitStubs(Tcl_Interp* interp)
{
    int i, j;
    int status;			/* Status of Tcl library calls */
    Tcl_Obj* path;		/* Path name of a module to be loaded */
    Tcl_Obj* shlibext;		/* Extension to use for load modules */
    Tcl_LoadHandle handle = NULL;
				/* Handle to a load module */

    /* Determine the shared library extension */

    status = Tcl_EvalEx(interp, "::info sharedlibextension", -1,
			TCL_EVAL_GLOBAL);
    if (status != TCL_OK) return NULL;
    shlibext = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(shlibext);

    /* Walk the list of possible library names to find an PostgreSQL client */

    status = TCL_ERROR;
    for (i = 0; status == TCL_ERROR && pqStubLibNames[i] != NULL; ++i) {
	for (j = 0; status == TCL_ERROR && (j < sizeof(pqSuffixes)/sizeof(pqSuffixes[0])); ++j) {
	    path = Tcl_NewStringObj(LIBPREFIX, -1);
	    Tcl_AppendToObj(path, pqStubLibNames[i], -1);
#ifdef __CYGWIN__
	    if (*pqSuffixes[j]) {
		Tcl_AppendToObj(path, "-", -1);
		Tcl_AppendToObj(path, pqSuffixes[j]+1, -1);
	    }
#endif
	    Tcl_AppendObjToObj(path, shlibext);
#ifndef __CYGWIN__
	    Tcl_AppendToObj(path, pqSuffixes[j], -1);
#endif
	    Tcl_IncrRefCount(path);

	    /* Try to load a client library and resolve symbols within it. */

	    Tcl_ResetResult(interp);
	    status = Tcl_LoadFile(interp, path, pqSymbolNames, 0,
			      &pqStubsTable, &handle);
	    Tcl_DecrRefCount(path);
	}
    }

    /*
     * Either we've successfully loaded a library (status == TCL_OK),
     * or we've run out of library names (in which case status==TCL_ERROR
     * and the error message reflects the last unsuccessful load attempt).
     */
    Tcl_DecrRefCount(shlibext);
    if (status != TCL_OK) {
	return NULL;
    }
    return handle;
}
