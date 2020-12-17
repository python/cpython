# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: fs.tcl,v 1.6 2004/03/28 02:44:57 hobbs Exp $
#
# File system routines to handle some file system variations
# and how that interoperates with the Tix widgets (mainly HList).
#
# Copyright (c) 2004 ActiveState

##
## Cross-platform
##

proc tixFSSep {} { return "/" }

proc tixFSNormalize {path} {
    # possibly use tixFSTilde ?
    return [file normalize $path]
}

proc tixFSVolumes {} {
    return [file volumes]
}

proc tixFSAncestors {path} {
    return [file split [file normalize $path]]
}

# how a filename should be displayed
proc tixFSDisplayFileName {path} {
    if {$path eq [file dirname $path]} {
	return $path
    } else {
	return [file tail $path]
    }
}

# dir:		Make a listing of this directory
# showSubDir:	Want to list the subdirectories?
# showFile:	Want to list the non-directory files in this directory?
# showPrevDir:	Want to list ".." as well?
# showHidden:	Want to list the hidden files?
#
# return value:	a list of files and/or subdirectories
#
proc tixFSListDir {dir showSubDir showFile showPrevDir \
		       showHidden {pattern ""}} {

    if {$pattern eq ""} { set pattern [list "*"] }
    if {$::tcl_platform(platform) eq "unix"
	&& $showHidden && $pattern eq "*"} { lappend pattern ".*" }

    if {[catch {eval [list glob -nocomplain -directory $dir] \
		    $pattern} files]} {
	# The user has entered an invalid or unreadable directory
	# %% todo: prompt error, go back to last succeed directory
	return ""
    }
    set list ""
    foreach f [lsort -dictionary $files] {
	set tail [file tail $f]
	# file tail handles this automatically
	#if {[string match ~* $tail]} { set tail ./$tail }
	if {[file isdirectory $f]} {
	    if {$tail eq "."} { continue }
	    if {$showSubDir} {
		if {$tail eq ".." && !$showPrevDir} { continue }
		lappend list $tail
	    }
	} else {
	    if {$showFile} { lappend list $tail }
	}
    }
    return $list
}

# in:	internal name
# out:	native name
proc tixFSNativeNorm {path} {
    return [tixFSNative [tixFSNormalize $path]]
}

# tixFSDisplayName --
#
#	Returns the name of a normalized path which is usually displayed by
#	the OS
#
proc tixFSDisplayName {path} {
    return [tixFSNative $path]
}

proc tixFSTilde {path} {
    # verify that paths with leading ~ are files or real users
    if {[string match ~* $path]} {
	# The following will report if the user doesn't exist
	if {![file isdirectory $path]} {
	    set path ./$path
	} else {
	    set path [file normalize $path]
	}
    }
    return $path
}

proc tixFSJoin {dir sub} {
    return [tixFSNative [file join $dir [tixFSTilde $sub]]]
}

proc tixFSNative {path} {
    return $path
}

if {$::tcl_platform(platform) eq "windows"} {

    ##
    ## WINDOWS
    ##

    # is an absoulte path only if it starts with a baclskash
    # or starts with "<drive letter>:"
    #
    # in: nativeName
    #
    proc tixFSIsAbsPath {nativeName} {
	set ptype [file pathtype $nativename]
	return [expr {$ptype eq "absolute" || $ptype eq "volumerelative"}]
    }

    # tixFSIsValid --
    #
    #	Checks whether a native pathname contains invalid characters.
    #
    proc tixFSIsValid {path} {
	#if {$::tcl_platform(platform) eq "windows"} {set bad "\\/:*?\"<>|\0"}
	return 1
    }

    proc tixFSExternal {path} {
	# Avoid normalization on root adding unwanted volumerelative pwd
	if {[string match -nocase {[A-Z]:} $path]} {
	    return $path/
	}
	return [file normalize $path]
    }

    proc tixFSInternal {path} {
	# Only need to watch for ^[A-Z]:/$, but this does the trick
	return [string trimright [file normalize $path] /]
    }

} else {

    ##
    ## UNIX
    ##

    proc tixFSIsAbsPath {path} {
	return [string match {[~/]*} $path]
    }

    # tixFSIsValid --
    #
    #	Checks whether a native pathname contains invalid characters.
    #
    proc tixFSIsValid {path} { return 1 }

    proc tixFSExternal {path} { return $path }
    proc tixFSInternal {path} { return $path }

}
