# all.tcl --
#
# This file contains a top-level script to run all of the Tcl
# tests.  Execute it by invoking "source all.test" when running tcltest
# in this directory.
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# Copyright (c) 2000 by Ajuba Solutions
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

package require tcltest 2.2
namespace import tcltest::*
configure {*}$argv -testdir [file dir [info script]]
if {[singleProcess]} {
    interp debug {} -frame 1
}
runAllTests
proc exit args {}
