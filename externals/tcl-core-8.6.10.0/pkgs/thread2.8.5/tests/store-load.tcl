#!/usr/bin/env tclsh

lappend auto_path .
package require Thread

if {[llength $argv] != 3} {
    puts "Usage: $argv0 handle path times"
    puts {
    handle
        A persistent storage handle (see [tsv::array bind] manpage).
    path
        The path to file containing lines in the form of "key<tab>val", where
        key is a single-word and val is everyting else.
    times
        The number of times to reload the data from persistent storage.

    This script reads lines of data from <path> and stores them into the
    persistent storage described by <handle>. Values for duplicate keys are
    handled as a lists. The persistent storage engine is then stress-tested by
    reloading the whole store <times> times.
    }
    exit 1
}

lassign $argv handle path times

### Cleanup
set filename [string range $handle [string first : $handle]+1 end]
file delete -force $filename

### Load and store tab-separated values
tsv::array bind a $handle
set fd [open $path r]
set start [clock milliseconds]
set pairs 0
while {[gets $fd line] >  0} {
    if {[string index $line 0] eq {#}} {
        continue
    }
    set tab [string first {	} $line]
    if {$tab == -1} {
        continue
    }

    set k [string range $line 0 $tab-1]
    set v [string range $line $tab+1 end]

    if {![tsv::exists a $k]} {
        incr pairs
    }

    tsv::lappend a $k $v
}
puts "Stored $pairs pairs in [expr {[clock milliseconds]-$start}] milliseconds"

tsv::array unbind a
tsv::unset a

### Reload
set pairs 0
set iter [time {
    tsv::array bind a $handle
    set pairs [tsv::array size a]
    tsv::array unbind a
    tsv::unset a
} $times]
puts "Loaded $pairs pairs $times times at $iter"

## Dump file stats
puts "File $filename is [file size $filename] bytes long"
