# genStubs.tcl --
#
#	This script generates a set of stub files for a given
#	interface.
#
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# Copyright (c) 2007 Daniel A. Steffen <das@users.sourceforge.net>
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

package require Tcl 8.4

namespace eval genStubs {
    # libraryName --
    #
    #	The name of the entire library.  This value is used to compute
    #	the USE_*_STUBS macro and the name of the init file.

    variable libraryName "UNKNOWN"

    # interfaces --
    #
    #	An array indexed by interface name that is used to maintain
    #   the set of valid interfaces.  The value is empty.

    array set interfaces {}

    # curName --
    #
    #	The name of the interface currently being defined.

    variable curName "UNKNOWN"

    # scspec --
    #
    #	Storage class specifier for external function declarations.
    #	Normally "EXTERN", may be set to something like XYZAPI
    #
    variable scspec "EXTERN"

    # epoch, revision --
    #
    #	The epoch and revision numbers of the interface currently being defined.
    #   (@@@TODO: should be an array mapping interface names -> numbers)
    #

    variable epoch {}
    variable revision 0

    # hooks --
    #
    #	An array indexed by interface name that contains the set of
    #	subinterfaces that should be defined for a given interface.

    array set hooks {}

    # stubs --
    #
    #	This three dimensional array is indexed first by interface name,
    #	second by platform name, and third by a numeric offset or the
    #	constant "lastNum".  The lastNum entry contains the largest
    #	numeric offset used for a given interface/platform combo.  Each
    #	numeric offset contains the C function specification that
    #	should be used for the given entry in the stub table.  The spec
    #	consists of a list in the form returned by parseDecl.

    array set stubs {}

    # outDir --
    #
    #	The directory where the generated files should be placed.

    variable outDir .
}

# genStubs::library --
#
#	This function is used in the declarations file to set the name
#	of the library that the interfaces are associated with (e.g. "tcl").
#	This value will be used to define the inline conditional macro.
#
# Arguments:
#	name	The library name.
#
# Results:
#	None.

proc genStubs::library {name} {
    variable libraryName $name
}

# genStubs::interface --
#
#	This function is used in the declarations file to set the name
#	of the interface currently being defined.
#
# Arguments:
#	name	The name of the interface.
#
# Results:
#	None.

proc genStubs::interface {name} {
    variable curName $name
    variable interfaces

    set interfaces($name) {}
    return
}

# genStubs::scspec --
#
#	Define the storage class macro used for external function declarations.
#	Typically, this will be a macro like XYZAPI or EXTERN that
#	expands to either DLLIMPORT or DLLEXPORT, depending on whether
#	-DBUILD_XYZ has been set.
#
proc genStubs::scspec {value} {
    variable scspec $value
}

# genStubs::epoch --
#
#	Define the epoch number for this library.  The epoch
#	should be incrememented when a release is made that
#	contains incompatible changes to the public API.
#
proc genStubs::epoch {value} {
    variable epoch $value
}

# genStubs::hooks --
#
#	This function defines the subinterface hooks for the current
#	interface.
#
# Arguments:
#	names	The ordered list of interfaces that are reachable through the
#		hook vector.
#
# Results:
#	None.

proc genStubs::hooks {names} {
    variable curName
    variable hooks

    set hooks($curName) $names
    return
}

# genStubs::declare --
#
#	This function is used in the declarations file to declare a new
#	interface entry.
#
# Arguments:
#	index		The index number of the interface.
#	platform	The platform the interface belongs to.  Should be one
#			of generic, win, unix, or macosx or aqua or x11.
#	decl		The C function declaration, or {} for an undefined
#			entry.
#
# Results:
#	None.

