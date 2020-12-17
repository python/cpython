# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: VResize.tcl,v 1.3 2004/03/28 02:44:57 hobbs Exp $
#
# VResize.tcl --
#
#	tixVResize:
#	Virtual base class for all classes that provide resize capability,
#	such as the resize handle and the MDI client window.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixVResize {
    -virtual true
    -classname TixVResize
    -superclass tixPrimitive
    -method {
	drag dragend dragstart
    }
    -flag {
	-gridded -gridx -gridy -minwidth -minheight
    }
    -configspec {
 	{-gridded gridded Gridded false}
	{-gridx gridX Grid 10}
	{-gridy gridY Grid 10}
	{-minwidth minWidth MinWidth 0}
	{-minheight minHeight MinHeight 0}
   }
}


proc tixVResize:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(movePending) 0
    set data(aborted) 0
    set data(depress) 0
}

#----------------------------------------------------------------------
#                    Public methods
#----------------------------------------------------------------------
# Start dragging a window
#
proc tixVResize:dragstart {w win depress rootx rooty wrect mrect} {
    upvar #0 $w data

    set data(rootx) $rootx
    set data(rooty) $rooty

    set data(mx) [lindex $mrect 0]
    set data(my) [lindex $mrect 1]
    set data(mw) [lindex $mrect 2]
    set data(mh) [lindex $mrect 3]

    set data(fx) [lindex $wrect 0]
    set data(fy) [lindex $wrect 1]
    set data(fw) [lindex $wrect 2]
    set data(fh) [lindex $wrect 3]

    set data(old_x) [lindex $wrect 0]
    set data(old_y) [lindex $wrect 1]
    set data(old_w) [lindex $wrect 2]
    set data(old_h) [lindex $wrect 3]

    if {$data(mw) < 0} {
	set data(maxx)  [expr {$data(fx) + $data(old_w) - $data(-minwidth)}]
    } else {
	set data(maxx) 32000
    }
    if {$data(mh) < 0} {
	set data(maxy)  [expr {$data(fy) + $data(old_h) - $data(-minheight)}]
    } else {
	set data(maxy) 32000
    }

    set data(aborted) 0

    tixCallMethod $w ShowHintFrame
    tixCallMethod $w SetHintFrame $data(fx) $data(fy) $data(fw) $data(fh)

    # Grab so that all button events are captured
    #
    grab $win
    focus $win

    set data(depress) $depress
    if {$depress} {
	set data(oldRelief) [$win cget -relief]
	$win config -relief sunken
    }
}


proc tixVResize:drag {w rootx rooty} {
    upvar #0 $w data

    if {$data(aborted) == 0} {
	set data(newrootx) $rootx
	set data(newrooty) $rooty

	if {$data(movePending) == 0} {
	    set data(movePending) 1
	    after 2 tixVResize:DragCompressed $w
	}
    }
}

proc tixVResize:dragend {w win isAbort rootx rooty} {
    upvar #0 $w data

    if {$data(aborted)} {
	if {$isAbort == 0} {
	    grab release $win
	}
	return
    }

    # Just in case some draggings are not applied.
    #
    update

    tixCallMethod $w HideHintFrame

    if {$isAbort} {
	set data(aborted) 1
    } else {
	# Apply the changes
	#
	tixCallMethod $w UpdateSize $data(fx) $data(fy) $data(fw) $data(fh)

	# Release the grab
	#
	grab release $win
    }

    if {$data(depress)} {
	$win config -relief $data(oldRelief)
    }
}

#----------------------------------------------------------------------
#                    Internal methods
#----------------------------------------------------------------------

proc tixVResize:DragCompressed {w} {
    if {![winfo exists $w]} {
	return
    }

    upvar #0 $w data

    if {$data(aborted) == 1 || $data(movePending) == 0} {
	return
    }

    set dx [expr {$data(newrootx) - $data(rootx)}]
    set dy [expr {$data(newrooty) - $data(rooty)}]

    set data(fx) [expr {$data(old_x) + ($dx * $data(mx))}]
    set data(fy) [expr {$data(old_y) + ($dy * $data(my))}]
    set data(fw) [expr {$data(old_w) + ($dx * $data(mw))}]
    set data(fh) [expr {$data(old_h) + ($dy * $data(mh))}]

    if {$data(fw) < $data(-minwidth)} {
	set data(fw) $data(-minwidth)
    }
    if {$data(fh) < $data(-minheight)} {
	set data(fh) $data(-minheight)
    }

    if {$data(fx) > $data(maxx)} {
	set data(fx) $data(maxx)
    }
    if {$data(fy) > $data(maxy)} {
	set data(fy) $data(maxy)
    }

    # If we need grid, set x,y,w,h to fit the grid
    #
    # *note* grid overrides minwidth and maxwidth ...
    #
    if {$data(-gridded)} {
	set data(fx) [expr {round(double($data(fx))/$data(-gridx)) * $data(-gridx)}]
	set data(fy) [expr {round(double($data(fy))/$data(-gridy)) * $data(-gridy)}]

	set fx2  [expr {$data(fx) + $data(fw) - 2}]
	set fy2  [expr {$data(fy) + $data(fh) - 2}]

	set fx2 [expr {round(double($fx2)/$data(-gridx)) * $data(-gridx)}]
	set fy2 [expr {round(double($fy2)/$data(-gridy)) * $data(-gridy)}]

	set data(fw) [expr {$fx2 - $data(fx) + 1}]
	set data(fh) [expr {$fy2 - $data(fy) + 1}]
    }

    tixCallMethod $w SetHintFrame $data(fx) $data(fy) $data(fw) $data(fh)

    update idletasks

    set data(movePending) 0
}
