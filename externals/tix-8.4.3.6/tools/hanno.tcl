#
#	$Id: hanno.tcl,v 1.1.1.1 2000/05/17 11:08:53 idiscovery Exp $
#
#!/bin/sh
# the next line restarts using tixwish \
exec tclsh7.6 "$0" "$@"

# Options
#
#	-v : Verbose mode. Print out what hanno is doing.
#
set verbose 0

if {[info exists env(TIX_VERBOSE)] && $env(TIX_VERBOSE) == 1} {
    set verbose 1
}

if {[lsearch -glob $argv -v*] != -1} {
    set verbose 1
}

set files [exec find . -name *.html -print]

foreach file $files {
    if {$verbose} {
	puts "\[html anno]: checking $file"
    }
    set output {}
    set src [open $file RDONLY]

    set changed 1

    while {![eof $src]} {
	set line [gets $src]

	if {[regexp -nocase {[ \t]*\<hr>\<i>Last modified.*} $line]} {
	    # Do nothing
	} elseif {[regexp -nocase {[ \t]*\<i>Serial.*\</i>} $line]} {
	    if {[scan $line "<i>Serial %d</i>" lastmtime] == 1} {
		if {[expr [file mtime $file] - $lastmtime] >= 10} {
		    set changed 1
		} else {
		    set changed 0
		}
	    }
	} else {
	    append output $line\n
	}
    }
    close $src

    if {$changed == 1} {
	if {$verbose} {
	    puts "\[html anno]: modifying tag of $file"
	}

	set date [clock format [file mtime $file]]

	set des [open $file {WRONLY TRUNC}]
	puts -nonewline $des $output

	# Somehow the "seek" is necessary
	#
	seek $des -1 current
	puts $des "<hr><i>Last modified $date </i> --- "
	puts $des "<i>Serial [file mtime $file]</i>"
	close $des
    }
}
