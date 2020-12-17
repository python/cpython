# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: ComboBox.tcl,v 1.9 2008/02/28 22:39:13 hobbs Exp $
#
# tixCombobox --
#
#	A combobox widget is basically a listbox widget with an entry
#	widget.
#
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

global tkPriv
if {![llength [info globals tkPriv]]} {
    tk::unsupported::ExposePrivateVariable tkPriv
}
#--------------------------------------------------------------------------
# tkPriv elements used in this file:
#
# afterId -		Token returned by "after" for autoscanning.
#--------------------------------------------------------------------------
#
foreach fun {tkCancelRepeat tkListboxUpDown tkButtonUp} {
    if {![llength [info commands $fun]]} {
	tk::unsupported::ExposePrivateCommand $fun
    }
}
unset fun

tixWidgetClass tixComboBox {
    -classname TixComboBox
    -superclass tixLabelWidget
    -method {
	addhistory align appendhistory flash invoke insert pick popdown
    }
    -flag {
	-anchor -arrowbitmap -browsecmd -command -crossbitmap
	-disablecallback -disabledforeground -dropdown -editable
	-fancy -grab -histlimit -historylimit -history -listcmd
	-listwidth -prunehistory -selection -selectmode -state
	-tickbitmap -validatecmd -value -variable
    }
    -static {
	-dropdown -fancy
    }
    -forcecall {
	-variable -selectmode -state
    }
    -configspec {
	{-arrowbitmap arrowBitmap ArrowBitmap ""}
	{-anchor anchor Anchor w}
	{-browsecmd browseCmd BrowseCmd ""}
        {-command command Command ""}
	{-crossbitmap crossBitmap CrossBitmap ""}
	{-disablecallback disableCallback DisableCallback 0 tixVerifyBoolean}
	{-disabledforeground disabledForeground DisabledForeground #606060}
	{-dropdown dropDown DropDown true tixVerifyBoolean}
	{-editable editable Editable false tixVerifyBoolean}
	{-fancy fancy Fancy false tixVerifyBoolean}
	{-grab grab Grab global}
	{-listcmd listCmd ListCmd ""}
	{-listwidth listWidth ListWidth ""}
	{-historylimit historyLimit HistoryLimit ""}
	{-history history History false tixVerifyBoolean}
	{-prunehistory pruneHistory PruneHistory true tixVerifyBoolean}
	{-selectmode selectMode SelectMode browse}
	{-selection selection Selection ""}
        {-state state State normal}
	{-validatecmd validateCmd ValidateCmd ""}
	{-value value Value ""}
	{-variable variable Variable ""}
	{-tickbitmap tickBitmap TickBitmap ""}
    }
    -alias {
	{-histlimit -historylimit}
    }
    -default {
	{*Entry.relief				sunken}
	{*TixScrolledListBox.scrollbar		auto}
	{*Listbox.exportSelection		false}
	{*Listbox.takeFocus			false}
	{*shell.borderWidth			2}
	{*shell.relief				raised}
	{*shell.cursor				arrow}
	{*Button.anchor				c}
	{*Button.borderWidth			1}
	{*Button.highlightThickness		0}
	{*Button.padX				0}
	{*Button.padY				0}
	{*tick.width				18}
	{*tick.height				18}
	{*cross.width				18}
	{*cross.height				18}
	{*arrow.anchor				c}
	{*arrow.width				15}
	{*arrow.height				18}
    }
}

# States: normal numbers: for dropdown style
#         d+digit(s)    : for non-dropdown style
#
proc tixComboBox:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(curIndex)    ""
    set data(varInited)	  0
    set data(state)       none
    set data(ignore)      0

    if {$data(-history)} {
        set data(-editable) 1
    }

    if {$data(-arrowbitmap) eq ""} {
	set data(-arrowbitmap) [tix getbitmap cbxarrow]
    }
    if {$data(-crossbitmap) eq ""} {
	set data(-crossbitmap) [tix getbitmap cross]
    }
    if {$data(-tickbitmap) eq ""} {
	set data(-tickbitmap) [tix getbitmap tick]
    }
}

proc tixComboBox:ConstructFramedWidget {w frame} {
    upvar #0 $w data

    tixChainMethod $w ConstructFramedWidget $frame

    if {$data(-dropdown)} {
	tixComboBox:ConstructEntryFrame $w $frame
	tixComboBox:ConstructListShell $w
    } else {
	set f1 [frame $frame.f1]
	set f2 [frame $frame.f2]

	tixComboBox:ConstructEntryFrame $w $f1
	tixComboBox:ConstructListFrame  $w $f2
	pack $f1 -side top -pady 2 -fill x
	pack $f2 -side top -pady 2 -fill both -expand yes
    }
}

