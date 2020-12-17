# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: ResizeH.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# ResizeH.tcl --
#
#	tixResizeHandle: A general purpose "resizing handle"
#	widget. You can use it to resize pictures, widgets, etc. When
#	using it to resize a widget, you can use the "attachwidget"
#	command to attach it to a widget and it will handle all the
#	events for you.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
#
#

tixWidgetClass tixResizeHandle {
    -classname TixResizeHandle
    -superclass tixVResize

    -method {
	attachwidget detachwidget hide show
    }
    -flag {
	-command -cursorfg -cursorbg -handlesize -hintcolor -hintwidth -x -y
    }
    -configspec {
	{-command command Command ""}
	{-cursorfg cursorFg CursorColor white}
	{-cursorbg cursorBg CursorColor red}
	{-handlesize handleSize HandleSize 6}
	{-hintcolor hintColor HintColor red}
	{-hintwidth hintWidth HintWidth 1}
	{-x x X 0}
	{-y y Y 0}
    }
}

proc tixResizeHandle:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(shown)  0
    set data(widget) ""
}

proc tixResizeHandle:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    # Create the hints
    #
    set data(w_ht) $w:tix_priv_ht
    set data(w_hb) $w:tix_priv_hb
    set data(w_hl) $w:tix_priv_hl
    set data(w_hr) $w:tix_priv_hr

    frame $data(w_ht) -height $data(-hintwidth) -bg $data(-background)
    frame $data(w_hb) -height $data(-hintwidth) -bg $data(-background)
    frame $data(w_hl) -width  $data(-hintwidth) -bg $data(-background)
    frame $data(w_hr) -width  $data(-hintwidth) -bg $data(-background)

    # Create the corner resize handles
    #
    set data(w_r00) $w

#   Windows don't like this
#    $data(rootCmd) config\
#	-cursor "top_left_corner $data(-cursorbg) $data(-cursorfg)"

    $data(rootCmd) config -cursor top_left_corner

    set data(w_r01) $w:tix_priv_01
    set data(w_r10) $w:tix_priv_10
    set data(w_r11) $w:tix_priv_11

    frame $data(w_r01) -relief $data(-relief) -bd $data(-borderwidth) \
	-cursor "bottom_left_corner"\
	-bg $data(-background)
    frame $data(w_r10) -relief $data(-relief) -bd $data(-borderwidth) \
	-cursor "top_right_corner"\
	-bg $data(-background)
    frame $data(w_r11) -relief $data(-relief) -bd $data(-borderwidth) \
	-cursor "bottom_right_corner"\
	-bg $data(-background)

    # Create the border resize handles
    #
    set data(w_bt)  $w:tix_priv_bt
    set data(w_bb)  $w:tix_priv_bb
    set data(w_bl)  $w:tix_priv_bl
    set data(w_br)  $w:tix_priv_br

    frame $data(w_bt) -relief $data(-relief) -bd $data(-borderwidth) \
	-cursor "top_side"\
	-bg $data(-background)
    frame $data(w_bb) -relief $data(-relief) -bd $data(-borderwidth) \
	-cursor "bottom_side"\
	-bg $data(-background)
    frame $data(w_bl) -relief $data(-relief) -bd $data(-borderwidth) \
	-cursor "left_side"\
	-bg $data(-background)
    frame $data(w_br) -relief $data(-relief) -bd $data(-borderwidth) \
	-cursor "right_side"\
	-bg $data(-background)
}

