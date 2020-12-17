# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: FloatEnt.tcl,v 1.4 2004/03/28 02:44:57 hobbs Exp $
#
# FloatEnt.tcl --
#
#	An entry widget that can be attached on top of any widget to
#	provide dynamic editing. It is used to provide dynamic editing
#	for the tixGrid widget, among other things.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixFloatEntry {
    -classname TixFloatEntry
    -superclass tixPrimitive
    -method {
	invoke post unpost
    }
    -flag {
	-command -value
    }
    -configspec {
	{-value value Value ""}
	{-command command Command ""}
    }
    -default {
	{.entry.highlightThickness	0}
    }
}

#----------------------------------------------------------------------
#
#	Initialization bindings
#
#----------------------------------------------------------------------

proc tixFloatEntry:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec
}

proc tixFloatEntry:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
    set data(w:entry) [entry $w.entry]
    pack $data(w:entry) -expand yes -fill both
}

proc tixFloatEntry:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings
    tixBind $data(w:entry) <Return> [list tixFloatEntry:invoke $w]
}

#----------------------------------------------------------------------
#
#	Class bindings
#
#----------------------------------------------------------------------

proc tixFloatEntryBind {} {
    tixBind TixFloatEntry <FocusIn>  {
      if {[focus -displayof [set %W(w:entry)]] ne [set %W(w:entry)]} {
	  focus [%W subwidget entry]
	  [set %W(w:entry)] selection from 0
	  [set %W(w:entry)] selection to end
	  [set %W(w:entry)] icursor end
      }
    }
}

#----------------------------------------------------------------------
#
#	Public methods
#
#----------------------------------------------------------------------
proc tixFloatEntry:post {w x y {width ""} {height ""}} {
    upvar #0 $w data

    if {$width == ""} {
	set width [winfo reqwidth $data(w:entry)]
    }
    if {$height == ""} {
	set height [winfo reqheight $data(w:entry)]
    }

    place $w -x $x -y $y -width $width -height $height -bordermode ignore
    raise $w
    focus $data(w:entry)
}

proc tixFloatEntry:unpost {w} {
    upvar #0 $w data

    place forget $w
}

proc tixFloatEntry:config-value {w val} {
    upvar #0 $w data

    $data(w:entry) delete 0 end
    $data(w:entry) insert 0 $val

    $data(w:entry) selection from 0
    $data(w:entry) selection to end
    $data(w:entry) icursor end
}
#----------------------------------------------------------------------
#
#	Private methods
#
#----------------------------------------------------------------------

proc tixFloatEntry:invoke {w} {
    upvar #0 $w data

    if {[llength $data(-command)]} {
	set bind(specs) {%V}
	set bind(%V)    [$data(w:entry) get]

	tixEvalCmdBinding $w $data(-command) bind $bind(%V)
    }
}
