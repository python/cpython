# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Control.tcl,v 1.9 2004/03/28 02:44:57 hobbs Exp $
#
# Control.tcl --
#
# 	Implements the TixControl Widget. It is called the "SpinBox"
# 	in other toolkits.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixControl {
    -classname  TixControl
    -superclass tixLabelWidget
    -method {
	incr decr invoke update
    }
    -flag {
	-allowempty -autorepeat -command -decrcmd -disablecallback
	-disabledforeground -incrcmd -initwait -integer -llimit
	-repeatrate -max -min -selectmode -step -state -validatecmd
	-value -variable -ulimit
    }
    -forcecall {
	-variable -state
    }
    -configspec {
	{-allowempty allowEmpty AllowEmpty false}
	{-autorepeat autoRepeat AutoRepeat true}
	{-command command Command ""}
	{-decrcmd decrCmd DecrCmd ""}
	{-disablecallback disableCallback DisableCallback 0 tixVerifyBoolean}
	{-disabledforeground disabledForeground DisabledForeground #303030}
	{-incrcmd incrCmd IncrCmd ""}
	{-initwait initWait InitWait 500}
	{-integer integer Integer false}
	{-max max Max ""}
	{-min min Min ""}
	{-repeatrate repeatRate RepeatRate 50}
	{-step step Step 1}
	{-state state State normal}
	{-selectmode selectMode SelectMode normal}
	{-validatecmd validateCmd ValidateCmd ""}
	{-value value Value 0}
	{-variable variable Variable ""}
    }
    -alias {
	{-llimit -min}
	{-ulimit -max}
    }
    -default {
	{.borderWidth 			0}
	{*entry.relief			sunken}
	{*entry.width			5}
	{*label.anchor			e}
	{*label.borderWidth		0}
	{*Button.anchor			c}
	{*Button.borderWidth		2}
	{*Button.highlightThickness	1}
	{*Button.takeFocus		0}
    }
}

proc tixControl:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(varInited)	  0
    set data(serial)	0
}

proc tixControl:ConstructFramedWidget {w frame} {
    upvar #0 $w data

    tixChainMethod $w ConstructFramedWidget $frame

    set data(w:entry)  [entry $frame.entry]

    set data(w:incr) \
	[button $frame.incr -bitmap [tix getbitmap incr] -takefocus 0]
    set data(w:decr) \
	[button $frame.decr -bitmap [tix getbitmap decr] -takefocus 0]

#    tixForm $data(w:entry) -left 0 -top 0 -bottom -1 -right $data(w:decr) 
#    tixForm $data(w:incr) -right -1 -top 0 -bottom %50
#    tixForm $data(w:decr) -right -1 -top $data(w:incr) -bottom -1

    pack $data(w:entry) -side left   -expand yes -fill both
    pack $data(w:decr)  -side bottom -fill both -expand yes
    pack $data(w:incr)  -side top    -fill both -expand yes

    $data(w:entry) delete 0 end
    $data(w:entry) insert 0 $data(-value)

    # This value is used to configure the disable/normal fg of the ebtry
    set data(entryfg) [$data(w:entry) cget -fg]
    set data(labelfg) [$data(w:label) cget -fg]
}

proc tixControl:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    bind $data(w:incr) <ButtonPress-1> \
	[list after idle tixControl:StartRepeat $w 1]
    bind $data(w:decr) <ButtonPress-1> \
	[list after idle tixControl:StartRepeat $w -1]

    # These bindings will stop the button autorepeat when the 
    # mouse button is up
    foreach btn [list $data(w:incr) $data(w:decr)] {
	bind $btn <ButtonRelease-1> [list tixControl:StopRepeat $w]
    }

    tixSetMegaWidget $data(w:entry) $w

    # If user press <return>, verify the value and call the -command
    #
    tixAddBindTag $data(w:entry) TixControl:Entry
}

proc tixControlBind {} {
    tixBind TixControl:Entry <Return> {
	tixControl:Invoke [tixGetMegaWidget %W] 1
    }
    tixBind TixControl:Entry <Escape> {
	tixControl:Escape [tixGetMegaWidget %W]
    }
    tixBind TixControl:Entry <Up> {
	[tixGetMegaWidget %W] incr
    }
    tixBind TixControl:Entry <Down> {
	[tixGetMegaWidget %W] decr
    }
    tixBind TixControl:Entry <FocusOut> {
	if {"%d" eq "NotifyNonlinear" || "%d" eq "NotifyNonlinearVirtual"} {
	    tixControl:Tab [tixGetMegaWidget %W] %d
	}
    }
    tixBind TixControl:Entry <Any-KeyPress> {
	tixControl:KeyPress [tixGetMegaWidget %W]
    }
    tixBind TixControl:Entry <Any-Tab> {
	# This has a higher priority than the <Any-KeyPress>  binding
	# --> so that data(edited) is not set
    }
}

