# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: PanedWin.tcl,v 1.7 2004/03/28 02:44:57 hobbs Exp $
#
# PanedWin.tcl --
#
#	This file implements the TixPanedWindow widget
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


tixWidgetClass tixPanedWindow {
    -classname TixPanedWindow
    -superclass tixPrimitive
    -method {
	add delete forget manage panecget paneconfigure panes setsize
    }
    -flag {
	-command -dynamicgeometry -handleactivebg -handlebg -orient
	-orientation -panebd -paneborderwidth -panerelief
	-separatoractivebg -separatorbg
    }
    -static {
	-orientation
    }
    -configspec {
	{-command command Command ""}
	{-dynamicgeometry dynamicGeometry DynamicGeometry 1 tixVerifyBoolean}
	{-handleactivebg handleActiveBg HandleActiveBg #ececec}
	{-handlebg handleBg Background #d9d9d9}
	{-orientation orientation Orientation vertical}
	{-paneborderwidth paneBorderWidth PaneBorderWidth 1}
	{-panerelief paneRelief PaneRelief raised}
	{-separatoractivebg separatorActiveBg SeparatorActiveBg red}
	{-separatorbg separatorBg Background #d9d9d9}
    }
    -alias {
	{-panebd -paneborderwidth}
	{-orient -orientation}
    }
}

#----------------------------------------------------------------------
# ClassInitialization:
#----------------------------------------------------------------------

proc tixPanedWindow:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(items)       ""
    set data(nItems)      0
    set data(totalsize)   0
    set data(movePending) 0

    set data(repack)      0
    set data(counter)     0

    set data(maxReqW)     1
    set data(maxReqH)     1
}

proc tixPanedWindow:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
    # Do nothing
}

proc tixPanedWindow:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    bind $w <Configure> [list tixPanedWindow:MasterGeomProc $w ""]
}

#----------------------------------------------------------------------
# ConfigOptions:
#----------------------------------------------------------------------
proc tixPanedWindow:config-handlebg {w arg} {
    upvar #0 $w data

    for {set i 1} {$i < $data(nItems)} {incr i} {
	$data(btn,$i) config -bg $arg
    }
}

#----------------------------------------------------------------------
# PublicMethods:
#----------------------------------------------------------------------


# method: add
#
#    Adds a new pane into the PanedWindow.
#
# options -size -max -min -allowresize
#
proc tixPanedWindow:add {w name args} {
    upvar #0 $w data

    if {[winfo exists $w.$name] && !$data($name,forgotten)} {
	error "Pane $name is already managed"
    }
    # Step 1: Parse the options to get the children's size options

    # The default values
    #
    if {[info exists data($name,forgotten)]} {
	set option(-size)        $data($name,size)
	set option(-min)         $data($name,min)
	set option(-max)         $data($name,max)
	set option(-allowresize) $data($name,allowresize)
	set option(-expand)      $data($name,expand)
    } else {
	set option(-size)        0
	set option(-min)         0
	set option(-max)         100000
	set option(-allowresize) 1
	set option(-expand)      0
    }

    set option(-before)      ""
    set option(-after)       ""
    set option(-at)          ""
    set validOpts {-after -allowresize -at -before -expand -max -min -size}

    tixHandleOptions option $validOpts $args

    set data($name,size)        $option(-size)
    set data($name,rsize)       $option(-size)
    set data($name,min)         $option(-min)
    set data($name,max)         $option(-max)
    set data($name,allowresize) $option(-allowresize)
    set data($name,expand)      $option(-expand)
    set data($name,forgotten)   0

    if {$data($name,expand) < 0} {
	set data($name,expand) 0
    }

    # Step 2: Add the frame and the separator (if necessary)
    #
    if {![winfo exist $w.$name]} {
	# need to check because the frame may have been "forget'ten"
	#
	frame $w.$name -bd $data(-paneborderwidth) -relief $data(-panerelief)
    }

    if {$option(-at) != ""} {
	set at [tixGetInt $option(-at)]
	if {$at < 0} {
	    set at 0
	}
    } elseif {$option(-after) != ""} {
	set index [lsearch -exact $data(items) $option(-after)]
	if {$index == -1} {
	    error "Pane $option(-after) doesn't exists"
	} else {
	    set at [incr index]
	}
    } elseif {$option(-before) != ""} {
	set index [lsearch -exact $data(items) $option(-before)]
	if {$index == -1} {
	    error "Pane $option(-before) doesn't exists"
	}
	set at $index
    } else {
	set at end
    }

    set data(items) [linsert $data(items) $at $name]    
    incr data(nItems)

    if {$data(nItems) > 1} {
	tixPanedWindow:AddSeparator $w
    }
    set data(w:$name) $w.$name

    # Step 3: Add the new frame. Adjust the window later (do when idle)
    #
    tixManageGeometry $w.$name [list tixPanedWindow:ClientGeomProc $w]
    bind $w.$name <Configure> \
	[list tixPanedWindow:ClientGeomProc $w "" $w.$name]

    tixPanedWindow:RepackWhenIdle $w

    return $w.$name
}

