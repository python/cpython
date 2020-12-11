# TODO - When integrating this with the Core, path names will need to be
# swizzled here.

package require msgcat
set d [file dirname [file dirname [info script]]]
puts "getting transition data from [file join $d library tzdata America Detroit]"
source [file join $d library/tzdata/America/Detroit]

namespace eval ::tcl::clock {
    ::msgcat::mcmset en_US_roman {
	LOCALE_ERAS {
	    {-62164627200 {} 0}
	    {-59008867200 c 100}
	    {-55853107200 cc 200}
	    {-52697347200 ccc 300}
	    {-49541587200 cd 400}
	    {-46385827200 d 500}
	    {-43230067200 dc 600}
	    {-40074307200 dcc 700}
	    {-36918547200 dccc 800}
	    {-33762787200 cm 900}
	    {-30607027200 m 1000}
	    {-27451267200 mc 1100}
	    {-24295507200 mcc 1200}
	    {-21139747200 mccc 1300}
	    {-17983987200 mcd 1400}
	    {-14828227200 md 1500}
	    {-11672467200 mdc 1600}
	    {-8516707200 mdcc 1700}
	    {-5364662400 mdccc 1800}
	    {-2208988800 mcm 1900}
	    {946684800 mm 2000}
	}
	LOCALE_NUMERALS {
	    ? i ii iii iv v vi vii viii ix
	    x xi xii xiii xiv xv xvi xvii xviii xix
	    xx xxi xxii xxiii xxiv xxv xxvi xxvii xxviii xxix
	    xxx xxxi xxxii xxxiii xxxiv xxxv xxxvi xxxvii xxxviii xxxix
	    xl xli xlii xliii xliv xlv xlvi xlvii xlviii xlix
	    l li lii liii liv lv lvi lvii lviii lix
	    lx lxi lxii lxiii lxiv lxv lxvi lxvii lxviii lxix
	    lxx lxxi lxxii lxxiii lxxiv lxxv lxxvi lxxvii lxxviii lxxix
	    lxxx lxxxi lxxxii lxxxiii lxxxiv lxxxv lxxxvi lxxxvii lxxxviii
	    lxxxix
	    xc xci xcii xciii xciv xcv xcvi xcvii xcviii xcix
	    c
	}
	DATE_FORMAT {%m/%d/%Y}
	TIME_FORMAT {%H:%M:%S}
	DATE_TIME_FORMAT {%x %X}
	LOCALE_DATE_FORMAT {die %Od mensis %Om annoque %EY}
	LOCALE_TIME_FORMAT {%OH h %OM m %OS s}
	LOCALE_DATE_TIME_FORMAT {%Ex %EX}
    }
}

#----------------------------------------------------------------------
#
# listYears --
#
#	List the years to test in the common clock test cases.
#
# Parameters:
#	startOfYearArray - Name of an array in caller's scope that will
#	                   be initialized as
# Results:
#       None
#
# Side effects:
#	Determines the year numbers of one common year, one leap year, one year
#	following a common year, and one year following a leap year -- starting
#	on each day of the week -- in the XIXth, XXth and XXIth centuries.
#	Initializes the given array to have keys equal to the year numbers and
#	values equal to [clock seconds] at the start of the corresponding
#	years.
#
#----------------------------------------------------------------------

proc listYears { startOfYearArray } {

    upvar 1 $startOfYearArray startOfYear

    # List years after 1970

    set y 1970
    set s 0
    set dw 4 ;# Thursday
    while { $y < 2100 } {
	if { $y % 4 == 0 && $y % 100 != 0 || $y % 400 == 0 } {
	    set l 1
	    incr dw 366
	    set s2 [expr { $s + wide( 366 * 86400 ) }]
	} else {
	    set l 0
	    incr dw 365
	    set s2 [expr { $s + wide( 365 * 86400 ) }]
	}
	set x [expr { $y >= 2037 }]
	set dw [expr {$dw % 7}]
	set c [expr { $y / 100 }]
	if { ![info exists do($x$c$dw$l)] } {
	    set do($x$c$dw$l) $y
	    set startOfYear($y) $s
	    set startOfYear([expr {$y + 1}]) $s2
	}
	set s $s2
	incr y
    }

    # List years before 1970

    set y 1970
    set s 0
    set dw 4; # Thursday
    while { $y >= 1801 } {
	set s0 $s
	incr dw 371
	incr y -1
	if { $y % 4 == 0 && $y % 100 != 0 || $y % 400 == 0 } {
	    set l 1
	    incr dw -366
	    set s [expr { $s - wide(366 * 86400) }]
	} else {
	    set l 0
	    incr dw -365
	    set s [expr { $s - wide(365 * 86400) }]
	}
	set dw [expr {$dw % 7}]
	set c [expr { $y / 100 }]
	if { ![info exists do($c$dw$l)] } {
	    set do($c$dw$l) $y
	    set startOfYear($y) $s
	    set startOfYear([expr {$y + 1}]) $s0
	}
    }

}

