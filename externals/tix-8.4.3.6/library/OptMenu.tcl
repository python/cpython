# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: OptMenu.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# OptMenu.tcl --
#
#	This file implements the TixOptionMenu widget.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


tixWidgetClass tixOptionMenu {
    -classname TixOptionMenu
    -superclass tixLabelWidget
    -method {
	add delete disable enable entrycget entryconfigure entries
    }
    -flag {
	-command -disablecallback -dynamicgeometry -value -variable
	-validatecmd -state
    }
    -forcecall {
	-variable -state
    }
    -configspec {
	{-command command Command ""}
	{-disablecallback disableCallback DisableCallback 0 tixVerifyBoolean}
	{-dynamicgeometry dynamicGeometry DynamicGeometry 0 tixVerifyBoolean}
	{-state state State normal}
	{-value value Value ""}
	{-validatecmd validateCmd ValidateCmd ""}
	{-variable variable Variable ""}
    }
    -default {
	{.highlightThickness			0}
	{.takeFocus				0}
	{.frame.menubutton.relief		raised}
	{.frame.menubutton.borderWidth		2}
	{.frame.menubutton.anchor		w}
	{.frame.menubutton.highlightThickness	2}
	{.frame.menubutton.takeFocus		1}
    }
}

proc tixOptionMenu:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec
 
    set data(nItems)	0
    set data(items)     ""
    set data(posted)	0
    set data(varInited)	0
    set data(maxWidth)  0
}

proc tixOptionMenu:ConstructFramedWidget {w frame} {
    upvar #0 $w data

    tixChainMethod $w ConstructFramedWidget $frame

    set data(w:menubutton) [menubutton $frame.menubutton -indicatoron 1]
    set data(w:menu)       [menu $frame.menubutton.menu -tearoff 0]
    pack $data(w:menubutton) -side left -expand yes -fill both

    $data(w:menubutton) config -menu $data(w:menu)

    bind $data(w:menubutton) <Up>   [bind Menubutton <space>]
    bind $data(w:menubutton) <Down> [bind Menubutton <space>]

    tixSetMegaWidget $data(w:menubutton) $w
}

proc tixOptionMenu:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings
}

#----------------------------------------------------------------------
# Private methods
#----------------------------------------------------------------------
proc tixOptionMenu:Invoke {w name} {
    upvar #0 $w data

    if {"$data(-state)" == "normal"} {
	tixOptionMenu:SetValue $w $name
    }
}

proc tixOptionMenu:SetValue {w value {noUpdate 0}} {
    upvar #0 $w data

    if {$data(-validatecmd) != ""} {
	set value [tixEvalCmdBinding $w $data(-validatecmd) "" $value]
    }

    set name $value

    if {$name == "" || [info exists data(varInited)]} {
	# variable may contain a bogus value
	if {![info exists data($name,index)]} {
	    set data(-value) ""
	    tixVariable:UpdateVariable $w
	    $data(w:menubutton) config -text ""
	    return
	}
    }

    if {[info exists data($name,index)]} {
       $data(w:menubutton) config -text $data($name,label)

       set data(-value) $value

       if {! $noUpdate} {
	   tixVariable:UpdateVariable $w
       }

       if {$data(-command) != "" && !$data(-disablecallback)} {
	   if {![info exists data(varInited)]} {
	       set bind(specs) ""
	       tixEvalCmdBinding $w $data(-command) bind $value
	   }
       }
    } else {
	error "item \"$value\" does not exist"
    }
}

proc tixOptionMenu:SetMaxWidth {w} {
    upvar #0 $w data

    foreach name $data(items) {
	set len [string length $data($name,label)]
	if {$data(maxWidth) < $len} {
	    set data(maxWidth) $len
	}
    }

    if {$data(maxWidth) > 0} {
	$data(w:menubutton) config -width $data(maxWidth)
    }
}

#----------------------------------------------------------------------
# Configuration
#----------------------------------------------------------------------
proc tixOptionMenu:config-state {w value} {
    upvar #0 $w data

    if {![info exists data(w:label)]} {
	return
    }

    if {$value == "normal"} {
	catch {
	    $data(w:label) config -fg \
		[$data(w:menubutton) cget -foreground]
	}
	$data(w:menubutton) config -state $value
    } else {
	catch {
	    $data(w:label) config -fg \
		[$data(w:menubutton) cget -disabledforeground]
	}
	$data(w:menubutton) config -state $value
    }
}

proc tixOptionMenu:config-value {w value} {
    upvar #0 $w data

    tixOptionMenu:SetValue $w $value

    # This will tell the Intrinsics: "Please use this value"
    # because "value" might be altered by SetValues
    #
    return $data(-value)
}

proc tixOptionMenu:config-variable {w arg} {
    upvar #0 $w data

    if {[tixVariable:ConfigVariable $w $arg]} {
       # The value of data(-value) is changed if tixVariable:ConfigVariable 
       # returns true
       tixOptionMenu:SetValue $w $data(-value) 1
    }
    catch {
	unset data(varInited)
    }
    set data(-variable) $arg
}