proc tixResizeHandle:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    bind $data(w_r00)   <1> \
	"tixResizeHandle:dragstart $w $data(w_r00)   1 %X %Y  {1  1 -1 -1}"
    bind $data(w_r01)   <1> \
	"tixResizeHandle:dragstart $w $data(w_r01)   1 %X %Y  {1  0 -1  1}"
    bind $data(w_r10)   <1> \
	"tixResizeHandle:dragstart $w $data(w_r10)   1 %X %Y  {0  1  1 -1}"
    bind $data(w_r11)   <1> \
	"tixResizeHandle:dragstart $w $data(w_r11)   1 %X %Y  {0  0  1  1}"
    bind $data(w_bt)    <1> \
	"tixResizeHandle:dragstart $w $data(w_bt)    1 %X %Y  {0  1  0 -1}"
    bind $data(w_bb)    <1> \
	"tixResizeHandle:dragstart $w $data(w_bb)    1 %X %Y  {0  0  0  1}"
    bind $data(w_bl)    <1> \
	"tixResizeHandle:dragstart $w $data(w_bl)    1 %X %Y  {1  0 -1  0}"
    bind $data(w_br)    <1> \
	"tixResizeHandle:dragstart $w $data(w_br)    1 %X %Y  {0  0  1  0}"

    foreach win [list \
		 $data(w_r00)\
		 $data(w_r01)\
		 $data(w_r10)\
		 $data(w_r11)\
		 $data(w_bt)\
		 $data(w_bb)\
		 $data(w_bl)\
		 $data(w_br)\
		 ] {
	bind $win <B1-Motion>       "tixVResize:drag    $w %X %Y"
	bind $win <ButtonRelease-1> "tixVResize:dragend $w $win 0 %X %Y"
	bind $win <Any-Escape>      "tixVResize:dragend $w $win 1  0  0"
    }
}

#----------------------------------------------------------------------
# 		Config Methods
#----------------------------------------------------------------------
proc tixResizeHandle:config-width {w value} {
    tixWidgetDoWhenIdle tixResizeHandle:ComposeWindow $w
}

proc tixResizeHandle:config-height {w value} {
    tixWidgetDoWhenIdle tixResizeHandle:ComposeWindow $w
}

proc tixResizeHandle:config-x {w value} {
    tixWidgetDoWhenIdle tixResizeHandle:ComposeWindow $w
}

proc tixResizeHandle:config-y {w value} {
    tixWidgetDoWhenIdle tixResizeHandle:ComposeWindow $w
}


#----------------------------------------------------------------------
# 		Public Methods
#----------------------------------------------------------------------
proc tixResizeHandle:dragstart {w win depress rootx rooty mrect} {
    upvar #0 $w data

    set wx $data(-x)
    set wy $data(-y)
    set ww $data(-width)
    set wh $data(-height)

    tixVResize:dragstart $w $win $depress $rootx $rooty \
	[list $wx $wy $ww $wh] $mrect
}

# tixDeleteBindTag --
#
#	Delete the bindtag(s) in the args list from the bindtags of the widget
#
proc tixDeleteBindTag {w args} {
    if {![winfo exists $w]} {
	return
    }
    set newtags ""

    foreach tag [bindtags $w] {
	if {[lsearch $args $tag] == -1} {
	    lappend newtags $tag
	}
    }
    bindtags $w $newtags
}

proc tixAddBindTag {w args} {
    bindtags $w [concat [bindtags $w] $args]
}

proc tixResizeHandle:attachwidget {w widget args} {
    upvar #0 $w data

    set opt(-move) 0
    tixHandleOptions opt {-move} $args

    if {$data(widget) != ""} {
	tixDeleteBindTag $data(widget) TixResizeHandleTag:$w
    }

    set data(widget) $widget

    if {$data(widget) != ""} {
	# Just in case TixResizeHandleTag was already there
	tixDeleteBindTag $data(widget) TixResizeHandleTag:$w
	tixAddBindTag $data(widget) TixResizeHandleTag:$w

	
	set data(-x)      [winfo x      $data(widget)]
	set data(-y)      [winfo y      $data(widget)]
	set data(-width)  [winfo width  $data(widget)]
	set data(-height) [winfo height $data(widget)]

	tixResizeHandle:show $w
	tixResizeHandle:ComposeWindow $w

	# Now set the bindings
	#
	if {$opt(-move)} {
	    bind TixResizeHandleTag:$w <1> \
		"tixResizeHandle:Attach $w %X %Y"
	    bind TixResizeHandleTag:$w <B1-Motion> \
		"tixResizeHandle:BMotion $w %X %Y"
	    bind TixResizeHandleTag:$w <Any-Escape> \
		"tixResizeHandle:BRelease $w 1 %X %Y"
	    bind TixResizeHandleTag:$w <ButtonRelease-1>\
		"tixResizeHandle:BRelease $w 0 %X %Y"
	} else {
	    # if "move" is false, then the widget won't be moved as a whole -- 
	    # ResizeHandle will only move its sides
	    bind TixResizeHandleTag:$w <1> 		 {;}
	    bind TixResizeHandleTag:$w <B1-Motion>	 {;}
	    bind TixResizeHandleTag:$w <Any-Escape>	 {;}
	    bind TixResizeHandleTag:$w <ButtonRelease-1> {;}
	}
    }
}

