# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: itk.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# This file tests the pixmap image reader
#

proc About {} {
    return "This file performs tests with ITK mega widgets"
}

proc Test {} {
    frame .f
    pack .f
    tixPanedWindow .f.tpw
    pack .f.tpw -side left -expand yes -fill both
    set p1 [.f.tpw add t1 -min 20 -size 120 ]
    set p2 [.f.tpw add t2 -min 20  -size 80 ]
    frame $p1.t1
    frame $p2.t2
    pack $p1.t1 $p2.t2
    tixScrolledListBox $p1.t1.list
    tixScrolledListBox $p2.t2.list
    pack  $p1.t1.list  $p2.t2.list

    Combobox .ibox -labeltext "ItkBox" -items {one two three}
    pack .ibox
}
