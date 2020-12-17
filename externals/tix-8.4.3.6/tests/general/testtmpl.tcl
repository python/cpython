# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: testtmpl.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# testtmpl.tcl --
#
#	Test Template:
#
#	This program is used as the first test: see whether we can execute any
#	case at all.
#
#	This program is also used as a template file for writing other test
#	cases.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Testing whether the test program starts up properly"
}

proc Test {} {
    TestBlock testtmpl-1.1 {NULL test} {
	#
	# If this fails, we are in big trouble and probably none of the
	# tests can pass. Abort all the tests
	#
    } 1 abortall
}