proc tixPanedWindow:manage {w name args} {
    upvar #0 $w data

    if {![winfo exists $w.$name]} {
	error "Pane $name does not exist"
    }
    if {!$data($name,forgotten)} {
	error "Pane $name is already managed"
    }
    tixMapWindow $data(w:$name)
    eval tixPanedWindow:add $w [list $name] $args
}

proc tixPanedWindow:forget {w name} {
    upvar #0 $w data

    if {![winfo exists $w.$name]} {
	error "Pane $name does not exist"
    }
    if $data($name,forgotten) {
	# It has already been forgotten
	#
	return
    }

    set items ""
    foreach item $data(items) {
	if {$item != $name} {
	    lappend items $item
	}
    }
    set data(items) $items
    incr data(nItems) -1

    set i $data(nItems)
    if {$i > 0} {
	destroy $data(btn,$i)
	destroy $data(sep,$i)
	unset data(btn,$i)
	unset data(sep,$i)
    }
    set data($name,forgotten) 1

    tixUnmapWindow $w.$name

    tixPanedWindow:RepackWhenIdle $w
}

proc tixPanedWindow:delete {w name} {
    upvar #0 $w data

    if {![winfo exists $w.$name]} {
	error "Pane $name does not exist"
    }


    if {!$data($name,forgotten)} {
	set items ""
	foreach item $data(items) {
	    if {$item != $name} {
		lappend items $item
	    }
	}
	set data(items) $items
	incr data(nItems) -1

	set i $data(nItems)
	if {$i > 0} {
	    destroy $data(btn,$i)
	    destroy $data(sep,$i)
	    unset data(btn,$i)
	    unset data(sep,$i)
	}
    }
    unset data($name,allowresize)
    unset data($name,expand)
    unset data($name,forgotten)
    unset data($name,max)
    unset data($name,min)
    unset data($name,rsize)
    unset data($name,size)
    unset data(w:$name)
    destroy $w.$name

    tixPanedWindow:RepackWhenIdle $w
}

