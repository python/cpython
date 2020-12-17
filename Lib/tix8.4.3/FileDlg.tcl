# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: FileDlg.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# FileDlg.tcl --
#
#	Implements the File Selection Dialog widget.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixFileSelectDialog {
    -classname TixFileSelectDialog
    -superclass tixStdDialogShell
    -method {
    }
    -flag {
	-command
    }
    -configspec {
	{-command command Command ""}

	{-title title Title "Select A File"}
    }
}

proc tixFileSelectDialog:ConstructTopFrame {w frame} {
    upvar #0 $w data

    tixChainMethod $w ConstructTopFrame $frame

    set data(w:fsbox) [tixFileSelectBox $frame.fsbox \
	-command [list tixFileSelectDialog:Invoke $w]]
    pack $data(w:fsbox) -expand yes -fill both
}

proc tixFileSelectDialog:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    $data(w:btns) subwidget ok     config -command "$data(w:fsbox) invoke" \
	-underline 0
    $data(w:btns) subwidget apply  config -command "$data(w:fsbox) filter" \
	-text Filter -underline 0
    $data(w:btns) subwidget cancel config -command "wm withdraw $w" \
	-underline 0
    $data(w:btns) subwidget help config -underline 0


    bind $w <Alt-Key-l> "focus [$data(w:fsbox) subwidget filelist]"
    bind $w <Alt-Key-d> "focus [$data(w:fsbox) subwidget dirlist]"
    bind $w <Alt-Key-s> "focus [$data(w:fsbox) subwidget selection]"
    bind $w <Alt-Key-t> "focus [$data(w:fsbox) subwidget filter]"
    bind $w <Alt-Key-o> "tkButtonInvoke [$data(w:btns) subwidget ok]"
    bind $w <Alt-Key-f> "tkButtonInvoke [$data(w:btns) subwidget apply]"
    bind $w <Alt-Key-c> "tkButtonInvoke [$data(w:btns) subwidget cancel]"
    bind $w <Alt-Key-h> "tkButtonInvoke [$data(w:btns) subwidget help]"
}

proc tixFileSelectDialog:Invoke {w filename} {
    upvar #0 $w data

    wm withdraw $w

    if {$data(-command) != ""} {
	set bind(specs) "%V"
	set bind(%V) $filename
	tixEvalCmdBinding $w $data(-command) bind $filename
    }
}
