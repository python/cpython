# fixtommath.tcl --
#
#	Changes to 'tommath.h' to make it conform with Tcl's linking
#	conventions.
#
# Copyright (c) 2005 by Kevin B. Kenny.  All rights reserved.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#----------------------------------------------------------------------

set f [open [lindex $argv 0] r]
set data [read $f]
close $f

set eat_endif 0
set eat_semi 0
set def_count 0
foreach line [split $data \n] {
    if {!$eat_semi && !$eat_endif} {
	switch -regexp -- $line {
	    {#define BN_H_} {
		puts $line
		puts {}
		puts "\#include \"tclInt.h\""
		puts "\#include \"tclTomMathDecls.h\""
		puts "\#ifndef MODULE_SCOPE"
		puts "\#define MODULE_SCOPE extern"
		puts "\#endif"
	    }
	    {typedef\s+unsigned long\s+mp_digit;} {
		# change the second 'typedef unsigned long mp
		incr def_count
		puts "\#ifndef MP_DIGIT_DECLARED"
		if {$def_count == 2} {
		    puts [string map {long int} $line]
		} else {
		    puts $line
		}
		puts "\#define MP_DIGIT_DECLARED"
		puts "\#endif"
	    }
	    {typedef.*mp_digit;} {
		puts "\#ifndef MP_DIGIT_DECLARED"
		puts $line
		puts "\#define MP_DIGIT_DECLARED"
		puts "\#endif"
	    }
	    {typedef struct} {
		puts "\#ifndef MP_INT_DECLARED"
		puts "\#define MP_INT_DECLARED"
		puts "typedef struct mp_int mp_int;"
		puts "\#endif"
		puts "struct mp_int \{"
	    }
	    \}\ mp_int\; {
		puts "\};"
	    }
	    {^(char|int|void)} {
		puts "/*"
		puts $line
		set eat_semi 1
		set after_semi "*/"
	    }
	    {^extern (int|const)} {
		puts "\#if defined(BUILD_tcl) || !defined(_WIN32)"
		puts [regsub {^extern} $line "MODULE_SCOPE"]
		set eat_semi 1
		set after_semi "\#endif"
	    }
	    {define heap macros} {
		puts $line
		puts "\#if 0 /* these are macros in tclTomMathDecls.h */"
		set eat_endif 1
	    }
	    {__x86_64__} {
		puts "[string map {__x86_64__ NEVER} $line]\
                      /* 128-bit ints fail in too many places */"
	    }
	    {#include} {
		# remove all includes
	    }
	    default {
		puts $line
	    }
	}
    } else {
	puts $line
    }
    if {$eat_semi} {
	if {[regexp {; *$} $line]} {
	    puts $after_semi
	    set eat_semi 0
	}
    }
    if {$eat_endif} {
	if {[regexp {^\#endif} $line]} {
	    puts "\#endif"
	    set eat_endif 0
	}
    }
}
