#! /usr/bin/env tclsh

package require tcltest 2.2
namespace import ::tcltest::*

testConstraint exec          [llength [info commands exec]]
testConstraint fcopy         [llength [info commands fcopy]]
testConstraint fileevent     [llength [info commands fileevent]]
testConstraint thread        [
    expr {0 == [catch {package require Thread 2.7-}]}]
testConstraint notValgrind   [expr {![testConstraint valgrind]}]
