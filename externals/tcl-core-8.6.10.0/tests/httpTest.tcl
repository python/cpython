# httpTest.tcl
#
#	Test HTTP/1.1 concurrent requests including
#	queueing, pipelining and retries.
#
# Copyright (C) 2018 Keith Nash <kjnash@users.sourceforge.net>
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

# ------------------------------------------------------------------------------
# "Package" httpTest for analysis of Log output of http requests.
# ------------------------------------------------------------------------------
# This is a specialised test kit for examining the presence, ordering, and
# overlap of multiple HTTP transactions over a persistent ("Keep-Alive")
# connection; and also for testing reconnection in accordance with RFC 7230 when
# the connection is lost.
#
# This kit is probably not useful for other purposes.  It depends on the
# presence of specific Log commands in the http library, and it interprets the
# logs that these commands create.
# ------------------------------------------------------------------------------

package require http

namespace eval ::http {
    variable TestStartTimeInMs [clock milliseconds]
#    catch {puts stdout "Start time (zero ms) is $TestStartTimeInMs"}
}

namespace eval ::httpTest {
    variable testResults {}
    variable testOptions
    array set testOptions {
        -verbose 0
        -dotted  1
    }
    # -verbose - 0 quiet 1 write to stdout 2 write more
    # -dotted  - (boolean) use dots for absences in lists of transactions
}

proc httpTest::Puts {txt} {
    variable testOptions
    if {$testOptions(-verbose) > 0} {
        puts stdout $txt
        flush stdout
    }
    return
}

# http::Log
#
# A special-purpose logger used for running tests.
# - Processes Log calls that have "^" in their arguments, and records them in
#   variable ::httpTest::testResults.
# - Also writes them to stdout (using Puts) if ($testOptions(-verbose) > 0).
# - Also writes Log calls that do not have "^", if ($testOptions(-verbose) > 1).

proc http::Log {args} {
    variable TestStartTimeInMs
    set time [expr {[clock milliseconds] - $TestStartTimeInMs}]
    set txt [list $time {*}$args]
    if {[string first ^ $txt] != -1} {
        ::httpTest::LogRecord $txt
        ::httpTest::Puts $txt
    } elseif {$::httpTest::testOptions(-verbose) > 1} {
        ::httpTest::Puts $txt
    }
    return
}


# Called by http::Log (the "testing" version) to record logs for later analysis.

proc httpTest::LogRecord {txt} {
    variable testResults

    set pos [string first ^ $txt]
    set len [string length  $txt]
    if {$pos > $len - 3} {
        puts stdout "Logging Error: $txt"
        puts stdout "Fix this call to Log in http-*.tm so it has ^ then\
		a letter then a numeral."
        flush stdout
    } elseif {$pos == -1} {
        # Called by mistake.
    } else {
        set letter [string index $txt [incr pos]]
        set number [string index $txt [incr pos]]
        # Max 9 requests!
        lappend testResults [list $letter $number]
    }

    return
}


# ------------------------------------------------------------------------------
# Commands for analysing the logs recorded when calling http::geturl.
# ------------------------------------------------------------------------------

# httpTest::TestOverlaps --
#
# The main test for correct behaviour of pipelined and sequential
# (non-pipelined) transactions.  Other tests should be run first to detect
# any inconsistencies in the data (e.g. absence of the elements that are
# examined here).
#
# Examine the sequence $someResults for each transaction from 1 to $n,
# ignoring any that are listed in $badTrans.
# Determine whether the elements "B" to $term for one transaction overlap
# elements "B" to $term for the previous and following transactions.
#
# Transactions in the list $badTrans are not included in "clean" or
# "dirty", but their possible overlap with other transactions is noted.
# Transactions in the list $notPiped are a subset of $badTrans, and
# their possible overlap with other transactions is NOT noted.
#
# Arguments:
# someResults - list of results, each of the form {letter numeral}
# n           - number of HTTP transactions
# term        - letter that indicated end of search range. "E" for testing
#               overlaps from start of request to end of response headers.
#               "F" to extend to the end of the response body.
# msg         - the cumulative message from sanity checks.  Append to it only
#               to report a test failure.
# badTrans    - list of transaction numbers not to be assessed as "clean" or
#               "dirty"
# notPiped    - subset of badTrans.  List of transaction numbers that cannot
#               taint another transaction by overlapping with it, because it
#               used a different socket.
#
# Return value: [list $msg $clean $dirty]
# msg   - warning messages: nothing will be appended to argument $msg if there
#         is an error with the test.
# clean - list of transactions that have no overlap with other transactions
# dirty - list of transactions that have YES overlap with other transactions

