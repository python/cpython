# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SWindow.tcl,v 1.4 2001/12/09 05:04:02 idiscovery Exp $
#
# SWindow.tcl --
#
#	This file implements Scrolled Window widgets
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#
#
# Example:
#	
#	tixScrolledWindow .w
#	set window [.w subwidget window]
#		# Now you can put a whole widget hierachy inside $window.
#		#
#	button $window.b
#	pack $window.b
#
# Author's note
#
# Note, the current implementation does not allow the child window
# to be outside of the parent window when the parent's size is larger
# than the child's size. This is fine for normal operations. However,
# it is not suitable for an MDI master window. Therefore, you will notice
# that the MDI master window is not a subclass of ScrolledWidget at all.
#
#

tixWidgetClass tixScrolledWindow {
    -classname TixScrolledWindow
    -superclass tixScrolledWidget
    -method {
    }
    -flag {
	-expandmode -shrink -xscrollincrement -yscrollincrement
    }
    -static {
    }
    -configspec {
	{-expandmode expandMode ExpandMode expand}
	{-shrink shrink Shrink ""}
	{-xscrollincrement xScrollIncrement ScrollIncrement ""}
	{-yscrollincrement yScrollIncrement ScrollIncrement ""}

	{-scrollbarspace scrollbarSpace ScrollbarSpace {both}}
    }
    -default {
	{.scrollbar			auto}
	{*window.borderWidth		1}
	{*f1.borderWidth		1}
	{*Scrollbar.borderWidth		1}
	{*Scrollbar.takeFocus		0}
    }
}

proc tixScrolledWindow:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(dx) 0
    set data(dy) 0
}

proc tixScrolledWindow:ConstructWidget {w} {
    upvar #0 $w data
    global tcl_platform

    tixChainMethod $w ConstructWidget

    set data(pw:f1) \
	[frame $w.f1 -relief sunken]
    set data(pw:f2) \
	[frame $w.f2 -bd 0]
    set data(w:window) \
	[frame $w.f2.window -bd 0]
    pack $data(pw:f2) -in $data(pw:f1) -expand yes -fill both

    set data(w:hsb) \
	[scrollbar $w.hsb -orient horizontal -takefocus 0]
    set data(w:vsb) \
	[scrollbar $w.vsb -orient vertical -takefocus 0]
#   set data(w:pann) \
#	[frame $w.pann -bd 2 -relief groove]
    
    $data(pw:f1) config -highlightthickness \
	[$data(w:hsb) cget -highlightthickness]

    set data(pw:client) $data(pw:f1)
}

proc tixScrolledWindow:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    $data(w:hsb) config -command "tixScrolledWindow:ScrollBarCB $w x"
    $data(w:vsb) config -command "tixScrolledWindow:ScrollBarCB $w y"

    tixManageGeometry $data(w:window) "tixScrolledWindow:WindowGeomProc $w"
}

# This guy just keeps asking for a same size as the w:window 
#
proc tixScrolledWindow:WindowGeomProc {w args} {
    upvar #0 $w data

    set rw [winfo reqwidth  $data(w:window)]
    set rh [winfo reqheight $data(w:window)]

    if {$rw != [winfo reqwidth  $data(pw:f2)] ||
	$rh != [winfo reqheight $data(pw:f2)]} {
	tixGeometryRequest $data(pw:f2) $rw $rh
    }
}

proc tixScrolledWindow:Scroll {w axis total window first args} {
    upvar #0 $w data

    case [lindex $args 0] {
	"scroll" {
	    set amt  [lindex $args 1]
	    set unit [lindex $args 2]

	    case $unit {
		"units" {
		    set incr $axis\scrollincrement
		    if {$data(-$incr) != ""} {
			set by $data(-$incr)
		    } else {
			set by [expr $window / 16]
		    }
		    set first [expr $first + $amt * $by]
		}
		"pages" {
		    set first [expr $first + $amt * $window]
		}
	    }
	}
	"moveto" {
	    set to [lindex $args 1]
	    set first [expr int($to * $total)]
	}
    }

    if {[expr $first + $window] > $total} {
	set first [expr $total - $window]
    }
    if {$first < 0} {
	set first 0
    }

    return $first
}