#----------------------------------------------------------------------
# Public Methdos
#----------------------------------------------------------------------
proc tixOptionMenu:add {w type name args} {
    upvar #0 $w data

    if {[info exists data($name,index)]} {
	error "item $name already exists in the option menu $w"
    }

    case $type {
	"command" {
	    set validOptions {
		-command -label
	    }
	    set opt(-command)		""
	    set opt(-label)		$name

	    tixHandleOptions -nounknown opt $validOptions $args

	    if {$opt(-command)	!= ""} {
		error "option -command cannot be specified"
	    }

	    # Create a new item inside the menu
	    #
	    eval $data(w:menu) add command $args \
		[list -label $opt(-label) \
		-command "tixOptionMenu:Invoke $w \{$name\}"]
	    set index $data(nItems)

	    # Store info about this item
	    #
	    set data($index,name) $name
	    set data($name,type) cmd
	    set data($name,label) $opt(-label)
	    set data($name,index) $index

	    if {$index == 0} {
		$data(w:menubutton) config -text $data($name,label)
		tixOptionMenu:SetValue $w $name
	    }

	    incr data(nItems)
	    lappend data(items) $name

	    if $data(-dynamicgeometry) {
		tixOptionMenu:SetMaxWidth $w
	    }
	}
	"separator" {
	    $data(w:menu) add separator

	    set index $data(nItems)
	    # Store info about this item
	    #
	    set data($index,name) $name
	    set data($name,type) sep
	    set data($name,label) ""
	    set data($name,index) $index

	    incr data(nItems)
	    lappend data(items) $name
	}
	default {
	    error "only types \"separator\" and \"command\" are allowed"
	}
    }

    return ""
}

proc tixOptionMenu:delete {w item} {
    upvar #0 $w data

    if {![info exists data($item,index)]} {
	error "item $item does not exist in $w"
    }

    # Rehash the item list
    set newItems ""
    set oldIndex 0
    set newIndex 0
    foreach name $data(items) {
	if {$item == $name} {
	    unset data($name,label)
	    unset data($name,index)
	    unset data($name,type)
	    $data(w:menu) delete $oldIndex
	} else {
	    set data($name,index)    $newIndex
	    set data($newIndex,name) $name
	    incr newIndex
	    lappend newItems $name
	}
	incr oldIndex
    }
    incr oldIndex -1; unset data($oldIndex,name)
    set data(nItems) $newIndex
    set data(items) $newItems

    if {$data(-value) == $item} {
	set newVal ""
	foreach item $data(items) {
	    if {$data($item,type) == "cmd"} {
		set newVal $item
	    }
	}
	tixOptionMenu:SetValue $w $newVal
    }

    return ""
}


proc tixOptionMenu:disable {w item} {
    upvar #0 $w data

    if {![info exists data($item,index)]} {
	error "item $item does not exist in $w"
    } else {
	catch {$data(w:menu) entryconfig $data($item,index) -state disabled}
    }
}

proc tixOptionMenu:enable {w item} {
    upvar #0 $w data

    if {![info exists data($item,index)]} {
	error "item $item does not exist in $w"
    } else {
	catch {$data(w:menu) entryconfig $data($item,index) -state normal}
    }
}

proc tixOptionMenu:entryconfigure {w item args} {
    upvar #0 $w data

    if {![info exists data($item,index)]} {
	error "item $item does not exist in $w"
    } else {
	return [eval $data(w:menu) entryconfig $data($item,index) $args]
    }
}

proc tixOptionMenu:entrycget {w item arg} {
    upvar #0 $w data

    if {![info exists data($item,index)]} {
	error "item $item does not exist in $w"
    } else {
	return [$data(w:menu) entrycget $data($item,index) $arg]
    }
}

proc tixOptionMenu:entries {w} {
    upvar #0 $w data

    return $data(items)
}


proc tixOptionMenu:Destructor {w} {

    tixVariable:DeleteVariable $w

    # Chain this to the superclass
    #
    tixChainMethod $w Destructor
}

#----------------------------------------------------------------------
# Obsolete
# These have been replaced by new commands in Tk 4.0
#
proc tixOptionMenu:Post {w} {
    upvar #0 $w data

    set rootx [winfo rootx $data(w:frame)]
    set rooty [winfo rooty $data(w:frame)]

    # adjust for the border of the menu and frame
    #
    incr rootx [lindex [$data(w:menu)  config -border] 4]
    incr rooty [lindex [$data(w:frame) config -border] 4]
    incr rooty [lindex [$data(w:menu)  config -border] 4]

    set value $data(-value)
    set y [$data(w:menu) yposition $data($value,index)]

    $data(w:menu) post $rootx [expr $rooty - $y]
    $data(w:menu) activate $data($value,index)
    grab -global $data(w:menubutton)
    set data(posted) 1
}
