# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SGrid.tcl,v 1.6 2002/01/24 09:13:58 idiscovery Exp $
#
# SGrid.tcl --
#
#	This file implements Scrolled Grid widgets
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

global tkPriv
if {![llength [info globals tkPriv]]} {
    tk::unsupported::ExposePrivateVariable tkPriv
}
#--------------------------------------------------------------------------
# tkPriv elements used in this file:
#
# x -	
# y -	
# X -	
# Y -	
#--------------------------------------------------------------------------
#

tixWidgetClass tixScrolledGrid {
    -classname TixScrolledGrid
    -superclass tixScrolledWidget
    -method {
    }
    -flag {
    }
    -configspec {
    }
    -default {
	{.scrollbar			auto}
	{*grid.borderWidth		1}
	{*grid.Background		#c3c3c3}
	{*grid.highlightBackground	#d9d9d9}
	{*grid.relief			sunken}
	{*grid.takeFocus		1}
	{*Scrollbar.takeFocus		0}
    }
}

proc tixScrolledGrid:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    set data(w:grid) [tixGrid $w.grid]

    set data(w:hsb) \
	[scrollbar $w.hsb -orient horizontal -takefocus 0]
    set data(w:vsb) \
	[scrollbar $w.vsb -orient vertical -takefocus 0]

    set data(pw:client) $data(w:grid)

    pack $data(w:grid) -expand yes -fill both -padx 0 -pady 0
}

proc tixScrolledGrid:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    $data(w:grid) config \
	-xscrollcommand "$data(w:hsb) set"\
	-yscrollcommand "$data(w:vsb) set"\
	-sizecmd [list tixScrolledWidget:Configure $w] \
	-formatcmd "tixCallMethod $w FormatCmd"

    $data(w:hsb) config -command "$data(w:grid) xview"
    $data(w:vsb) config -command "$data(w:grid) yview"

    bindtags $data(w:grid) \
	"$data(w:grid) TixSGrid TixGrid [winfo toplevel $data(w:grid)] all"    

    tixSetMegaWidget $data(w:grid) $w
}

#----------------------------------------------------------------------
#			RAW event bindings
#----------------------------------------------------------------------
proc tixScrolledGridBind {} {
    tixBind TixScrolledGrid <ButtonPress-1> {
	tixScrolledGrid:Button-1 [tixGetMegaWidget %W] %x %y
    }
    tixBind TixScrolledGrid <Shift-ButtonPress-1> {
	tixScrolledGrid:Shift-Button-1 %W %x %y
    }
    tixBind TixScrolledGrid <Control-ButtonPress-1> {
	tixScrolledGrid:Control-Button-1 %W %x %y
    }
    tixBind TixScrolledGrid <ButtonRelease-1> {
	tixScrolledGrid:ButtonRelease-1 %W %x %y
    }
    tixBind TixScrolledGrid <Double-ButtonPress-1> {
	tixScrolledGrid:Double-1 %W  %x %y
    }
    tixBind TixScrolledGrid <B1-Motion> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixScrolledGrid:B1-Motion %W %x %y
    }
    tixBind TixScrolledGrid <Control-B1-Motion> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixScrolledGrid:Control-B1-Motion %W %x %y
    }
    tixBind TixScrolledGrid <B1-Leave> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixScrolledGrid:B1-Leave %W
    }
    tixBind TixScrolledGrid <B1-Enter> {
	tixScrolledGrid:B1-Enter %W %x %y
    }
    tixBind TixScrolledGrid <Control-B1-Leave> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixScrolledGrid:Control-B1-Leave %W
    }
    tixBind TixScrolledGrid <Control-B1-Enter> {
	tixScrolledGrid:Control-B1-Enter %W %x %y
    }

    # Keyboard bindings
    #
    tixBind TixScrolledGrid <Up> {
	tixScrolledGrid:DirKey %W up
    }
    tixBind TixScrolledGrid <Down> {
	tixScrolledGrid:DirKey %W down
    }
    tixBind TixScrolledGrid <Left> {
	tixScrolledGrid:DirKey %W left
    }
    tixBind TixScrolledGrid <Right> {
	tixScrolledGrid:DirKey %W right
    }
    tixBind TixScrolledGrid <Prior> {
	%W yview scroll -1 pages
    }
    tixBind TixScrolledGrid <Next> {
	%W yview scroll 1 pages
    }
    tixBind TixScrolledGrid <Return> {
	tixScrolledGrid:Return %W 
    }
    tixBind TixScrolledGrid <space> {
	tixScrolledGrid:Space %W 
    }
}

#----------------------------------------------------------------------
#
#
#			 Mouse bindings
#
#
#----------------------------------------------------------------------
proc tixScrolledGrid:Button-1 {w x y} {
    if {[$w cget -state] == "disabled"} {
	return
    }
    if {[$w cget -takefocus]} {
	focus $w
    }
    case [tixScrolled:GetState $w] {
	{0} {
	    tixScrolledGrid:GoState s1 $w $x $y
       	}
	{b0} {
	    tixScrolledGrid:GoState b1 $w $x $y
       	}
	{m0} {
	    tixScrolledGrid:GoState m1 $w $x $y
       	}
	{e0} {
	    tixScrolledGrid:GoState e1 $w $x $y
       	}
    }
}



#----------------------------------------------------------------------
#
#		option configs
#----------------------------------------------------------------------

#----------------------------------------------------------------------
#
#		Widget commands
#----------------------------------------------------------------------


#----------------------------------------------------------------------
#
#		Private Methods
#----------------------------------------------------------------------

#----------------------------------------------------------------------
#		Virtual Methods
#----------------------------------------------------------------------
proc tixScrolledGrid:FormatCmd {w area x1 y1 x2 y2} {
    # do nothing
}

#----------------------------------------------------------------------
# virtual functions to query the client window's scroll requirement
#----------------------------------------------------------------------
proc tixScrolledGrid:GeometryInfo {w mW mH} {
    upvar #0 $w data


    if {$mW < 1} {
	set mW 1
    }
    if {$mH < 1} {
	set mH 1
    }

    return [$data(w:grid) geometryinfo $mW $mH]
}
