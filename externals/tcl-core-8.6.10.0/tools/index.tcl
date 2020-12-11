# index.tcl --
#
# This file defines procedures that are used during the first pass of
# the man page conversion.  It is used to extract information used to
# generate a table of contents and a keyword list.
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

# Global variables used by these scripts:
#
# state -	state variable that controls action of text proc.
#
# topics -	array indexed by (package,section,topic) with value
# 		of topic ID.
#
# keywords -	array indexed by keyword string with value of topic ID.
#
# curID - 	current topic ID, starts at 0 and is incremented for
# 		each new topic file.
#
# curPkg -	current package name (e.g. Tcl).
#
# curSect -	current section title (e.g. "Tcl Built-In Commands").
#

# getPackages --
#
# Generate a sorted list of package names from the topics array.
#
# Arguments:
# none.

proc getPackages {} {
    global topics
    foreach i [array names topics] {
	regsub {^(.*),.*,.*$} $i {\1} i
	set temp($i) {}
    }
    lsort [array names temp]
}

# getSections --
#
# Generate a sorted list of section titles in the specified package
# from the topics array.
#
# Arguments:
# pkg -			Name of package to search.

proc getSections {pkg} {
    global topics
    regsub -all {[][*?\\]} $pkg {\\&} pkg
    foreach i [array names topics "${pkg},*"] {
	regsub {^.*,(.*),.*$} $i {\1} i
	set temp($i) {}
    }
    lsort [array names temp]
}

# getTopics --
#
# Generate a sorted list of topics in the specified section of the
# specified package from the topics array.
#
# Arguments:
# pkg -			Name of package to search.
# sect -		Name of section to search.

proc getTopics {pkg sect} {
    global topics
    regsub -all {[][*?\\]} $pkg {\\&} pkg
    regsub -all {[][*?\\]} $sect {\\&} sect
    foreach i [array names topics "${pkg},${sect},*"] {
	regsub {^.*,.*,(.*)$} $i {\1} i
	set temp($i) {}
    }
    lsort [array names temp]
}

# text --
#
# This procedure adds entries to the hypertext arrays topics and keywords.
#
# Arguments:
# string -		Text to index.


proc text string {
    global state curID curPkg curSect topics keywords

    switch $state {
	NAME {
	    foreach i [split $string ","] {
		set topic [string trim $i]
		set index "$curPkg,$curSect,$topic"
		if {[info exists topics($index)]
		    && [string compare $topics($index) $curID] != 0} {
		    puts stderr "duplicate topic $topic in $curPkg"
		}
		set topics($index) $curID
		lappend keywords($topic) $curID
	    }
	}
	KEY {
	    foreach i [split $string ","] {
		lappend keywords([string trim $i]) $curID
	    }
	}
	DT -
	OFF -
	DASH {}
	default {
	    puts stderr "text: unknown state: $state"
	}
    }
}


# macro --
#
# This procedure is invoked to process macro invocations that start
# with "." (instead of ').
#
# Arguments:
# name -	The name of the macro (without the ".").
# args -	Any additional arguments to the macro.

proc macro {name args} {
    switch $name {
	SH - SS {
	    global state

	    switch $args {
		NAME {
		    if {$state eq "INIT" } {
			set state NAME
		    }
		}
		DESCRIPTION {set state DT}
		INTRODUCTION {set state DT}
		KEYWORDS {set state KEY}
		default {set state OFF}
	    }

	}
	TH {
	    global state curID curPkg curSect topics keywords
	    set state INIT
	    if {[llength $args] != 5} {
		set args [join $args " "]
		puts stderr "Bad .TH macro: .$name $args"
	    }
	    incr curID
	    set topic	[lindex $args 0]	;# Tcl_UpVar
	    set curPkg	[lindex $args 3]	;# Tcl
	    set curSect	[lindex $args 4]	;# {Tcl Library Procedures}
	    regsub -all {\\ } $curSect { } curSect
	    set index "$curPkg,$curSect,$topic"
	    set topics($index) $curID
	    lappend keywords($topic) $curID
	}
    }
}


# dash --
#
# This procedure is invoked to handle dash characters ("\-" in
# troff).  It only function in pass1 is to terminate the NAME state.
#
# Arguments:
# None.

proc dash {} {
    global state
    if {$state eq "NAME"} {
	set state DASH
    }
}



# initGlobals, tab, font, char, macro2 --
#
# These procedures do nothing during the first pass.
#
# Arguments:
# None.

proc initGlobals {} {}
proc newline {} {}
proc tab {} {}
proc font type {}
proc char name {}
proc macro2 {name args} {}

