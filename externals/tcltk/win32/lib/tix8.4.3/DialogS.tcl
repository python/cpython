# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DialogS.tcl,v 1.5 2004/03/28 02:44:57 hobbs Exp $
#
# DialogS.tcl --
#
#
#	Implements the DialogShell widget. It tells the window
#	manager that it is a dialog window and should be treated specially.
#	The exact treatment depends on the treatment of the window
#	manager
#
# Copyright (c) 1994-1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixDialogShell {
    -superclass tixShell
    -classname  TixDialogShell
    -method {
	popdown popup center
    }
    -flag   {
	-mapped -minheight -minwidth -parent -transient
    }
    -static {}
    -configspec {
	{-mapped mapped Mapped 0}
	{-minwidth minWidth MinWidth 0}
	{-minheight minHeight MinHeight 0}
	{-transient transient Transient true}
	{-parent parent Parent ""}
    }
}

#----------------------------------------------------------------------
#		Construct widget
#----------------------------------------------------------------------

proc tixDialogShell:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    # Set the title of this shell appropriately
    #
    if {$data(-title) == ""} {
	# dynamically sets the title
	#
	set data(-title) [winfo name $w]
    }
    wm title $w $data(-title)

    # Set the parent of this dialog shell
    #
    if {$data(-parent) == ""} {
	set data(-parent) [winfo parent $w]
    }

    # Set the minsize and maxsize of the thing
    #
    wm minsize $w $data(-minwidth) $data(-minheight)
    wm transient $w ""
}

# The next procedures manage the dialog boxes
#
proc tixDialogShell:popup {w {parent ""}} {
    upvar #0 $w data

    # First update to make sure the boxes are the right size
    #
    update idletask

    # Then we set the position and update
    #
    # tixDialogShell:center $w $parent

    # and now make it visible. Viola!  Centered over parent.
    #
    wm deiconify $w
    after idle raise $w
}

# This procedure centers a dialog box over a window making sure that the 
# dialog box doesn't appear off the screen
#
# However, if the parent is smaller than this dialog, make this dialog
# appear at parent(x,y) + (20,20)
#
proc tixDialogShell:center {w {parent ""}} {
    upvar #0 $w data

    # Tell the WM that we'll do this ourselves.
    wm sizefrom $w user
    wm positionfrom $w user

    if {$parent == ""} {
	set parent $data(-parent)
    }
    if {$parent == "" || [catch {set parent [winfo toplevel $parent]}]} {
	set parent "."
    }

    # Where is my parent and what are it's dimensions
    #
    if {$parent != ""} {
	set pargeo [split [wm geometry $parent] "+x"]
	set parentW [lindex $pargeo 0]
	set parentH [lindex $pargeo 1]
	set parx [lindex $pargeo 2]
	set pary [lindex $pargeo 3]

	if {[string is true -strict $data(-transient)]} {
	    wm transient $w $parent
	}
    } else {
	set parentW [winfo screenwidth $w]
	set parentH [winfo screenheight $w]
	set parx 0
	set pary 0
	set parent [winfo parent $w]
    }

    # What are is the offset of the virtual window
    set vrootx [winfo vrootx $parent]
    set vrooty [winfo vrooty $parent]

    # What are my dimensions ?
    set dialogW [winfo reqwidth $w]
    set dialogH [winfo reqheight $w]

    if {$dialogW < $parentW-30 || $dialogW < $parentH-30} {
	set dialogx [expr {$parx+($parentW-$dialogW)/2+$vrootx}]
	set dialogy [expr {$pary+($parentH-$dialogH)/2+$vrooty}]
    } else {
	# This dialog is too big. Place it at (parentx, parenty) + (20,20)
	#
	set dialogx [expr {$parx+20+$vrootx}]
	set dialogy [expr {$pary+20+$vrooty}]
    }

    set maxx [expr {[winfo screenwidth  $parent] - $dialogW}]
    set maxy [expr {[winfo screenheight $parent] - $dialogH}]

    # Make sure it doesn't go off screen
    #
    if {$dialogx < 0} {
	set dialogx 0
    } else {
	if {$dialogx > $maxx} {
	    set dialogx $maxx
	}
    }
    if {$dialogy < 0} {
	set dialogy 0
    } else {
	if {$dialogy > $maxy} {
	    set dialogy $maxy
	}
    }

    # set my new position (and dimensions)
    #
    if {[wm geometry $w] == "1x1+0+0"} {
	wm geometry $w ${dialogW}x${dialogH}+${dialogx}+${dialogy}
    }
}

proc tixDialogShell:popdown {w args} {
    wm withdraw $w
}

