## Super aggressive EOL-fixer!
##
##  Will even understand screwed up ones like CRCRLF.
##  (found in bad CVS repositories, caused by spacey developers
##   abusing CVS)
##
##  davygrvy@pobox.com    3:41 PM 10/12/2001
##

package provide EOL-fix 1.1

namespace eval ::EOL {
    variable outMode crlf
}

proc EOL::fix {filename {newfilename {}}} {
    variable outMode

    if {![file exists $filename]} {
	return
    }
    puts "EOL Fixing: $filename"

    file rename ${filename} ${filename}.o
    set fhnd [open ${filename}.o r]

    if {$newfilename ne ""} {
	set newfhnd [open ${newfilename} w]
    } else {
	set newfhnd [open ${filename} w]
    }

    fconfigure $newfhnd -translation [list auto $outMode]
    seek $fhnd 0 end
    set theEnd [tell $fhnd]
    seek $fhnd 0 start

    fconfigure $fhnd -translation binary -buffersize $theEnd
    set rawFile [read $fhnd $theEnd]
    close $fhnd

    regsub -all {(\r)|(\r){1,2}(\n)} $rawFile "\n" rawFile

    set lineList [split $rawFile \n]

    foreach line $lineList {
	puts $newfhnd $line
    }

    close $newfhnd
    file delete ${filename}.o
}

proc EOL::fixall {args} {
    if {[llength $args] == 0} {
	puts stderr "no files to fix"
	exit 1
    } else {
	set cmd [lreplace $args -1 -1 glob -nocomplain]
    }

    foreach f [eval $cmd] {
	if {[file isfile $f]} {fix $f}
    }
}

if {$tcl_interactive == 0 && $argc > 0} {
    if {[string index [lindex $argv 0] 0] eq "-"} {
	switch -- [lindex $argv 0] {
	    -cr   {set ::EOL::outMode cr}
	    -crlf {set ::EOL::outMode crlf}
	    -lf   {set ::EOL::outMode lf}
	    default {puts stderr "improper mode switch"; exit 1}
        }
	set argv [lrange $argv 1 end]
    }
    eval EOL::fixall $argv
} else {
    return
}
