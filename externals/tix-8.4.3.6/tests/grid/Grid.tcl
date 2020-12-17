# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Grid.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# This tests the Grid widget.
#
#
#
proc About {} {
    return "Basic tests for the Grid widget"
}

proc Test {} {
    TestBlock grid-1.1 {Grid creation} {
	test {tixGrid} {args}
	test {tixGrid .g -ff} {unknown}
	test {tixGrid .g -width} {missing}

	Assert {[info command .g] == {}}
	Assert {![winfo exists .g]}
    }

    TestBlock grid-1.2 {Grid creation} {
	set g [tixGrid .g]
	pack $g -expand yes -fill both
	update
	destroy $g
    }

    TestBlock grid-2.1 {Grid widget commands} {
	set g [tixGrid .g]
	pack $g -expand yes -fill both
	test {$g} {args}
	set foo ""
    }
    TestBlock grid-2.2 {Grid widget commands} {
	$g config -selectmode browse
	Assert {[tixStrEq [$g cget -selectmode] browse]}
    }

    #----------------------------------------
    # Sites
    #----------------------------------------
    foreach cmd {anchor dragsite dropsite} {
	TestBlock grid-3.1 "Grid \"$cmd\" widget command" {
	    test1 {$g $cmd} \
		"wrong # args: should be \".g $cmd option ?x y?\""
	}
	TestBlock grid-3.2 "Grid \"$cmd\" widget command" {
	    test1 {$g $cmd foo} \
		{wrong option "foo", must be clear, get or set}
	}
	TestBlock grid-3.3 "Grid \"$cmd\" widget command" {
	    test1 {$g $cmd clear bar} \
		"wrong # of arguments, must be: .g $cmd clear"
	}
	TestBlock grid-3.4 "Grid \"$cmd\" widget command" {
	    test1 {$g $cmd set 0 0 bar} \
		"wrong # args: should be \".g $cmd option ?x y?\""
	}
	TestBlock grid-3.5 "Grid \"$cmd\" widget command" {
	    test1 {$g $cmd set xxx 0} \
		{expected integer but got "xxx"}
	}
	TestBlock grid-3.6 "Grid \"$cmd\" widget command" {
	    test1 {$g $cmd set 0 xxx} \
		{expected integer but got "xxx"}
	}
	foreach selunit {row column cell} {
	    TestBlock grid-3.7 "Grid \"$cmd\" widget command" {
		$g config -selectunit $selunit
		$g $cmd set 0 0
		update
	    }
        }
	TestBlock grid-3.8 "Grid \"$cmd\" widget command" {
	    $g $cmd set 0 0
	    Assert {[tixStrEq [$g $cmd get] "0 0"]}
	}
	TestBlock grid-3.9 "Grid \"$cmd\" widget command" {
	    $g $cmd set -20 -0
	    Assert {[tixStrEq [$g $cmd get] "0 0"]}
	}
	TestBlock grid-3.10 "Grid \"$cmd\" widget command" {
	    $g $cmd set 10000000 100000000
	    Assert {[tixStrEq [$g $cmd get] "10000000 100000000"]}
	}
    }

    #----------------------------------------
    # set
    #----------------------------------------
    TestBlock grid-4.1 {Grid "set" widget command} {
	test {$g set} {args}
	test {$g set 0 0 -foo} {missing}
	test {$g set 0 0 -foo bar} {unknown}
	test {$g set 0 0 -itemtype foo} {unknown}
	test {$g set 0 0 -itemtype imagetext -image foo} {image}
	test {$g set 0 0 -itemtype imagetext -text Hello -image \
	    [tix getimage folder]
	}
	update
    } 

    TestBlock grid-4.2 {Grid "set" widget command} {
	for {set x 0} {$x < 19} {incr x} {
	    for {set y 0} {$y < 13} {incr y} {
		$g set $x $y -itemtype imagetext -text ($x,$y) \
		    -image [tix getimage folder]
	    }
	}
	update
    }

    TestBlock grid-4.3 {Grid "unset" widget command} {
	for {set x 0} {$x < 23} {incr x} {
	    for {set y 0} {$y < 19} {incr y} {
		$g unset $x $y
	    }
	}
	update
    }


    #----------------------------------------
    # delete
    #----------------------------------------
    TestBlock grid-5.1 {Grid "delete" widget command} {
	for {set x 0} {$x < 19} {incr x} {
	    for {set y 0} {$y < 13} {incr y} {
		$g set $x $y -itemtype imagetext -text ($x,$y) \
		    -image [tix getimage folder]
	    }
	}
	foreach index {0 1 3 2 6 3 1 1 max 19 13 max} {
	    $g delete row $index
	    $g delete col $index
	    update
	}
    } 
    #----------------------------------------
    # move
    #----------------------------------------
    TestBlock grid-6.1 {Grid "move" widget command} {
	for {set x 0} {$x < 19} {incr x} {
	    for {set y 0} {$y < 13} {incr y} {
		$g set $x $y -itemtype imagetext -text ($x,$y) \
		    -image [tix getimage folder]
	    }
	}
	foreach index {0 1 3 2 6 3 1 1 max 19 13 max} {
	    $g move row $index $index 3
	    $g move col $index $index -2
	    update
	}
    } 

}

