# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SWindow.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixScrolledWindow widget.
#

proc RunSample {w} {

    # We create the frame and the ScrolledWindow widget
    # at the top of the dialog box
    #
    frame $w.top -relief raised -bd 1

    # Create a complex window inside the ScrolledWindow widget.
    # ScrolledWindow are very convenient: unlink the canvas widget,
    # you don't need to specify the scroll-redions for the
    # ScrolledWindow. It will automatically adjust itself to fit
    # size of the "window" subwidget
    #
    # [Hint] Be sure you create and pack new widgets inside the
    #	     "window" subwidget and NOT inside $w.top.a itself!
    #
    tixScrolledWindow $w.top.a
    pack $w.top.a -expand yes -fill both -padx 10 -pady 10 -side left

    set f [$w.top.a subwidget window]
    tixNoteBook $f.nb
    pack $f.nb -expand yes -fill both -padx 20 -pady 20


    $f.nb add image   -label "Image"   -underline 0
    $f.nb add buttons -label "Buttons" -underline 0

    # The first page: an image
    #
    global demo_dir
    set p [$f.nb subwidget image]
    set im [image create photo -file $demo_dir/bitmaps/tix.gif]
    label $p.lab -image $im
    pack $p.lab -padx 20 -pady 20

    # The second page: buttons
    #
    set p [$f.nb subwidget buttons]
    button $p.b1 -text "Welcome"  -width 8
    button $p.b2 -text "to"       -width 8
    button $p.b3 -text "the"      -width 8
    button $p.b4 -text "World"    -width 8
    button $p.b5 -text "of"       -width 8
    button $p.b6 -text "Tix"      -width 8

    pack $p.b1 $p.b2 $p.b3 $p.b4 $p.b5 $p.b6 -anchor c -side top


    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "destroy $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes

    wm geometry $w 240x220
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

