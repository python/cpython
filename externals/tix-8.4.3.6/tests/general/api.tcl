# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: api.tcl,v 1.2 2002/11/13 21:12:17 idiscovery Exp $
#
# api.tcl --
#
#	Performs a comprehensive test on all the Tix widgets and
#	commands. This test knows the types and arguments of many
#	common Tix widget methods. It calls each widget method and
#	ensure that it work as expected.
#
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

set depd(init)	       ""
set info(init)         "Initialization, find out all the widget classes"
set depd(wcreate)      "init"
set info(wcreate)      "Try to create each widget"
set depd(method)       "init wcreate"
set info(method)       "Try to call each public method of all widgets"
set depd(config-state) "init wcreate method"
set info(config-state) "Configuring -state of widgets"

proc APITest:init {} {
    global widCmd cmdNames auto_index testConfig

    TestBlock api-1.1 {Find out all the widget classes} {
	# (1) Stores all the Tix commands in the associative array 
	#     cmdNames
	#
	foreach cmd [info commands tix*] {
	    if [regexp : $cmd] {
		continue
	    }
	    set cmdNames($cmd) ""
	}

	foreach name [array names auto_index "tix*:AutoLoad"] {
	    if [regsub {:AutoLoad} $name "" cmd] {
		set cmdNames($cmd) ""
	    }
	}

	# (3). Don't want to mess with the console routines
	#
	foreach name [array names cmdNames] {
	    if [string match tixCon* $name] {
		catch {
		    unset cmdNames($name)
		}
	    }
	}

	# (2) Find out the names of the widget creation commands
	#
	foreach cmd [lsort [array names cmdNames]] {
	    if [info exists $cmd\(superClass\)] {
		if {[set $cmd\(superClass\)] == ""} {
		    continue
		}
	    }
	    switch -regexp -- $cmd {
		{(DoWhenIdle)|(:)} {
		    continue
		}
	    }

	    if [info exists err] {
		unset err
	    }

	    catch {
		auto_load $cmd
	    }
	    catch {
		if {[uplevel #0 set $cmd\(isWidget\)] == 1} {
		    if {$testConfig(VERBOSE) > 20} {
			puts "Found widget class: $cmd"
		    }
		    set widCmd($cmd) ""
		}
	    }
	}
    }
}

proc APITest:wcreate {} {
    global widCmd testConfig

    TestBlock api-2 {Find out all the widget classes} {
	foreach cls [lsort [array names widCmd]] {
	    if {[uplevel #0 set $cls\(virtual\)] == 1} {
		# This is a virtual base class. Skip it.
		#
		continue
	    }

	    TestBlock api-2.1-$cls "Create widget of class: $cls" {
		$cls .c
		if ![tixStrEq [winfo toplevel .c] .c] {
		    pack .c -expand yes -fill both
		}
		update
	    }

	    TestBlock api-2.2-$cls "Widget Deletion" {
		catch {
		    destroy .c
		}

		frame .c
		update idletasks
		global .c
		if {[info exists .c] && [array names .c] != "context"} {
		    catch {
			parray .c
		    }
		    catch {
			puts [set .c]
		    }
		    error "widget record has not been deleted properly"
		}
	    }
	    catch {
		destroy .c
	    }
	}
    }
}

proc APITest:method {} {
    global widCmd testConfig

    TestBlock api-3 {Call all the methods of a widget class} {

	foreach cls [lsort [array names widCmd]] {
	    if {[uplevel #0 set $cls\(virtual\)] == 1} {
		continue
	    }

	    TestBlock api-3.1-$cls "Widget class: $cls" {
		$cls .c

		upvar #0 $cls classRec
		foreach method [lsort $classRec(methods)] {
		    TestBlock api-3.1.1 "method: $method" {
			catch {
			    .c $method
			}
		    }
		}
	    }
	    catch {
		destroy .c
	    }
	}
    }
}

proc APITest:config-state {} {
    global widCmd testConfig

    TestBlock api-4 {Call the config-state method} {

	foreach cls [lsort [array names widCmd]] {
	    if {[uplevel #0 set $cls\(virtual\)] == 1} {
		continue
	    }

	    $cls .c
	    catch {
		pack .c
	    }
	    if [catch {.c cget -state}] {
		destroy .c
		continue
	    }

	    if [tixStrEq $cls tixBalloon] {
		destroy .c
		continue
	    }

	    TestBlock api-4.1-$cls "Class: $cls" {
		.c config -state disabled
		Assert {[tixStrEq [.c cget -state] "disabled"]}
		update
		Assert {[tixStrEq [.c cget -state] "disabled"]}

		.c config -state normal
		Assert {[tixStrEq [.c cget -state] "normal"]}
		update
		Assert {[tixStrEq [.c cget -state] "normal"]}


		.c config -state disabled
		Assert {[tixStrEq [.c cget -state] "disabled"]}
		.c config -state normal
		Assert {[tixStrEq [.c cget -state] "normal"]}

	    }
	    catch {
		destroy .c; update
	    }
	}
    }
}

proc APITest {t {level 0}} {
    global depd tested info

    if {$level > 300} {
	error "possibly circular dependency"
    }

    set tested(none)  1

    if [info exist tested($t)] {
	return
    }
    foreach dep $depd($t) {
	if {![info exists tested($dep)]} {
	    APITest $dep [expr $level + 1]
	}
    }

    if {$t == "all"} {
	set tested($t) 1
	return
    } else {
	update
	eval APITest:$t
	set tested($t) 1
    }
}

proc About {} {
    return "Tix API Testing Suite"
}

proc Test {} {
    global depd env

    if [info exists env(APT_SUBSET)] {
	set tests $env(APT_SUBSET)
    } else {
	set tests [array names depd]
    }

    foreach test $tests {
	APITest $test
    }
}

