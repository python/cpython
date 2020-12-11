# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: mwm.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# mwm.tcl --
#
#	Test tixMwm command.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Testing tixMwm command"
}

proc Test {} {
    if ![string compare [info command tixMwm] ""] {
	puts "(OK) The tixMwm command is not available."
	return
    }
    if ![tixMwm ismwmrunning .] {
	puts "(OK) Mwm is not running on this display."
	return
    }
    
    toplevel .d
    toplevel .e

    test {tixMwm protocol .d add MY_PRINT_HELLO {"Print Hello"  _H Ctrl<Key>H}}
    wm protocol .d MY_PRINT_HELLO {puts Hello}

    test {tixMwm protocol .e add MY_PRINT_HELLO {"Print Hello"  _H Ctrl<Key>H}}
    wm protocol .e MY_PRINT_HELLO {puts Hello}

    test {destroy .d}

    test {tixMwm protocol .e add MY_PRINT_HELLO {"Print Hello"  _H Ctrl<Key>H}}
    wm protocol .e MY_PRINT_HELLO {puts Hello}

    test {tixMwm protocol . delete MY_PRINT_HELLO}
    wm protocol . MY_PRINT_HELLO {}

    test {tixMwm protocol .e add MY_PRINT_HELLO {"Print Hello"  _H Ctrl<Key>H}}
    wm protocol .e MY_PRINT_HELLO {puts Hello}

    test {destroy .e}
}