proc tixPanedWindow:paneconfigure {w name args} {
    upvar #0 $w data

    if {![info exists data($name,size)]} {
	error "pane \"$name\" does not exist in $w"
    }

    set len [llength $args]

    if {$len == 0} {
	set value [$data(w:$name) configure]
	lappend value [list -allowresize "" "" "" $data($name,allowresize)]
	lappend value [list -expand "" "" "" $data($name,expand)]
	lappend value [list -max "" "" "" $data($name,max)]
	lappend value [list -min "" "" "" $data($name,min)]
	lappend value [list -size "" "" "" $data($name,size)]
	return $value
    }

    if {$len == 1} {
	case [lindex $args 0] {
	    -allowresize {
		return [list -allowresize "" "" "" $data($name,allowresize)]
	    }
	    -expand {
		return [list -expand "" "" "" $data($name,expand)]
	    }
	    -min {
		return [list -min "" "" "" $data($name,min)]
	    }
	    -max {
		return [list -max "" "" "" $data($name,max)]
	    }
	    -size {
		return [list -size "" "" "" $data($name,size)]
	    }
	    default {
		return [$data(w:$name) configure [lindex $args 0]]
	    }
	}
    }

    # By default handle each of the options
    #
    set option(-allowresize) $data($name,allowresize)
    set option(-expand)      $data($name,expand)
    set option(-min)         $data($name,min)
    set option(-max)         $data($name,max)
    set option(-size)        $data($name,size)

    tixHandleOptions -nounknown option {-allowresize -expand -max -min -size} \
	$args

    #
    # the widget options
    set new_args ""
    foreach {flag value} $args {
	case $flag {
	    {-expand -min -max -allowresize -size} {

	    }
	    default {
		lappend new_args $flag
		lappend new_args $value
	    }
	}
    }

    if {[llength $new_args] >= 2} {
	eval $data(w:$name) configure $new_args
    }

    #
    # The add-on options
    set data($name,allowresize) $option(-allowresize)
    set data($name,expand)      $option(-expand)
    set data($name,max)         $option(-max)
    set data($name,min)         $option(-min)
    set data($name,rsize)       $option(-size)
    set data($name,size)        $option(-size)

    # 
    # Integrity check
    if {$data($name,expand) < 0} {
	set data($name,expand) 0
    }
    if {$data($name,size) < $data($name,min)} {
	set data($name,size) $data($name,min)
    }
    if {$data($name,size) > $data($name,max)} {
	set data($name,size) $data($name,max)
    }

    tixPanedWindow:RepackWhenIdle $w
    return ""
}

proc tixPanedWindow:panecget {w name option} {
    upvar #0 $w data

    if {![info exists data($name,size)]} {
	error "pane \"$name\" does not exist in $w"
    }

    case $option {
	{-min -max -allowresize -size} {
	    regsub \\\- $option "" option
	    return "$data($name,$option)"
	}
	default {
	    return [$data(w:$name) cget $option]
	}
    }
}

# return the name of all panes
proc tixPanedWindow:panes {w} {
    upvar #0 $w data

    return $data(items)
}

# set the size of a pane, specifying which direction it should
# grow/shrink
proc tixPanedWindow:setsize {w item size {direction next}} {
    upvar #0 $w data

    set posn [lsearch $data(items) $item]
    if {$posn == -1} {
	error "pane \"$item\" does not exist"
    }

    set diff [expr {$size - $data($item,size)}]
    if {$diff == 0} {
	return
    }

    if {$posn == 0 && $direction eq "prev"} {
	set direction next
    }
    if {$posn == $data(nItems)-1 && $direction eq "next"} {
	set direction prev
    }

    if {$data(-orientation) eq "vertical"} {
        set rx [winfo rooty $data(w:$item)]
    } else {
        set rx [winfo rootx $data(w:$item)]
    }
    if {$direction eq "prev"} {
	set rx [expr {$rx - $diff}]
    } elseif {$data(-orientation) eq "vertical"} {
	set rx [expr {$rx + [winfo height $data(w:$item)] + $diff}]
	incr posn
    } else {
	set rx [expr {$rx + [winfo width $data(w:$item)] + $diff}]
	incr posn
    }

    # Set up the panedwin in a proper state
    #
    tixPanedWindow:BtnDown $w $posn 1
    tixPanedWindow:BtnMove $w $posn $rx 1
    tixPanedWindow:BtnUp $w $posn 1

    return $data(items)
}

#----------------------------------------------------------------------
# PrivateMethods:
#----------------------------------------------------------------------

