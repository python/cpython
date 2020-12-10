# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: BtnBox.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# BtnBox.tcl --
#
#	Implements the tixButtonBox widget
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixButtonBox {
    -superclass tixPrimitive
    -classname  TixButtonBox
    -method {
	add invoke button buttons
    }
    -flag {
	-orientation -orient -padx -pady -state
    }
    -static {
	-orientation
    }
    -configspec {
	{-orientation orientation Orientation horizontal}
	{-padx padX Pad 0}
	{-pady padY Pad 0}
	{-state state State normal}
    }
    -alias {
	{-orient -orientation}
    }
    -default {
	{.borderWidth 		1}
	{.relief 		raised}
	{.padX 			5}
	{.padY 			10}
	{*Button.anchor		c}
	{*Button.padX		5}
    }
}

proc tixButtonBox:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(g:buttons) ""
}

#----------------------------------------------------------------------
#                           CONFIG OPTIONS
#----------------------------------------------------------------------
proc tixButtonBox:config-padx {w arg} {
    upvar #0 $w data

    foreach item $data(g:buttons) {
	pack configure $w.$item -padx $arg
    }
}

proc tixButtonBox:config-pady {w arg} {
    upvar #0 $w data

    foreach item $data(g:buttons) {
	pack configure $w.$item -pady $arg
    }
}

proc tixButtonBox:config-state {w arg} {
    upvar #0 $w data

    foreach item $data(g:buttons) {
	$w.$item config -state $arg
    }
}

#----------------------------------------------------------------------
# Methods
#                     WIDGET COMMANDS
#----------------------------------------------------------------------
proc tixButtonBox:add {w name args} {
    upvar #0 $w data

    eval button $w.$name $args
    if {$data(-orientation) == "horizontal"} {
	pack $w.$name -side left -expand yes -fill y\
	    -padx $data(-padx) -pady $data(-pady)
    } else {
	pack $w.$name -side top -expand yes  -fill x\
	    -padx $data(-padx) -pady $data(-pady)
    }

    # allow for subwidget access
    #
    lappend data(g:buttons) $name
    set data(w:$name) $w.$name

    return $w.$name
}

proc tixButtonBox:button {w name args} {
    return [eval tixCallMethod $w subwidget $name $args]
}

proc tixButtonBox:buttons {w args} {
    return [eval tixCallMethod $w subwidgets -group buttons $args]
}

#
# call the command
proc tixButtonBox:invoke {w name} {
    upvar #0 $w data

    $w.$name invoke
}
