# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Balloon.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixBalloon widget, which provides
# a interesting way to give help tips about elements in your user interface.
# Your can display the help message in a "balloon" and a status bar widget.
#
proc RunSample {w} {

    # Create the status bar widget
    #
    label $w.status -width 40 -relief sunken -bd 1
    pack $w.status -side bottom -fill y -padx 2 -pady 1

    # These are two a mysterious widgets that need some explanation
    #
    button $w.button1 -text " Something Unexpected " \
	-command "destroy $w"
    button $w.button2 -text " Something Else Unexpected " \
	-command "destroy $w.button2"
    pack $w.button1 $w.button2 -side top -expand yes

    # Create the balloon widget and associate it with the widgets that we want
    # to provide tips for:
    tixBalloon $w.b -statusbar $w.status

    $w.b bind $w.button1 -balloonmsg "Close window" \
	-statusmsg "Press this button to close this window" 
    $w.b bind $w.button2 -balloonmsg "Self-destruct\nButton" \
	-statusmsg "Press this button and it will get rid of itself" 
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> {if {"%W" == ".demo"} exit}
}