proc tixComboBox:ConstructEntryFrame {w frame} {
    upvar #0 $w data

    # (1) The entry
    #
    set data(w:entry) [entry $frame.entry]

    if {!$data(-editable)} {
	set bg [$w cget -bg]
	$data(w:entry) config -bg $bg -state disabled -takefocus 1
    }

    # This is used during "config-state"
    #
    set data(entryfg) [$data(w:entry) cget -fg]

    # (2) The dropdown button, not necessary when not in dropdown mode
    #
    set data(w:arrow) [button $frame.arrow -bitmap $data(-arrowbitmap)]
    if {!$data(-dropdown)} {
	set xframe [frame $frame.xframe -width 19]
    }

    # (3) The fancy tick and cross buttons
    #
    if {$data(-fancy)} {
	if {$data(-editable)} {
           set data(w:cross)  [button $frame.cross -bitmap $data(-crossbitmap)]
	   set data(w:tick)   [button $frame.tick  -bitmap $data(-tickbitmap)]

	   pack $frame.cross -side left -padx 1
	   pack $frame.tick  -side left -padx 1
	} else {
	   set data(w:tick)   [button $frame.tick  -bitmap $data(-tickbitmap)]
	   pack $frame.tick  -side left -padx 1
	}
    }

    if {$data(-dropdown)} {
	pack $data(w:arrow) -side right -padx 1
	foreach wid [list $data(w:frame) $data(w:label)] {
	    tixAddBindTag $wid TixComboWid
	    tixSetMegaWidget $wid $w TixComboBox
	}
    } else {
	pack $xframe -side right -padx 1
    }
    pack $frame.entry -side right -fill x -expand yes -padx 1
}

proc tixComboBox:ConstructListShell {w} {
    upvar #0 $w data

    # Create the shell and the list
    #------------------------------
    set data(w:shell) [menu $w.shell -bd 2 -relief raised -tearoff 0]
    wm overrideredirect $data(w:shell) 1
    wm withdraw $data(w:shell)

    set data(w:slistbox) [tixScrolledListBox $data(w:shell).slistbox \
	-anchor $data(-anchor) -scrollbarspace y \
	-options {listbox.selectMode "browse"}]

    set data(w:listbox) [$data(w:slistbox) subwidget listbox]

    pack $data(w:slistbox) -expand yes -fill both -padx 2 -pady 2
}

proc tixComboBox:ConstructListFrame {w frame} {
    upvar #0 $w data

    set data(w:slistbox) [tixScrolledListBox $frame.slistbox \
	-anchor $data(-anchor)]

    set data(w:listbox) [$data(w:slistbox) subwidget listbox]

    pack $data(w:slistbox) -expand yes -fill both
}


proc tixComboBox:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    # (1) Fix the bindings for the combobox
    #
    bindtags $w [list $w TixComboBox [winfo toplevel $w] all]

    # (2) The entry subwidget
    #
    tixSetMegaWidget $data(w:entry) $w TixComboBox

    bindtags $data(w:entry) [list $data(w:entry) Entry TixComboEntry\
	TixComboWid [winfo toplevel $data(w:entry)] all]

    # (3) The listbox and slistbox
    #
    $data(w:slistbox) config -browsecmd \
	[list tixComboBox:LbBrowse  $w]
    $data(w:slistbox) config -command\
	[list tixComboBox:LbCommand $w]
    $data(w:listbox) config -takefocus 0

    tixAddBindTag $data(w:listbox)  TixComboLb
    tixAddBindTag $data(w:slistbox) TixComboLb
    tixSetMegaWidget $data(w:listbox)  $w TixComboBox
    tixSetMegaWidget $data(w:slistbox) $w TixComboBox

    # (4) The buttons
    #
    if {$data(-dropdown)} {
	$data(w:arrow) config -takefocus 0
	tixAddBindTag $data(w:arrow) TixComboArrow
	tixSetMegaWidget $data(w:arrow) $w TixComboBox

	bind $data(w:root) <1>                [list tixComboBox:RootDown $w]
	bind $data(w:root) <ButtonRelease-1>  [list tixComboBox:RootUp   $w]
    }

    if {$data(-fancy)} {
	if {$data(-editable)} {
	    $data(w:cross) config -command [list tixComboBox:CrossBtn $w] \
		-takefocus 0
	}
	$data(w:tick) config -command [list tixComboBox:Invoke $w] -takefocus 0
    }

    if {$data(-dropdown)} {
	set data(state) 0
    } else {
	set data(state) n0
    }
}

