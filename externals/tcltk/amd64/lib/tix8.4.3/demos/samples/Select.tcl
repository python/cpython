# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Select.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixSelect widget.
#
proc RunSample {w} {
    global demo_dir

    # Create the frame on the top of the dialog box with two tixSelect
    # widgets inside.
    #
    frame $w.top

    # There can be one and only type of justification for any piece of text.
    # So we set -radio to be true. Also, -allowzero is set to false: the user
    # cannot select a "none" justification
    #
    tixSelect $w.top.just -allowzero false -radio true \
	-label "Justification: "\
	-options {
	    label.width 15
	    label.padx 4
	    label.anchor e
	}

    # The user can select one or many or none of the font attributes in
    # the font Select widget, so we set -radio to false (can select one or
    # many) and -allowzero to true (can select none)
    #
    tixSelect $w.top.font -allowzero true  -radio false \
	-label "Font: " \
	-options {
	    label.width 15
	    label.padx 4
	    label.anchor e
	}

    pack $w.top.just $w.top.font -side top -expand yes -anchor c \
	-padx 4 -pady 4

    # Add the choices of available font attributes
    #
    #
    $w.top.font add bold      -bitmap @$demo_dir/bitmaps/bold.xbm
    $w.top.font add italic    -bitmap @$demo_dir/bitmaps/italic.xbm
    $w.top.font add underline -bitmap @$demo_dir/bitmaps/underlin.xbm
    $w.top.font add capital   -bitmap @$demo_dir/bitmaps/capital.xbm

    # Add the choices of available justification types
    #
    #
    $w.top.just add left      -bitmap @$demo_dir/bitmaps/leftj.xbm
    $w.top.just add right     -bitmap @$demo_dir/bitmaps/rightj.xbm
    $w.top.just add center    -bitmap @$demo_dir/bitmaps/centerj.xbm
    $w.top.just add justified -bitmap @$demo_dir/bitmaps/justify.xbm

    $w.top.font config -variable sel_font
    $w.top.just config -variable sel_just

    # Set the default value of the two Select widgets
    #
    #
    global sel_just sel_font
    set sel_just justified
    set sel_font {bold underline}

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -width 6\
	-command "sel:cmd $w; destroy $w"
	
    $w.box add apply  -text Apply  -underline 0 -width 6\
	-command "sel:cmd $w"

    $w.box add cancel -text Cancel -underline 0 -width 6\
	-command "destroy $w"

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

# This procedure is called whenever the user pressed the OK or the Apply button
#
#
proc sel:cmd {w} {
    global sel_font sel_just

    tixDemo:Status "The justification is $sel_just"

    if {$sel_font == {}} {
	tixDemo:Status "The font is normal"
    } else {
	tixDemo:Status "The font is $sel_font"
    }

}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind .demo <Destroy> exit
}
