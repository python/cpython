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

package require Tcl 8.6
package require tcltest 2.2
::tcltest::configure \
    -testdir [file dirname [file normalize [info script]]] \
    {*}$argv
::tcltest::runAllTests
rename exit {}