proc tixPanedWindow:AddSeparator {w} {
    global tcl_platform

    upvar #0 $w data

    set n [expr {$data(nItems)-1}]

    # CYGNUS: On Windows, use relief ridge and a thicker line.
    if {$tcl_platform(platform) eq "windows"} then {
      set relief "ridge"
      set thickness 4
    } else {
      set relief "sunken"
      set thickness 2
    }
    if {$data(-orientation) eq "vertical"} {
	set data(sep,$n) [frame $w.sep$n -relief $relief \
	    -bd 1 -height $thickness -width 10000 -bg $data(-separatorbg)]
    } else {
	set data(sep,$n) [frame $w.sep$n -relief $relief \
	    -bd 1 -width $thickness -height 10000 -bg $data(-separatorbg)]
    }

    set data(btn,$n) [frame $w.btn$n -relief raised \
	-bd 1 -width 9 -height 9 \
	-bg $data(-handlebg)]

    if {$data(-orientation) eq "vertical"} {
	set cursor sb_v_double_arrow
    } else {
	set cursor sb_h_double_arrow
    }
    $data(sep,$n) config -cursor $cursor
    $data(btn,$n) config -cursor $cursor

    foreach wid [list $data(btn,$n) $data(sep,$n)] {
	bind $wid \
	    <ButtonPress-1>   [list tixPanedWindow:BtnDown $w $n]
	bind $wid \
	    <ButtonRelease-1> [list tixPanedWindow:BtnUp   $w $n]
	bind $wid \
	    <Any-Enter>       [list tixPanedWindow:HighlightBtn $w $n]
	bind $wid \
	    <Any-Leave>       [list tixPanedWindow:DeHighlightBtn $w $n]
    }

    if {$data(-orientation) eq "vertical"} {
	bind  $data(btn,$n) <B1-Motion> [list tixPanedWindow:BtnMove $w $n %Y]
    } else {
	bind  $data(btn,$n) <B1-Motion> [list tixPanedWindow:BtnMove $w $n %X]
    }

    if {$data(-orientation) eq "vertical"} {
#	place $data(btn,$n) -relx 0.90 -y [expr "$data(totalsize)-5"]
#	place $data(sep,$n) -x 0 -y [expr "$data(totalsize)-1"] -relwidth 1
    } else {
#	place $data(btn,$n) -rely 0.90 -x [expr "$data(totalsize)-5"]
#	place $data(sep,$n) -y 0 -x [expr "$data(totalsize)-1"] -relheight 1
    }
}

proc tixPanedWindow:BtnDown {w item {fake 0}} {
    upvar #0 $w data

    if {$data(-orientation) eq "vertical"} {
	set spec -height
    } else {
	set spec -width
    }

    if {!$fake} {
	for {set i 1} {$i < $data(nItems)} {incr i} {
	    $data(sep,$i) config -bg $data(-separatoractivebg) $spec 1
	}
	update idletasks
	$data(btn,$item) config -relief sunken
    }

    tixPanedWindow:GetMotionLimit $w $item $fake

    if {!$fake} {
	grab -global $data(btn,$item)
    }
    set data(movePending) 0
}

proc tixPanedWindow:Min2 {a b} {
    if {$a < $b} {
	return $a
    } else {
	return $b
    }
}

proc tixPanedWindow:GetMotionLimit {w item fake} {
    upvar #0 $w data

    set curBefore 0
    set minBefore 0
    set maxBefore 0

    for {set i 0} {$i < $item} {incr i} {
	set name [lindex $data(items) $i]
	incr curBefore $data($name,size)
	incr minBefore $data($name,min)
	incr maxBefore $data($name,max)
    }

    set curAfter 0
    set minAfter 0
    set maxAfter 0
    while {$i < $data(nItems)} {
	set name [lindex $data(items) $i]
	incr curAfter $data($name,size)
	incr minAfter $data($name,min)
	incr maxAfter $data($name,max)
	incr i
    }

    set beforeToGo [tixPanedWindow:Min2 \
			[expr {$curBefore-$minBefore}] \
			[expr {$maxAfter-$curAfter}]]

    set afterToGo [tixPanedWindow:Min2 \
		       [expr {$curAfter-$minAfter}] \
		       [expr {$maxBefore-$curBefore}]]

    set data(beforeLimit) [expr {$curBefore-$beforeToGo}]
    set data(afterLimit)  [expr {$curBefore+$afterToGo}]
    set data(curSize)     $curBefore

    if {!$fake} {
	tixPanedWindow:PlotHandles $w 1
    }
}

