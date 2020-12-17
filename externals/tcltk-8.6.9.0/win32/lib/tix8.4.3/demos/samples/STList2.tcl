# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: STList2.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# Demonstrates the scrolled tlist widget
#

proc RunSample {w} {
    set top [frame $w.f -bd 1 -relief raised]
    set box [tixButtonBox $w.b -bd 1 -relief raised]

    pack $box -side bottom -fill both
    pack $top -side top -fill both -expand yes

    # Create the Paned Window to contain two scrolled tlist's
    #
    set p [tixPanedWindow $top.p -orient horizontal]
    pack $p -expand yes -fill both -padx 4 -pady 4

    set p1 [$p add pane1 -expand 1]
    set p2 [$p add pane2 -expand 1]

    $p1 config -relief flat
    $p2 config -relief flat

    # Create a TList with vertical orientation
    #
    tixScrolledTList $p1.st -options {
	tlist.orient vertical
	tlist.selectMode single
    }
    label $p1.lab -text "Vertical Orientation"

    pack $p1.lab -anchor c -side top -pady 2
    pack $p1.st -expand yes -fill both -padx 10 -pady 10

    # Create a TList with horizontal orientation
    #	
    tixScrolledTList $p2.st -options {
	tlist.orient horizontal
	tlist.selectMode single
    }
    label $p2.lab -text "Horizontal Orientation"

    pack $p2.lab -anchor c -side top -pady 2
    pack $p2.st -expand yes -fill both -padx 10 -pady 10

    # Insert a list of numbers into the two tlist subwidget's
    #
    set vt [$p1.st subwidget tlist]
    set ht [$p2.st subwidget tlist]

    set numbers {
	one two three fours five six seven eight nine ten eleven
	twelve thirdteen fourteen
    }

    foreach num $numbers {
	$vt insert end -itemtype imagetext -text $num \
	    -image [tix getimage openfold]
	$ht insert end -itemtype imagetext -text $num \
	    -image [tix getimage openfold]
    }

    # Create the buttons
    #
    $box add ok     -text Ok     -command "destroy $w" -width 6
    $box add cancel -text Cancel -command "destroy $w" -width 6
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
