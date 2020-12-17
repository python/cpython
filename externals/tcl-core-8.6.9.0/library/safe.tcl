# safe.tcl --
#
# This file provide a safe loading/sourcing mechanism for safe interpreters.
# It implements a virtual path mecanism to hide the real pathnames from the
# slave. It runs in a master interpreter and sets up data structure and
# aliases that will be invoked when used from a slave interpreter.
#
# See the safe.n man page for details.
#
# Copyright (c) 1996-1997 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.

#
# The implementation is based on namespaces. These naming conventions are
# followed:
# Private procs starts with uppercase.
# Public  procs are exported and starts with lowercase
#

# Needed utilities package
package require opt 0.4.1

# Create the safe namespace
namespace eval ::safe {
    # Exported API:
    namespace export interpCreate interpInit interpConfigure interpDelete \
	interpAddToAccessPath interpFindInAccessPath setLogCmd
}

# Helper function to resolve the dual way of specifying staticsok (either
# by -noStatics or -statics 0)
proc ::safe::InterpStatics {} {
    foreach v {Args statics noStatics} {
	upvar $v $v
    }
    set flag [::tcl::OptProcArgGiven -noStatics]
    if {$flag && (!$noStatics == !$statics)
	&& ([::tcl::OptProcArgGiven -statics])} {
	return -code error\
	    "conflicting values given for -statics and -noStatics"
    }
    if {$flag} {
	return [expr {!$noStatics}]
    } else {
	return $statics
    }
}

# Helper function to resolve the dual way of specifying nested loading
# (either by -nestedLoadOk or -nested 1)
proc ::safe::InterpNested {} {
    foreach v {Args nested nestedLoadOk} {
	upvar $v $v
    }
    set flag [::tcl::OptProcArgGiven -nestedLoadOk]
    # note that the test here is the opposite of the "InterpStatics" one
    # (it is not -noNested... because of the wanted default value)
    if {$flag && (!$nestedLoadOk != !$nested)
	&& ([::tcl::OptProcArgGiven -nested])} {
	return -code error\
	    "conflicting values given for -nested and -nestedLoadOk"
    }
    if {$flag} {
	# another difference with "InterpStatics"
	return $nestedLoadOk
    } else {
	return $nested
    }
}

####
#
#  API entry points that needs argument parsing :
#
####

# Interface/entry point function and front end for "Create"
proc ::safe::interpCreate {args} {
    set Args [::tcl::OptKeyParse ::safe::interpCreate $args]
    InterpCreate $slave $accessPath \
	[InterpStatics] [InterpNested] $deleteHook
}

proc ::safe::interpInit {args} {
    set Args [::tcl::OptKeyParse ::safe::interpIC $args]
    if {![::interp exists $slave]} {
	return -code error "\"$slave\" is not an interpreter"
    }
    InterpInit $slave $accessPath \
	[InterpStatics] [InterpNested] $deleteHook
}

# Check that the given slave is "one of us"
proc ::safe::CheckInterp {slave} {
    namespace upvar ::safe S$slave state
    if {![info exists state] || ![::interp exists $slave]} {
	return -code error \
	    "\"$slave\" is not an interpreter managed by ::safe::"
    }
}

# Interface/entry point function and front end for "Configure".  This code
# is awfully pedestrian because it would need more coupling and support
# between the way we store the configuration values in safe::interp's and
# the Opt package. Obviously we would like an OptConfigure to avoid
# duplicating all this code everywhere.
# -> TODO (the app should share or access easily the program/value stored
# by opt)