# Compress the motion so that update is quick even on slow machines
#
# rootp = root position (either rootx or rooty)
proc tixPanedWindow:BtnMove {w item rootp {fake 0}} {
    upvar #0 $w data

    set data(rootp) $rootp

    if {$fake} {
	tixPanedWindow:BtnMoveCompressed $w $item $fake
    } else {
	if {$data(movePending) == 0} {
	    after 2 tixPanedWindow:BtnMoveCompressed $w $item
	    set data(movePending) 1
	}
    } 
}

proc tixPanedWindow:BtnMoveCompressed {w item {fake 0}} {
    if {![winfo exists $w]} {
	return
    }

    upvar #0 $w data

    if {$data(-orientation) eq "vertical"} {
	set p [expr {$data(rootp)-[winfo rooty $w]}]
    } else {
	set p [expr {$data(rootp)-[winfo rootx $w]}]
    }

    if {$p == $data(curSize)} {
	set data(movePending) 0
	return
    }

    if {$p < $data(beforeLimit)} {
	set p $data(beforeLimit)
    }
    if {$p >= $data(afterLimit)} {
	set p $data(afterLimit)
    }
    tixPanedWindow:CalculateChange $w $item $p $fake

    if {!$fake} {
	# Force the redraw to happen
	#
	update idletasks
    }
    set data(movePending) 0
}

# Calculate the change in response to mouse motions
#
proc tixPanedWindow:CalculateChange {w item p {fake 0}} {
    upvar #0 $w data

    if {$p < $data(curSize)} {
	tixPanedWindow:MoveBefore $w $item $p
    } elseif {$p > $data(curSize)} {
	tixPanedWindow:MoveAfter $w $item $p
    }

    if {!$fake} {
	tixPanedWindow:PlotHandles $w 1
    }
}

proc tixPanedWindow:MoveBefore {w item p} {
    upvar #0 $w data

    set n [expr {$data(curSize)-$p}]

    # Shrink the frames before
    #
    set from [expr {$item-1}]
    set to   0
    tixPanedWindow:Iterate $w $from $to tixPanedWindow:Shrink $n

    # Adjust the frames after
    #
    set from $item
    set to   [expr {$data(nItems)-1}]
    tixPanedWindow:Iterate $w $from $to tixPanedWindow:Grow $n

    set data(curSize) $p
}

proc tixPanedWindow:MoveAfter {w item p} {
    upvar #0 $w data

    set n    [expr {$p-$data(curSize)}]

    # Shrink the frames after
    #
    set from $item
    set to   [expr {$data(nItems)-1}]
    tixPanedWindow:Iterate $w $from $to tixPanedWindow:Shrink $n

    # Graw the frame before
    #
    set from [expr {$item-1}]
    set to   0
    tixPanedWindow:Iterate $w $from $to tixPanedWindow:Grow $n

    set data(curSize) $p
}

proc tixPanedWindow:CancleLines {w} {
    upvar #0 $w data

    if {[info exists data(lines)]} {
	foreach line $data(lines) {
	    set x1 [lindex $line 0]
	    set y1 [lindex $line 1]
	    set x2 [lindex $line 2]
	    set y2 [lindex $line 3]

	    tixTmpLine $x1 $y1 $x2 $y2 $w
	}

	catch {unset data(lines)}
    }
}

proc tixPanedWindow:PlotHandles {w transient} {
    global tcl_platform

    upvar #0 $w data

    set totalsize 0
    set i 0

    if {$data(-orientation) eq "vertical"} {
	set btnp [expr {[winfo width $w]-13}]
    } else {
	set h [winfo height $w]
	if {$h > 18} {
	    set btnp 9
	} else {
	    set btnp [expr {$h-9}]
	}
    }

    set firstpane [lindex $data(items) 0]
    set totalsize $data($firstpane,size)

    if {$transient} {
	tixPanedWindow:CancleLines $w
	set data(lines) ""
    }

    for {set i 1} {$i < $data(nItems)} {incr i} {
	if {! $transient} {
	    if {$data(-orientation) eq "vertical"} {
		place $data(btn,$i) -x $btnp -y [expr {$totalsize-4}]
		place $data(sep,$i) -x 0 -y [expr {$totalsize-1}] -relwidth 1
	    } else {
		place $data(btn,$i) -y $btnp -x [expr {$totalsize-5}]
		place $data(sep,$i) -y 0 -x [expr {$totalsize-1}] -relheight 1
	    }
	} else {
	    if {$data(-orientation) eq "vertical"} {
		set x1 [winfo rootx $w]
		set x2 [expr {$x1 + [winfo width $w]}]
		set y  [expr {$totalsize-1+[winfo rooty $w]}]

		tixTmpLine $x1 $y $x2 $y $w
		lappend data(lines) [list $x1 $y $x2 $y]
	    } else {
		set y1 [winfo rooty $w]
		set y2 [expr {$y1 + [winfo height $w]}]
		set x  [expr {$totalsize-1+[winfo rootx $w]}]

		tixTmpLine $x $y1 $x $y2 $w
		lappend data(lines) [list $x $y1 $x $y2]
	    }
	}

	set name [lindex $data(items) $i]
	incr totalsize $data($name,size)
    }
}