proc genStubs::declare {args} {
    variable stubs
    variable curName
    variable revision

    incr revision
    if {[llength $args] == 2} {
	lassign $args index decl
	set platformList generic
    } elseif {[llength $args] == 3} {
	lassign $args index platformList decl
    } else {
	puts stderr "wrong # args: declare $args"
	return
    }

    # Check for duplicate declarations, then add the declaration and
    # bump the lastNum counter if necessary.

    foreach platform $platformList {
	if {[info exists stubs($curName,$platform,$index)]} {
	    puts stderr "Duplicate entry: declare $args"
	}
    }
    regsub -all "\[ \t\n\]+" [string trim $decl] " " decl
    set decl [parseDecl $decl]

    foreach platform $platformList {
	if {$decl ne ""} {
	    set stubs($curName,$platform,$index) $decl
	    if {![info exists stubs($curName,$platform,lastNum)] \
		    || ($index > $stubs($curName,$platform,lastNum))} {
		set stubs($curName,$platform,lastNum) $index
	    }
	}
    }
    return
}

# genStubs::export --
#
#	This function is used in the declarations file to declare a symbol
#	that is exported from the library but is not in the stubs table.
#
# Arguments:
#	decl		The C function declaration, or {} for an undefined
#			entry.
#
# Results:
#	None.

proc genStubs::export {args} {
    if {[llength $args] != 1} {
	puts stderr "wrong # args: export $args"
    }
    return
}

# genStubs::rewriteFile --
#
#	This function replaces the machine generated portion of the
#	specified file with new contents.  It looks for the !BEGIN! and
#	!END! comments to determine where to place the new text.
#
# Arguments:
#	file	The name of the file to modify.
#	text	The new text to place in the file.
#
# Results:
#	None.

proc genStubs::rewriteFile {file text} {
    if {![file exists $file]} {
	puts stderr "Cannot find file: $file"
	return
    }
    set in [open ${file} r]
    set out [open ${file}.new w]
    fconfigure $out -translation lf

    while {![eof $in]} {
	set line [gets $in]
	if {[string match "*!BEGIN!*" $line]} {
	    break
	}
	puts $out $line
    }
    puts $out "/* !BEGIN!: Do not edit below this line. */"
    puts $out $text
    while {![eof $in]} {
	set line [gets $in]
	if {[string match "*!END!*" $line]} {
	    break
	}
    }
    puts $out "/* !END!: Do not edit above this line. */"
    puts -nonewline $out [read $in]
    close $in
    close $out
    file rename -force ${file}.new ${file}
    return
}

# genStubs::addPlatformGuard --
#
#	Wrap a string inside a platform #ifdef.
#
# Arguments:
#	plat	Platform to test.
#
# Results:
#	Returns the original text inside an appropriate #ifdef.

proc genStubs::addPlatformGuard {plat iftxt {eltxt {}} {withCygwin 0}} {
    set text ""
    switch $plat {
	win {
	    append text "#if defined(_WIN32)"
	    if {$withCygwin} {
		append text " || defined(__CYGWIN__)"
	    }
	    append text " /* WIN */\n${iftxt}"
	    if {$eltxt ne ""} {
		append text "#else /* WIN */\n${eltxt}"
	    }
	    append text "#endif /* WIN */\n"
	}
	unix {
	    append text "#if !defined(_WIN32)"
	    if {$withCygwin} {
		append text " && !defined(__CYGWIN__)"
	    }
	    append text " && !defined(MAC_OSX_TCL)\
		    /* UNIX */\n${iftxt}"
	    if {$eltxt ne ""} {
		append text "#else /* UNIX */\n${eltxt}"
	    }
	    append text "#endif /* UNIX */\n"
	}
	macosx {
	    append text "#ifdef MAC_OSX_TCL /* MACOSX */\n${iftxt}"
	    if {$eltxt ne ""} {
		append text "#else /* MACOSX */\n${eltxt}"
	    }
	    append text "#endif /* MACOSX */\n"
	}
	aqua {
	    append text "#ifdef MAC_OSX_TK /* AQUA */\n${iftxt}"
	    if {$eltxt ne ""} {
		append text "#else /* AQUA */\n${eltxt}"
	    }
	    append text "#endif /* AQUA */\n"
	}
	x11 {
	    append text "#if !(defined(_WIN32)"
	    if {$withCygwin} {
		append text " || defined(__CYGWIN__)"
	    }
	    append text " || defined(MAC_OSX_TK))\
		    /* X11 */\n${iftxt}"
	    if {$eltxt ne ""} {
		append text "#else /* X11 */\n${eltxt}"
	    }
	    append text "#endif /* X11 */\n"
	}
	default {
	    append text "${iftxt}${eltxt}"
	}
    }
    return $text
}

