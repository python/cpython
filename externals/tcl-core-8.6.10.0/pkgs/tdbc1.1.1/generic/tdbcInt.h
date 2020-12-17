/*
 * tdbcInt.h --
 *
 *	Declarations of the public API for Tcl DataBase Connectivity (TDBC)
 *
 * Copyright (c) 2006 by Kevin B. Kenny
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 *
 *-----------------------------------------------------------------------------
 */
#ifndef TDBCINT_H_INCLUDED
#define TDBCINT_H_INCLUDED 1

#include "tdbc.h"

/*
 * Used to tag functions that are only to be visible within the module being
 * built and not outside it (where this is supported by the linker).
 */

#ifndef MODULE_SCOPE
#   ifdef __cplusplus
#	define MODULE_SCOPE extern "C"
#   else
#	define MODULE_SCOPE extern
#   endif
#endif

/*
 * Linkage to procedures not exported from this module
 */

MODULE_SCOPE int TdbcTokenizeObjCmd(ClientData clientData, Tcl_Interp* interp,
				    int objc, Tcl_Obj *const objv[]);

#endif
