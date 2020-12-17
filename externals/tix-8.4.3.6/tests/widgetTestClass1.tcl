# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
# widgetTestClass1.test --
#
#       This class is used by several test files in this
#       directory.
#
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# $Id: widgetTestClass1.tcl,v 1.2 2002/11/13 21:12:17 idiscovery Exp $

set longword xx
for {set i 0} {$i < 10} {incr i} {
    set longword $longword$longword
}

tixWidgetClassEx widgetTestClass1 {
    -classname WidgetTestClass1
    -superclass tixFileEntry
    -flag {
	-$longword -x$longword
    }
    -configspec {
	{-$longword $longword $longword $longword}
	{-x$longword x$longword x$longword x$longword}
    }
}

proc widgetTestClass1:ConstructFramedWidget {w frame} {
    upvar #0 $w data
    set longword [widgetTestClass1_longword]

    tixChainMethod $w ConstructFramedWidget $frame

    set data(w:$longword) [button $w.$longword]
}

proc widgetTestClass1_longword {} [list return $longword]

proc widgetTestClass1:config-x$longword {w value} {
    upvar #0 $w data
    set longword [widgetTestClass1_longword]

    set data(-x$longword) $value-x$longword
}
