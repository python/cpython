# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Primitiv.tcl,v 1.7 2004/03/28 02:44:57 hobbs Exp $
#
# Primitiv.tcl --
#
#	This is the primitive widget. It is just a frame with proper
#	inheritance wrapping. All new Tix widgets will be derived from
#	this widget
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


# No superclass, so the superclass switch is not used
#
#
tixWidgetClass tixPrimitive {
    -virtual true
    -superclass {}
    -classname  TixPrimitive
    -method {
	cget configure subwidget subwidgets
    }
    -flag {
	-background -borderwidth -cursor
	-height -highlightbackground -highlightcolor -highlightthickness
	-options -relief -takefocus -width -bd -bg
    }
    -static {
	-options
    }
    -configspec {
	{-background background Background #d9d9d9}
	{-borderwidth borderWidth BorderWidth 0}
	{-cursor cursor Cursor ""}
	{-height height Height 0}
	{-highlightbackground highlightBackground HighlightBackground #c3c3c3}
	{-highlightcolor highlightColor HighlightColor black}
	{-highlightthickness highlightThickness HighlightThickness 0}
	{-options options Options ""}
	{-relief relief Relief flat}
	{-takefocus takeFocus TakeFocus 0 tixVerifyBoolean}
	{-width width Width 0}
    }
    -alias {
	{-bd -borderwidth}
	{-bg -background}
    }
}

#----------------------------------------------------------------------
# ClassInitialization:
#----------------------------------------------------------------------

# not used
# Implemented in C
#
# Override: never
proc tixPrimitive:Constructor {w args} {

    upvar #0 $w data
    upvar #0 $data(className) classRec

    # Set up some minimal items in the class record.
    #
    set data(w:root)  $w
    set data(rootCmd) $w:root

    # We need to create the root widget in order to parse the options
    # database
    tixCallMethod $w CreateRootWidget

    # Parse the default options from the options database
    #
    tixPrimitive:ParseDefaultOptions $w

    # Parse the options supplied by the user
    #
    tixPrimitive:ParseUserOptions $w $args

    # Rename the widget command so that it can be use to access
    # the methods of this class

    tixPrimitive:MkWidgetCmd $w

    # Inistalize the Widget Record
    #
    tixCallMethod $w InitWidgetRec

    # Construct the compound widget
    #
    tixCallMethod $w ConstructWidget

    # Do the bindings
    #
    tixCallMethod $w SetBindings

    # Call the configuration methods for all "force call" options
    #
    foreach option $classRec(forceCall) {
	tixInt_ChangeOptions $w $option $data($option)
    }
}


# Create only the root widget. We need the root widget to query the option
# database.
#
# Override: seldom. (unless you want to use a toplevel as root widget)
# Chain   : never.

proc tixPrimitive:CreateRootWidget {w args} {
    upvar #0 $w data
    upvar #0 $data(className) classRec

    frame $w -class $data(ClassName)
}

proc tixPrimitive:ParseDefaultOptions {w} {
    upvar #0 $w data
    upvar #0 $data(className) classRec

    # SET UP THE INSTANCE RECORD ACCORDING TO DEFAULT VALUES IN
    # THE OPTIONS DATABASE
    #
    foreach option $classRec(options) {
	set spec [tixInt_GetOptionSpec $data(className) $option]

	if {[lindex $spec 0] eq "="} {
	    continue
	}

	set o_name    [lindex $spec 1]
	set o_class   [lindex $spec 2]
	set o_default [lindex $spec 3]

	if {![catch {option get $w $o_name $o_class} db_default]} {
	    if {$db_default ne ""} {
		set data($option) $db_default
	    } else {
		set data($option) $o_default
	    }
	} else {
	    set data($option) $o_default
	}
    }
}

proc tixPrimitive:ParseUserOptions {w arglist} {
    upvar #0 $w data
    upvar #0 $data(className) classRec

    # SET UP THE INSTANCE RECORD ACCORDING TO COMMAND ARGUMENTS FROM
    # THE USER OF THE TIX LIBRARY (i.e. Application programmer:)
    #
    foreach {option arg} $arglist {
	if {[lsearch $classRec(options) $option] != "-1"} {
	    set spec [tixInt_GetOptionSpec $data(className) $option]

	    if {[lindex $spec 0] ne "="} {
		set data($option) $arg
	    } else {
		set realOption [lindex $spec 1]
		set data($realOption) $arg
	    }
	} else {
	    error "unknown option $option. Should be: [tixInt_ListOptions $w]"
	}
    }
}

#----------------------------------------------------------------------
# Initialize the widget record
# 
#
# Override: always
# Chain   : always, before
proc tixPrimitive:InitWidgetRec {w} {
    # default: do nothing
}

#----------------------------------------------------------------------
# SetBindings
# 
#
# Override: sometimes
# Chain   : sometimes, before
#
bind TixDestroyHandler <Destroy> {
    [tixGetMethod %W [set %W(className)] Destructor] %W
}

proc tixPrimitive:SetBindings {w} {
    upvar #0 $w data

    if {[winfo toplevel $w] eq $w} {
	bindtags $w [concat TixDestroyHandler [bindtags $w]]
    } else {
	bind $data(w:root) <Destroy> \
	    "[tixGetMethod $w $data(className) Destructor] $w"
    }
}

#----------------------------------------------------------------------
# PrivateMethod: ConstructWidget
# 
# Construct and set up the compound widget
#
# Override: sometimes
# Chain   : sometimes, before
#
proc tixPrimitive:ConstructWidget {w} {
    upvar #0 $w data

    $data(rootCmd) config \
	-background  $data(-background) \
	-borderwidth $data(-borderwidth) \
	-cursor      $data(-cursor) \
	-relief      $data(-relief)

    if {$data(-width) != 0} {
	$data(rootCmd) config -width $data(-width)
    }
    if {$data(-height) != 0} {
	$data(rootCmd) config -height $data(-height)
    }

    set rootname *[string range $w 1 end]

    foreach {spec value} $data(-options) {
	option add $rootname*$spec $value 100
    }
}

#----------------------------------------------------------------------
# PrivateMethod: MkWidgetCmd
# 
# Construct and set up the compound widget
#
# Override: sometimes
# Chain   : sometimes, before
#
proc tixPrimitive:MkWidgetCmd {w} {
    upvar #0 $w data

    rename $w $data(rootCmd)
    tixInt_MkInstanceCmd $w
}


#----------------------------------------------------------------------
# ConfigOptions:
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# ConfigMethod: config
#
# Configure one option.
# 
# Override: always
# Chain   : automatic.
#
# Note the hack of [winfo width] in this procedure
#
# The hack is necessary because of the bad interaction between TK's geometry
# manager (the packer) and the frame widget. The packer determines the size
# of the root widget of the ComboBox (a frame widget) according to the
# requirement of the slaves inside the frame widget, NOT the -width
# option of the frame widget.
#
# However, everytime the frame widget is
# configured, it sends a geometry request to the packer according to its
# -width and -height options and the packer will temporarily resize
# the frame widget according to the requested size! The packer then realizes
# something is wrong and revert to the size determined by the slaves. This
# cause a flash on the screen.
#
foreach opt {-height -width -background -borderwidth -cursor
        -highlightbackground -highlightcolor -relief -takefocus -bd -bg} {

    set tixPrimOpt($opt) 1
}

proc tixPrimitive:config {w option value} {
    global tixPrimOpt
    upvar #0 $w data

    if {[info exists tixPrimOpt($option)]} {
	$data(rootCmd) config $option $value
    }
}

#----------------------------------------------------------------------
# PublicMethods:
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# This method is used to implement the "subwidgets" widget command.
# Will be re-written in C. It can't be used as a public method because
# of the lame substring comparison routines used in tixClass.c
#
#
proc tixPrimitive:subwidgets {w type args} {
    upvar #0 $w data

    case $type {
	-class {
	    set name [lindex $args 0]
	    set args [lrange $args 1 end]
	    # access subwidgets of a particular class
	    #
	    # note: if $name=="Frame", will *not return the root widget as well
	    #
	    set sub ""
	    foreach des [tixDescendants $w] {
		if {[winfo class $des] eq $name} {
		    lappend sub $des
		}
	    }

	    # Note: if the there is no subwidget of this class, does not
	    # cause any error.
	    #
	    if {$args eq ""} {
		return $sub
	    } else {
		foreach des $sub {
		    eval [linsert $args 0 $des]
		}
		return ""
	    }
	}
	-group {
	    set name [lindex $args 0]
	    set args [lrange $args 1 end]
	    # access subwidgets of a particular group
	    #
	    if {[info exists data(g:$name)]} {
		if {$args eq ""} {
		    set ret ""
		    foreach item $data(g:$name) {
			lappend ret $w.$item
		    }
		    return $ret
		} else {
		    foreach item $data(g:$name) {
			eval [linsert $args 0 $w.$item]
		    }
		    return ""
		}
	    } else {
		error "no such subwidget group $name"
	    }
	}
	-all {
	    set sub [tixDescendants $w]

	    if {$args eq ""} {
		return $sub
	    } else {
		foreach des $sub {
		    eval [linsert $args 0 $des]
		}
		return ""
	    }
	}
	default {
	    error "unknown flag $type, should be -all, -class or -group"
	}
    }
}

#----------------------------------------------------------------------
# PublicMethod: subwidget
#
# Access a subwidget withe a particular name 
#
# Override: never
# Chain   : never
#
# This is implemented in native C code in tixClass.c
#
proc tixPrimitive:subwidget {w name args} {
    upvar #0 $w data

    if {[info exists data(w:$name)]} {
	if {$args eq ""} {
	    return $data(w:$name)
	} else {
	    return [eval [linsert $args 0 $data(w:$name)]]
	}
    } else {
	error "no such subwidget $name"
    }
}


#----------------------------------------------------------------------
# PrivateMethods:
#----------------------------------------------------------------------

# delete the widget record and remove the command
#
proc tixPrimitive:Destructor {w} {
    upvar #0 $w data

    if {![info exists data(w:root)]} {
	return
    }

    if {[llength [info commands $w]]} {
	# remove the command
	rename $w ""
    }

    if {[llength [info commands $data(rootCmd)]]} {
	# remove the command of the root widget
	rename $data(rootCmd) ""
    }

    # delete the widget record
    catch {unset data}
}
