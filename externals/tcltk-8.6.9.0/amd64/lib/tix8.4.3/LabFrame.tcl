# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: LabFrame.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# LabFrame.tcl --
#
# 	TixLabelFrame Widget: a frame box with a label
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixLabelFrame {
    -classname TixLabelFrame
    -superclass tixLabelWidget
    -method {
	frame
    }
    -flag {}
    -static {}
    -configspec {
	{-labelside labelSide LabelSide acrosstop}
	{-padx padX Pad 2}
	{-pady padY Pad 2}
    }
    -alias {}
    -default {
	{*Label.anchor          c}
	{.frame.borderWidth	2}
	{.frame.relief		groove}
	{.border.borderWidth	2}
	{.border.relief		groove}
	{.borderWidth 	 	2}
	{.padX 		 	2}
	{.padY 		 	2}
	{.anchor 	 	sw}
    }
}

#----------------------------------------------------------------------
# Public methods
#----------------------------------------------------------------------
proc tixLabelFrame:frame {w args} {

    return [eval tixCallMethod $w subwidget frame $args]
}
