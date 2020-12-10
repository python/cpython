# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: CObjView.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# CObjView.tcl --
#
#	This file implements the Canvas Object View widget. This is a base
#	class of IconView. It implements:

#	(1) Automatic placement/adjustment of the scrollbars according
#	to the canvas objects inside the canvas subwidget. The
#	scrollbars are adjusted so that the canvas is just large
#	enough to see all the objects.
#
#	(2) D+D bindings of the objects (%% not implemented)
#
#	(3) Keyboard traversal of the objects (%% not implemented). By the
#	virtual method :SelectObject.
#
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixCObjView {
    -classname TixCObjView
    -superclass tixScrolledWidget
    -method {
	adjustscrollregion
    }
    -flag {
	-xscrollincrement -yscrollincrement
    }
    -static {
    }
    -configspec {
	{-xscrollincrement xScrollIncrement ScrollIncrement 10}
	{-yscrollincrement yScrollIncrement ScrollIncrement 10}
    }
    -default {
	{.scrollbar			auto}
	{*borderWidth			1}
	{*canvas.background		#c3c3c3}
	{*canvas.highlightBackground	#d9d9d9}
	{*canvas.relief			sunken}
	{*canvas.takeFocus		1}
	{*Scrollbar.takeFocus		0}
    }
    -forcecall {
	-scrollbar
    }
}

proc tixCObjView:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    set data(w:canvas) \
	[canvas $w.canvas]
    set data(w:hsb) \
	[scrollbar $w.hsb -orient horizontal]
    set data(w:vsb) \
	[scrollbar $w.vsb -orient vertical]

    set data(pw:client) $data(w:canvas)

    set data(xorig) 0
    set data(yorig) 0

    set data(sx1) 0
    set data(sy1) 0
    set data(sx2) 0
    set data(sy2) 0
}

proc tixCObjView:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

# %% scan/drag of canvas??
#
#    $data(w:canvas) config \
#	-xscrollcommand "tixCObjView:XScroll $w"\
#	-yscrollcommand "tixCObjView:YScroll $w"

    $data(w:hsb) config -command "tixCObjView:UserScroll $w x"
    $data(w:vsb) config -command "tixCObjView:UserScroll $w y"
}

#----------------------------------------------------------------------
#
#		option configs
#----------------------------------------------------------------------
proc tixCObjView:config-takefocus {w value} {
    upvar #0 $w data
  
    $data(w:canvas) config -takefocus $value
}	

#----------------------------------------------------------------------
#
#		Widget commands
#----------------------------------------------------------------------
proc tixCObjView:adjustscrollregion {w} {
    upvar #0 $w data

    set cW [tixWinWidth  $data(w:canvas)]
    set cH [tixWinHeight $data(w:canvas)]

    tixCObjView:GetScrollRegion $w $cW $cH 1 1
}

#----------------------------------------------------------------------
#
#		Private Methods
#----------------------------------------------------------------------

proc tixCObjView:GeometryInfo {w cW cH} {
    upvar #0 $w data

    set bd \
	[expr [$data(w:canvas) cget -bd] + [$data(w:canvas) cget -highlightthickness]]

    incr cW -[expr {2*$bd}]
    incr cH -[expr {2*$bd}]

    return [tixCObjView:GetScrollRegion $w $cW $cH 0 0]
}

proc tixCObjView:PlaceWindow {w} {
    upvar #0 $w data

    set cW [tixWinWidth  $data(w:canvas)]
    set cH [tixWinHeight $data(w:canvas)]

    tixCObjView:GetScrollRegion $w $cW $cH 1 0

    tixChainMethod $w PlaceWindow
}

proc tixCObjView:GetScrollRegion {w cW cH setReg callConfig} {
    upvar #0 $w data

    set x1max $data(xorig)
    set y1max $data(yorig)

    set x2min [expr {$x1max + $cW - 1}]
    set y2min [expr {$y1max + $cH - 1}]
 
    set bbox [$data(w:canvas) bbox all]

    if {$bbox == ""} {
	set bbox {0 0 1 1}
    }

    set x1 [lindex $bbox 0]
    set y1 [lindex $bbox 1]
    set x2 [lindex $bbox 2]
    set y2 [lindex $bbox 3]

    set bd \
	[expr [$data(w:canvas) cget -bd] + [$data(w:canvas) cget -highlightthickness]]

    incr x1 -$bd
    incr y1 -$bd
    incr x2 -$bd
    incr y2 -$bd

    if {$x1 > $x1max} {
	set x1 $x1max
    }
    if {$y1 > $y1max} {
	set y1 $y1max
    }
    if {$x2 < $x2min} {
	set x2 $x2min
    }
    if {$y2 < $y2min} {
	set y2 $y2min
    }

    set data(sx1) $x1
    set data(sy1) $y1
    set data(sx2) $x2
    set data(sy2) $y2

    set sW [expr {$x2 - $x1 + 1}]
    set sH [expr {$y2 - $y1 + 1}]

#    puts "sregion = {$x1 $y1 $x2 $y2}; sW=$sW; cW=$cW"

    if {$sW > $cW} {
	set hsbSpec {0.5 1}
    } else {
	set hsbSpec {0 1}
    }
    if {$sH > $cH} {
	set vsbSpec {0.5 1}
    } else {
	set vsbSpec {0 1}
    }

    if $setReg {
	tixCObjView:SetScrollBars $w $cW $cH $sW $sH
    }
    if $callConfig {
	tixWidgetDoWhenIdle tixScrolledWidget:Configure $w
    }

    return [list $hsbSpec $vsbSpec]
}

