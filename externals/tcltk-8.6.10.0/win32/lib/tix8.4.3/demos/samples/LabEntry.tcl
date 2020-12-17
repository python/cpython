# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: LabEntry.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixLabelEntry widget -- an entry that
# come with a label at its side, so you don't need to create
# extra frames on your own and do the messy hierarchical packing. This
# example is adapted from the tixControl example, except now you don't
# have arrow buttons to adjust the values for you ...
#

proc RunSample {w} {

    # Create the tixLabelEntrys on the top of the dialog box
    #
    frame $w.top -border 1 -relief raised

    # $w.top.a allows only integer values
    #
    # [Hint] The -options switch sets the options of the subwidgets.
    # [Hint] We set the label.width subwidget option of the Controls to 
    #        be 16 so that their labels appear to be aligned.
    #
    global lent_demo_maker lent_demo_thrust lent_demo_num_engins
    set lent_demo_maker	P&W
    set lent_demo_thrust	20000.0
    set lent_demo_num_engins 2

    tixLabelEntry $w.top.a -label "Number of Engines: " \
	-options {
	    entry.width 10
	    label.width 20
	    label.anchor e
	    entry.textVariable lent_demo_num_engins
	}

    tixLabelEntry $w.top.b -label "Thrust: "\
	-options {
	    entry.width 10
	    label.width 20
	    label.anchor e
	    entry.textVariable lent_demo_thrust
	}

    tixLabelEntry $w.top.c -label "Engin Maker: " \
	-options {
	    entry.width 10
	    label.width 20
	    label.anchor e
	    entry.textVariable lent_demo_maker
	}

    pack $w.top.a $w.top.b $w.top.c -side top -anchor w

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "labe:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

proc labe:okcmd {w} {
    global lent_demo_maker lent_demo_thrust lent_demo_num_engins

    tixDemo:Status "You selected $lent_demo_num_engins engin(s) of thrust $lent_demo_thrust made \
by $lent_demo_maker"

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
