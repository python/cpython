# make_pkgIndex.tcl
#
#       Creates a pkgIndex.tcl file for in the Windows installation
#       directory
#
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# $Id: make_pkgIndex.tcl,v 1.4 2008/03/17 23:08:12 hobbs Exp $

if {[llength $argv] != 3} {
    puts "usage: $argv0 <pkgIndex.tcl> <tix dll name> <tix version>"
    exit -1
}

set fd [open [lindex $argv 0] {WRONLY TRUNC CREAT}]
puts -nonewline $fd "package ifneeded Tix [lindex $argv 2] "
puts -nonewline $fd "\[list load \[file join \$dir "
puts -nonewline $fd "[file tail [lindex $argv 1]]\] Tix\]"
puts $fd ""
puts -nonewline $fd "package ifneeded wm_default 1.0 "
puts -nonewline $fd "\[list source \[file join \$dir pref WmDefault.tcl\]\]"
puts $fd ""
close $fd