# This is even more complicated by the boolean flags with no values that
# we had the bad idea to support for the sake of user simplicity in
# create/init but which makes life hard in configure...
# So this will be hopefully written and some integrated with opt1.0
# (hopefully for tcl8.1 ?)
proc ::safe::interpConfigure {args} {
    switch [llength $args] {
	1 {
	    # If we have exactly 1 argument the semantic is to return all
	    # the current configuration. We still call OptKeyParse though
	    # we know that "slave" is our given argument because it also
	    # checks for the "-help" option.
	    set Args [::tcl::OptKeyParse ::safe::interpIC $args]
	    CheckInterp $slave
	    namespace upvar ::safe S$slave state

	    return [join [list \
		[list -accessPath $state(access_path)] \
		[list -statics    $state(staticsok)]   \
		[list -nested     $state(nestedok)]    \
	        [list -deleteHook $state(cleanupHook)]]]
	}
	2 {
	    # If we have exactly 2 arguments the semantic is a "configure
	    # get"
	    lassign $args slave arg

	    # get the flag sub program (we 'know' about Opt's internal
	    # representation of data)
	    set desc [lindex [::tcl::OptKeyGetDesc ::safe::interpIC] 2]
	    set hits [::tcl::OptHits desc $arg]
	    if {$hits > 1} {
		return -code error [::tcl::OptAmbigous $desc $arg]
	    } elseif {$hits == 0} {
		return -code error [::tcl::OptFlagUsage $desc $arg]
	    }
	    CheckInterp $slave
	    namespace upvar ::safe S$slave state

	    set item [::tcl::OptCurDesc $desc]
	    set name [::tcl::OptName $item]
	    switch -exact -- $name {
		-accessPath {
		    return [list -accessPath $state(access_path)]
		}
		-statics    {
		    return [list -statics $state(staticsok)]
		}
		-nested     {
		    return [list -nested $state(nestedok)]
		}
		-deleteHook {
		    return [list -deleteHook $state(cleanupHook)]
		}
		-noStatics {
		    # it is most probably a set in fact but we would need
		    # then to jump to the set part and it is not *sure*
		    # that it is a set action that the user want, so force
		    # it to use the unambigous -statics ?value? instead:
		    return -code error\
			"ambigous query (get or set -noStatics ?)\
				use -statics instead"
		}
		-nestedLoadOk {
		    return -code error\
			"ambigous query (get or set -nestedLoadOk ?)\
				use -nested instead"
		}
		default {
		    return -code error "unknown flag $name (bug)"
		}
	    }
	}
	default {
	    # Otherwise we want to parse the arguments like init and
	    # create did
	    set Args [::tcl::OptKeyParse ::safe::interpIC $args]
	    CheckInterp $slave
	    namespace upvar ::safe S$slave state

	    # Get the current (and not the default) values of whatever has
	    # not been given:
	    if {![::tcl::OptProcArgGiven -accessPath]} {
		set doreset 1
		set accessPath $state(access_path)
	    } else {
		set doreset 0
	    }
	    if {
		![::tcl::OptProcArgGiven -statics]
		&& ![::tcl::OptProcArgGiven -noStatics]
	    } then {
		set statics    $state(staticsok)
	    } else {
		set statics    [InterpStatics]
	    }
	    if {
		[::tcl::OptProcArgGiven -nested] ||
		[::tcl::OptProcArgGiven -nestedLoadOk]
	    } then {
		set nested     [InterpNested]
	    } else {
		set nested     $state(nestedok)
	    }
	    if {![::tcl::OptProcArgGiven -deleteHook]} {
		set deleteHook $state(cleanupHook)
	    }
	    # we can now reconfigure :
	    InterpSetConfig $slave $accessPath $statics $nested $deleteHook
	    # auto_reset the slave (to completly synch the new access_path)
	    if {$doreset} {
		if {[catch {::interp eval $slave {auto_reset}} msg]} {
		    Log $slave "auto_reset failed: $msg"
		} else {
		    Log $slave "successful auto_reset" NOTICE
		}
	    }
	}
    }
}

####
#
#  Functions that actually implements the exported APIs
#
####

#
# safe::InterpCreate : doing the real job
#
# This procedure creates a safe slave and initializes it with the safe
# base aliases.
# NB: slave name must be simple alphanumeric string, no spaces, no (), no
# {},...  {because the state array is stored as part of the name}
#
# Returns the slave name.
#
# Optional Arguments :
# + slave name : if empty, generated name will be used
# + access_path: path list controlling where load/source can occur,
#                if empty: the master auto_path will be used.
# + staticsok  : flag, if 0 :no static package can be loaded (load {} Xxx)
#                      if 1 :static packages are ok.
# + nestedok: flag, if 0 :no loading to sub-sub interps (load xx xx sub)
#                      if 1 : multiple levels are ok.

