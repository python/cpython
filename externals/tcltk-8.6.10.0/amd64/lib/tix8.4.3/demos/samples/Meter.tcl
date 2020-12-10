# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Meter.tcl,v 1.4 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This program demonstrates the use of the tixMeter widget -- it is
# used to display the progress of a background job
#

proc RunSample {w} {
    set top [frame $w.f -bd 1 -relief raised]
    set box [tixButtonBox $w.b -bd 1 -relief raised]

    pack $box -side bottom -fill both
    pack $top -side top -fill both -expand yes

    # Create the Meter and the Label
    #
    label $top.lab -text "Work in progress ...."
    tixMeter $top.met -value 0 -text 0%

    pack $top.lab -side top -padx 50 -pady 10 -anchor c
    pack $top.met -side top -padx 50 -pady 10 -anchor c


    # Create the buttons
    #
    $box add cancel  -text Cancel  -command "destroy $w" \
	-width 6 -under 0
    $box add restart -text Restart -width 6 -under 0

    $box subwidget restart config -command \
	"Meter:Start $top.met [$box subwidget cancel] [$box subwidget restart]"

    $box subwidget restart invoke
}

proc Meter:Start {meter cancel restart} {
    $restart config -state disabled
    $cancel config -text Cancel
    after 40 Meter:BackgroundJob $meter 0 $cancel $restart
}

proc Meter:BackgroundJob {meter progress cancel restart} {
    if ![winfo exists $meter] {
	# the window has already been destroyed	
	#
	return
    }

    set progress [expr $progress + 0.02]
    set text [expr int($progress*100.0)]%

    $meter config -value $progress -text $text

    if {$progress < 1.0} {
	after 40 Meter:BackgroundJob $meter $progress $cancel $restart
    } else {
	$cancel config -text OK -under 0
	$restart config -state normal
    }
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> {if {"%W" == ".demo"} exit}
}
