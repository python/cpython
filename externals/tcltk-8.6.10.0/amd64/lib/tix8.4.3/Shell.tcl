# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Shell.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# Shell.tcl --
#
#	This is the base class to all shell widget
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#
# type : normal, transient, overrideredirect
#
tixWidgetClass tixShell {
    -superclass tixPrimitive
    -classname  TixShell
    -flag {
	-title
    }
    -configspec {
	{-title title Title ""}
    }
    -forcecall {
	-title
    }
}

#----------------------------------------------------------------------
# ClassInitialization:
#----------------------------------------------------------------------
proc tixShell:CreateRootWidget {w args} {
    upvar #0 $w data
    upvar #0 $data(className) classRec

    toplevel $w -class $data(ClassName)
    wm transient $w ""
    wm withdraw $w
}

proc tixShell:config-title {w value} {
    wm title $w $value
}
