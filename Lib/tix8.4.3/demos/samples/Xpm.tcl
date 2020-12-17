# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Xpm.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of XPM images.
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

    frame $w.top -relief raised -bd 1
    button $w.top.b -image [image create pixmap -data $hard_disk_pixmap]
    pack $w.top -expand yes -fill both
    pack $w.top.b -expand yes -padx 20  -pady 20

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
    bind $w <Destroy> exit
}

