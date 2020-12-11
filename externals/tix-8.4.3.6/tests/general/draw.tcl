# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: draw.tcl,v 1.2 2002/11/13 21:12:17 idiscovery Exp $
#
# draw.tcl --
#
#	Test the drawing functions in Tix.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Test the drawing functions in Tix."
}

proc Test {} {
    TestBlock draw-1.1 {tixTmpLine} {
	tixTmpLine 0 50 300 50	
	tixTmpLine 0 50 300 50
	tixTmpLine 0 50 300 50 .	
	tixTmpLine 0 50 300 50 .
    }
}
