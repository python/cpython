# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: LabEntry.tcl,v 1.4 2004/03/28 02:44:57 hobbs Exp $
#
# LabEntry.tcl --
#
# 	TixLabelEntry Widget: an entry box with a label
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


tixWidgetClass tixLabelEntry {
    -classname TixLabelEntry
    -superclass tixLabelWidget
    -method {
    }
    -flag {
	-disabledforeground -state
    }
    -forcecall {
	-state
    }
    -static {
    }
    -configspec {
	{-disabledforeground disabledForeground DisabledForeground #303030}
	{-state state State normal}
    }
    -default {
	{.borderWidth 			0}
	{*entry.relief			sunken}
	{*entry.width			7}
	{*label.anchor			e}
	{*label.borderWidth		0}
    }
}

proc tixLabelEntry:ConstructFramedWidget {w frame} {
    upvar #0 $w data

    tixChainMethod $w ConstructFramedWidget $frame

    set data(w:entry)  [entry $frame.entry]
    pack $data(w:entry) -side left -expand yes -fill both

    # This value is used to configure the disable/normal fg of the ebtry
    #
    set data(entryfg) [$data(w:entry) cget -fg]
    set data(labelfg) [$data(w:label) cget -fg]
}

proc tixLabelEntryBind {} {
  tixBind TixLabelEntry <FocusIn>  {
    if {[focus -displayof [set %W(w:entry)]] ne [set %W(w:entry)]} {
	focus [%W subwidget entry]
	[set %W(w:entry)] selection from 0
	[set %W(w:entry)] selection to end
	[set %W(w:entry)] icursor end
    }
  }
}


#----------------------------------------------------------------------
#                           CONFIG OPTIONS
#----------------------------------------------------------------------
proc tixLabelEntry:config-state {w value} {
    upvar #0 $w data

    if {$value == "normal"} {
	catch {
	    $data(w:label) config -fg $data(labelfg)
	}
	$data(w:entry) config -state $value -fg $data(entryfg)
    } else {
	catch {
	    $data(w:label) config -fg $data(-disabledforeground)
	}
	$data(w:entry) config -state $value -fg $data(-disabledforeground)
    }
}