# use the full name and no indent so auto_mkIndex can find us
proc ::safe::InterpCreate {
			   slave
			   access_path
			   staticsok
			   nestedok
			   deletehook
		       } {
    # Create the slave.
    if {$slave ne ""} {
	::interp create -safe $slave
    } else {
	# empty argument: generate slave name
	set slave [::interp create -safe]
    }
    Log $slave "Created" NOTICE

    # Initialize it. (returns slave name)
    InterpInit $slave $access_path $staticsok $nestedok $deletehook
}

#
# InterpSetConfig (was setAccessPath) :
#    Sets up slave virtual auto_path and corresponding structure within
#    the master. Also sets the tcl_library in the slave to be the first
#    directory in the path.
#    NB: If you change the path after the slave has been initialized you
#    probably need to call "auto_reset" in the slave in order that it gets
#    the right auto_index() array values.

proc ::safe::InterpSetConfig {slave access_path staticsok nestedok deletehook} {
    global auto_path

    # determine and store the access path if empty
    if {$access_path eq ""} {
	set access_path $auto_path

	# Make sure that tcl_library is in auto_path and at the first
	# position (needed by setAccessPath)
	set where [lsearch -exact $access_path [info library]]
	if {$where == -1} {
	    # not found, add it.
	    set access_path [linsert $access_path 0 [info library]]
	    Log $slave "tcl_library was not in auto_path,\
			added it to slave's access_path" NOTICE
	} elseif {$where != 0} {
	    # not first, move it first
	    set access_path [linsert \
				 [lreplace $access_path $where $where] \
				 0 [info library]]
	    Log $slave "tcl_libray was not in first in auto_path,\
			moved it to front of slave's access_path" NOTICE
	}

	# Add 1st level sub dirs (will searched by auto loading from tcl
	# code in the slave using glob and thus fail, so we add them here
	# so by default it works the same).
	set access_path [AddSubDirs $access_path]
    }

    Log $slave "Setting accessPath=($access_path) staticsok=$staticsok\
		nestedok=$nestedok deletehook=($deletehook)" NOTICE

    namespace upvar ::safe S$slave state

    # clear old autopath if it existed
    # build new one
    # Extend the access list with the paths used to look for Tcl Modules.
    # We save the virtual form separately as well, as syncing it with the
    # slave has to be defered until the necessary commands are present for
    # setup.

    set norm_access_path  {}
    set slave_access_path {}
    set map_access_path   {}
    set remap_access_path {}
    set slave_tm_path     {}

    set i 0
    foreach dir $access_path {
	set token [PathToken $i]
	lappend slave_access_path  $token
	lappend map_access_path    $token $dir
	lappend remap_access_path  $dir $token
	lappend norm_access_path   [file normalize $dir]
	incr i
    }

    set morepaths [::tcl::tm::list]
    while {[llength $morepaths]} {
	set addpaths $morepaths
	set morepaths {}

	foreach dir $addpaths {
	    # Prevent the addition of dirs on the tm list to the
	    # result if they are already known.
	    if {[dict exists $remap_access_path $dir]} {
		continue
	    }

	    set token [PathToken $i]
	    lappend access_path        $dir
	    lappend slave_access_path  $token
	    lappend map_access_path    $token $dir
	    lappend remap_access_path  $dir $token
	    lappend norm_access_path   [file normalize $dir]
	    lappend slave_tm_path $token
	    incr i

	    # [Bug 2854929]
	    # Recursively find deeper paths which may contain
	    # modules. Required to handle modules with names like
	    # 'platform::shell', which translate into
	    # 'platform/shell-X.tm', i.e arbitrarily deep
	    # subdirectories.
	    lappend morepaths {*}[glob -nocomplain -directory $dir -type d *]
	}
    }

    set state(access_path)       $access_path
    set state(access_path,map)   $map_access_path
    set state(access_path,remap) $remap_access_path
    set state(access_path,norm)  $norm_access_path
    set state(access_path,slave) $slave_access_path
    set state(tm_path_slave)     $slave_tm_path
    set state(staticsok)         $staticsok
    set state(nestedok)          $nestedok
    set state(cleanupHook)       $deletehook

    SyncAccessPath $slave
}

#
#
# FindInAccessPath:
#    Search for a real directory and returns its virtual Id (including the
#    "$")
proc ::safe::interpFindInAccessPath {slave path} {
    namespace upvar ::safe S$slave state

    if {![dict exists $state(access_path,remap) $path]} {
	return -code error "$path not found in access path $access_path"
    }

    return [dict get $state(access_path,remap) $path]
}

