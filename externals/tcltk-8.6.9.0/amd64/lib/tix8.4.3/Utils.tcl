# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Utils.tcl,v 1.4 2004/03/28 02:44:57 hobbs Exp $
#
# Util.tcl --
#
#	The Tix utility commands. Some of these commands are
#	replacement of or extensions to the existing TK
#	commands. Occasionaly, you have to use the commands inside
#	this file instead of thestandard TK commands to make your
#	applicatiion work better with Tix. Please read the
#	documentations (programmer's guide, man pages) for information
#	about these utility commands.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


#
# kludge: should be able to handle all kinds of flags
#         now only handles "-flag value" pairs.
#
proc tixHandleArgv {p_argv p_options validFlags} {
    upvar $p_options opt
    upvar $p_argv    argv

    set old_argv $argv
    set argv ""

    foreac {flag value} $old_argv {
	if {[lsearch $validFlags $flag] != -1} {
	    # The caller will handle this option exclusively
	    # It won't be added back to the original arglist
	    #
	    eval $opt($flag,action) $value
	} else {
	    # The caller does not handle this option
	    #
	    lappend argv $flag
	    lappend argv $value
	}
    }
}

#-----------------------------------------------------------------------
# tixDisableAll -
#
# 	Disable all members in a sub widget tree
#
proc tixDisableAll {w} {
    foreach x [tixDescendants $w] {
	catch {$x config -state disabled}
    }
}

#----------------------------------------------------------------------
# tixEnableAll -
#
# 	enable all members in a sub widget tree
#
proc tixEnableAll {w} {
    foreach x [tixDescendants $w] {
	catch {$x config -state normal}
    }
}

#----------------------------------------------------------------------
# tixDescendants -
#
#	Return a list of all the member of a widget subtree, including
# the tree's root widget.
#
proc tixDescendants {parent} {
    set des ""
    lappend des $parent

    foreach w [winfo children $parent] {
	foreach x [tixDescendants $w] {
	    lappend des $x
	}
    }
    return $des
}

#----------------------------------------------------------------------
# tixTopLevel -
#
#	Create a toplevel widget and unmap it immediately. This will ensure
# that this toplevel widgets will not be popped up prematurely when you
# create Tix widgets inside it.
#
#	"tixTopLevel" also provide options for you to specify the appearance
# and behavior of this toplevel.
#
#
#
proc tixTopLevel {w args} {
    set opt (-geometry) ""
    set opt (-minsize)  ""
    set opt (-maxsize)  ""
    set opt (-width)    ""
    set opt (-height)   ""

    eval [linsert $args 0 toplevel $w]
    wm withdraw $w
}

# This is a big kludge
#
#	Substitutes all [...] and $.. in the string in $args
#
proc tixInt_Expand {args} {
    return $args
}

# Print out all the config options of a widget
#
proc tixPConfig {w} {
    puts [join [lsort [$w config]] \n]
}

proc tixAppendBindTag {w tag} {
    bindtags $w [concat [bindtags $w] $tag]
}

proc tixAddBindTag {w tag} {
    bindtags $w [concat $tag [bindtags $w] ]
}

proc tixSubwidgetRef {sub} {
    return $::tixSRef($sub)
}

proc tixSubwidgetRetCreate {sub ref} {
    set ::tixSRef($sub) $ref
}

proc tixSubwidgetRetDelete {sub} {
    catch {unset ::tixSRef($sub)}
}

proc tixListboxGetCurrent {listbox} {
    return [tixEvent flag V]
}


# tixSetMegaWidget --
#
#	Associate a subwidget with its mega widget "owner". This is mainly
#	used when we add a new bindtag to a subwidget and we need to find out
#	the name of the mega widget inside the binding.
#
proc tixSetMegaWidget {w mega {type any}} {
    set ::tixMega($type,$w) $mega
}

proc tixGetMegaWidget {w {type any}} {
    return $::tixMega($type,$w)
}

proc tixUnsetMegaWidget {w} {
    if {[info exists ::tixMega($w)]} { unset ::tixMega($w) }
}

