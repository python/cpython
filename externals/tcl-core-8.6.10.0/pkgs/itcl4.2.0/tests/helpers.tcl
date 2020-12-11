# helpers.tcl --
#
# This file contains helper scripts for all tests, like a mem-leak checker, etc.

# -loadfile overwrites -load, so restore it from ::env(TESTFLAGS):
if {[info exists ::env(TESTFLAGS)]} {
  array set testargs $::env(TESTFLAGS)
  if {[info exists ::testargs(-load)]} {
    eval $::testargs(-load)
  }
  unset testargs
}

package require itcl

if {[namespace which -command memory] ne "" && (
    ![info exists ::tcl::inl_mem_test] || $::tcl::inl_mem_test
  )
} {
  proc getbytes {} {lindex [split [memory info] \n] 3 3}
  proc leaktest {script {iterations 3}} {
      set end [getbytes]
      for {set i 0} {$i < $iterations} {incr i} {
          uplevel 1 $script
          set tmp $end
          set end [getbytes]
      }
      return [expr {$end - $tmp}]
  }
  proc itcl_leaktest {testfile} {
    set leak [leaktest [string map [list \
      @test@ $testfile \
      @testargv@ [if {[info exists ::argv]} {list tcltest::configure {*}$::argv}]
    ] {
      interp create i
      load {} Itcl i
      i eval {set ::tcl::inl_mem_test 0}
      i eval {package require tcltest; @testargv@}
      i eval [list source @test@]
      interp delete i
    }]]
    if {$leak} {
      puts "LEAKED: $leak bytes"
    }
  }
  itcl_leaktest [info script]
  return -code return
}
