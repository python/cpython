# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Init.tcl,v 1.18 2008/02/28 04:35:16 hobbs Exp $
#
# Init.tcl --
#
#	Initializes the Tix library and performes version checking to ensure
#	the Tcl, Tk and Tix script libraries loaded matches with the binary
#	of the respective packages.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
namespace eval ::tix {
}

proc tixScriptVersion {}    { return $::tix_version }
proc tixScriptPatchLevel {} { return $::tix_patchLevel }

proc ::tix::Init {dir} {
    global tix env tix_library tcl_platform auto_path

    if {[info exists tix(initialized)]} {
	return
    }

    if {![info exists tix_library]} {
        # we're running from stand-alone module. 
        set tix_library ""
    } elseif {[file isdir $tix_library]} {
        if {![info exists auto_path] ||
            [lsearch $auto_path $tix_library] == -1} {
            lappend auto_path $tix_library
        }
    }

    # STEP 1: Version checking
    #
    #
    package require Tcl 8.4
    package require -exact Tix 8.4.3

    # STEP 2: Initialize file compatibility modules
    #

    foreach file {
	fs.tcl
	Tix.tcl		Event.tcl
	Balloon.tcl	BtnBox.tcl
	CObjView.tcl	ChkList.tcl
	ComboBox.tcl	Compat.tcl
	Console.tcl	Control.tcl
	DefSchm.tcl	DialogS.tcl
	DirBox.tcl	DirDlg.tcl
	DirList.tcl	DirTree.tcl
	DragDrop.tcl	DtlList.tcl
	EFileBox.tcl	EFileDlg.tcl
	FileBox.tcl	FileCbx.tcl
	FileDlg.tcl	FileEnt.tcl
	FloatEnt.tcl
	Grid.tcl	HList.tcl
	HListDD.tcl	IconView.tcl
	LabEntry.tcl	LabFrame.tcl
	LabWidg.tcl	ListNBk.tcl
	Meter.tcl	MultView.tcl
	NoteBook.tcl	OldUtil.tcl
	OptMenu.tcl	PanedWin.tcl
	PopMenu.tcl	Primitiv.tcl
	ResizeH.tcl	SGrid.tcl
	SHList.tcl	SListBox.tcl
	STList.tcl	SText.tcl
	SWidget.tcl	SWindow.tcl
	Select.tcl	Shell.tcl
	SimpDlg.tcl	StackWin.tcl
	StatBar.tcl	StdBBox.tcl
	StdShell.tcl	TList.tcl
	Tree.tcl
	Utils.tcl	VResize.tcl
	VStack.tcl	VTree.tcl
	Variable.tcl	WInfo.tcl
    } {
	uplevel \#0 [list source [file join $dir $file]]
    }

    # STEP 3: Initialize the Tix application context
    #
    tixAppContext tix

    # DO NOT DO THIS HERE !!
    # This causes the global defaults to be altered, which may not
    # be desirable.  The user can call this after requiring Tix if
    # they wish to use different defaults.
    #
    #tix initstyle

    # STEP 4: Initialize the bindings for widgets that are implemented in C
    #
    foreach w {
	HList TList Grid ComboBox Control FloatEntry
	LabelEntry ScrolledGrid ScrolledListBox
    } {
	tix${w}Bind
    }

    rename ::tix::Init ""
}

# tixWidgetClassEx --
#
#       This procedure is similar to tixWidgetClass, except it
#       performs a [subst] on the class declaration before evaluating
#       it. This gives us a chance to specify platform-specific widget
#       default without using a lot of ugly double quotes.
#
#       The use of subst'able entries in the class declaration should
#       be restrained to widget default values only to avoid producing
#       unreadable code.
#
# Arguments:
# name -	The name of the class to declare.
# classDecl -	Various declarations about the class. See documentation
#               of tixWidgetClass for details.

proc tixWidgetClassEx {name classDecl} {
    tixWidgetClass $name [uplevel [list subst $classDecl]]
}

#
# Deprecated tix* functions
#
interp alias {} tixFileJoin {} file join
interp alias {} tixStrEq {} string equal
proc tixTrue  {args} { return 1 }
proc tixFalse {args} { return 0 }
proc tixStringSub {var fromStr toStr} {
    upvar 1 var var
    set var [string map $var [list $fromStr $toStr]]
}
proc tixGetBoolean {args} {
    set len [llength [info level 0]]
    set nocomplain 0
    if {$len == 3} {
	if {[lindex $args 0] ne "-nocomplain"} {
	    return -code error "wrong \# args:\
		must be [lindex [info level 0] 0] ?-nocomplain? string"
	}
	set nocomplain 1
	set val [lindex $args 1]
    } elseif {$len != 2} {
	return -code error "wrong \# args:\
		must be [lindex [info level 0] 0] ?-nocomplain? string"
    } else {
	set val [lindex $args 0]
    }
    if {[string is boolean -strict $val] || $nocomplain} {
	return [string is true -strict $val]
    } elseif {$nocomplain} {
	return 0
    } else {
	return -code error "\"$val\" is not a valid boolean"
    }
}
interp alias {} tixVerifyBoolean {} tixGetBoolean
proc tixGetInt {args} {
    set len [llength [info level 0]]
    set nocomplain 0
    set trunc      0
    for {set i 1} {$i < $len-1} {incr i} {
	set arg [lindex $args 0]
	if {$arg eq "-nocomplain"} {
	    set nocomplain 1
	} elseif {$arg eq "-trunc"} {
	    set trunc 1
	} else {
	    return -code error "wrong \# args: must be\
		[lindex [info level 0] 0] ?-nocomplain? ?-trunc? string"
	}
    }
    if {$i != $len-1} {
	return -code error "wrong \# args: must be\
		[lindex [info level 0] 0] ?-nocomplain? ?-trunc? string"
    }
    set val [lindex $args end]
    set code [catch {expr {round($val)}} res]
    if {$code} {
	if {$nocomplain} {
	    return 0
	} else {
	    return -code error "\"$val\" cannot be converted to integer"
	}
    }
    if {$trunc} {
	return [expr {int($val)}]
    } else {
	return $res
    }
}
proc tixFile {option filename} {
    set len [string length $option]
    if {$len > 1 && [string equal -length $len $option "tildesubst"]} {
	set code [catch {file normalize $filename} res]
	if {$code == 0} {
	    set filename $res
	}
    } elseif {$len > 1 && [string equal -length $len $option "trimslash"]} {
	# normalize extra slashes
	set filename [file join $filename]
	if {$filename ne "/"} {
	    set filename [string trimright $filename "/"]
	}
    } else {
	return -code error "unknown option \"$option\",\
		must be tildesubst or trimslash"
    }
    return $filename
}

interp alias {} tixRaiseWindow {} raise
#proc tixUnmapWindow {w} { }

#
# if tix_library is not defined, we're running in SAM mode. ::tix::Init
# will be called later by the Tix_Init() C code.
#

if {[info exists tix_library]} {
    ::tix::Init [file dirname [info script]]
}
