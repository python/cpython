# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: labentry.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# labentry.tcl
#
# Tests the TixLabelEntry widget.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Testing the TixLabelEntry widget"
}

proc Test {} {
    TestBlock labent-1.1 {LabelEntry focus management} {
        set t [toplevel .t]

        set w [tixLabelEntry .t.c -label "Stuff(c): "]
        pack $w -padx 20 -pady 10
        tixLabelEntry .t.d -label "Stuff(d): "
        pack .t.d -padx 20 -pady 10
        focus $w
        update

        set px [winfo pointerx $t]
        set py [winfo pointery $t]
        set W [winfo width $t]
        set H [winfo height $t]

        if {$W < 100} {
            set W 100
        }
        if {$H < 100} {
            set H 100
        }

        set mx [expr $px - $W / 2]
        set my [expr $py - $H / 2]

        # We must move the window under the cursor in order to test
        # the current focus
        #
        wm geometry $t $W\x$H+$mx+$my
        raise $t
        update

        # On some platforms (e.g. Red Hat Linux 5.2/x86), this fails
        # because we get: LHS = .t.c, RHS = .t.c.frame.entry
        # (not clear why).
        #
        Assert {[focus -lastfor $t] == [$w subwidget entry]}

        destroy $t
    };    # TestBlock
};        # Test
