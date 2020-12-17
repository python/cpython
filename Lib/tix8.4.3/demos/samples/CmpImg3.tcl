# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: CmpImg3.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# Demonstrates how to use compound images to display icons in a canvas widget.
#

proc RunSample {w} {
    set top [frame $w.f -bd 1 -relief raised]
    set box [tixButtonBox $w.b -bd 1 -relief raised]

    pack $box -side bottom -fill both
    pack $top -side top -fill both -expand yes

    label $top.lab -text "Drag the icons"
    pack $top.lab -anchor c -side top -pady 4

    # Create the canvas to display the icons
    #
    set c [canvas $top.c -relief sunken -bd 1]
    pack $c -side top -expand yes -fill both -padx 4 -pady 4

    # create several compound images in the canvas
    #
    set network  [tix getimage network]
    set harddisk [tix getimage harddisk]

    set cmp_1 [image create compound -window $c -bd 1]
    $cmp_1 add image -image $network
    $cmp_1 add line
    $cmp_1 add text   -text " Network "

    set cmp_2 [image create compound -window $c -bd 1]
    $cmp_2 add image -image $harddisk
    $cmp_2 add line
    $cmp_2 add text   -text " Hard disk "

    set cmp_3 [image create compound -window $c -bd 1 \
	-background #c0c0ff -relief raised \
	-showbackground 1]
    $cmp_3 add image -image $network
    $cmp_3 add line
    $cmp_3 add text   -text  " Network 2 "

    $c create image  50  50  -image $cmp_1
    $c create image 150  50  -image $cmp_2
    $c create image 250  50  -image $cmp_3

    bind $c <1>         "itemStartDrag $c %x %y"
    bind $c <B1-Motion> "itemDrag $c %x %y"

    # Create the buttons
    #
    $box add ok     -text Ok     -command "destroy $w" -width 6
    $box add cancel -text Cancel -command "destroy $w" -width 6
}


proc itemStartDrag {c x y} {
    global lastX lastY
    $c raise current

    set lastX [$c canvasx $x]
    set lastY [$c canvasy $y]
}

proc itemDrag {c x y} {
    global lastX lastY
    set x [$c canvasx $x]
    set y [$c canvasy $y]
    $c move current [expr $x-$lastX] [expr $y-$lastY]
    set lastX $x
    set lastY $y
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
