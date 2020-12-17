#! /usr/bin/env tclsh

package ifneeded tcltests 0.1 "
    source [list $dir/tcltests.tcl]
    package provide tcltests 0.1
"
