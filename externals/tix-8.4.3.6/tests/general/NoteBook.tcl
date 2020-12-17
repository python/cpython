# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: NoteBook.tcl,v 1.2 2002/11/13 21:12:17 idiscovery Exp $
#
proc About {} {
    return "Testing the notebook widgets"
}

proc NoteBookPageConfig {w pages} {
    foreach page $pages {
	Assert {"x[$w pagecget $page -label]" == "x$page"}
	Assert {"x[$w pageconfigure $page -label]" == "x-label {} {} {} $page"}
	$w pageconfigure $page -label foo
	Assert {"x[$w pagecget $page -label]" == "xfoo"}
	update
    }
}

proc Test {} {
    foreach class {tixListNoteBook tixNoteBook tixStackWindow} {
	set w [$class .d]
	pack $w
	update

	set pages {1 2 3 4 5 6 1111111112221}

	foreach page $pages {
	    if {$class == "tixListNoteBook"} {
		$w subwidget hlist add $page -itemtype imagetext \
		    -image [tix getimage folder] -text $page
	    }
	    set p [$w add $page -label $page]
	    for {set x 1} {$x < 10} {incr x} {
		button $p.$x -text $x
		pack $p.$x -fill x
	    }
	}

	foreach page $pages {
	    $w raise $page
	    Assert {"x[$w raised]" == "x$page"}
	    update
	}

	Assert {[string compare $pages [$w pages]] == 0}

	# test the "hooking" of the notebook frame subwidget
	#
	#
	if {$class == "tixNoteBook"} {
	    NoteBookPageConfig $w $pages
	}

	foreach page $pages {
	    Assert {"x[$w pagecget $page -raisecmd]" == "x"}
#	    Assert {"x[$w pageconfigure $page -raisecmd]" == "x-raisecmd {} {} {} {}"}
	    $w pageconfigure $page -raisecmd "RaiseCmd $page"
	    Assert {"x[$w pagecget $page -raisecmd]" == "xRaiseCmd $page"}
	    update
	}

	destroy $w
    }
}
