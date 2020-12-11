# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Test.tcl,v 1.2 2002/11/13 21:12:17 idiscovery Exp $
#
#! /bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

# Test.tcl --
#
#	This file executes the Tix test suite for the Unix platform.
#	Don't execute this file directly. Read the README file in this
#	directory first.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

set targets  [lindex $argv 0]
set argvfiles [lrange $argv 1 end]

set env(WAITTIME) 200

set genDirs {
    general xpm hlist
}

set env(TCL_LIBRARY) 	$env(TEST_TCL_LIBRARY)
set env(TK_LIBRARY) 	$env(TEST_TK_LIBRARY)
set env(ITCL_LIBRARY) 	$env(TEST_ITCL_LIBRARY)
set env(ITK_LIBRARY) 	$env(TEST_ITK_LIBRARY)
set BINSRC_DIR		$env(TEST_BINSRC_DIR)

catch {
    unset env(TIX_DEBUG_INTERACTIVE)
}

set load(bin)   $BINSRC_DIR/../tk4.1/unix/wish
set tk40(bin)   $BINSRC_DIR/unix-tk4.0/tixwish
set tk41(bin)   $BINSRC_DIR/unix-tk4.1/tixwish
set tk42(bin)   $BINSRC_DIR/unix-tk4.2/tixwish
set itcl20(bin) $BINSRC_DIR/unix-itcl2.0/itixwish
set itcl21(bin) $BINSRC_DIR/unix-itcl2.1/itixwish

if ![info exists env(LD_LIBRARY_PATH)] {
    set env(LD_LIBRARY_PATH) ""
}
if [info exists env(TEST_LDPATHS)] {
    set env(LD_LIBRARY_PATH) $env(TEST_LDPATHS):$env(LD_LIBRARY_PATH)
}

foreach t $targets {
    upvar #0 $t target

    puts "Executing ---\n"
    puts "env TCL_LIBRARY=$env(TCL_LIBRARY) TK_LIBRARY=$env(TK_LIBRARY) ITCL_LIBRARY=$env(ITCL_LIBRARY) ITK_LIBRARY=$env(ITK_LIBRARY) LD_LIBRARY_PATH=$env(LD_LIBRARY_PATH) TIX_LIBRARY=$env(TIX_LIBRARY) $target(bin)"
    puts ""


    puts "Testing target $t with executable $target(bin)"
    eval exec $target(bin) Driver.tcl $argvfiles >@ stdout 2>@ stderr
}
