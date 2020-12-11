# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: HList.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# HList.tcl --
#
#	General HList test.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "General tests for the HList widget"
}


proc Test {} {
    set h [tixHList .h -selectmode single]
    pack $h -expand yes -fill both

    #
    PutP "Testing the selection command"
    #

    for {set x 0} {$x < 40} {incr x} {
	$h add foo$x -text Foo$x
    }
    update

    test {$h selection set} {arg}
    test {$h selection set foo1}

    test {$h selection get foo} {arg}
    Assert {[tixStrEq [$h selection get] "foo1"]}
    Assert {[tixStrEq [$h selection get] [$h info selection]]}

    #
    PutP "Testing the info bbox command"
    #
    $h config -browsecmd "HLTest_BrowseCmd $h"
    global hlTest_selected
    for {set x 0} {$x <= 3} {incr x} {
	set ent foo[expr $x * 8]
	$h see $ent
	update

	set bbox [$h info bbox $ent]
	Assert {![tixStrEq "$bbox" ""]}

	set hlTest_selected ""
	Click $h [lindex $bbox 0] [lindex $bbox 1]
	update
	Assert {[tixStrEq "$hlTest_selected" "$ent"]}

	set hlTest_selected ""
	Click $h [lindex $bbox 2] [lindex $bbox 3]
	update
	Assert {[tixStrEq "$hlTest_selected" "$ent"]}
    }

    #
    PutP "Testing the ClickHListEntry test function"
    #
    for {set x 0} {$x <= 3} {incr x} {
	set hlTest_selected ""
	set ent foo[expr $x * 8]
	ClickHListEntry $h $ent
	update
	Assert {[tixStrEq "$hlTest_selected" "$ent"]}
    }
}

proc HLTest_BrowseCmd {w args} {
    global hlTest_selected

    set hlTest_selected [tixEvent value]
}