proc tixComboBoxBind {} {
    #----------------------------------------------------------------------
    # The class bindings for the TixComboBox
    #
    tixBind TixComboBox <Escape> {
	if {[tixComboBox:EscKey %W]} {
	    break
	}
    }
    tixBind TixComboBox <Configure> {
	tixWidgetDoWhenIdle tixComboBox:align %W
    }
    # Only the two "linear" detail_fields  are for tabbing (moving) among
    # widgets inside the same toplevel. Other detail_fields are sort
    # of irrelevant
    #
    tixBind TixComboBox <FocusOut>  {
	if {[string equal %d NotifyNonlinear] ||
	    [string equal %d NotifyNonlinearVirtual]} {

	    if {[info exists %W(cancelTab)]} {
		unset %W(cancelTab)
	    } else {
		if {[set %W(-state)] ne "disabled"} {
		    if {[set %W(-selection)] ne [set %W(-value)]} {
			tixComboBox:Invoke %W
		    }
		}
	    }
	}
    }
    tixBind TixComboBox <FocusIn>  {
	if {"%d" eq "NotifyNonlinear" || "%d" eq "NotifyNonlinearVirtual"} {
	    focus [%W subwidget entry]

	    # CYGNUS: Setting the selection if there is no data
	    # causes backspace to misbehave.
	    if {[[set %W(w:entry)] get] ne ""} {
  		[set %W(w:entry)] selection from 0
  		[set %W(w:entry)] selection to end
  	    }

	}
    }

    #----------------------------------------------------------------------
    # The class tixBindings for the arrow button widget inside the TixComboBox
    #

    tixBind TixComboArrow <1>               {
	tixComboBox:ArrowDown [tixGetMegaWidget %W TixComboBox]
    }
    tixBind TixComboArrow <ButtonRelease-1> {
	tixComboBox:ArrowUp   [tixGetMegaWidget %W TixComboBox]
    }
    tixBind TixComboArrow <Escape>          {
	if {[tixComboBox:EscKey [tixGetMegaWidget %W TixComboBox]]} {
	    break
	}
    }


    #----------------------------------------------------------------------
    # The class tixBindings for the entry widget inside the TixComboBox
    #
    tixBind TixComboEntry <Up>		{
	tixComboBox:EntDirKey [tixGetMegaWidget %W TixComboBox] up
    }
    tixBind TixComboEntry <Down>	{
	tixComboBox:EntDirKey [tixGetMegaWidget %W TixComboBox] down
    }
    tixBind TixComboEntry <Prior>	{
	tixComboBox:EntDirKey [tixGetMegaWidget %W TixComboBox] pageup
    }
    tixBind TixComboEntry <Next>	{
	tixComboBox:EntDirKey [tixGetMegaWidget %W TixComboBox] pagedown
    }
    tixBind TixComboEntry <Return>	{
	tixComboBox:EntReturnKey [tixGetMegaWidget %W TixComboBox]
    }
    tixBind TixComboEntry <KeyPress>	{
	tixComboBox:EntKeyPress [tixGetMegaWidget %W TixComboBox]
    }
    tixBind TixComboEntry <Escape> 	{
	if {[tixComboBox:EscKey [tixGetMegaWidget %W TixComboBox]]} {
	    break
	}
    }
    tixBind TixComboEntry <Tab> 	{
	if {[set [tixGetMegaWidget %W TixComboBox](-state)] ne "disabled"} {
	    if {[tixComboBox:EntTab [tixGetMegaWidget %W TixComboBox]]} {
		break
	    }
	}
    }
    tixBind TixComboEntry <1>	{
	if {[set [tixGetMegaWidget %W TixComboBox](-state)] ne "disabled"} {
	    focus %W
	}
    }
    tixBind TixComboEntry <ButtonRelease-2>	{
	tixComboBox:EntKeyPress [tixGetMegaWidget %W TixComboBox]
    }

    #----------------------------------------------------------------------
    # The class bindings for the listbox subwidget
    #

    tixBind TixComboWid <Escape> {
	if {[tixComboBox:EscKey [tixGetMegaWidget %W TixComboBox]]} {
	    break
	}
    }

    #----------------------------------------------------------------------
    # The class bindings for some widgets inside ComboBox
    #
    tixBind TixComboWid <ButtonRelease-1> {
	tixComboBox:WidUp [tixGetMegaWidget %W TixComboBox]
    }
    tixBind TixComboWid <Escape> {
	if {[tixComboBox:EscKey [tixGetMegaWidget %W TixComboBox]]} {
	    break
	}
    }
}

#----------------------------------------------------------------------
#              Cooked events
#----------------------------------------------------------------------
proc tixComboBox:ArrowDown {w} {
    upvar #0 $w data

    if {$data(-state) eq "disabled"} {
	return
    }

    switch -exact -- $data(state) {
	0	{ tixComboBox:GoState 1 $w }
	2	{ tixComboBox:GoState 19 $w }
	default	{ tixComboBox:StateError $w }
    }
}

proc tixComboBox:ArrowUp {w} {
    upvar #0 $w data
    
    switch -exact -- $data(state) {
	1	{ tixComboBox:GoState 2 $w }
	19	{
	    # data(ignore) was already set in state 19
	    tixComboBox:GoState 4 $w
	}
	5	{ tixComboBox:GoState 13 $w }
	default	{ tixComboBox:StateError $w }
    }
}

proc tixComboBox:RootDown {w} {
    upvar #0 $w data
    
    switch -exact -- $data(state) {
	0	{
	    # Ignore
	}
	2	{ tixComboBox:GoState 3 $w }
	default { tixComboBox:StateError $w }
    }
}

proc tixComboBox:RootUp {w} {
    upvar #0 $w data
    
    switch -exact -- $data(state) {
	{1} {
	    tixComboBox:GoState 12 $w
	}
	{3} {
	    # data(ignore) was already set in state 3
	    tixComboBox:GoState 4 $w
	}
	{5} {
	    tixComboBox:GoState 7 $w
	}
	default {
	    tixComboBox:StateError $w
	}
    }
}

proc tixComboBox:WidUp {w} {
    upvar #0 $w data
    
    switch -exact -- $data(state) {
	{1} {
	    tixComboBox:GoState 12 $w
	}
	{5} {
	    tixComboBox:GoState 13 $w
	}
    }
}

proc tixComboBox:LbBrowse {w args} {
    upvar #0 $w data

    set event [tixEvent type]
    set x [tixEvent flag x]
    set y [tixEvent flag y]
    set X [tixEvent flag X]
    set Y [tixEvent flag Y]

    if {$data(-state) eq "disabled"} { return }

    switch -exact -- $event {
	<1> {
	    case $data(state) {
		{2} {
		    tixComboBox:GoState 5 $w $x $y $X $Y
		}
		{5} {
		    tixComboBox:GoState 5 $w $x $y $X $Y
		}
		{n0} {
		    tixComboBox:GoState n6 $w $x $y $X $Y
		}
		default {
		    tixComboBox:StateError $w
		}
	    }
	}
	<ButtonRelease-1> {
	    case $data(state) {
		{5} {
		    tixComboBox:GoState 6 $w $x $y $X $Y
		}
		{n6} {
		    tixComboBox:GoState n0 $w
		}
		default {
		    tixComboBox:StateError $w
		}
	    }
	}
	default {
	    # Must be a motion event
	    case $data(state) {
		{1} {
		    tixComboBox:GoState 9 $w $x $y $X $Y
		}
		{5} {
		    tixComboBox:GoState 5 $w $x $y $X $Y
		}
		{n6} {
		    tixComboBox:GoState n6 $w $x $y $X $Y
		}
		default {
		    tixComboBox:StateError $w
		}
	    }
	}
    }
}

