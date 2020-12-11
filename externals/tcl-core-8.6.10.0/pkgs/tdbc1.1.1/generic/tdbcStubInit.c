/*
 * tdbcStubInit.c --
 *
 *	Initialization of the Stubs table for the exported API of
 *	Tcl DataBase Connectivity (TDBC)
 *
 * Copyright (c) 2008 by Kevin B. Kenny.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 *
 */

#include "tdbcInt.h"

MODULE_SCOPE const TdbcStubs tdbcStubs;

#define Tdbc_Init_ Tdbc_Init

/* !BEGIN!: Do not edit below this line. */

const TdbcStubs tdbcStubs = {
    TCL_STUB_MAGIC,
    TDBC_STUBS_EPOCH,
    TDBC_STUBS_REVISION,
    0,
    Tdbc_Init_, /* 0 */
    Tdbc_TokenizeSql, /* 1 */
    Tdbc_MapSqlState, /* 2 */
};

/* !END!: Do not edit above this line. */
