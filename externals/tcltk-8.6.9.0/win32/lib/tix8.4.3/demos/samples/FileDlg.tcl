# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: FileDlg.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixFileSelectDialog widget --
# This is a neat file selection dialog that looks like the Motif 
# file-selection dialog widget. I know that Motif sucks, but 
# tixFileSelectDialog looks neat nevertheless.
#
proc RunSample {w} {

    # Create an entry for the user to input a filename. If he can't
    # bother to type in the name, he can press the "Browse ..." button
    # and call up the file dialog
    #
    frame $w.top -border 1 -relief raised

    tixLabelEntry $w.top.ent -label "Select A File:" -labelside top \
	-options {
	    entry.width 25
	    entry.textVariable demo_fdlg_filename
	    label.anchor w
	}
    bind [$w.top.ent subwidget entry] <Return> "fdlg:okcmd $w"

    uplevel #0 set demo_fdlg_filename {}


    button $w.top.btn -text "Browse ..." -command "fdlg:browse"

    pack $w.top.ent -side left -expand yes -fill x -anchor s -padx 4 -pady 4
    pack $w.top.btn -side left -anchor s -padx 4 -pady 4

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "fdlg:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

# Pop up a file selection dialog
#
proc fdlg:browse {} {
    # [Hint]
    # The best way to use an FileSelectDialog is not to create one yourself
    # but to call the command "tix filedialog". This command creates one file
    # dialog box that is shared by different parts of the application.
    # This way, your application can save resources because it doesn't
    # need to create a lot of file dialog boxes even if it needs to input
    # file names at a lot of different occasions.
    #
    set dialog [tix filedialog tixFileSelectDialog]
    $dialog config -command fdlg:select_file

    $dialog popup
}

proc fdlg:select_file {file} {
    global demo_fdlg_filename 

    set demo_fdlg_filename $file
}

proc fdlg:okcmd {w} {
    global demo_fdlg_filename 

    if {$demo_fdlg_filename != {}} {
	tixDemo:Status "You have selected the file $demo_fdlg_filename"
    } else {
	tixDemo:Status "You haven't selected any file"
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
