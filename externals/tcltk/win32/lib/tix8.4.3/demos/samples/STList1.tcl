# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: STList1.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
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

    # Create the scrolled tlist
    #
    tixScrolledTList $top.st -options {
	tlist.orient vertical
	tlist.selectMode single
    }
    pack $top.st -expand yes -fill both -padx 10 -pady 10

    # Insert a list of numbers into the tlist subwidget
    #
    set tlist [$top.st subwidget tlist]

    set numbers {
	one two three fours five six seven eight nine ten eleven
	twelve thirdteen fourteen
    }

    foreach num $numbers {
	$tlist insert end -itemtype imagetext -text $num \
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