#----------------------------------------------------------------------
#
# processFile -
#
#	Processes the 'clock.test' file, updating the test cases in it.
#
# Parameters:
#	None.
#
# Side effects:
#	Replaces the file with a new copy, constructing needed test cases.
#
#----------------------------------------------------------------------

proc processFile {d} {

    # Open two files

    set f1 [open [file join $d tests/clock.test] r]
    set f2 [open [file join $d tests/clock.new] w]

    # Copy leading portion of the test file

    set state {}
    while { [gets $f1 line] >= 0 } {
	switch -exact -- $state {
	    {} {
		puts $f2 $line
		if { [regexp "^\# BEGIN (.*)" $line -> cases]
		     && [string compare {} [info commands $cases]] } {
		    set state inCaseSet
		    $cases $f2
		}
	    }
	    inCaseSet {
		if { [regexp "^\#\ END $cases\$" $line] } {
		    puts $f2 $line
		    set state {}
		}
	    }
	}
    }

    # Rotate the files

    close $f1
    close $f2
    file delete -force [file join $d tests/clock.bak]
    file rename -force [file join $d tests/clock.test] \
	[file join $d tests/clock.bak]
    file rename [file join $d tests/clock.new] [file join $d tests/clock.test]

}

#----------------------------------------------------------------------
#
# testcases2 --
#
#	Outputs the 'clock-2.x' test cases.
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for formatting in Gregorian calendar are written to the
#	output file.
#
#----------------------------------------------------------------------