proc httpTest::TestOverlaps {someResults n term msg badTrans notPiped} {
    variable testOptions

    # Check whether transactions overlap:
    set clean {}
    set dirty {}
    for {set i 1} {$i <= $n} {incr i} {
        if {$i in $badTrans} {
            continue
        }
        set myStart   [lsearch -exact $someResults [list B $i]]
        set myEnd     [lsearch -exact $someResults [list $term $i]]

        if {($myStart == -1 || $myEnd == -1)} {
            set res "Cannot find positions of transaction $i"
	    append msg $res \n
	    Puts $res
        }

	set overlaps {}
	for {set j $myStart} {$j <= $myEnd} {incr j} {
	    lassign [lindex $someResults $j] letter number
	    if {$number != $i && $letter ne "A" && $number ni $notPiped} {
		lappend overlaps $number
	    }
	}

        if {[llength $overlaps] == 0} {
	    set res "Transaction $i has no overlaps"
	    Puts $res
	    lappend clean $i
	    if {$testOptions(-dotted)} {
		# N.B. results from different segments are concatenated.
		lappend dirty .
	    } else {
	    }
        } else {
	    set res "Transaction $i overlaps with [join $overlaps { }]"
	    Puts $res
	    lappend dirty $i
	    if {$testOptions(-dotted)} {
		# N.B. results from different segments are concatenated.
		lappend clean .
	    } else {
	    }
        }
    }
    return [list $msg $clean $dirty]
}

# httpTest::PipelineNext --
#
# Test whether prevPair, pair are valid as consecutive elements of a pipelined
# sequence (Start 1), (End 1), (Start 2), (End 2) ...
# Numbers are integers increasing (by 1 if argument "any" is false), and need
# not begin with 1.
# The first element of the sequence has prevPair {} and is always passed as
# valid.
#
# Arguments;
# Start        - string that labels the start of a segment
# End          - string that labels the end of a segment
# prevPair     - previous "pair" (list of string and number) element of a
#                sequence, or {} if argument "pair" is the first in the
#                sequence.
# pair         - current "pair" (list of string and number) element of a
#                sequence
# any          - (boolean) iff true, accept any increasing sequence of integers.
#                If false, integers must increase by 1.
#
# Return value - boolean, true iff the two pairs are valid consecutive elements.

proc httpTest::PipelineNext {Start End prevPair pair any} {
    if {$prevPair eq {}} {
        return 1
    }

    lassign $prevPair letter number
    lassign $pair newLetter newNumber
    if {$letter eq $Start} {
	return [expr {($newLetter eq $End) && ($newNumber == $number)}]
    } elseif {$any} {
        set nxt [list $Start [expr {$number + 1}]]
	return [expr {($newLetter eq $Start) && ($newNumber > $number)}]
    } else {
        set nxt [list $Start [expr {$number + 1}]]
	return [expr {($newLetter eq $Start) && ($newNumber == $number + 1)}]
    }
}

# httpTest::TestPipeline --
#
# Given a sequence of "pair" elements, check that the elements whose string is
# $Start or $End form a valid pipeline. Ignore other elements.
#
# Return value: {} if valid pipeline, otherwise a non-empty error message.

proc httpTest::TestPipeline {someResults n Start End msg desc badTrans} {
    set sequence {}
    set prevPair {}
    set ok 1
    set any [llength $badTrans]
    foreach pair $someResults {
        lassign $pair letter number
        if {($letter in [list $Start $End]) && ($number ni $badTrans)} {
            lappend sequence $pair
            if {![PipelineNext $Start $End $prevPair $pair $any]} {
		set ok 0
		break
            }
            set prevPair $pair
        }
    }

    if {!$ok} {
        set res "$desc are not pipelined: {$sequence}"
        append msg $res \n
        Puts $res
    }
    return $msg
}

# httpTest::TestSequence --
#
# Examine each transaction from 1 to $n, ignoring any that are listed
# in $badTrans.
# Check that each transaction has elements A to F, in alphabetical order.