proc tixComboBox:LbCommand {w} {
    upvar #0 $w data

    if {$data(state) eq "n0"} {
	tixComboBox:GoState n1 $w
    }
}

#----------------------------------------------------------------------
#           General keyboard event

# returns 1 if the combobox is in some special state and the Escape key
# shouldn't be handled by the toplevel bind tag. As a result, when a combobox
# is popped up in a dialog box, Escape will popdown the combo. If the combo
# is not popped up, Escape will invoke the toplevel bindtag (which can
# pop down the dialog box)
#
proc tixComboBox:EscKey {w} {
    upvar #0 $w data

    if {$data(-state) eq "disabled"} { return 0 }

    switch -exact -- $data(state) {
	{0} {
	    tixComboBox:GoState 17 $w
	}
	{2} {
	    tixComboBox:GoState 16 $w
	    return 1
	}
	{n0} {
	    tixComboBox:GoState n4 $w
	}
	default {
	    # ignore
	    return 1
	}
    }

    return 0
}

#----------------------------------------
# Keyboard events
#----------------------------------------
proc tixComboBox:EntDirKey {w dir} {
    upvar #0 $w data

    if {$data(-state) eq "disabled"} { return }

    switch -exact -- $data(state) {
	{0} {
	    tixComboBox:GoState 10 $w $dir
	}
	{2} {
	    tixComboBox:GoState 11 $w $dir
	}
	{5} {
	    # ignore
	}
	{n0} {
	    tixComboBox:GoState n3 $w $dir
	}
    }
}

proc tixComboBox:EntReturnKey {w} {
    upvar #0 $w data

    if {$data(-state) eq "disabled"} { return }

    switch -exact -- $data(state) {
	{0} {
	    tixComboBox:GoState 14 $w
	}
	{2} {
	    tixComboBox:GoState 15 $w
	}
	{5} {
	    # ignore
	}
	{n0} {
	    tixComboBox:GoState n1 $w
	}
    }
}

# Return 1 == break from the binding == no keyboard focus traversal
proc tixComboBox:EntTab {w} {
    upvar #0 $w data

    switch -exact -- $data(state) {
	{0} {
	    tixComboBox:GoState 14 $w
	    set data(cancelTab) ""
	    return 0
	}
	{2} {
	    tixComboBox:GoState 15 $w
	    set data(cancelTab) ""
	    return 0
	}
	{n0} {
	    tixComboBox:GoState n1 $w
	    set data(cancelTab) ""
	    return 0
	}
	default {
	    return 1
	}
    }
}

proc tixComboBox:EntKeyPress {w} {
    upvar #0 $w data

    if {$data(-state) eq "disabled" || !$data(-editable)} { return }

    switch -exact -- $data(state) {
	0 - 2 - n0 {
	    tixComboBox:ClearListboxSelection $w
	    tixComboBox:SetSelection $w [$data(w:entry) get] 0 0
	}
    }
}

#----------------------------------------------------------------------

proc tixComboBox:HandleDirKey {w dir} {
    upvar #0 $w data

    if {[tixComboBox:CheckListboxSelection $w]} {
	switch -exact -- $dir {
	    "up" {
		tkListboxUpDown $data(w:listbox) -1
		set data(curIndex) [lindex [$data(w:listbox) curselection] 0]
		tixComboBox:SetSelectionFromListbox $w
	    }
	    "down" {
		tkListboxUpDown $data(w:listbox)  1
		set data(curIndex) [lindex [$data(w:listbox) curselection] 0]
		tixComboBox:SetSelectionFromListbox $w
	    }
	    "pageup" {
		$data(w:listbox) yview scroll -1 pages
	    }
	    "pagedown" {
		$data(w:listbox) yview scroll  1 pages
	    }
	}
    } else {
	# There wasn't good selection in the listbox.
	#
	tixComboBox:SetSelectionFromListbox $w
    }
}

proc tixComboBox:Invoke {w} {
    upvar #0 $w data

    tixComboBox:SetValue $w $data(-selection)
    if {![winfo exists $w]} {
	return
    }

    if {$data(-history)} {
	tixComboBox:addhistory $w $data(-value)
	set data(curIndex) 0
    }
    $data(w:entry) selection from 0
    $data(w:entry) selection to end
    $data(w:entry) icursor end
}