#
# addToAccessPath:
#    add (if needed) a real directory to access path and return its
#    virtual token (including the "$").
proc ::safe::interpAddToAccessPath {slave path} {
    # first check if the directory is already in there
    # (inlined interpFindInAccessPath).
    namespace upvar ::safe S$slave state

    if {[dict exists $state(access_path,remap) $path]} {
	return [dict get $state(access_path,remap) $path]
    }

    # new one, add it:
    set token [PathToken [llength $state(access_path)]]

    lappend state(access_path)       $path
    lappend state(access_path,slave) $token
    lappend state(access_path,map)   $token $path
    lappend state(access_path,remap) $path $token
    lappend state(access_path,norm)  [file normalize $path]

    SyncAccessPath $slave
    return $token
}

# This procedure applies the initializations to an already existing
# interpreter. It is useful when you want to install the safe base aliases
# into a preexisting safe interpreter.
proc ::safe::InterpInit {
			 slave
			 access_path
			 staticsok
			 nestedok
			 deletehook
		     } {
    # Configure will generate an access_path when access_path is empty.
    InterpSetConfig $slave $access_path $staticsok $nestedok $deletehook

    # NB we need to add [namespace current], aliases are always absolute
    # paths.

    # These aliases let the slave load files to define new commands
    # This alias lets the slave use the encoding names, convertfrom,
    # convertto, and system, but not "encoding system <name>" to set the
    # system encoding.
    # Handling Tcl Modules, we need a restricted form of Glob.
    # This alias interposes on the 'exit' command and cleanly terminates
    # the slave.

    foreach {command alias} {
	source   AliasSource
	load     AliasLoad
	encoding AliasEncoding
	exit     interpDelete
	glob     AliasGlob
    } {
	::interp alias $slave $command {} [namespace current]::$alias $slave
    }

    # This alias lets the slave have access to a subset of the 'file'
    # command functionality.

    ::interp expose $slave file
    foreach subcommand {dirname extension rootname tail} {
	::interp alias $slave ::tcl::file::$subcommand {} \
	    ::safe::AliasFileSubcommand $slave $subcommand
    }
    foreach subcommand {
	atime attributes copy delete executable exists isdirectory isfile
	link lstat mtime mkdir nativename normalize owned readable readlink
	rename size stat tempfile type volumes writable
    } {
	::interp alias $slave ::tcl::file::$subcommand {} \
	    ::safe::BadSubcommand $slave file $subcommand
    }

    # Subcommands of info
    foreach {subcommand alias} {
	nameofexecutable   AliasExeName
    } {
	::interp alias $slave ::tcl::info::$subcommand \
	    {} [namespace current]::$alias $slave
    }

    # The allowed slave variables already have been set by Tcl_MakeSafe(3)

    # Source init.tcl and tm.tcl into the slave, to get auto_load and
    # other procedures defined:

    if {[catch {::interp eval $slave {
	source [file join $tcl_library init.tcl]
    }} msg opt]} {
	Log $slave "can't source init.tcl ($msg)"
	return -options $opt "can't source init.tcl into slave $slave ($msg)"
    }

    if {[catch {::interp eval $slave {
	source [file join $tcl_library tm.tcl]
    }} msg opt]} {
	Log $slave "can't source tm.tcl ($msg)"
	return -options $opt "can't source tm.tcl into slave $slave ($msg)"
    }

    # Sync the paths used to search for Tcl modules. This can be done only
    # now, after tm.tcl was loaded.
    namespace upvar ::safe S$slave state
    if {[llength $state(tm_path_slave)] > 0} {
	::interp eval $slave [list \
		::tcl::tm::add {*}[lreverse $state(tm_path_slave)]]
    }
    return $slave
}

# Add (only if needed, avoid duplicates) 1 level of sub directories to an
# existing path list.  Also removes non directories from the returned
# list.
proc ::safe::AddSubDirs {pathList} {
    set res {}
    foreach dir $pathList {
	if {[file isdirectory $dir]} {
	    # check that we don't have it yet as a children of a previous
	    # dir
	    if {$dir ni $res} {
		lappend res $dir
	    }
	    foreach sub [glob -directory $dir -nocomplain *] {
		if {[file isdirectory $sub] && ($sub ni $res)} {
		    # new sub dir, add it !
		    lappend res $sub
		}
	    }
	}
    }
    return $res
}

