# man2html1.tcl --
#
# This file defines procedures that are used during the first pass of the
# man page to html conversion process. It is sourced by h.tcl.
#
# Copyright (c) 1996 by Sun Microsystems, Inc.

# Global variables used by these scripts:
#
# state -	state variable that controls action of text proc.
#
# curFile -	tail of current man page.
#
# file -	file pointer; for both xref.tcl and contents.html
#
# NAME_file -	array indexed by NAME and containing file names used
#		for hyperlinks.
#
# KEY_file -	array indexed by KEYWORD and containing file names used
#		for hyperlinks.
#
# lib -		contains package name. Used to label section in contents.html
#
# inDT -	in dictionary term.


# text --
#
# This procedure adds entries to the hypertext arrays NAME_file
# and KEY_file.
#
# DT: might do this: if first word of $dt matches $name and [llength $name==1]
# 	and [llength $dt > 1], then add to NAME_file.
#
# Arguments:
# string -		Text to index.

proc text string {
    global state curFile NAME_file KEY_file inDT

    switch $state {
	NAME {
	    foreach i [split $string ","] {
		lappend NAME_file([string trim $i]) $curFile
	    }
	}
	KEY {
	    foreach i [split $string ","] {
		lappend KEY_file([string trim $i]) $curFile
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
		    if {$state eq "INIT"} {
			set state NAME
		    }
		}
		DESCRIPTION {set state DT}
		INTRODUCTION {set state DT}
		KEYWORDS {set state KEY}
		default {set state OFF}
	    }

	}
	TP {
	    global inDT
	    set inDT 1
	}
	TH {
	    global lib state inDT
	    set inDT 0
	    set state INIT
	    if {[llength $args] != 5} {
		set args [join $args " "]
		puts stderr "Bad .TH macro: .$name $args"
	    }
	    set lib [lindex $args 3]				;# Tcl or Tk
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


# newline --
#
# This procedure is invoked to handle newlines in the troff input.
# It's only purpose is to terminate a DT (dictionary term).
#
# Arguments:
# None.

proc newline {} {
    global inDT
    set inDT 0
}


# initGlobals, tab, font, char, macro2 --
#
# These procedures do nothing during the first pass.
#
# Arguments:
# None.

proc initGlobals {} {}
proc tab {} {}
proc font type {}
proc char name {}
proc macro2 {name args} {}


# doListing --
#
# Writes an ls like list to a file. Searches NAME_file for entries
# that match the input pattern.
#
# Arguments:
# file -		Output file pointer.
# pattern -		glob style match pattern

proc doListing {file pattern} {
    global NAME_file

    set max_len 0
    foreach name [lsort [array names NAME_file]] {
	set ref $NAME_file($name)
	    if [string match $pattern $ref] {
		lappend type $name
		if {[string length $name] > $max_len} {
		set max_len [string length $name]
	    }
	}
    }
    if [catch {llength $type} ] {
	puts stderr "       doListing: no names matched pattern ($pattern)"
	return
    }
    incr max_len
    set ncols [expr {90/$max_len}]
    set nrows [expr {int(ceil([llength $type] / double($ncols)))} ]

#	? max_len ncols nrows

    set index 0
    foreach f $type {
	lappend row([expr {$index % $nrows}]) $f
	incr index
    }

    puts -nonewline $file "<PRE>"
    for {set i 0} {$i<$nrows} {incr i} {
	foreach name $row($i) {
	    set str [format "%-*s" $max_len $name]
	    regsub $name $str "<A HREF=\"$NAME_file($name).html\">$name</A>" str
	    puts -nonewline $file $str
	}
	puts $file {}
    }
    puts $file "</PRE>"
}


# doContents --
#
# Generates a HTML contents file using the NAME_file array
# as its input database.
#
# Arguments:
# file -		name of the contents file.
# packageName -		string used in the title and sub-heads of the HTML
#			page. Normally name of the package without version
#			numbers.

proc doContents {file packageName} {
    global footer

    set file [open $file w]

    puts $file "<HTML><HEAD><TITLE>$packageName Manual</TITLE></HEAD><BODY>"
    puts $file "<H3>$packageName</H3>"
    doListing $file "*.1"

    puts $file "<HR><H3>$packageName Commands</H3>"
    doListing $file "*.n"

    puts $file "<HR><H3>$packageName Library</H3>"
    doListing $file "*.3"

    puts $file $footer
    puts $file "</BODY></HTML>"
    close $file
}


# do --
#
# This is the toplevel procedure that searches a man page
# for hypertext links.  It builds a data base consisting of
# two arrays: NAME_file and KEY file. It runs the man2tcl
# program to turn the man page into a script, then it evals
# that script.
#
# Arguments:
# fileName -		Name of the file to scan.

proc do fileName {
    global curFile
    set curFile [file tail $fileName]
    set file stdout
    puts "  Pass 1 -- $fileName"
    flush stdout
    if [catch {eval [exec man2tcl [glob $fileName]]} msg] {
	global errorInfo
	puts stderr $msg
	puts "in"
	puts $errorInfo
	exit 1
    }
}