#----------------------------------------------------------------------
#                           CONFIG OPTIONS
#----------------------------------------------------------------------
proc tixControl:config-state {w arg} {
    upvar #0 $w data

    if {$arg eq "normal"} {
	$data(w:incr)  config -state $arg
	$data(w:decr)  config -state $arg
	catch {
	    $data(w:label) config -fg $data(labelfg)
	}
	$data(w:entry) config -state $arg -fg $data(entryfg)
    } else {
	$data(w:incr)  config -state $arg
	$data(w:decr)  config -state $arg
	catch {
	    $data(w:label) config -fg $data(-disabledforeground)
	}
	$data(w:entry) config -state $arg -fg $data(-disabledforeground)
    }
}

proc tixControl:config-value {w value} {
    upvar #0 $w data

    tixControl:SetValue $w $value 0 1

    # This will tell the Intrinsics: "Please use this value"
    # because "value" might be altered by SetValues
    #
    return $data(-value)
}

proc tixControl:config-variable {w arg} {
    upvar #0 $w data

    if {[tixVariable:ConfigVariable $w $arg]} {
       # The value of data(-value) is changed if tixVariable:ConfigVariable 
       # returns true
       tixControl:SetValue $w $data(-value) 1 1
    }
    catch {
	unset data(varInited)
    }
    set data(-variable) $arg
}

#----------------------------------------------------------------------
#                         User Commands
#----------------------------------------------------------------------
proc tixControl:incr {w {by 1}} {
    upvar #0 $w data

    if {$data(-state) ne "disabled"} {
	if {![catch {$data(w:entry) index sel.first}]} {
	    $data(w:entry) select from end
	    $data(w:entry) select to   end
	}
	# CYGNUS - why set value before changing it?
	#tixControl:SetValue $w [$data(w:entry) get] 0 1
	tixControl:AdjustValue $w $by
    }
}

proc tixControl:decr {w {by 1}} {
    upvar #0 $w data

    if {$data(-state) ne "disabled"} {
	if {![catch {$data(w:entry) index sel.first}]} {
	    $data(w:entry) select from end
	    $data(w:entry) select to   end
	}
	# CYGNUS - why set value before changing it?
	#tixControl:SetValue $w [$data(w:entry) get] 0 1
	tixControl:AdjustValue $w [expr {0 - $by}]
    }
}

proc tixControl:invoke {w} {
    upvar #0 $w data

    tixControl:Invoke $w 0
}

proc tixControl:update {w} {
    upvar #0 $w data

    if {[info exists data(edited)]} {
	tixControl:invoke $w
    }
}

#----------------------------------------------------------------------
#                       Internal Commands
#----------------------------------------------------------------------

# Change the value by a multiple of the data(-step)
#
proc tixControl:AdjustValue {w amount} {
    upvar #0 $w data

    if {$amount == 1 && [llength $data(-incrcmd)]} {
	set newValue [tixEvalCmdBinding $w $data(-incrcmd) "" $data(-value)]
    } elseif {$amount == -1 && [llength $data(-decrcmd)]} {
	set newValue [tixEvalCmdBinding $w $data(-decrcmd) "" $data(-value)]
    } else {
	set newValue [expr {$data(-value) + $amount * $data(-step)}]
    }

    if {$data(-state) ne "disabled"} {
	tixControl:SetValue $w $newValue 0 1
    }
}

