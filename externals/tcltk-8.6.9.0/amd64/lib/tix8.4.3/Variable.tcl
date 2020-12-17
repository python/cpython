# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Variable.tcl,v 1.4 2001/12/09 05:04:02 idiscovery Exp $
#
# Variable.tcl --
#
#	Routines in this file are used to set up and operate variables
#	for classes that support the -variable option
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#



# tixVariable:ConfigVariable --
#
# 	Set up the -variable option for the object $w
#
# Side effects:
#
#	data(-variable) is changed to the name of the global variable
#	if the global variable exists, data(-value) takes the value of this
#	variable.
#	if the global variable does not exist, it is created with the
#	current data(-value)
#
# Return value:
#
#	true is data(-value) is changed, indicating that data(-command)
#	should be invoked.
#
proc tixVariable:ConfigVariable {w arg} {
    upvar #0 $w data

    set changed 0

    if {$data(-variable) != ""} {
	uplevel #0 \
	    [list trace vdelete $data(-variable) w "tixVariable:TraceProc $w"]
    }

    if {$arg != ""} {
	if {[uplevel #0 info exists [list $arg]]} {
	    # This global variable exists, we use its value
	    #
	    set data(-value) [uplevel #0 set [list $arg]]
	    set changed 1
	} else {
	    # This global variable does not exist; let's set it 
	    #
	    uplevel #0 [list set $arg $data(-value)]
	}
	uplevel #0 \
	    [list trace variable $arg w "tixVariable:TraceProc $w"]
    }

    return $changed
}

proc tixVariable:UpdateVariable {w} {
    upvar #0 $w data

    if {$data(-variable) != ""} {
	uplevel #0 \
	    [list trace vdelete  $data(-variable) w "tixVariable:TraceProc $w"]
	uplevel #0 \
	    [list set $data(-variable) $data(-value)]
	uplevel #0 \
	    [list trace variable $data(-variable) w "tixVariable:TraceProc $w"]

	# just in case someone has another trace and restricted my change
	#
	set data(-value) [uplevel #0 set [list $data(-variable)]]
    }
}

proc tixVariable:TraceProc {w name1 name2 op} {
    upvar #0 $w data
    set varname $data(-variable)

    if {[catch {$w config -value [uplevel #0 [list set $varname]]} err]} {
	uplevel #0 [list set $varname [list [$w cget -value]]]
	error $err
    }
    return
}

proc tixVariable:DeleteVariable {w} {
    upvar #0 $w data

    # Must delete the trace command of the -variable
    #
    if {$data(-variable) != ""} {
	uplevel #0 \
	    [list trace vdelete $data(-variable) w "tixVariable:TraceProc $w"]
    }
}
