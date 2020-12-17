#----------------------------------------------------------------------
#
# loadICU,tcl --
#
#	Extracts locale strings from a distribution of ICU
#	(http://oss.software.ibm.com/developerworks/opensource/icu/project/)
#	and makes Tcl message catalogs for the 'clock' command.
#
# Usage:
#	loadICU.tcl sourceDir destDir
#
# Parameters:
#	sourceDir -- Path name of the 'data' directory of your ICU4C
#		     distribution.
#	destDir -- Directory into which the Tcl message catalogs should go.
#
# Results:
#	None.
#
# Side effects:
#	Creates the message catalogs.
#
#----------------------------------------------------------------------
#
# Copyright (c) 2004 by Kevin B. Kenny.  All rights reserved.
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#----------------------------------------------------------------------

# Calculate the Chinese numerals from zero to ninety-nine.

set zhDigits [list {} \u4e00 \u4e8c \u4e09 \u56db \
		  \u4e94 \u516d \u4e03 \u516b \u4e5d]
set t 0
foreach zt $zhDigits {
    if { $t == 0 } {
	set zt {}
    } elseif { $t == 10 } {
	set zt \u5341
    } else {
	append zt \u5341
    }
    set d 0
    foreach zd $zhDigits {
	if { $t == 0 && $d == 0 } {
	    set zd \u3007
	} elseif { $t == 20 && $d != 0 } {
	    set zt \u5eff
	} elseif { $t == 30 && $d != 0 } {
	    set zt \u5345
	}
	lappend zhNumbers $zt$zd
	incr d
    }
    incr t 10
}

# Set format overrides for various locales.

set format(zh,LOCALE_NUMERALS) $zhNumbers
set format(ja,LOCALE_ERAS) [list \
				[list -9223372036854775808 \u897f\u66a6 0 ] \
				[list -3061011600 \u660e\u6cbb 1867] \
				[list -1812186000 \u5927\u6b63 1911] \
				[list -1357635600 \u662d\u548c 1925] \
				[list 600220800 \u5e73\u6210 1988]]
set format(zh,LOCALE_DATE_FORMAT) "\u516c\u5143%Y\u5e74%B%Od\u65E5"
set format(ja,LOCALE_DATE_FORMAT) "%EY\u5e74%m\u6708%d\u65E5"
set format(ko,LOCALE_DATE_FORMAT) "%Y\ub144%B%Od\uc77c"
set format(zh,LOCALE_TIME_FORMAT) "%OH\u65f6%OM\u5206%OS\u79d2"
set format(ja,LOCALE_TIME_FORMAT) "%H\u6642%M\u5206%S\u79d2"
set format(ko,LOCALE_TIME_FORMAT) "%H\uc2dc%M\ubd84%S\ucd08"
set format(zh,LOCALE_DATE_TIME_FORMAT) "%A %Y\u5e74%B%Od\u65E5%OH\u65f6%OM\u5206%OS\u79d2 %z"
set format(ja,LOCALE_DATE_TIME_FORMAT) "%EY\u5e74%m\u6708%d\u65E5 (%a) %H\u6642%M\u5206%S\u79d2 %z"
set format(ko,LOCALE_DATE_TIME_FORMAT) "%A %Y\ub144%B%Od\uc77c%H\uc2dc%M\ubd84%S\ucd08 %z"
set format(ja,TIME_FORMAT_12) {%P %I:%M:%S}

# The next set of format overrides were obtained from the glibc
# localization strings.

