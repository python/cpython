# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SimpDlg.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# SimpDlg.tcl --
#
#	This file implements Simple Dialog widgets
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixSimpleDialog {
    -classname TixSimpleDialog
    -superclass tixDialogShell
    -method {}
    -flag   {
	-buttons -message -type
    }
    -configspec {
	{-buttons buttons Buttons ""}
	{-message message Message ""}
	{-type type Type info}
    }
}

proc tixSimpleDialog:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    frame $w.top

    label $w.top.icon -image [tix getimage $data(-type)]
    label $w.top.message -text $data(-message)

    pack $w.top.icon    -side left -padx 20 -pady 50 -anchor c
    pack $w.top.message -side left -padx 10 -pady 50 -anchor c

    frame $w.bot

    pack $w.bot -side bottom -fill x
    pack $w.top -side top -expand yes -fill both
}
