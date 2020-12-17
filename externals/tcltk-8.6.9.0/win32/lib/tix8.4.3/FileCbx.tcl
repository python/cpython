# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: FileCbx.tcl,v 1.5 2004/03/28 02:44:57 hobbs Exp $
#
# tixFileCombobox --
#
#	A combobox widget for entering file names, directory names, file
#	patterns, etc.
#
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

# tixFileComboBox displays and accepts the DOS pathnames only. It doesn't
# recognize UNC file names or Tix VPATHS.
#
tixWidgetClass tixFileComboBox {
    -classname TixFileComboBox
    -superclass tixPrimitive
    -method {
	invoke
    }
    -flag {
	-command -defaultfile -directory -text
    }
    -forcecall {
	-directory
    }
    -configspec {
	{-defaultfile defaultFile DefaultFile ""}
	{-directory directory Directory ""}
	{-command command Command ""}
	{-text text Text ""}
    }
    -default {
    }
}

proc tixFileComboBox:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    if {$data(-directory) eq ""} {
	set data(-directory) [pwd]
    }
}

proc tixFileComboBox:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
    set data(w:combo) [tixComboBox $w.combo -editable true -dropdown true]
    pack $data(w:combo) -expand yes -fill both
}

proc tixFileComboBox:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings
    $data(w:combo) config -command [list tixFileComboBox:OnComboCmd $w]
}

proc tixFileComboBox:OnComboCmd {w args} {
    upvar #0 $w data

    set text [string trim [tixEvent value]]

    set path [tixFSJoin $data(-directory) $text]
    if {[file isdirectory $path]} {
	set path [tixFSJoin $path $data(-defaultfile)]
	set tail $data(-defaultfile)
    } else {
	set tail [file tail $path]
    }
    set norm [tixFSNormalize $path]
    tixSetSilent $data(w:combo) $norm
    if {[llength $data(-command)]} {
	set bind(specs) {%V}
	set bind(%V)    [list $norm $path $tail ""]
	tixEvalCmdBinding $w $data(-command) bind $bind(%V)
    }
}

proc tixFileComboBox:config-text {w val} {
    upvar #0 $w data

    tixSetSilent $data(w:combo) $val
}

proc tixFileComboBox:config-directory {w val} {
    upvar #0 $w data

    set data(-directory) [tixFSNormalize $val]
    return $data(-directory)
}

proc tixFileComboBox:invoke {w} {
    upvar #0 $w data

    $data(w:combo) invoke
}


