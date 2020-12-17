# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Meter.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# Meter.tcl --
#
#	Implements the tixMeter widget
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


tixWidgetClass tixMeter {
    -classname TixMeter
    -superclass tixPrimitive
    -method {
    }
    -flag {
	-foreground -text -value
    }
    -configspec {
	{-fillcolor fillColor FillColor #8080ff}
	{-foreground foreground Foreground black}
	{-text text Text ""}
	{-value value Value 0}
    }
    -default {
	{.relief		sunken}
	{.borderWidth		2}
	{.width			150}
    }
}

proc tixMeter:InitWidgetRec {w} {
    upvar #0 $w data
    global env

    tixChainMethod $w InitWidgetRec
}

#----------------------------------------------------------------------
#		Construct widget
#----------------------------------------------------------------------
proc tixMeter:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    set data(w:canvas) [canvas $w.canvas]
    pack $data(w:canvas) -expand yes -fill both

    tixMeter:Update $w
}

proc tixMeter:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings
}

proc tixMeter:Update {w} {
    upvar #0 $w data

    # set the width of the canvas
    set W [expr $data(-width)-\
	([$data(w:root) cget -bd]+[$data(w:root) cget -highlightthickness]*2)]
    $data(w:canvas) config -width $W

    if {$data(-text) == ""} {
	set text [format "%d%%" [expr int($data(-value)*100)]]
    } else {
	set text $data(-text)
    }

    # (Create/Modify) the text item.
    #
    if {![info exists data(text)]} {
	set data(text) [$data(w:canvas) create text 0 0 -text $text]
    } else {
	$data(w:canvas) itemconfig $data(text) -text $text
    }

    set bbox [$data(w:canvas) bbox $data(text)]

    set itemW [expr [lindex $bbox 2]-[lindex $bbox 0]]
    set itemH [expr [lindex $bbox 3]-[lindex $bbox 1]]


    $data(w:canvas) coord $data(text) [expr $W/2] [expr $itemH/2+4]

    set H [expr $itemH + 4]
    $data(w:canvas) config -height [expr $H]


    set rectW [expr int($W*$data(-value))]

    if {![info exists data(rect)]} {
	set data(rect) [$data(w:canvas) create rectangle 0 0 $rectW 1000]
    } else {
	$data(w:canvas) coord $data(rect) 0 0 $rectW 1000
    }

    $data(w:canvas) itemconfig $data(rect) \
	-fill $data(-fillcolor) -outline $data(-fillcolor)

    $data(w:canvas) raise $data(text)
}

#----------------------------------------------------------------------
# Configuration
#----------------------------------------------------------------------
proc tixMeter:config-value {w value} {
    upvar #0 $w data

    set data(-value) $value
    tixMeter:Update $w
}

proc tixMeter:config-text {w value} {
    upvar #0 $w data

    set data(-text) $value
    tixMeter:Update $w
}

proc tixMeter:config-fillcolor {w value} {
    upvar #0 $w data

    set data(-fillcolor) $value
    tixMeter:Update $w
}
  

