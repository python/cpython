# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DirTree.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixDirTree widget -- you can
# use it for the user to select a directory. For example, an installation
# program can use the tixDirList widget to ask the user to select the
# installation directory for an application.
#
proc RunSample {w} {

    # Create the tixDirTree and the tixLabelEntry widgets on the on the top
    # of the dialog box
    #
    frame $w.top -border 1 -relief raised

    # Create the DirTree widget. By default it will show the current
    # directory (returned by [pwd])
    #
    #
    tixDirTree $w.top.dir -browsecmd "dtree:browse $w.top.ent"

    # When the user presses the ".." button, the selected directory
    # is "transferred" into the entry widget
    #

    # We use a LabelEntry to hold the installation directory. The user
    # can choose from the DirTree widget, or he can type in the directory 
    # manually
    #
    tixLabelEntry $w.top.ent -label "Installation Directory:" -labelside top \
	-options {
	    entry.width 25
	    entry.textVariable demo_dtree_dir
	    label.anchor w
	}
    bind [$w.top.ent subwidget entry] <Return> "dtree:okcmd $w"

    uplevel #0 set demo_dtree_dir [list [pwd]]

    pack $w.top.dir -side left -expand yes -fill both -padx 4 -pady 4
    pack $w.top.ent -side left -fill x -anchor c -padx 4 -pady 4

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "dtree:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

proc dtree:browse {ent filename} {
    uplevel #0 set demo_dtree_dir $filename

}

proc dtree:copy_name {w} {
    global demo_dtree_dir

    set demo_dtree_dir [$w cget -value]
}

proc dtree:okcmd {w} {
    global demo_dtree_dir

    tixDemo:Status "You have selected the directory $demo_dtree_dir"

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
