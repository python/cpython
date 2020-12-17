# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: combobox.tcl,v 1.2 2002/11/13 21:12:17 idiscovery Exp $
#
# combobox.tcl --
#
#	Tests the ComboBox widget.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Testing the ComboBox widget."
}

proc cbTest_Command {args} {
    global cbTest_selected

    set cbTest_selected [tixEvent value]
}

proc cbTest_ListCmd {w} {
    global counter

    incr counter

    $w subwidget listbox delete 0 end
    $w subwidget listbox insert end 0
    $w subwidget listbox insert end 1
    $w subwidget listbox insert end 2
}


proc Test {} {
    global cbTest_selected

    for {set dropdown 1} {$dropdown >= 0} {incr dropdown -1} {

	TestBlock combo-1.1 {Config -value} {
	    set w [tixComboBox .c -command cbTest_Command -dropdown $dropdown \
		-editable true]
	    pack $w
	    update
	    set val "Testing some value .."
	    $w config -value $val
	    Assert {[tixStrEq "$cbTest_selected" $val]}
	}

	TestBlock combo-1.2 {selection from listbox} {
	    $w subwidget listbox insert end "entry 0"
	    $w subwidget listbox insert end "entry 1"
	    $w subwidget listbox insert end "entry 2"

	    for {set x 0} {$x <= 2} {incr x} {
		Click [$w subwidget arrow] 
		update

		if $dropdown {
		    ClickListboxEntry [$w subwidget listbox] $x single
		} else {
		    ClickListboxEntry [$w subwidget listbox] $x single
		    ClickListboxEntry [$w subwidget listbox] $x double
		}
		update

		Assert {[tixStrEq "$cbTest_selected" "entry $x"]}
	    }
	}

	TestBlock combo-1.3 {invokation by keyboard} {
	    set val "Testing by key with \\ slashes"
	    KeyboardString [$w subwidget entry] $val
	    KeyboardEvent [$w subwidget entry] <Return>
	    update

	    Assert {[tixStrEq "$cbTest_selected" "$val"]}
	}

	catch {
	    destroy $w
	}
    }

    TestBlock combo-2.1 {-listcmd of ComboBox} {
	global counter
	set counter 0
	tixComboBox .c -listcmd "cbTest_ListCmd .c"
	pack .c -expand yes -fill both
	update

	Click [.c subwidget arrow]
	update
	Assert {$counter == 1}
	Click [.c subwidget arrow]
	update

	Click [.c subwidget arrow]
	update
	Click [.c subwidget arrow]
	update
	Assert {$counter == 2}


	Assert {[.c subwidget listbox get 0] == "0"}
	Assert {[.c subwidget listbox get 1] == "1"}
	Assert {[.c subwidget listbox get 2] == "2"}
    }
}
