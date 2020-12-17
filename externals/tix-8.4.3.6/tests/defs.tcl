# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
# defs.tcl --
#
#	This file contains support code for the Tcl/Tk test suite.It is
#	It is normally sourced by the individual files in the test suite
#	before they run their tests.  This improved approach to testing
#	was designed and initially implemented by Mary Ann May-Pumphrey
#	of Sun Microsystems.
#
# Copyright (c) 1990-1994 The Regents of the University of California.
# Copyright (c) 1994-1996 Sun Microsystems, Inc.
# Copyright (c) 1998-1999 by Scriptics Corporation.
# All rights reserved.
# 
# Copied from Tk 8.3.2 without change.
# Original RCS Id: defs.tcl,v 1.7 1999/12/14 06:53:12 hobbs Exp
# Tix RCS Id: $Id: defs.tcl,v 1.3 2002/11/13 21:12:17 idiscovery Exp $

# Initialize wish shell

if {[info exists tk_version]} {
    tk appname tktest
    wm title . tktest
} else {

    # Ensure that we have a minimal auto_path so we don't pick up extra junk.

    set auto_path [list [info library]]
}

# create the "tcltest" namespace for all testing variables and procedures

namespace eval tcltest {
    set procList [list test cleanupTests dotests saveState restoreState \
	    normalizeMsg makeFile removeFile makeDirectory removeDirectory \
	    viewFile bytestring set_iso8859_1_locale restore_locale \
	    safeFetch threadReap]
    if {[info exists tk_version]} {
	lappend procList setupbg dobg bgReady cleanupbg fixfocus
    }
    foreach proc $procList {
	namespace export $proc
    }

    # setup ::tcltest default vars
    foreach {var default} {verbose b match {} skip {}} {
	if {![info exists $var]} {
	    variable $var $default
	}
    }

    # Tests should not rely on the current working directory.
    # Files that are part of the test suite should be accessed relative to
    # ::tcltest::testsDir.

    set originalDir [pwd]
    set tDir [file join $originalDir [file dirname [info script]]]
    cd $tDir
    variable testsDir [pwd]
    cd $originalDir

    # Count the number of files tested (0 if all.tcl wasn't called).
    # The all.tcl file will set testSingleFile to false, so stats will
    # not be printed until all.tcl calls the cleanupTests proc.
    # The currentFailure var stores the boolean value of whether the
    # current test file has had any failures.  The failFiles list
    # stores the names of test files that had failures.

    variable numTestFiles 0
    variable testSingleFile true
    variable currentFailure false
    variable failFiles {}

    # Tests should remove all files they create.  The test suite will
    # check the current working dir for files created by the tests.
    # ::tcltest::filesMade keeps track of such files created using the
    # ::tcltest::makeFile and ::tcltest::makeDirectory procedures.
    # ::tcltest::filesExisted stores the names of pre-existing files.

    variable filesMade {}
    variable filesExisted {}

    # ::tcltest::numTests will store test files as indices and the list
    # of files (that should not have been) left behind by the test files.

    array set ::tcltest::createdNewFiles {}

    # initialize ::tcltest::numTests array to keep track fo the number of
    # tests that pass, fial, and are skipped.

    array set numTests [list Total 0 Passed 0 Skipped 0 Failed 0]

    # initialize ::tcltest::skippedBecause array to keep track of
    # constraints that kept tests from running

    array set ::tcltest::skippedBecause {}

    # tests that use thread need to know which is the main thread

    variable ::tcltest::mainThread 1
    if {[info commands testthread] != {}} {
	puts "Tk with threads enabled is known to have problems with X"
	set ::tcltest::mainThread [testthread names]
    }
}

# If there is no "memory" command (because memory debugging isn't
# enabled), generate a dummy command that does nothing.

if {[info commands memory] == ""} {
    proc memory args {}
}

# ::tcltest::initConfig --
#
# Check configuration information that will determine which tests
# to run.  To do this, create an array ::tcltest::testConfig.  Each
# element has a 0 or 1 value.  If the element is "true" then tests
# with that constraint will be run, otherwise tests with that constraint
# will be skipped.  See the README file for the list of built-in
# constraints defined in this procedure.
#
# Arguments:
#	none
#
# Results:
#	The ::tcltest::testConfig array is reset to have an index for
#	each built-in test constraint.

