# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: BtnBox.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixButtonBox widget, which is a
# group of TK buttons. You can use it to manage the buttons in a dialog box,
# for example.
#
proc RunSample {w} {

    # Create the label on the top of the dialog box
    #
    label $w.top -padx 20 -pady 10 -border 1 -relief raised -anchor c -text \
	"This dialog box is\n a demostration of the\n tixButtonBox widget"

    # Create the button box and add a few buttons in it. Set the
    # -width of all the buttons to the same value so that they
    # appear in the same size.
    #
    # Note that the -text, -underline, -command and -width options are all
    # standard options of the button widgets.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok    -text OK    -underline 0 -command "destroy $w" -width 5
    $w.box add close -text Close -underline 0 -command "destroy $w" -width 5

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes

    # "after 0" is used so that the key bindings won't interfere with
    # tkTraverseMenu
    #
    bind [winfo toplevel $w] <Alt-o>  \
	"after 0 tkButtonInvoke [$w.box subwidget ok]"
    bind [winfo toplevel $w] <Alt-c>  \
	"after 0 tkButtonInvoke [$w.box subwidget close]"
    bind [winfo toplevel $w] <Escape> \
	"after 0 tkButtonInvoke [$w.box subwidget close]"

    focus [$w.box subwidget ok] 
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