# genStubs::emitSlots --
#
#	Generate the stub table slots for the given interface.  If there
#	are no generic slots, then one table is generated for each
#	platform, otherwise one table is generated for all platforms.
#
# Arguments:
#	name	The name of the interface being emitted.
#	textVar	The variable to use for output.
#
# Results:
#	None.

proc genStubs::emitSlots {name textVar} {
    upvar $textVar text

    forAllStubs $name makeSlot 1 text {"    void (*reserved$i)(void);\n"}
    return
}

# genStubs::parseDecl --
#
#	Parse a C function declaration into its component parts.
#
# Arguments:
#	decl	The function declaration.
#
# Results:
#	Returns a list of the form {returnType name args}.  The args
#	element consists of a list of type/name pairs, or a single
#	element "void".  If the function declaration is malformed
#	then an error is displayed and the return value is {}.

proc genStubs::parseDecl {decl} {
    if {![regexp {^(.*)\((.*)\)$} $decl all prefix args]} {
	set prefix $decl
	set args {}
    }
    set prefix [string trim $prefix]
    if {![regexp {^(.+[ ][*]*)([^ *]+)$} $prefix all rtype fname]} {
	puts stderr "Bad return type: $decl"
	return
    }
    set rtype [string trim $rtype]
    if {$args eq ""} {
	return [list $rtype $fname {}]
    }
    foreach arg [split $args ,] {
	lappend argList [string trim $arg]
    }
    if {![string compare [lindex $argList end] "..."]} {
	set args TCL_VARARGS
	foreach arg [lrange $argList 0 end-1] {
	    set argInfo [parseArg $arg]
	    if {[llength $argInfo] == 2 || [llength $argInfo] == 3} {
		lappend args $argInfo
	    } else {
		puts stderr "Bad argument: '$arg' in '$decl'"
		return
	    }
	}
    } else {
	set args {}
	foreach arg $argList {
	    set argInfo [parseArg $arg]
	    if {![string compare $argInfo "void"]} {
		lappend args "void"
		break
	    } elseif {[llength $argInfo] == 2 || [llength $argInfo] == 3} {
		lappend args $argInfo
	    } else {
		puts stderr "Bad argument: '$arg' in '$decl'"
		return
	    }
	}
    }
    return [list $rtype $fname $args]
}

# genStubs::parseArg --
#
#	This function parses a function argument into a type and name.
#
# Arguments:
#	arg	The argument to parse.
#
# Results:
#	Returns a list of type and name with an optional third array
#	indicator.  If the argument is malformed, returns "".

proc genStubs::parseArg {arg} {
    if {![regexp {^(.+[ ][*]*)([^][ *]+)(\[\])?$} $arg all type name array]} {
	if {$arg eq "void"} {
	    return $arg
	} else {
	    return
	}
    }
    set result [list [string trim $type] $name]
    if {$array ne ""} {
	lappend result $array
    }
    return $result
}

# genStubs::makeDecl --
#
#	Generate the prototype for a function.
#
# Arguments:
#	name	The interface name.
#	decl	The function declaration.
#	index	The slot index for this function.
#
# Results:
#	Returns the formatted declaration string.