proc ::tcltest::initConfig {} {

    global tcl_platform tcl_interactive tk_version

    catch {unset ::tcltest::testConfig}

    # The following trace procedure makes it so that we can safely refer to
    # non-existent members of the ::tcltest::testConfig array without causing an
    # error.  Instead, reading a non-existent member will return 0.  This is
    # necessary because tests are allowed to use constraint "X" without ensuring
    # that ::tcltest::testConfig("X") is defined.

    trace variable ::tcltest::testConfig r ::tcltest::safeFetch

    proc ::tcltest::safeFetch {n1 n2 op} {
	if {($n2 != {}) && ([info exists ::tcltest::testConfig($n2)] == 0)} {
	    set ::tcltest::testConfig($n2) 0
	}
    }

    set ::tcltest::testConfig(unixOnly) \
	    [expr {$tcl_platform(platform) == "unix"}]
    set ::tcltest::testConfig(macOnly) \
	    [expr {$tcl_platform(platform) == "macintosh"}]
    set ::tcltest::testConfig(pcOnly) \
	    [expr {$tcl_platform(platform) == "windows"}]

    set ::tcltest::testConfig(unix) $::tcltest::testConfig(unixOnly)
    set ::tcltest::testConfig(mac) $::tcltest::testConfig(macOnly)
    set ::tcltest::testConfig(pc) $::tcltest::testConfig(pcOnly)

    set ::tcltest::testConfig(unixOrPc) \
	    [expr {$::tcltest::testConfig(unix) || $::tcltest::testConfig(pc)}]
    set ::tcltest::testConfig(macOrPc) \
	    [expr {$::tcltest::testConfig(mac) || $::tcltest::testConfig(pc)}]
    set ::tcltest::testConfig(macOrUnix) \
	    [expr {$::tcltest::testConfig(mac) || $::tcltest::testConfig(unix)}]

    set ::tcltest::testConfig(nt) [expr {$tcl_platform(os) == "Windows NT"}]
    set ::tcltest::testConfig(95) [expr {$tcl_platform(os) == "Windows 95"}]

    # The following config switches are used to mark tests that should work,
    # but have been temporarily disabled on certain platforms because they don't
    # and we haven't gotten around to fixing the underlying problem.

    set ::tcltest::testConfig(tempNotPc) [expr {!$::tcltest::testConfig(pc)}]
    set ::tcltest::testConfig(tempNotMac) [expr {!$::tcltest::testConfig(mac)}]
    set ::tcltest::testConfig(tempNotUnix) [expr {!$::tcltest::testConfig(unix)}]

    # The following config switches are used to mark tests that crash on
    # certain platforms, so that they can be reactivated again when the
    # underlying problem is fixed.

    set ::tcltest::testConfig(pcCrash) [expr {!$::tcltest::testConfig(pc)}]
    set ::tcltest::testConfig(macCrash) [expr {!$::tcltest::testConfig(mac)}]
    set ::tcltest::testConfig(unixCrash) [expr {!$::tcltest::testConfig(unix)}]

    # Set the "fonts" constraint for wish apps

    if {[info exists tk_version]} {
	set ::tcltest::testConfig(fonts) 1
	catch {destroy .e}
	entry .e -width 0 -font {Helvetica -12} -bd 1
	.e insert end "a.bcd"
	if {([winfo reqwidth .e] != 37) || ([winfo reqheight .e] != 20)} {
	    set ::tcltest::testConfig(fonts) 0
	}
	destroy .e
	catch {destroy .t}
	text .t -width 80 -height 20 -font {Times -14} -bd 1
	pack .t
	.t insert end "This is\na dot."
	update
	set x [list [.t bbox 1.3] [.t bbox 2.5]]
	destroy .t
	if {[string match {{22 3 6 15} {31 18 [34] 15}} $x] == 0} {
	    set ::tcltest::testConfig(fonts) 0
	}

	# Test to see if we have are running Unix apps on Exceed,
	# which won't return font failures (Windows-like), which is
	# not what we want from ann X server (other Windows X servers
	# operate as expected)

	set ::tcltest::testConfig(noExceed) 1
	if {$::tcltest::testConfig(unixOnly) && \
		[catch {font actual "\{xyz"}] == 0} {
	    puts "Running X app on Exceed, skipping problematic font tests..."
	    set ::tcltest::testConfig(noExceed) 0
	}
    }

    # Skip empty tests

    set ::tcltest::testConfig(emptyTest) 0

    # By default, tests that expost known bugs are skipped.

    set ::tcltest::testConfig(knownBug) 0

    # By default, non-portable tests are skipped.

    set ::tcltest::testConfig(nonPortable) 0

    # Some tests require user interaction.

    set ::tcltest::testConfig(userInteraction) 0

    # Some tests must be skipped if the interpreter is not in interactive mode

    set ::tcltest::testConfig(interactive) $tcl_interactive

    # Some tests must be skipped if you are running as root on Unix.
    # Other tests can only be run if you are running as root on Unix.

    set ::tcltest::testConfig(root) 0
    set ::tcltest::testConfig(notRoot) 1
    set user {}
    if {$tcl_platform(platform) == "unix"} {
	catch {set user [exec whoami]}
	if {$user == ""} {
	    catch {regexp {^[^(]*\(([^)]*)\)} [exec id] dummy user}
	}
	if {($user == "root") || ($user == "")} {
	    set ::tcltest::testConfig(root) 1
	    set ::tcltest::testConfig(notRoot) 0
	}
    }

    # Set nonBlockFiles constraint: 1 means this platform supports
    # setting files into nonblocking mode.

    if {[catch {set f [open defs r]}]} {
	set ::tcltest::testConfig(nonBlockFiles) 1
    } else {
	if {[catch {fconfigure $f -blocking off}] == 0} {
	    set ::tcltest::testConfig(nonBlockFiles) 1
	} else {
	    set ::tcltest::testConfig(nonBlockFiles) 0
	}
	close $f
    }

    # Set asyncPipeClose constraint: 1 means this platform supports
    # async flush and async close on a pipe.
    #
    # Test for SCO Unix - cannot run async flushing tests because a
    # potential problem with select is apparently interfering.
    # (Mark Diekhans).

    if {$tcl_platform(platform) == "unix"} {
	if {[catch {exec uname -X | fgrep {Release = 3.2v}}] == 0} {
	    set ::tcltest::testConfig(asyncPipeClose) 0
	} else {
	    set ::tcltest::testConfig(asyncPipeClose) 1
	}
    } else {
	set ::tcltest::testConfig(asyncPipeClose) 1
    }

    # Test to see if we have a broken version of sprintf with respect
    # to the "e" format of floating-point numbers.

    set ::tcltest::testConfig(eformat) 1
    if {[string compare "[format %g 5e-5]" "5e-05"] != 0} {
	set ::tcltest::testConfig(eformat) 0
    }

    # Test to see if execed commands such as cat, echo, rm and so forth are
    # present on this machine.

    set ::tcltest::testConfig(unixExecs) 1
    if {$tcl_platform(platform) == "macintosh"} {
	set ::tcltest::testConfig(unixExecs) 0
    }
    if {($::tcltest::testConfig(unixExecs) == 1) && \
	    ($tcl_platform(platform) == "windows")} {
	if {[catch {exec cat defs}] == 1} {
	    set ::tcltest::testConfig(unixExecs) 0
	}
	if {($::tcltest::testConfig(unixExecs) == 1) && \
		([catch {exec echo hello}] == 1)} {
	    set ::tcltest::testConfig(unixExecs) 0
	}
	if {($::tcltest::testConfig(unixExecs) == 1) && \
		([catch {exec sh -c echo hello}] == 1)} {
	    set ::tcltest::testConfig(unixExecs) 0
	}
	if {($::tcltest::testConfig(unixExecs) == 1) && \
		([catch {exec wc defs}] == 1)} {
	    set ::tcltest::testConfig(unixExecs) 0
	}
	if {$::tcltest::testConfig(unixExecs) == 1} {
	    exec echo hello > removeMe
	    if {[catch {exec rm removeMe}] == 1} {
		set ::tcltest::testConfig(unixExecs) 0
	    }
	}
	if {($::tcltest::testConfig(unixExecs) == 1) && \
		([catch {exec sleep 1}] == 1)} {
	    set ::tcltest::testConfig(unixExecs) 0
	}
	if {($::tcltest::testConfig(unixExecs) == 1) && \
		([catch {exec fgrep unixExecs defs}] == 1)} {
	    set ::tcltest::testConfig(unixExecs) 0
	}
	if {($::tcltest::testConfig(unixExecs) == 1) && \
		([catch {exec ps}] == 1)} {
	    set ::tcltest::testConfig(unixExecs) 0
	}
	if {($::tcltest::testConfig(unixExecs) == 1) && \
		([catch {exec echo abc > removeMe}] == 0) && \
		([catch {exec chmod 644 removeMe}] == 1) && \
		([catch {exec rm removeMe}] == 0)} {
	    set ::tcltest::testConfig(unixExecs) 0
	} else {
	    catch {exec rm -f removeMe}
	}
	if {($::tcltest::testConfig(unixExecs) == 1) && \
		([catch {exec mkdir removeMe}] == 1)} {
	    set ::tcltest::testConfig(unixExecs) 0
	} else {
	    catch {exec rm -r removeMe}
	}
    }
}

