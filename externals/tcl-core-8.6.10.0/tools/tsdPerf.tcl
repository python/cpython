
package require Thread

set ::tids [list]
for {set i 0} {$i < 4} {incr i} {
    lappend ::tids [thread::create [string map [list IVALUE $i] {
	set curdir [file dirname [info script]]
	load [file join $curdir tsdPerf[info sharedlibextension]]

	while 1 {
	    tsdPerfSet IVALUE
	}
    }]]
}

puts TIDS:$::tids

set curdir [file dirname [info script]]
load [file join $curdir tsdPerf[info sharedlibextension]]

tsdPerfSet 1234
while 1 {
    puts "TIME:[time {set value [tsdPerfGet]} 1000] VALUE:$value"
}
