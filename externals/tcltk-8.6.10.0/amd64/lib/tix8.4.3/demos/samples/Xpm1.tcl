# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Xpm1.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of XPM images in the menu.
#

proc RunSample {w} {

    set hard_disk_pixmap {/* XPM */
	static char * drivea_xpm[] = {
	    /* width height ncolors chars_per_pixel */
	    "32 32 5 1",
	    /* colors */
	    " 	s None	c None",
	    ".	c #000000000000",
	    "X	c white",
	    "o	c #c000c000c000",
	    "O	c #800080008000",
	    /* pixels */
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "   ..........................   ",
	    "   .XXXXXXXXXXXXXXXXXXXXXXXo.   ",
	    "   .XooooooooooooooooooooooO.   ",
	    "   .Xooooooooooooooooo..oooO.   ",
	    "   .Xooooooooooooooooo..oooO.   ",
	    "   .XooooooooooooooooooooooO.   ",
	    "   .Xoooooooo.......oooooooO.   ",
	    "   .Xoo...................oO.   ",
	    "   .Xoooooooo.......oooooooO.   ",
	    "   .XooooooooooooooooooooooO.   ",
	    "   .XooooooooooooooooooooooO.   ",
	    "   .XooooooooooooooooooooooO.   ",
	    "   .XooooooooooooooooooooooO.   ",
	    "   .oOOOOOOOOOOOOOOOOOOOOOOO.   ",
	    "   ..........................   ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                ",
	    "                                "};
    }
    # We create the frame and the ScrolledText widget
    # at the top of the dialog box
    #
    frame $w.top -relief raised -bd 1

    set m [frame $w.top.menu -relief raised -bd 2]
    set mb [menubutton $m.mb -text Options -menu $m.mb.m]
    set menu [menu $mb.m]

    pack $m -side top -fill x
    pack $mb -side left -fill y

    # Put the label there
    #
    set lab [label $w.top.label -text "Go to the \"Options\" menu" -anchor c]
    pack $lab -padx 40 -pady 40 -fill both -expand yes

    set image [image create pixmap -data $hard_disk_pixmap]
    $menu add command -image  $image \
	-command "$lab config -image $image"

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "destroy $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes

    wm geometry $w 300x300
}


# This "if" statement makes it possible to run this script file inside or
# outside of the main demo program "widget".
#
if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind .demo <Destroy> exit
}