proc tixResizeHandle:detachwidget {w} {
    upvar #0 $w data

    if {$data(widget) != ""} {
	tixDeleteBindTag $data(widget) TixResizeHandleTag:$w
    }
    tixResizeHandle:hide $w
}

proc tixResizeHandle:show {w} {
    upvar #0 $w data

    set data(shown) 1

    raise $data(w_ht)
    raise $data(w_hb)
    raise $data(w_hl)
    raise $data(w_hr)

    raise $data(w_r00)
    raise $data(w_r01)
    raise $data(w_r10)
    raise $data(w_r11)

    raise $data(w_bt)
    raise $data(w_bb)
    raise $data(w_bl)
    raise $data(w_br)

#   tixCancleIdleTask tixResizeHandle:ComposeWindow $w
    tixResizeHandle:ComposeWindow $w
}


proc tixResizeHandle:hide {w} {
    upvar #0 $w data

    if {!$data(shown)} {
	return
    }

    set data(shown) 0

    place forget $data(w_r00)
    place forget $data(w_r01)
    place forget $data(w_r10)
    place forget $data(w_r11)

    place forget $data(w_bt)
    place forget $data(w_bb)
    place forget $data(w_bl)
    place forget $data(w_br)

    place forget $data(w_ht)
    place forget $data(w_hb)
    place forget $data(w_hl)
    place forget $data(w_hr)
}

proc tixResizeHandle:Destructor {w} {
    upvar #0 $w data

    if {$data(widget) != ""} {
	tixDeleteBindTag $data(widget) TixResizeHandleTag:$w
    }

    catch {destroy $data(w_r01)}
    catch {destroy $data(w_r10)}
    catch {destroy $data(w_r11)}

    catch {destroy $data(w_bt)}
    catch {destroy $data(w_bb)}
    catch {destroy $data(w_bl)}
    catch {destroy $data(w_br)}

    catch {destroy $data(w_ht)}
    catch {destroy $data(w_hb)}
    catch {destroy $data(w_hl)}
    catch {destroy $data(w_hr)}

    tixChainMethod $w Destructor
}

#----------------------------------------------------------------------
# 	  Private Methods Dealing With Attached Widgets
#----------------------------------------------------------------------
proc tixResizeHandle:Attach {w rx ry} {
    upvar #0 $w data

    tixResizeHandle:dragstart $w $data(widget) 0 $rx $ry {1 1 0 0}
}

proc tixResizeHandle:BMotion {w rx ry} {
    tixVResize:drag $w $rx $ry
}


proc tixResizeHandle:BRelease {w isAbort rx ry} {
    upvar #0 $w data

    tixVResize:dragend $w $data(widget) $isAbort $rx $ry
}

#----------------------------------------------------------------------
# 		Private Methods
#----------------------------------------------------------------------
proc tixResizeHandle:DrawTmpLines {w} {
    upvar #0 $w data

    # I've seen this error - mike
    if {![info exists data(hf:x1)]} {return}
    set x1 $data(hf:x1)
    if {![info exists data(hf:y1)]} {return}
    set y1 $data(hf:y1)
    if {![info exists data(hf:x2)]} {return}
    set x2 $data(hf:x2)
    if {![info exists data(hf:y2)]} {return}
    set y2 $data(hf:y2)

    tixTmpLine $x1 $y1 $x2 $y1 $w
    tixTmpLine $x1 $y2 $x2 $y2 $w
    tixTmpLine $x1 $y1 $x1 $y2 $w
    tixTmpLine $x2 $y1 $x2 $y2 $w
}

