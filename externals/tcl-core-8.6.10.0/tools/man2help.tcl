# man2help.tcl --
#
# This file defines procedures that work in conjunction with the
# man2tcl program to generate a Windows help file from Tcl manual
# entries.
#
# Copyright (c) 1996 by Sun Microsystems, Inc.

#
# PASS 1
#

set man2tclprog [file join [file dirname [info script]] \
	man2tcl[file extension [info nameofexecutable]]]

proc generateContents {basename version files} {
    global curID topics
    set curID 0
    foreach f $files {
	puts "Pass 1 -- $f"
	flush stdout
	doFile $f
    }
    set fd [open [file join [file dirname [info script]] $basename$version.cnt] w]
    fconfigure $fd -translation crlf
    puts $fd ":Base $basename$version.hlp"
    foreach package [getPackages] {
	foreach section [getSections $package] {
            if {![info exists lastSection]} {
	        set lastSection {}
            }
            if {[string compare $lastSection $section]} {
	    puts $fd "1 $section"
            }
            set lastSection $section
	    set lastTopic {}
	    foreach topic [getTopics $package $section] {
		if {[string compare $lastTopic $topic]} {
		    set id $topics($package,$section,$topic)
		    puts $fd "2 $topic=$id"
		    set lastTopic $topic
		}
	    }
	}
    }
    close $fd
}


#
# PASS 2
#

proc generateHelp {basename files} {
    global curID topics keywords file id_keywords
    set curID 0

    foreach key [array names keywords] {
	foreach id $keywords($key) {
	    lappend id_keywords($id) $key
	}
    }

    set file [open [file join [file dirname [info script]] $basename.rtf] w]
    fconfigure $file -translation crlf
    puts $file "\{\\rtf1\\ansi \\deff0\\deflang1033\{\\fonttbl\{\\f0\\froman\\fcharset0\\fprq2 Times New Roman\;\}\{\\f1\\fmodern\\fcharset0\\fprq1 Courier New\;\}\}"
    foreach f $files {
	puts "Pass 2 -- $f"
	flush stdout
	initGlobals
	doFile $f
	pageBreak
    }
    puts $file "\}"
    close $file
}

# doFile --
#
# Given a file as argument, translate the file to a tcl script and
# evaluate it.
#
# Arguments:
# file -		Name of file to translate.

proc doFile {file} {
    global man2tclprog
    if {[catch {eval [exec $man2tclprog [glob $file]]} msg]} {
	global errorInfo
	puts stderr $msg
	puts "in"
	puts $errorInfo
	exit 1
    }
}

# doDir --
#
# Given a directory as argument, translate all the man pages in
# that directory.
#
# Arguments:
# dir -			Name of the directory.

proc doDir dir {
    puts "Generating man pages for $dir..."
    foreach f [lsort [glob -directory $dir "*.\[13n\]"]] {
	doFile $f
    }
}

# process command line arguments

if {$argc < 3} {
    puts stderr "usage: $argv0 \[options\] projectName version manFiles..."
    exit 1
}

set arg 0

if {![string compare [lindex $argv $arg] "-bitmap"]} {
    set bitmap [lindex $argv [incr arg]]
    incr arg
}
set baseName [lindex $argv $arg]
set version [lindex $argv [incr arg]]
set files {}
foreach i [lrange $argv [incr arg] end] {
    set i [file join $i]
    if {[file isdir $i]} {
	foreach f [lsort [glob -directory $i "*.\[13n\]"]] {
	    lappend files $f
	}
    } elseif {[file exists $i]} {
	lappend files $i
    }
}
source [file join [file dirname [info script]] index.tcl]
generateContents $baseName $version $files
source [file join [file dirname [info script]] man2help2.tcl]
generateHelp $baseName $files