#----------------------------------------------------------------------
#                   MAINTAINING THE -VALUE
#----------------------------------------------------------------------
proc tixComboBox:SetValue {w newValue {noUpdate 0} {updateEnt 1}} {
    upvar #0 $w data

    if {[llength $data(-validatecmd)]} {
       set data(-value) [tixEvalCmdBinding $w $data(-validatecmd) "" $newValue]
    } else {
	set data(-value) $newValue
    }

    if {! $noUpdate} {
	tixVariable:UpdateVariable $w
    }

    if {$updateEnt} {
	if {!$data(-editable)} {
	    $data(w:entry) delete 0 end
	    $data(w:entry) insert 0 $data(-value)
	}
    }

    if {!$data(-disablecallback) && [llength $data(-command)]} {
	if {![info exists data(varInited)]} {
	    set bind(specs) {%V}
	    set bind(%V)    $data(-value)

	    tixEvalCmdBinding $w $data(-command) bind $data(-value)
	    if {![winfo exists $w]} {
		# The user destroyed the window!
		return
	    }
	}
    }

    set data(-selection) $data(-value)
    if {$updateEnt} {
	tixSetEntry $data(w:entry) $data(-value)

	if {$data(-anchor) eq "e"} {
	    tixComboBox:EntryAlignEnd $w
	}
    }
}

# markSel: should the all the text in the entry be highlighted?
#
proc tixComboBox:SetSelection {w value {markSel 1} {setent 1}} {
    upvar #0 $w data

    if {$setent} {
	tixSetEntry $data(w:entry) $value
    }
    set data(-selection) $value

    if {$data(-selectmode) eq "browse"} {
	if {$markSel} {
	    $data(w:entry) selection range 0 end
	}
	if {[llength $data(-browsecmd)]} {
	    set bind(specs) {%V}
	    set bind(%V)    [$data(w:entry) get]
	    tixEvalCmdBinding $w $data(-browsecmd) bind [$data(w:entry) get]
	}
    } else {
	tixComboBox:SetValue $w $value 0 0
    }
}

proc tixComboBox:ClearListboxSelection {w} {
    upvar #0 $w data

    if {![winfo exists $data(w:listbox)]} {
	tixDebug "tixComboBox:ClearListboxSelection error non-existent $data(w:listbox)"
	return
    }

    $data(w:listbox) selection clear 0 end
}

proc tixComboBox:UpdateListboxSelection {w index} {
    upvar #0 $w data

    if {![winfo exists $data(w:listbox)]} {
	tixDebug "tixComboBox:UpdateListboxSelection error non-existent $data(w:listbox)"
	return
    }
    if {$index != ""} {
	$data(w:listbox) selection set $index
	$data(w:listbox) selection anchor $index
    }
}


proc tixComboBox:Cancel {w} {
    upvar #0 $w data

    tixSetEntry $data(w:entry) $data(-value)
    tixComboBox:SetSelection $w $data(-value)

    if {[tixComboBox:LbGetSelection $w] ne $data(-selection)} {
	tixComboBox:ClearListboxSelection $w
    }
}

proc tixComboBox:flash {w} {
    tixComboBox:BlinkEntry $w
}

# Make the entry blink when the user selects a choice
#
proc tixComboBox:BlinkEntry {w} {
    upvar #0 $w data

    if {![info exists data(entryBlacken)]} {
	set old_bg [$data(w:entry) cget -bg]
	set old_fg [$data(w:entry) cget -fg]

	$data(w:entry) config -fg $old_bg
	$data(w:entry) config -bg $old_fg

	set data(entryBlacken) 1
	after 50 tixComboBox:RestoreBlink $w [list $old_bg] [list $old_fg]
    }
}

proc tixComboBox:RestoreBlink {w old_bg old_fg} {
    upvar #0 $w data

    if {[info exists data(w:entry)] && [winfo exists $data(w:entry)]} {
	$data(w:entry) config -fg $old_fg
	$data(w:entry) config -bg $old_bg
    }

    if {[info exists data(entryBlacken)]} {
	unset data(entryBlacken)
    }
}

#----------------------------------------
#  Handle events inside the list box
#----------------------------------------

proc tixComboBox:LbIndex {w {flag ""}} {
    upvar #0 $w data

    if {![winfo exists $data(w:listbox)]} {
	tixDebug "tixComboBox:LbIndex error non-existent $data(w:listbox)"
	if {$flag eq "emptyOK"} {
	    return ""
	} else {
	    return 0
	}
    }
    set sel [lindex [$data(w:listbox) curselection] 0]
    if {$sel != ""} {
	return $sel
    } else {
	if {$flag eq "emptyOK"} {
	    return ""
	} else {
	    return 0
	}
    }
}

#----------------------------------------------------------------------
#
#			STATE MANIPULATION
#
#----------------------------------------------------------------------
proc tixComboBox:GoState-0 {w} {
    upvar #0 $w data

    if {[info exists data(w:root)] && [grab current] eq "$data(w:root)"} {
	grab release $w
    }
}

proc tixComboBox:GoState-1 {w} {
    upvar #0 $w data

    tixComboBox:Popup $w
}

proc tixComboBox:GoState-2 {w} {
    upvar #0 $w data

}

proc tixComboBox:GoState-3 {w} {
    upvar #0 $w data

    set data(ignore) 1
    tixComboBox:Popdown $w
}

proc tixComboBox:GoState-4 {w} {
    upvar #0 $w data

    tixComboBox:Ungrab $w
    if {$data(ignore)} {
	tixComboBox:Cancel $w
    } else {
	tixComboBox:Invoke $w
    }
    tixComboBox:GoState 0 $w
}

proc tixComboBox:GoState-5 {w x y X Y} {
    upvar #0 $w data

    tixComboBox:LbSelect $w $x $y $X $Y
}

