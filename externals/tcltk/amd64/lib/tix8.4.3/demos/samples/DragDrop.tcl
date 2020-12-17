# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DragDrop.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the Drag+Drop features in Tix. Drag+Drop is still
# experimental in Tix. Please don't use. For your eyes only.
#
#
proc RunSample {w} {

    text $w.d -height 5
    $w.d insert end {Quick and dirty example:
click on any node on on the directory lists and drag. You can see the
cursor change its shape. The "dropsite" of the directory lists will be
highlighted when you drag the cursor accorss the directory nodes.
Nothing will happen when you drop. }

    pack $w.d -padx 10 -pady 5

    tixDirList $w.d1; pack $w.d1 -fill both -padx 10 -pady 5 \
	-side left
    tixDirList $w.d2; pack $w.d2 -fill both -padx 10 -pady 5 \
	-side left

    button $w.b -text "Close" -command "destroy $w"
    pack $w.b -side left -anchor c -expand yes

    $w.d1 subwidget hlist config -selectmode dragdrop
    $w.d2 subwidget hlist config -selectmode dragdrop
}

# This "if" statement makes it possible to run this script file inside or
# outside of the main demo program "widget".
#
if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> "exit"
}

