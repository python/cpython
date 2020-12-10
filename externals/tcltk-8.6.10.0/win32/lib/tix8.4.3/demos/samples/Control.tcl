# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Control.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixControl widget -- it is an
# entry widget with up/down arrow buttons. You can use the arrow buttons
# to adjust the value inside the entry widget.
#
# This example program uses three Control widgets. One lets you select
# integer values; one lets you select floating point values and the last
# one lets you select a few names.
#
proc RunSample {w} {

    # Create the tixControls on the top of the dialog box
    #
    frame $w.top -border 1 -relief raised

    # $w.top.a allows only integer values
    #
    # [Hint] The -options switch sets the options of the subwidgets.
    # [Hint] We set the label.width subwidget option of the Controls to 
    #        be 16 so that their labels appear to be aligned.
    #
    global demo_maker demo_thrust demo_num_engins
    set demo_maker	P&W
    set demo_thrust	20000.0
    set demo_num_engins 2


    tixControl $w.top.a -label "Number of Engines: " -integer true \
	-variable demo_num_engins -min 1 -max 4\
	-options {
	    entry.width 10
	    label.width 20
	    label.anchor e
	}

    tixControl $w.top.b -label "Thrust: " -integer false \
	-min 10000.0 -max 60000.0 -step 500\
	-variable demo_thrust \
	-options {
	    entry.width 10
	    label.width 20
	    label.anchor e
	}

    tixControl $w.top.c -label "Engin Maker: " \
	-incrcmd "ctl:adjust_maker $w.top.c +1" \
	-decrcmd "ctl:adjust_maker $w.top.c -1" \
	-validatecmd "ctl:validate_maker $w.top.c" \
	-value "P&W" \
	-options {
	    entry.width 10
	    label.width 20
	    label.anchor e
	}

    pack $w.top.a $w.top.b $w.top.c -side top -anchor w

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "ctl:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

set ctl_makers {GE P&W "Rolls Royce"}

# This procedure gets called when the user presses the up/down arrow buttons.
# We return the "previous" or "next" engin maker according to the "$by"
# argument
#
proc ctl:adjust_maker {w by value} {
    global ctl_makers

    set index [lsearch $ctl_makers $value]
    set len   [llength $ctl_makers]
    set index [expr $index $by]
	       
    if {$index < 0} {
	set index [expr $len -1]
    }
    if {$index >= $len} {
	set index 0
    }

    return [lindex $ctl_makers $index]
}

proc ctl:validate_maker {w value} {
    global ctl_makers

    if {[lsearch $ctl_makers $value] == -1} {
	return [lindex $ctl_makers 0]
    } else {
	return $value
    }
}

proc ctl:okcmd {w} {
    global demo_maker demo_thrust demo_num_engins

    tixDemo:Status "You selected $demo_num_engins engin(s) of thrust $demo_thrust made \
by $demo_maker"

    destroy $w
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
