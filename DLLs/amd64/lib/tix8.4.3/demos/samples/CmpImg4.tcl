# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: CmpImg4.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates how to use the compound image to add
# colors in Notebook tabs.
#
proc RunSample {w} {

    # Create the notebook widget and set its backpagecolor to gray.
    # Note that the -backpagecolor option belongs to the "nbframe"
    # subwidget.
    tixNoteBook $w.nb -ipadx 6 -ipady 6
    # $w config -bg gray
    # $w.nb subwidget nbframe config -backpagecolor gray -tabpady 0

    # Create the two compound images --
    #
    # Create the first image:
    #
    # Notice that the -window option must be set to the nbframe
    # subwidget of the notebook because the image will be displayed
    # in that widget.
    #
    set hdd_img [image create compound -window [$w.nb subwidget nbframe] \
	-pady 4 -padx 4 -bg #f09090 -showbackground 1]
    $hdd_img add line
    $hdd_img add text -text "Hard Disk" -underline 0 -padx 6 -pady 4
 
    # Create the second compound image. Very similar to what we did above
    #
    set net_img [image create compound -window [$w.nb subwidget nbframe] \
	-pady 4 -pady 4 -bg #9090f0 -showbackground 1]
    $net_img add line
    $net_img add text -text "Network" -underline 0 -padx 6 -pady 4

    #
    # Now create the pages
    #

    # We use these options to set the sizes of the subwidgets inside the
    # notebook, so that they are well-aligned on the screen.
    #
    set name [tixOptionName $w]
    option add *$name*TixControl*entry.width 10
    option add *$name*TixControl*label.width 18
    option add *$name*TixControl*label.anchor e

    # Create the two tabs on the notebook. The -underline option
    # puts a underline on the first character of the labels of the tabs.
    # Keyboard accelerators will be defined automatically according
    # to the underlined character.	
    #
    $w.nb add hard_disk -image $hdd_img
    $w.nb add network   -image $net_img
    pack $w.nb -expand yes -fill both -padx 5 -pady 5 -side top

    #----------------------------------------
    # Create the first page
    #----------------------------------------
    set f [$w.nb subwidget hard_disk]

    # Create two frames: one for the common buttons, one for the
    # other widgets
    #
    frame $f.f
    frame $f.common
    pack $f.f      -side left  -padx 2 -pady 2 -fill both -expand yes
    pack $f.common -side right -padx 2 -pady 2 -fill y

    # Create the controls that only belong to this page
    #
    tixControl $f.f.a -value 12   -label "Access Time: "
    tixControl $f.f.w -value 400  -label "Write Throughput: "
    tixControl $f.f.r -value 400  -label "Read Throughput: "
    tixControl $f.f.c -value 1021 -label "Capacity: "
    pack $f.f.a $f.f.w $f.f.r $f.f.c  -side top -padx 20 -pady 2

    # Create the common buttons
    #
    CreateCommonButtons $w $f.common
    
    #----------------------------------------
    # Create the second page	
    #----------------------------------------
    set f [$w.nb subwidget network]

    frame $f.f
    frame $f.common
    pack $f.f      -side left  -padx 2 -pady 2 -fill both -expand yes
    pack $f.common -side right -padx 2 -pady 2 -fill y

    tixControl $f.f.a -value 12   -label "Access Time: "
    tixControl $f.f.w -value 400  -label "Write Throughput: "
    tixControl $f.f.r -value 400  -label "Read Throughput: "
    tixControl $f.f.c -value 1021 -label "Capacity: "
    tixControl $f.f.u -value 10   -label "Users: "

    pack $f.f.a $f.f.w $f.f.r $f.f.c $f.f.u -side top -padx 20 -pady 2

    CreateCommonButtons $w $f.common
}

proc CreateCommonButtons {w f} {
    button $f.ok     -text OK     -width 6 -command "destroy $w"
    button $f.cancel -text Cancel -width 6 -command "destroy $w"

    pack $f.ok $f.cancel -side top -padx 2 -pady 2
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
