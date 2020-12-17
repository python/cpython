# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: ListNBk.tcl,v 1.5 2004/03/28 02:44:57 hobbs Exp $
#
# ListNBk.tcl --
#
#	"List NoteBook" widget. Acts similarly to the notebook but uses a
#	HList widget to represent the pages.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixListNoteBook {
    -classname TixListNoteBook
    -superclass tixVStack
    -method {
    }
    -flag {
	-height -width
    }
    -configspec {
	{-width width Width 0}
	{-height height Height 0}
    }
    -forcecall {
	-dynamicgeometry -width -height
    }
    -default {
	{*Orientation		horizontal}
    }
}

proc tixListNoteBook:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
    set data(w_pane) [tixPanedWindow $w.pane -panerelief flat]
    set p1 [$data(w_pane) add p1 -expand 0]
    set p2 [$data(w_pane) add p2 -expand 1]
    set data(w_p2) $p2
    set data(w:shlist) [tixScrolledHList $p1.shlist]
    set data(w:hlist) [$data(w:shlist) subwidget hlist]

    if {[$data(w_pane) cget -orientation] eq "vertical"} {
	pack $data(w:shlist) -expand yes -fill both -padx 2 -pady 3
    } else {
	pack $data(w:shlist) -expand yes -fill both -padx 3 -pady 2
    }

    $data(w:hlist) config \
	-command   [list tixListNoteBook:Choose $w] \
	-browsecmd [list tixListNoteBook:Choose $w] \
	-selectmode single

    pack $data(w_pane) -expand yes -fill both
}

proc tixListNoteBook:add {w child args} {
    upvar #0 $w data

    if {[string match *.* $child]} {
	error "the name of the page cannot contain the \".\" character"
    }
    return [eval tixChainMethod $w add $child $args]
}

#----------------------------------------------------------------------
# Virtual Methods
#----------------------------------------------------------------------
proc tixListNoteBook:InitGeometryManager {w} {
    tixWidgetDoWhenIdle tixListNoteBook:InitialRaise $w
}

proc tixListNoteBook:InitialRaise {w} {
    upvar #0 $w data

    if {$data(topchild) eq ""} {
	set top [lindex $data(windows) 0]
    } else {
	set top $data(topchild)
    }

    if {$top ne ""} {
	tixCallMethod $w raise $top
    }
}

proc tixListNoteBook:CreateChildFrame {w child} {
    upvar #0 $w data
    return [frame $data(w_p2).$child]
}

proc tixListNoteBook:RaiseChildFrame {w child} {
    upvar #0 $w data

    if {$data(topchild) ne $child} {
	if {$data(topchild) ne ""} {
	    pack forget $data(w:$data(topchild))
	}
	pack $data(w:$child) -expand yes -fill both
    }
}

#
#----------------------------------------------------------------------
#

proc tixListNoteBook:config-dynamicgeometry {w value} {
    upvar #0 $w data

    $data(w_pane) config -dynamicgeometry $value
}

proc tixListNoteBook:config-width {w value} {
    upvar #0 $w data

    if {$value != 0} {
	$data(w_pane) config -width $value
    }
}

proc tixListNoteBook:config-height {w value} {
    upvar #0 $w data

    if {$value != 0} {
	$data(w_pane) config -height $value
    }
}

proc tixListNoteBook:raise {w child} {
    upvar #0 $w data

    $data(w:hlist) selection clear
    $data(w:hlist) selection set $child
    $data(w:hlist) anchor set $child

    tixChainMethod $w raise $child
}

proc tixListNoteBook:Choose {w args} {
    upvar #0 $w data

    set entry [tixEvent flag V]

    if {[lsearch $data(windows) $entry] != -1} {
	tixCallMethod $w raise $entry
    }
}