proc genStubs::makeDecl {name decl index} {
    variable scspec
    lassign $decl rtype fname args

    append text "/* $index */\n"
    set line "$scspec $rtype"
    set count [expr {2 - ([string length $line] / 8)}]
    append line [string range "\t\t\t" 0 $count]
    set pad [expr {24 - [string length $line]}]
    if {$pad <= 0} {
	append line " "
	set pad 0
    }
    if {$args eq ""} {
	append line $fname
	append text $line
	append text ";\n"
	return $text
    }
    append line $fname

    set arg1 [lindex $args 0]
    switch -exact $arg1 {
	void {
	    append line "(void)"
	}
	TCL_VARARGS {
	    set sep "("
	    foreach arg [lrange $args 1 end] {
		append line $sep
		set next {}
		append next [lindex $arg 0]
		if {[string index $next end] ne "*"} {
		    append next " "
		}
		append next [lindex $arg 1] [lindex $arg 2]
		if {[string length $line] + [string length $next] \
			+ $pad > 76} {
		    append text [string trimright $line] \n
		    set line "\t\t\t\t"
		    set pad 28
		}
		append line $next
		set sep ", "
	    }
	    append line ", ...)"
	    if {[lindex $args end] eq "{const char *} format"} {
		append line " TCL_FORMAT_PRINTF(" [expr [llength $args] - 1] ", " [llength $args] ")"
	    }
	}
	default {
	    set sep "("
	    foreach arg $args {
		append line $sep
		set next {}
		append next [lindex $arg 0]
		if {[string index $next end] ne "*"} {
		    append next " "
		}
		append next [lindex $arg 1] [lindex $arg 2]
		if {[string length $line] + [string length $next] \
			+ $pad > 76} {
		    append text [string trimright $line] \n
		    set line "\t\t\t\t"
		    set pad 28
		}
		append line $next
		set sep ", "
	    }
	    append line ")"
	}
    }
    return "$text$line;\n"
}

# genStubs::makeMacro --
#
#	Generate the inline macro for a function.
#
# Arguments:
#	name	The interface name.
#	decl	The function declaration.
#	index	The slot index for this function.
#
# Results:
#	Returns the formatted macro definition.

proc genStubs::makeMacro {name decl index} {
    lassign $decl rtype fname args

    set lfname [string tolower [string index $fname 0]]
    append lfname [string range $fname 1 end]

    set text "#define $fname \\\n\t("
    if {$args eq ""} {
	append text "*"
    }
    append text "${name}StubsPtr->$lfname)"
    append text " /* $index */\n"
    return $text
}

# genStubs::makeSlot --
#
#	Generate the stub table entry for a function.
#
# Arguments:
#	name	The interface name.
#	decl	The function declaration.
#	index	The slot index for this function.
#
# Results:
#	Returns the formatted table entry.

proc genStubs::makeSlot {name decl index} {
    lassign $decl rtype fname args

    set lfname [string tolower [string index $fname 0]]
    append lfname [string range $fname 1 end]

    set text "    "
    if {$args eq ""} {
	append text $rtype " *" $lfname "; /* $index */\n"
	return $text
    }
    if {[string range $rtype end-8 end] eq "__stdcall"} {
	append text [string trim [string range $rtype 0 end-9]] " (__stdcall *" $lfname ") "
    } else {
	append text $rtype " (*" $lfname ") "
    }
    set arg1 [lindex $args 0]
    switch -exact $arg1 {
	void {
	    append text "(void)"
	}
	TCL_VARARGS {
	    set sep "("
	    foreach arg [lrange $args 1 end] {
		append text $sep [lindex $arg 0]
		if {[string index $text end] ne "*"} {
		    append text " "
		}
		append text [lindex $arg 1] [lindex $arg 2]
		set sep ", "
	    }
	    append text ", ...)"
	    if {[lindex $args end] eq "{const char *} format"} {
		append text " TCL_FORMAT_PRINTF(" [expr [llength $args] - 1] ", " [llength $args] ")"
	    }
	}
	default {
	    set sep "("
	    foreach arg $args {
		append text $sep [lindex $arg 0]
		if {[string index $text end] ne "*"} {
		    append text " "
		}
		append text [lindex $arg 1] [lindex $arg 2]
		set sep ", "
	    }
	    append text ")"
	}
    }

    append text "; /* $index */\n"
    return $text
}

# genStubs::makeInit --
#
#	Generate the prototype for a function.
#
# Arguments:
#	name	The interface name.
#	decl	The function declaration.
#	index	The slot index for this function.
#
# Results:
#	Returns the formatted declaration string.

