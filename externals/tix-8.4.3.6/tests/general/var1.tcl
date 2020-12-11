# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: var1.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc About {} {
    return "Testing -variable option with Tix widgets"
}

proc Test {} {
    global foo bar arr

    set classes {tixControl tixComboBox}
    set value 1234

    foreach class $classes {
	set w [$class .foo]
	pack $w
	update idletasks

	TestBlock var1-1.1 {$class: config -variable with initialized value} {
	    set bar $value
	    $w config -variable bar
	    update idletasks
	    Assert {[$w cget -value] == $value}
	}

	TestBlock var1-1.2 {$class: config -variable w/ uninitialized value} {
	    destroy $w
	    set w [$class .foo]
	    $w config -variable bar
	    Assert {[$w cget -value] == $bar}
	}

	TestBlock var1-1.2 {$class: config -variable} {
	    set foo 111
	    $w config -variable foo
	    update idletasks
	    Assert {[$w cget -value] == $foo}
	}

	TestBlock var1-1.2 {$class: config -value} {
	    $w config -value 123
	    Assert {[$w cget -value] == 123}
	    Assert {[set [$w cget -variable]] == 123}
	}

	TestBlock var1-1.2 {$class: config -variable on array variable} {
	    set arr(12) 1234
	    $w config -variable arr(12)
	    Assert {[$w cget -value] == $arr(12)}
	}

	TestBlock var1-1.2 {$class: config -value on array variable} {
	    $w config -value 12
	    Assert {[$w cget -value] == 12}
	    Assert {[set [$w cget -variable]] == 12}
	}

	catch {
	    destroy $w
	}
    }
}
