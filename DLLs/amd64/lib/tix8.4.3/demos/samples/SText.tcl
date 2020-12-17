# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SText.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixScrolledText widget.
#

proc RunSample {w} {

    # We create the frame and the ScrolledText widget
    # at the top of the dialog box
    #
    frame $w.top -relief raised -bd 1

    # Create a Scrolled Text widget.
    #
    tixScrolledText $w.top.a
    pack $w.top.a -expand yes -fill both -padx 10 -pady 10 -side left

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "destroy $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes

    # Put the junk inside the text subwidget of the ScrolledText widget
    #
    $w.top.a subwidget text insert end {
Mon, 19 Jun 1995 11:39:52        comp.lang.tcl              Thread   34 of  220
Lines 353       A new way to put text and bitmaps together
ioi@xpi.com                Ioi K. Lam at Expert Interface Technologies

Hi,

I have implemented a new image type called "compound". It allows you
to glue together a bunch of bitmaps, images and text strings together
to form a bigger image. Then you can use this image with widgets that
support the -image option. This way you can display very fancy stuffs
in your GUI. For example, you can display a text string string
together with a bitmap, at the same time, inside a TK button widget. A
screenshot of compound images can be found at the bottom of this page:

        http://www.xpi.com/tix/screenshot.html

You can also you is in other places such as putting fancy bitmap+text
in menus, tabs of tixNoteBook widgets, etc. This feature will be
included in the next release of Tix (4.0b1). Count on it to make jazzy
interfaces!}
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

