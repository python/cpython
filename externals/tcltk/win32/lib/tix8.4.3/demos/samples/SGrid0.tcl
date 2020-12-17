# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SGrid0.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# A very simple demonstration of the tixGrid widget
#

proc RunSample {w} {
    wm title $w "The First Grid Example"
    wm geometry $w 480x300

    set top [frame $w.f -bd 1 -relief raised]
    set box [tixButtonBox $w.b -bd 1 -relief raised]

    pack $box -side bottom -fill both
    pack $top -side top -fill both -expand yes

    label $top.lab -text "This widget is still under alpha
Please ignore the debug messages
Not all features have been implemented" -justify left
    pack $top.lab -side top -anchor c -padx 3 -pady 3

    MakeGrid $top

    # Create the buttons
    #
    $box add ok     -text Ok     -command "destroy $w" -width 6
    $box add cancel -text Cancel -command "destroy $w" -width 6
}

# This command is called whenever the background of the grid needs to
# be reformatted. The x1, y1, x2, y2 specifies the four corners of the area
# that needs to be reformatted.
#
# area:
#  x-margin:	the horizontal margin
#  y-margin:	the vertical margin
#  s-margin:	the overlap area of the x- and y-margins
#  main:	The rest
#
proc SimpleFormat {w area x1 y1 x2 y2} {

    global margin
    set bg(s-margin) gray65
    set bg(x-margin) gray65
    set bg(y-margin) gray65
    set bg(main)     gray20

    case $area {
	main {
	    # The "grid" format is consecutive boxes without 3d borders
	    #
	    $w format grid $x1 $y1 $x2 $y2 \
		-relief raised -bd 1 -bordercolor $bg($area) -filled 0 -bg red\
		-xon 1 -yon 1 -xoff 0 -yoff 0 -anchor se
	}
	{x-margin y-margin s-margin} {
	    # border specifies consecutive 3d borders
	    #
	    $w format border $x1 $y1 $x2 $y2 \
		-fill 1 -relief raised -bd 1 -bg $bg($area) \
		-selectbackground gray80
	}
    }
}

# Print a number in $ format
#
#
proc Dollar {s} {
    set n [string len $s]
    set start [expr $n % 3]
    if {$start == 0} {
	set start 3
    }

    set str ""
    for {set i 0} {$i < $n} {incr i} {
	if {$start == 0} {
	    append str ","
	    set start 3
	}
	incr start -1
	append str [string index $s $i]
    }
    return $str
}

proc MakeGrid {w} {
    # Create the grid
    #
    tixScrolledGrid $w.g -bd 0
    pack $w.g -expand yes -fill both -padx 3 -pady 3

    set grid [$w.g subwidget grid]
    $grid config -formatcmd "SimpleFormat $grid"


    # Set the size of the columns
    #
    $grid size col 0 -size 10char
    $grid size col 1 -size auto
    $grid size col 2 -size auto
    $grid size col 3 -size auto
    $grid size col 4 -size auto

    # set the default size of the column and rows. these sizes will be used
    # if the size of a row or column has not be set via the "size col ?"
    # command
    $grid size col default -size 5char
    $grid size row default -size 1.1char -pad0 3

    for {set x 0} {$x < 10} {incr x} {
	for {set y 0} {$y < 10} {incr y} {
	    $grid set $x $y -itemtype text -text ($x,$y)
	}
    }
}


if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
