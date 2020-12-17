# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: WInfo.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# WInfo.tcl --
#
#	This file implements the command tixWInfo, which return various
#	information about a Tix widget.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc tixWInfo {option w} {
    upvar #0 $w data

    case $option {
	tix {
	    # Is this a Tix widget?
	    #
	    return [info exists data(className)]
	}
	compound {
	    # Is this a compound widget?
	    #	Currently this is the same as "tixWinfo tix" because only
	    # Tix compilant compound widgets are supported
	    return [info exists data(className)]
	}
	class {
	    if {[info exists data(className)]} {
		return $data(className)
	    } else {
		return ""
	    }
	}
    }
}
