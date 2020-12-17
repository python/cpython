# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: OldUtil.tcl,v 1.5 2004/03/28 02:44:57 hobbs Exp $
#
# OldUtil.tcl -
#
#	This is an undocumented file.
#	   Are these features used in Tix : NO.
#	   Should I use these features    : NO.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc setenv {name args} {
    global env

    if {[llength $args] == 1} {
        return [set env($name) [lindex $args 0]]
    } else {
        if {[info exists env($ename)] == 0} {
            bgerror "Error in setenv: "
                    "environment variable \"$name\" does not exist"
        } else {
            return $env($name)
        }
    }
}
#----------------------------------------------------------------------
#
#
#           U T I L I T Y   F U N C T I O N S  F O R   T I X 
#
#
#----------------------------------------------------------------------

# RESET THE STRING IN THE ENTRY
proc tixSetEntry {entry string} {
    set oldstate [lindex [$entry config -state] 4]
    $entry config -state normal
    $entry delete 0 end
    $entry insert 0 $string
    $entry config -state $oldstate
}

# GET THE FIRST SELECTED ITEM IN A LIST
proc tixListGetSingle {lst} {
    set indices [$lst curselection]
    if {$indices != ""} {
	return [$lst get [lindex $indices 0]]
    } else {
	return ""
    }
}

#----------------------------------------------------------------------
# RECORD A DIALOG'S POSITION AND RESTORE IT THE NEXT TIME IT IS OPENED
#----------------------------------------------------------------------
proc tixDialogRestore {w {flag -geometry}} {
    global tixDPos

    if {[info exists tixDPos($w)]} {
	if {![winfo ismapped $w]} {
	    wm geometry $w $tixDPos($w)
	    wm deiconify $w
	}
    } elseif {$flag eq "-geometry"} {
	update
	set tixDPos($w) [winfo geometry $w]
    } else {
	update
	set tixDPos($w) +[winfo rootx $w]+[winfo rooty $w]
    }
}
#----------------------------------------------------------------------
# RECORD A DIALOG'S POSITION AND RESTORE IT THE NEXT TIME IT IS OPENED
#----------------------------------------------------------------------
proc tixDialogWithdraw {w {flag -geometry}} {
    global tixDPos

    if {[winfo ismapped $w]} {
	if {$flag eq "-geometry"} {
	    set tixDPos($w) [winfo geometry $w]
	} else {
	    set tixDPos($w) +[winfo rootx $w]+[winfo rooty $w]
	}
	wm withdraw $w
    }
}
#----------------------------------------------------------------------
# RECORD A DIALOG'S POSITION AND RESTORE IT THE NEXT TIME IT IS OPENED
#----------------------------------------------------------------------
proc tixDialogDestroy {w {flag -geometry}} {
    global tixDPos

    if {[winfo ismapped $w]} {
	if {$flag eq "-geometry"} {
	    set tixDPos($w) [winfo geometry $w]
	} else {
	    set tixDPos($w) +[winfo rootx $w]+[winfo rooty $w]
	}
    }
    destroy $w
}
