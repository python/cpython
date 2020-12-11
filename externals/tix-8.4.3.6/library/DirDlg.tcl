# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DirDlg.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# DirDlg.tcl --
#
#	Implements the Directory Selection Dialog widget.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixDirSelectDialog {
    -classname TixDirSelectDialog
    -superclass tixDialogShell
    -method {}
    -flag   {
	-command
    }
    -configspec {
	{-command command Command ""}
	{-title title Title "Select A Directory"}
    }

    -default {
	{*ok.text		"OK"}
	{*ok.underline		0}
	{*ok.width		6}
	{*cancel.text		"Cancel"}
	{*cancel.underline	0}
	{*cancel.width		6}
	{*dirbox.borderWidth	1}
	{*dirbox.relief		raised}
    }
}

proc tixDirSelectDialog:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    # the buttons
    frame $w.f -relief raised -bd 1
    set data(w:ok)     [button $w.f.ok -command \
	"tixDirSelectDialog:OK $w"]
    set data(w:cancel) [button $w.f.cancel -command \
	"tixDirSelectDialog:Cancel $w"]

    pack $data(w:ok) $data(w:cancel) -side left -expand yes -padx 10 -pady 8
    pack $w.f -side bottom -fill x
    # the dir select box
    set data(w:dirbox) [tixDirSelectBox $w.dirbox \
	-command [list tixDirSelectDialog:DirBoxCmd $w]]
    pack $data(w:dirbox) -expand yes -fill both
}

proc tixDirSelectDialog:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    bind $w <Alt-Key-d> "focus [$data(w:dirbox) subwidget dircbx]"
}

proc tixDirSelectDialog:OK {w} {
    upvar #0 $w data

    wm withdraw $w
    $data(w:dirbox) subwidget dircbx invoke
}

proc tixDirSelectDialog:DirBoxCmd {w args} {
    upvar #0 $w data

    set value [tixEvent flag V]
    wm withdraw $w
    tixDirSelectDialog:CallCmd $w $value
}

proc tixDirSelectDialog:CallCmd {w value} {
    upvar #0 $w data

    if {$data(-command) != ""} {
	set bind(specs) "%V"
	set bind(%V) $value
	tixEvalCmdBinding $w $data(-command) bind $value
    }
}

proc tixDirSelectDialog:Cancel {w} {
    wm withdraw $w
}
