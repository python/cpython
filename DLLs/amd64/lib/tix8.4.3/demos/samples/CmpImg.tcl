# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: CmpImg.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the compound images: it uses compound
# images to display a text string together with a pixmap inside
# buttons
#
proc RunSample {w} {

    set img0 [tix getimage network]
    set img1 [tix getimage harddisk]

    button $w.hdd -padx 4 -pady 1 -width 120
    button $w.net -padx 4 -pady 1 -width 120

    # Create the first image: we create a line, then put a string,
    # a space and a image into this line, from left to right.
    # The result: we have a one-line image that consists of three
    # individual items
    #
    set hdd_img [image create compound -window $w.hdd]
    $hdd_img add line
    $hdd_img add text -text "Hard Disk" -underline 0
    $hdd_img add space -width 7
    $hdd_img add image -image $img1
 
    # Put this image into the first button
    #
    $w.hdd config -image $hdd_img

    # Create the second compound image. Very similar to what we did above
    #
    set net_img [image create compound -window $w.net]
    $net_img add line
    $net_img add text -text "Network" -underline 0
    $net_img add space -width 7
    $net_img add image -image $img0

    $w.net config -image $net_img

    # The button to close the window
    #

    button $w.clo -pady 1 -text Close -command "destroy $w"

    pack $w.hdd $w.net $w.clo -side left -padx 10 -pady 10 -fill y -expand yes
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
