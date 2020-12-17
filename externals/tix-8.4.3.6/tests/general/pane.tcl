# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: pane.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# pane.tcl --
#
#	Test the PanedWindow widget.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Test the PanedWindow widget."
}

proc Test {} {
    TestBlock pane-1.1 {tixPanedWindow -expand} {
	tixPanedWindow .p -orient horizontal
	pack .p -expand yes -fill both
	set p1 [.p add pane1 -expand 0.3]
	set p2 [.p add pane2 -expand 1]
	set p3 [.p add pane3 -size 20]
	.p config -width 300 -height 200
	update
	.p config -width 500
	update
	.p config -width 200
	update
    }
}
