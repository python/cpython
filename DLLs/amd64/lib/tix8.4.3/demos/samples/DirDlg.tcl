# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DirDlg.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixDirSelectDialog widget:
# it allows the user to select a directory.
#
proc RunSample {w} {

    # Create an entry for the user to input a directory. If he can't
    # bother to type in the name, he can press the "Browse ..." button
    # and call up the diretcory dialog
    #
    frame $w.top -border 1 -relief raised

    tixLabelEntry $w.top.ent -label "Select A Directory:" -labelside top \
	-options {
	    entry.width 25
	    entry.textVariable demo_ddlg_dirname
	    label.anchor w
	}
    bind [$w.top.ent subwidget entry] <Return> "ddlg:okcmd $w"

    uplevel #0 set demo_ddlg_dirname {}

    button $w.top.btn -text "Browse ..." -command "ddlg:browse"

    pack $w.top.ent -side left -expand yes -fill x -anchor s -padx 4 -pady 4
    pack $w.top.btn -side left -anchor s -padx 4 -pady 4

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "ddlg:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

# Pop up a directory selection dialog
#
proc ddlg:browse {} {
    set dialog .dirdlg_popup
    if ![winfo exists $dialog] {
	tixDirSelectDialog $dialog
    }
    $dialog config -command ddlg:select_dir

    $dialog popup
}

proc ddlg:select_dir {dir} {
    global demo_ddlg_dirname 

    set demo_ddlg_dirname $dir
}

proc ddlg:okcmd {w} {
    global demo_ddlg_dirname 

    if {$demo_ddlg_dirname != {}} {
	tixDemo:Status "You have selected the directory $demo_ddlg_dirname"
    } else {
	tixDemo:Status "You haven't selected any directory"
    }

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
