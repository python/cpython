# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Select.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# Select.tcl --
#
#	Implement the tixSelect widget.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixSelect {
    -superclass tixLabelWidget
    -classname  TixSelect
    -method {
	add button invoke
    }
    -flag {
	-allowzero -buttontype -command -disablecallback -orientation
	-orient -padx -pady -radio -selectedbg -state -validatecmd
	-value -variable
    }
    -forcecall {
	-variable -state
    }
    -static {
	-allowzero -orientation -padx -pady -radio
    }
    -configspec {
	{-allowzero allowZero AllowZero 0 tixVerifyBoolean}
	{-buttontype buttonType ButtonType button}
	{-command command Command ""}
	{-disablecallback disableCallback DisableCallback 0 tixVerifyBoolean}
	{-orientation orientation Orientation horizontal}
	{-padx padx Pad 0}
	{-pady pady Pad 0}
	{-radio radio Radio 0 tixVerifyBoolean}
	{-selectedbg selectedBg SelectedBg gray}
	{-state state State normal}
	{-validatecmd validateCmd ValidateCmd ""}
	{-value value Value ""}
	{-variable variable Variable ""}
    }
    -alias {
	{-orient -orientation}
    }
    -default {
	{*frame.borderWidth			1}
	{*frame.relief				sunken}
	{*Button.borderWidth			2}
	{*Button.highlightThickness		0}
    }
}

proc tixSelect:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec
    set data(items)    ""
    set data(buttonbg) ""
    set data(varInited)	  0
}

#----------------------------------------------------------------------
#                           CONFIG OPTIONS
#----------------------------------------------------------------------
proc tixSelect:config-state {w arg} {
    upvar #0 $w data

    if {$arg == "disabled"} {
	foreach item $data(items) {
	    $data(w:$item) config -state disabled -relief raised \
		-bg $data(buttonbg)
	}
	if {![info exists data(labelFg)]} {
	    set data(labelFg) [$data(w:label) cget -foreground]
	    catch {
		$data(w:label) config -fg [tix option get disabled_fg]
	    }
	}
    } else {
	foreach item $data(items) {
	    if {[lsearch $data(-value) $item] != -1} {
		# This button is selected
		#
		$data(w:$item) config -relief sunken -bg $data(-selectedbg) \
		    -state normal
	    } else {
		$data(w:$item) config -relief raised -bg $data(buttonbg) \
		    -command "$w invoke $item" -state normal
	    }
	}
	if {[info exists data(labelFg)]} {
	    catch {
		$data(w:label) config -fg $data(labelFg)
	    }
	    unset data(labelFg)
	}
    }

    return ""
}

proc tixSelect:config-variable {w arg} {
    upvar #0 $w data

    set oldValue $data(-value)

    if {[tixVariable:ConfigVariable $w $arg]} {
	# The value of data(-value) is changed if tixVariable:ConfigVariable 
	# returns true
	set newValue $data(-value)
	set data(-value) $oldValue
	tixSelect:config-value $w $newValue
    }
    catch {
	unset data(varInited)
    }
    set data(-variable) $arg
}

proc tixSelect:config-value {w value} {
    upvar #0 $w data

    # sanity checking
    #
    foreach item $value {
	if {[lsearch $data(items) $item] == "-1"} {
	    error "subwidget \"$item\" does not exist"
	}
    }

    tixSelect:SetValue $w $value
}

#----------------------------------------------------------------------
#                     WIDGET COMMANDS
#----------------------------------------------------------------------
proc tixSelect:add {w name args} {
    upvar #0 $w data

    set data(w:$name) [eval $data(-buttontype) $data(w:frame).$name -command \
	 [list "$w invoke $name"] -takefocus 0 $args]

    if {$data(-orientation) == "horizontal"} {
	pack $data(w:$name) -side left -expand yes -fill y\
	    -padx $data(-padx) -pady $data(-pady)
    } else {
	pack $data(w:$name) -side top -expand yes  -fill x\
	    -padx $data(-padx) -pady $data(-pady)
    }

    if {$data(-state) == "disabled"} {
	$data(w:$name) config -relief raised -state disabled
    }

    # find out the background of the buttons
    #
    if {$data(buttonbg) == ""} {
	set data(buttonbg) [lindex [$data(w:$name) config -background] 4]
	
    }

    lappend data(items) $name
}

# Obsolete command
#
proc tixSelect:button {w name args} {
    upvar #0 $w data

    if {$args != ""} {
	return [eval $data(w:$name) $args]
    } else {
	return $w.$name
    }
}

# This is called when a button is invoked
#
proc tixSelect:invoke {w button} {
    upvar #0 $w data

    if {$data(-state) != "normal"} {
	return
    }

    set newValue $data(-value)

    if {[lsearch $data(-value) $button] != -1} {
	# This button was selected
	#
    	if {[llength $data(-value)] > 1 || [tixGetBoolean $data(-allowzero)]} {

	    # Take the button from the selected list
	    #
	    set newValue ""
	    foreach item $data(-value) {
		if {$item != $button} {
		    lappend newValue $item
		}
	    }
	}
    } else {
	# This button was not selected
	#
	if {[tixGetBoolean $data(-radio)]} {
	    # The button become the sole item in the list
	    #
	    set newValue [list $button]
	} else {
	    # Add this button into the list
	    #
	    lappend newValue $button
	}
    }

    if {$newValue != $data(-value)} {
	tixSelect:SetValue $w $newValue
    }
}

#----------------------------------------------------------------------
#                Private functions
#----------------------------------------------------------------------
proc tixSelect:SetValue {w newValue {noUpdate 0}} {
    upvar #0 $w data

    set oldValue $data(-value)

    if {$data(-validatecmd) != ""} {
       set data(-value) [tixEvalCmdBinding $w $data(-validatecmd) "" $newValue]
    } else {
	if {[tixGetBoolean $data(-radio)] && [llength $newValue] > 1} {
	    error "cannot choose more than one items in a radio box"
	}

	if {![tixGetBoolean $data(-allowzero)] && [llength $newValue] == 0} {
	    error "empty selection not allowed"
	}

	set data(-value) $newValue
    }

    if {! $noUpdate} {
	tixVariable:UpdateVariable $w
    }

    # Reset all to be unselected
    #
    foreach item $data(items) {
	if {[lsearch $data(-value) $item] == -1} {
	    # Is unselected
	    #
	    if {[lsearch $oldValue $item] != -1} {
		# was selected
		# -> popup the button, call command
		#
		$data(w:$item) config -relief raised -bg $data(buttonbg)
		tixSelect:CallCommand $w $item 0
	    }
	} else {
	    # Is selected
	    #
	    if {[lsearch $oldValue $item] == -1} {
		# was unselected
		# -> push down the button, call command
		#
		$data(w:$item) config -relief sunken -bg $data(-selectedbg)
		tixSelect:CallCommand $w  $item 1
	    }
	}
    }
}

proc tixSelect:CallCommand {w name value} {
    upvar #0 $w data

    if {!$data(-disablecallback) && $data(-command) != ""} {
	if {![info exists data(varInited)]} {
	    set bind(specs) "name value"
	    set bind(name)  $name
	    set bind(value) $value
	    tixEvalCmdBinding $w $data(-command) bind $name $value
	}
    }
}

proc tixSelect:Destructor {w} {

    tixVariable:DeleteVariable $w

    # Chain this to the superclass
    #
    tixChainMethod $w Destructor
}
