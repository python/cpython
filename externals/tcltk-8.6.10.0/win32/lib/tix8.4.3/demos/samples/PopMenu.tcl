# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: PopMenu.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixPopupMenu widget.
#

proc RunSample {w} {

    # We create the frame and the button, then we'll bind the PopupMenu
    # to both widgets. The result is, when you press the right mouse
    # button over $w.top or $w.top.but, the PopupMenu will come up.
    #

    frame $w.top -relief raised -bd 1

    button $w.top.but -text {Press the right mouse button over
this button or its surrounding area}

    pack $w.top.but -expand yes -fill both -padx 50 -pady 50

    tixPopupMenu $w.top.p -title "Popup Test"
    $w.top.p bind $w.top
    $w.top.p bind $w.top.but

    # Set the entries inside the PopupMenu widget. 
    # [Hint] You have to manipulate the "menu" subwidget.
    #	     $w.top.p itself is NOT a menu widget.
    # [Hint] Watch carefully how the sub-menu is created
    #
    set menu [$w.top.p subwidget menu]
    $menu add command -label Desktop -under 0
    $menu add command -label Select  -under 0
    $menu add command -label Find    -under 0
    $menu add command -label System  -under 1
    $menu add command -label Help    -under 0
    $menu add cascade -label More -menu $menu.m1
    menu $menu.m1
    $menu.m1 add command -label Hello

    pack $w.top.but -side top -padx 40 -pady 50

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