proc tixScrolledWindow:ScrollBarCB {w axis args} {
    upvar #0 $w data

    set bd \
       [expr [$data(pw:f1) cget -bd] + [$data(pw:f1) cget -highlightthickness]]
    set fw [expr [winfo width  $data(pw:f1)] - 2*$bd]
    set fh [expr [winfo height $data(pw:f1)] - 2*$bd]
    set ww [winfo reqwidth  $data(w:window)]
    set wh [winfo reqheight $data(w:window)]

    if {$axis == "x"} {
	set data(dx) \
	    [eval tixScrolledWindow:Scroll $w $axis $ww $fw $data(dx) $args]
    } else {
	set data(dy) \
	    [eval tixScrolledWindow:Scroll $w $axis $wh $fh $data(dy) $args]
    }

    tixWidgetDoWhenIdle tixScrolledWindow:PlaceWindow $w
}

proc tixScrolledWindow:PlaceWindow {w} {
    upvar #0 $w data

    set bd \
       [expr [$data(pw:f1) cget -bd] + [$data(pw:f1) cget -highlightthickness]]
    set fw [expr [winfo width  $data(pw:f1)] - 2*$bd]
    set fh [expr [winfo height $data(pw:f1)] - 2*$bd]
    set ww [winfo reqwidth  $data(w:window)]
    set wh [winfo reqheight $data(w:window)]

    tixMapWindow $data(w:window)

    if {$data(-expandmode) == "expand"} {
	if {$ww < $fw} {
	    set ww $fw
	}
	if {$wh < $fh} {
	    set wh $fh
	}
    }
    if {$data(-shrink) == "x"} {
	if {$fw < $ww} {
	    set ww $fw
	}
    }

    tixMoveResizeWindow $data(w:window) -$data(dx) -$data(dy) $ww $wh

    set first [expr $data(dx).0 / $ww.0]
    set last  [expr $first + ($fw.0 / $ww.0)]
    $data(w:hsb) set $first $last

    set first [expr $data(dy).0 / $wh.0]
    set last  [expr $first + ($fh.0 / $wh.0)]
    $data(w:vsb) set $first $last
}

#----------------------------------------------------------------------
# virtual functions to query the client window's scroll requirement
#
# When this function is called, the scrolled window is going to be
# mapped, if it is still unmapped. Also, it is going to change its
# size. Therefore, it is a good time to check whether the w:window needs
# to be re-positioned due to the new parent window size.
#----------------------------------------------------------------------
proc tixScrolledWindow:GeometryInfo {w mW mH} {
    upvar #0 $w data

    set bd \
       [expr [$data(pw:f1) cget -bd] + [$data(pw:f1) cget -highlightthickness]]
    set fw [expr $mW -2*$bd]
    set fh [expr $mH -2*$bd]
    set ww [winfo reqwidth  $data(w:window)]
    set wh [winfo reqheight $data(w:window)]

    # Calculate the X info
    #
    if {$fw >= $ww} {
	if {$data(dx) > 0} {
	    set data(dx) 0
	}
	set xinfo [list 0.0 1.0]
    } else {
	set maxdx [expr $ww - $fw]
	if {$data(dx) > $maxdx} {
	    set data(dx) $maxdx
	}
	set first [expr $data(dx).0 / $ww.0]
	set last  [expr $first + ($fw.0 / $ww.0)]
	set xinfo [list $first $last]
    }
    # Calculate the Y info
    #
    if {$fh >= $wh} {
	if {$data(dy) > 0} {
	    set data(dy) 0
	}
	set yinfo [list 0.0 1.0]
    } else {
	set maxdy [expr $wh - $fh]
	if {$data(dy) > $maxdy} {
	    set data(dy) $maxdy
	}
	set first [expr $data(dy).0 / $wh.0]
	set last  [expr $first + ($fh.0 / $wh.0)]
	set yinfo [list $first $last]
    }

    return [list $xinfo $yinfo]
}