proc tixPanedWindow:BtnUp {w item {fake 0}} {
    upvar #0 $w data

    if {!$fake} {
	tixPanedWindow:CancleLines $w
    }

    tixPanedWindow:UpdateSizes $w

    if {!$fake} {
	$data(btn,$item) config -relief raised
	grab release $data(btn,$item)
    }
}


proc tixPanedWindow:HighlightBtn {w item} {
    upvar #0 $w data

    $data(btn,$item) config -background $data(-handleactivebg)
}

proc tixPanedWindow:DeHighlightBtn {w item} {
    upvar #0 $w data

    $data(btn,$item) config -background $data(-handlebg)
}

#----------------------------------------------------------------------
#
#
# Geometry management routines
#
#
#----------------------------------------------------------------------

# update the sizes of each pane according to the data($name,size) variables
#
proc tixPanedWindow:UpdateSizes {w} {
    global tcl_platform

    upvar #0 $w data

    set data(totalsize) 0

    set mw [winfo width  $w]
    set mh [winfo height $w]

    for {set i 0} {$i < $data(nItems)} {incr i} {
	set name [lindex $data(items) $i]

	if {$data($name,size) > 0} {
	    if {$data(-orientation) eq "vertical"} {
		tixMoveResizeWindow $w.$name 0 $data(totalsize) \
		    $mw $data($name,size)
		tixMapWindow $w.$name
		raise $w.$name
	    } else {
		tixMoveResizeWindow $w.$name $data(totalsize) 0 \
		    $data($name,size) $mh
		tixMapWindow $w.$name
		raise $w.$name
	    }
	} else {
	    tixUnmapWindow $w.$name
	}
	incr data(totalsize) $data($name,size)
    }

    # Reset the color and width of the separator
    #
    if {$data(-orientation) eq "vertical"} {
	set spec -height
    } else {
	set spec -width
    }

    # CYGNUS: On Windows, use a thicker line.
    if {$tcl_platform(platform) eq "windows"} then {
	set thickness 4
    } else {
	set thickness 2
    }

    for {set i 1} {$i < $data(nItems)} {incr i} {
	$data(sep,$i) config -bg $data(-separatorbg) $spec $thickness
	raise $data(sep,$i)
	raise $data(btn,$i)
    }


    # Invoke the callback command
    #
    if {$data(-command) != ""} {
	set sizes ""
	foreach item $data(items) {
	    lappend sizes $data($item,size)
	}
	set bind(specs) ""
	tixEvalCmdBinding $w $data(-command) bind [list $sizes]
    }
}

