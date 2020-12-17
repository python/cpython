# regexpTestLib.tcl --
#
# This file contains tcl procedures used by spencer2testregexp.tcl and
# spencer2regexp.tcl, which are programs written to convert Henry
# Spencer's test suite to tcl test files.
#
# Copyright (c) 1996 by Sun Microsystems, Inc.

proc readInputFile {} {
    global inFileName
    global lineArray

    set fileId [open $inFileName r]

    set i 0
    while {[gets $fileId line] >= 0} {

	set len [string length $line]

	if {($len > 0) && ([string index $line [expr $len - 1]] == "\\")} {
	    if {[info exists lineArray(c$i)] == 0} {
		set lineArray(c$i) 1
	    } else {
		incr lineArray(c$i)
	    }
	    set line [string range $line 0 [expr $len - 2]]
	    append lineArray($i) $line
	    continue
	}
	if {[info exists lineArray(c$i)] == 0} {
	    set lineArray(c$i) 1
	} else {
	    incr lineArray(c$i)
	}
	append lineArray($i) $line
	incr i
    }

    close $fileId
    return $i
}

#
# strings with embedded @'s are truncated
# unpreceeded @'s are replaced by {}
#
proc removeAts {ls} {
    set len [llength $ls]
    set newLs {}
    foreach item $ls {
	regsub @.* $item "" newItem
	lappend newLs $newItem
    }
    return $newLs
}

proc convertErrCode {code} {

    set errMsg "couldn't compile regular expression pattern:"

    if {[string compare $code "INVARG"] == 0} {
	return "$errMsg invalid argument to regex routine"
    } elseif {[string compare $code "BADRPT"] == 0} {
	return "$errMsg ?+* follows nothing"
    } elseif {[string compare $code "BADBR"] == 0} {
	return "$errMsg invalid repetition count(s)"
    } elseif {[string compare $code "BADOPT"] == 0} {
	return "$errMsg invalid embedded option"
    } elseif {[string compare $code "EPAREN"] == 0} {
	return "$errMsg unmatched ()"
    } elseif {[string compare $code "EBRACE"] == 0} {
	return "$errMsg unmatched {}"
    } elseif {[string compare $code "EBRACK"] == 0} {
	return "$errMsg unmatched \[\]"
    } elseif {[string compare $code "ERANGE"] == 0} {
	return "$errMsg invalid character range"
    } elseif {[string compare $code "ECTYPE"] == 0} {
	return "$errMsg invalid character class"
    } elseif {[string compare $code "ECOLLATE"] == 0} {
	return "$errMsg invalid collating element"
    } elseif {[string compare $code "EESCAPE"] == 0} {
	return "$errMsg invalid escape sequence"
    } elseif {[string compare $code "BADPAT"] == 0} {
	return "$errMsg invalid regular expression"
    } elseif {[string compare $code "ESUBREG"] == 0} {
	return "$errMsg invalid backreference number"
    } elseif {[string compare $code "IMPOSS"] == 0} {
	return "$errMsg can never match"
    }
    return "$errMsg $code"
}

proc writeOutputFile {numLines fcn} {
    global outFileName
    global lineArray

    # open output file and write file header info to it.

    set fileId [open $outFileName w]

    puts $fileId "# Commands covered:  $fcn"
    puts $fileId "#"
    puts $fileId "# This Tcl-generated file contains tests for the $fcn tcl command."
    puts $fileId "# Sourcing this file into Tcl runs the tests and generates output for"
    puts $fileId "# errors.  No output means no errors were found.  Setting VERBOSE to"
    puts $fileId "# -1 will run tests that are known to fail."
    puts $fileId "#"
    puts $fileId "# Copyright (c) 1998 Sun Microsystems, Inc."
    puts $fileId "#"
    puts $fileId "# See the file \"license.terms\" for information on usage and redistribution"
    puts $fileId "# of this file, and for a DISCLAIMER OF ALL WARRANTIES."
    puts $fileId "#"
    puts $fileId "\# SCCS: \%Z\% \%M\% \%I\% \%E\% \%U\%"
    puts $fileId "\nproc print \{arg\} \{puts \$arg\}\n"
    puts $fileId "if \{\[string compare test \[info procs test\]\] == 1\} \{"
    puts $fileId "    source defs ; set VERBOSE -1\n\}\n"
    puts $fileId "if \{\$VERBOSE != -1\} \{"
    puts $fileId "    proc print \{arg\} \{\}\n\}\n"
    puts $fileId "#"
    puts $fileId "# The remainder of this file is Tcl tests that have been"
    puts $fileId "# converted from Henry Spencer's regexp test suite."
    puts $fileId "#\n"

    set lineNum 0
    set srcLineNum 1
    while {$lineNum < $numLines} {

	set currentLine $lineArray($lineNum)

	# copy comment string to output file and continue

	if {[string index $currentLine 0] == "#"} {
	    puts $fileId $currentLine
	    incr srcLineNum $lineArray(c$lineNum)
	    incr lineNum
	    continue
	}

	set len [llength $currentLine]

	# copy empty string to output file and continue

	if {$len == 0} {
	    puts $fileId "\n"
	    incr srcLineNum $lineArray(c$lineNum)
	    incr lineNum
	    continue
	}
	if {($len < 3)} {
	    puts "warning: test is too short --\n\t$currentLine"
	    incr srcLineNum $lineArray(c$lineNum)
	    incr lineNum
	    continue
	}

	puts $fileId [convertTestLine $currentLine $len $lineNum $srcLineNum]

	incr srcLineNum $lineArray(c$lineNum)
	incr lineNum
    }

    close $fileId
}

