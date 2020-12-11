# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: options.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc About {} {
    return "Testing the option configuration of the Tix widgets"
}

proc Test {} {
    test {tixComboBox .c -xxxxx} 	{missing}
    test {tixComboBox .c -xxxxx xxx} 	{unknown}
    test {tixComboBox .c -d xxx} 	{ambi}
    test {tixComboBox .c -disab 0}	{ambi} 
    test {tixComboBox .c -disablecal 0}
    Assert {[.c cget -disablecallback] == 0}
    Assert {[.c cget -disableca] == 0}
    test {tixComboBox .d -histl 10} 
    Assert {[.d cget -histlimit] == 10}
    Assert {[.d cget -histlim] == 10}
    Assert {[.d cget -historylimit] == 10}
}
