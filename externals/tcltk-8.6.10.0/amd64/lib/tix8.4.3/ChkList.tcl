# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: ChkList.tcl,v 1.6 2004/03/28 02:44:57 hobbs Exp $
#
# ChkList.tcl --
#
#	This file implements the TixCheckList widget.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixCheckList {
    -classname TixCheckList
    -superclass tixTree
    -method {
	getselection getstatus setstatus
    }
    -flag {
	-radio
    }
    -configspec {
	{-radio radio Radio false tixVerifyBoolean}

	{-ignoreinvoke ignoreInvoke IgnoreInvoke true tixVerifyBoolean}
    }
    -static {
	-radio
    }
    -default {
	{.scrollbar			auto}
	{.doubleClick			false}
	{*Scrollbar.takeFocus           0}
	{*borderWidth                   1}
	{*hlist.background              #c3c3c3}
	{*hlist.drawBranch              1}
	{*hlist.height                  10}
	{*hlist.highlightBackground      #d9d9d9}
	{*hlist.indicator               1}
	{*hlist.indent                  20}
	{*hlist.itemType                imagetext}
	{*hlist.padX                    3}
	{*hlist.padY                    0}
	{*hlist.relief                  sunken}
	{*hlist.takeFocus               1}
	{*hlist.wideSelection           0}
	{*hlist.width                   20}
    }
}

proc tixCheckList:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    if {$data(-radio)} {
	set data(selected) ""
    }
}

#----------------------------------------------------------------------
#
#			Widget commands
#
#----------------------------------------------------------------------

# Helper function for getselection
#
proc tixCheckList:GetSel {w var ent mode} {
    upvar #0 $w data
    upvar $var img

    set ents ""

    catch {
	if {[$data(w:hlist) entrycget $ent -bitmap] eq $img($mode)} {
	    lappend ents $ent
	}
    }

    foreach child [$data(w:hlist) info children $ent] {
	set ents [concat $ents [tixCheckList:GetSel $w img $child $mode]]
    }

    return $ents
}


# Mode can be on, off, default
#
proc tixCheckList:getselection {w {mode on}} {
    upvar #0 $w data

    set img(on)      [tix getbitmap ck_on]
    set img(off)     [tix getbitmap ck_off]
    set img(default) [tix getbitmap ck_def]

    set ents ""
    foreach child [$data(w:hlist) info children] {
	set ents [concat $ents [tixCheckList:GetSel $w img $child $mode]]
    }
    return $ents
}

proc tixCheckList:getstatus {w ent} {
    upvar #0 $w data

    if {[$data(w:hlist) entrycget $ent -itemtype] eq "imagetext"} {
	set img(on)      [tix getbitmap ck_on]
	set img(off)     [tix getbitmap ck_off]
	set img(default) [tix getbitmap ck_def]

	set bitmap [$data(w:hlist) entrycget $ent -bitmap]

	if {$bitmap eq $img(on)} {
	    set status on
	}
	if {$bitmap eq $img(off)} {
	    set status off
	}
	if {$bitmap eq $img(default)} {
	    set status default
	}
    }

    if {[info exists status]} {
	return $status
    } else {
	return "none"
    }
}

proc tixCheckList:setstatus {w ent {mode on}} {
    upvar #0 $w data

    if {$data(-radio)} {
	set status [tixCheckList:getstatus $w $ent]

	if {$status eq $mode} {
	    return
	}

	if {$mode eq "on"} {
	    if {$data(selected) != ""} {
		tixCheckList:Select $w $data(selected) off
	    }
	    set data(selected) $ent
	    tixCheckList:Select $w $ent $mode
	} elseif {$mode eq "off"} {
	    if {$data(selected) eq $ent} {
		return
	    }
	    tixCheckList:Select $w $ent $mode
	} else {
	    tixCheckList:Select $w $ent $mode
	}
    } else {
	tixCheckList:Select $w $ent $mode
    }
}

proc tixCheckList:Select {w ent mode} {
    upvar #0 $w data

    if {[$data(w:hlist) entrycget $ent -itemtype] eq "imagetext"} {
	set img(on)      ck_on
	set img(off)     ck_off
	set img(default) ck_def

	if [catch {
	    set bitmap [tix getbitmap $img($mode)]
	    $data(w:hlist) entryconfig $ent -bitmap $bitmap
	}] {
	    # must be the "none" mode
	    #
	    catch {
		$data(w:hlist) entryconfig $ent -bitmap ""
	    }
	}
    }

    return $mode
}

proc tixCheckList:HandleCheck {w ent} {
    upvar #0 $w data

    if {[$data(w:hlist) entrycget $ent -itemtype] eq "imagetext"} {
	set img(on)      [tix getbitmap ck_on]
	set img(off)     [tix getbitmap ck_off]
	set img(default) [tix getbitmap ck_def]

	set curMode [tixCheckList:getstatus $w $ent]

	case $curMode {
	    on {
		tixCheckList:setstatus $w $ent off
	    }
	    off {
		tixCheckList:setstatus $w $ent on
	    }
	    none {
		return
	    }
	    default {
		tixCheckList:setstatus $w $ent on
	    }
	}
    }
}

proc tixCheckList:Command {w B} {
    upvar #0 $w data
    upvar $B bind

    set ent [tixEvent flag V]
    tixCheckList:HandleCheck $w $ent

    tixChainMethod $w Command $B
}

proc tixCheckList:BrowseCmd {w B} {
    upvar #0 $w data
    upvar $B bind

    set ent [tixEvent flag V]

    case [tixEvent type] {
	{<ButtonPress-1> <space>} {
	    tixCheckList:HandleCheck $w $ent
	}
    }

    tixChainMethod $w BrowseCmd $B 
}