proc tixComboBox:GoState-6 {w x y X Y} {
    upvar #0 $w data

    tixComboBox:Popdown $w

    if {[tixWithinWindow $data(w:shell) $X $Y]} {
	set data(ignore) 0
    } else {
	set data(ignore) 1
    }

    tixComboBox:GoState 4 $w
}

proc tixComboBox:GoState-7 {w} {
    upvar #0 $w data

    tixComboBox:Popdown $w
    set data(ignore) 1
    catch {
	global tkPriv
	if {$tkPriv(afterId) != ""} {
	    tkCancelRepeat
	}
    }

    set data(ignore) 1
    tixComboBox:GoState 4 $w
}

proc tixComboBox:GoState-9 {w x y X Y} {
    upvar #0 $w data

    catch {
	tkButtonUp $data(w:arrow)
    }
    tixComboBox:GoState 5 $w $x $y $X $Y
}

proc tixComboBox:GoState-10 {w dir} {
    upvar #0 $w data

    tixComboBox:Popup $w
    if {![tixComboBox:CheckListboxSelection $w]} {
	# There wasn't good selection in the listbox.
	#
	tixComboBox:SetSelectionFromListbox $w
    }

    tixComboBox:GoState 2 $w
}

proc tixComboBox:GoState-11 {w dir} {
    upvar #0 $w data

    tixComboBox:HandleDirKey $w $dir

    tixComboBox:GoState 2 $w
}

proc tixComboBox:GoState-12 {w} {
    upvar #0 $w data

    catch {
	tkButtonUp $data(w:arrow)
    }

    tixComboBox:GoState 2 $w
}

proc tixComboBox:GoState-13 {w} {
    upvar #0 $w data

    catch {
	global tkPriv
	if {$tkPriv(afterId) != ""} {
	    tkCancelRepeat
	}
    }
    tixComboBox:GoState 2 $w
}

proc tixComboBox:GoState-14 {w} {
    upvar #0 $w data

    tixComboBox:Invoke $w
    tixComboBox:GoState 0 $w
}

proc tixComboBox:GoState-15 {w} {
    upvar #0 $w data

    tixComboBox:Popdown $w
    set data(ignore) 0
    tixComboBox:GoState 4 $w
}

proc tixComboBox:GoState-16 {w} {
    upvar #0 $w data

    tixComboBox:Popdown $w
    tixComboBox:Cancel $w
    set data(ignore) 1
    tixComboBox:GoState 4 $w
}

proc tixComboBox:GoState-17 {w} {
    upvar #0 $w data

    tixComboBox:Cancel $w
    tixComboBox:GoState 0 $w
}

proc tixComboBox:GoState-19 {w} {
    upvar #0 $w data

    set data(ignore) [string equal $data(-selection) $data(-value)]
    tixComboBox:Popdown $w
}

#----------------------------------------------------------------------
#                      Non-dropdown states
#----------------------------------------------------------------------
proc tixComboBox:GoState-n0 {w} {
    upvar #0 $w data
}

proc tixComboBox:GoState-n1 {w} {
    upvar #0 $w data

    tixComboBox:Invoke $w
    tixComboBox:GoState n0 $w
}

proc tixComboBox:GoState-n3 {w dir} {
    upvar #0 $w data

    tixComboBox:HandleDirKey $w $dir
    tixComboBox:GoState n0 $w
}

proc tixComboBox:GoState-n4 {w} {
    upvar #0 $w data

    tixComboBox:Cancel $w
    tixComboBox:GoState n0 $w
}

proc tixComboBox:GoState-n6 {w x y X Y} {
    upvar #0 $w data

    tixComboBox:LbSelect $w $x $y $X $Y
}

#----------------------------------------------------------------------
#                      General State Manipulation
#----------------------------------------------------------------------
proc tixComboBox:GoState {s w args} {
    upvar #0 $w data

    tixComboBox:SetState $w $s
    eval tixComboBox:GoState-$s $w $args
}

proc tixComboBox:SetState {w s} {
    upvar #0 $w data

#    catch {puts [info level -2]}
#    puts "setting state $data(state) --> $s"
    set data(state) $s
}

proc tixComboBox:StateError {w} {
    upvar #0 $w data

#    error "wrong state $data(state)"
}

#----------------------------------------------------------------------
#                      Listbox handling
#----------------------------------------------------------------------

# Set a selection if there isn't one. Returns true if there was already
# a good selection inside the listbox
#
proc tixComboBox:CheckListboxSelection {w} {
    upvar #0 $w data

    if {![winfo exists $data(w:listbox)]} {
	tixDebug "tixComboBox:CheckListboxSelection error non-existent $data(w:listbox)"
	return 0
    }
    if {[$data(w:listbox) curselection] == ""} {
	if {$data(curIndex) == ""} {
	    set data(curIndex) 0
	}

	$data(w:listbox) activate $data(curIndex)
	$data(w:listbox) selection clear 0 end
	$data(w:listbox) selection set $data(curIndex)
	$data(w:listbox) see $data(curIndex)
	return 0
    } else {
	return 1
    }
}

proc tixComboBox:SetSelectionFromListbox {w} {
    upvar #0 $w data

    set string [$data(w:listbox) get $data(curIndex)] 
    tixComboBox:SetSelection $w $string
    tixComboBox:UpdateListboxSelection $w $data(curIndex)
}

proc tixComboBox:LbGetSelection {w} {
    upvar #0 $w data
    set index [tixComboBox:LbIndex $w emptyOK]

    if {$index >=0} {
	return [$data(w:listbox) get $index]
    } else {
	return ""
    }
}