proc genStubs::makeInit {name decl index} {
    if {[lindex $decl 2] eq ""} {
	append text "    &" [lindex $decl 1] ", /* " $index " */\n"
    } else {
	append text "    " [lindex $decl 1] ", /* " $index " */\n"
    }
    return $text
}

# genStubs::forAllStubs --
#
#	This function iterates over all of the platforms and invokes
#	a callback for each slot.  The result of the callback is then
#	placed inside appropriate platform guards.
#
# Arguments:
#	name		The interface name.
#	slotProc	The proc to invoke to handle the slot.  It will
#			have the interface name, the declaration,  and
#			the index appended.
#	onAll		If 1, emit the skip string even if there are
#			definitions for one or more platforms.
#	textVar		The variable to use for output.
#	skipString	The string to emit if a slot is skipped.  This
#			string will be subst'ed in the loop so "$i" can
#			be used to substitute the index value.
#
# Results:
#	None.

proc genStubs::forAllStubs {name slotProc onAll textVar
	{skipString {"/* Slot $i is reserved */\n"}}} {
    variable stubs
    upvar $textVar text

    set plats [array names stubs $name,*,lastNum]
    if {[info exists stubs($name,generic,lastNum)]} {
	# Emit integrated stubs block
	set lastNum -1
	foreach plat [array names stubs $name,*,lastNum] {
	    if {$stubs($plat) > $lastNum} {
		set lastNum $stubs($plat)
	    }
	}
	for {set i 0} {$i <= $lastNum} {incr i} {
	    set slots [array names stubs $name,*,$i]
	    set emit 0
	    if {[info exists stubs($name,generic,$i)]} {
		if {[llength $slots] > 1} {
		    puts stderr "conflicting generic and platform entries:\
			    $name $i"
		}
		append text [$slotProc $name $stubs($name,generic,$i) $i]
		set emit 1
	    } elseif {[llength $slots] > 0} {
		array set slot {unix 0 x11 0 win 0 macosx 0 aqua 0}
		foreach s $slots {
		    set slot([lindex [split $s ,] 1]) 1
		}
		# "aqua", "macosx" and "x11" are special cases:
		# "macosx" implies "unix", "aqua" implies "macosx" and "x11"
		# implies "unix", so we need to be careful not to emit
		# duplicate stubs entries:
		if {($slot(unix) && $slot(macosx)) || (
			($slot(unix) || $slot(macosx)) &&
			($slot(x11)  || $slot(aqua)))} {
		    puts stderr "conflicting platform entries: $name $i"
		}
		## unix ##
		set temp {}
		set plat unix
		if {!$slot(aqua) && !$slot(x11)} {
		    if {$slot($plat)} {
			append temp [$slotProc $name $stubs($name,$plat,$i) $i]
		    } elseif {$onAll} {
			eval {append temp} $skipString
		    }
		}
		if {$temp ne ""} {
		    append text [addPlatformGuard $plat $temp]
		    set emit 1
		}
		## x11 ##
		set temp {}
		set plat x11
		if {!$slot(unix) && !$slot(macosx)} {
		    if {$slot($plat)} {
			append temp [$slotProc $name $stubs($name,$plat,$i) $i]
		    } elseif {$onAll} {
			eval {append temp} $skipString
		    }
		}
		if {$temp ne ""} {
		    append text [addPlatformGuard $plat $temp]
		    set emit 1
		}
		## win ##
		set temp {}
		set plat win
		if {$slot($plat)} {
		    append temp [$slotProc $name $stubs($name,$plat,$i) $i]
		} elseif {$onAll} {
		    eval {append temp} $skipString
		}
		if {$temp ne ""} {
		    append text [addPlatformGuard $plat $temp]
		    set emit 1
		}
		## macosx ##
		set temp {}
		set plat macosx
		if {!$slot(aqua) && !$slot(x11)} {
		    if {$slot($plat)} {
			append temp [$slotProc $name $stubs($name,$plat,$i) $i]
		    } elseif {$slot(unix)} {
			append temp [$slotProc $name $stubs($name,unix,$i) $i]
		    } elseif {$onAll} {
			eval {append temp} $skipString
		    }
		}
		if {$temp ne ""} {
		    append text [addPlatformGuard $plat $temp]
		    set emit 1
		}
		## aqua ##
		set temp {}
		set plat aqua
		if {!$slot(unix) && !$slot(macosx)} {
		    if {[string range $skipString 1 2] ne "/*"} {
			# genStubs.tcl previously had a bug here causing it to
			# erroneously generate both a unix entry and an aqua
			# entry for a given stubs table slot. To preserve
			# backwards compatibility, generate a dummy stubs entry
			# before every aqua entry (note that this breaks the
			# correspondence between emitted entry number and
			# actual position of the entry in the stubs table, e.g.
			# TkIntStubs entry 113 for aqua is in fact at position
			# 114 in the table, entry 114 at position 116 etc).
			eval {append temp} $skipString
			set temp "[string range $temp 0 end-1] /*\
				Dummy entry for stubs table backwards\
				compatibility */\n"
		    }
		    if {$slot($plat)} {
			append temp [$slotProc $name $stubs($name,$plat,$i) $i]
		    } elseif {$onAll} {
			eval {append temp} $skipString
		    }
		}
		if {$temp ne ""} {
		    append text [addPlatformGuard $plat $temp]
		    set emit 1
		}
	    }
	    if {!$emit} {
		eval {append text} $skipString
	    }
	}
    } else {
	# Emit separate stubs blocks per platform
	array set block {unix 0 x11 0 win 0 macosx 0 aqua 0}
	foreach s [array names stubs $name,*,lastNum] {
	    set block([lindex [split $s ,] 1]) 1
	}
	## unix ##
	if {$block(unix) && !$block(x11)} {
	    set temp {}
	    set plat unix
	    set lastNum $stubs($name,$plat,lastNum)
	    for {set i 0} {$i <= $lastNum} {incr i} {
		if {[info exists stubs($name,$plat,$i)]} {
		    append temp [$slotProc $name $stubs($name,$plat,$i) $i]
		} else {
		    eval {append temp} $skipString
		}
	    }
	    append text [addPlatformGuard $plat $temp {} true]
	}
	## win ##
	if {$block(win)} {
	    set temp {}
	    set plat win
	    set lastNum $stubs($name,$plat,lastNum)
	    for {set i 0} {$i <= $lastNum} {incr i} {
		if {[info exists stubs($name,$plat,$i)]} {
		    append temp [$slotProc $name $stubs($name,$plat,$i) $i]
		} else {
		    eval {append temp} $skipString
		}
	    }
	    append text [addPlatformGuard $plat $temp {} true]
	}
	## macosx ##
	if {($block(unix) || $block(macosx)) && !$block(aqua) && !$block(x11)} {
	    set temp {}
	    set lastNum -1
	    foreach plat {unix macosx} {
		if {$block($plat)} {
		    set lastNum [expr {$lastNum > $stubs($name,$plat,lastNum)
			    ? $lastNum : $stubs($name,$plat,lastNum)}]
		}
	    }
	    for {set i 0} {$i <= $lastNum} {incr i} {
		set emit 0
		foreach plat {unix macosx} {
		    if {[info exists stubs($name,$plat,$i)]} {
			append temp [$slotProc $name $stubs($name,$plat,$i) $i]
			set emit 1
			break
		    }
		}
		if {!$emit} {
		    eval {append temp} $skipString
		}
	    }
	    append text [addPlatformGuard macosx $temp]
	}
	## aqua ##
	if {$block(aqua)} {
	    set temp {}
	    set lastNum -1
	    foreach plat {unix macosx aqua} {
		if {$block($plat)} {
		    set lastNum [expr {$lastNum > $stubs($name,$plat,lastNum)
			    ? $lastNum : $stubs($name,$plat,lastNum)}]
		}
	    }
	    for {set i 0} {$i <= $lastNum} {incr i} {
		set emit 0
		foreach plat {unix macosx aqua} {
		    if {[info exists stubs($name,$plat,$i)]} {
			append temp [$slotProc $name $stubs($name,$plat,$i) $i]
			set emit 1
			break
		    }
		}
		if {!$emit} {
		    eval {append temp} $skipString
		}
	    }
	    append text [addPlatformGuard aqua $temp]
	}
	## x11 ##
	if {$block(x11)} {
	    set temp {}
	    set lastNum -1
	    foreach plat {unix macosx x11} {
		if {$block($plat)} {
		    set lastNum [expr {$lastNum > $stubs($name,$plat,lastNum)
			    ? $lastNum : $stubs($name,$plat,lastNum)}]
		}
	    }
	    for {set i 0} {$i <= $lastNum} {incr i} {
		set emit 0
		foreach plat {unix macosx x11} {
		    if {[info exists stubs($name,$plat,$i)]} {
			if {$plat ne "macosx"} {
			    append temp [$slotProc $name \
				    $stubs($name,$plat,$i) $i]
			} else {
			    eval {set etxt} $skipString
			    append temp [addPlatformGuard $plat [$slotProc \
				    $name $stubs($name,$plat,$i) $i] $etxt true]
			}
			set emit 1
			break
		    }
		}
		if {!$emit} {
		    eval {append temp} $skipString
		}
	    }
	    append text [addPlatformGuard x11 $temp {} true]
	}
    }
}

