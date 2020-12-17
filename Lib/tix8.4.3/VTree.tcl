# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: VTree.tcl,v 1.6 2004/03/28 02:44:57 hobbs Exp $
#
# VTree.tcl --
#
#	Virtual base class for Tree widgets.
#
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixVTree {
    -virtual true
    -classname TixVTree
    -superclass tixScrolledHList
    -method {
    }
    -flag {
	-ignoreinvoke
    }
    -configspec {
	{-ignoreinvoke ignoreInvoke IgnoreInvoke false tixVerifyBoolean}
    }
    -default {
    }
}

proc tixVTree:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec
}

proc tixVTree:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    set data(indStyle) \
	[tixDisplayStyle image -refwindow $data(w:hlist) -padx 0 -pady 0]
}

proc tixVTree:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    $data(w:hlist) config \
	-indicatorcmd [list tixVTree:IndicatorCmd $w] \
	-browsecmd [list tixVTree:BrowseCmdHook $w] \
	-command [list tixVTree:CommandHook $w]
}

proc tixVTree:IndicatorCmd {w args} {
    upvar #0 $w data

    set event [tixEvent type]
    set ent   [tixEvent flag V]

    set type     [tixVTree:GetType $w $ent]
    set plus	 [tix getimage plus]
    set plusarm	 [tix getimage plusarm]
    set minus	 [tix getimage minus]
    set minusarm [tix getimage minusarm]

    if {![$data(w:hlist) info exists $ent]} {return}
    switch -exact -- $event {
	<Arm> {
	    if {![$data(w:hlist) indicator exists $ent]} {return}
	    $data(w:hlist) indicator config $ent \
		-image [expr {$type eq "open" ? $plusarm : $minusarm}]
	}
	<Disarm> {
	    if {![$data(w:hlist) indicator exists $ent]} {return}
	    $data(w:hlist) indicator config $ent \
		-image [expr {$type eq "open" ? $plus : $minus}]
	}
	<Activate> {
	    upvar bind bind
	    tixCallMethod $w Activate $ent $type
	    set bind(%V) $ent
	    tixVTree:BrowseCmdHook $w
	}
    }
}

proc tixVTree:GetType {w ent} {
    upvar #0 $w data

    if {![$data(w:hlist) indicator exists $ent]} {
	return none
    }

    set img [$data(w:hlist) indicator cget $ent -image]
    if {$img eq [tix getimage plus] || $img eq [tix getimage plusarm]} {
	return open
    }
    return close
}

proc tixVTree:Activate {w ent type} {
    upvar #0 $w data

    if {$type eq "open"} {
	tixCallMethod $w OpenCmd $ent
	$data(w:hlist) indicator config $ent -image [tix getimage minus]
    } else {
	tixCallMethod $w CloseCmd $ent
	$data(w:hlist) indicator config $ent -image [tix getimage plus]
    }
}

proc tixVTree:CommandHook {w args} {
    upvar #0 $w data
    upvar bind bind

    tixCallMethod $w Command bind
}

proc tixVTree:BrowseCmdHook {w args} {
    upvar #0 $w data
    upvar bind bind

    tixCallMethod $w BrowseCmd bind
}

proc tixVTree:SetMode {w ent mode} {
    upvar #0 $w data

    switch -exact -- $mode {
	open {
	    $data(w:hlist) indicator create $ent -itemtype image \
		-image [tix getimage plus]  -style $data(indStyle)
	}
	close {
	    $data(w:hlist) indicator create $ent -itemtype image \
		-image [tix getimage minus] -style $data(indStyle)
	}
	none {
	    if {[$data(w:hlist) indicator exist $ent]} {
		$data(w:hlist) indicator delete $ent 
	    }
	}
    }
}

#----------------------------------------------------------------------
#
#			Virtual Methods
#
#----------------------------------------------------------------------
proc tixVTree:OpenCmd {w ent} {
    upvar #0 $w data

    # The default action
    foreach kid [$data(w:hlist) info children $ent] {
	$data(w:hlist) show entry $kid
    }
}

proc tixVTree:CloseCmd {w ent} {
    upvar #0 $w data

    # The default action
    foreach kid [$data(w:hlist) info children $ent] {
	$data(w:hlist) hide entry $kid
    }
}

proc tixVTree:Command {w B} {
    upvar #0 $w data
    upvar $B bind

    if {$data(-ignoreinvoke)} {
	return
    }
    set ent [tixEvent flag V]
    if {[$data(w:hlist) indicator exist $ent]} {
	tixVTree:Activate $w $ent [tixVTree:GetType $w $ent]
    }
}

proc tixVTree:BrowseCmd {w B} {
}
#----------------------------------------------------------------------
#
#			Widget commands
#
#----------------------------------------------------------------------
