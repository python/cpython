#----------------------------------------------------------------------
#
# tclZIC.tcl --
#
#	Take the time zone data source files from Arthur Olson's
#	repository at elsie.nci.nih.gov, and prepare time zone
#	information files for Tcl.
#
# Usage:
#	tclsh tclZIC.tcl inputDir outputDir
#
# Parameters:
#	inputDir - Directory (e.g., tzdata2003e) where Olson's source
#		   files are to be found.
#	outputDir - Directory (e.g., ../library/tzdata) where
#		    the time zone information files are to be placed.
#
# Results:
#	May produce error messages on the standard error.  An exit
#	code of zero denotes success; any other exit code is failure.
#
# This program parses the timezone data in a means analogous to the
# 'zic' command, and produces Tcl time zone information files suitable
# for loading into the 'clock' namespace.
#
#----------------------------------------------------------------------
#
# Copyright (c) 2004 by Kevin B. Kenny.	 All rights reserved.
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#----------------------------------------------------------------------

# Define the names of the Olson files that we need to load.
# We avoid the solar time files and the leap seconds.

set olsonFiles {
    africa antarctica asia australasia
    backward etcetera europe northamerica
    pacificnew southamerica systemv
}

# Define the year at which the DST information will stop.

set maxyear 2100

# Determine how big a wide integer is.

set MAXWIDE [expr {wide(1)}]
while 1 {
    set next [expr {wide($MAXWIDE + $MAXWIDE + 1)}]
    if {$next < 0} {
	break
    }
    set MAXWIDE $next
}
set MINWIDE [expr {-$MAXWIDE-1}]

#----------------------------------------------------------------------
#
# loadFiles --
#
#	Loads the time zone files for each continent into memory
#
# Parameters:
#	dir - Directory where the time zone source files are found
#
# Results:
#	None.
#
# Side effects:
#	Calls 'loadZIC' for each continent's data file in turn.
#	Reports progress on stdout.
#
#----------------------------------------------------------------------

proc loadFiles {dir} {
    variable olsonFiles
    foreach file $olsonFiles {
	puts "loading: [file join $dir $file]"
	loadZIC [file join $dir $file]
    }
    return
}

#----------------------------------------------------------------------
#
# checkForwardRuleRefs --
#
#	Checks to make sure that all references to Daylight Saving
#	Time rules designate defined rules.
#
# Parameters:
#	None.
#
# Results:
#	None.
#
# Side effects:
#	Produces an error message and increases the error count if
#	any undefined rules are present.
#
#----------------------------------------------------------------------

proc checkForwardRuleRefs {} {
    variable forwardRuleRefs
    variable rules

    foreach {rule where} [array get forwardRuleRefs] {
	if {![info exists rules($rule)]} {
	    foreach {fileName lno} $where {
		puts stderr "$fileName:$lno:can't locate rule \"$rule\""
		incr errorCount
	    }
	}
    }
}

#----------------------------------------------------------------------
#
# loadZIC --
#
#	Load one continent's data into memory.
#
# Parameters:
#	fileName -- Name of the time zone source file.
#
# Results:
#	None.
#
# Side effects:
#	The global variable, 'errorCount' counts the number of errors.
#	The global array, 'links', contains a distillation of the
#	'Link' directives in the file. The keys are 'links to' and
#	the values are 'links from'.  The 'parseRule' and 'parseZone'
#	procedures are called to handle 'Rule' and 'Zone' directives.
#
#----------------------------------------------------------------------

