/*
 * odbcStubInit.c --
 *
 *	Stubs tables for the foreign ODBC libraries so that
 *	Tcl extensions can use them without the linker's knowing about them.
 *
 * @CREATED@ 2018-05-12 16:18:48Z by genExtStubs.tcl from odbcStubDefs.txt
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
#include "fakesql.h"

/*
 * Static data used in this file
 */

/*
 * Names of the libraries that might contain the ODBC API
 */

static const char *const odbcStubLibNames[] = {
#if defined(__APPLE__)
    "libiodbc.2",
#elif defined(__OpenBSD__)
    "libiodbc",
#else
    "odbc32", "odbc", "libodbc32", "libodbc", "libiodbc",
#endif
    NULL
    /* @END@ */
};
static const char *const odbcOptLibNames[] = {
#if defined(__APPLE__)
    "libiodbcinst.2",
#elif defined(__OpenBSD__)
    "libiodbcinst",
#else
    "odbccp", "odbccp32", "odbcinst",
    "libodbccp", "libodbccp32", "libodbcinst", "libiodbcinst",
#endif
    NULL
};

/*
 * Names of the functions that we need from ODBC
 */

static const char *const odbcSymbolNames[] = {
    /* @SYMNAMES@: DO NOT EDIT THESE NAMES */
    "SQLAllocHandle",
    "SQLBindParameter",
    "SQLCloseCursor",
    "SQLColumnsW",
    "SQLDataSourcesW",
    "SQLDescribeColW",
    "SQLDescribeParam",
    "SQLDisconnect",
    "SQLDriverConnectW",
    "SQLDriversW",
    "SQLEndTran",
    "SQLExecute",
    "SQLFetch",
    "SQLForeignKeysW",
    "SQLFreeHandle",
    "SQLGetConnectAttr",
    "SQLGetData",
    "SQLGetDiagFieldA",
    "SQLGetDiagRecW",
    "SQLGetInfoW",
    "SQLGetTypeInfo",
    "SQLMoreResults",
    "SQLNumParams",
    "SQLNumResultCols",
    "SQLPrepareW",
    "SQLPrimaryKeysW",
    "SQLRowCount",
    "SQLSetConnectAttr",
    "SQLSetConnectOption",
    "SQLSetEnvAttr",
    "SQLTablesW",
    NULL
    /* @END@ */
};

/*
 * Table containing pointers to the functions named above.
 */

static odbcStubDefs odbcStubsTable;
const odbcStubDefs* odbcStubs = &odbcStubsTable;

/*
 * Pointers to optional functions in ODBCINST
 */

BOOL (INSTAPI* SQLConfigDataSourceW)(HWND, WORD, LPCWSTR, LPCWSTR)
= NULL;
BOOL (INSTAPI* SQLConfigDataSource)(HWND, WORD, LPCSTR, LPCSTR)
= NULL;
BOOL (INSTAPI* SQLInstallerError)(WORD, DWORD*, LPSTR, WORD, WORD*)
= NULL;

/*
 *-----------------------------------------------------------------------------
 *
 * OdbcInitStubs --
 *
 *	Initialize the Stubs table for the ODBC API
 *
 * Results:
 *	Returns the handle to the loaded ODBC client library, or NULL
 *	if the load is unsuccessful. Leaves an error message in the
 *	interpreter.
 *
 *-----------------------------------------------------------------------------
 */

MODULE_SCOPE Tcl_LoadHandle
OdbcInitStubs(Tcl_Interp* interp,
				/* Tcl interpreter */
	      Tcl_LoadHandle* handle2Ptr)
				/* Pointer to a second load handle
				 * that represents the ODBCINST library */
{
    int i;
    int status;			/* Status of Tcl library calls */
    Tcl_Obj* path;		/* Path name of a module to be loaded */
    Tcl_Obj* shlibext;		/* Extension to use for load modules */
    Tcl_LoadHandle handle = NULL;
				/* Handle to a load module */

    SQLConfigDataSourceW = NULL;
    SQLConfigDataSource = NULL;
    SQLInstallerError = NULL;

    /*
     * Determine the shared library extension
     */
    status = Tcl_EvalEx(interp, "::info sharedlibextension", -1,
			TCL_EVAL_GLOBAL);
    if (status != TCL_OK) return NULL;
    shlibext = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(shlibext);

    /*
     * Walk the list of possible library names to find an ODBC client
     */
    status = TCL_ERROR;
    for (i = 0; status == TCL_ERROR && odbcStubLibNames[i] != NULL; ++i) {
	path = Tcl_NewStringObj(odbcStubLibNames[i], -1);
	Tcl_AppendObjToObj(path, shlibext);
	Tcl_IncrRefCount(path);
	Tcl_ResetResult(interp);

	/*
	 * Try to load a client library and resolve the ODBC API within it.
	 */
	status = Tcl_LoadFile(interp, path, odbcSymbolNames, 0,
			      (void*)odbcStubs, &handle);
	Tcl_DecrRefCount(path);
    }

    /*
     * If a client library is found, then try to load ODBCINST as well.
     */
    if (status == TCL_OK) {
	int status2 = TCL_ERROR;
	for (i = 0; status2 == TCL_ERROR && odbcOptLibNames[i] != NULL; ++i) {
	    path = Tcl_NewStringObj(odbcOptLibNames[i], -1);
	    Tcl_AppendObjToObj(path, shlibext);
	    Tcl_IncrRefCount(path);
	    status2 = Tcl_LoadFile(interp, path, NULL, 0, NULL, handle2Ptr);
	    if (status2 == TCL_OK) {
		SQLConfigDataSourceW =
		    (BOOL (INSTAPI*)(HWND, WORD, LPCWSTR, LPCWSTR))
		    Tcl_FindSymbol(NULL, *handle2Ptr, "SQLConfigDataSourceW");
		if (SQLConfigDataSourceW == NULL) {
		    SQLConfigDataSource =
			(BOOL (INSTAPI*)(HWND, WORD, LPCSTR, LPCSTR))
			Tcl_FindSymbol(NULL, *handle2Ptr,
				       "SQLConfigDataSource");
		}
		SQLInstallerError =
		    (BOOL (INSTAPI*)(WORD, DWORD*, LPSTR, WORD, WORD*))
		    Tcl_FindSymbol(NULL, *handle2Ptr, "SQLInstallerError");
	    } else {
		Tcl_ResetResult(interp);
	    }
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
