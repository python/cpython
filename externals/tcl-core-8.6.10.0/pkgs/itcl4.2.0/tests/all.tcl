# all.tcl --
#
# This file contains a top-level script to run all of the Tcl
# tests.  Execute it by invoking "source all.test" when running tcltest
# in this directory.
#
# Copyright (c) 1998-2000 by Ajuba Solutions
# All rights reserved.

if {"-testdir" ni $argv} {
    lappend argv -testdir [file dir [info script]]
}

if {[namespace which -command memory] ne "" && "-loadfile" ni $argv} {
    puts "Tests running in sub-interpreters of leaktest circuit"
    # -loadfile overwrites -load, so save it for helper in ::env(TESTFLAGS):
    if {![info exists ::env(TESTFLAGS)] && [llength $argv]} {
        set ::env(TESTFLAGS) $argv
    }
    lappend argv -loadfile [file join [file dirname [info script]] helpers.tcl]
}

package prefer latest

package require Tcl 8.6-
package require tcltest 2.2

tcltest::configure {*}$argv
tcltest::runAllTests

return