proc testcases2 { f2 } {

    listYears startOfYear

    # Define the roman numerals

    set roman {
 	? i ii iii iv v vi vii viii ix
	x xi xii xiii xiv xv xvi xvii xviii xix
	xx xxi xxii xxiii xxiv xxv xxvi xxvii xxviii xxix
	xxx xxxi xxxii xxxiii xxxiv xxxv xxxvi xxxvii xxxviii xxxix
	xl xli xlii xliii xliv xlv xlvi xlvii xlviii xlix
	l li lii liii liv lv lvi lvii lviii lix
	lx lxi lxii lxiii lxiv lxv lxvi lxvii lxviii lxix
	lxx lxxi lxxii lxxiii lxxiv lxxv lxxvi lxxvii lxxviii lxxix
	lxxx lxxxi lxxxii lxxxiii lxxxiv lxxxv lxxxvi lxxxvii lxxxviii lxxxix
	xc xci xcii xciii xciv xcv xcvi xcvii xcviii xcix
	c
    }
    set romanc {
 	? c cc ccc cd d dc dcc dccc cm
	m mc mcc mccc mcd md mdc mdcc mdccc mcm
	mm mmc mmcc mmccc mmcd mmd mmdc mmdcc mmdccc mmcm
	mmm mmmc mmmcc mmmccc mmmcd mmmd mmmdc mmmdcc mmmdccc mmmcm
    }

    # Names of the months

    set short {{} Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec}
    set long {
	{} January February March April May June July August September
	October November December
    }

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test formatting of Gregorian year, month, day, all formats"
    puts $f2 "\# Formats tested: %b %B %c %Ec %C %EC %d %Od %e %Oe %h %j %J %m %Om %N %x %Ex %y %Oy %Y %EY"
    puts $f2 ""

    # Generate the test cases for the first and last day of every month
    # from 1896 to 2045

    set n 0
    foreach { y } [lsort -integer [array names startOfYear]] {
	set s [expr { $startOfYear($y) + wide(12*3600 + 34*60 + 56) }]
	set m 0
	set yd 1
	foreach hath { 31 28 31 30 31 30 31 31 30 31 30 31 } {
	    incr m
	    if { $m == 2 && ( $y%4 == 0 && $y%100 != 0 || $y%400 == 0 ) } {
		incr hath
	    }

	    set b [lindex $short $m]
	    set B [lindex $long $m]
	    set C [format %02d [expr { $y / 100 }]]
	    set h $b
	    set j [format %03d $yd]
	    set mm [format %02d $m]
	    set N [format %2d $m]
	    set yy [format %02d [expr { $y % 100 }]]

	    set J [expr { ( $s / 86400 ) + 2440588 }]

	    set dt $y-$mm-01
	    set result ""
	    append result $b " " $B " " \
		$mm /01/ $y " 12:34:56 " \
		"die i mensis " [lindex $roman $m] " annoque " \
		[lindex $romanc [expr { $y / 100 }]] \
		[lindex $roman [expr { $y % 100 }]] " " \
		[lindex $roman 12] " h " [lindex $roman 34] " m " \
		[lindex $roman 56] " s " \
		$C " " [lindex $romanc [expr { $y / 100 }]] \
		" 01 i  1 i " \
		$h " " $j " " $J " " $mm " " [lindex $roman $m] " " $N \
		" " $mm "/01/" $y \
		" die i mensis " [lindex $roman $m] " annoque " \
		[lindex $romanc [expr { $y / 100 }]] \
		[lindex $roman [expr { $y % 100 }]]	\
		" " $yy " " [lindex $roman [expr { $y % 100 }]] " " $y
	    puts $f2 "test clock-2.[incr n] {conversion of $dt} {"
	    puts $f2 "    clock format $s \\"
	    puts $f2 "\t-format {%b %B %c %Ec %C %EC %d %Od %e %Oe %h %j %J %m %Om %N %x %Ex %y %Oy %Y} \\"
	    puts $f2 "\t-gmt true -locale en_US_roman"
	    puts $f2 "} {$result}"

	    set hm1 [expr { $hath - 1 }]
	    incr s [expr { 86400 * ( $hath - 1 ) }]
	    incr yd $hm1

	    set dd [format %02d $hath]
	    set ee [format %2d $hath]
	    set j [format %03d $yd]

	    set J [expr { ( $s / 86400 ) + 2440588 }]

	    set dt $y-$mm-$dd
	    set result ""
	    append result $b " " $B " " \
		$mm / $dd / $y " 12:34:56 " \
		"die " [lindex $roman $hath] " mensis " [lindex $roman $m] \
		" annoque " \
		[lindex $romanc [expr { $y / 100 }]] \
		[lindex $roman [expr { $y % 100 }]] " " \
		[lindex $roman 12] " h " [lindex $roman 34] " m " \
		[lindex $roman 56] " s " \
		$C " " [lindex $romanc [expr { $y / 100 }]] \
		" " $dd " " [lindex $roman $hath] " " \
		$ee " " [lindex $roman $hath] " "\
		$h " " $j " " $J " " $mm " " [lindex $roman $m] " " $N \
		" " $mm "/" $dd "/" $y \
		" die " [lindex $roman $hath] " mensis " [lindex $roman $m] \
		" annoque " \
		[lindex $romanc [expr { $y / 100 }]] \
		[lindex $roman [expr { $y % 100 }]]	\
		" " $yy " " [lindex $roman [expr { $y % 100 }]] " " $y
	    puts $f2 "test clock-2.[incr n] {conversion of $dt} {"
	    puts $f2 "    clock format $s \\"
	    puts $f2 "\t-format {%b %B %c %Ec %C %EC %d %Od %e %Oe %h %j %J %m %Om %N %x %Ex %y %Oy %Y} \\"
	    puts $f2 "\t-gmt true -locale en_US_roman"
	    puts $f2 "} {$result}"

	    incr s 86400
	    incr yd
	}
    }
    puts "testcases2: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases3 --
#
#	Generate test cases for ISO8601 calendar.
#
# Parameters:
#	f2 - Channel handle to the output file
#
# Results:
#	None
#
# Side effects:
#	Makes a test case for the first and last day of weeks 51, 52, and 1
#	plus the first and last day of a year.  Does so for each possible
#	weekday on which a Common Year or Leap Year can begin.
#
#----------------------------------------------------------------------

proc testcases3 { f2 } {

    listYears startOfYear

    set case 0
    foreach { y } [lsort -integer [array names startOfYear]] {
	set secs $startOfYear($y)
	set ym1 [expr { $y - 1 }]
	set dow [expr { ( $secs / 86400  + 4 ) % 7}]
	switch -exact $dow {
	    0 {
		# Year starts on a Sunday.
		# Prior year started on a Friday or Saturday, and was
		# a 52-week year.
		# 1 January is ISO week 52 of the prior year. 2 January
		# begins ISO week 1 of the current year.
		# 1 January is week 1 according to %U. According to %W,
		# week 1 begins on 2 January
		testISO $f2 $ym1 52 1 [expr { $secs - 6*86400 }]
		testISO $f2 $ym1 52 6 [expr { $secs - 86400 }]
		testISO $f2 $ym1 52 7 $secs
		testISO $f2 $y 1 1 [expr { $secs + 86400 }]
		testISO $f2 $y 1 6 [expr { $secs + 6*86400}]
		testISO $f2 $y 1 7 [expr { $secs + 7*86400 }]
		testISO $f2 $y 2 1 [expr { $secs + 8*86400 }]
	    }
	    1 {
		# Year starts on a Monday.
		# Previous year started on a Saturday or Sunday, and was
		# a 52-week year.
		# 1 January is ISO week 1 of the current year
		# According to %U, it's week 0 until 7 January
		# 1 January is week 1 according to %W
		testISO $f2 $ym1 52 1 [expr { $secs - 7*86400 }]
		testISO $f2 $ym1 52 6 [expr {$secs - 2*86400}]
		testISO $f2 $ym1 52 7 [expr { $secs - 86400 }]
		testISO $f2 $y 1 1 $secs
		testISO $f2 $y 1 6 [expr {$secs + 5*86400}]
		testISO $f2 $y 1 7 [expr { $secs + 6*86400 }]
		testISO $f2 $y 2 1 [expr { $secs + 7*86400 }]
	    }
	    2 {
		# Year starts on a Tuesday.
		testISO $f2 $ym1 52 1 [expr { $secs - 8*86400 }]
		testISO $f2 $ym1 52 6 [expr {$secs - 3*86400}]
		testISO $f2 $ym1 52 7 [expr { $secs - 2*86400 }]
		testISO $f2 $y 1 1 [expr { $secs - 86400 }]
		testISO $f2 $y 1 2 $secs
		testISO $f2 $y 1 6 [expr {$secs + 4*86400}]
		testISO $f2 $y 1 7 [expr { $secs + 5*86400 }]
		testISO $f2 $y 2 1 [expr { $secs + 6*86400 }]
	    }
	    3 {
		testISO $f2 $ym1 52 1 [expr { $secs - 9*86400 }]
		testISO $f2 $ym1 52 6 [expr {$secs - 4*86400}]
		testISO $f2 $ym1 52 7 [expr { $secs - 3*86400 }]
		testISO $f2 $y 1 1 [expr { $secs - 2*86400 }]
		testISO $f2 $y 1 3 $secs
		testISO $f2 $y 1 6 [expr {$secs + 3*86400}]
		testISO $f2 $y 1 7 [expr { $secs + 4*86400 }]
		testISO $f2 $y 2 1 [expr { $secs + 5*86400 }]
	    }
	    4 {
		testISO $f2 $ym1 52 1 [expr { $secs - 10*86400 }]
		testISO $f2 $ym1 52 6 [expr {$secs - 5*86400}]
		testISO $f2 $ym1 52 7 [expr { $secs - 4*86400 }]
		testISO $f2 $y 1 1 [expr { $secs - 3*86400 }]
		testISO $f2 $y 1 4 $secs
		testISO $f2 $y 1 6 [expr {$secs + 2*86400}]
		testISO $f2 $y 1 7 [expr { $secs + 3*86400 }]
		testISO $f2 $y 2 1 [expr { $secs + 4*86400 }]
	    }
	    5 {
		testISO $f2 $ym1 53 1 [expr { $secs - 4*86400 }]
		testISO $f2 $ym1 53 5 $secs
		testISO $f2 $ym1 53 6 [expr {$secs + 86400}]
		testISO $f2 $ym1 53 7 [expr { $secs + 2*86400 }]
		testISO $f2 $y 1 1 [expr { $secs + 3*86400 }]
		testISO $f2 $y 1 6 [expr {$secs + 8*86400}]
		testISO $f2 $y 1 7 [expr { $secs + 9*86400 }]
		testISO $f2 $y 2 1 [expr { $secs + 10*86400 }]
	    }
	    6 {
		# messy case because previous year may have had 52 or 53 weeks
		if { $y%4 == 1 } {
		    testISO $f2 $ym1 53 1 [expr { $secs - 5*86400 }]
		    testISO $f2 $ym1 53 6 $secs
		    testISO $f2 $ym1 53 7 [expr { $secs + 86400 }]
		} else {
		    testISO $f2 $ym1 52 1 [expr { $secs - 5*86400 }]
		    testISO $f2 $ym1 52 6 $secs
		    testISO $f2 $ym1 52 7 [expr { $secs + 86400 }]
		}
		testISO $f2 $y 1 1 [expr { $secs + 2*86400 }]
		testISO $f2 $y 1 6 [expr { $secs + 7*86400 }]
		testISO $f2 $y 1 7 [expr { $secs + 8*86400 }]
		testISO $f2 $y 2 1 [expr { $secs + 9*86400 }]
	    }
	}
    }
    puts "testcases3: $case test cases."

}

proc testISO { f2 G V u secs } {

    upvar 1 case case

    set longdays {Sunday Monday Tuesday Wednesday Thursday Friday Saturday Sunday}
    set shortdays {Sun Mon Tue Wed Thu Fri Sat Sun}

    puts $f2 "test clock-3.[incr case] {ISO week-based calendar [format %04d-W%02d-%d $G $V $u]} {"
    puts $f2 "    clock format $secs -format {%a %A %g %G %u %U %V %w %W} -gmt true; \# $G-W[format %02d $V]-$u"
    puts $f2 "} {[lindex $shortdays $u] [lindex $longdays $u]\
             [format %02d [expr { $G % 100 }]] $G\
             $u\
             [clock format $secs -format %U -gmt true]\
             [format %02d $V] [expr { $u % 7 }]\
             [clock format $secs -format %W -gmt true]}"

}

#----------------------------------------------------------------------
#
# testcases4 --
#
#	Makes the test cases that test formatting of time of day.
#
# Parameters:
#	f2 - Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Writes test cases to the output.
#
#----------------------------------------------------------------------

proc testcases4 { f2 } {

    puts $f2 {}
    puts $f2 "\# Test formatting of time of day"
    puts $f2 "\# Format groups tested: %H %OH %I %OI %k %Ok %l %Ol %M %OM %p %P %r %R %S %OS %T %X %EX %+"
    puts $f2 {}

    set i 0
    set fmt "%H %OH %I %OI %k %Ok %l %Ol %M %OM %p %P %r %R %S %OS %T %X %EX %+"
    foreach { h romanH I romanI am } {
	0 ? 12 xii AM
	1 i 1 i AM
	11 xi 11 xi AM
	12 xii 12 xii PM
	13 xiii 1 i PM
	23 xxiii 11 xi PM
    } {
	set hh [format %02d $h]
	set II [format %02d $I]
	set hs [format %2d $h]
	set Is [format %2d $I]
	foreach { m romanM } { 0 ? 1 i 58 lviii 59 lix } {
	    set mm [format %02d $m]
	    foreach { s romanS } { 0 ? 1 i 58 lviii 59 lix } {
		set ss [format %02d $s]
		set x [expr { ( $h * 60 + $m ) * 60 + $s }]
		set result ""
		append result $hh " " $romanH " " $II " " $romanI " " \
		    $hs " " $romanH " " $Is " " $romanI " " $mm " " $romanM " " \
		    $am " " [string tolower $am] " " \
		    $II ":" $mm ":" $ss " " [string tolower $am] " " \
		    $hh ":" $mm " " \
		    $ss " " $romanS " " \
		    $hh ":" $mm ":" $ss " " \
		    $hh ":" $mm ":" $ss " " \
		    $romanH " h " $romanM " m " $romanS " s " \
		    "Thu Jan  1 " $hh : $mm : $ss " GMT 1970"
		puts $f2 "test clock-4.[incr i] { format time of day $hh:$mm:$ss } {"
		puts $f2 "    clock format $x \\"
		puts $f2 "        -format [list $fmt] \\"
		puts $f2 "	  -locale en_US_roman \\"
		puts $f2 "        -gmt true"
		puts $f2 "} {$result}"
	    }
	}
    }

    puts "testcases4: $i test cases."
}

#----------------------------------------------------------------------
#
# testcases5 --
#
#	Generates the test cases for Daylight Saving Time
#
# Parameters:
#	f2 - Channel handle for the input file
#
# Results:
#	None.
#
# Side effects:
#	Makes test cases for each known or anticipated time change
#	in Detroit.
#
#----------------------------------------------------------------------

proc testcases5 { f2 } {
    variable TZData

    puts $f2 {}
    puts $f2 "\# Test formatting of Daylight Saving Time"
    puts $f2 {}

    set fmt {%H:%M:%S %z %Z}

    set i 0
    puts $f2 "test clock-5.[incr i] {does Detroit exist} {"
    puts $f2 "    clock format 0 -format {} -timezone :America/Detroit"
    puts $f2 "    concat"
    puts $f2 "} {}"
    puts $f2 "test clock-5.[incr i] {does Detroit have a Y2038 problem} detroit {"
    puts $f2 "    if { \[clock format 2158894800 -format %z -timezone :America/Detroit\] ne {-0400} } {"
    puts $f2 "        concat {y2038 problem}"
    puts $f2 "    } else {"
    puts $f2 "        concat {ok}"
    puts $f2 "    }"
    puts $f2 "} ok"

    foreach row $TZData(:America/Detroit) {
	foreach { t offset isdst tzname } $row break
	if { $t > -4000000000000 } {
	    set conds [list detroit]
	    if { $t > wide(0x7fffffff) } {
		set conds [list detroit y2038]
	    }
	    incr t -1
	    set x [clock format $t -format {%Y-%m-%d %H:%M:%S} \
		       -timezone :America/Detroit]
	    set r [clock format $t -format $fmt \
		       -timezone :America/Detroit]
	    puts $f2 "test clock-5.[incr i] {time zone boundary case $x} [list $conds] {"
	    puts $f2 "    clock format $t -format [list $fmt] \\"
	    puts $f2 "        -timezone :America/Detroit"
	    puts $f2 "} [list $r]"
	    incr t
	    set x [clock format $t -format {%Y-%m-%d %H:%M:%S} \
		       -timezone :America/Detroit]
	    set r [clock format $t -format $fmt \
		       -timezone :America/Detroit]
	    puts $f2 "test clock-5.[incr i] {time zone boundary case $x} [list $conds] {"
	    puts $f2 "    clock format $t -format [list $fmt] \\"
	    puts $f2 "        -timezone :America/Detroit"
	    puts $f2 "} [list $r]"
	    incr t
	    set x [clock format $t -format {%Y-%m-%d %H:%M:%S} \
		       -timezone :America/Detroit]
	    set r [clock format $t -format $fmt \
		       -timezone :America/Detroit]
	    puts $f2 "test clock-5.[incr i] {time zone boundary case $x} [list $conds] {"
	    puts $f2 "    clock format $t -format [list $fmt] \\"
	    puts $f2 "        -timezone :America/Detroit"
	    puts $f2 "} [list $r]"
	}
    }
    puts "testcases5: $i test cases"
}

#----------------------------------------------------------------------
#
# testcases8 --
#
#	Outputs the 'clock-8.x' test cases.
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for parsing dates in ccyymmdd format are written to the
#	output file.
#
#----------------------------------------------------------------------

proc testcases8 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of ccyymmdd"
    puts $f2 ""

    set n 0
    foreach year {1970 1971 2000 2001} {
	foreach month {01 12} {
	    foreach day {02 31} {
		set scanned [clock scan $year$month$day -gmt true]
		foreach ccyy {%C%y %Y} {
		    foreach mm {%b %B %h %m %Om %N} {
			foreach dd {%d %Od %e %Oe} {
			    set string [clock format $scanned \
					    -format "$ccyy $mm $dd" \
					    -locale en_US_roman \
					    -gmt true]
			    puts $f2 "test clock-8.[incr n] {parse ccyymmdd} {"
			    puts $f2 "    [list clock scan $string -format [list $ccyy $mm $dd] -locale en_US_roman -gmt 1]"
			    puts $f2 "} $scanned"
			}
		    }
		}
		foreach fmt {%x %D} {
		    set string [clock format $scanned \
				    -format $fmt \
				    -locale en_US_roman \
				    -gmt true]
		    puts $f2 "test clock-8.[incr n] {parse ccyymmdd} {"
		    puts $f2 "    [list clock scan $string -format $fmt -locale en_US_roman -gmt 1]"
		    puts $f2 "} $scanned"
		}
	    }
	}
    }

    puts "testcases8: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases11 --
#
#	Outputs the 'clock-11.x' test cases.
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for precedence among YYYYMMDD and YYYYDDD are written
#	to f2.
#
#----------------------------------------------------------------------

proc testcases11 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test precedence among yyyymmdd and yyyyddd"
    puts $f2 ""

    array set v {
	Y 1970
	m 01
	d 01
	j 002
    }

    set n 0

    foreach {a b c d} {
	Y m d j		m Y d j		d Y m j		j Y m d
	Y m j d		m Y j d		d Y j m		j Y d m
	Y d m j		m d Y j		d m Y j		j m Y d
	Y d j m		m d j Y		d m j Y		j m d Y
	Y j m d		m j Y d		d j Y m		j d Y m
	Y j d m		m j d Y		d j m Y		j d m Y
    } {
	foreach x [list $a $b $c $d] {
	    switch -exact -- $x {
		m - d {
		    set value 0
		}
		j {
		    set value 86400
		}
	    }
	}
	set format "%$a%$b%$c%$d"
	set string "$v($a)$v($b)$v($c)$v($d)"
	puts $f2 "test clock-11.[incr n] {precedence of ccyyddd and ccyymmdd} {"
	puts $f2 "    [list clock scan $string -format $format -gmt 1]"
	puts $f2 "} $value"
    }

    puts "testcases11: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases12 --
#
#	Outputs the 'clock-12.x' test cases, parsing CCyyWwwd
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for parsing dates in Gregorian calendar are written to the
#	output file.
#
#----------------------------------------------------------------------

proc testcases12 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of ccyyWwwd"
    puts $f2 ""

    set n 0
    foreach year {1970 1971 2000 2001} {
	foreach month {01 12} {
	    foreach day {02 31} {
		set scanned [clock scan $year$month$day -gmt true]
		foreach d {%a %A %u %w %Ou %Ow} {
		    set string [clock format $scanned \
				    -format "%G W%V $d" \
				    -locale en_US_roman \
				    -gmt true]
		    puts $f2 "test clock-12.[incr n] {parse ccyyWwwd} {"
		    puts $f2 "    [list clock scan $string -format [list %G W%V $d] -locale en_US_roman -gmt 1]"
		    puts $f2 "} $scanned"
		}
	    }
	}
    }

    puts "testcases12: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases14 --
#
#	Outputs the 'clock-14.x' test cases.
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for parsing yymmdd dates are output.
#
#----------------------------------------------------------------------

proc testcases14 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of yymmdd"
    puts $f2 ""

    set n 0
    foreach year {1938 1970 2000 2037} {
	foreach month {01 12} {
	    foreach day {02 31} {
		set scanned [clock scan $year$month$day -gmt true]
		foreach yy {%y %Oy} {
		    foreach mm {%b %B %h %m %Om %N} {
			foreach dd {%d %Od %e %Oe} {
			    set string [clock format $scanned \
					    -format "$yy $mm $dd" \
					    -locale en_US_roman \
					    -gmt true]
			    puts $f2 "test clock-14.[incr n] {parse yymmdd} {"
			    puts $f2 "    [list clock scan $string -format [list $yy $mm $dd] -locale en_US_roman -gmt 1]"
			    puts $f2 "} $scanned"
			}
		    }
		}
	    }
	}
    }

    puts "testcases14: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases17 --
#
#	Outputs the 'clock-17.x' test cases, parsing yyWwwd
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for parsing dates in Gregorian calendar are written to the
#	output file.
#
#----------------------------------------------------------------------

proc testcases17 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of yyWwwd"
    puts $f2 ""

    set n 0
    foreach year {1970 1971 2000 2001} {
	foreach month {01 12} {
	    foreach day {02 31} {
		set scanned [clock scan $year$month$day -gmt true]
		foreach d {%a %A %u %w %Ou %Ow} {
		    set string [clock format $scanned \
				    -format "%g W%V $d" \
				    -locale en_US_roman \
				    -gmt true]
		    puts $f2 "test clock-17.[incr n] {parse yyWwwd} {"
		    puts $f2 "    [list clock scan $string -format [list %g W%V $d] -locale en_US_roman -gmt 1]"
		    puts $f2 "} $scanned"
		}
	    }
	}
    }

    puts "testcases17: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases19 --
#
#	Outputs the 'clock-19.x' test cases.
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for parsing mmdd dates are output.
#
#----------------------------------------------------------------------

proc testcases19 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of mmdd"
    puts $f2 ""

    set n 0
    foreach year {1938 1970 2000 2037} {
	set base [clock scan ${year}0101 -gmt true]
	foreach month {01 12} {
	    foreach day {02 31} {
		set scanned [clock scan $year$month$day -gmt true]
		foreach mm {%b %B %h %m %Om %N} {
		    foreach dd {%d %Od %e %Oe} {
			set string [clock format $scanned \
					-format "$mm $dd" \
					-locale en_US_roman \
					-gmt true]
			puts $f2 "test clock-19.[incr n] {parse mmdd} {"
			puts $f2 "    [list clock scan $string -format [list $mm $dd] -locale en_US_roman -base $base -gmt 1]"
			puts $f2 "} $scanned"
		    }
		}
	    }
	}
    }

    puts "testcases19: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases21 --
#
#	Outputs the 'clock-21.x' test cases, parsing Wwwd
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for parsing dates in Gregorian calendar are written to the
#	output file.
#
#----------------------------------------------------------------------

proc testcases22 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of Wwwd"
    puts $f2 ""

    set n 0
    foreach year {1970 1971 2000 2001} {
	set base [clock scan ${year}0104 -gmt true]
	foreach month {03 10} {
	    foreach day {01 31} {
		set scanned [clock scan $year$month$day -gmt true]
		foreach d {%a %A %u %w %Ou %Ow} {
		    set string [clock format $scanned \
				    -format "W%V $d" \
				    -locale en_US_roman \
				    -gmt true]
		    puts $f2 "test clock-22.[incr n] {parse Wwwd} {"
		    puts $f2 "    [list clock scan $string -format [list W%V $d] -locale en_US_roman -gmt 1] -base $base"
		    puts $f2 "} $scanned"
		}
	    }
	}
    }

    puts "testcases22: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases24 --
#
#	Outputs the 'clock-24.x' test cases.
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for parsing naked day of the month are output.
#
#----------------------------------------------------------------------

proc testcases24 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of naked day-of-month"
    puts $f2 ""

    set n 0
    foreach year {1970 2000} {
	foreach month {01 12} {
	    set base [clock scan ${year}${month}01 -gmt true]
	    foreach day {02 28} {
		set scanned [clock scan $year$month$day -gmt true]
		foreach dd {%d %Od %e %Oe} {
		    set string [clock format $scanned \
				    -format "$dd" \
				    -locale en_US_roman \
				    -gmt true]
		    puts $f2 "test clock-24.[incr n] {parse naked day of month} {"
		    puts $f2 "    [list clock scan $string -format $dd -locale en_US_roman -base $base -gmt 1]"
		    puts $f2 "} $scanned"
		}
	    }
	}
    }

    puts "testcases24: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases26 --
#
#	Outputs the 'clock-26.x' test cases, parsing naked day of week
#
# Parameters:
#	f2 -- Channel handle to the output file
#
# Results:
#	None.
#
# Side effects:
#	Test cases for parsing dates in Gregorian calendar are written to the
#	output file.
#
#----------------------------------------------------------------------

proc testcases26 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of naked day of week"
    puts $f2 ""

    set n 0
    foreach year {1970 2001} {
	foreach week {01 52} {
	    set base [clock scan ${year}W${week}4 \
			  -format %GW%V%u -gmt true]
	    foreach day {1 7} {
		set scanned [clock scan ${year}W${week}${day} \
				 -format %GW%V%u -gmt true]
		foreach d {%a %A %u %w %Ou %Ow} {
		    set string [clock format $scanned \
				    -format "$d" \
				    -locale en_US_roman \
				    -gmt true]
		    puts $f2 "test clock-26.[incr n] {parse naked day of week} {"
		    puts $f2 "    [list clock scan $string -format $d -locale en_US_roman -gmt 1] -base $base"
		    puts $f2 "} $scanned"
		}
	    }
	}
    }

    puts "testcases26: $n test cases"
}

