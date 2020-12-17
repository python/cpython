# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: EFileDlg.tcl,v 1.3 2002/01/24 09:13:58 idiscovery Exp $
#
# EFileDlg.tcl --
#
#	Implements the Extended File Selection Dialog widget.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

foreach fun {tkButtonInvoke} {
    if {![llength [info commands $fun]]} {
	tk::unsupported::ExposePrivateCommand $fun
    }
}
unset fun

tixWidgetClass tixExFileSelectDialog {
    -classname TixExFileSelectDialog
    -superclass tixDialogShell
    -method {}
    -flag   {
	-command
    }
    -configspec {
	{-command command Command ""}

	{-title title Title "Select A File"}
    }
}

proc tixExFileSelectDialog:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
    set data(w:fsbox) [tixExFileSelectBox $w.fsbox -dialog $w \
	-command $data(-command)]
    pack $data(w:fsbox) -expand yes -fill both


}

proc tixExFileSelectDialog:config-command {w value} {
    upvar #0 $w data

    $data(w:fsbox) config -command $value
}

proc tixExFileSelectDialog:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    bind $w <Alt-Key-f> "focus [$data(w:fsbox) subwidget file]"
    bind $w <Alt-Key-t> "focus [$data(w:fsbox) subwidget types]"
    bind $w <Alt-Key-d> "focus [$data(w:fsbox) subwidget dir]"
    bind $w <Alt-Key-o> "tkButtonInvoke [$data(w:fsbox) subwidget ok]"
    bind $w <Alt-Key-c> "tkButtonInvoke [$data(w:fsbox) subwidget cancel]"
    bind $w <Alt-Key-s> "tkButtonInvoke [$data(w:fsbox) subwidget hidden]"
}