# This procedure deletes a safe slave managed by Safe Tcl and cleans up
# associated state:

proc ::safe::interpDelete {slave} {
    Log $slave "About to delete" NOTICE

    namespace upvar ::safe S$slave state

    # If the slave has a cleanup hook registered, call it.  Check the
    # existance because we might be called to delete an interp which has
    # not been registered with us at all

    if {[info exists state(cleanupHook)]} {
	set hook $state(cleanupHook)
	if {[llength $hook]} {
	    # remove the hook now, otherwise if the hook calls us somehow,
	    # we'll loop
	    unset state(cleanupHook)
	    try {
		{*}$hook $slave
	    } on error err {
		Log $slave "Delete hook error ($err)"
	    }
	}
    }

    # Discard the global array of state associated with the slave, and
    # delete the interpreter.

    if {[info exists state]} {
	unset state
    }

    # if we have been called twice, the interp might have been deleted
    # already
    if {[::interp exists $slave]} {
	::interp delete $slave
	Log $slave "Deleted" NOTICE
    }

    return
}

# Set (or get) the logging mecanism

proc ::safe::setLogCmd {args} {
    variable Log
    set la [llength $args]
    if {$la == 0} {
	return $Log
    } elseif {$la == 1} {
	set Log [lindex $args 0]
    } else {
	set Log $args
    }

    if {$Log eq ""} {
	# Disable logging completely. Calls to it will be compiled out
	# of all users.
	proc ::safe::Log {args} {}
    } else {
	# Activate logging, define proper command.

	proc ::safe::Log {slave msg {type ERROR}} {
	    variable Log
	    {*}$Log "$type for slave $slave : $msg"
	    return
	}
    }
}

# ------------------- END OF PUBLIC METHODS ------------

#
# Sets the slave auto_path to the master recorded value.  Also sets
# tcl_library to the first token of the virtual path.
#
proc ::safe::SyncAccessPath {slave} {
    namespace upvar ::safe S$slave state

    set slave_access_path $state(access_path,slave)
    ::interp eval $slave [list set auto_path $slave_access_path]

    Log $slave "auto_path in $slave has been set to $slave_access_path"\
	NOTICE

    # This code assumes that info library is the first element in the
    # list of auto_path's. See -> InterpSetConfig for the code which
    # ensures this condition.

    ::interp eval $slave [list \
	      set tcl_library [lindex $slave_access_path 0]]
}

# Returns the virtual token for directory number N.
proc ::safe::PathToken {n} {
    # We need to have a ":" in the token string so [file join] on the
    # mac won't turn it into a relative path.
    return "\$p(:$n:)" ;# Form tested by case 7.2
}

#
# translate virtual path into real path
#
proc ::safe::TranslatePath {slave path} {
    namespace upvar ::safe S$slave state

    # somehow strip the namespaces 'functionality' out (the danger is that
    # we would strip valid macintosh "../" queries... :
    if {[string match "*::*" $path] || [string match "*..*" $path]} {
	return -code error "invalid characters in path $path"
    }

    # Use a cached map instead of computed local vars and subst.

    return [string map $state(access_path,map) $path]
}

# file name control (limit access to files/resources that should be a
# valid tcl source file)
proc ::safe::CheckFileName {slave file} {
    # This used to limit what can be sourced to ".tcl" and forbid files
    # with more than 1 dot and longer than 14 chars, but I changed that
    # for 8.4 as a safe interp has enough internal protection already to
    # allow sourcing anything. - hobbs

    if {![file exists $file]} {
	# don't tell the file path
	return -code error "no such file or directory"
    }

    if {![file readable $file]} {
	# don't tell the file path
	return -code error "not readable"
    }
}

# AliasFileSubcommand handles selected subcommands of [file] in safe
# interpreters that are *almost* safe. In particular, it just acts to
# prevent discovery of what home directories exist.

proc ::safe::AliasFileSubcommand {slave subcommand name} {
    if {[string match ~* $name]} {
	set name ./$name
    }
    tailcall ::interp invokehidden $slave tcl:file:$subcommand $name
}

# AliasGlob is the target of the "glob" alias in safe interpreters.