proc tixComboBox:LbSelect {w x y X Y} {
    upvar #0 $w data

    set index [tixComboBox:LbIndex $w emptyOK]
    if {$index == ""} {
	set index [$data(w:listbox) nearest $y]
    }

    if {$index >= 0} {
	if {[focus -lastfor $data(w:entry)] ne $data(w:entry) &&
	    [focus -lastfor $data(w:entry)] ne $data(w:listbox)} {
	    focus $data(w:entry)
	}

	set string [$data(w:listbox) get $index] 
	tixComboBox:SetSelection $w $string

	tixComboBox:UpdateListboxSelection $w $index
    }
}

#----------------------------------------------------------------------
# Internal commands
#----------------------------------------------------------------------
proc tixComboBox:CrossBtn {w} {
    upvar #0 $w data

    $data(w:entry) delete 0 end
    tixComboBox:ClearListboxSelection $w
    tixComboBox:SetSelection $w ""
}

#--------------------------------------------------
#		Popping up list shell
#--------------------------------------------------

# Popup the listbox and grab
#
#
proc tixComboBox:Popup {w} {
    global tcl_platform
    upvar #0 $w data

    if {![winfo ismapped $data(w:root)]} {
	return
    }

    #---------------------------------------------------------------------
    # 				Pop up
    #
    if {$data(-listcmd) != ""} {
	# This option allows the user to fill in the listbox on demand
	#
	tixEvalCmdBinding $w $data(-listcmd)
    }

    # calculate the size
    set  y [winfo rooty $data(w:entry)]
    incr y [winfo height $data(w:entry)]
    incr y 3

    set bd [$data(w:shell) cget -bd]
#   incr bd [$data(w:shell) cget -highlightthickness]
    set height [expr {[winfo reqheight $data(w:slistbox)] + 2*$bd}]

    set x1 [winfo rootx $data(w:entry)]
    if {$data(-listwidth) == ""} {
	if {[winfo ismapped $data(w:arrow)]} {
	    set x2  [winfo rootx $data(w:arrow)]
	    if {$x2 >= $x1} {
		incr x2 [winfo width $data(w:arrow)]
		set width  [expr {$x2 - $x1}]
	    } else {
		set width  [winfo width $data(w:entry)]
		set x2 [expr {$x1 + $width}]
	    }
	} else {
	    set width  [winfo width $data(w:entry)]
	    set x2 [expr {$x1 + $width}]
	}
    } else {
	set width $data(-listwidth)
	set x2 [expr {$x1 + $width}]
    }

    set reqwidth [winfo reqwidth $data(w:shell)]
    if {$reqwidth < $width} {
	set reqwidth $width
    } else {
	if {$reqwidth > [expr {$width *3}]} {
	    set reqwidth [expr {$width *3}]
	}
	if {$reqwidth > [winfo vrootwidth .]} {
	    set reqwidth [winfo vrootwidth .]
	}
    }
    set width $reqwidth


    # If the listbox is too far right, pull it back to the left
    #
    set scrwidth [winfo vrootwidth .]
    if {$x2 > $scrwidth} {
	set x1 [expr {$scrwidth - $width}]
    }

    # If the listbox is too far left, pull it back to the right
    #
    if {$x1 < 0} {
	set x1 0
    }

    # If the listbox is below bottom of screen, put it upwards
    #
    set scrheight [winfo vrootheight .]
    set bottom [expr {$y+$height}]
    if {$bottom > $scrheight} {
	set y [expr {$y-$height-[winfo height $data(w:entry)]-5}]
    }

    # OK , popup the shell
    #
    global tcl_platform

    wm geometry $data(w:shell) $reqwidth\x$height+$x1+$y
    if {$tcl_platform(platform) eq "windows"} {
	update
    }
    wm deiconify $data(w:shell)
    if {$tcl_platform(platform) eq "windows"} {
	update
    }
    raise $data(w:shell)
    focus $data(w:entry)
    set data(popped) 1

    # add for safety
    update
    
    tixComboBox:Grab $w
}

proc tixComboBox:SetCursor {w cursor} {
    upvar #0 $w data

    $w config -cursor $cursor
}

proc tixComboBox:Popdown {w} {
    upvar #0 $w data

    wm withdraw $data(w:shell)
    tixComboBox:SetCursor $w ""
}

# Grab the server so that user cannot move the windows around
proc tixComboBox:Grab {w} {
    upvar #0 $w data

    tixComboBox:SetCursor $w arrow
    if {[catch {
	# We catch here because grab may fail under a lot of circumstances
	# Just don't want to break the code ...
	switch -exact -- $data(-grab) {
	    global { tixPushGrab -global $data(w:root) }
	    local  { tixPushGrab $data(w:root) }
	}
    } err]} {
	tixDebug "tixComboBox:Grab+: Error grabbing $data(w:root)\n$err"
    }
}

proc tixComboBox:Ungrab {w} {
    upvar #0 $w data

    if {[catch {
	catch {
	    switch -exact -- $data(-grab) {
		global { tixPopGrab }
		local  { tixPopGrab }
	    }
	}
    } err]} {
	tixDebug "tixComboBox:Grab+: Error grabbing $data(w:root)\n$err"
    }
}

#----------------------------------------------------------------------
#		 Alignment
#----------------------------------------------------------------------
# The following two routines can emulate a "right align mode" for the
# entry in the combo box.

proc tixComboBox:EntryAlignEnd {w} {
    upvar #0 $w data
    $data(w:entry) xview end
}


