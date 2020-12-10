# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: AllSampl.tcl,v 1.4 2001/12/09 05:31:07 idiscovery Exp $
#
# AllSampl.tcl --
#
#	This file is a directory of all the sample programs in the
#	demos/samples subdirectory.
#
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
#

# The following data structures contains information about the requirements
# of the sample programs, as well as the relationship/grouping of the sample
# programs.
#
# Each element in an info list has four parts: type, name, group/filename, and
# condition. A group or a file is loaded only if the conditions are met.
#
# types: "d" directory "f" file
# conditions:
#	"i":	an image type must exist
#	"c":	a command must exist
#	"v": 	a variable must exist

set root {
    {d "File Selectors"		file	}
    {d "Hierachical ListBox"	hlist	}
    {d "Tabular ListBox"	tlist	{c tixTList}}
    {d "Grid Widget"		grid	{c tixGrid}}
    {d "Manager Widgets"	manager	}
    {d "Scrolled Widgets"	scroll	}
    {d "Miscellaneous Widgets"	misc	}
    {d "Image Types"		image	}
}

set image {
    {d "Compound Image"		cmpimg	}
    {d "XPM Image"		xpm	{i pixmap}}
}

set cmpimg {
    {f "In Buttons"		CmpImg.tcl	}
    {f "In NoteBook"		CmpImg2.tcl	}
    {f "Notebook Color Tabs"	CmpImg4.tcl	}
    {f "Icons"			CmpImg3.tcl	}
}

set xpm {
    {f "In Button"		Xpm.tcl		{i pixmap}}
    {f "In Menu"		Xpm1.tcl	{i pixmap}}
}

set file {
    {f DirList				DirList.tcl	}
    {f DirTree				DirTree.tcl	}
    {f DirSelectDialog			DirDlg.tcl	}
    {f ExFileSelectDialog		EFileDlg.tcl	}
    {f FileSelectDialog			FileDlg.tcl	}
    {f FileEntry			FileEnt.tcl	}
}

set hlist {
    {f HList			HList1.tcl	}
    {f CheckList		ChkList.tcl	{c tixCheckList}}
    {f "ScrolledHList (1)"	SHList.tcl	}
    {f "ScrolledHList (2)"	SHList2.tcl	}
    {f Tree			Tree.tcl	}
    {f "Tree (Dynamic)"		DynTree.tcl	{v win}}
}

set tlist {
    {f "ScrolledTList (1)"	STList1.tcl	{c tixTList}}
    {f "ScrolledTList (2)"	STList2.tcl	{c tixTList}}
}
global tcl_platform
#  This demo hangs windows
if {$tcl_platform(platform) != "windows"} {
lappend tlist     {f "TList File Viewer"	STList3.tcl	{c tixTList}}
}

set grid {
    {f "Simple Grid"		SGrid0.tcl	{c tixGrid}}
    {f "ScrolledGrid"		SGrid1.tcl	{c tixGrid}}
    {f "Editable Grid"		EditGrid.tcl	{c tixGrid}}
}

set scroll {
    {f ScrolledListBox		SListBox.tcl	}
    {f ScrolledText		SText.tcl	}
    {f ScrolledWindow		SWindow.tcl	}
    {f "Canvas Object View"	CObjView.tcl	{c tixCObjView}}
}

set manager {
    {f ListNoteBook		ListNBK.tcl	}
    {f NoteBook			NoteBook.tcl	}
    {f PanedWindow		PanedWin.tcl	}
}

set misc {
    {f Balloon			Balloon.tcl	}
    {f ButtonBox		BtnBox.tcl	}
    {f ComboBox			ComboBox.tcl	}
    {f Control			Control.tcl	}
    {f LabelEntry		LabEntry.tcl	}
    {f LabelFrame		LabFrame.tcl	}
    {f Meter			Meter.tcl	{c tixMeter}}
    {f OptionMenu		OptMenu.tcl	}
    {f PopupMenu		PopMenu.tcl	}
    {f Select			Select.tcl	}
    {f StdButtonBox		StdBBox.tcl	}
}

# ForAllSamples --
#
#	Iterates over all the samples that can be run on this platform.
#
# Arguments:
#	name:		For outside callers, it must be "root"
#	token:		An arbtrary string passed in by the caller.
#	command:	Command prefix to be executed for each node
#			in the samples hierarchy. It should return the
#			token of the node that it has just created, if any.
#
proc ForAllSamples {name token command} {
    global $name win

    if {[tix platform] == "windows"} {
	set win 1
    }

    foreach line [set $name] {
	set type [lindex $line 0]
	set text [lindex $line 1]
	set dest [lindex $line 2]
	set cond [lindex $line 3]

	case [lindex $cond 0] {
	    c {
		set cmd [lindex $cond 1]
		if {[info command $cmd] != $cmd} {
		    if ![auto_load $cmd] {
			continue
		    }
		}
	    }
	    i {
		if {[lsearch [image types] [lindex $cond 1]] == -1} {
		    continue
		}
	    }
	    v {
		set doit 1
		foreach var [lrange $cond 1 end] {
		    if [uplevel #0 info exists [list $var]] {
			set doit 0
			break
		    }
		}
		if !$doit {
		    continue
		}
	    }
	}


	if {$type == "d"} {
	    set tok [eval $command [list $token] $type [list $text] \
	        [list $dest]]
	    ForAllSamples $dest $tok $command
	    eval $command [list $tok] done xx xx
	} else {
	    set tok [eval $command [list $token] $type [list $text] \
		[list $dest]]
	}
    }
}


proc DoAll {hlist {path ""}} {
    catch {
	set theSample [$hlist info data $path]
	if {$theSample != {}} {
	    set title [lindex $theSample 0]
	    set prog  [lindex $theSample 1]

	    RunProg $title $prog
	   update
	}
    }

    foreach p [$hlist info children $path] {
	DoAll $hlist $p
    }    
}