proc httpTest::TestSequence {someResults n msg badTrans} {
    variable testOptions

    for {set i 1} {$i <= $n} {incr i} {
        if {$i in $badTrans} {
	    continue
        }
        set sequence {}
        foreach pair $someResults {
            lassign $pair letter number
            if {$number == $i} {
                lappend sequence $letter
            }
        }
        if {$sequence eq {A B C D E F}} {
        } else {
            set res "Wrong sequence for token ::http::$i - {$sequence}"
	    append msg $res \n
	    Puts $res
            if {"X" in $sequence} {
                set res "- and error(s) X"
		append msg $res \n
		Puts $res
            }
            if {"Y" in $sequence} {
                set res "- and warnings(s) Y"
		append msg $res \n
		Puts $res
            }
        }
    }
    return $msg
}

#
# Arguments:
# someResults  - list of elements, each a list of a letter and a number
# n            - (positive integer) the number of HTTP requests
# msg          - accumulated warning messages
# skipOverlaps - (boolean) whether to skip testing of transaction overlaps
# badTrans     - list of transaction numbers not to be assessed as "clean" or
#                "dirty" by their overlaps
#   for 1/2 includes all transactions
#   for 3/4 includes an increasing (with recursion) set that will not be included in the list because they are already handled.
# notPiped     - subset of badTrans.  List of transaction numbers that cannot
#                taint another transaction by overlapping with it, because it
#                used a different socket.
#
# Return value: [list $msg $cleanE $cleanF $dirtyE $dirtyF]
# msg    - warning messages: nothing will be appended to argument $msg if there
#          is no error with the test.
# cleanE - list of transactions that have no overlap with other transactions
#          (not considering response body)
# dirtyE - list of transactions that have YES overlap with other transactions
#          (not considering response body)
# cleanF - list of transactions that have no overlap with other transactions
#          (including response body)
# dirtyF - list of transactions that have YES overlap with other transactions
#          (including response body)

proc httpTest::MostAnalysis {someResults n msg skipOverlaps badTrans notPiped} {
    variable testOptions

    # Check that stages for "good" transactions are all present and correct:
    set msg [TestSequence $someResults $n $msg $badTrans]

    # Check that requests are pipelined:
    set msg [TestPipeline $someResults $n B C $msg Requests $notPiped]

    # Check that responses are pipelined:
    set msg [TestPipeline $someResults $n D F $msg Responses $notPiped]

    if {$skipOverlaps} {
	set cleanE {}
	set dirtyE {}
	set cleanF {}
	set dirtyF {}
    } else {
	Puts "Overlaps including response body (test for non-pipelined case)"
	lassign [TestOverlaps $someResults $n F $msg $badTrans $notPiped] msg cleanF dirtyF

	Puts "Overlaps without response body (test for pipelined case)"
	lassign [TestOverlaps $someResults $n E $msg $badTrans $notPiped] msg cleanE dirtyE
    }

    return [list $msg $cleanE $cleanF $dirtyE $dirtyF]
}

# httpTest::ProcessRetries --
#
# Command to examine results for socket-changing records [PQR],
# divide the results into segments for each connection, and analyse each segment
# individually.
# (Could add $sock to the logging to simplify this, but never mind.)
#
# In each segment, identify any transactions that are not included, and
# any that are aborted, to assist subsequent testing.
#
# Prepend A records (socket-independent) to each segment for transactions that
# were scheduled (by A) but not completed (by F).  Pass each segment to
# MostAnalysis for processing.

