#
#	$Id: TList.tcl,v 1.1.1.1 2000/05/17 11:08:53 idiscovery Exp $
#
# This tests the TList widget.
#
#
# Assumptions:
#	None
#
proc About {} {
    return "Basic tests for the TList widget"
}

proc Test {} {

    #
    # Test the creation
    #
    test {tixTList} {args}
    test {tixTList .t -ff} {unknown}
    test {tixTList .t -width} {missing}

    if {[info command .t] != {}} {
	error "widget not destroyed when creation failed"
    }

    set t [tixTList .t]
    test {$t} {args}

    #
    # Test the "insert" command
    #
    test {$t insert} {args}
    test {$t insert 0 -foo} {missing}
    test {$t insert 0 -foo bar} {unknown}
    test {$t insert 0 -itemtype foo} {unknown}
    test {$t insert 0 -itemtype text -image foo} {unknown}
    test {$t insert 0 -itemtype text -text Hello} 

    pack $t
}