proc ::safe::AliasGlob {slave args} {
    Log $slave "GLOB ! $args" NOTICE
    set cmd {}
    set at 0
    array set got {
	-directory 0
	-nocomplain 0
	-join 0
	-tails 0
	-- 0
    }

    if {$::tcl_platform(platform) eq "windows"} {
	set dirPartRE {^(.*)[\\/]([^\\/]*)$}
    } else {
	set dirPartRE {^(.*)/([^/]*)$}
    }

    set dir        {}
    set virtualdir {}

    while {$at < [llength $args]} {
	switch -glob -- [set opt [lindex $args $at]] {
	    -nocomplain - -- - -join - -tails {
		lappend cmd $opt
		set got($opt) 1
		incr at
	    }
	    -types - -type {
		lappend cmd -types [lindex $args [incr at]]
		incr at
	    }
	    -directory {
		if {$got($opt)} {
		    return -code error \
			{"-directory" cannot be used with "-path"}
		}
		set got($opt) 1
		set virtualdir [lindex $args [incr at]]
		incr at
	    }
	    pkgIndex.tcl {
		# Oops, this is globbing a subdirectory in regular package
		# search. That is not wanted. Abort, handler does catch
		# already (because glob was not defined before). See
		# package.tcl, lines 484ff in tclPkgUnknown.
		return -code error "unknown command glob"
	    }
	    -* {
		Log $slave "Safe base rejecting glob option '$opt'"
		return -code error "Safe base rejecting glob option '$opt'"
	    }
	    default {
		break
	    }
	}
	if {$got(--)} break
    }

    # Get the real path from the virtual one and check that the path is in the
    # access path of that slave. Done after basic argument processing so that
    # we know if -nocomplain is set.
    if {$got(-directory)} {
	try {
	    set dir [TranslatePath $slave $virtualdir]
	    DirInAccessPath $slave $dir
	} on error msg {
	    Log $slave $msg
	    if {$got(-nocomplain)} return
	    return -code error "permission denied"
	}
	lappend cmd -directory $dir
    }

    # Apply the -join semantics ourselves
    if {$got(-join)} {
	set args [lreplace $args $at end [join [lrange $args $at end] "/"]]
    }

    # Process remaining pattern arguments
    set firstPattern [llength $cmd]
    foreach opt [lrange $args $at end] {
	if {![regexp $dirPartRE $opt -> thedir thefile]} {
	    set thedir .
	} elseif {[string match ~* $thedir]} {
	    set thedir ./$thedir
	}
	if {$thedir eq "*" &&
		($thefile eq "pkgIndex.tcl" || $thefile eq "*.tm")} {
	    set mapped 0
	    foreach d [glob -directory [TranslatePath $slave $virtualdir] \
			   -types d -tails *] {
		catch {
		    DirInAccessPath $slave \
			[TranslatePath $slave [file join $virtualdir $d]]
		    lappend cmd [file join $d $thefile]
		    set mapped 1
		}
	    }
	    if {$mapped} continue
	}
	try {
	    DirInAccessPath $slave [TranslatePath $slave \
		    [file join $virtualdir $thedir]]
	} on error msg {
	    Log $slave $msg
	    if {$got(-nocomplain)} continue
	    return -code error "permission denied"
	}
	lappend cmd $opt
    }

    Log $slave "GLOB = $cmd" NOTICE

    if {$got(-nocomplain) && [llength $cmd] eq $firstPattern} {
	return
    }
    try {
	set entries [::interp invokehidden $slave glob {*}$cmd]
    } on error msg {
	Log $slave $msg
	return -code error "script error"
    }

    Log $slave "GLOB < $entries" NOTICE

    # Translate path back to what the slave should see.
    set res {}
    set l [string length $dir]
    foreach p $entries {
	if {[string equal -length $l $dir $p]} {
	    set p [string replace $p 0 [expr {$l-1}] $virtualdir]
	}
	lappend res $p
    }

    Log $slave "GLOB > $res" NOTICE
    return $res
}

# AliasSource is the target of the "source" alias in safe interpreters.

