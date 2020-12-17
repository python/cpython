# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DirList.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixDirList widget -- you can
# use it for the user to select a directory. For example, an installation
# program can use the tixDirList widget to ask the user to select the
# installation directory for an application.
#
proc RunSample {w} {

    # Create the tixDirList and the tixLabelEntry widgets on the on the top
    # of the dialog box
    #
    frame $w.top -border 1 -relief raised

    # Create the DirList widget. By default it will show the current
    # directory (returned by [pwd])
    #
    #
    tixDirList $w.top.dir

    # When the user presses the ".." button, the selected directory
    # is "transferred" into the entry widget
    #
    button $w.top.btn -text "  >>  " -pady 0 \
	-command "dlist:copy_name $w.top.dir"

    # We use a LabelEntry to hold the installation directory. The user
    # can choose from the DirList widget, or he can type in the directory 
    # manually
    #
    tixLabelEntry $w.top.ent -label "Installation Directory:" -labelside top \
	-options {
	    entry.width 25
	    entry.textVariable demo_dlist_dir
	    label.anchor w
	}
    bind [$w.top.ent subwidget entry] <Return> "dlist:okcmd $w"

    uplevel #0 set demo_dlist_dir [list [pwd]]

    pack $w.top.dir -side left -expand yes -fill both -padx 4 -pady 4
    pack $w.top.btn -side left -anchor s -padx 4 -pady 4
    pack $w.top.ent -side left -fill x -anchor s -padx 4 -pady 4

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "dlist:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}


proc dlist:copy_name {w} {
    global demo_dlist_dir

    set demo_dlist_dir [$w cget -value]
}

proc dlist:okcmd {w} {
    global demo_dlist_dir

    tixDemo:Status "You have selected the directory $demo_dlist_dir"

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
    bind $w <Destroy> "exit"
}
