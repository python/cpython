# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: namesp.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# This file tests the pixmap image reader
#

proc About {} {
    return "This file performs test on name space"
}

proc Test {} {
    namespace mySpace {
	variable hsl ".hsl"
	proc creatHSL {} {
	    global hsl
	    tixScrolledHList $hsl
	}
	proc packHSL {} {
	    global hsl
	    pack $hsl
	}
    }
    mySpace::creatHSL
    mySpace::packHSL
}