# genStubs::emitDeclarations --
#
#	This function emits the function declarations for this interface.
#
# Arguments:
#	name	The interface name.
#	textVar	The variable to use for output.
#
# Results:
#	None.

proc genStubs::emitDeclarations {name textVar} {
    upvar $textVar text

    append text "\n/*\n * Exported function declarations:\n */\n\n"
    forAllStubs $name makeDecl 0 text
    return
}

# genStubs::emitMacros --
#
#	This function emits the inline macros for an interface.
#
# Arguments:
#	name	The name of the interface being emitted.
#	textVar	The variable to use for output.
#
# Results:
#	None.

proc genStubs::emitMacros {name textVar} {
    variable libraryName
    upvar $textVar text

    set upName [string toupper $libraryName]
    append text "\n#if defined(USE_${upName}_STUBS)\n"
    append text "\n/*\n * Inline function declarations:\n */\n\n"

    forAllStubs $name makeMacro 0 text

    append text "\n#endif /* defined(USE_${upName}_STUBS) */\n"
    return
}

# genStubs::emitHeader --
#
#	This function emits the body of the <name>Decls.h file for
#	the specified interface.
#
# Arguments:
#	name	The name of the interface being emitted.
#
# Results:
#	None.

proc genStubs::emitHeader {name} {
    variable outDir
    variable hooks
    variable epoch
    variable revision

    set capName [string toupper [string index $name 0]]
    append capName [string range $name 1 end]

    if {$epoch ne ""} {
	set CAPName [string toupper $name]
	append text "\n"
	append text "#define ${CAPName}_STUBS_EPOCH $epoch\n"
	append text "#define ${CAPName}_STUBS_REVISION $revision\n"
    }

    append text "\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n"

    emitDeclarations $name text

    if {[info exists hooks($name)]} {
	append text "\ntypedef struct {\n"
	foreach hook $hooks($name) {
	    set capHook [string toupper [string index $hook 0]]
	    append capHook [string range $hook 1 end]
	    append text "    const struct ${capHook}Stubs *${hook}Stubs;\n"
	}
	append text "} ${capName}StubHooks;\n"
    }
    append text "\ntypedef struct ${capName}Stubs {\n"
    append text "    int magic;\n"
    if {$epoch ne ""} {
	append text "    int epoch;\n"
	append text "    int revision;\n"
    }
    if {[info exists hooks($name)]} {
	append text "    const ${capName}StubHooks *hooks;\n\n"
    } else {
	append text "    void *hooks;\n\n"
    }

    emitSlots $name text

    append text "} ${capName}Stubs;\n\n"

    append text "extern const ${capName}Stubs *${name}StubsPtr;\n\n"
    append text "#ifdef __cplusplus\n}\n#endif\n"

    emitMacros $name text

    rewriteFile [file join $outDir ${name}Decls.h] $text
    return
}

