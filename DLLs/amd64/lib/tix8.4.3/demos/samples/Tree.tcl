# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Tree.tcl,v 1.4 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates how to use the TixTree widget to display
# hierachical data (A hypothetical DOS disk drive).
#

proc RunSample {w} {

    # We create the frame and the ScrolledHList widget
    # at the top of the dialog box
    #
    frame $w.top -relief raised -bd 1

    # Create a TixTree widget to display the hypothetical DOS disk drive
    # 
    #
    tixTree $w.top.a -options {
	separator "\\"
    }

    pack $w.top.a -expand yes -fill both -padx 10 -pady 10 -side left
 
    set tree $w.top.a 
    set hlist [$w.top.a subwidget hlist]

    # STEP (1) Add the directories into the TixTree widget (using the
    #	       hlist subwidget)

    set directories {
	C:
	C:\\Dos
	C:\\Windows
	C:\\Windows\\System
    }

    foreach d $directories {
	set text [lindex [split $d \\] end]
	$hlist add $d -itemtype imagetext \
	    -text $text -image [tix getimage folder]
    }

    # STEP (2) Use the "autosetmode" method of TixTree to indicate 
    #	       which entries can be opened or closed. The
    #	       "autosetmode" command will call the "setmode" method
    #	       to set the mode of each entry to the following:
    #
    #		"open" : the entry has some children and the children are
    #			 currently visible
    #		"close": the entry has some children and the children are
    #			 currently INvisible
    #		"none": the entry does not have children.
    #
    #    If you don't like the "autosetmode" method, you can always call
    #    "setmode" yourself, but that takes more work.
    
    $tree autosetmode

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "destroy $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}


# This "if" statement makes it possible to run this script file inside or
# outside of the main demo program "widget".
#
if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> {if {"%W" == ".demo"} exit}
}

