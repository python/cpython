# -*- tcl -*-
#
# tdbc.decls --
#
#	Declarations of Stubs-exported functions from the support layer
#	of Tcl DataBase Connectivity (TDBC).
#
# Copyright (c) 2008 by Kevin B. Kenny.
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
#  RCS: @(#) $Id$
#
#-----------------------------------------------------------------------------

library tdbc
interface tdbc
epoch 0
scspec TDBCAPI

# The public API for TDBC

# Just a dummy definition, meant to keep TDBC_STUBS_REVISION the same
declare 0 current {
    int Tdbc_Init_(Tcl_Interp* interp)
}
declare 1 current {
    Tcl_Obj* Tdbc_TokenizeSql(Tcl_Interp* interp, const char* statement)
}
declare 2 current {
    const char* Tdbc_MapSqlState(const char* sqlstate)
}
