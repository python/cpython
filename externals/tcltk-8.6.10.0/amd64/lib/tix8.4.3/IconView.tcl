# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: IconView.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# IconView.tcl --
#
#	This file implements the Icon View widget: the "icon" view mode of
#	the MultiView widget. It implements:
#
#	(1) Creation of the icons in the canvas subwidget.
#	(2) Automatic arrangement of the objects
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixIconView {
    -classname TixIconView
    -superclass tixCObjView
    -method {
	add arrange
    }
    -flag {
	-autoarrange
    }
    -static {
    }
    -configspec {
	{-autoarrange autoArrange AutoArrange 0 tixVerifyBoolean}
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
    }
}

proc tixIconView:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
}

proc tixIconView:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    set c $data(w:canvas)

    bind $c <1>         "tixIconView:StartDrag $w %x %y"
    bind $c <B1-Motion> "tixIconView:Drag $w %x %y"
    bind $c <ButtonRelease-1> "tixIconView:EndDrag $w"
}

proc tixIconView:StartDrag {w x y} {
    upvar #0 $w data
    global lastX lastY

    set c $data(w:canvas)
    $c raise current

    set lastX [$c canvasx $x]
    set lastY [$c canvasy $y]
}


proc tixIconView:Drag {w x y} {
    upvar #0 $w data
    global lastX lastY

    set c $data(w:canvas)
    set x [$c canvasx $x]
    set y [$c canvasy $y]
    $c move current [expr $x-$lastX] [expr $y-$lastY]
    set lastX $x
    set lastY $y
}

proc tixIconView:EndDrag {w} {
    upvar #0 $w data

    tixCallMethod $w adjustscrollregion
}

#----------------------------------------------------------------------
#
#		option configs
#----------------------------------------------------------------------
proc tixIconView:add {w tag image text} {
    upvar #0 $w data

    set cmp [image create compound -window $data(w:canvas)]

    $cmp add image -image $image
    $cmp add line
    $cmp add text -text $text

    set id [$data(w:canvas) create image 0 0 -image $cmp -anchor nw]
    $data(w:canvas) addtag $tag withtag $id

    if {$data(-autoarrange)} {
	tixWidgetDoWhenIdle tixIconView:Arrange $w 1
    }
}

# Do it in an idle handler, so that Arrange is not called before the window
# is properly mapped.
#
proc tixIconView:arrange {w} {
    tixWidgetDoWhenIdle tixIconView:Arrange $w 1
}


proc tixIconView:PackOneRow {w row y maxH bd padX padY} {
    upvar #0 $w data

    set iX [expr $bd+$padX]
    foreach i $row {
	set box [$data(w:canvas) bbox $i]
	set W [expr [lindex $box 2]-[lindex $box 0]+1]
	set H [expr [lindex $box 3]-[lindex $box 1]+1]

	set iY [expr $y + $maxH - $H]
	$data(w:canvas) coords $i $iX $iY
	incr iX [expr $W+$padX]
    }
}

# virtual method
#
proc tixIconView:PlaceWindow {w} {
    upvar #0 $w data

    if {$data(-autoarrange)} {
	tixWidgetDoWhenIdle tixIconView:Arrange $w 0
    }

    tixChainMethod $w PlaceWindow
}

proc tixIconView:Arrange {w adjust} {
    upvar #0 $w data

    set padX 2
    set padY 2

    tixIconView:ArrangeGrid $w $padX $padY
    if {$adjust} {
	tixCallMethod $w adjustscrollregion
    }
}

# the items are not packed
#
proc tixIconView:ArrangeGrid {w padX padY} {
    upvar #0 $w data

    set maxW 0
    set maxH 0
    foreach item [$data(w:canvas) find all] {
	set box [$data(w:canvas) bbox $item]
	set itemW [expr [lindex $box 2]-[lindex $box 0]+1]
	set itemH [expr [lindex $box 3]-[lindex $box 1]+1]
	if {$maxW < $itemW} {
	    set maxW $itemW
	}
	if {$maxH < $itemH} {
	    set maxH $itemH
	}
    }
    if {$maxW == 0 || $maxH == 0} {
	return
    }

    set winW [tixWinWidth $data(w:canvas)]
    set bd [expr [$data(w:canvas) cget -bd]+\
	[$data(w:canvas) cget -highlightthickness]]
    set cols [expr $winW / ($maxW+$padX)]
    if {$cols < 1} {
	set cols 1
    }
    set y $bd

    set c 0
    set x $bd
    foreach item [$data(w:canvas) find all] {
	set box [$data(w:canvas) bbox $item]
	set itemW [expr [lindex $box 2]-[lindex $box 0]+1]
	set itemH [expr [lindex $box 3]-[lindex $box 1]+1]

	set iX [expr $x + $padX + ($maxW-$itemW)/2]
	set iY [expr $y + $padY + ($maxH-$itemH)  ]

	$data(w:canvas) coords $item $iX $iY
	incr c
	incr x [expr $maxW + $padY]
	if {$c == $cols} {
	    set c 0
	    incr y [expr $maxH + $padY]
	    set x $bd
	}
    }
}

# the items are packed
#
proc tixIconView:ArrangePack {w padX padY} {
    upvar #0 $w data

    set winW [tixWinWidth $data(w:canvas)]
    set bd [expr [$data(w:canvas) cget -bd]+\
	[$data(w:canvas) cget -highlightthickness]]
    set y [expr $bd + $padY]

    set maxH 0
    set usedW $padX
    set row ""
    foreach item [$data(w:canvas) find all] {
	set box [$data(w:canvas) bbox $item]
	set itemW [expr [lindex $box 2]-[lindex $box 0]+1]
	set itemH [expr [lindex $box 3]-[lindex $box 1]+1]

	if {[expr $usedW + $itemW] > $winW} {
	    if {$row == ""} {
		# only one item in this row
		#
		$data(w:canvas) coords $item [expr $bd + $padX] $y
		incr y [expr $itemH+$padY]
		continue
	    } else {
		# this item is not in this row. Arrange the previous items
		# first
		#
		tixIconView:PackOneRow $w $row $y $maxH $bd $padX $padY

		incr y $maxH
		set row ""
		set maxH 0
		set usedW $padX
	    }
	}
	lappend row $item
	if {$maxH < $itemH} {
	    set maxH $itemH
	}
	incr usedW [expr $padX+$itemW]
    }
    if {$row != ""} {
	tixIconView:PackOneRow $w $row $y $maxH $bd $padX $padY
    }
}

#----------------------------------------------------------------------
#
#		Widget commands
#----------------------------------------------------------------------

#----------------------------------------------------------------------
#
#		Private Methods
#----------------------------------------------------------------------