# genStubs::emitInit --
#
#	Generate the table initializers for an interface.
#
# Arguments:
#	name		The name of the interface to initialize.
#	textVar		The variable to use for output.
#
# Results:
#	Returns the formatted output.

proc genStubs::emitInit {name textVar} {
    variable hooks
    variable interfaces
    variable epoch
    upvar $textVar text
    set root 1

    set capName [string toupper [string index $name 0]]
    append capName [string range $name 1 end]

    if {[info exists hooks($name)]} {
	append text "\nstatic const ${capName}StubHooks ${name}StubHooks = \{\n"
	set sep "    "
	foreach sub $hooks($name) {
	    append text $sep "&${sub}Stubs"
	    set sep ",\n    "
	}
	append text "\n\};\n"
    }
    foreach intf [array names interfaces] {
	if {[info exists hooks($intf)]} {
	    if {[lsearch -exact $hooks($intf) $name] >= 0} {
		set root 0
		break
	    }
	}
    }

    append text "\n"
    if {!$root} {
	append text "static "
    }
    append text "const ${capName}Stubs ${name}Stubs = \{\n    TCL_STUB_MAGIC,\n"
    if {$epoch ne ""} {
	set CAPName [string toupper $name]
	append text "    ${CAPName}_STUBS_EPOCH,\n"
	append text "    ${CAPName}_STUBS_REVISION,\n"
    }
    if {[info exists hooks($name)]} {
	append text "    &${name}StubHooks,\n"
    } else {
	append text "    0,\n"
    }

    forAllStubs $name makeInit 1 text {"    0, /* $i */\n"}

    append text "\};\n"
    return
}

