# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DirList.tcl,v 1.3 2004/03/28 02:44:57 hobbs Exp $
#
# This file tests the pixmap image reader
#

proc About {} {
    return "This file performs test on the DirList widget"
}

proc Test {} {
    set w .dirlist

    tixDirList $w
    pack $w

    set h [$w subwidget hlist]

    # If we didn't specifi -value, the DirList should display the
    # current directory
    Assert {[string equal [$w cget -value] [pwd]]}

    # After changing the directory, the selection and anchor should change as
    # well
    set root [$h info children ""]
    ClickHListEntry $h $root single
    Assert {[string equal [$w cget -value] [$h info data $root]]}
    Assert {[string equal [$h info selection] $root]}
    Assert {[string equal [$h info anchor]    $root]}

    if {$::tcl_platform(platform) eq "windows"} {
	set dir1 C:\\Windows
	set dir2 C:\\Backup
    } else {
	set dir1 /etc
	set dir2 /etc
    }

    foreach dir [list $dir1 $dir2] {
	if ![file exists $dir] {
	    continue
	}

	$w config -value $dir
	Assert {[string equal [$w cget -value] $dir]}
	Assert {[string equal [$h info data [$h info anchor]] $dir]}
    }
}
