# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: StatBar.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# StatBar.tcl --
#
#	The StatusBar of an application.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixStatusBar {
    -classname TixStatusBar
    -superclass tixPrimitive
    -method {
    }
    -flag {
	-fields
    }
    -static {
	-fields
    }
    -configspec {
	{-fields fields Fields ""}
    }
}

#--------------------------
# Create Widget
#--------------------------
proc tixStatusBar:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    foreach field $data(-fields) {
	set name  [lindex $field 0]
	set width [lindex $field 1]

	set data(w:width) [label $w.$name -width $width]
    }
}


#----------------------------------------------------------------------
#                         Public methods
#----------------------------------------------------------------------


#----------------------------------------------------------------------
#                         Internal commands
#----------------------------------------------------------------------
