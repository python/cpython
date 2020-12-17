# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: StdBBox.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixStdButtonBox widget, which is a
# group of "Standard" buttons for Motif-like dialog boxes.
#
proc RunSample {w} {

    # Create the label on the top of the dialog box
    #
    label $w.top -padx 20 -pady 10 -border 1 -relief raised -text \
	"This dialog box is\n a demostration of the\n tixStdButtonBox widget" \
	-justify center -anchor c

    # Create the button box. We also do some manipulation of the
    # button widgets inside: we disable the help button and change
    # the label string of the "apply" button to "Filter"
    #
    # Note that the -text, -underline, -command and -width options are all
    # standard options of the button widgets.
    #
    tixStdButtonBox $w.box
    $w.box subwidget ok     config \
	-command "tixDemo:Status {OK pressed}; destroy $w"
    $w.box subwidget apply  config -text "Filter" -underline 0 \
	-command "tixDemo:Status {Filter pressed}"
    $w.box subwidget cancel config \
	-command "tixDemo:Status {Cancel pressed}; destroy $w"
    $w.box subwidget help config -state disabled

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes -anchor c


    # "after 0" is used so that the key bindings won't interfere with
    # tkTraverseMenu
    #
    bind [winfo toplevel $w] <Alt-o> \
	"after 0 tkButtonInvoke [$w.box subwidget ok]"
    bind [winfo toplevel $w] <Alt-f> \
	"after 0 tkButtonInvoke [$w.box subwidget apply]"
    bind [winfo toplevel $w] <Alt-c> \
	"after 0 tkButtonInvoke [$w.box subwidget cancel]"
    bind [winfo toplevel $w] <Escape> \
	"after 0 tkButtonInvoke [$w.box subwidget cancel]"

    focus [$w.box subwidget apply] 
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