set format(cs_CZ,DATE_FORMAT) %d.%m.%Y
set format(cs_CZ,DATE_TIME_FORMAT) {%a %e. %B %Y, %H:%M:%S %z}
set format(cs_CZ,TIME_FORMAT) %H:%M:%S
set format(cs_CZ,TIME_FORMAT_12) %I:%M:%S
set format(da_DK,DATE_FORMAT) %d-%m-%Y
set format(da_DK,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(da_DK,TIME_FORMAT) %T
set format(da_DK,TIME_FORMAT_12) %T
set format(de_AT,DATE_FORMAT) %Y-%m-%d
set format(de_AT,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(de_AT,TIME_FORMAT) %T
set format(de_AT,TIME_FORMAT_12) %T
set format(de_BE,DATE_FORMAT) %Y-%m-%d
set format(de_BE,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(de_BE,TIME_FORMAT) %T
set format(de_BE,TIME_FORMAT_12) %T
set format(de_CH,DATE_FORMAT) %Y-%m-%d
set format(de_CH,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(de_CH,TIME_FORMAT) %T
set format(de_CH,TIME_FORMAT_12) %T
set format(de_DE,DATE_FORMAT) %Y-%m-%d
set format(de_DE,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(de_DE,TIME_FORMAT) %T
set format(de_DE,TIME_FORMAT_12) %T
set format(de_LU,DATE_FORMAT) %Y-%m-%d
set format(de_LU,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(de_LU,TIME_FORMAT) %T
set format(de_LU,TIME_FORMAT_12) %T
set format(en_CA,DATE_FORMAT) %d/%m/%y
set format(en_CA,DATE_TIME_FORMAT) {%a %d %b %Y %r %z}
set format(en_CA,TIME_FORMAT) %r
set format(en_CA,TIME_FORMAT_12) {%I:%M:%S %p}
set format(en_DK,DATE_FORMAT) %Y-%m-%d
set format(en_DK,DATE_TIME_FORMAT) {%Y-%m-%dT%T %z}
set format(en_DK,TIME_FORMAT) %T
set format(en_DK,TIME_FORMAT_12) %T
set format(en_GB,DATE_FORMAT) %d/%m/%y
set format(en_GB,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(en_GB,TIME_FORMAT) %T
set format(en_GB,TIME_FORMAT_12) %T
set format(en_IE,DATE_FORMAT) %d/%m/%y
set format(en_IE,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(en_IE,TIME_FORMAT) %T
set format(en_IE,TIME_FORMAT_12) %T
set format(en_US,DATE_FORMAT) %m/%d/%y
set format(en_US,DATE_TIME_FORMAT) {%a %d %b %Y %r %z}
set format(en_US,TIME_FORMAT) %r
set format(en_US,TIME_FORMAT_12) {%I:%M:%S %p}
set format(es_ES,DATE_FORMAT) %d/%m/%y
set format(es_ES,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(es_ES,TIME_FORMAT) %T
set format(es_ES,TIME_FORMAT_12) %T
set format(et_EE,DATE_FORMAT) %d.%m.%Y
set format(et_EE,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(et_EE,TIME_FORMAT) %T
set format(et_EE,TIME_FORMAT_12) %T
set format(eu_ES,DATE_FORMAT) {%a, %Yeko %bren %da}
set format(eu_ES,DATE_TIME_FORMAT) {%y-%m-%d %T %z}
set format(eu_ES,TIME_FORMAT) %T
set format(eu_ES,TIME_FORMAT_12) %T
set format(fi_FI,DATE_FORMAT) %d.%m.%Y
set format(fi_FI,DATE_TIME_FORMAT) {%a %e %B %Y %T}
set format(fi_FI,TIME_FORMAT) %T
set format(fi_FI,TIME_FORMAT_12) %T
set format(fo_FO,DATE_FORMAT) %d/%m-%Y
set format(fo_FO,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(fo_FO,TIME_FORMAT) %T
set format(fo_FO,TIME_FORMAT_12) %T
set format(fr_BE,DATE_FORMAT) %d/%m/%y
set format(fr_BE,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(fr_BE,TIME_FORMAT) %T
set format(fr_BE,TIME_FORMAT_12) %T
set format(fr_CA,DATE_FORMAT) %Y-%m-%d
set format(fr_CA,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(fr_CA,TIME_FORMAT) %T
set format(fr_CA,TIME_FORMAT_12) %T
set format(fr_CH,DATE_FORMAT) {%d. %m. %y}
set format(fr_CH,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(fr_CH,TIME_FORMAT) %T
set format(fr_CH,TIME_FORMAT_12) %T
set format(fr_FR,DATE_FORMAT) %d.%m.%Y
set format(fr_FR,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(fr_FR,TIME_FORMAT) %T
set format(fr_FR,TIME_FORMAT_12) %T
set format(fr_LU,DATE_FORMAT) %d.%m.%Y
set format(fr_LU,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(fr_LU,TIME_FORMAT) %T
set format(fr_LU,TIME_FORMAT_12) %T
set format(ga_IE,DATE_FORMAT) %d.%m.%y
set format(ga_IE,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(ga_IE,TIME_FORMAT) %T
set format(ga_IE,TIME_FORMAT_12) %T
set format(gr_GR,DATE_FORMAT) %d/%m/%Y
set format(gr_GR,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(gr_GR,TIME_FORMAT) %T
set format(gr_GR,TIME_FORMAT_12) %T
set format(hr_HR,DATE_FORMAT) %d.%m.%y
set format(hr_HR,DATE_TIME_FORMAT) {%a %d %b %Y %T}
set format(hr_HR,TIME_FORMAT) %T
set format(hr_HR,TIME_FORMAT_12) %T
set format(hu_HU,DATE_FORMAT) %Y-%m-%d
set format(hu_HU,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(hu_HU,TIME_FORMAT) %T
set format(hu_HU,TIME_FORMAT_12) %T
set format(is_IS,DATE_FORMAT) {%a %e.%b %Y}
set format(is_IS,DATE_TIME_FORMAT) {%a %e.%b %Y, %T %z}
set format(is_IS,TIME_FORMAT) %T
set format(is_IS,TIME_FORMAT_12) %T
set format(it_IT,DATE_FORMAT) %d/%m/%Y
set format(it_IT,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(it_IT,TIME_FORMAT) %T
set format(it_IT,TIME_FORMAT_12) %T
set format(iw_IL,DATE_FORMAT) %d/%m/%y
set format(iw_IL,DATE_TIME_FORMAT) {%z %H:%M:%S %Y %b %d %a}
set format(iw_IL,TIME_FORMAT) %H:%M:%S
set format(iw_IL,TIME_FORMAT_12) {%I:%M:%S %P}
set format(kl_GL,DATE_FORMAT) {%d %b %Y}
set format(kl_GL,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(kl_GL,TIME_FORMAT) %T
set format(kl_GL,TIME_FORMAT_12) %T
set format(lt_LT,DATE_FORMAT) %Y.%m.%d
set format(lt_LT,DATE_TIME_FORMAT) {%Y m. %B %d d. %T}
set format(lt_LT,TIME_FORMAT) %T
set format(lt_LT,TIME_FORMAT_12) %T
set format(lv_LV,DATE_FORMAT) %Y.%m.%d.
set format(lv_LV,DATE_TIME_FORMAT) {%A, %Y. gada %e. %B, plkst. %H un %M}
set format(lv_LV,TIME_FORMAT) %T
set format(lv_LV,TIME_FORMAT_12) %T
set format(nl_BE,DATE_FORMAT) %d-%m-%y
set format(nl_BE,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(nl_BE,TIME_FORMAT) %T
set format(nl_BE,TIME_FORMAT_12) %T
set format(nl_NL,DATE_FORMAT) %d-%m-%y
set format(nl_NL,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(nl_NL,TIME_FORMAT) %T
set format(nl_NL,TIME_FORMAT_12) %T
set format(no_NO,DATE_FORMAT) %d-%m-%Y
set format(no_NO,DATE_TIME_FORMAT) {%a %d-%m-%Y %T %z}
set format(no_NO,TIME_FORMAT) %T
set format(no_NO,TIME_FORMAT_12) %T
set format(pl_PL,DATE_FORMAT) %Y-%m-%d
set format(pl_PL,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(pl_PL,TIME_FORMAT) %T
set format(pl_PL,TIME_FORMAT_12) %T
set format(pt_BR,DATE_FORMAT) %d-%m-%Y
set format(pt_BR,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(pt_BR,TIME_FORMAT) %T
set format(pt_BR,TIME_FORMAT_12) %T
set format(pt_PT,DATE_FORMAT) %d-%m-%Y
set format(pt_PT,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(pt_PT,TIME_FORMAT) %T
set format(pt_PT,TIME_FORMAT_12) %T
set format(ro_RO,DATE_FORMAT) %Y-%m-%d
set format(ro_RO,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(ro_RO,TIME_FORMAT) %T
set format(ro_RO,TIME_FORMAT_12) %T
set format(ru_RU,DATE_FORMAT) %d.%m.%Y
set format(ru_RU,DATE_TIME_FORMAT) {%a %d %b %Y %T}
set format(ru_RU,TIME_FORMAT) %T
set format(ru_RU,TIME_FORMAT_12) %T
set format(sl_SI,DATE_FORMAT) %d.%m.%Y
set format(sl_SI,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(sl_SI,TIME_FORMAT) %T
set format(sl_SI,TIME_FORMAT_12) %T
set format(sv_FI,DATE_FORMAT) %Y-%m-%d
set format(sv_FI,DATE_TIME_FORMAT) {%a %e %b %Y %H.%M.%S}
set format(sv_FI,TIME_FORMAT) %H.%M.%S
set format(sv_FI,TIME_FORMAT_12) %H.%M.%S
set format(sv_SE,DATE_FORMAT) %Y-%m-%d
set format(sv_SE,DATE_TIME_FORMAT) {%a %e %b %Y %H.%M.%S}
set format(sv_SE,TIME_FORMAT) %H.%M.%S
set format(sv_SE,TIME_FORMAT_12) %H.%M.%S
set format(tr_TR,DATE_FORMAT) %Y-%m-%d
set format(tr_TR,DATE_TIME_FORMAT) {%a %d %b %Y %T %z}
set format(tr_TR,TIME_FORMAT) %T
set format(tr_TR,TIME_FORMAT_12) %T

#----------------------------------------------------------------------
#
# handleLocaleFile --
#
#	Extracts strings from an ICU locale definition.
#
# Parameters:
#	localeName - Name of the locale (e.g., de_AT_euro)
#	fileName - Name of the file containing the data
#	msgFileName - Name of the file containing the Tcl message catalog
#
# Results:
#	None.
#
# Side effects:
#	Writes the Tcl message catalog.
#
#----------------------------------------------------------------------

proc handleLocaleFile { localeName fileName msgFileName } {
    variable format

    # Get the content of the ICU file

    set f [open $fileName r]
    fconfigure $f -encoding utf-8
    set data [read $f]
    close $f

    # Parse the ICU data

    set state {}
    foreach line [split $data \n] {
	switch -exact -- $state {
	    {} {

		# Look for the beginnings of data blocks

		switch -regexp -- $line {
		    {^[[:space:]]*AmPmMarkers[[:space:]]+[\{]} {
			set state data
			set key AmPmMarkers
		    }
		    {^[[:space:]]*DateTimePatterns[[:space:]]+[\{]} {
			set state data
			set key DateTimePatterns
		    }
		    {^[[:space:]]*DayAbbreviations[[:space:]]+[\{]} {
			set state data
			set key DayAbbreviations
		    }
		    {^[[:space:]]*DayNames[[:space:]]+[\{]} {
			set state data
			set key DayNames
		    }
		    {^[[:space:]]*Eras[[:space:]]+[\{]} {
			set state data
			set key Eras
		    }
		    {^[[:space:]]*MonthAbbreviations[[:space:]]+[\{]} {
			set state data
			set key MonthAbbreviations
		    }
		    {^[[:space:]]*MonthNames[[:space:]]+[\{]} {
			set state data
			set key MonthNames
		    }
		}
	    }
	    data {


		# Inside a data block, collect the strings, doing backslash
		# expansion to pick up the Unicodes

		if { [regexp {"(.*)",} $line -> item] } {
		    lappend items($key) [subst -nocommands -novariables $item]
		} elseif { [regexp {^[[:space:]]*[\}][[:space:]]*$} $line] } {
		    set state {}
		}
	    }
	}
    }

    # Skip locales that don't change time strings.

    if {![array exists items]} return

    # Write the Tcl message catalog

    set f [open $msgFileName w]

    # Write a header

    puts $f "\# created by $::argv0 -- do not edit"
    puts $f "namespace eval ::tcl::clock \{"

    # Do ordinary sets of strings (weekday and month names)

    foreach key {
	DayAbbreviations DayNames MonthAbbreviations MonthNames
    } tkey {
	DAYS_OF_WEEK_ABBREV DAYS_OF_WEEK_FULL
	MONTHS_ABBREV MONTHS_FULL
    } {
	if { [info exists items($key)] } {
	    set itemList $items($key)
	    set cmd1 "    ::msgcat::mcset "
	    append cmd1 $localeName " " $tkey " \[list "
	    foreach item $itemList {
		append cmd1 \\\n {        } \" [backslashify $item] \"
	    }
	    append cmd1 \]
	    puts $f $cmd1
	}
    }

    # Do the eras, B.C.E., and C.E.

    if { [info exists items(Eras)] } {
	foreach { bce ce } $items(Eras) break
	set cmd "    ::msgcat::mcset "
	append cmd $localeName " " BCE " \"" [backslashify $bce] \"
	puts $f $cmd
	set cmd "    ::msgcat::mcset "
	append cmd $localeName " " CE " \"" [backslashify $ce] \"
	puts $f $cmd
    }

    # Do the AM and PM markers

    if { [info exists items(AmPmMarkers)] } {
	foreach { am pm } $items(AmPmMarkers) break
	set cmd "    ::msgcat::mcset "
	append cmd $localeName " " AM " \"" [backslashify $am] \"
	puts $f $cmd
	set cmd "    ::msgcat::mcset "
	append cmd $localeName " " PM " \"" [backslashify $pm] \"
	puts $f $cmd
    }

    # Do the date/time patterns. First date...

    if { [info exists format($localeName,DATE_FORMAT)]
	 || [info exists items(DateTimePatterns)] } {

	# Find the shortest date format that includes a 4-digit year.

	if { ![info exists format($localeName,DATE_FORMAT)] } {
	    for { set i 7 } { $i >= 4 } { incr i -1 } {
		if { [regexp yyyy [lindex $items(DateTimePatterns) $i]] } {
		    break
		}
	    }
	    set fmt \
		[backslashify \
		     [percentify [lindex $items(DateTimePatterns) $i]]]
	    set format($localeName,DATE_FORMAT) $fmt
	}

	# Put it to the message catalog

	set cmd "    ::msgcat::mcset "
	append cmd $localeName " DATE_FORMAT \"" \
	    $format($localeName,DATE_FORMAT) "\""
	puts $f $cmd
    }

    # Time

    if { [info exists format($localeName,TIME_FORMAT)]
	 || [info exists items(DateTimePatterns)] } {

	# Find the shortest time pattern that includes the seconds

	if { ![info exists format($localeName,TIME_FORMAT)] } {
	    for { set i 3 } { $i >= 0 } { incr i -1 } {
		if { [regexp H [lindex $items(DateTimePatterns) $i]]
		     && [regexp s [lindex $items(DateTimePatterns) $i]] } {
		    break
		}
	    }
	    if { $i >= 0 } {
		set fmt \
		    [backslashify \
			 [percentify [lindex $items(DateTimePatterns) $i]]]
		regsub { %Z} $fmt {} format($localeName,TIME_FORMAT)
	    }
	}

	# Put it to the message catalog

	if { [info exists format($localeName,TIME_FORMAT)] } {
	    set cmd "    ::msgcat::mcset "
	    append cmd $localeName " TIME_FORMAT \"" \
		$format($localeName,TIME_FORMAT) "\""
	    puts $f $cmd
	}
    }

    # 12-hour time...

    if { [info exists format($localeName,TIME_FORMAT_12)]
	 || [info exists items(DateTimePatterns)] } {

	# Shortest patterm with 12-hour time that includes seconds

	if { ![info exists format($localeName,TIME_FORMAT_12)] } {
	    for { set i 3 } { $i >= 0 } { incr i -1 } {
		if { [regexp h [lindex $items(DateTimePatterns) $i]]
		     && [regexp s [lindex $items(DateTimePatterns) $i]] } {
		    break
		}
	    }
	    if { $i >= 0 } {
		set fmt \
		    [backslashify \
			 [percentify [lindex $items(DateTimePatterns) $i]]]
		regsub { %Z} $fmt {} format($localeName,TIME_FORMAT_12)
	    }
	}

	# Put it to the catalog

	if { [info exists format($localeName,TIME_FORMAT_12)] } {
	    set cmd "    ::msgcat::mcset "
	    append cmd $localeName " TIME_FORMAT_12 \"" \
		$format($localeName,TIME_FORMAT_12) "\""
	    puts $f $cmd
	}
    }

    # Date and time... Prefer 24-hour format to 12-hour format.

    if { ![info exists format($localeName,DATE_TIME_FORMAT)]
	 && [info exists format($localeName,DATE_FORMAT)]
	 && [info exists format($localeName,TIME_FORMAT)]} {
	set format($localeName,DATE_TIME_FORMAT) \
	    $format($localeName,DATE_FORMAT)
	append format($localeName,DATE_TIME_FORMAT) \
	    " " $format($localeName,TIME_FORMAT) " %z"
    }
    if { ![info exists format($localeName,DATE_TIME_FORMAT)]
	 && [info exists format($localeName,DATE_FORMAT)]
	 && [info exists format($localeName,TIME_FORMAT_12)]} {
	set format($localeName,DATE_TIME_FORMAT) \
	    $format($localeName,DATE_FORMAT)
	append format($localeName,DATE_TIME_FORMAT) \
	    " " $format($localeName,TIME_FORMAT_12) " %z"
    }

    # Write date/time format to the file

    if { [info exists format($localeName,DATE_TIME_FORMAT)] } {
	set cmd "    ::msgcat::mcset "
	append cmd $localeName " DATE_TIME_FORMAT \"" \
	    $format($localeName,DATE_TIME_FORMAT) "\""
	puts $f $cmd
    }

    # Write the string sets to the file.

    foreach key {
	LOCALE_NUMERALS LOCALE_DATE_FORMAT LOCALE_TIME_FORMAT
	LOCALE_DATE_TIME_FORMAT LOCALE_ERAS LOCALE_YEAR_FORMAT
    } {
	if { [info exists format($localeName,$key)] } {
	    set cmd "    ::msgcat::mcset "
	    append cmd $localeName " " $key " \"" \
		[backslashify $format($localeName,$key)] "\""
	    puts $f $cmd
	}
    }

    # Footer

    puts $f "\}"
    close $f
}

#----------------------------------------------------------------------
#
# percentify --
#
#	Converts a Java/ICU-style time format to a C/Tcl style one.
#
# Parameters:
#	string -- Format to convert
#
# Results:
#	Returns the converted format.
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc percentify { string } {
    set retval {}
    foreach { unquoted quoted } [split $string '] {
	append retval [string map {
	    EEEE %A MMMM %B yyyy %Y
	    MMM %b EEE %a
	    dd %d hh %I HH %H mm %M MM %m ss %S yy %y
	    a %P d %e h %l H %k M %m z %z
	} $unquoted]
	append retval $quoted
    }
    return $retval
}

#----------------------------------------------------------------------
#
# backslashify --
#
#	Converts a UTF-8 string to a plain ASCII one with escapes.
#
# Parameters:
#	string -- String to convert
#
# Results:
#	Returns the converted string
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc backslashify { string } {

    set retval {}
    foreach char [split $string {}] {
	scan $char %c ccode
	if { $ccode >= 0x0020 && $ccode < 0x007f && $char ne "\""
	     && $char ne "\{" && $char ne "\}" && $char ne "\["
	     && $char ne "\]" && $char ne "\\" && $char ne "\$" } {
	    append retval $char
	} else {
	    append retval \\u [format %04x $ccode]
	}
    }
    return $retval
}

#----------------------------------------------------------------------
#
# MAIN PROGRAM
#
#----------------------------------------------------------------------

# Extract directories from command line

foreach { icudir msgdir } $argv break

# Walk the ICU files and create corresponding Tcl message catalogs

foreach fileName [glob -directory $icudir *.txt] {
    set n [file rootname [file tail $fileName]]
    if { [regexp {^[a-z]{2,3}(_[A-Z]{2,3}(_.*)?)?$} $n] } {
	handleLocaleFile $n $fileName [file join $msgdir [string tolower $n].msg]
    }
}