#----------------------------------------------------------------------
#
# testcases29 --
#
#	Makes test cases for parsing of time of day.
#
# Parameters:
#	f2 -- Channel where tests are to be written
#
# Results:
#	None.
#
# Side effects:
#	Writes the tests.
#
#----------------------------------------------------------------------

proc testcases29 { f2 } {

    # Put out a header describing the tests

    puts $f2 ""
    puts $f2 "\# Test parsing of time of day"
    puts $f2 ""

    set n 0
    foreach hour {0 1 11 12 13 23} \
	hampm {12 1 11 12 1 11} \
	lhour {? i xi xii xiii xxiii} \
	lhampm {xii i xi xii i xi} \
	ampmind {am am am pm pm pm} {
	    set sphr [format %2d $hour]
	    set 2dhr [format %02d $hour]
	    set sphampm [format %2d $hampm]
	    set 2dhampm [format %02d $hampm]
	    set AMPMind [string toupper $ampmind]
	    foreach minute {00 01 59} lminute {? i lix} {
		foreach second {00 01 59} lsecond {? i lix} {
		    set time [expr { ( 60 * $hour + $minute ) * 60 + $second }]
		    foreach {hfmt afmt} [list \
					     %H {} %k {} %OH {} %Ok {} \
					     %I %p %l %p \
					     %OI %p %Ol %p \
					     %I %P %l %P \
					     %OI %P %Ol %P] \
			{hfld afld} [list \
					 $2dhr {} $sphr {} $lhour {} $lhour {} \
					 $2dhampm $AMPMind $sphampm $AMPMind \
					 $lhampm $AMPMind $lhampm $AMPMind \
					 $2dhampm $ampmind $sphampm $ampmind \
					 $lhampm $ampmind $lhampm $ampmind] \
			{
			    if { $second eq "00" } {
				if { $minute eq "00" } {
				    puts $f2 "test clock-29.[incr n] {time parsing} {"
				    puts $f2 "    clock scan {2440588 $hfld $afld} \\"
				    puts $f2 "        -gmt true -locale en_US_roman \\"
				    puts $f2 "        -format {%J $hfmt $afmt}"
				    puts $f2 "} $time"
				}
				puts $f2 "test clock-29.[incr n] {time parsing} {"
				puts $f2 "    clock scan {2440588 $hfld:$minute $afld} \\"
				puts $f2 "        -gmt true -locale en_US_roman \\"
				puts $f2 "        -format {%J $hfmt:%M $afmt}"
				puts $f2 "} $time"
				puts $f2 "test clock-29.[incr n] {time parsing} {"
				puts $f2 "    clock scan {2440588 $hfld:$lminute $afld} \\"
				puts $f2 "        -gmt true -locale en_US_roman \\"
				puts $f2 "        -format {%J $hfmt:%OM $afmt}"
				puts $f2 "} $time"
			    }
			    puts $f2 "test clock-29.[incr n] {time parsing} {"
			    puts $f2 "    clock scan {2440588 $hfld:$minute:$second $afld} \\"
			    puts $f2 "        -gmt true -locale en_US_roman \\"
			    puts $f2 "        -format {%J $hfmt:%M:%S $afmt}"
			    puts $f2 "} $time"
			    puts $f2 "test clock-29.[incr n] {time parsing} {"
			    puts $f2 "    clock scan {2440588 $hfld:$lminute:$lsecond $afld} \\"
			    puts $f2 "        -gmt true -locale en_US_roman \\"
			    puts $f2 "        -format {%J $hfmt:%OM:%OS $afmt}"
			    puts $f2 "} $time"
			}
		}
	    }

	}
    puts "testcases29: $n test cases"
}

processFile $d