::tcltest::initConfig


# ::tcltest::processCmdLineArgs --
#
#	Use command line args to set the verbose, skip, and
#	match variables.  This procedure must be run after
#	constraints are initialized, because some constraints can be
#	overridden.
#
# Arguments:
#	none
#
# Results:
#	::tcltest::verbose is set to <value>

proc ::tcltest::processCmdLineArgs {} {
    global argv

    # The "argv" var doesn't exist in some cases, so use {}
    # The "argv" var doesn't exist in some cases.

    if {(![info exists argv]) || ([llength $argv] < 2)} {
	set flagArray {}
    } else {
	set flagArray $argv
    }

    if {[catch {array set flag $flagArray}]} {
	puts stderr "Error:  odd number of command line args specified:"
	puts stderr "        $argv"
	exit
    }
    
    # Allow for 1-char abbreviations, where applicable (e.g., -match == -m).
    # Note that -verbose cannot be abbreviated to -v in wish because it
    # conflicts with the wish option -visual.

    foreach arg {-verbose -match -skip -constraints} {
	set abbrev [string range $arg 0 1]
	if {([info exists flag($abbrev)]) && \
		([lsearch -exact $flagArray $arg] < \
		[lsearch -exact $flagArray $abbrev])} {
	    set flag($arg) $flag($abbrev)
	}
    }

    # Set ::tcltest::workingDir to [pwd].
    # Save the names of files that already exist in ::tcltest::workingDir.

    set ::tcltest::workingDir [pwd]
    foreach file [glob -nocomplain [file join $::tcltest::workingDir *]] {
	lappend ::tcltest::filesExisted [file tail $file]
    }

    # Set ::tcltest::verbose to the arg of the -verbose flag, if given

    if {[info exists flag(-verbose)]} {
	set ::tcltest::verbose $flag(-verbose)
    }

    # Set ::tcltest::match to the arg of the -match flag, if given

    if {[info exists flag(-match)]} {
	set ::tcltest::match $flag(-match)
    }

    # Set ::tcltest::skip to the arg of the -skip flag, if given

    if {[info exists flag(-skip)]} {
	set ::tcltest::skip $flag(-skip)
    }

    # Use the -constraints flag, if given, to turn on constraints that are
    # turned off by default: userInteractive knownBug nonPortable.  This
    # code fragment must be run after constraints are initialized.

    if {[info exists flag(-constraints)]} {
	foreach elt $flag(-constraints) {
	    set ::tcltest::testConfig($elt) 1
	}
    }
}