proc tixPanedWindow:GetNaturalSizes {w} {
    upvar #0 $w data

    set data(totalsize) 0
    set totalreq 0

    if {$data(-orientation) eq "vertical"} {
	set majorspec height
	set minorspec width
    } else {
	set majorspec width
	set minorspec height
    }

    set minorsize 0
    foreach name $data(items) {
	if {[winfo manager $w.$name] ne "tixGeometry"} {
	    error "Geometry management error: pane \"$w.$name\" cannot be managed by \"[winfo manager $w.$name]\"\nhint: delete the line \"[winfo manager $w.$name] $w.$name ...\" from your program"
	}

	# set the minor size
	#
	set req_minor [winfo req$minorspec $w.$name]

	if {$req_minor > $minorsize} {
	    set minorsize $req_minor
	}

	# Check the natural size against the max, min requirements.
	# Change the natural size if necessary
	#
	if {$data($name,size) <= 1} {
	    set data($name,size) [winfo req$majorspec $w.$name]
	}

	if {$data($name,size) > 1} {
	    # If we get zero maybe the widget was not initialized yet ...
	    #
	    # %% hazard : what if the window is really 1x1?
	    #
	    if {$data($name,size) < $data($name,min)} {
		set data($name,size) $data($name,min)
	    }
	    if {$data($name,size) > $data($name,max)} {
		set data($name,size) $data($name,max)
	    }
	}

	# kludge: because a frame always returns req size of {1,1} before
	# the packer processes it, we do the following to mark the
	# pane as "size unknown"
	#
#	if {$data($name,size) == 1 && ![winfo ismapped $w.$name]} {
#	    set data($name,size) 0
#	}

	# Add up the total size
	#
	incr data(totalsize) $data($name,size)

	# Find out the request size
	#
	if {$data($name,rsize) == 0} {
	    set rsize [winfo req$majorspec $w.$name]
	} else {
	    set rsize $data($name,rsize)
	}

	if {$rsize < $data($name,min)} {
	    set rsize $data($name,min)
	}
	if {$rsize > $data($name,max)} {
	    set rsize $data($name,max)
	}

	incr totalreq $rsize
    }

    if {$data(-orientation) eq "vertical"} {
	return [list $minorsize $totalreq]
    } else {
	return [list $totalreq $minorsize]
    }
}

#--------------------------------------------------
# Handling resize
#--------------------------------------------------
proc tixPanedWindow:ClientGeomProc {w type client} {
    tixPanedWindow:RepackWhenIdle $w
}

#
# This monitor the sizes of the master window
#
proc tixPanedWindow:MasterGeomProc {w master} {
    tixPanedWindow:RepackWhenIdle $w
}

proc tixPanedWindow:RepackWhenIdle {w} {
    if {![winfo exist $w]} {
	return
    }
    upvar #0 $w data

    if {$data(repack) == 0} {
	tixWidgetDoWhenIdle tixPanedWindow:Repack $w
	set data(repack) 1
    }
}

#
# This monitor the sizes of the master window
#
proc tixPanedWindow:Repack {w} {
    upvar #0 $w data

    # Calculate the desired size of the master
    #
    set dim [tixPanedWindow:GetNaturalSizes $w]

    if {$data(-width) != 0} {
	set mreqw $data(-width)
    } else {
	set mreqw [lindex $dim 0]
    }

    if {$data(-height) != 0} {
	set mreqh $data(-height)
    } else {
	set mreqh [lindex $dim 1]
    }

    if !$data(-dynamicgeometry) {
	if {$mreqw < $data(maxReqW)} {
	    set mreqw $data(maxReqW)
	}
	if {$mreqh < $data(maxReqH)} {
	    set mreqh $data(maxReqH)
	}
	set data(maxReqW) $mreqw
	set data(maxReqH) $mreqh
    }
    if {$mreqw != [winfo reqwidth $w] || $mreqh != [winfo reqheight $w] } {
	if {![info exists data(counter)]} {
	    set data(counter) 0
	}
	if {$data(counter) < 50} {
	    incr data(counter)
	    tixGeometryRequest $w $mreqw $mreqh
	    tixWidgetDoWhenIdle tixPanedWindow:Repack $w
	    set data(repack) 1
	    return
	}
    }

    set data(counter) 0

    if {$data(nItems) == 0} {
	set data(repack) 0
	return
    }

    tixWidgetDoWhenIdle tixPanedWindow:DoRepack $w
}

