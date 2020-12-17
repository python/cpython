# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DragDrop.tcl,v 1.4 2001/12/09 05:04:02 idiscovery Exp $
#
# DragDrop.tcl ---
#
#	Implements drag+drop for Tix widgets.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixClass tixDragDropContext {
    -superclass {}
    -classname  TixDragDropContext
    -method {
	cget configure drag drop set startdrag
    }
    -flag {
	-command -source
    }
    -configspec {
	{-command ""}
	{-source ""}
    }
}

proc tixDragDropContext:Constructor {w} {
    upvar #0 $w data
}

#----------------------------------------------------------------------
# Private methods
#
#----------------------------------------------------------------------
proc tixDragDropContext:CallCommand {w target command X Y} {
    upvar #0 $w data
     
    set x [expr $X-[winfo rootx $target]]
    set y [expr $Y-[winfo rooty $target]]
    
    regsub %x $command $x command
    regsub %y $command $y command
    regsub %X $command $X command
    regsub %Y $command $Y command
    regsub %W $command $target command
    regsub %S $command [list $data(-command)] command

    eval $command
}

proc tixDragDropContext:Send {w target event X Y} {
    upvar #0 $w data
    global tixDrop

    foreach tag [tixDropBindTags $target] {
	if {[info exists tixDrop($tag,$event)]} {
	    tixDragDropContext:CallCommand $w $target \
		$tixDrop($tag,$event) $X $Y
	}
    }
}

#----------------------------------------------------------------------
# set --
#
#	Set the "small data" of the type supported by the source widget
#----------------------------------------------------------------------

proc tixDragDropContext:set {w type data} {

}

#----------------------------------------------------------------------
# startdrag --
#
#	Start the dragging action
#----------------------------------------------------------------------
proc tixDragDropContext:startdrag {w x y} {
    upvar #0 $w data

    set data(oldTarget) ""

    $data(-source) config -cursor "[tix getbitmap drop] black"
    tixDragDropContext:drag $w $x $y
}

#----------------------------------------------------------------------
# drag --
#
#	Continue the dragging action
#----------------------------------------------------------------------
proc tixDragDropContext:drag {w X Y} {
    upvar #0 $w data
    global tixDrop

    set target [winfo containing -displayof $w $X $Y]
 
    if {$target != $data(oldTarget)} {
	if {$data(oldTarget) != ""} {
	    tixDragDropContext:Send $w $data(oldTarget) <Out> $X $Y 
	}
	if {$target != ""} {
	    tixDragDropContext:Send $w $target <In> $X $Y
	}
	set data(oldTarget) $target
    }
    if {$target != ""} {
	tixDragDropContext:Send $w $target <Over> $X $Y
    }
}

proc tixDragDropContext:drop {w X Y} {
    upvar #0 $w data
    global tixDrop

    set target [winfo containing -displayof $w $X $Y]
    if {$target != ""} {
	tixDragDropContext:Send $w $target <Drop> $X $Y
    }

    if {$data(-source) != ""} {
	$data(-source) config -cursor ""
    }
    set data(-source) ""
}

#----------------------------------------------------------------------
# Public Procedures -- This is NOT a member of the tixDragDropContext
#		       class!
#
# parameters :
#	$w:	who wants to start dragging? (currently ignored)
#----------------------------------------------------------------------
proc tixGetDragDropContext {w} {
    global tixDD
    if {[info exists tixDD]} {
	return tixDD
    }

    return [tixDragDropContext tixDD]
}

proc tixDropBind {w event command} {
    global tixDrop

    set tixDrop($w) 1
    set tixDrop($w,$event) $command
}

proc tixDropBindTags {w args} {
    global tixDropTags

    if {$args == ""} {
	if {[info exists tixDropTags($w)]} {
	    return $tixDropTags($w)
	} else {
	    return [list [winfo class $w] $w]
	}
    } else {
	set tixDropTags($w) $args
    }
}
