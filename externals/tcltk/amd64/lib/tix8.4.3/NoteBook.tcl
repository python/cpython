# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: NoteBook.tcl,v 1.7 2004/03/28 02:44:57 hobbs Exp $
#
# NoteBook.tcl --
#
#	tixNoteBook: NoteBook type of window.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixNoteBook {
    -classname TixNoteBook
    -superclass tixVStack
    -method {
    }
    -flag {
    }
    -configspec {
	{-takefocus takeFocus TakeFocus 0 tixVerifyBoolean}
    }
    -default {
	{.nbframe.tabPadX	8}
	{.nbframe.tabPadY	5}
	{.nbframe.borderWidth	2}
	{*nbframe.relief	raised}
    }
}

proc tixNoteBook:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(pad-x1) 0
    set data(pad-x2) 0
    set data(pad-y1) 20
    set data(pad-y2) 0
}

proc tixNoteBook:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    set data(w:top) [tixNoteBookFrame $w.nbframe -slave 1 -takefocus 1]
    set data(w:nbframe) $data(w:top)

    bind $data(w:top) <ButtonPress-1> [list tixNoteBook:MouseDown $w %x %y]
    bind $data(w:top) <ButtonRelease-1> [list tixNoteBook:MouseUp $w %x %y]

    bind $data(w:top) <B1-Motion> [list tixNoteBook:MouseDown $w %x %y]

    bind $data(w:top) <Left>  [list tixNoteBook:FocusNext $w prev]
    bind $data(w:top) <Right> [list tixNoteBook:FocusNext $w next]

    bind $data(w:top) <Return> [list tixNoteBook:SetFocusByKey $w]
    bind $data(w:top) <space>  [list tixNoteBook:SetFocusByKey $w]
}

#----------------------------------------------------------------------
# Public methods
#----------------------------------------------------------------------
proc tixNoteBook:add {w child args} {
    upvar #0 $w data

    set ret [eval tixChainMethod $w add $child $args]

    set new_args ""
    foreach {flag value} $args {
	if {$flag ne "-createcmd" && $flag ne "-raisecmd"} {
	    lappend new_args $flag
	    lappend new_args $value
	}
    }

    eval [linsert $new_args 0 $data(w:top) add $child]

    return $ret
}

proc tixNoteBook:raise {w child} {
    upvar #0 $w data

    tixChainMethod $w raise $child

    if {[$data(w:top) pagecget $child -state] eq "normal"} {
	$data(w:top) activate $child
    }
}

proc tixNoteBook:delete {w child} {
    upvar #0 $w data

    tixChainMethod $w delete $child
    $data(w:top) delete $child
}

#----------------------------------------------------------------------
# Private methods
#----------------------------------------------------------------------
proc tixNoteBook:Resize {w} {
    upvar #0 $w data

    # We have to take care of the size of the tabs so that 
    #
    set rootReq [$data(w:top) geometryinfo]
    set tW [lindex $rootReq 0]
    set tH [lindex $rootReq 1]

    set data(pad-x1) 2
    set data(pad-x2) 2
    set data(pad-y1) [expr {$tH + $data(-ipadx) + 1}]
    set data(pad-y2) 2
    set data(minW)   [expr {$tW}]
    set data(minH)   [expr {$tH}]

    # Now that we know data(pad-y1), we can chain the call
    #
    tixChainMethod $w Resize
}

proc tixNoteBook:MouseDown {w x y} {
    upvar #0 $w data

    focus $data(w:top)

    set name [$data(w:top) identify $x $y]
    $data(w:top) focus $name
    set data(w:down) $name
}

proc tixNoteBook:MouseUp {w x y} {
    upvar #0 $w data

    #it could happen (using the tk/menu) that a MouseUp
    #proceeds without a MouseDown event!!
    if {![info exists data(w:down)] || ![info exists data(w:top)]} {
	return
    }

    set name [$data(w:top) identify $x $y]

    if {$name ne "" && $name eq $data(w:down)
	&& [$data(w:top) pagecget $name -state] eq "normal"} {
        $data(w:top) activate $name
        tixCallMethod $w raise $name
    } else {
        $data(w:top) focus ""
    }
}


#----------------------------------------------------------------------
#
# Section for keyboard bindings
#
#----------------------------------------------------------------------

proc tixNoteBook:FocusNext {w dir} {
    upvar #0 $w data

    if {[$data(w:top) info focus] == ""} {
	set name [$data(w:top) info active]
	$data(w:top) focus $name

	if {$name ne ""} {
	    return
	}
    } else {
	set name [$data(w:top) info focus$dir]
 	$data(w:top) focus $name
   }
}

proc tixNoteBook:SetFocusByKey {w} {
    upvar #0 $w data

    set name [$data(w:top) info focus]

    if {$name ne "" && [$data(w:top) pagecget $name -state] eq "normal"} {
	tixCallMethod $w raise $name
	$data(w:top) activate $name
    }
}

#----------------------------------------------------------------------
# Automatic bindings for alt keys
#----------------------------------------------------------------------
proc tixNoteBookFind {w char} {
    set char [string tolower $char]

    foreach child [winfo child $w] {
	if {![winfo ismapped $w]} {
	    continue
	}
	switch -exact -- [winfo class $child] {
	    Toplevel    { continue }
	    TixNoteBook {
		set nbframe [$child subwidget nbframe]
		foreach page [$nbframe info pages] {
		    set char2 [string index [$nbframe pagecget $page -label] \
				   [$nbframe pagecget $page -underline]]
		    if {($char eq [string tolower $char2] || $char eq "")
			&& [$nbframe pagecget $page -state] ne "disabled"} {
			return [list $child $page]
		    }
		}
	    }
	}
	# Well, this notebook doesn't match with the key, but maybe
	# it contains a "subnotebook" that will match ..
	set match [tixNoteBookFind $child $char]
	if {$match ne ""} {
	    return $match
	}
    }
    return ""
}

proc tixTraverseToNoteBook {w char} {
    if {$char eq ""} {
	return 0
    }
    if {![winfo exists $w]} {
	return 0
    }
    set list [tixNoteBookFind [winfo toplevel $w] $char]
    if {$list ne ""} {
	[lindex $list 0] raise [lindex $list 1]
	return 1
    }
    return 0
}

#----------------------------------------------------------------------
# Set default class bindings
#----------------------------------------------------------------------

bind all <Alt-KeyPress> "+tixTraverseToNoteBook %W %A"
bind all <Meta-KeyPress> "+tixTraverseToNoteBook %W %A"

