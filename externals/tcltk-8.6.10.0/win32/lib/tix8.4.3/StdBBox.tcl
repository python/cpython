# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: StdBBox.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# StdBBox.tcl --
#
#	Standard Button Box, used in standard dialog boxes
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#



tixWidgetClass tixStdButtonBox {
    -classname TixStdButtonBox
    -superclass tixButtonBox
    -flag {
	-applycmd -cancelcmd -helpcmd -okcmd
    }
    -configspec {
	{-applycmd applyCmd ApplyCmd ""}
	{-cancelcmd cancelCmd CancelCmd ""}
	{-helpcmd helpCmd HelpCmd ""}
	{-okcmd okCmd OkCmd ""}
    }
    -default {
	{.borderWidth 	1}
	{.relief 	raised}
	{.padX 		5}
	{.padY 		10}
	{*Button.anchor	c}
	{*Button.padX	5}
    }
}

proc tixStdButtonBox:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    $w add ok     -text OK     -under 0 -width 6 -command $data(-okcmd)
    $w add apply  -text Apply  -under 0 -width 6 -command $data(-applycmd)
    $w add cancel -text Cancel -under 0 -width 6 -command $data(-cancelcmd)
    $w add help   -text Help   -under 0 -width 6 -command $data(-helpcmd)
}

proc tixStdButtonBox:config {w flag value} {
    upvar #0 $w data

    case $flag {
	-okcmd {
	    $data(w:ok)     config -command $value
	}
	-applycmd {
	    $data(w:apply)  config -command $value
	}
	-cancelcmd {
	    $data(w:cancel) config -command $value
	}
	-helpcmd {
	    $data(w:help)   config -command $value
	}
	default {
	    tixChainMethod $w config $flag $value
	}
    }
}