# Place the hint frame to indicate the changes
#
proc tixResizeHandle:SetHintFrame {w x1 y1 width height} {
    upvar #0 $w data

    # The four sides of the window
    #
    set x2 [expr "$x1+$width"]
    set y2 [expr "$y1+$height"]

    set rx [winfo rootx [winfo parent $w]]
    set ry [winfo rooty [winfo parent $w]]

    incr x1 $rx
    incr y1 $ry
    incr x2 $rx
    incr y2 $ry

    if {[info exists data(hf:x1)]} {
	tixResizeHandle:DrawTmpLines $w
    }

    set data(hf:x1) $x1
    set data(hf:y1) $y1
    set data(hf:x2) $x2
    set data(hf:y2) $y2

    tixResizeHandle:DrawTmpLines $w
}

proc tixResizeHandle:ShowHintFrame {w} {
    upvar #0 $w data

    place forget $data(w_ht)
    place forget $data(w_hb)
    place forget $data(w_hl)
    place forget $data(w_hr)

    update
}

proc tixResizeHandle:HideHintFrame {w} {
    upvar #0 $w data

    tixResizeHandle:DrawTmpLines $w
    unset data(hf:x1)
    unset data(hf:y1)
    unset data(hf:x2)
    unset data(hf:y2)
}

proc tixResizeHandle:UpdateSize {w x y width height} {
    upvar #0 $w data

    set data(-x)      $x
    set data(-y)      $y
    set data(-width)  $width
    set data(-height) $height

    tixResizeHandle:ComposeWindow $w

    if {$data(widget) != ""} {
	place $data(widget) -x $x -y $y -width $width -height $height
    }

    if {$data(-command) != ""} {
	eval $data(-command) $x $y $width $height
    }
}

proc tixResizeHandle:ComposeWindow {w} {
    upvar #0 $w data

    set px $data(-x)
    set py $data(-y)
    set pw $data(-width)
    set ph $data(-height)

    # Show the hint frames
    #
    set x1 $px
    set y1 $py
    set x2 [expr "$px+$pw"]
    set y2 [expr "$py+$ph"]

    place $data(w_ht) -x $x1 -y $y1 -width  $pw -bordermode outside
    place $data(w_hb) -x $x1 -y $y2 -width  $pw -bordermode outside
    place $data(w_hl) -x $x1 -y $y1 -height $ph -bordermode outside
    place $data(w_hr) -x $x2 -y $y1 -height $ph -bordermode outside

    # Set the four corner resize handles
    #
    set sz_2 [expr $data(-handlesize)/2]

    set x1 [expr "$px - $sz_2"]
    set y1 [expr "$py - $sz_2"]
    set x2 [expr "$px - $sz_2" + $pw]
    set y2 [expr "$py - $sz_2" + $ph]

    place $data(w_r00) -x $x1 -y $y1 \
	-width $data(-handlesize) -height $data(-handlesize)
    place $data(w_r01) -x $x1 -y $y2\
	-width $data(-handlesize) -height $data(-handlesize)
    place $data(w_r10) -x $x2 -y $y1\
	-width $data(-handlesize) -height $data(-handlesize)
    place $data(w_r11) -x $x2 -y $y2\
	-width $data(-handlesize) -height $data(-handlesize)


    # Set the four border resize handles
    #
    set mx [expr "$px + $pw/2 - $sz_2"]
    set my [expr "$py + $ph/2 - $sz_2"]

    place $data(w_bt) -x $mx -y $y1 \
	-width $data(-handlesize) -height $data(-handlesize)
    place $data(w_bb) -x $mx -y $y2 \
	-width $data(-handlesize) -height $data(-handlesize)
    place $data(w_bl) -x $x1 -y $my \
	-width $data(-handlesize) -height $data(-handlesize)
    place $data(w_br) -x $x2 -y $my \
	-width $data(-handlesize) -height $data(-handlesize)
}