# tixBusy : display busy cursors on a window
#
#
# Should flush the event queue (but not do any idle tasks) before blocking
# the target window (I am not sure if it is aready doing so )
#
# ToDo: should take some additional windows to raise
#
proc tixBusy {w flag {focuswin ""}} {

    if {[info command tixInputOnly] == ""} {
	return
    }

    global tixBusy
    set toplevel [winfo toplevel $w]

    if {![info exists tixBusy(cursor)]} {
	set tixBusy(cursor) watch
#	set tixBusy(cursor) "[tix getbitmap hourglass] \
#	    [string range [tix getbitmap hourglass.mask] 1 end]\
# 	    black white"
    }

    if {$toplevel eq "."} {
	set inputonly0 .__tix__busy0
	set inputonly1 .__tix__busy1
	set inputonly2 .__tix__busy2
	set inputonly3 .__tix__busy3
    } else {
	set inputonly0 $toplevel.__tix__busy0
	set inputonly1 $toplevel.__tix__busy1
	set inputonly2 $toplevel.__tix__busy2
	set inputonly3 $toplevel.__tix__busy3
    }

    if {![winfo exists $inputonly0]} {
	for {set i 0} {$i < 4} {incr i} {
	    tixInputOnly [set inputonly$i] -cursor $tixBusy(cursor)
	}
    }

    if {$flag eq "on"} {
	if {$focuswin != "" && [winfo id $focuswin] != 0} {
	    if {[info exists tixBusy($focuswin,oldcursor)]} {
		return
	    }
	    set tixBusy($focuswin,oldcursor) [$focuswin cget -cursor]
	    $focuswin config -cursor $tixBusy(cursor)

	    set x1 [expr {[winfo rootx $focuswin]-[winfo rootx $toplevel]}]
	    set y1 [expr {[winfo rooty $focuswin]-[winfo rooty $toplevel]}]

	    set W  [winfo width $focuswin]
	    set H  [winfo height $focuswin]
	    set x2 [expr {$x1 + $W}]
	    set y2 [expr {$y1 + $H}]


	    if {$y1 > 0} {
		tixMoveResizeWindow $inputonly0 0   0   10000 $y1
	    }
	    if {$x1 > 0} {
		tixMoveResizeWindow $inputonly1 0   0   $x1   10000
	    }
	    tixMoveResizeWindow $inputonly2 0   $y2 10000 10000
	    tixMoveResizeWindow $inputonly3 $x2 0   10000 10000

	    for {set i 0} {$i < 4} {incr i} {
		tixMapWindow [set inputonly$i] 
		tixRaiseWindow [set inputonly$i]
	    }
	    tixFlushX $w
	} else {
	    tixMoveResizeWindow $inputonly0 0 0 10000 10000
	    tixMapWindow $inputonly0
	    tixRaiseWindow $inputonly0
	}
    } else {
	tixUnmapWindow $inputonly0
	tixUnmapWindow $inputonly1
	tixUnmapWindow $inputonly2
	tixUnmapWindow $inputonly3

	if {$focuswin != "" && [winfo id $focuswin] != 0} {
	    if {[info exists tixBusy($focuswin,oldcursor)]} {
		$focuswin config -cursor $tixBusy($focuswin,oldcursor)
		if {[info exists tixBusy($focuswin,oldcursor)]} {
		    unset tixBusy($focuswin,oldcursor)
		}
	    }
	}
    }
}

proc tixOptionName {w} {
    return [string range $w 1 end]
}

proc tixSetSilent {chooser value} {
    $chooser config -disablecallback true
    $chooser config -value $value
    $chooser config -disablecallback false
}

# This command is useful if you want to ingore the arguments
# passed by the -command or -browsecmd options of the Tix widgets. E.g
#
# tixFileSelectDialog .c -command "puts foo; tixBreak"
#
#
proc tixBreak {args} {}

#----------------------------------------------------------------------
# tixDestroy -- deletes a Tix class object (not widget classes)
#----------------------------------------------------------------------
proc tixDestroy {w} {
    upvar #0 $w data

    set destructor ""
    if {[info exists data(className)]} {
	catch {
	    set destructor [tixGetMethod $w $data(className) Destructor]
	}
    }
    if {$destructor != ""} {
	$destructor $w
    }
    catch {rename $w ""}
    catch {unset data}
    return ""
}

