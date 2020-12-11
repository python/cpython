# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
# auto_loaded.tcl --
#
#       This file is auto-loaded by various test code to test whether
#       the Tix code is compatible with Tcl autoloading.
#
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# $Id: auto_loaded.tcl,v 1.2 2002/11/13 21:12:17 idiscovery Exp $

proc tixTestClass_method:foo {w args} {
    upvar #0 $w data

    return returned_by_tixTestClass_method:foo
}

