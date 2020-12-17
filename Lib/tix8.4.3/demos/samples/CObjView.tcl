# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: CObjView.tcl,v 1.4 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This program demonstrates the use of the CObjView (Canvas Object
# View) class.
#
# $Id: CObjView.tcl,v 1.4 2001/12/09 05:31:07 idiscovery Exp $

proc RunSample {w} {
    label $w.lab  -justify left -text \
"Click on the buttons to add or delete canvas
objects randomally. Notice the scrollbars automatically
adjust to include all objects in the scroll-region."

    pack $w.lab -anchor c -padx 10 -pady 6 -side top
    frame $w.f
    pack $w.f -side bottom -fill y
    tixCObjView $w.c
    pack $w.c -expand yes -fill both -padx 4 -pady 2 -side top
    button $w.add -command "CVDemo_Add $w.c"    -text Add    -width 6
    button $w.del -command "CVDemo_Delete $w.c" -text Delete -width 6
    button $w.exit -command "destroy $w" -text Exit -width 6
    pack $w.add $w.del $w.exit -side left -padx 20 -pady 10 \
        -anchor c -expand yes -in $w.f
}

proc CVDemo_Add {cov} {
    # Generate four pseudo random numbers (x,y,w,h) to define the coordinates
    # of a rectangle object in the canvas.
    #
    set colors {red green blue white black gray yellow}

    set x [expr int(rand() * 400) - 120]
    set y [expr int(rand() * 400) - 120]
    set w [expr int(rand() * 120)]
    set h [expr int(rand() * 120)]

    # Create the canvas object
    #
    $cov subwidget canvas create rectangle $x $y [expr $x+$w] [expr $y+$h] \
	-fill [lindex $colors [expr int(rand() * [llength $colors])]]

    # Call the adjustscrollregion command to set the scroll bars so that all
    # objects are included in the scroll-region
    #
    $cov adjustscrollregion
}

proc CVDemo_Delete {cov} {
    set px [lindex [time update] 0]
    set w [$cov subwidget canvas]
    set items [$w find withtag all]

    if [string compare $items ""] {
	# There are items in the canvas, randomally delete one of them
	# and re-adjust the scroll-region
	#
	set toDelete [expr $px % [llength $items]]
	$w delete [lindex $items $toDelete]

	$cov adjustscrollregion
    }
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> {after 10 exit}
}