proc ::safe::AliasSource {slave args} {
    set argc [llength $args]
    # Extended for handling of Tcl Modules to allow not only "source
    # filename", but "source -encoding E filename" as well.
    if {[lindex $args 0] eq "-encoding"} {
	incr argc -2
	set encoding [lindex $args 1]
	set at 2
	if {$encoding eq "identity"} {
	    Log $slave "attempt to use the identity encoding"
	    return -code error "permission denied"
	}
    } else {
	set at 0
	set encoding {}
    }
    if {$argc != 1} {
	set msg "wrong # args: should be \"source ?-encoding E? fileName\""
	Log $slave "$msg ($args)"
	return -code error $msg
    }
    set file [lindex $args $at]

    # get the real path from the virtual one.
    if {[catch {
	set realfile [TranslatePath $slave $file]
    } msg]} {
	Log $slave $msg
	return -code error "permission denied"
    }

    # check that the path is in the access path of that slave
    if {[catch {
	FileInAccessPath $slave $realfile
    } msg]} {
	Log $slave $msg
	return -code error "permission denied"
    }

    # do the checks on the filename :
    if {[catch {
	CheckFileName $slave $realfile
    } msg]} {
	Log $slave "$realfile:$msg"
	return -code error $msg
    }

    # Passed all the tests, lets source it. Note that we do this all manually
    # because we want to control [info script] in the slave so information
    # doesn't leak so much. [Bug 2913625]
    set old [::interp eval $slave {info script}]
    set replacementMsg "script error"
    set code [catch {
	set f [open $realfile]
	fconfigure $f -eofchar \032
	if {$encoding ne ""} {
	    fconfigure $f -encoding $encoding
	}
	set contents [read $f]
	close $f
	::interp eval $slave [list info script $file]
    } msg opt]
    if {$code == 0} {
	set code [catch {::interp eval $slave $contents} msg opt]
	set replacementMsg $msg
    }
    catch {interp eval $slave [list info script $old]}
    # Note that all non-errors are fine result codes from [source], so we must
    # take a little care to do it properly. [Bug 2923613]
    if {$code == 1} {
	Log $slave $msg
	return -code error $replacementMsg
    }
    return -code $code -options $opt $msg
}

# AliasLoad is the target of the "load" alias in safe interpreters.

proc ::safe::AliasLoad {slave file args} {
    set argc [llength $args]
    if {$argc > 2} {
	set msg "load error: too many arguments"
	Log $slave "$msg ($argc) {$file $args}"
	return -code error $msg
    }

    # package name (can be empty if file is not).
    set package [lindex $args 0]

    namespace upvar ::safe S$slave state

    # Determine where to load. load use a relative interp path and {}
    # means self, so we can directly and safely use passed arg.
    set target [lindex $args 1]
    if {$target ne ""} {
	# we will try to load into a sub sub interp; check that we want to
	# authorize that.
	if {!$state(nestedok)} {
	    Log $slave "loading to a sub interp (nestedok)\
			disabled (trying to load $package to $target)"
	    return -code error "permission denied (nested load)"
	}
    }

    # Determine what kind of load is requested
    if {$file eq ""} {
	# static package loading
	if {$package eq ""} {
	    set msg "load error: empty filename and no package name"
	    Log $slave $msg
	    return -code error $msg
	}
	if {!$state(staticsok)} {
	    Log $slave "static packages loading disabled\
			(trying to load $package to $target)"
	    return -code error "permission denied (static package)"
	}
    } else {
	# file loading

	# get the real path from the virtual one.
	try {
	    set file [TranslatePath $slave $file]
	} on error msg {
	    Log $slave $msg
	    return -code error "permission denied"
	}

	# check the translated path
	try {
	    FileInAccessPath $slave $file
	} on error msg {
	    Log $slave $msg
	    return -code error "permission denied (path)"
	}
    }

    try {
	return [::interp invokehidden $slave load $file $package $target]
    } on error msg {
	Log $slave $msg
	return -code error $msg
    }
}

# FileInAccessPath raises an error if the file is not found in the list of
# directories contained in the (master side recorded) slave's access path.

# the security here relies on "file dirname" answering the proper
# result... needs checking ?
proc ::safe::FileInAccessPath {slave file} {
    namespace upvar ::safe S$slave state
    set access_path $state(access_path)

    if {[file isdirectory $file]} {
	return -code error "\"$file\": is a directory"
    }
    set parent [file dirname $file]

    # Normalize paths for comparison since lsearch knows nothing of
    # potential pathname anomalies.
    set norm_parent [file normalize $parent]

    namespace upvar ::safe S$slave state
    if {$norm_parent ni $state(access_path,norm)} {
	return -code error "\"$file\": not in access_path"
    }
}