::tcltest::processCmdLineArgs


# ::tcltest::cleanupTests --
#
# Remove files and dirs created using the makeFile and makeDirectory
# commands since the last time this proc was invoked.
#
# Print the names of the files created without the makeFile command
# since the tests were invoked.
#
# Print the number tests (total, passed, failed, and skipped) since the
# tests were invoked.
#

proc ::tcltest::cleanupTests {{calledFromAllFile 0}} {
    set tail [file tail [info script]]

    # Remove files and directories created by the :tcltest::makeFile and
    # ::tcltest::makeDirectory procedures.
    # Record the names of files in ::tcltest::workingDir that were not
    # pre-existing, and associate them with the test file that created them.

    if {!$calledFromAllFile} {

	foreach file $::tcltest::filesMade {
	    if {[file exists $file]} {
		catch {file delete -force $file}
	    }
	}
	set currentFiles {}
	foreach file [glob -nocomplain [file join $::tcltest::workingDir *]] {
	    lappend currentFiles [file tail $file]
	}
	set newFiles {}
	foreach file $currentFiles {
	    if {[lsearch -exact $::tcltest::filesExisted $file] == -1} {
		lappend newFiles $file
	    }
	}
	set ::tcltest::filesExisted $currentFiles
	if {[llength $newFiles] > 0} {
	    set ::tcltest::createdNewFiles($tail) $newFiles
	}
    }

    if {$calledFromAllFile || $::tcltest::testSingleFile} {

	# print stats

	puts -nonewline stdout "$tail:"
	foreach index [list "Total" "Passed" "Skipped" "Failed"] {
	    puts -nonewline stdout "\t$index\t$::tcltest::numTests($index)"
	}
	puts stdout ""

	# print number test files sourced
	# print names of files that ran tests which failed

	if {$calledFromAllFile} {
	    puts stdout "Sourced $::tcltest::numTestFiles Test Files."
	    set ::tcltest::numTestFiles 0
	    if {[llength $::tcltest::failFiles] > 0} {
		puts stdout "Files with failing tests: $::tcltest::failFiles"
		set ::tcltest::failFiles {}
	    }
	}

	# if any tests were skipped, print the constraints that kept them
	# from running.

	set constraintList [array names ::tcltest::skippedBecause]
	if {[llength $constraintList] > 0} {
	    puts stdout "Number of tests skipped for each constraint:"
	    foreach constraint [lsort $constraintList] {
		puts stdout \
			"\t$::tcltest::skippedBecause($constraint)\t$constraint"
		unset ::tcltest::skippedBecause($constraint)
	    }
	}

	# report the names of test files in ::tcltest::createdNewFiles, and
	# reset the array to be empty.

	set testFilesThatTurded [lsort [array names ::tcltest::createdNewFiles]]
	if {[llength $testFilesThatTurded] > 0} {
	    puts stdout "Warning: test files left files behind:"
	    foreach testFile $testFilesThatTurded {
		puts "\t$testFile:\t$::tcltest::createdNewFiles($testFile)"
		unset ::tcltest::createdNewFiles($testFile)
	    }
	}

	# reset filesMade, filesExisted, and numTests

	set ::tcltest::filesMade {}
	foreach index [list "Total" "Passed" "Skipped" "Failed"] {
	    set ::tcltest::numTests($index) 0
	}

	# exit only if running Tk in non-interactive mode

	global tk_version tcl_interactive
	if {[info exists tk_version] && !$tcl_interactive} {
	    exit
	}
    } else {

	# if we're deferring stat-reporting until all files are sourced,
	# then add current file to failFile list if any tests in this file
	# failed

	incr ::tcltest::numTestFiles
	if {($::tcltest::currentFailure) && \
		([lsearch -exact $::tcltest::failFiles $tail] == -1)} {
	    lappend ::tcltest::failFiles $tail
	}
	set ::tcltest::currentFailure false
    }
}


