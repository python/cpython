# all.tcl --
#
# This file contains a top-level script to run all of the Tcl
# tests.  Execute it by invoking "source all.test" when running tcltest
# in this directory.
#
# Copyright (c) 1998-2000 by Scriptics Corporation.
# All rights reserved.
# 
# RCS: @(#) $Id$

package require Tcl 8.5
package require tcltest 2.2
::tcltest::configure \
    -testdir [file dirname [file normalize [info script]]] \
    {*}$argv
::tcltest::runAllTests