proc tixControl:SetValue {w newvalue noUpdate forced} {
    upvar #0 $w data

    if {[$data(w:entry) selection present]} {
	set oldSelection [list [$data(w:entry) index sel.first] \
			      [$data(w:entry) index sel.last]]
    }

    set oldvalue $data(-value)
    set oldCursor [$data(w:entry) index insert]
    set changed 0


    if {[llength $data(-validatecmd)]} {
	# Call the user supplied validation command
	#
       set data(-value) [tixEvalCmdBinding $w $data(-validatecmd) "" $newvalue]
    } else {
	# Here we only allow int or floating numbers
	#
	# If the new value is not a valid number, the old value will be
	# kept due to the "catch" statements
	#
	if {[catch {expr 0+$newvalue}]} {
	    set newvalue 0
	    set data(-value) 0
	    set changed 1
	}

	if {$newvalue == ""} {
	    if {![string is true -strict $data(-allowempty)]} {
		set newvalue 0
		set changed 1
	    } else {
		set data(-value) ""
	    }
	}

	if {$newvalue != ""} {
	    # Change this to a valid decimal string (trim leading 0)
	    #
	    regsub -- {^[0]*} $newvalue "" newvalue
	    if {[catch {expr 0+$newvalue}]} {
		set newvalue 0
		set data(-value) 0
		set changed 1
	    }
	    if {$newvalue == ""} {
		set newvalue 0
	    }

	    if {[string is true -strict $data(-integer)]} {
		set data(-value) [tixGetInt -nocomplain $newvalue]
	    } else {
		if {[catch {set data(-value) [format "%d" $newvalue]}]} {
		    if {[catch {set data(-value) [expr $newvalue+0.0]}]} {
			set data(-value) $oldvalue
		    }
		}
	    }
	    
	    # Now perform boundary checking
	    #
	    if {$data(-max) != "" && $data(-value) > $data(-max)} {
		set data(-value) $data(-max)
	    }
	    if {$data(-min) != "" && $data(-value) < $data(-min)} {
		set data(-value) $data(-min)
	    }
	}
    }

    if {! $noUpdate} {
	tixVariable:UpdateVariable $w
    }

    if {$forced || ($newvalue ne $data(-value)) || $changed} {
	$data(w:entry) delete 0 end
	$data(w:entry) insert 0 $data(-value)
	$data(w:entry) icursor $oldCursor
	if {[info exists oldSelection]} {
	    eval [list $data(w:entry) selection range] $oldSelection
	}
    }

    if {!$data(-disablecallback) && $data(-command) != ""} {
	if {![info exists data(varInited)]} {
	    set bind(specs) ""
	    tixEvalCmdBinding $w $data(-command) bind $data(-value)
	}
    }
}

proc tixControl:Invoke {w forced} {
    upvar #0 $w data

    catch {
	unset data(edited)
    }

    if {[catch {$data(w:entry) index sel.first}] == 0} {
	# THIS ENTRY OWNS SELECTION --> TURN IT OFF
	#
	$data(w:entry) select from end
	$data(w:entry) select to   end
    }

    tixControl:SetValue $w [$data(w:entry) get] 0 $forced
}

#----------------------------------------------------------------------
# The three functions StartRepeat, Repeat and StopRepeat make use of the
# data(serial) variable to discard spurious repeats: If a button is clicked
# repeatedly but is not hold down, the serial counter will increase
# successively and all "after" time event handlers will be discarded
#----------------------------------------------------------------------
proc tixControl:StartRepeat {w amount} {
    if {![winfo exists $w]} {
	return
    }

    upvar #0 $w data

    incr data(serial)
    # CYGNUS bug fix
    # Need to set a local variable because otherwise the buttonrelease
    # callback could change the value of data(serial) between now and
    # the time the repeat is scheduled.
    set serial $data(serial)

    if {![catch {$data(w:entry) index sel.first}]} {
	$data(w:entry) select from end
	$data(w:entry) select to   end
    }

    if {[info exists data(edited)]} {
	unset data(edited)
	tixControl:SetValue $w [$data(w:entry) get] 0 1
    }

    tixControl:AdjustValue $w $amount

    if {$data(-autorepeat)} {
	after $data(-initwait) tixControl:Repeat $w $amount $serial
    }

    focus $data(w:entry)
}

proc tixControl:Repeat {w amount serial} {
    if {![winfo exists $w]} {
	return
    }
    upvar #0 $w data

    if {$serial eq $data(serial)} {
	tixControl:AdjustValue $w $amount

	if {$data(-autorepeat)} {
	   after $data(-repeatrate) tixControl:Repeat $w $amount $serial
	}
    }
}

proc tixControl:StopRepeat {w} {
    upvar #0 $w data

    incr data(serial)
}

proc tixControl:Destructor {w} {

    tixVariable:DeleteVariable $w

    # Chain this to the superclass
    #
    tixChainMethod $w Destructor
}

# ToDo: maybe should return -code break if the value is not good ...
#
proc tixControl:Tab {w detail} {
    upvar #0 $w data

    if {![info exists data(edited)]} {
	return
    } else {
	unset data(edited)
    }

    tixControl:invoke $w
}

proc tixControl:Escape {w} {
    upvar #0 $w data

    $data(w:entry) delete 0 end
    $data(w:entry) insert 0 $data(-value)
}

proc tixControl:KeyPress {w} {
    upvar #0 $w data

    if {$data(-selectmode) eq "normal"} {
	set data(edited) 0
	return
    } else {
	# == "immediate"
	after 1 tixControl:invoke $w
    }
}
