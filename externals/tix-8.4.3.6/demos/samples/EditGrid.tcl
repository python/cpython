# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: EditGrid.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# Demonstrates the use of editable entries in a Grid widget.
#

proc RunSample {w} {
    global editgrid

    wm title $w "Doe Inc. Performance"
    wm geometry $w 640x300

    label $w.lab -justify left -text \
"The left column is calculated automatically. To calculate the right column,
press the \"Calculate\" button"
    pack $w.lab -side top -anchor c -padx 3 -pady 3

    # Create the buttons
    #
    set f [frame $w.f -relief flat]
    pack $f -side right -fill y
    set add   [button $f.add   -text "Add Row"   -width 9 \
	-command "EditGrid_addRow"]
    set edit  [button $f.edit  -text "Edit"      -width 9 \
	-command "EditGrid_edit"]
    set cal   [button $f.cal   -text "Calculate" -width 9 \
	-command "EditGrid_calculate"]
    set close [button $f.close -text "Close"     -width 9 \
	-command "destroy $w"]
    pack $add   -side top    -padx 10
    pack $edit  -side top    -padx 10
    pack $cal   -side top    -padx 10 -pady 2
    pack $close -side bottom -padx 10

    # Create the grid and set options to make it editable.
    #
    tixScrolledGrid $w.g -bd 0
    pack $w.g -expand yes -fill both -padx 3 -pady 3

    set grid [$w.g subwidget grid]
    $grid config \
	-formatcmd "EditGrid_format $grid" \
	-editnotifycmd "EditGrid_editNotify" \
	-editdonecmd "EditGrid_editDone" \
	-selectunit cell \
	-selectmode single

    # Insert some initial data
    #
    $grid set 0 1 -text "City #1"
    $grid set 0 2 -text "City #2"
    $grid set 0 3 -text "City #3"
    $grid set 0 5 -text "Combined"

    $grid set 2 0 -text "Population"
    $grid set 4 0 -text "Avg. Income"

    $grid set 2 1 -text 125
    $grid set 2 2 -text  81
    $grid set 2 3 -text 724

    $grid set 4 1 -text 24432.12
    $grid set 4 2 -text 18290.24
    $grid set 4 3 -text 18906.34

    # Global data used by other EditGrid_ procedures.
    #
    set editgrid(g)   $grid
    set editgrid(top) 1
    set editgrid(bot) 3
    set editgrid(result) 5

    EditGrid_calPop
    EditGrid_calIncome
}

# EditGrid_edit --
#
#	Prompts the user to edit a cell.
#
proc EditGrid_edit {} {
    global editgrid
    set grid $editgrid(g)

    set ent [$grid anchor get]
    if [string comp $ent ""] {
	$grid edit set [lindex $ent 0]  [lindex $ent 1]
    }
}

# EditGrid_addRow --
#
#	Adds a new row to the table.
#
proc EditGrid_addRow {} {
    global editgrid
    set grid $editgrid(g)

    $grid edit apply

    $grid move row $editgrid(result) $editgrid(result) 1

    incr editgrid(bot)
    set editgrid(result) [expr $editgrid(bot) + 2]
    $grid set 0 $editgrid(bot) -text "City #$editgrid(bot)"
    $grid set 2 $editgrid(bot) -text 0
    $grid set 4 $editgrid(bot) -text 0.0

    EditGrid_calPop
    EditGrid_calIncome
}

# EditGrid_calPop --
#
#	Calculates the total population
#
proc EditGrid_calPop {} {
    global editgrid
    set grid $editgrid(g)

    set pop 0

    for {set i $editgrid(top)} {$i <= $editgrid(bot)} {incr i} {
	incr pop [$grid entrycget 2 $i -text]
    }

    $grid set 2 $editgrid(result) -text $pop
}

# EditGrid_calIncome --
#
#	Calculates the average income.
#
proc EditGrid_calIncome {} {
    global editgrid
    set grid $editgrid(g)

    set income 0
    set total_pop 0
    for {set i $editgrid(top)} {$i <= $editgrid(bot)} {incr i} {
	set pop [$grid entrycget 2 $i -text]
	set inc [$grid entrycget 4 $i -text]
	set income [expr $income + $pop.0 * $inc]
	incr total_pop $pop
    }

    $grid set 4 $editgrid(result) -text [expr $income/$total_pop]

}

# EditGrid_calculate --
#
#	Recalculates both columns.
#
proc EditGrid_calculate {} {
    global editgrid
    set grid $editgrid(g)

    $grid edit apply
    EditGrid_calIncome
}

# EditGrid_editNotify --
#
#	Returns true if an entry can be edited.
#
proc EditGrid_editNotify {x y} {
    global editgrid
    set grid $editgrid(g)

    if {$x == 2 || $x == 4} {
	if {$y >= $editgrid(top) && $y <= $editgrid(bot)} {
	    set editgrid(oldValue) [$grid entrycget $x $y -text]
	    return 1
	}
    }	
    return 0
}

# EditGrid_editDone --
#
#	Gets called when the user is done editing an entry.
#
proc EditGrid_editDone {x y} {
    global editgrid
    set grid $editgrid(g)

    if {$x == 2} {
	set pop [$grid entrycget $x $y -text]
	if [catch {
	    format %d $pop
	}] {
	    $grid entryconfig $x $y -text $editgrid(oldValue)
	    tk_dialog .editGridWarn "" \
		"$pop is not an valid integer. Try again" \
		warning 0 Ok
        } else {
	    $grid entryconfig 4 $editgrid(result) -text "-"
	    EditGrid_calPop
	}
    } else {
	set income [$grid entrycget $x $y -text]
	if [catch {
	    format %f $income
	}] {
	    $grid entryconfig $x $y -text $editgrid(oldValue)
	    tk_dialog .editGridWarn "" \
		"$income is not an valid floating number. Try again" \
		warning 0 Ok
        } else {
	    $grid entryconfig 4 $editgrid(result) -text "-"
	}
    }
}

# EditGrid_format --
#
#	This command is called whenever the background of the grid
#	needs to be reformatted. The x1, y1, x2, y2 sprcifies the four
#	corners of the area that needs to be reformatted.
#
proc EditGrid_format {w area x1 y1 x2 y2} {
    global editgrid

    set bg(s-margin) gray65
    set bg(x-margin) gray65
    set bg(y-margin) gray65
    set bg(main)     gray20

    case $area {
	main {
	    foreach col {2 4} {
		$w format border $col 1 $col $editgrid(bot) \
		    -relief flat -filled 1 -yon 1 -yoff 1\
		    -bd 0 -bg #b0b0f0 -selectbackground #a0b0ff
		$w format border $col 2 $col $editgrid(bot) \
		    -relief flat -filled 1 -yon 1 -yoff 1\
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

#    case $area {
#	{main y-margin} {
#	    set y [expr $editgrid(bot) + 1]
#	    $w format border 0 $y 100 $y -bg black -filled 1 -bd 0
#	}
#   }
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
