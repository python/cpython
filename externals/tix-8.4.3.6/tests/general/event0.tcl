# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: event0.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc About {} {
    return "Testing the event emulation routines in the test suite"
}

proc TestEntry_Invoke {w} {
    global testEntry_Invoked testEntry_value1

    set testEntry_Invoked 1
    set testEntry_value1 [$w get]
}

proc Test {} {
    global foo
    set foo 0

    TestBlock event0-1.1 {Typing return in an entry widget} {
	global testEntry_Invoked testEntry_value0 testEntry_value1

	set testEntry_Invoked 0
	entry .e -textvariable testEntry_value0
	set testEntry_value0 "Entering some text ..."
	bind .e <Return> "TestEntry_Invoke .e"
	pack .e
	update

	KeyboardEvent .e <Return>
	update
	Assert {$testEntry_Invoked == 1}
	Assert {$testEntry_value0 == $testEntry_value1}
    }

    TestBlock event0-1.2 {Typing characters in an entry widget} {
	set testEntry_value0 ""
	set val "Typing the keyboard ..."

	focus .e
	.e delete 0 end
	update
	KeyboardString .e $val
	update
	Assert {[tixStrEq "$testEntry_value0" "$val"]}
    }

    TestBlock event0-1.3 {Typing characters and slashes in an entry widget} {
	set testEntry_value0 ""
	set val "Typing the \\ keyboard ..."

	focus .e
	.e delete 0 end
	KeyboardString .e $val
	update
	Assert {[tixStrEq "$testEntry_value0" "$val"]}

	destroy .e
    }

    TestBlock event0-1.4 {Testing ClickListboxEntry} {
	listbox .l -selectmode single
	.l insert end "index 0"
	.l insert end "index 1"
	.l insert end "index 2"

	pack .l; update

	for {set x 0} {$x <= 2} {incr x} {
	    ClickListboxEntry .l $x single
	    update
	    Assert {[.l index active] == $x}
	    Assert {[.l curselection] == $x}
	}

	destroy .l
	update
    }

    TestBlock event0-1.5 {Clicking a button} {
	button .b -command "set foo 1"
	pack .b; update

	Click .b
	Assert {$foo == 1}
    }

    TestBlock event0-1.6 {Drag and selecting a combobox} {
	tixComboBox .c
	.c insert end 10
	.c insert end 10
	.c insert end 10
	.c insert end 10
	.c insert end 10
	pack .c; update

	HoldDown [.c subwidget arrow]
	Drag [.c subwidget listbox] 10 10
	Release [.c subwidget listbox] 10 10
	Release [.c subwidget arrow] -30 30

	Assert {[.c cget -value] == "10"}
    }
}