proc ::safe::DirInAccessPath {slave dir} {
    namespace upvar ::safe S$slave state
    set access_path $state(access_path)

    if {[file isfile $dir]} {
	return -code error "\"$dir\": is a file"
    }

    # Normalize paths for comparison since lsearch knows nothing of
    # potential pathname anomalies.
    set norm_dir [file normalize $dir]

    namespace upvar ::safe S$slave state
    if {$norm_dir ni $state(access_path,norm)} {
	return -code error "\"$dir\": not in access_path"
    }
}

# This procedure is used to report an attempt to use an unsafe member of an
# ensemble command.

proc ::safe::BadSubcommand {slave command subcommand args} {
    set msg "not allowed to invoke subcommand $subcommand of $command"
    Log $slave $msg
    return -code error -errorcode {TCL SAFE SUBCOMMAND} $msg
}

# AliasEncoding is the target of the "encoding" alias in safe interpreters.

proc ::safe::AliasEncoding {slave option args} {
    # Note that [encoding dirs] is not supported in safe slaves at all
    set subcommands {convertfrom convertto names system}
    try {
	set option [tcl::prefix match -error [list -level 1 -errorcode \
		[list TCL LOOKUP INDEX option $option]] $subcommands $option]
	# Special case: [encoding system] ok, but [encoding system foo] not
	if {$option eq "system" && [llength $args]} {
	    return -code error -errorcode {TCL WRONGARGS} \
		"wrong # args: should be \"encoding system\""
	}
    } on error {msg options} {
	Log $slave $msg
	return -options $options $msg
    }
    tailcall ::interp invokehidden $slave encoding $option {*}$args
}

# Various minor hiding of platform features. [Bug 2913625]

proc ::safe::AliasExeName {slave} {
    return ""
}

proc ::safe::Setup {} {
    ####
    #
    # Setup the arguments parsing
    #
    ####

    # Share the descriptions
    set temp [::tcl::OptKeyRegister {
	{-accessPath -list {} "access path for the slave"}
	{-noStatics "prevent loading of statically linked pkgs"}
	{-statics true "loading of statically linked pkgs"}
	{-nestedLoadOk "allow nested loading"}
	{-nested false "nested loading"}
	{-deleteHook -script {} "delete hook"}
    }]

    # create case (slave is optional)
    ::tcl::OptKeyRegister {
	{?slave? -name {} "name of the slave (optional)"}
    } ::safe::interpCreate

    # adding the flags sub programs to the command program (relying on Opt's
    # internal implementation details)
    lappend ::tcl::OptDesc(::safe::interpCreate) $::tcl::OptDesc($temp)

    # init and configure (slave is needed)
    ::tcl::OptKeyRegister {
	{slave -name {} "name of the slave"}
    } ::safe::interpIC

    # adding the flags sub programs to the command program (relying on Opt's
    # internal implementation details)
    lappend ::tcl::OptDesc(::safe::interpIC) $::tcl::OptDesc($temp)

    # temp not needed anymore
    ::tcl::OptKeyDelete $temp

    ####
    #
    # Default: No logging.
    #
    ####

    setLogCmd {}

    # Log eventually.
    # To enable error logging, set Log to {puts stderr} for instance,
    # via setLogCmd.
    return
}

namespace eval ::safe {
    # internal variables

    # Log command, set via 'setLogCmd'. Logging is disabled when empty.
    variable Log {}

    # The package maintains a state array per slave interp under its
    # control. The name of this array is S<interp-name>. This array is
    # brought into scope where needed, using 'namespace upvar'. The S
    # prefix is used to avoid that a slave interp called "Log" smashes
    # the "Log" variable.
    #
    # The array's elements are:
    #
    # access_path       : List of paths accessible to the slave.
    # access_path,norm  : Ditto, in normalized form.
    # access_path,slave : Ditto, as the path tokens as seen by the slave.
    # access_path,map   : dict ( token -> path )
    # access_path,remap : dict ( path -> token )
    # tm_path_slave     : List of TM root directories, as tokens seen by the slave.
    # staticsok         : Value of option -statics
    # nestedok          : Value of option -nested
    # cleanupHook       : Value of option -deleteHook
}

::safe::Setup
