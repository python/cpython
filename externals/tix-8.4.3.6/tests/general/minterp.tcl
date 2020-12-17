# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: minterp.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# minterp.tcl
#
#	Tests Tix running under multiple interpreters.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Tests Tix running under multiple interpreters."
}

proc Test {} {
    global tix tcl_version
    if ![string comp [info commands interp] ""] {
	# Does not support multiple interpreters.
	return
    }

    if {[lsearch [package names] Itcl] != -1} {
	#
	# multiple interpreters currently core dumps under itcl2.1
	#
#	return
    }

    TestBlock minterp-1.1 {multiple interpreters} {
	for {set x 0} {$x < 5} {incr x} {
	    global testConfig
	    interp create a
	    interp eval a "set dynlib [list $testConfig(dynlib)]"
	    if {[info exists tix(et)] && $tix(et) == 1} {
		interp eval a {
		    catch {load "" Tk}
		    catch {load "" ITcl}
		    catch {load "" ITk}
		    catch {load "" Tclsam}
		    catch {load "" Tksam}
		    catch {load "" Tixsam}
		}
	    } else {
		interp eval a {
		    load "" Tk
		    load $dynlib Tix
		}
	    }
	    interp eval a {
		tixControl .d -label Test
		tixComboBox .e -label Test
		tixDirList .l
		pack .l	-expand yes -fill both
		pack .d .e -expand yes -fill both
		update
	    }
	    interp delete a
	}
    }
}
