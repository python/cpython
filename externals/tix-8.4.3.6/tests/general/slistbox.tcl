# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: slistbox.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc About {} {
    return "Testing ScrolledListBox"
}

proc Test {} {
    set w [tixScrolledListBox .listbox]
    pack $w

    foreach item {{1 1} 2 3 4 5 6} {
	$w subwidget listbox insert end $item
    }

    Click [$w subwidget listbox] 30 30

    destroy $w
}
