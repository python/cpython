# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: StdShell.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# StdShell.tcl --
#
#	Standard Dialog Shell.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixStdDialogShell {
    -classname TixStdDialogShell
    -superclass tixDialogShell
    -method {}
    -flag   {
	-cached
    }
    -configspec {
	{-cached cached Cached ""}
    }
}

proc tixStdDialogShell:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
    set data(w:btns)   [tixStdButtonBox $w.btns]
    set data(w_tframe) [frame $w.tframe]

    pack $data(w_tframe) -side top -expand yes -fill both
    pack $data(w:btns) -side bottom -fill both

    tixCallMethod $w ConstructTopFrame $data(w_tframe)
}


# Subclasses of StdDialogShell should override this method instead of
# ConstructWidget.
#
# Override : always
# chain    : before
proc tixStdDialogShell:ConstructTopFrame {w frame} {
    # Do nothing
}
