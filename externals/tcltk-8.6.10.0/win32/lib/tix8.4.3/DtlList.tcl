# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DtlList.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# DtlList.tcl --
#
#	This file implements DetailList widgets
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixDetailList {
    -classname TixDetailList
    -superclass tixScrolledGrid
    -method {
    }
    -flag {
	-hdrbackground
    }
    -configspec {
	{-hdrbackground hdrBackground HdrBackground #606060}
    }
    -alias {
	{-hdrbg -hdrbackground}
    }
    -default {
	{*grid.topMargin		1}
	{*grid.leftMargin		0}
    }
}


proc tixDetailList:FormatCmd {w area x1 y1 x2 y2} {
    upvar #0 $w data

    case $area {
	main {
	}
	default {
	    $data(w:grid) format border $x1 $y1 $x2 $y2 \
		-filled 1 \
		-relief raised -bd 1 -bg $data(-hdrbackground)
	}
    }
}
