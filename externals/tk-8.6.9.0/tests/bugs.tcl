# This file is a Tcl script to test out various known bugs that will
# cause Tk to crash.  This file ends with .tcl instead of .test to make
# sure it isn't run when you type "source all".  We currently are not
# shipping this file with the rest of the source release.
#
# Copyright (c) 1996 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

if {[info procs test] != "test"} {
    source defs
}

test crash-1.0 {imgPhoto} {
    image create photo p1
    image create photo p2
    catch {image create photo p2 -file bogus}
    p1 copy p2
    label .l -image p1
    destroy .l
    set foo ""
} {}

test crash-1.1 {color} {
    . configure -bg rgb:345
    set foo ""
} {}













