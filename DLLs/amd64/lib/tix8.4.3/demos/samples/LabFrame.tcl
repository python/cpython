# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: LabFrame.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixLabelFrame widget -- a frame that
# come with a label at its side. It looks nifty when you use the set the
# -labelside option to "acrosstop". Note that a lot of Tix widgets, such
# as tixComboBox or tixControl, have the -labelside and -label options. So
# you can use these options to achieve the same effect as in this file
#

proc RunSample {w} {

    # Create the radiobuttons at the top of the dialog box, put them
    # inside two tixLabelFrames:
    #
    frame $w.top -border 1 -relief raised

    tixLabelFrame $w.top.a -label Font: -labelside acrosstop -options {
	label.padX 5
    }
    tixLabelFrame $w.top.b -label Size: -labelside acrosstop -options {
	label.padX 5
    }

    pack $w.top.a $w.top.b  -side left -expand yes -fill both

    # Create the radiobuttons inside the left frame.
    #
    # [Hint] You *must* create the new widgets inside the "frame"
    #	     subwidget, *not* as immediate children of $w.top.a!
    #
    set f [$w.top.a subwidget frame]
    foreach color {Red Green Blue Yellow Orange Purple} {
	set lower [string tolower $color]
	radiobutton $f.$lower -text $color -variable demo_color \
	    -relief flat -value $lower -bd 2 -pady 0 -width 7 -anchor w
	pack $f.$lower -side top -pady 0 -anchor w -padx 6
    }

    # Create the radiobuttons inside the right frame.
    #
    set f [$w.top.b subwidget frame]
    foreach point {8 10 12 14 18 24} {
	set lower [string tolower $point]
	radiobutton $f.$lower -text $point -variable demo_point \
	    -relief flat -value $lower -bd 2 -pady 0 -width 4 -anchor w
	pack $f.$lower -side top -pady 0 -anchor w -padx 8
    }

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "labf:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

proc labf:okcmd {w} {
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