proc convertTestLine {currentLine len lineNum srcLineNum} {

    regsub -all {(?b)\\} $currentLine {\\\\} currentLine
    set re [lindex $currentLine 0]
    set flags [lindex $currentLine 1]
    set str [lindex $currentLine 2]

    # based on flags, decide whether to skip the test

    if {[findSkipFlag $flags]} {
	regsub -all {\[|\]|\(|\)|\{|\}|\#} $currentLine {\&} line
	set msg "\# skipping char mapping test from line $srcLineNum\n"
	append msg "print \{... skip test from line $srcLineNum:  $line\}"
	return $msg
    }

    # perform mapping if '=' flag exists

    set noBraces 0
    if {[regexp {=|>} $flags] == 1} {
	regsub -all {_} $currentLine {\\ } currentLine
	regsub -all {A} $currentLine {\\007} currentLine
	regsub -all {B} $currentLine {\\b} currentLine
	regsub -all {E} $currentLine {\\033} currentLine
	regsub -all {F} $currentLine {\\f} currentLine
	regsub -all {N} $currentLine {\\n} currentLine

	# if and \r substitutions are made, do not wrap re, flags,
	# str, and result in braces

	set noBraces [regsub -all {R} $currentLine {\\\u000D} currentLine]
	regsub -all {T} $currentLine {\\t} currentLine
	regsub -all {V} $currentLine {\\v} currentLine
	if {[regexp {=} $flags] == 1} {
	    set re [lindex $currentLine 0]
	}
	set str [lindex $currentLine 2]
    }
    set flags [removeFlags $flags]

    # find the test result

    set numVars [expr $len - 3]
    set vars {}
    set vals {}
    set result 0
    set v 0

    if {[regsub {\*} "$flags" "" newFlags] == 1} {
	# an error is expected

	if {[string compare $str "EMPTY"] == 0} {
	    # empty regexp is not an error
	    # skip this test

	    return "\# skipping the empty-re test from line $srcLineNum\n"
	}
	set flags $newFlags
	set result "\{1 \{[convertErrCode $str]\}\}"
    } elseif {$numVars > 0} {
	# at least 1 match is made

	if {[regexp {s} $flags] == 1} {
	    set result "\{0 1\}"
	} else {
	    while {$v < $numVars} {
		append vars " var($v)"
		append vals " \$var($v)"
		incr v
	    }
	    set tmp [removeAts [lrange $currentLine 3 $len]]
	    set result "\{0 \{1 $tmp\}\}"
	    if {$noBraces} {
		set result "\[subst $result\]"
	    }
	}
    } else {
	# no match is made

	set result "\{0 0\}"
    }

    # set up the test and write it to the output file

    set cmd [prepareCmd $flags $re $str $vars $noBraces]
    if {$cmd == -1} {
	return "\# skipping test with metasyntax from line $srcLineNum\n"
    }

    set test "test regexp-1.$srcLineNum \{converted from line $srcLineNum\} \{\n"
    append test "\tcatch {unset var}\n"
    append test "\tlist \[catch \{\n"
    append test "\t\tset match \[$cmd\]\n"
    append test "\t\tlist \$match $vals\n"
    append test "\t\} msg\] \$msg\n"
    append test "\} $result\n"
    return $test
}

