#!/bin/sh
#\
exec tclsh "$0" ${1+"$@"}

#----------------------------------------------------------------------
#
# installData.tcl --
#
#	This file installs a hierarchy of data found in the directory
#	specified by its first argument into the directory specified
#	by its second.
#
#----------------------------------------------------------------------
#
# Copyright (c) 2004 by Kevin B. Kenny.  All rights reserved.
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#----------------------------------------------------------------------

proc copyDir {d1 d2} {

    puts [format {%*sCreating %s} [expr {4 * [info level]}] {} \
	      [file tail $d2]]

    file delete -force -- $d2
    file mkdir $d2

    foreach ftail [glob -directory $d1 -nocomplain -tails *] {
	set f [file join $d1 $ftail]
	if {[file isdirectory $f] && [string compare CVS $ftail]} {
	    copyDir $f [file join $d2 $ftail]
	} elseif {[file isfile $f]} {
	    file copy -force $f [file join $d2 $ftail]
	    if {$::tcl_platform(platform) eq {unix}} {
		file attributes [file join $d2 $ftail] -permissions 0644
	    } else {
		file attributes [file join $d2 $ftail] -readonly 1
	    }
	}
    }

    if {$::tcl_platform(platform) eq {unix}} {
	file attributes $d2 -permissions 0755
    } else {
	file attributes $d2 -readonly 1
    }

}

copyDir [file normalize [lindex $argv 0]] [file normalize [lindex $argv 1]]
