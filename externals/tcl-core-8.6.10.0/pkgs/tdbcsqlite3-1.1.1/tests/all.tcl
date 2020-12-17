# all.tcl --
#
# This file contains a top-level script to run all of the Tcl
# tests.  Execute it by invoking "source all.test" when running tcltest
# in this directory.
#
# Copyright (c) 1998-2000 by Scriptics Corporation.
# All rights reserved.
#
# RCS: @(#) $Id: all.tcl,v 1.4 2004/07/04 22:04:20 patthoyts Exp $

if {[lsearch [namespace children] ::tcltest] == -1} {
    package require tcltest
    namespace import ::tcltest::*
}

set ::tcltest::testSingleFile false
set ::tcltest::testsDirectory [file dir [info script]]

# We need to ensure that the testsDirectory is absolute
if {[catch {::tcltest::normalizePath ::tcltest::testsDirectory}]} {
    # The version of tcltest we have here does not support
    # 'normalizePath', so we have to do this on our own.

    set oldpwd [pwd]
    catch {cd $::tcltest::testsDirectory}
    set ::tcltest::testsDirectory [pwd]
    cd $oldpwd
}

set chan $::tcltest::outputChannel

puts $chan "Tests running in interp:       [info nameofexecutable]"
puts $chan "Tests running with pwd:        [pwd]"
puts $chan "Tests running in working dir:  $::tcltest::testsDirectory"
if {[llength $::tcltest::skip] > 0} {
    puts $chan "Skipping tests that match:            $::tcltest::skip"
}
if {[llength $::tcltest::match] > 0} {
    puts $chan "Only running tests that match:        $::tcltest::match"
}

if {[llength $::tcltest::skipFiles] > 0} {
    puts $chan "Skipping test files that match:       $::tcltest::skipFiles"
}
if {[llength $::tcltest::matchFiles] > 0} {
    puts $chan "Only sourcing test files that match:  $::tcltest::matchFiles"
}

set timeCmd {clock format [clock seconds]}
puts $chan "Tests began at [eval $timeCmd]"

# source each of the specified tests
foreach file [lsort [::tcltest::getMatchingFiles]] {
    set tail [file tail $file]
    puts $chan $tail
    if {[catch {source $file} msg]} {
	puts $chan $msg
    }
}

# cleanup
puts $chan "\nTests ended at [eval $timeCmd]"
::tcltest::cleanupTests 1
return

