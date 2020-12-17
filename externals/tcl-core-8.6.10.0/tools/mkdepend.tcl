#==============================================================================
#
# mkdepend : generate dependency information from C/C++ files
#
# Copyright (c) 1998, Nat Pryce
#
# Permission is hereby granted, without written agreement and without
# license or royalty fees, to use, copy, modify, and distribute this
# software and its documentation for any purpose, provided that the
# above copyright notice and the following two paragraphs appear in
# all copies of this software.
#
# IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
# SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
# THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHOR HAS BEEN ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#
# THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
# BASIS, AND THE AUTHOR HAS NO OBLIGATION TO PROVIDE  MAINTENANCE, SUPPORT,
# UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
#==============================================================================
#
# Modified heavily by David Gravereaux <davygrvy@pobox.com> about 9/17/2006.
# Original can be found @
#	http://web.archive.org/web/20070616205924/http://www.doc.ic.ac.uk/~np2/software/mkdepend.html
#==============================================================================

array set mode_data {}
set mode_data(vc32) {cl -nologo -E}

set source_extensions [list .c .cpp .cxx .cc]

set excludes [list]
if [info exists env(INCLUDE)] {
    set rawExcludes [split [string trim $env(INCLUDE) ";"] ";"]
    foreach exclude $rawExcludes {
	lappend excludes [file normalize $exclude]
    }
}


# openOutput --
#
#	Opens the output file.
#
# Arguments:
#	file	The file to open
#
# Results:
#	None.

proc openOutput {file} {
    global output
    set output [open $file w]
    puts $output "# Automatically generated at [clock format [clock seconds] -format "%Y-%m-%dT%H:%M:%S"] by [info script]\n"
}

# closeOutput --
#
#	Closes output file.
#
# Arguments:
#	none
#
# Results:
#	None.

proc closeOutput {} {
    global output
    if {[string match stdout $output] != 0} {
        close $output
    }
}

# readDepends --
#
#	Read off CCP pipe for #line references.
#
# Arguments:
#	chan	The pipe channel we are reading in.
#
# Results:
#	Raw dependency list pairs.