# genStubs::emitInits --
#
#	This function emits the body of the <name>StubInit.c file for
#	the specified interface.
#
# Arguments:
#	name	The name of the interface being emitted.
#
# Results:
#	None.

proc genStubs::emitInits {} {
    variable hooks
    variable outDir
    variable libraryName
    variable interfaces

    # Assuming that dependencies only go one level deep, we need to emit
    # all of the leaves first to avoid needing forward declarations.

    set leaves {}
    set roots {}
    foreach name [lsort [array names interfaces]] {
	if {[info exists hooks($name)]} {
	    lappend roots $name
	} else {
	    lappend leaves $name
	}
    }
    foreach name $leaves {
	emitInit $name text
    }
    foreach name $roots {
	emitInit $name text
    }

    rewriteFile [file join $outDir ${libraryName}StubInit.c] $text
}

# genStubs::init --
#
#	This is the main entry point.
#
# Arguments:
#	None.
#
# Results:
#	None.

proc genStubs::init {} {
    global argv argv0
    variable outDir
    variable interfaces

    if {[llength $argv] < 2} {
	puts stderr "usage: $argv0 outDir declFile ?declFile...?"
	exit 1
    }

    set outDir [lindex $argv 0]

    foreach file [lrange $argv 1 end] {
	source $file
    }

    foreach name [lsort [array names interfaces]] {
	puts "Emitting $name"
	emitHeader $name
    }

    emitInits
}

# lassign --
#
#	This function emulates the TclX lassign command.
#
# Arguments:
#	valueList	A list containing the values to be assigned.
#	args		The list of variables to be assigned.
#
# Results:
#	Returns any values that were not assigned to variables.

if {[string length [namespace which lassign]] == 0} {
    proc lassign {valueList args} {
	if {[llength $args] == 0} {
	    error "wrong # args: should be \"lassign list varName ?varName ...?\""
	}
	uplevel [list foreach $args $valueList {break}]
	return [lrange $valueList [llength $args] end]
    }
}

genStubs::init
