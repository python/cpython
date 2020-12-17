# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SListBox.tcl,v 1.5 2004/03/28 02:44:57 hobbs Exp $
#
# SListBox.tcl --
#
#	This file implements Scrolled Listbox widgets
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


# ToDo:
# -anchor (none)
#

tixWidgetClass tixScrolledListBox {
    -classname TixScrolledListBox
    -superclass tixScrolledWidget
    -method {
    }
    -flag {
	-anchor -browsecmd -command -state
    }
    -static {
	-anchor
    }
    -configspec {
	{-anchor anchor Anchor w}
	{-browsecmd browseCmd BrowseCmd ""}
	{-command command Command ""}
	{-state state State normal}
	{-takefocus takeFocus TakeFocus 1 tixVerifyBoolean}
    }
    -default {
	{.scrollbar			auto}
	{*borderWidth			1}
	{*listbox.highlightBackground	#d9d9d9}
	{*listbox.relief		sunken}
	{*listbox.background		#c3c3c3}
	{*listbox.takeFocus		1}
	{*Scrollbar.takeFocus		0}
    }
}

proc tixScrolledListBox:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(x-first) 0
    set data(x-last)  1
    set data(y-first) 0
    set data(y-last)  1
}

proc tixScrolledListBox:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    set data(w:listbox) \
	[listbox $w.listbox]
    set data(w:hsb) \
	[scrollbar $w.hsb -orient horizontal]
    set data(w:vsb) \
	[scrollbar $w.vsb -orient vertical ]

    set data(pw:client) $data(w:listbox)
}

proc tixScrolledListBox:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    $data(w:listbox) config \
	-xscrollcommand "tixScrolledListBox:XView $w"\
	-yscrollcommand "tixScrolledListBox:YView $w"

    $data(w:hsb) config -command "$data(w:listbox) xview"
    $data(w:vsb) config -command "$data(w:listbox) yview"

    bind $w <Configure> "+tixScrolledListBox:Configure $w"
    bind $w <FocusIn> "focus $data(w:listbox)"

    bindtags $data(w:listbox) \
    "$data(w:listbox) TixListboxState Listbox TixListbox [winfo toplevel $data(w:listbox)] all"
    tixSetMegaWidget $data(w:listbox) $w
}

proc tixScrolledListBoxBind {} {
    tixBind TixListboxState <1> {
	if {[set [tixGetMegaWidget %W](-state)] eq "disabled"} {
	    break
	}
    }
    tixBind TixListbox      <1> {
	if {[string is true -strict [%W cget -takefocus]]} {
	    focus %W
	}
	tixScrolledListBox:Browse [tixGetMegaWidget %W]
    }

    tixBind TixListboxState <B1-Motion> {
	if {[set [tixGetMegaWidget %W](-state)] eq "disabled"} {
	    break
	}
    }
    tixBind TixListbox      <B1-Motion> {
	tixScrolledListBox:Browse [tixGetMegaWidget %W]
    }

    tixBind TixListboxState <Up> {
	if {[set [tixGetMegaWidget %W](-state)] eq "disabled"} {
	    break
	}
    }
    tixBind TixListbox      <Up> {
	tixScrolledListBox:KeyBrowse [tixGetMegaWidget %W]
    }

    tixBind TixListboxState <Down> {
	if {[set [tixGetMegaWidget %W](-state)] eq "disabled"} {
	    break
	}
    }
    tixBind TixListbox      <Down> {
	tixScrolledListBox:KeyBrowse [tixGetMegaWidget %W]
    }

    tixBind TixListboxState <Return> {
	if {[set [tixGetMegaWidget %W](-state)] eq "disabled"} {
	    break
	}
    }
    tixBind TixListbox      <Return> {
	tixScrolledListBox:KeyInvoke [tixGetMegaWidget %W]
    }


    tixBind TixListboxState <Double-1> {
	if {[set [tixGetMegaWidget %W](-state)] eq "disabled"} {
	    break
	}
    }
    tixBind TixListbox      <Double-1> {
	tixScrolledListBox:Invoke [tixGetMegaWidget %W]
    }

    tixBind TixListboxState <ButtonRelease-1> {
	if {[set [tixGetMegaWidget %W](-state)] eq "disabled"} {
	    break
	}
    }
    tixBind TixListbox      <ButtonRelease-1> {
	tixScrolledListBox:Browse [tixGetMegaWidget %W]
    }
}