proc loadZIC {fileName} {
    variable errorCount
    variable links

    # Suck the text into memory.

    set f [open $fileName r]
    set data [read $f]
    close $f

    # Break the input into lines, and count line numbers.

    set lno 0
    foreach line [split $data \n] {
	incr lno

	# Break a line of input into words.

	regsub {\s*(\#.*)?$} $line {} line
	if {$line eq ""} {
	    continue
	}
	set words {}
	if {[regexp {^\s} $line]} {
	    # Detect continuations of a zone and flag the list appropriately
	    lappend words ""
	}
	lappend words {*}[regexp -all -inline {\S+} $line]

	# Switch on the directive

	switch -exact -- [lindex $words 0] {
	    Rule {
		parseRule $fileName $lno $words
	    }
	    Link {
		set links([lindex $words 2]) [lindex $words 1]
	    }
	    Zone {
		set lastZone [lindex $words 1]
		set until [parseZone $fileName $lno \
			$lastZone [lrange $words 2 end] "minimum"]
	    }
	    {} {
		set i 0
		foreach word $words {
		    if {[lindex $words $i] ne ""} {
			break
		    }
		    incr i
		}
		set words [lrange $words $i end]
		set until [parseZone $fileName $lno $lastZone $words $until]
	    }
	    default {
		incr errorCount
		puts stderr "$fileName:$lno:unknown line type \"[lindex $words 0]\""
	    }
	}
    }

    return
}

#----------------------------------------------------------------------
#
# parseRule --
#
#	Parses a Rule directive in an Olson file.
#
# Parameters:
#	fileName -- Name of the file being parsed.
#	lno - Line number within the file
#	words - The line itself, broken into words.
#
# Results:
#	None.
#
# Side effects:
#	The rule is analyzed and added to the 'rules' array.
#	Errors are reported and counted.
#
#----------------------------------------------------------------------

proc parseRule {fileName lno words} {
    variable rules
    variable errorCount

    # Break out the columns

    lassign $words  Rule name from to type in on at save letter

    # Handle the 'only' keyword

    if {$to eq "only"} {
	set to $from
    }

    # Process the start year

    if {![string is integer $from]} {
	if {![string equal -length [string length $from] $from "minimum"]} {
	    puts stderr "$fileName:$lno:FROM field \"$from\" not an integer."
	    incr errorCount
	    return
	} else {
	    set from "minimum"
	}
    }

    # Process the end year

    if {![string is integer $to]} {
	if {![string equal -length [string length $to] $to "maximum"]} {
	    puts stderr "$fileName:$lno:TO field \"$to\" not an integer."
	    incr errorCount
	    return
	} else {
	    set to "maximum"
	}
    }

    # Process the type of year in which the rule applies

    if {$type ne "-"} {
	puts stderr "$fileName:$lno:year types are not yet supported."
	incr errorCount
	return
    }

    # Process the month in which the rule starts

    if {[catch {lookupMonth $in} in]} {
	puts stderr "$fileName:$lno:$in"
	incr errorCount
	return
    }

    # Process the day of the month on which the rule starts

    if {[catch {parseON $on} on]} {
	puts stderr "$fileName:$lno:$on"
	incr errorCount
	return
    }

    # Process the time of day on which the rule starts

    if {[catch {parseTOD $at} at]} {
	puts stderr "$fileName:$lno:$at"
	incr errorCount
	return
    }

    # Process the DST adder

    if {[catch {parseOffsetTime $save} save]} {
	puts stderr "$fileName:$lno:$save"
	incr errorCount
	return
    }

    # Process the letter to use for summer time

    if {$letter eq "-"} {
	set letter ""
    }

    # Accumulate all the data.

    lappend rules($name) $from $to $type $in $on $at $save $letter
    return

}

#----------------------------------------------------------------------
#
# parseON --
#
#	Parse a specification for a day of the month
#
# Parameters:
#	on - the ON field from a line in an Olson file.
#
# Results:
#	Returns a partial Tcl command.	When the year and number of the
#	month are appended, the command will return the Julian Day Number
#	of the desired date.
#
# Side effects:
#	None.
#
# The specification can be:
#	- a simple number, which designates a constant date.
#	- The name of a weekday, followed by >= or <=, followed by a number.
#	    This designates the nearest occurrence of the given weekday on
#	    or before (on or after) the given day of the month.
#	- The word 'last' followed by a weekday name with no intervening
#	  space.  This designates the last occurrence of the given weekday
#	  in the month.
#
#----------------------------------------------------------------------

proc parseON {on} {
    if {![regexp -expanded {
	^(?:
	  # first possibility - simple number - field 1
	  ([[:digit:]]+)
	|
	  # second possibility - weekday >= (or <=) number
	  # field 2 - weekday
	  ([[:alpha:]]+)
	  # field 3 - direction
	  ([<>]=)
	  # field 4 - number
	  ([[:digit:]]+)
	|
	  # third possibility - lastWeekday - field 5
	  last([[:alpha:]]+)
	)$
    } $on -> dom1 wday2 dir2 num2 wday3]} {
	error "can't parse ON field \"$on\""
    }
    if {$dom1 ne ""} {
	return [list onDayOfMonth $dom1]
    } elseif {$wday2 ne ""} {
	set wday2 [lookupDayOfWeek $wday2]
	return [list onWeekdayInMonth $wday2 $dir2 $num2]
    } elseif {$wday3 ne ""} {
	set wday3 [lookupDayOfWeek $wday3]
	return [list onLastWeekdayInMonth $wday3]
    } else {
	error "in parseOn \"$on\": can't happen"
    }
}

#----------------------------------------------------------------------
#
# onDayOfMonth --
#
#	Find a given day of a given month
#
# Parameters:
#	day - Day of the month
#	year - Gregorian year
#	month - Number of the month (1-12)
#
# Results:
#	Returns the Julian Day Number of the desired day.
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc onDayOfMonth {day year month} {
    scan $day %d day
    scan $year %d year
    scan $month %d month
    set date [::tcl::clock::GetJulianDayFromEraYearMonthDay \
	    [dict create era CE year $year month $month dayOfMonth $day] \
		 2361222]
    return [dict get $date julianDay]
}

#----------------------------------------------------------------------
#
# onWeekdayInMonth --
#
#	Find the weekday falling on or after (on or before) a
#	given day of the month
#
# Parameters:
#	dayOfWeek - Day of the week (Monday=1, Sunday=7)
#	relation - <= for the weekday on or before a given date, >= for
#		   the weekday on or after the given date.
#	dayOfMonth - Day of the month
#	year - Gregorian year
#	month - Number of the month (1-12)
#
# Results:
#	Returns the Juloan Day Number of the desired day.
#
# Side effects:
#	None.
#
# onWeekdayInMonth is used to compute Daylight Saving Time rules
# like 'Sun>=1' (for the nearest Sunday on or after the first of the month)
# or "Mon<=4' (for the Monday on or before the fourth of the month).
#
#----------------------------------------------------------------------

proc onWeekdayInMonth {dayOfWeek relation dayOfMonth year month} {
    set date [::tcl::clock::GetJulianDayFromEraYearMonthDay [dict create \
	    era CE year $year month $month dayOfMonth $dayOfMonth] 2361222]
    switch -exact -- $relation {
	<= {
	    return [::tcl::clock::WeekdayOnOrBefore $dayOfWeek \
		    [dict get $date julianDay]]
	}
	>= {
	    return [::tcl::clock::WeekdayOnOrBefore $dayOfWeek \
		    [expr {[dict get $date julianDay] + 6}]]
	}
    }
}

#----------------------------------------------------------------------
#
# onLastWeekdayInMonth --
#
#	Find the last instance of a given weekday in a month.
#
# Parameters:
#	dayOfWeek - Weekday to find (Monday=1, Sunday=7)
#	year - Gregorian year
#	month - Month (1-12)
#
# Results:
#	Returns the Julian Day number of the last instance of
#	the given weekday in the given month
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc onLastWeekdayInMonth {dayOfWeek year month} {
    incr month
    # Find day 0 of the following month, which is the last day of
    # the current month.  Yes, it works to ask for day 0 of month 13!
    set date [::tcl::clock::GetJulianDayFromEraYearMonthDay [dict create \
	    era CE year $year month $month dayOfMonth 0] 2361222]
    return [::tcl::clock::WeekdayOnOrBefore $dayOfWeek \
	    [dict get $date julianDay]]
}

#----------------------------------------------------------------------
#
# parseTOD --
#
#	Parses the specification of a time of day in an Olson file.
#
# Parameters:
#	tod - Time of day, which may be followed by 'w', 's', 'u', 'g'
#	      or 'z'.  'w' (or no letter) designates a wall clock time,
#	      's' designates Standard Time in the given zone, and
#	      'u', 'g', and 'z' all designate UTC.
#
# Results:
#	Returns a two element list containing a count of seconds from
#	midnight and the letter that followed the time.
#
# Side effects:
#	Reports and counts an error if the time cannot be parsed.
#
#----------------------------------------------------------------------

proc parseTOD {tod} {
    if {![regexp -expanded {
	^
	([[:digit:]]{1,2})		# field 1 - hour
	(?:
	    :([[:digit:]]{2})		# field 2 - minute
	    (?:
		:([[:digit:]]{2})	# field 3 - second
	    )?
	)?
	(?:
	    ([wsugz])			# field 4 - type indicator
	)?
    } $tod -> hour minute second ind]} {
	puts stderr "$fileName:$lno:can't parse time field \"$tod\""
	incr errorCount
    }
    scan $hour %d hour
    if {$minute ne ""} {
	scan $minute %d minute
    } else {
	set minute 0
    }
    if {$second ne ""} {
	scan $second %d second
    } else {
	set second 0
    }
    if {$ind eq ""} {
	set ind w
    }
    return [list [expr {($hour * 60 + $minute) * 60 + $second}] $ind]
}

#----------------------------------------------------------------------
#
# parseOffsetTime --
#
#	Parses the specification of an offset time in an Olson file.
#
# Parameters:
#	offset - Offset time as [+-]hh:mm:ss
#
# Results:
#	Returns the offset time as a count of seconds.
#
# Side effects:
#	Reports and counts an error if the time cannot be parsed.
#
#----------------------------------------------------------------------

proc parseOffsetTime {offset} {
    if {![regexp -expanded {
	^
	([-+])?				# field 1 - signum
	([[:digit:]]{1,2})		# field 2 - hour
	(?:
	    :([[:digit:]]{2})		# field 3 - minute
	    (?:
		:([[:digit:]]{2})	# field 4 - second
	    )?
	)?
    } $offset -> signum hour minute second]} {
	puts stderr "$fileName:$lno:can't parse offset time \"$offset\""
	incr errorCount
    }
    append signum 1
    scan $hour %d hour
    if {$minute ne ""} {
	scan $minute %d minute
    } else {
	set minute 0
    }
    if {$second ne ""} {
	scan $second %d second
    } else {
	set second 0
    }
    return [expr {(($hour * 60 + $minute) * 60 + $second) * $signum}]

}

#----------------------------------------------------------------------
#
# lookupMonth -
#	Looks up a month by name
#
# Parameters:
#	month - Name of a month.
#
# Results:
#	Returns the number of the month.
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc lookupMonth {month} {
    set indx [lsearch -regexp {
	{} January February March April May June
	July August September October November December
    } ${month}.*]
    if {$indx < 1} {
	error "unknown month name \"$month\""
    }
    return $indx
}

#----------------------------------------------------------------------
#
# lookupDayOfWeek --
#
#	Looks up the name of a weekday.
#
# Parameters:
#	wday - Weekday name (or a unique prefix).
#
# Results:
#	Returns the weekday number (Monday=1, Sunday=7)
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc lookupDayOfWeek {wday} {
    set indx [lsearch -regexp {
	{} Monday Tuesday Wednesday Thursday Friday Saturday Sunday
    } ${wday}.*]
    if {$indx < 1} {
	error "unknown weekday name \"$wday\""
    }
    return $indx
}

#----------------------------------------------------------------------
#
# parseZone --
#
#	Parses a Zone directive in an Olson file
#
# Parameters:
#	fileName -- Name of the file being parsed.
#	lno -- Line number within the file.
#	zone -- Name of the time zone
#	words -- Remaining words on the line.
#	start -- 'Until' time from the previous line if this is a
#		 continuation line, or 'minimum' if this is the first line.
#
# Results:
#	Returns the 'until' field of the current line
#
# Side effects:
#	Stores a row in the 'zones' array describing the current zone.
#	The row consists of a start time (year month day tod), a Standard
#	Time offset from Greenwich, a Daylight Saving Time offset from
#	Standard Time, and a format for printing the time zone.
#
#	The start time is the result of an earlier call to 'parseUntil'
#	or else the keyword 'minimum'.	The GMT offset is the
#	result of a call to 'parseOffsetTime'.	The Daylight Saving
#	Time offset is represented as a partial Tcl command. To the
#	command will be appended a start time (seconds from epoch)
#	the current offset of Standard Time from Greenwich, the current
#	offset of Daylight Saving Time from Greenwich, the default
#	offset from this line, the name pattern from this line,
#	the 'until' field from this line, and a variable name where points
#	are to be stored.  This command is implemented by the 'applyNoRule',
#	'applyDSTOffset' and 'applyRules' procedures.
#
#----------------------------------------------------------------------

proc parseZone {fileName lno zone words start} {
    variable zones
    variable rules
    variable errorCount
    variable forwardRuleRefs

    lassign $words gmtoff save format
    if {[catch {parseOffsetTime $gmtoff} gmtoff]} {
	puts stderr "$fileName:$lno:$gmtoff"
	incr errorCount
	return
    }
    if {[info exists rules($save)]} {
	set save [list applyRules $save]
    } elseif {$save eq "-"} {
	set save [list applyNoRule]
    } elseif {[catch {parseOffsetTime $save} save2]} {
	lappend forwardRuleRefs($save) $fileName $lno
	set save [list applyRules $save]
    } else {
	set save [list applyDSTOffset $save2]
    }
    lappend zones($zone) $start $gmtoff $save $format
    if {[llength $words] >= 4} {
	return [parseUntil [lrange $words 3 end]]
    } else {
	return {}
    }
}

#----------------------------------------------------------------------
#
# parseUntil --
#
#	Parses the 'UNTIL' part of a 'Zone' directive.
#
# Parameters:
#	words - The 'UNTIL' part of the directie.
#
# Results:
#	Returns a list comprising the year, the month, the day, and
#	the time of day. Time of day is represented as the result of
#	'parseTOD'.
#
#----------------------------------------------------------------------

proc parseUntil {words} {
    variable firstYear

    if {[llength $words] >= 1} {
	set year [lindex $words 0]
	if {![string is integer $year]} {
	    error "can't parse UNTIL field \"$words\""
	}
	if {![info exists firstYear] || $year < $firstYear} {
	    set firstYear $year
	}
    } else {
	set year "maximum"
    }
    if {[llength $words] >= 2} {
	set month [lookupMonth [lindex $words 1]]
    } else {
	set month 1
    }
    if {[llength $words] >= 3} {
	set day [parseON [lindex $words 2]]
    } else {
	set day {onDayOfMonth 1}
    }
    if {[llength $words] >= 4} {
	set tod [parseTOD [lindex $words 3]]
    } else {
	set tod {0 w}
    }
    return [list $year $month $day $tod]
}

#----------------------------------------------------------------------
#
# applyNoRule --
#
#	Generates time zone data for a zone without Daylight Saving
#	Time.
#
# Parameters:
#	year - Year in which the rule applies
#	startSecs - Time at which the rule starts.
#	stdGMTOffset - Offset from Greenwich prior to the start of the
#		       rule
#	DSTOffset - Offset of Daylight from Standard prior to the
#		    start of the rule.
#	nextGMTOffset - Offset from Greenwich when the rule is in effect.
#	namePattern - Name of the timezone.
#	until - Time at which the rule expires.
#	pointsVar - Name of a variable in callers scope that receives
#		    transition times
#
# Results:
#	Returns a two element list comprising 'nextGMTOffset' and
#	0 - the zero indicates that Daylight Saving Time is not
#	in effect.
#
# Side effects:
#	Appends a row to the 'points' variable comprising the start time,
#	the offset from GMT, a zero (indicating that DST is not in effect),
#	and the name of the time zone.
#
#----------------------------------------------------------------------

proc applyNoRule {year startSecs stdGMTOffset DSTOffset nextGMTOffset
		  namePattern until pointsVar} {
    upvar 1 $pointsVar points
    lappend points $startSecs $nextGMTOffset 0 \
	    [convertNamePattern $namePattern -]
    return [list $nextGMTOffset 0]
}

#----------------------------------------------------------------------
#
# applyDSTOffset --
#
#	Generates time zone data for a zone with permanent Daylight
#	Saving Time.
#
# Parameters:
#	nextDSTOffset - Offset of Daylight from Standard while the
#			rule is in effect.
#	year - Year in which the rule applies
#	startSecs - Time at which the rule starts.
#	stdGMTOffset - Offset from Greenwich prior to the start of the
#		       rule
#	DSTOffset - Offset of Daylight from Standard prior to the
#		    start of the rule.
#	nextGMTOffset - Offset from Greenwich when the rule is in effect.
#	namePattern - Name of the timezone.
#	until - Time at which the rule expires.
#	pointsVar - Name of a variable in callers scope that receives
#		    transition times
#
# Results:
#	Returns a two element list comprising 'nextGMTOffset' and
#	'nextDSTOffset'.
#
# Side effects:
#	Appends a row to the 'points' variable comprising the start time,
#	the offset from GMT, a one (indicating that DST is in effect),
#	and the name of the time zone.
#
#----------------------------------------------------------------------

proc applyDSTOffset {nextDSTOffset year startSecs
		     stdGMTOffset DSTOffset nextGMTOffset
		     namePattern until pointsVar} {
    upvar 1 $pointsVar points
    lappend points \
	    $startSecs \
	    [expr {$nextGMTOffset + $nextDSTOffset}] \
	    1 \
	    [convertNamePattern $namePattern S]
    return [list $nextGMTOffset $nextDSTOffset]
}

#----------------------------------------------------------------------
#
# applyRules --
#
#	Applies a rule set to a time zone for a given range of time
#
# Parameters:
#	ruleSet - Name of the rule set to apply
#	year - Starting year for the rules
#	startSecs - Time at which the rules begin to apply
#	stdGMTOffset - Offset from Greenwich prior to the start of the
#		       rules.
#	DSTOffset - Offset of Daylight from Standard prior to the
#		    start of the rules.
#	nextGMTOffset - Offset from Greenwich when the rules are in effect.
#	namePattern - Name pattern for the time zone.
#	until - Time at which the rule set expires.
#	pointsVar - Name of a variable in callers scope that receives
#		    transition times
#
# Results:
#	Returns a two element list comprising the offset from GMT
#	to Standard and the offset from Standard to Daylight (if DST
#	is in effect) at the end of the period in which the rules apply
#
# Side effects:
#	Appends one or more rows to the 'points' variable, each of which
#	comprises a transition time, the offset from GMT that is
#	in effect after the transition, a flag for whether DST is in
#	effect, and the name of the time zone.
#
#----------------------------------------------------------------------

proc applyRules {ruleSet year startSecs stdGMTOffset DSTOffset nextGMTOffset
		 namePattern until pointsVar} {
    variable done
    variable rules
    variable maxyear

    upvar 1 $pointsVar points

    # Extract the rules that apply to the current year, and the number
    # of rules (now or in future) that will end at a specific year.
    # Ignore rules entirely in the past.

    lassign [divideRules $ruleSet $year] currentRules nSunsetRules

    # If the first transition is later than $startSecs, and $stdGMTOffset is
    # different from $nextGMTOffset, we will need an initial record like:
    #	 lappend points $startSecs $stdGMTOffset 0 \
    #			[convertNamePattern $namePattern -]

    set didTransitionIn false

    # Determine the letter to use in Standard Time

    set prevLetter ""
    foreach {
	fromYear toYear yearType monthIn daySpecOn timeAt save letter
    } $rules($ruleSet) {
	if {$save == 0} {
	    set prevLetter $letter
	    break
	}
    }

    # Walk through each year in turn. This loop will break when
    #	 (a) the 'until' time is passed
    # or (b) the 'until' time is empty and all remaining rules extend to
    #	     the end of time

    set stdGMTOffset $nextGMTOffset

    # convert "until" to seconds from epoch in current time zone

    if {$until ne ""} {
	lassign $until untilYear untilMonth untilDaySpec untilTimeOfDay
	lappend untilDaySpec $untilYear $untilMonth
	set untilJCD [eval $untilDaySpec]
	set untilBaseSecs [expr {
		wide(86400) * wide($untilJCD) - 210866803200 }]
	set untilSecs [convertTimeOfDay $untilBaseSecs $stdGMTOffset \
		$DSTOffset {*}$untilTimeOfDay]
    }

    set origStartSecs $startSecs

    while {($until ne "" && $startSecs < $untilSecs)
	    || ($until eq "" && ($nSunsetRules > 0 || $year < $maxyear))} {
	set remainingRules $currentRules
	while {[llength $remainingRules] > 0} {

	    # Find the rule with the earliest start time from among the
	    # active rules that haven't yet been processed.

	    lassign [findEarliestRule $remainingRules $year \
		    $stdGMTOffset $DSTOffset] earliestSecs earliestIndex

	    set endi [expr {$earliestIndex + 7}]
	    set rule [lrange $remainingRules $earliestIndex $endi]
	    lassign $rule fromYear toYear \
		    yearType monthIn daySpecOn timeAt save letter

	    # Test if the rule is in effect.

	    if {
		$earliestSecs > $startSecs &&
		($until eq "" || $earliestSecs < $untilSecs)
	    } {
		# Test if the initial transition has been done.
		# If not, do it now.

		if {!$didTransitionIn && $earliestSecs > $origStartSecs} {
		    set nm [convertNamePattern $namePattern $prevLetter]
		    lappend points \
			    $origStartSecs \
			    [expr {$stdGMTOffset + $DSTOffset}] \
			    0 \
			    $nm
		    set didTransitionIn true
		}

		# Add a row to 'points' for the rule

		set nm [convertNamePattern $namePattern $letter]
		lappend points \
			$earliestSecs \
			[expr {$stdGMTOffset + $save}] \
			[expr {$save != 0}] \
			$nm
	    }

	    # Remove the rule just applied from the queue

	    set remainingRules [lreplace \
		    $remainingRules[set remainingRules {}] \
		    $earliestIndex $endi]

	    # Update current DST offset and time zone letter

	    set DSTOffset $save
	    set prevLetter $letter

	    # Reconvert the 'until' time in the current zone.

	    if {$until ne ""} {
		set untilSecs [convertTimeOfDay $untilBaseSecs \
			$stdGMTOffset $DSTOffset {*}$untilTimeOfDay]
	    }
	}

	# Advance to the next year

	incr year
	set date [::tcl::clock::GetJulianDayFromEraYearMonthDay \
		[dict create era CE year $year month 1 dayOfMonth 1] 2361222]
	set startSecs [expr {
	    [dict get $date julianDay] * wide(86400) - 210866803200
		- $stdGMTOffset - $DSTOffset
	}]

	# Get rules in effect in the new year.

	lassign [divideRules $ruleSet $year] currentRules nSunsetRules
    }

    return [list $stdGMTOffset $DSTOffset]
}

#----------------------------------------------------------------------
#
# divideRules --
#	Determine what Daylight Saving Time rules may be in effect in
#	a given year.
#
# Parameters:
#	ruleSet - Set of rules from 'parseRule'
#	year - Year to test
#
# Results:
#	Returns a two element list comprising the subset of 'ruleSet'
#	that is in effect in the given year, and the count of rules
#	that expire in the future (as opposed to those that expire in
#	the past or not at all). If this count is zero, the rules do
#	not change in future years.
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc divideRules {ruleSet year} {
    variable rules

    set currentRules {}
    set nSunsetRules 0

    foreach {
	fromYear toYear yearType monthIn daySpecOn timeAt save letter
    } $rules($ruleSet) {
	if {$toYear ne "maximum" && $year > $toYear} {
	    # ignore - rule is in the past
	} else {
	    if {$fromYear eq "minimum" || $fromYear <= $year} {
		lappend currentRules $fromYear $toYear $yearType $monthIn \
			$daySpecOn $timeAt $save $letter
	    }
	    if {$toYear ne "maximum"} {
		incr nSunsetRules
	    }
	}
    }

    return [list $currentRules $nSunsetRules]

}

#----------------------------------------------------------------------
#
# findEarliestRule --
#
#	Find the rule in a rule set that has the earliest start time.
#
# Parameters:
#	remainingRules -- Rules to search
#	year - Year being processed.
#	stdGMTOffset - Current offset of standard time from GMT
#	DSTOffset - Current offset of daylight time from standard,
#		    if daylight time is in effect.
#
# Results:
#	Returns the index in remainingRules of the next rule to
#	go into effect.
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc findEarliestRule {remainingRules year stdGMTOffset DSTOffset} {
    set earliest $::MAXWIDE
    set i 0
    foreach {
	fromYear toYear yearType monthIn daySpecOn timeAt save letter
    } $remainingRules {
	lappend daySpecOn $year $monthIn
	set dayIn [eval $daySpecOn]
	set secs [expr {wide(86400) * wide($dayIn) - 210866803200}]
	set secs [convertTimeOfDay $secs \
		$stdGMTOffset $DSTOffset {*}$timeAt]
	if {$secs < $earliest} {
	    set earliest $secs
	    set earliestIdx $i
	}
	incr i 8
    }

    return [list $earliest $earliestIdx]
}

#----------------------------------------------------------------------
#
# convertNamePattern --
#
#	Converts a name pattern to the name of the time zone.
#
# Parameters:
#	pattern - Patthern to convert
#	flag - Daylight Time flag. An empty string denotes Standard
#	       Time, anything else is Daylight Time.
#
# Results;
#	Returns the name of the time zone.
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc convertNamePattern {pattern flag} {
    if {[regexp {(.*)/(.*)} $pattern -> standard daylight]} {
	if {$flag ne ""} {
	    set pattern $daylight
	} else {
	    set pattern $standard
	}
    }
    return [string map [list %s $flag] $pattern]
}

#----------------------------------------------------------------------
#
# convertTimeOfDay --
#
#	Takes a time of day specifier from 'parseAt' and converts
#	to seconds from the Epoch,
#
# Parameters:
#	seconds -- Time at which the GMT day starts, in seconds
#		   from the Posix epoch
#	stdGMTOffset - Offset of Standard Time from Greenwich
#	DSTOffset - Offset of Daylight Time from standard.
#	timeOfDay - Time of day to convert, in seconds from midnight
#	flag - Flag indicating whether the time is Greenwich, Standard
#	       or wall-clock. (g, s, or w)
#
# Results:
#	Returns the time of day in seconds from the Posix epoch.
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc convertTimeOfDay {seconds stdGMTOffset DSTOffset timeOfDay flag} {
    incr seconds $timeOfDay
    switch -exact $flag {
	g - u - z {
	}
	w {
	    incr seconds [expr {-$stdGMTOffset}]
	    incr seconds [expr {-$DSTOffset}]
	}
	s {
	    incr seconds [expr {-$stdGMTOffset}]
	}
    }
    return $seconds
}

#----------------------------------------------------------------------
#
# processTimeZone --
#
#	Generate the information about all time transitions in a
#	time zone.
#
# Parameters:
#	zoneName - Name of the time zone
#	zoneData - List containing the rows describing the time zone,
#		   obtained from 'parseZone.
#
# Results:
#	Returns a list of rows.	 Each row consists of a time in
#	seconds from the Posix epoch, an offset from GMT to local
#	that begins at that time, a flag indicating whether DST
#	is in effect after that time, and the printable name of the
#	timezone that goes into effect at that time.
#
# Side effects:
#	None.
#
#----------------------------------------------------------------------

proc processTimeZone {zoneName zoneData} {
    set points {}
    set i 0
    foreach {startTime nextGMTOffset dstRule namePattern} $zoneData {
	incr i 4
	set until [lindex $zoneData $i]
	if {![info exists stdGMTOffset]} {
	    set stdGMTOffset $nextGMTOffset
	}
	if {![info exists DSTOffset]} {
	    set DSTOffset 0
	}
	if {$startTime eq "minimum"} {
	    set secs $::MINWIDE
	    set year 0
	} else {
	    lassign $startTime year month dayRule timeOfDay
	    lappend dayRule $year $month
	    set startDay [eval $dayRule]
	    set secs [expr {wide(86400) * wide($startDay) -210866803200}]
	    set secs [convertTimeOfDay $secs \
		    $stdGMTOffset $DSTOffset {*}$timeOfDay]
	}
	lappend dstRule \
		$year $secs $stdGMTOffset $DSTOffset $nextGMTOffset \
		$namePattern $until points
	lassign [eval $dstRule] stdGMTOffset DSTOffset
    }
    return $points
}

#----------------------------------------------------------------------
#
# writeZones --
#
#	Writes all the time zone information files.
#
# Parameters:
#	outDir - Directory in which to store the files.
#
# Results:
#	None.
#
# Side effects:
#	Writes the time zone information files; traces what's happening
#	on the standard output.
#
#----------------------------------------------------------------------

proc writeZones {outDir} {
    variable zones

    # Walk the zones

    foreach zoneName [lsort -dictionary [array names zones]] {
	puts "calculating: $zoneName"
	set fileName [eval [list file join $outDir] [file split $zoneName]]

	# Create directories as needed

	set dirName [file dirname $fileName]
	if {![file exists $dirName]} {
	    puts "creating directory: $dirName"
	    file mkdir $dirName
	}

	# Generate data for a zone

	set data ""
	foreach {
	    time offset dst name
	} [processTimeZone $zoneName $zones($zoneName)] {
	    append data "\n    " [list [list $time $offset $dst $name]]
	}
	append data \n

	# Write the data to the information file

	set f [open $fileName w]
	fconfigure $f -translation lf
	puts $f "\# created by $::argv0 - do not edit"
	puts $f ""
	puts $f [list set TZData(:$zoneName) $data]
	close $f
    }

    return
}

#----------------------------------------------------------------------
#
# writeLinks --
#
#	Write files describing time zone synonyms (the Link directives
#	from the Olson files)
#
# Parameters:
#	outDir - Name of the directory where the output files go.
#
# Results:
#	None.
#
# Side effects:
#	Creates a file for each link.

proc writeLinks {outDir} {
    variable links

    # Walk the links

    foreach zoneName [lsort -dictionary [array names links]] {
	puts "creating link: $zoneName"
	set fileName [eval [list file join $outDir] [file split $zoneName]]

	# Create directories as needed

	set dirName [file dirname $fileName]
	if {![file exists $dirName]} {
	    puts "creating directory: $dirName"
	    file mkdir $dirName
	}

	# Create code for the synonym

	set linkTo $links($zoneName)
	set sourceCmd "\n    [list LoadTimeZoneFile $linkTo]\n"
	set ifCmd [list if "!\[info exists TZData($linkTo)\]" $sourceCmd]
	set setCmd "set TZData(:$zoneName) \$TZData(:$linkTo)"

	# Write the file

	set f [open $fileName w]
	fconfigure $f -translation lf
	puts $f "\# created by $::argv0 - do not edit"
	puts $f $ifCmd
	puts $f $setCmd
	close $f
    }

    return
}

#----------------------------------------------------------------------
#
# MAIN PROGRAM
#
#----------------------------------------------------------------------

puts "Compiling time zones -- [clock format [clock seconds] \
                                   -format {%x %X} -locale system]"

# Determine directories

lassign $argv inDir outDir

puts "Olson files in $inDir"
puts "Tcl files to be placed in $outDir"

# Initialize count of errors

set errorCount 0

# Parse the Olson files

loadFiles $inDir
if {$errorCount > 0} {
    exit 1
}

# Check that all riles appearing in Zone and Link lines actually exist

checkForwardRuleRefs
if {$errorCount > 0} {
    exit 1
}

# Write the time zone information files

writeZones $outDir
writeLinks $outDir
if {$errorCount > 0} {
    exit 1
}

# All done!

exit
