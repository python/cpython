# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: HLInd.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc Test {} {
    set h [tixHList .h -indicator 1 -indent 20]
    pack $h -expand yes -fill both
    button .b -text close -command "Done forced"
    pack .b

    $h add hello -text hello
    $h add noind -text hello

    test {$h indicator} {args}
    test {$h indicator bad} {unknown}

    # Test for create
    #
    #

    test {$h indicator create} {args}
    test {$h indicator create xyz} {{not found}}
    test {$h indicator create hello -itemtype} {missing}
    test {$h indicator create hello -itemtype bad} {unknown}
    test {$h indicator create hello -itemtype imagetext \
	-image [tix getimage plus]}

    # Test for cget
    #
    test {$h indicator cget} {args}
    test {$h indicator cget hello} {args}
    test {$h indicator cget hello arg arg} {args}
    test {$h indicator cget noind -text} {{does not have}}
    test {$h indicator cget hello -bad} {{unknown}}
    test {$h indicator cget hello -image}

    # Test for size
    #
    test {$h indicator size} {args}
    test {$h indicator size hello hi} {args}
    test {$h indicator size bad} {{not found}}
    test {$h indicator size noind} {{does not have}}
    test {set x [$h indicator size hello]}
    test {$h indicator cget hello -image} {{does not}}

    # Test for delete
    #
    test {$h indicator delete} {args}
    test {$h indicator delete hello hi} {args}
    test {$h indicator delete bad} {{not found}}
    test {$h indicator delete hello}
    test {$h indicator cget hello -image} {{does not}}

    update
}
