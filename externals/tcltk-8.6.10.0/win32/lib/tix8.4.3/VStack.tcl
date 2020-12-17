# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: VStack.tcl,v 1.4 2004/03/28 02:44:57 hobbs Exp $
#
# VStack.tcl --
#
#	Virtual base class, do not instantiate!  This is the core
#	class for all NoteBook style widgets. Stack maintains a list
#	of windows. It provides methods to create, delete windows as
#	well as stepping through them.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
#


tixWidgetClass tixVStack {
    -virtual true
    -classname TixVStack
    -superclass tixPrimitive
    -method {
	add delete pageconfigure pagecget pages raise raised
    }
    -flag {
	-dynamicgeometry -ipadx -ipady
    }
    -configspec {
	{-dynamicgeometry dynamicGeometry DynamicGeometry 0 tixVerifyBoolean}
	{-ipadx ipadX Pad 0}
	{-ipady ipadY Pad 0}
    }
}

proc tixVStack:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(pad-x1) 0
    set data(pad-x2) 0
    set data(pad-y1) 0
    set data(pad-y2) 0

    set data(windows)  ""
    set data(nWindows) 0
    set data(topchild) ""

    set data(minW)   1
    set data(minH)   1

    set data(w:top)  $w
    set data(counter) 0
    set data(repack) 0
}

proc tixVStack:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings
    tixCallMethod $w InitGeometryManager
}

#----------------------------------------------------------------------
# Public methods
#----------------------------------------------------------------------
proc tixVStack:add {w child args} {
    upvar #0 $w data

    set validOptions {-createcmd -raisecmd}

    set opt(-createcmd)  ""
    set opt(-raisecmd)   ""

    tixHandleOptions -nounknown opt $validOptions $args

    set data($child,raisecmd)  $opt(-raisecmd)
    set data($child,createcmd) $opt(-createcmd)

    set data(w:$child) [tixCallMethod $w CreateChildFrame $child]

    lappend data(windows) $child
    incr data(nWindows) 1

    return $data(w:$child) 
}

proc tixVStack:delete {w child} {
    upvar #0 $w data

    if {[info exists data($child,createcmd)]} {
	if {[winfo exists $data(w:$child)]} {
	    bind $data(w:$child) <Destroy> {;}
	    destroy $data(w:$child)
	}
	catch {unset data($child,createcmd)}
	catch {unset data($child,raisecmd)}
	catch {unset data(w:$child)}

	set index [lsearch $data(windows) $child]
	if {$index >= 0} {
	    set data(windows) [lreplace $data(windows) $index $index]
	    incr data(nWindows) -1
	}

	if {[string equal $data(topchild) $child]} {
	    set data(topchild) ""
	    foreach page $data(windows) {
		if {$page ne $child} {
		    $w raise $page
		    set data(topchild) $page
		    break
		}
	    }
	}
    } else {
	error "page $child does not exist"
    }
}

proc tixVStack:pagecget {w child option} {
    upvar #0 $w data

    if {![info exists data($child,createcmd)]} {
	error "page \"$child\" does not exist in $w"
    }

    case $option {
	-createcmd {
	    return "$data($child,createcmd)"
	}
	-raisecmd {
	    return "$data($child,raisecmd)"
	}
	default {
	    if {$data(w:top) ne $w} {
		return [$data(w:top) pagecget $child $option]
	    } else {
		error "unknown option \"$option\""
	    }
	}
    }
}

proc tixVStack:pageconfigure {w child args} {
    upvar #0 $w data

    if {![info exists data($child,createcmd)]} {
	error "page \"$child\" does not exist in $w"
    }

    set len [llength $args]

    if {$len == 0} {
	set value [$data(w:top) pageconfigure $child]
	lappend value [list -createcmd "" "" "" $data($child,createcmd)]
	lappend value [list -raisecmd "" "" "" $data($child,raisecmd)]
	return $value
    }

    if {$len == 1} {
	case [lindex $args 0] {
	    -createcmd {
		return [list -createcmd "" "" "" $data($child,createcmd)]
	    }
	    -raisecmd {
		return [list -raisecmd "" "" "" $data($child,raisecmd)]
	    }
	    default {
		return [$data(w:top) pageconfigure $child [lindex $args 0]]
	    }
	}
    }

    # By default handle each of the options
    #
    set opt(-createcmd)  $data($child,createcmd)
    set opt(-raisecmd)   $data($child,raisecmd)

    tixHandleOptions -nounknown opt {-createcmd -raisecmd} $args

    #
    # the widget options
    set new_args ""
    foreach {flag value} $args {
	if {$flag ne "-createcmd" && $flag ne "-raisecmd"} {
	    lappend new_args $flag
	    lappend new_args $value
	}
    }

    if {[llength $new_args] >= 2} {
	eval $data(w:top) pageconfig $child $new_args
    }

    #
    # The add-on options
    set data($child,raisecmd)  $opt(-raisecmd)
    set data($child,createcmd) $opt(-createcmd)

    return ""
}

proc tixVStack:pages {w} {
    upvar #0 $w data

    return $data(windows)
}

proc tixVStack:raise {w child} {
    upvar #0 $w data

    if {![info exists data($child,createcmd)]} {
	error "page $child does not exist"
    }

    if {[llength $data($child,createcmd)]} {
	uplevel #0 $data($child,createcmd)
	set data($child,createcmd) ""
    }

    tixCallMethod $w RaiseChildFrame $child

    set oldTopChild $data(topchild)
    set data(topchild) $child

    if {$oldTopChild ne $child} {
	if {[llength $data($child,raisecmd)]} {
 	    uplevel #0 $data($child,raisecmd)
	}
    }
}