#xF = xFirst
#
proc tixCObjView:SetScrollBars {w cW cH sW sH} {
    upvar #0 $w data

#    puts "$data(xorig) <--> $data(sx1)"

    set xF [expr ($data(xorig).0-$data(sx1).0)/$sW.0]
    set xL [expr $cW.0/$sW.0 + $xF]

    set yF [expr ($data(yorig).0-$data(sy1).0)/$sH.0]
    set yL [expr $cH.0/$sH.0 + $yF]

#    puts "$xF $xL : $yF $yL"
    $data(w:hsb) set $xF $xL    
    $data(w:vsb) set $yF $yL    
}

proc tixCObjView:UserScroll {w dir type args} {
    upvar #0 $w data

    $data(w:canvas) config -xscrollincrement 1 -yscrollincrement 1

    case $dir {
	x {
	    set n $data(xorig)
	    set orig $data(xorig)
	    set s1 $data(sx1)
	    set total [expr {$data(sx2)-$data(sx1)}]
	    set page  [tixWinWidth $data(w:canvas)]
	    set min $data(sx1)
	    set max [expr {$data(sx1)+$total-$page}]
	    set inc $data(-xscrollincrement)
	}
	y {
	    set n $data(yorig)
	    set orig $data(yorig)
	    set s1 $data(sy1)
	    set total [expr {$data(sy2)-$data(sy1)}]
	    set page  [tixWinHeight $data(w:canvas)]
	    set min $data(sy1)
	    set max [expr {$data(sy1)+$total-$page}]
	    set inc $data(-yscrollincrement)
	}
    }
	    
    case $type {
	scroll {
	    set amt  [lindex $args 0] 
	    set unit [lindex $args 1] 

	    case $unit {
		units {
		    incr n [expr int($inc)*$amt]
		}
		pages {
		    incr n [expr {$page*$amt}]
		}
	    }
	}
	moveto {
	    set first [lindex $args 0] 
	    set n [expr round($first*$total)+$s1]
	}
    }

    if {$n < $min} {
	set n $min
    }
    if {$n > $max} {
	set n $max
    }

#    puts "n=$n min=$min max=$max"

    case $dir {
	x {
	    $data(w:canvas) xview scroll [expr {$n-$orig}] units
	    set data(xorig) $n
	}
	y {
	    $data(w:canvas) yview scroll [expr {$n-$orig}] units
	    set data(yorig) $n
	}
    }

    set cW [tixWinWidth $data(w:canvas)]
    set cH [tixWinHeight $data(w:canvas)]
    set sW [expr {$data(sx2)-$data(sx1)+1}]
    set sH [expr {$data(sy2)-$data(sy1)+1}]

    tixCObjView:SetScrollBars $w $cW $cH $sW $sH
}

# Junk
#
#
proc tixCObjView:XScroll {w first last} {
    upvar #0 $w data

    set sc [$data(w:canvas) cget -scrollregion]
    if {$sc == ""} {
	set x1 1
	set x2 [tixWinWidth $data(w:canvas)]
    } else {
	set x1 [lindex $sc 0]
	set x2 [lindex $sc 2]
    }
    
    set W [expr {$x2 - $x1}]
    if {$W < 1} {
	set W 1
    }

    $data(w:hsb) set $first $last

#    tixWidgetDoWhenIdle tixScrolledWidget:Configure $w
}

# Junk
#
proc tixCObjView:YScroll {w first last} {
    upvar #0 $w data

    set sc [$data(w:canvas) cget -scrollregion]

    if {$sc == ""} {
	set y1 1
	set y2 [tixWinHeight $data(w:canvas)]
    } else {
	set y1 [lindex $sc 1]
	set y2 [lindex $sc 3]
    }
    
    set H [expr {$y2 - $y1}]
    if {$H < 1} {
	set H 1
    }

    $data(w:vsb) set $first $last

#   tixWidgetDoWhenIdle tixScrolledWidget:Configure $w
}
