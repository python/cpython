# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: FileEnt.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixFileEntry widget -- an
# easy of letting the user select a filename
#
proc RunSample {w} {

    # Create the tixFileEntry's on the top of the dialog box
    #
    frame $w.top -border 1 -relief raised

    global demo_fent_from demo_fent_to

    tixFileEntry $w.top.a -label "Move File From: " \
	-variable demo_fent_from \
	-options {
	    entry.width 25
	    label.width 16
	    label.underline 10
	    label.anchor e
	}

    tixFileEntry $w.top.b -label "To: " \
	-variable demo_fent_to \
	-options {
	    entry.width 25
	    label.underline 0
	    label.width 16
	    label.anchor e
	}

    pack $w.top.a $w.top.b -side top -anchor w -pady 3

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "fent:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes

    # Let's set some nice bindings for keyboard accelerators
    #
    bind $w <Alt-f> "focus $w.top.a" 
    bind $w <Alt-t> "focus $w.top.b" 
    bind $w <Alt-o> "[$w.box subwidget ok] invoke; break" 
    bind $w <Alt-c> "[$w.box subwidget cancel] invoke; break" 
}

proc fent:okcmd {w} {
    global demo_fent_from demo_fent_to

    # tixDemo:Status "You wanted to move file from $demo_fent_from to $demo_fent_to"

    destroy $w
}


# This "if" statement makes it possible to run this script file inside or
# outside of the main demo program "widget".
#
if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