proc tixScrolledListBox:Browse {w} {
    upvar #0 $w data

    if {$data(-browsecmd) != ""} {
	set bind(specs) {%V}
	set bind(%V) [$data(w:listbox) get \
	    [$data(w:listbox) nearest [tixEvent flag y]]]
	tixEvalCmdBinding $w $data(-browsecmd) bind
    }
}

proc tixScrolledListBox:KeyBrowse {w} {
    upvar #0 $w data

    if {$data(-browsecmd) != ""} {
	set bind(specs) {%V}
	set bind(%V) [$data(w:listbox) get active]
	tixEvalCmdBinding $w $data(-browsecmd) bind
    }
}

# tixScrolledListBox:Invoke --
#
#	The user has invoked the listbox by pressing either the <Returh>
# key or double-clicking. Call the user-supplied -command function.
#
# For both -browsecmd and -command, it is the responsibility of the
# user-supplied function to determine the current selection of the listbox
# 
proc tixScrolledListBox:Invoke {w} {
    upvar #0 $w data

    if {$data(-command) != ""} {
	set bind(specs) {%V}
	set bind(%V) [$data(w:listbox) get \
	    [$data(w:listbox) nearest [tixEvent flag y]]]
	tixEvalCmdBinding $w $data(-command) bind
    }
}

proc tixScrolledListBox:KeyInvoke {w} {
    upvar #0 $w data

    if {$data(-command) != ""} {
	set bind(specs) {%V}
	set bind(%V) [$data(w:listbox) get active]
	tixEvalCmdBinding $w $data(-command) bind
    }
}

#----------------------------------------------------------------------
#
#		option configs
#----------------------------------------------------------------------
proc tixScrolledListBox:config-takefocus {w value} {
    upvar #0 $w data
    $data(w:listbox) config -takefocus $value
}


#----------------------------------------------------------------------
#
#		Widget commands
#----------------------------------------------------------------------


#----------------------------------------------------------------------
#
#		Private Methods
#----------------------------------------------------------------------
proc tixScrolledListBox:XView {w first last} {
    upvar #0 $w data

    set data(x-first) $first
    set data(x-last) $last

    $data(w:hsb) set $first $last
    tixWidgetDoWhenIdle tixScrolledWidget:Configure $w


}

proc tixScrolledListBox:YView {w first last} {
    upvar #0 $w data

    set data(y-first) $first
    set data(y-last) $last

    $data(w:vsb) set $first $last
    tixWidgetDoWhenIdle tixScrolledWidget:Configure $w

    # Somehow an update here must be used to advoid osscilation
    #
    update idletasks
}

#
#----------------------------------------------------------------------
# virtual functions to query the client window's scroll requirement
#----------------------------------------------------------------------
proc tixScrolledListBox:GeometryInfo {w mW mH} {
    upvar #0 $w data

    return [list \
	[list $data(x-first) $data(x-last)]\
	[list $data(y-first) $data(y-last)]]
}

proc tixScrolledListBox:Configure {w} {
    upvar #0 $w data

    tixWidgetDoWhenIdle tixScrolledListBox:TrickScrollbar $w

    if {$data(-anchor) eq "e"} {
	$data(w:listbox) xview 100000
    }
}

# This procedure is necessary because listbox does not call x,y scroll command
# when its size is changed
#
proc tixScrolledListBox:TrickScrollbar {w} {
    upvar #0 $w data

    set inc [$data(w:listbox) select include 0]

    $data(w:listbox) select set 0
    if {!$inc} {
	$data(w:listbox) select clear 0
    }
}