proc readDepends {chan} {
    set line ""
    array set depends {}

    while {[gets $chan line] != -1} {
        if {[regexp {^#line [0-9]+ \"(.*)\"$} $line dummy fname] != 0} {
	    set fname [file normalize $fname]
            if {![info exists target]} {
		# this is ourself
		set target $fname
		puts stderr "processing [file tail $fname]"
            } else {
		# don't include ourselves as a dependency of ourself.
		if {![string compare $fname $target]} {continue}
		# store in an array so multiple occurances are not counted.
                set depends($target|$fname) ""
            }
        }
    }

    set result {}
    foreach n [array names depends] {
        set pair [split $n "|"]
        lappend result [list [lindex $pair 0] [lindex $pair 1]]
    }

    return $result
}

# writeDepends --
#
#	Write the processed list out to the file.
#
# Arguments:
#	out		The channel to write to.
#	depends		The list of dependency pairs
#
# Results:
#	None.

proc writeDepends {out depends} {
    foreach pair $depends {
        puts $out "[lindex $pair 0] : \\\n\t[join [lindex $pair 1] " \\\n\t"]"
    }
}

# stringStartsWith --
#
#	Compares second string to the beginning of the first.
#
# Arguments:
#	str		The string to test the beginning of.
#	prefix		The string to test against
#
# Results:
#	the result of the comparison.

proc stringStartsWith {str prefix} {
    set front [string range $str 0 [expr {[string length $prefix] - 1}]]
    return [expr {[string compare [string tolower $prefix] \
                                  [string tolower $front]] == 0}]
}

# filterExcludes --
#
#	Remove non-project header files.
#
# Arguments:
#	depends		List of dependency pairs.
#	excludes	List of directories that should be removed
#
# Results:
#	the processed dependency list.

proc filterExcludes {depends excludes} {
    set filtered {}

    foreach pair $depends {
        set excluded 0
        set file [lindex $pair 1]

        foreach dir $excludes {
            if [stringStartsWith $file $dir] {
                set excluded 1
                break;
            }
        }

        if {!$excluded} {
            lappend filtered $pair
        }
    }

    return $filtered
}

# replacePrefix --
#
#	Take the normalized search path and put back the
#	macro name for it.
#
# Arguments:
#	file	filename.
#
# Results:
#	filename properly replaced with macro for it.

proc replacePrefix {file} {
    global srcPathList srcPathReplaceList

    foreach was $srcPathList is $srcPathReplaceList {
	regsub $was $file $is file
    }
    return $file
}

# rebaseFiles --
#
#	Replaces normalized paths with original macro names.
#
# Arguments:
#	depends		Dependency pair list.
#
# Results:
#	The processed dependency pair list.

proc rebaseFiles {depends} {
    set rebased {}
    foreach pair $depends {
        lappend rebased [list \
                [replacePrefix [lindex $pair 0]] \
		[replacePrefix [lindex $pair 1]]]

    }
    return $rebased
}

# compressDeps --
#
#	Compresses same named tragets into one pair with
#	multiple deps.
#
# Arguments:
#	depends	Dependency pair list.
#
# Results:
#	The processed list.

proc compressDeps {depends} {
    array set compressed [list]

    foreach pair $depends {
	lappend compressed([lindex $pair 0]) [lindex $pair 1]
    }

    set result [list]
    foreach n [array names compressed] {
        lappend result [list $n [lsort $compressed($n)]]
    }

    return $result
}

# addSearchPath --
#
#	Adds a new set of path and replacement string to the global list.
#
# Arguments:
#	newPathInfo	comma seperated path and replacement string
#
# Results:
#	None.

proc addSearchPath {newPathInfo} {
    global srcPathList srcPathReplaceList

    set infoList [split $newPathInfo ,]
    lappend srcPathList [file normalize [lindex $infoList 0]]
    lappend srcPathReplaceList [lindex $infoList 1]
}


# displayUsage --
#
#	Displays usage to stderr
#
# Arguments:
#	none.
#
# Results:
#	None.

proc displayUsage {} {
    puts stderr "mkdepend.tcl \[options\] genericDir,macroName compatDir,macroName platformDir,macroName"
}

# readInputListFile --
#
#	Open and read the object file list.
#
# Arguments:
#	objectListFile - name of the file to open.
#
# Results:
#	None.

proc readInputListFile {objectListFile} {
    global srcFileList srcPathList source_extensions
    set f [open $objectListFile r]
    set fl [read $f]
    close $f

    # fix native path seperator so it isn't treated as an escape.
    regsub -all {\\} $fl {/} fl

    # Treat the string as a list so filenames between double quotes are
    # treated as list elements.
    foreach fname $fl {
	# Compiled .res resource files should be ignored.
	if {[file extension $fname] ne ".obj"} {continue}

	# Just filename without path or extension because the path is
	# the build directory, not where the source files are located.
	set baseName [file rootname [file tail $fname]]

	set found 0
	foreach path $srcPathList {
	    foreach ext $source_extensions {
		set test [file join $path ${baseName}${ext}]
		if {[file exist $test]} {
		    lappend srcFileList $test
		    set found 1
		    break
		}
	    }
	    if {$found} break
	}
    }
}

# main --
#
#	The main procedure of this script.
#
# Arguments:
#	none.
#
# Results:
#	None.

proc main {} {
    global argc argv mode mode_data srcFileList srcPathList excludes
    global remove_prefix target_prefix output env

    set srcPathList [list]
    set srcFileList [list]

    if {$argc == 1} {displayUsage}

    # Parse mkdepend input
    for {set i 0} {$i < [llength $argv]} {incr i} {
	switch -glob -- [set arg [lindex $argv $i]] {
	    -vc32 {
		set mode vc32
	    }
	    -bc32 {
		set mode bc32
	    }
	    -wc32 {
		set mode wc32
	    }
	    -lc32 {
		set mode lc32
	    }
	    -mgw32 {
		set mode mgw32
	    }
	    -passthru:* {
		set passthru [string range $arg 10 end]
		regsub -all {"} $passthru {\"} passthru
		regsub -all {\\} $passthru {/} passthru
	    }
	    -out:* {
		openOutput [string range $arg 5 end]
	    }
	    @* {
		set objfile [string range $arg 1 end]
		regsub -all {\\} $objfile {/} objfile
		readInputListFile $objfile
	    }
	    -? - -help - --help {
		displayUsage
		exit 1
	    }
	    default {
		if {![info exist mode]} {
		    puts stderr "mode not set"
		    displayUsage
		}
		addSearchPath $arg
	    }
	}
    }

    # Execute the CPP command and parse output

    foreach srcFile $srcFileList {
	if {[catch {
	    set command "$mode_data($mode) $passthru \"$srcFile\""
	    set input [open |$command r]
	    set depends [readDepends $input]
	    set status [catch {close $input} result]
	    if {$status == 1 && [lindex $::errorCode 0] eq "CHILDSTATUS"} {
		foreach { - pid code } $::errorCode break
		if {$code == 2} {
		    # preprocessor died a cruel death.
		    error $result
		}
	    }
	} err]} {
	    puts stderr "error ocurred: $err\n"
	    continue
	}
	set depends [filterExcludes $depends $excludes]
	set depends [rebaseFiles $depends]
	set depends [compressDeps $depends]
	writeDepends $output $depends
    }

    closeOutput
}

# kick it up.
main
