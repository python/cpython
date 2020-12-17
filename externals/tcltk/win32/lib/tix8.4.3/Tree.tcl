# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Tree.tcl,v 1.7 2004/04/09 21:39:12 hobbs Exp $
#
# Tree.tcl --
#
#	This file implements the TixTree widget.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


tixWidgetClass tixTree {
    -classname TixTree
    -superclass tixVTree
    -method {
	autosetmode close getmode open setmode

	addchild anchor column delete entrycget
	entryconfigure header hide indicator info
	item nearest see selection show
    }
    -flag {
	-browsecmd -command -opencmd -closecmd
    }
    -configspec {
	{-browsecmd browseCmd BrowseCmd ""}
	{-command command Command ""}
	{-closecmd closeCmd CloseCmd ""}
	{-opencmd openCmd OpenCmd ""}
    }
    -default {
	{.scrollbar			auto}
	{*Scrollbar.takeFocus           0}
	{*borderWidth                   1}
	{*hlist.background              #c3c3c3}
	{*hlist.drawBranch              1}
	{*hlist.height                  10}
	{*hlist.highlightBackground      #d9d9d9}
	{*hlist.indicator               1}
	{*hlist.indent                  20}
	{*hlist.itemType                imagetext}
	{*hlist.padX                    2}
	{*hlist.padY                    2}
	{*hlist.relief                  sunken}
	{*hlist.takeFocus               1}
	{*hlist.wideSelection           0}
	{*hlist.width                   20}
    }
}

proc tixTree:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec
}

proc tixTree:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
}

proc tixTree:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings
}

#----------------------------------------------------------------------
#
#			Widget commands
#
#----------------------------------------------------------------------
proc tixTree:autosetmode {w} {
    tixTree:SetModes $w ""
}

proc tixTree:close {w ent} {
    upvar #0 $w data

    set type [tixVTree:GetType $w $ent]
    if {$type == "close"} {
	tixCallMethod $w Activate $ent $type
    }
}

proc tixTree:open {w ent} {
    upvar #0 $w data

    set type [tixVTree:GetType $w $ent]
    if {$type == "open"} {
	tixCallMethod $w Activate $ent $type
    }
}

proc tixTree:getmode {w ent} {
    tixVTree:GetType $w $ent
}

proc tixTree:setmode {w ent mode} {
    tixVTree:SetMode $w $ent $mode
}

foreach cmd {
    addchild anchor column delete entrycget
    entryconfigure header hide indicator info
    item nearest see selection show
} {
    proc tixTree:$cmd {w args} {
	# These are hlist passthrough methods to work around
	# Tix' ignorant inheritance model.
	upvar #0 $w data
	set cmd [lindex [split [lindex [info level 0] 0] :] end]
	uplevel 1 [linsert $args 0 $data(w:hlist) $cmd]
    }
}
unset cmd

#----------------------------------------------------------------------
#
#			Private Methods
#
#----------------------------------------------------------------------
proc tixTree:SetModes {w ent} {
    upvar #0 $w data
   
    set mode none

    if {$ent == ""} {
	set children [$data(w:hlist) info children]
    } else {
	set children [$data(w:hlist) info children $ent]
    }
 
    if {$children != ""} {
	set mode close

	foreach c $children {
	    if {[$data(w:hlist) info hidden $c]} {
		set mode open
	    }
	    tixTree:SetModes $w $c
	}
    }
    
    if {$ent != ""} {
	tixVTree:SetMode $w $ent $mode
    }
}
#----------------------------------------------------------------------
#
#			Virtual Methods
#
#----------------------------------------------------------------------
proc tixTree:OpenCmd {w ent} {
    upvar #0 $w data

    if {$data(-opencmd) != ""} {
	tixTree:CallSwitchCmd $w $data(-opencmd) $ent
    } else {
	tixChainMethod $w OpenCmd $ent

    }
}

proc tixTree:CloseCmd {w ent} {
    upvar #0 $w data

    if {$data(-closecmd) != ""} {
	tixTree:CallSwitchCmd $w $data(-closecmd) $ent
    } else {
	tixChainMethod $w CloseCmd $ent
    }
}

# Call the opencmd or closecmd, depending on the mode ($cmd argument)
#
proc tixTree:CallSwitchCmd {w cmd ent} {
    upvar #0 $w data

    set bind(specs) {%V}
    set bind(%V)    $ent

    tixEvalCmdBinding $w $cmd bind $ent
}

proc tixTree:Command {w B} {
    upvar #0 $w data
    upvar $B bind

    tixChainMethod $w Command $B

    set ent [tixEvent flag V]
    if {$data(-command) != ""} {
	tixEvalCmdBinding $w $data(-command) bind $ent
    }
}

proc tixTree:BrowseCmd {w B} {
    upvar #0 $w data

    set ent [tixEvent flag V]
    if {$data(-browsecmd) != ""} {
	tixEvalCmdBinding $w $data(-browsecmd) "" $ent
    }
}
