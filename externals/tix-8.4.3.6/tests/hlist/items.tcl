# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: items.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# items.tcl --
#
#	Test the handling of DisplayStyle and DisplayItem.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Test the handling of DisplayStyle and DisplayItem."
}

proc Test {} {
    TestBlock items-1.1 {tixTmpLine} {
	tixHList .c
	set style [tixDisplayStyle text -refwindow .c -font fixed]
	.c add a -itemtype text -style $style -text Hello
	.c add b -itemtype text -text Hello

	tixHList .d
	.d add a -itemtype text -style $style -text Hello
	.d add b -itemtype text -text Hello

	pack .c .d -expand yes -fill both
	update

	destroy .c
	update
	Assert {[string comp [info command $style] ""] == 0}
    }

    catch {
	destroy .c
    }
    catch {
	destroy .d
    }
}
