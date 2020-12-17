# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SGrid1.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# Demonstrates the tixGrid widget
#

proc RunSample {w} {
    wm title $w "Doe Inc. Performance"
    wm geometry $w 640x300

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
# be reformatted. The x1, y1, x2, y2 sprcifies the four corners of the area
# that needs to be reformatted.
#
proc gformat {w area x1 y1 x2 y2} {
    set bg(s-margin) gray65
    set bg(x-margin) gray65
    set bg(y-margin) gray65
    set bg(main)     gray20

    case $area {
	main {
	    for {set y [expr ($y1/2) * 2]} {$y <= $y2} {incr y 2} {
		$w format border $x1 $y $x2 $y \
		    -relief flat -filled 1\
		    -bd 0 -bg #80b080 -selectbackground #80b0ff
	    }
	    $w format grid $x1 $y1 $x2 $y2 \
		-relief raised -bd 1 -bordercolor $bg($area) -filled 0 -bg red\
		-xon 1 -yon 1 -xoff 0 -yoff 0 -anchor se
	}
	y-margin {
	    $w format border $x1 $y1 $x2 $y2 \
		-fill 1 -relief raised -bd 1 -bg $bg($area) \
		-selectbackground gray80
	}

	default {
	    $w format border $x1 $y1 $x2 $y2 \
		-filled 1 \
		-relief raised -bd 1 -bg $bg($area) \
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

    # data format {year revenue profit}
    #
    set data {
	{1970	1000000000	1000000}
	{1971	1100000000	2000000}
	{1972	1200000000	3000000}
	{1973	1300000000	4000000}
	{1974	1400000000	5000000}
	{1975	1500000000	6000000}
	{1976	1600000000	7000000}
	{1977	1700000000	8000000}
	{1978	1800000000	9000000}
	{1979	1900000000     10000000}
	{1980	2000000000     11000000}
	{1981	2100000000     22000000}
	{1982	2200000000     33000000}
	{1983	2300000000     44000000}
	{1984	2400000000     55000000}
	{1985	3500000000     36000000}
	{1986	4600000000     57000000}
	{1987	5700000000     68000000}
	{1988	6800000000     79000000}
	{1989	7900000000     90000000}
	{1990  13000000000    111000000}
	{1991  14100000000    122000000}
	{1992  16200000000    233000000}
	{1993  28300000000    344000000}
	{1994  29400000000    455000000}
	{1995  38500000000    536000000}
    }

    set headers {
	"Revenue ($)"
	"Rev. Growth (%)"
	"Profit ($)"
	"Profit Growth (%)"
    }

    # Create the grid
    #
    tixScrolledGrid $w.g -bd 0
    pack $w.g -expand yes -fill both -padx 3 -pady 3

    set grid [$w.g subwidget grid]
    $grid config -formatcmd "gformat $grid"

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

    set margin [tixDisplayStyle text -refwindow $grid   \
	-anchor c -padx 3 -font [tix option get bold_font]]
    set dollar [tixDisplayStyle text  -refwindow $grid  \
	-anchor e]

    # Create the headers
    #
    set x 1
    foreach h $headers {
	$grid set $x 0 -itemtype text -text $h -style $margin
	incr x
    }

    # Insert the data, year by year
    #
    set lastRevn {}
    set lastProf {}
    set i         1
    foreach line $data {
	set year [lindex $line 0]
	set revn [lindex $line 1]
	set prof [lindex $line 2]

	if {$lastRevn != {}} {
	    set rgrowth \
		[format %4.2f [expr ($revn.0-$lastRevn)/$lastRevn*100.0]]
	} else {
	    set rgrowth "-"
	}
	if {$lastProf != {}} {
	    set pgrowth \
		[format %4.2f [expr ($prof.0-$lastProf)/$lastProf*100.0]]
	} else {
	    set pgrowth "-"
	}
	
	$grid set 0 $i -itemtype text -style $margin -text $year
	$grid set 1 $i -itemtype text -style $dollar -text [Dollar $revn]
	$grid set 2 $i -itemtype text -style $dollar -text $rgrowth
	$grid set 3 $i -itemtype text -style $dollar -text [Dollar $prof]
	$grid set 4 $i -itemtype text -style $dollar -text $pgrowth

	set lastRevn $revn.0
	set lastProf $prof.0

	incr i
    }
}


if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