proc httpTest::ProcessRetries {someResults n msg skipOverlaps notIncluded notPiped} {
    variable testOptions

    set nextRetry [lsearch -glob -index 0 $someResults {[PQR]}]
    if {$nextRetry == -1} {
        return [MostAnalysis $someResults $n $msg $skipOverlaps $notIncluded $notPiped]
    }
    set badTrans $notIncluded
    set tryCount 0
    set try $nextRetry
    incr tryCount
    lassign [lindex $someResults $try] letter number
    Puts "Processing retry [lindex $someResults $try]"
    set beforeTry [lrange $someResults 0 $try-1]
    Puts [join $beforeTry \n]
    set afterTry [lrange $someResults $try+1 end]

    set dummyTry   {}
    for {set i 1} {$i <= $n} {incr i} {
        set first [lsearch -exact $beforeTry [list A $i]]
        set last  [lsearch -exact $beforeTry [list F $i]]
        if {$first == -1} {
	    set res "Transaction $i was not started in connection number $tryCount"
	    # So lappend it to badTrans and don't include it in the call below of MostAnalysis.
	    # append msg $res \n
	    Puts $res
	    if {$i ni $badTrans} {
		lappend badTrans $i
	    } else {
	    }
        } elseif {$last == -1} {
	    set res "Transaction $i was started but unfinished in connection number $tryCount"
	    # So lappend it to badTrans and don't include it in the call below of MostAnalysis.
	    # append msg $res \n
	    Puts $res
	    lappend badTrans $i
	    lappend dummyTry [list A $i]
        } else {
	    set res "Transaction $i was started and finished in connection number $tryCount"
	    # So include it in the call below of MostAnalysis.
	    # So lappend it to notIncluded and don't include it in the recursive call of
	    # ProcessRetries which handles the later connections.
	    # append msg $res \n
	    Puts $res
	    lappend notIncluded $i
        }
    }

    # Analyse the part of the results before the first replay:
    set HeadResults [MostAnalysis $beforeTry $n $msg $skipOverlaps $badTrans $notPiped]
    lassign $HeadResults msg cleanE1 cleanF1 dirtyE1 dirtyF1

    # Pass the rest of the results to be processed recursively.
    set afterTry [concat $dummyTry $afterTry]
    set TailResults [ProcessRetries $afterTry $n $msg $skipOverlaps $notIncluded $notPiped]
    lassign $TailResults msg cleanE2 cleanF2 dirtyE2 dirtyF2

    set cleanE [concat $cleanE1 $cleanE2]
    set cleanF [concat $cleanF1 $cleanF2]
    set dirtyE [concat $dirtyE1 $dirtyE2]
    set dirtyF [concat $dirtyF1 $dirtyF2]
    return [list $msg $cleanE $cleanF $dirtyE $dirtyF]
}

# httpTest::logAnalyse --
#
#	The main command called to analyse logs for a single test.
#
# Arguments:
# n            - (positive integer) the number of HTTP requests
# skipOverlaps - (boolean) whether to skip testing of transaction overlaps
# notIncluded  - list of transaction numbers not to be assessed as "clean" or
#                "dirty" by their overlaps
# notPiped     - subset of notIncluded.  List of transaction numbers that cannot
#                taint another transaction by overlapping with it, because it
#                used a different socket.
#
# Return value: [list $msg $cleanE $cleanF $dirtyE $dirtyF]
# msg    - warning messages: {} if there is no error with the test.
# cleanE - list of transactions that have no overlap with other transactions
#          (not considering response body)
# dirtyE - list of transactions that have YES overlap with other transactions
#          (not considering response body)
# cleanF - list of transactions that have no overlap with other transactions
#          (including response body)
# dirtyF - list of transactions that have YES overlap with other transactions
#          (including response body)

proc httpTest::logAnalyse {n skipOverlaps notIncluded notPiped} {
    variable testResults
    variable testOptions

    # Check that each data item has the correct form {letter numeral}.
    set ii 0
    set ok 1
    foreach pair $testResults {
	lassign $pair letter number
	if {    [string match {[A-Z]} $letter]
	     && [string match {[0-9]} $number]
	} {
	    # OK
	} else {
	    set ok 0
	    set res "Error: testResults has bad element {$pair} at position $ii"
	    append msg $res \n
	    Puts $res
	}
	incr ii
    }

    if {!$ok} {
	return $msg
    }
    set msg {}

    Puts [join $testResults \n]
    ProcessRetries $testResults $n $msg $skipOverlaps $notIncluded $notPiped
    # N.B. Implicit Return.
}

proc httpTest::cleanupHttpTest {} {
    variable testResults
    set testResults {}
    return
}

proc httpTest::setHttpTestOptions {key args} {
    variable testOptions
    if {$key ni {-dotted -verbose}} {
        return -code error {valid options are -dotted, -verbose}
    }
    set testOptions($key) {*}$args
}

namespace eval httpTest {
    namespace export cleanupHttpTest logAnalyse setHttpTestOptions
}