proc tixPushGrab {args} {
    global tix_priv

    if {![info exists tix_priv(grab-list)]} {
	set tix_priv(grab-list)    ""
	set tix_priv(grab-mode)    ""
	set tix_priv(grab-nopush) ""
    }

    set len [llength $args]
    if {$len == 1} {
	set opt ""
	set w [lindex $args 0]
    } elseif {$len == 2} {
	set opt [lindex $args 0]
	set w [lindex $args 1]
    } else {
	error "wrong # of arguments: tixPushGrab ?-global? window"
    }

    # Not everyone will call tixPushGrab. If someone else has a grab already
    # save that one as well, so that we can restore that later
    #
    set last [lindex $tix_priv(grab-list) end]
    set current [grab current $w]

    if {$current ne "" && $current ne $last} {
	# Someone called "grab" directly
	#
	lappend tix_priv(grab-list)   $current
	lappend tix_priv(grab-mode)   [grab status $current]
	lappend tix_priv(grab-nopush) 1
    }

    # Now push myself into the stack
    #
    lappend tix_priv(grab-list)   $w
    lappend tix_priv(grab-mode)   $opt
    lappend tix_priv(grab-nopush) 0

    if {$opt eq "-global"} {
	grab -global $w
    } else {
	grab $w
    }
}

proc tixPopGrab {} {
    global tix_priv

    if {![info exists tix_priv(grab-list)]} {
	set tix_priv(grab-list)   ""
	set tix_priv(grab-mode)   ""
	set tix_priv(grab-nopush) ""
    }

    set len [llength $tix_priv(grab-list)]
    if {$len <= 0} {
	error "no window is grabbed by tixGrab"
    }

    set w [lindex $tix_priv(grab-list) end]
    grab release $w

    if {$len > 1} {
	set tix_priv(grab-list)   [lrange $tix_priv(grab-list) 0 end-1]
	set tix_priv(grab-mode)   [lrange $tix_priv(grab-mode) 0 end-1]
	set tix_priv(grab-nopush) [lrange $tix_priv(grab-nopush) 0 end-1]

	set w  [lindex $tix_priv(grab-list) end]
	set m  [lindex $tix_priv(grab-list) end]
	set np [lindex $tix_priv(grab-nopush) end]

	if {$np == 1} {
	    # We have a grab set by "grab"
	    #
	    set len [llength $tix_priv(grab-list)]

	    if {$len > 1} {
		set tix_priv(grab-list)   [lrange $tix_priv(grab-list) 0 end-1]
		set tix_priv(grab-mode)   [lrange $tix_priv(grab-mode) 0 end-1]
		set tix_priv(grab-nopush) \
		    [lrange $tix_priv(grab-nopush) 0 end-1]
	    } else {
		set tix_priv(grab-list)   ""
		set tix_priv(grab-mode)   ""
		set tix_priv(grab-nopush) ""
	    }
	}

	if {$m == "-global"} {
	    grab -global $w
	} else {
	    grab $w
	}
    } else {
  	set tix_priv(grab-list)   ""
	set tix_priv(grab-mode)   ""
	set tix_priv(grab-nopush) ""
    }
}

proc tixWithinWindow {wid rootX rootY} {
    set wc  [winfo containing $rootX $rootY]
    if {$wid eq $wc} { return 1 }

    # no see if it is an enclosing parent
    set rx1 [winfo rootx $wid]
    set ry1 [winfo rooty $wid]
    set rw  [winfo width  $wid]
    set rh  [winfo height $wid]
    set rx2 [expr {$rx1+$rw}]
    set ry2 [expr {$ry1+$rh}]

    if {$rootX >= $rx1 && $rootX < $rx2 && $rootY >= $ry1 && $rootY < $ry2} {
	return 1
    } else {
	return 0
    }
}

proc tixWinWidth {w} {
    set W [winfo width $w]
    set bd [expr {[$w cget -bd] + [$w cget -highlightthickness]}]

    return [expr {$W - 2*$bd}]
}

proc tixWinHeight {w} {
    set H [winfo height $w]
    set bd [expr {[$w cget -bd] + [$w cget -highlightthickness]}]

    return [expr {$H - 2*$bd}]
}

# junk?
#
proc tixWinCmd {w} {
    return [winfo command $w]
}
