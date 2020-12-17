#
# Bindings for TScrollbar widget
#

# Still don't have a working ttk::scrollbar under OSX -
# Swap in a [tk::scrollbar] on that platform,
# unless user specifies -class or -style.
#
if {[tk windowingsystem] eq "aqua"} {
    rename ::ttk::scrollbar ::ttk::_scrollbar
    proc ttk::scrollbar {w args} {
	set constructor ::tk::scrollbar
	foreach {option _} $args {
	    if {$option eq "-class" || $option eq "-style"} {
		set constructor ::ttk::_scrollbar
		break
	    }
	}
	return [$constructor $w {*}$args]
    }
}

namespace eval ttk::scrollbar {
    variable State
    # State(xPress)	--
    # State(yPress)	-- initial position of mouse at start of drag.
    # State(first)	-- value of -first at start of drag.
}

bind TScrollbar <ButtonPress-1> 	{ ttk::scrollbar::Press %W %x %y }
bind TScrollbar <B1-Motion>		{ ttk::scrollbar::Drag %W %x %y }
bind TScrollbar <ButtonRelease-1>	{ ttk::scrollbar::Release %W %x %y }

bind TScrollbar <ButtonPress-2> 	{ ttk::scrollbar::Jump %W %x %y }
bind TScrollbar <B2-Motion>		{ ttk::scrollbar::Drag %W %x %y }
bind TScrollbar <ButtonRelease-2>	{ ttk::scrollbar::Release %W %x %y }

proc ttk::scrollbar::Scroll {w n units} {
    set cmd [$w cget -command]
    if {$cmd ne ""} {
	uplevel #0 $cmd scroll $n $units
    }
}

proc ttk::scrollbar::Moveto {w fraction} {
    set cmd [$w cget -command]
    if {$cmd ne ""} {
	uplevel #0 $cmd moveto $fraction
    }
}

proc ttk::scrollbar::Press {w x y} {
    variable State

    set State(xPress) $x
    set State(yPress) $y

    switch -glob -- [$w identify $x $y] {
    	*uparrow -
	*leftarrow {
	    ttk::Repeatedly Scroll $w -1 units
	}
	*downarrow -
	*rightarrow {
	    ttk::Repeatedly Scroll $w  1 units
	}
	*thumb {
	    set State(first) [lindex [$w get] 0]
	}
	*trough {
	    set f [$w fraction $x $y]
	    if {$f < [lindex [$w get] 0]} {
		# Clicked in upper/left trough
		ttk::Repeatedly Scroll $w -1 pages
	    } elseif {$f > [lindex [$w get] 1]} {
		# Clicked in lower/right trough
		ttk::Repeatedly Scroll $w 1 pages
	    } else {
		# Clicked on thumb (???)
		set State(first) [lindex [$w get] 0]
	    }
	}
    }
}

proc ttk::scrollbar::Drag {w x y} {
    variable State
    if {![info exists State(first)]} {
    	# Initial buttonpress was not on the thumb, 
	# or something screwy has happened.  In either case, ignore:
	return;
    }
    set xDelta [expr {$x - $State(xPress)}]
    set yDelta [expr {$y - $State(yPress)}]
    Moveto $w [expr {$State(first) + [$w delta $xDelta $yDelta]}]
}

proc ttk::scrollbar::Release {w x y} {
    variable State
    unset -nocomplain State(xPress) State(yPress) State(first)
    ttk::CancelRepeat
}

# scrollbar::Jump -- ButtonPress-2 binding for scrollbars.
# 	Behaves exactly like scrollbar::Press, except that
#	clicking in the trough jumps to the the selected position.
#
proc ttk::scrollbar::Jump {w x y} {
    variable State

    switch -glob -- [$w identify $x $y] {
	*thumb -
	*trough {
	    set State(first) [$w fraction $x $y]
	    Moveto $w $State(first)
	    set State(xPress) $x
	    set State(yPress) $y
	}
	default {
	    Press $w $x $y
	}
    }
}