proc tixPanedWindow:DoRepack {w} {
    upvar #0 $w data

    if {$data(-orientation) eq "vertical"} {
	set newSize [winfo height $w]
    } else {
	set newSize [winfo width $w]
    }

    if {$newSize <= 1} {
	# Probably this window is too small to see anyway
	# %%Kludge: I don't know if this always work.
	#
	set data(repack) 0
	return
    }

    set totalExp 0
    foreach name $data(items) {
	set totalExp [expr {$totalExp + $data($name,expand)}]
    }

    if {$newSize > $data(totalsize)} {
	# Grow
	#
	set toGrow [expr {$newSize-$data(totalsize)}]

	set p [llength $data(items)]
	foreach name $data(items) {
	    set toGrow [tixPanedWindow:xGrow $w $name $toGrow $totalExp $p]
	    if {$toGrow > 0} {
		set totalExp [expr {$totalExp-$data($name,expand)}]
		incr p -1
	    } else {
		break
	    }
	}
    } else {
	# Shrink
	#
	set toShrink [expr {$data(totalsize)-$newSize}]

	set usedSize 0
	foreach name $data(items) {
	    set toShrink [tixPanedWindow:xShrink $w $name $toShrink \
		$totalExp $newSize $usedSize]
	    if {$toShrink > 0} {
		set totalExp [expr {$totalExp-$data($name,expand)}]
		incr usedSize $data($name,size)
	    } else {
		break
	    }
	}
    }

    tixPanedWindow:UpdateSizes $w
    tixPanedWindow:PlotHandles $w 0

    set data(repack) 0
}

#--------------------------------------------------
# Shrink and grow items
#--------------------------------------------------
# 
#   toGrow: how much free area to grow into
#        p: == 1 if $name is the last in the list of items
# totalExp: used to calculate the amount of the free area that this
#	    window can grow into
#
proc tixPanedWindow:xGrow {w name toGrow totalExp p} {
    upvar #0 $w data

    if {$p == 1} {
	set canGrow $toGrow
    } else {
	if {$totalExp == 0} {
	    set canGrow 0
	} else {
	    set canGrow [expr {int($toGrow * $data($name,expand) / $totalExp)}]
	}
    }

    if {($canGrow + $data($name,size)) > $data($name,max)} {
	set canGrow [expr {$data($name,max) - $data($name,size)}]
    }

    incr data($name,size) $canGrow
    incr toGrow -$canGrow

    return $toGrow
}

proc tixPanedWindow:xShrink {w name toShrink totalExp newSize usedSize} {
    upvar #0 $w data

    if {$totalExp == 0} {
	set canShrink 0
    } else {
	set canShrink [expr {int($toShrink * $data($name,expand) / $totalExp)}]
    }

    if {$data($name,size) - $canShrink < $data($name,min)} {
	set canShrink [expr {$data($name,size) - $data($name,min)}]
    }
    if {$usedSize + $data($name,size) - $canShrink > $newSize} {
	set data($name,size) [expr {$newSize - $usedSize}]
	return 0
    } else {
	incr data($name,size) -$canShrink
	incr toShrink -$canShrink

	return $toShrink
    }
}

#--------------------------------------------------
# Shrink and grow items
#--------------------------------------------------
proc tixPanedWindow:Shrink {w name n} {
    upvar #0 $w data

    set canShrink [expr {$data($name,size) - $data($name,min)}]

    if {$canShrink > $n} {
	incr data($name,size) -$n
	return 0
    } elseif {$canShrink > 0} {
	set data($name,size) $data($name,min)
	incr n -$canShrink
    }
    return $n
}

proc tixPanedWindow:Grow {w name n} {
    upvar #0 $w data

    set canGrow [expr {$data($name,max) - $data($name,size)}]

    if {$canGrow > $n} {
	incr data($name,size) $n
	return 0
    } elseif {$canGrow > 0} {
	set data($name,size) $data($name,max)
	incr n -$canGrow
    }

    return $n
}

proc tixPanedWindow:Iterate {w from to proc n} {
    upvar #0 $w data

    if {$from <= $to} {
	for {set i $from} {$i <= $to} {incr i} {
	    set n [$proc $w [lindex $data(items) $i] $n]
	    if {$n == 0} {
		break
	    }
	}
    } else {
	for {set i $from} {$i >= $to} {incr i -1} {
	    set n [$proc $w [lindex $data(items) $i] $n]
	    if {$n == 0} {
		break
	    }
	}
    }
}