proc tixVStack:raised {w} {
    upvar #0 $w data

    return $data(topchild)
}

#----------------------------------------------------------------------
# Virtual Methods
#----------------------------------------------------------------------
proc tixVStack:InitGeometryManager {w} {
    upvar #0 $w data

    bind $w <Configure> "tixVStack:MasterGeomProc $w"
    bind $data(w:top) <Destroy> "+tixVStack:DestroyTop $w"

    if {$data(repack) == 0} {
	set data(repack) 1
	tixWidgetDoWhenIdle tixCallMethod $w Resize
    }
}

proc tixVStack:CreateChildFrame {w child} {
    upvar #0 $w data

    set f [frame $data(w:top).$child]

    tixManageGeometry $f "tixVStack:ClientGeomProc $w"
    bind $f <Configure> "tixVStack:ClientGeomProc $w -configure $f"
    bind $f <Destroy>   "$w delete $child"

    return $f
}

proc tixVStack:RaiseChildFrame {w child} {
    upvar #0 $w data

    # Hide the original visible window
    if {$data(topchild) ne "" && $data(topchild) ne $child} {
	tixUnmapWindow $data(w:$data(topchild))
    }

    set myW [winfo width  $w]
    set myH [winfo height $w]

    set cW [expr {$myW - $data(pad-x1) - $data(pad-x2) - 2*$data(-ipadx)}]
    set cH [expr {$myH - $data(pad-y1) - $data(pad-y2) - 2*$data(-ipady)}]
    set cX [expr {$data(pad-x1) + $data(-ipadx)}]
    set cY [expr {$data(pad-y1) + $data(-ipady)}]

    if {$cW > 0 && $cH > 0} {
	tixMoveResizeWindow $data(w:$child) $cX $cY $cW $cH
	tixMapWindow $data(w:$child)
	raise $data(w:$child)
    }
}



#----------------------------------------------------------------------
#
#	    G E O M E T R Y   M A N A G E M E N T
#
#----------------------------------------------------------------------
proc tixVStack:DestroyTop {w} {
    catch {
	destroy $w
    }
}

proc tixVStack:MasterGeomProc {w args} {
    if {![winfo exists $w]} {
	return
    }

    upvar #0 $w data

    if {$data(repack) == 0} {
	set data(repack) 1
	tixWidgetDoWhenIdle tixCallMethod $w Resize
    }
}

proc tixVStack:ClientGeomProc {w flag client} {
    if {![winfo exists $w]} {
	return
    }
    upvar #0 $w data

    if {$data(repack) == 0} {
	set data(repack) 1
	tixWidgetDoWhenIdle tixCallMethod $w Resize
    }

    if {$flag eq "-lostslave"} {
	error "Geometry Management Error: \
Another geometry manager has taken control of $client.\
This error is usually caused because a widget has been created\
in the wrong frame: it should have been created inside $client instead\
of $w"
    }
}

proc tixVStack:Resize {w} {
    if {![winfo exists $w]} {
	return
    }

    upvar #0 $w data

    if {$data(nWindows) == 0} {
	set data(repack) 0
	return
    }

    if {$data(-width) == 0 || $data(-height) == 0} {
	if {!$data(-dynamicgeometry)} {
	    # Calculate my required width and height
	    #
	    set maxW 1
	    set maxH 1

	    foreach child $data(windows) {
		set cW [winfo reqwidth  $data(w:$child)]
		set cH [winfo reqheight $data(w:$child)]

		if {$maxW < $cW} {
		    set maxW $cW
		}
		if {$maxH < $cH} {
		    set maxH $cH
		}
	    }
	    set reqW $maxW
	    set reqH $maxH
	} else {
	    if {$data(topchild) ne ""} {
		set reqW [winfo reqwidth  $data(w:$data(topchild))]
		set reqH [winfo reqheight $data(w:$data(topchild))]
	    } else {
		set reqW 1
		set reqH 1
	    }
	}

	incr reqW [expr {$data(pad-x1) + $data(pad-x2) + 2*$data(-ipadx)}]
	incr reqH [expr {$data(pad-y1) + $data(pad-y2) + 2*$data(-ipady)}]

	if {$reqW < $data(minW)} {
	    set reqW $data(minW)
	}
	if {$reqH < $data(minH)} {
	    set reqH $data(minH)
	}
    }
    # These take higher precedence
    #
    if {$data(-width)  != 0} {
	set reqW $data(-width)
    }
    if {$data(-height) != 0} {
	set reqH $data(-height)
    }

    if {[winfo reqwidth $w] != $reqW || [winfo reqheight $w] != $reqH} {
	if {![info exists data(counter)]} {
	    set data(counter) 0
	}
        if {$data(counter) < 50} {
            incr data(counter)
	    tixGeometryRequest $w $reqW $reqH
	    tixWidgetDoWhenIdle tixCallMethod $w Resize
	    set data(repack) 1
	    return
	}
    }
    set data(counter) 0

    if {$data(w:top) ne $w} {
	tixMoveResizeWindow $data(w:top) 0 0 [winfo width $w] [winfo height $w]
	tixMapWindow $data(w:top)
    }

    if {[string equal $data(topchild) ""]} {
	set top [lindex $data(windows) 0]
    } else {
	set top $data(topchild)
    }

    if {$top ne ""} {
	tixCallMethod $w raise $top
    }

    set data(repack) 0
}