proc tixComboBox:Destructor {w} {
    upvar #0 $w data

    tixUnsetMegaWidget $data(w:entry)
    tixVariable:DeleteVariable $w

    # Chain this to the superclass
    #
    tixChainMethod $w Destructor
}


#----------------------------------------------------------------------
#                           CONFIG OPTIONS
#----------------------------------------------------------------------

proc tixComboBox:config-state {w value} {
    upvar #0 $w data
    catch {if {[$data(w:arrow) cget -state] eq $value} {set a 1}}
    if {[info exists a]} {
	return
    }

    catch {$data(w:arrow) config -state $value}
    catch {$data(w:tick)  config -state $value}
    catch {$data(w:cross) config -state $value}
    catch {$data(w:slistbox) config -state $value}

    if {[string equal $value normal]} {
	set fg [$data(w:arrow) cget -fg]
	set entryFg $data(entryfg)
	set lbSelFg [lindex [$data(w:listbox) config -selectforeground] 3]
	set lbSelBg [lindex [$data(w:listbox) config -selectbackground] 3]
	set entrySelFg [lindex [$data(w:entry) config -selectforeground] 3]
	set entrySelBg [lindex [$data(w:entry) config -selectbackground] 3]
    } else {
	set fg [$data(w:arrow) cget -disabledforeground]
	set entryFg $data(-disabledforeground) 
	set lbSelFg $entryFg
	set lbSelBg [$data(w:listbox) cget -bg]
	set entrySelFg $entryFg
	set entrySelBg [$data(w:entry) cget -bg]
    }
    if {$fg ne ""} {
	$data(w:label) config -fg $fg
	$data(w:listbox) config -fg $fg -selectforeground $lbSelFg \
	  -selectbackground $lbSelBg
    }
    $data(w:entry) config -fg $entryFg -selectforeground $entrySelFg \
      -selectbackground $entrySelBg

    if {$value eq "normal"} {
	if {$data(-editable)} {
	    $data(w:entry) config -state normal
	}
        $data(w:entry) config -takefocus 1
    } else {
	if {$data(-editable)} {
	   $data(w:entry) config -state disabled
        }
        $data(w:entry) config -takefocus 0
    }
}

proc tixComboBox:config-value {w value} {
    upvar #0 $w data

    tixComboBox:SetValue $w $value

    set data(-selection) $value

    if {[tixComboBox:LbGetSelection $w] ne $value} {
	tixComboBox:ClearListboxSelection $w
    }
}

proc tixComboBox:config-selection {w value} {
    upvar #0 $w data

    tixComboBox:SetSelection $w $value

    if {[tixComboBox:LbGetSelection $w] ne $value} {
	tixComboBox:ClearListboxSelection $w
    }
}

proc tixComboBox:config-variable {w arg} {
    upvar #0 $w data

    if {[tixVariable:ConfigVariable $w $arg]} {
       # The value of data(-value) is changed if tixVariable:ConfigVariable 
       # returns true
       set data(-selection) $data(-value)
       tixComboBox:SetValue $w $data(-value) 1
    }
    catch {
	unset data(varInited)
    }
    set data(-variable) $arg
}


#----------------------------------------------------------------------
#                     WIDGET COMMANDS
#----------------------------------------------------------------------
proc tixComboBox:align {w args} {
    upvar #0 $w data

    if {$data(-anchor) eq "e"} {
	tixComboBox:EntryAlignEnd $w
    }
}

proc tixComboBox:addhistory {w value} {
    upvar #0 $w data

    tixComboBox:insert $w 0 $value
    $data(w:listbox) selection clear 0 end

    if {$data(-prunehistory)} {
	# Prune from the end
	# 
	set max [$data(w:listbox) size]
	if {$max <= 1} {
	    return
	}
	for {set i [expr {$max -1}]} {$i >= 1} {incr i -1} {
	    if {[$data(w:listbox) get $i] eq $value} {
		$data(w:listbox) delete $i
		break
	    }
	}
    }
}

proc tixComboBox:appendhistory {w value} {
    upvar #0 $w data

    tixComboBox:insert $w end $value
    $data(w:listbox) selection clear 0 end

    if {$data(-prunehistory)} {
	# Prune from the end
	# 
	set max [$data(w:listbox) size]
	if {$max <= 1} {
	    return
	}
	for {set i [expr {$max -2}]} {$i >= 0} {incr i -1} {
	    if {[$data(w:listbox) get $i] eq $value} {
		$data(w:listbox) delete $i
		break
	    }
	}
    }
}

proc tixComboBox:insert {w index newitem} {
    upvar #0 $w data

    $data(w:listbox) insert $index $newitem

    if {$data(-history) && $data(-historylimit) != ""
	&& [$data(w:listbox) size] eq $data(-historylimit)} {
	$data(w:listbox) delete 0
    }
}

proc tixComboBox:pick {w index} {
    upvar #0 $w data

    $data(w:listbox) activate $index
    $data(w:listbox) selection clear 0 end
    $data(w:listbox) selection set active
    $data(w:listbox) see active
    set text [$data(w:listbox) get $index]

    tixComboBox:SetValue $w $text

    set data(curIndex) $index
}

proc tixComboBox:invoke {w} {
    tixComboBox:Invoke $w
}

proc tixComboBox:popdown {w} {
    upvar #0 $w data

    if {$data(-dropdown)} {
	tixComboBox:Popdown $w
    }
}
