# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: HLHdr.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# This tests the "header" functions in HList
#
#
# Assumptions:
#	(1) add command OK
#

proc test {cmd {result {}} {ret {}}} {
    if [catch {set ret [uplevel 1 $cmd]} err] {
	set done 0
	foreach r $result {
	    if [regexp $r $err] {
		puts "error message OK: $err"
		set done 1
		break
	    }
	}
	if {!$done} {
	    error $err
	}
    } else {
	puts "execution OK: $cmd"
    }
    return $ret
}

set h [tixHList .h -header 1 -columns 2]
pack $h -expand yes -fill both
$h add hello -text hello
$h add noind -text hello

test {$h header} {args}
test {$h header bad} {unknown}

# Test for create
#
#

test {$h header create} {args}
test {$h header create 3} {{exist}}
test {$h header create 1 -itemtype} {missing}
test {$h header create 1 -itemtype bad} {unknown}
test {$h header create 1 -itemtype imagetext -text Hello -image [tix getimage folder]}


# Test for cget
#
test {$h header cget} {args}
test {$h header cget 0 -text} {does not have}
test {$h header cget 1} {args}
test {$h header cget 3 -text} {exist}
test {$h header cget 1 arg arg} {args}
test {$h header cget 1 -bad} {{unknown}}
test {$h header cget 1 -text}

# Test for config
#
test {$h header config} {args}
test {$h header config 3 -text} {exist}
test {$h header config 0 -text} {does not have}
test {$h header config 1 -bad} {{unknown}}
test {$h header config 1}
test {$h header config 1 -text}
test {$h header config 1 -text Hi}

# Test for size
#
test {$h header size} {args}
test {$h header size 0 0} {args}
test {$h header size 4} {exist}
test {$h header size 0} {not have}
test {puts [$h header size 1]}


# Test for exist
#
test {$h header exist} {args}
test {$h header exist hello hi} {args}
test {$h header exist 4} {exist}
test {puts [$h header exist 0]} 
test {puts [$h header exist 1]} 

# Test for delete
#
test {$h header delete} {args}
test {$h header delete hello hi} {args}
test {$h header delete 4} {exist}
test {$h header delete 0} {not have}
test {$h header delete 1} 

# just do it again ..
#
test {$h header create 1 -itemtype imagetext -text Hello -image [tix getimage folder]}