# test --
#
# This procedure runs a test and prints an error message if the test fails.
# If ::tcltest::verbose has been set, it also prints a message even if the
# test succeeds.  The test will be skipped if it doesn't match the
# ::tcltest::match variable, if it matches an element in
# ::tcltest::skip, or if one of the elements of "constraints" turns
# out not to be true.
#
# Arguments:
# name -		Name of test, in the form foo-1.2.
# description -		Short textual description of the test, to
#			help humans understand what it does.
# constraints -		A list of one or more keywords, each of
#			which must be the name of an element in
#			the array "::tcltest::testConfig".  If any of these
#			elements is zero, the test is skipped.
#			This argument may be omitted.
# script -		Script to run to carry out the test.  It must
#			return a result that can be checked for
#			correctness.
# expectedAnswer -	Expected result from script.

proc ::tcltest::test {name description script expectedAnswer args} {
    incr ::tcltest::numTests(Total)

    # skip the test if it's name matches an element of skip

    foreach pattern $::tcltest::skip {
	if {[string match $pattern $name]} {
	    incr ::tcltest::numTests(Skipped)
	    return
	}
    }
    # skip the test if it's name doesn't match any element of match

    if {[llength $::tcltest::match] > 0} {
	set ok 0
	foreach pattern $::tcltest::match {
	    if {[string match $pattern $name]} {
		set ok 1
		break
	    }
        }
	if {!$ok} {
	    incr ::tcltest::numTests(Skipped)
	    return
	}
    }
    set i [llength $args]
    if {$i == 0} {
	set constraints {}
    } elseif {$i == 1} {

	# "constraints" argument exists;  shuffle arguments down, then
	# make sure that the constraints are satisfied.

	set constraints $script
	set script $expectedAnswer
	set expectedAnswer [lindex $args 0]
	set doTest 0
	if {[string match {*[$\[]*} $constraints] != 0} {

	    # full expression, e.g. {$foo > [info tclversion]}

	    catch {set doTest [uplevel #0 expr $constraints]}

	} elseif {[regexp {[^.a-zA-Z0-9 ]+} $constraints] != 0} {

	    # something like {a || b} should be turned into 
	    # $::tcltest::testConfig(a) || $::tcltest::testConfig(b).

 	    regsub -all {[.a-zA-Z0-9]+} $constraints \
		    {$::tcltest::testConfig(&)} c
	    catch {set doTest [eval expr $c]}
	} else {

	    # just simple constraints such as {unixOnly fonts}.

	    set doTest 1
	    foreach constraint $constraints {
		if {![info exists ::tcltest::testConfig($constraint)]
			|| !$::tcltest::testConfig($constraint)} {
		    set doTest 0

		    # store the constraint that kept the test from running

		    set constraints $constraint
		    break
		}
	    }
	}
	if {$doTest == 0} {
	    incr ::tcltest::numTests(Skipped)
	    if {[string first s $::tcltest::verbose] != -1} {
		puts stdout "++++ $name SKIPPED: $constraints"
	    }

	    # add the constraint to the list of constraints the kept tests
	    # from running

	    if {[info exists ::tcltest::skippedBecause($constraints)]} {
		incr ::tcltest::skippedBecause($constraints)
	    } else {
		set ::tcltest::skippedBecause($constraints) 1
	    }
	    return	
	}
    } else {
	error "wrong # args: must be \"test name description ?constraints? script expectedAnswer\""
    }
    memory tag $name
    set code [catch {uplevel $script} actualAnswer]
    if {$code != 0 || [string compare $actualAnswer $expectedAnswer] != 0} {
	incr ::tcltest::numTests(Failed)
	set ::tcltest::currentFailure true
	if {[string first b $::tcltest::verbose] == -1} {
	    set script ""
	}
	puts stdout "\n==== $name $description FAILED"
	if {$script != ""} {
	    puts stdout "==== Contents of test case:"
	    puts stdout $script
	}
	if {$code != 0} {
	    if {$code == 1} {
		puts stdout "==== Test generated error:"
		puts stdout $actualAnswer
	    } elseif {$code == 2} {
		puts stdout "==== Test generated return exception;  result was:"
		puts stdout $actualAnswer
	    } elseif {$code == 3} {
		puts stdout "==== Test generated break exception"
	    } elseif {$code == 4} {
		puts stdout "==== Test generated continue exception"
	    } else {
		puts stdout "==== Test generated exception $code;  message was:"
		puts stdout $actualAnswer
	    }
	} else {
	    puts stdout "---- Result was:\n$actualAnswer"
	}
	puts stdout "---- Result should have been:\n$expectedAnswer"
	puts stdout "==== $name FAILED\n" 
    } else { 
	incr ::tcltest::numTests(Passed)
	if {[string first p $::tcltest::verbose] != -1} {
	    puts stdout "++++ $name PASSED"
	}
    }
}

# ::tcltest::dotests --
#
#	takes two arguments--the name of the test file (such
#	as "parse.test"), and a pattern selecting the tests you want to
#	execute.  It sets ::tcltest::matching to the second argument, calls
#	"source" on the file specified in the first argument, and restores
#	::tcltest::matching to its pre-call value at the end.
#
# Arguments:
#	file    name of tests file to source
#	args    pattern selecting the tests you want to execute
#
# Results:
#	none

proc ::tcltest::dotests {file args} {
    set savedTests $::tcltest::match
    set ::tcltest::match $args
    source $file
    set ::tcltest::match $savedTests
}

proc ::tcltest::openfiles {} {
    if {[catch {testchannel open} result]} {
	return {}
    }
    return $result
}

proc ::tcltest::leakfiles {old} {
    if {[catch {testchannel open} new]} {
        return {}
    }
    set leak {}
    foreach p $new {
    	if {[lsearch $old $p] < 0} {
	    lappend leak $p
	}
    }
    return $leak
}

set ::tcltest::saveState {}

proc ::tcltest::saveState {} {
    uplevel #0 {set ::tcltest::saveState [list [info procs] [info vars]]}
}

proc ::tcltest::restoreState {} {
    foreach p [info procs] {
	if {[lsearch [lindex $::tcltest::saveState 0] $p] < 0} {
	    rename $p {}
	}
    }
    foreach p [uplevel #0 {info vars}] {
	if {[lsearch [lindex $::tcltest::saveState 1] $p] < 0} {
	    uplevel #0 "unset $p"
	}
    }
}

proc ::tcltest::normalizeMsg {msg} {
    regsub "\n$" [string tolower $msg] "" msg
    regsub -all "\n\n" $msg "\n" msg
    regsub -all "\n\}" $msg "\}" msg
    return $msg
}

# makeFile --
#
# Create a new file with the name <name>, and write <contents> to it.
#
# If this file hasn't been created via makeFile since the last time
# cleanupTests was called, add it to the $filesMade list, so it will
# be removed by the next call to cleanupTests.
#
proc ::tcltest::makeFile {contents name} {
    set fd [open $name w]
    fconfigure $fd -translation lf
    if {[string index $contents [expr {[string length $contents] - 1}]] == "\n"} {
	puts -nonewline $fd $contents
    } else {
	puts $fd $contents
    }
    close $fd

    set fullName [file join [pwd] $name]
    if {[lsearch -exact $::tcltest::filesMade $fullName] == -1} {
	lappend ::tcltest::filesMade $fullName
    }
}

proc ::tcltest::removeFile {name} {
    file delete $name
}

# makeDirectory --
#
# Create a new dir with the name <name>.
#
# If this dir hasn't been created via makeDirectory since the last time
# cleanupTests was called, add it to the $directoriesMade list, so it will
# be removed by the next call to cleanupTests.
#
proc ::tcltest::makeDirectory {name} {
    file mkdir $name

    set fullName [file join [pwd] $name]
    if {[lsearch -exact $::tcltest::filesMade $fullName] == -1} {
	lappend ::tcltest::filesMade $fullName
    }
}

proc ::tcltest::removeDirectory {name} {
    file delete -force $name
}

proc ::tcltest::viewFile {name} {
    global tcl_platform
    if {($tcl_platform(platform) == "macintosh") || \
		($::tcltest::testConfig(unixExecs) == 0)} {
	set f [open $name]
	set data [read -nonewline $f]
	close $f
	return $data
    } else {
	exec cat $name
    }
}

#
# Construct a string that consists of the requested sequence of bytes,
# as opposed to a string of properly formed UTF-8 characters.  
# This allows the tester to 
# 1. Create denormalized or improperly formed strings to pass to C procedures 
#    that are supposed to accept strings with embedded NULL bytes.
# 2. Confirm that a string result has a certain pattern of bytes, for instance
#    to confirm that "\xe0\0" in a Tcl script is stored internally in 
#    UTF-8 as the sequence of bytes "\xc3\xa0\xc0\x80".
#
# Generally, it's a bad idea to examine the bytes in a Tcl string or to
# construct improperly formed strings in this manner, because it involves
# exposing that Tcl uses UTF-8 internally.

proc ::tcltest::bytestring {string} {
    encoding convertfrom identity $string
}

# Locate tcltest executable

if {![info exists tk_version]} {
    set tcltest [info nameofexecutable]

    if {$tcltest == "{}"} {
	set tcltest {}
    }
}

set thisdir [file dirname [info script]]
set ::tcltest::testConfig(stdio) 0
catch {
    catch {file delete -force [file join $thisdir tmp]}
    set f [open [file join $thisdir tmp] w]
    puts $f {
	exit
    }
    close $f

    set f [open "|[list $tcltest [file join $thisdir tmp]]" r]
    close $f
    
    set ::tcltest::testConfig(stdio) 1
}
catch {file delete -force [file join $thisdir tmp]}

# Deliberately call the socket with the wrong number of arguments.  The error
# message you get will indicate whether sockets are available on this system.

catch {socket} msg
set ::tcltest::testConfig(socket) \
	[expr {$msg != "sockets are not available on this system"}]

#
# Internationalization / ISO support procs     -- dl
#

if {[info commands testlocale]==""} {

    # No testlocale command, no tests...
    # (it could be that we are a sub interp and we could just load
    # the Tcltest package but that would interfere with tests
    # that tests packages/loading in slaves...)

    set ::tcltest::testConfig(hasIsoLocale) 0
} else {
    proc ::tcltest::set_iso8859_1_locale {} {
	set ::tcltest::previousLocale [testlocale ctype]
	testlocale ctype $::tcltest::isoLocale
    }

    proc ::tcltest::restore_locale {} {
	testlocale ctype $::tcltest::previousLocale
    }

    if {![info exists ::tcltest::isoLocale]} {
	set ::tcltest::isoLocale fr
        switch $tcl_platform(platform) {
	    "unix" {

		# Try some 'known' values for some platforms:

		switch -exact -- $tcl_platform(os) {
		    "FreeBSD" {
			set ::tcltest::isoLocale fr_FR.ISO_8859-1
		    }
		    HP-UX {
			set ::tcltest::isoLocale fr_FR.iso88591
		    }
		    Linux -
		    IRIX {
			set ::tcltest::isoLocale fr
		    }
		    default {

			# Works on SunOS 4 and Solaris, and maybe others...
			# define it to something else on your system
			#if you want to test those.

			set ::tcltest::isoLocale iso_8859_1
		    }
		}
	    }
	    "windows" {
		set ::tcltest::isoLocale French
	    }
	}
    }

    set ::tcltest::testConfig(hasIsoLocale) \
	    [string length [::tcltest::set_iso8859_1_locale]]
    ::tcltest::restore_locale
} 

#
# procedures that are Tk specific
#

if {[info exists tk_version]} {

    # If the main window isn't already mapped (e.g. because the tests are
    # being run automatically) , specify a precise size for it so that the
    # user won't have to position it manually.

    if {![winfo ismapped .]} {
	wm geometry . +0+0
	update
    }

    # The following code can be used to perform tests involving a second
    # process running in the background.
    
    # Locate the tktest executable

    set ::tcltest::tktest [info nameofexecutable]
    if {$::tcltest::tktest == "{}"} {
	set ::tcltest::tktest {}
	puts stdout \
		"Unable to find tktest executable, skipping multiple process tests."
    }

    # Create background process
    
    proc ::tcltest::setupbg args {
	if {$::tcltest::tktest == ""} {
	    error "you're not running tktest so setupbg should not have been called"
	}
	if {[info exists ::tcltest::fd] && ($::tcltest::fd != "")} {
	    cleanupbg
	}
	
	# The following code segment cannot be run on Windows prior
	# to Tk 8.1b3 due to a channel I/O bug (bugID 1495).

	global tcl_platform
	set ::tcltest::fd [open "|[list $::tcltest::tktest -geometry +0+0 -name tktest] $args" r+]
	puts $::tcltest::fd "puts foo; flush stdout"
	flush $::tcltest::fd
	if {[gets $::tcltest::fd data] < 0} {
	    error "unexpected EOF from \"$::tcltest::tktest\""
	}
	if {[string compare $data foo]} {
	    error "unexpected output from background process \"$data\""
	}
	fileevent $::tcltest::fd readable bgReady
    }
    
    # Send a command to the background process, catching errors and
    # flushing I/O channels

    proc ::tcltest::dobg {command} {
	puts $::tcltest::fd "catch [list $command] msg; update; puts \$msg; puts **DONE**; flush stdout"
	flush $::tcltest::fd
	set ::tcltest::bgDone 0
	set ::tcltest::bgData {}
	tkwait variable ::tcltest::bgDone
	set ::tcltest::bgData
    }

    # Data arrived from background process.  Check for special marker
    # indicating end of data for this command, and make data available
    # to dobg procedure.

    proc ::tcltest::bgReady {} {
	set x [gets $::tcltest::fd]
	if {[eof $::tcltest::fd]} {
	    fileevent $::tcltest::fd readable {}
	    set ::tcltest::bgDone 1
	} elseif {$x == "**DONE**"} {
	    set ::tcltest::bgDone 1
	} else {
	    append ::tcltest::bgData $x
	}
    }

    # Exit the background process, and close the pipes

    proc ::tcltest::cleanupbg {} {
	catch {
	    puts $::tcltest::fd "exit"
	    close $::tcltest::fd
	}
	set ::tcltest::fd ""
    }

    # Clean up focus after using generate event, which
    # can leave the window manager with the wrong impression
    # about who thinks they have the focus. (BW)
    
    proc ::tcltest::fixfocus {} {
	catch {destroy .focus}
	toplevel .focus
	wm geometry .focus +0+0
	entry .focus.e
	.focus.e insert 0 "fixfocus"
	pack .focus.e
	update
	focus -force .focus.e
	destroy .focus
    }
}

# threadReap --
#
#	Kill all threads except for the main thread.
#	Do nothing if testthread is not defined.
#
# Arguments:
#	none.
#
# Results:
#	Returns the number of existing threads.

if {[info commands testthread] != {}} {
    proc ::tcltest::threadReap {} {
	testthread errorproc ThreadNullError
	while {[llength [testthread names]] > 1} {
	    foreach tid [testthread names] {
		if {$tid != $::tcltest::mainThread} {
		    catch {testthread send -async $tid {testthread exit}}
		    update
		}
	    }
	}
	testthread errorproc ThreadError
	return [llength [testthread names]]
    }
} else {
    proc ::tcltest::threadReap {} {
	return 1
    }   
}

# Need to catch the import because it fails if defs.tcl is sourced
# more than once.

catch {namespace import ::tcltest::*}
return
