# man2help2.tcl --
#
# This file defines procedures that are used during the second pass of
# the man page conversion.  It converts the man format input to rtf
# form suitable for use by the Windows help compiler.
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

# Global variables used by these scripts:
#
# state -	state variable that controls action of text proc.
#
# topics -	array indexed by (package,section,topic) with value
# 		of topic ID.
#
# keywords -	array indexed by keyword string with value of topic ID.
#
# curID - 	current topic ID, starts at 0 and is incremented for
# 		each new topic file.
#
# curPkg -	current package name (e.g. Tcl).
#
# curSect -	current section title (e.g. "Tcl Built-In Commands").
#

# initGlobals --
#
# This procedure is invoked to set the initial values of all of the
# global variables, before processing a man page.
#
# Arguments:
# None.

proc initGlobals {} {
    uplevel \#0 unset state
    global state chars

    set state(paragraphPending) 0
    set state(breakPending) 0
    set state(firstIndent) 0
    set state(leftIndent) 0

    set state(inTP) 0
    set state(paragraph) 0
    set state(textState) 0
    set state(curFont) ""
    set state(startCode) "{\\b "
    set state(startEmphasis) "{\\i "
    set state(endCode) "}"
    set state(endEmphasis) "}"
    set state(noFill) 0
    set state(charCnt) 0
    set state(offset) [getTwips 0.5i]
    set state(leftMargin) [getTwips 0.5i]
    set state(nestingLevel) 0
    set state(intl) 0
    set state(sb) 0
    setTabs 0.5i

# set up international character table

    array set chars {
	o^ F4
    }
}


# beginFont --
#
# Arranges for future text to use a special font, rather than
# the default paragraph font.
#
# Arguments:
# font -		Name of new font to use.

proc beginFont {font} {
    global file state

    textSetup
    if {[string equal $state(curFont) $font]} {
	return
    }
    endFont
    puts -nonewline $file $state(start$font)
    set state(curFont) $font
}


# endFont --
#
# Reverts to the default font for the paragraph type.
#
# Arguments:
# None.

proc endFont {} {
    global state file

    if {[string compare $state(curFont) ""]} {
	puts -nonewline $file $state(end$state(curFont))
	set state(curFont) ""
    }
}


# textSetup --
#
# This procedure is called the first time that text is output for a
# paragraph.  It outputs the header information for the paragraph.
#
# Arguments:
# None.

proc textSetup {} {
    global file state

    if $state(breakPending) {
	puts $file "\\line"
    }
    if $state(paragraphPending) {
	puts $file [format "\\par\n\\pard\\fi%.0f\\li%.0f" \
			$state(firstIndent) $state(leftIndent)]
	foreach tab $state(tabs) {
	    puts $file [format "\\tx%.0f" $tab]
	}
	set state(tabs) {}
	if {$state(sb)} {
	    puts $file "\\sb$state(sb)"
	    set state(sb) 0
	}
    }
    set state(breakPending) 0
    set state(paragraphPending) 0
}


# text --
#
# This procedure adds text to the current state(paragraph).  If this is
# the first text in the state(paragraph) then header information for the
# state(paragraph) is output before the text.
#
# Arguments:
# string -		Text to output in the state(paragraph).

proc text {string} {
    global file state chars

    textSetup
    set string [string map [list \
	    "\\"	"\\\\" \
	    "\{"	"\\\{" \
	    "\}"	"\\\}" \
	    "\t"	{\tab } \
	    ''		"\\rdblquote " \
	    ``		"\\ldblquote " \
	    "\u00b7"	"\\bullet " \
	    ] $string]

    # Check if this is the beginning of an international character string.
    # If so, look up the sequence in the chars table and substitute the
    # appropriate hex value.

    if {$state(intl)} {
	if {[regexp {^'([^']*)'} $string dummy ch]} {
	    if {[info exists chars($ch)]} {
		regsub {^'[^']*'} $string "\\\\'$chars($ch)" string
	    } else {
		puts stderr "Unknown international character '$ch'"
	    }
	}
	set state(intl) 0
    }

    switch $state(textState) {
	REF {
	    if {$state(inTP) == 0} {
		set string [insertRef $string]
	    }
	}
	SEE {
	    global topics curPkg curSect
	    foreach i [split $string] {
		if {![regexp -nocase {^[a-z_0-9]+} [string trim $i] i ]} {
		    continue
		}
		if {![catch {set ref $topics($curPkg,$curSect,$i)} ]} {
		    regsub $i $string [link $i $ref] string
		}
	    }
	}
	KEY {
	    return
	}
    }
    puts -nonewline $file "$string"
}



# insertRef --
#
# This procedure looks for a string in the cross reference table and
# generates a hot-link to the appropriate topic.  Tries to find the
# nearest reference in the manual.
#
# Arguments:
# string -		Text to output in the state(paragraph).

proc insertRef {string} {
    global NAME_file curPkg curSect topics curID
    set path {}
    set string [string trim $string]
    set ref {}
    if {[info exists topics($curPkg,$curSect,$string)]} {
	set ref $topics($curPkg,$curSect,$string)
    } else {
	set sites [array names topics "$curPkg,*,$string"]
	set count [llength $sites]
	if {$count > 0} {
	    set ref $topics([lindex $sites 0])
	} else {
	    set sites [array names topics "*,*,$string"]
	    set count [llength $sites]
	    if {$count > 0} {
		set ref $topics([lindex $sites 0])
	    }
	}
    }

    if {($ref != "") && ($ref != $curID)} {
	set string [link $string $ref]
    }
    return $string
}



# macro --
#
# This procedure is invoked to process macro invocations that start
# with "." (instead of ').
#
# Arguments:
# name -		The name of the macro (without the ".").
# args -		Any additional arguments to the macro.

proc macro {name args} {
    global state file
    switch $name {
	AP {
	    if {[llength $args] != 3 && [llength $args] != 2} {
		puts stderr "Bad .AP macro: .$name [join $args " "]"
	    }
	    newPara 3.75i -3.75i
	    setTabs {1.25i 2.5i 3.75i}
	    font B
	    text [lindex $args 0]
	    tab
	    font I
	    text [lindex $args 1]
	    tab
	    font R
	    if {[llength $args] == 3} {
		text "([lindex $args 2])"
	    }
	    tab
	}
	AS {
	    # next page and previous page
	}
	br {
	    lineBreak
	}
	BS {}
	BE {}
	CE {
	    puts -nonewline $::file "\\f0\\fs20 "
	    set state(noFill) 0
	    set state(breakPending) 0
	    newPara ""
	    set state(leftIndent) [expr {$state(leftIndent) - $state(offset)}]
	    set state(sb) 80
	}
	CS {
	    # code section
	    set state(noFill) 1
	    newPara ""
	    set state(leftIndent) [expr {$state(leftIndent) + $state(offset)}]
	    set state(sb) 80
	    puts -nonewline $::file "\\f1\\fs18 "
	}
	DE {
	    set state(noFill) 0
	    decrNestingLevel
	    newPara 0i
	}
	DS {
	    set state(noFill) 1
	    incrNestingLevel
	    newPara 0i
	}
	fi {
	    set state(noFill) 0
	}
	IP {
	    IPmacro $args
	}
	LP {
	    newPara 0i
	    set state(sb) 80
	}
	ne {
	}
	nf {
	    set state(noFill) 1
	}
	OP {
	    if {[llength $args] != 3} {
		puts stderr "Bad .OP macro: .$name [join $args " "]"
	    }
	    set state(nestingLevel) 0
	    newPara 0i
	    set state(sb) 120
	    setTabs 4c
	    text "Command-Line Name:"
	    tab
	    font B
	    set x [lindex $args 0]
	    regsub -all {\\-} $x - x
	    text $x
	    lineBreak
	    font R
	    text "Database Name:"
	    tab
	    font B
	    text [lindex $args 1]
	    lineBreak
	    font R
	    text "Database Class:"
	    tab
	    font B
	    text [lindex $args 2]
	    font R
	    set state(inTP) 0
	    newPara 0.5i
	    set state(sb) 80
	}
	PP {
	    newPara 0i
	    set state(sb) 120
	}
	RE {
	    decrNestingLevel
	}
	RS {
	    incrNestingLevel
	}
	SE {
	    font R
	    set state(noFill) 0
	    set state(nestingLevel) 0
	    newPara 0i
	    text "See the "
	    font B
	    set temp $state(textState)
	    set state(textState) REF
	    text options
	    set state(textState) $temp
	    font R
	    text " manual entry for detailed descriptions of the above options."
	}
	SH {
	    SHmacro $args
	}
	SS {
	    SHmacro $args subsection
	}
	SO {
	    SHmacro "STANDARD OPTIONS"
	    set state(nestingLevel) 0
	    newPara 0i
	    setTabs {4c 8c 12c}
	    font B
	    set state(noFill) 1
	}
	so {
	    if {$args ne "man.macros"} {
		puts stderr "Unknown macro: .$name [join $args " "]"
	    }
	}
	sp {					;# needs work
	    if {$args eq ""} {
		set count 1
	    } else {
		set count [lindex $args 0]
	    }
	    while {$count > 0} {
		lineBreak
		incr count -1
	    }
	}
	ta {
	    setTabs $args
	}
	TH {
	    THmacro $args
	}
	TP {
	    TPmacro $args
	}
	UL {					;# underline
	    puts -nonewline $file "{\\ul "
	    text [lindex $args 0]
	    puts -nonewline $file "}"
	    if {[llength $args] == 2} {
		text [lindex $args 1]
	    }
	}
	VE {}
	VS {}
	QW {
	    formattedText "``[lindex $args 0]''[lindex $args 1] "
	}
	MT {
	    text "``'' "
	}
	PQ {
	    formattedText \
		"(``[lindex $args 0]''[lindex $args 1])[lindex $args 2] "
	}
	QR {
	    formattedText "``[lindex $args 0]"
	    dash
	    formattedText "[lindex $args 1]''[lindex $args 2] "
	}
	default {
	    puts stderr "Unknown macro: .$name [join $args " "]"
	}
    }
}


# link --
#
# This procedure returns the string for  a hot link to a different
# context location.
#
# Arguments:
# label -		String to display in hot-spot.
# id -			Context string to jump to.

proc link {label id} {
    return "{\\uldb $label}{\\v $id}"
}


# font --
#
# This procedure is invoked to handle font changes in the text
# being output.
#
# Arguments:
# type -		Type of font: R, I, B, or S.

proc font {type} {
    global state
    switch $type {
	P -
	R {
	    endFont
	    if {$state(textState) eq "REF"} {
		set state(textState) INSERT
	    }
	}
	C -
	B {
	    beginFont Code
	    if {$state(textState) eq "INSERT"} {
		set state(textState) REF
	    }
	}
	I {
	    beginFont Emphasis
	}
	S {
	}
	default {
	    puts stderr "Unknown font: $type"
	}
    }
}



# formattedText --
#
# Insert a text string that may also have \fB-style font changes
# and a few other backslash sequences in it.
#
# Arguments:
# text -		Text to insert.

proc formattedText {text} {
    global chars

    while {$text ne ""} {
	set index [string first \\ $text]
	if {$index < 0} {
	    text $text
	    return
	}
	text [string range $text 0 [expr {$index-1}]]
	set c [string index $text [expr {$index+1}]]
	switch -- $c {
	    f {
		font [string index $text [expr {$index+2}]]
		set text [string range $text [expr {$index+3}] end]
	    }
	    e {
		text "\\"
		set text [string range $text [expr {$index+2}] end]
	    }
	    - {
		dash
		set text [string range $text [expr {$index+2}] end]
	    }
	    & - | {
		set text [string range $text [expr {$index+2}] end]
	    }
	    ( {
		char [string range $text $index [expr {$index+3}]]
		set text [string range $text [expr {$index+4}] end]
	    }
	    default {
		puts stderr "Unknown sequence: \\$c"
		set text [string range $text [expr {$index+2}] end]
	    }
	}
    }
}


# dash --
#
# This procedure is invoked to handle dash characters ("\-" in
# troff).  It outputs a special dash character.
#
# Arguments:
# None.

proc dash {} {
    global state
    if {[string equal $state(textState) "NAME"]} {
    	set state(textState) 0
    }
    text "-"
}


# tab --
#
# This procedure is invoked to handle tabs in the troff input.
# Right now it does nothing.
#
# Arguments:
# None.

proc tab {} {
    global file

    textSetup
    puts -nonewline $file "\\tab "
}


# setTabs --
#
# This procedure handles the ".ta" macro, which sets tab stops.
#
# Arguments:
# tabList -	List of tab stops in *roff format

proc setTabs {tabList} {
    global file state

    set state(tabs) {}
    foreach arg $tabList {
	if {[string match +* $arg]} {
	    set relativeTo [lindex $state(tabs) end]
	    set arg [string range $arg 1 end]
	} else {
	    # Local left margin
	    set relativeTo [expr {$state(leftMargin) \
		    + ($state(offset) * $state(nestingLevel))}]
	}
	if {[regexp {^\\w'([^']*)'u$} $arg -> submatch]} {
	    # Magic factor!
	    set distance [expr {[string length $submatch] * 86.4}]
	} else {
	    set distance [getTwips $arg]
	}
	lappend state(tabs) [expr {round($distance + $relativeTo)}]
    }
}


# lineBreak --
#
# Generates a line break in the HTML output.
#
# Arguments:
# None.

proc lineBreak {} {
    global state
    textSetup
    set state(breakPending) 1
}



# newline --
#
# This procedure is invoked to handle newlines in the troff input.
# It outputs either a space character or a newline character, depending
# on fill mode.
#
# Arguments:
# None.

proc newline {} {
    global state

    if {$state(inTP)} {
    	set state(inTP) 0
	lineBreak
    } elseif {$state(noFill)} {
	lineBreak
    } else {
	text " "
    }
}


# pageBreak --
#
# This procedure is invoked to generate a page break.
#
# Arguments:
# None.

proc pageBreak {} {
    global file curVer
    if {[string equal $curVer ""]} {
	puts $file {\page}
    } else {
	puts $file {\par}
	puts $file {\pard\sb400\qc}
	puts $file "Last change: $curVer\\page"
    }
}


# char --
#
# This procedure is called to handle a special character.
#
# Arguments:
# name -		Special character named in troff \x or \(xx construct.

proc char {name} {
    global file state

    switch -exact $name {
        {\o} {
	    set state(intl) 1
	}
	{\ } {
	    textSetup
	    puts -nonewline $file " "
	}
	{\0} {
	    textSetup
	    puts -nonewline $file " \\emspace "
	}
	{\\} - {\e} {
	    textSetup
	    puts -nonewline $file "\\\\"
	}
	{\(+-} {
	    textSetup
	    puts -nonewline $file "\\'b1 "
	}
	{\%} - {\|} {
	}
	{\(->} {
	    textSetup
	    puts -nonewline $file "->"
	}
	{\(bu} {
	    textSetup
	    puts -nonewline $file "\\bullet "
	}
	{\(co} {
	    textSetup
	    puts -nonewline $file "\\'a9 "
	}
	{\(mi} {
	    textSetup
	    puts -nonewline $file "-"
	}
	{\(mu} {
	    textSetup
	    puts -nonewline $file "\\'d7 "
	}
	{\(em} - {\(en} {
	    textSetup
	    puts -nonewline $file "-"
	}
	{\(fm} {
	    textSetup
	    puts -nonewline $file "\\'27 "
	}
	default {
	    puts stderr "Unknown character: $name"
	}
    }
}


# macro2 --
#
# This procedure handles macros that are invoked with a leading "'"
# character instead of space.  Right now it just generates an
# error diagnostic.
#
# Arguments:
# name -		The name of the macro (without the ".").
# args -		Any additional arguments to the macro.

proc macro2 {name args} {
    puts stderr "Unknown macro: '$name [join $args " "]"
}



# SHmacro --
#
# Subsection head; handles the .SH and .SS macros.
#
# Arguments:
# name -		Section name.

proc SHmacro {argList {style section}} {
    global file state

    set args [join $argList " "]
    if {[llength $argList] < 1} {
	puts stderr "Bad .SH macro: .SH $args"
    }

    # control what the text proc does with text

    switch $args {
	NAME {set state(textState) NAME}
	DESCRIPTION {set state(textState) INSERT}
	INTRODUCTION {set state(textState) INSERT}
	"WIDGET-SPECIFIC OPTIONS" {set state(textState) INSERT}
	"SEE ALSO" {set state(textState) SEE}
	KEYWORDS {set state(textState) KEY; return}
    }

    if {$state(breakPending) != -1} {
	set state(breakPending) 1
    } else {
	set state(breakPending) 0
    }
    set state(noFill) 0
    if {[string compare "subsection" $style] == 0} {
	nextPara .25i
    } else {
	nextPara 0i
    }
    font B
    text $args
    font R
    nextPara .5i
}

# IPmacro --
#
# This procedure is invoked to handle ".IP" macros, which may take any
# of the following forms:
#
# .IP [1]		Translate to a "1Step" state(paragraph).
# .IP [x] (x > 1)	Translate to a "Step" state(paragraph).
# .IP			Translate to a "Bullet" state(paragraph).
# .IP text count	Translate to a FirstBody state(paragraph) with special
#			indent and tab stop based on "count", and tab after
#			"text".
#
# Arguments:
# argList -		List of arguments to the .IP macro.
#
# HTML limitations: 'count' in '.IP text count' is ignored.

proc IPmacro {argList} {
    global file state

    set length [llength $argList]
    foreach {text indent} $argList break
    if {$length > 2} {
	puts stderr "Bad .IP macro: .IP [join $argList " "]"
    }

    if {$length == 0} {
	set text {\(bu}
	set indent 5
    } elseif {$length == 1} {
	set indent 5
    }
    if {$text == {\(bu}} {
	set text "\u00b7"
    }

    set tab [expr $indent * 0.1]i
    newPara $tab -$tab
    set state(sb) 80
    setTabs $tab
    formattedText $text
    tab
}

# TPmacro --
#
# This procedure is invoked to handle ".TP" macros, which may take any
# of the following forms:
#
# .TP x		Translate to an state(indent)ed state(paragraph) with the
# 			specified state(indent) (in 100 twip units).
# .TP		Translate to an state(indent)ed state(paragraph) with
# 			default state(indent).
#
# Arguments:
# argList -		List of arguments to the .IP macro.
#
# HTML limitations: 'x' in '.TP x' is ignored.

proc TPmacro {argList} {
    global state
    set length [llength $argList]
    if {$length == 0} {
	set val 0.5i
    } else {
	set val [expr {([lindex $argList 0] * 100.0)/1440}]i
    }
    newPara $val -$val
    setTabs $val
    set state(inTP) 1
    set state(sb) 120
}


# THmacro --
#
# This procedure handles the .TH macro.  It generates the non-scrolling
# header section for a given man page, and enters information into the
# table of contents.  The .TH macro has the following form:
#
# .TH name section date footer header
#
# Arguments:
# argList -		List of arguments to the .TH macro.

proc THmacro {argList} {
    global file curPkg curSect curID id_keywords state curVer bitmap

    if {[llength $argList] != 5} {
	set args [join $argList " "]
	puts stderr "Bad .TH macro: .TH $args"
    }
    incr curID
    set name	[lindex $argList 0]		;# Tcl_UpVar
    set page	[lindex $argList 1]		;# 3
    set curVer	[lindex $argList 2]		;# 7.4
    set curPkg	[lindex $argList 3]		;# Tcl
    set curSect	[lindex $argList 4]		;# {Tcl Library Procedures}

    regsub -all {\\ } $curSect { } curSect	;# Clean up for [incr\ Tcl]

    puts $file "#{\\footnote $curID}"		;# Context string
    puts $file "\${\\footnote $name}"		;# Topic title
    set browse "${curSect}${name}"
    regsub -all {[ _-]} $browse {} browse
    puts $file "+{\\footnote $browse}"		;# Browse sequence

    # Suppress duplicates
    foreach i $id_keywords($curID) {
	set keys($i) 1
    }
    foreach i [array names keys] {
	set i [string trim $i]
	if {[string length $i] > 0} {
	    puts $file "K{\\footnote $i}"	;# Keyword strings
	}
    }
    unset keys
    puts $file "\\pard\\tx3000\\sb100\\sa100\\fs24\\keepn"
    font B
    text $name
    tab
    text $curSect
    font R
    if {[info exists bitmap]} {
	# a right justified bitmap
	puts $file "\\\{bmrt $bitmap\\\}"
    }
    puts $file "\\fs20"
    set state(breakPending) -1
}

# nextPara --
#
# Set the indents for a new paragraph, and start a paragraph break
#
# Arguments:
# leftIndent -		The new left margin for body lines.
# firstIndent -		The offset from the left margin for the first line.

proc nextPara {leftIndent {firstIndent 0i}} {
    global state
    set state(leftIndent) [getTwips $leftIndent]
    set state(firstIndent) [getTwips $firstIndent]
    set state(paragraphPending) 1
}


# newPara --
#
# This procedure sets the left and hanging state(indent)s for a line.
# State(Indent)s are specified in units of inches or centimeters, and are
# relative to the current nesting level and left margin.
#
# Arguments:
# leftState(Indent) -		The new left margin for lines after the first.
# firstState(Indent) -		The new left margin for the first line of a state(paragraph).

proc newPara {leftIndent {firstIndent 0i}} {
    global state file
    if $state(paragraph) {
	puts -nonewline $file "\\line\n"
    }
    if {$leftIndent ne ""} {
	set state(leftIndent) [expr {$state(leftMargin) \
		+ ($state(offset) * $state(nestingLevel)) \
		+ [getTwips $leftIndent]}]
    }
    set state(firstIndent) [getTwips $firstIndent]
    set state(paragraphPending) 1
}


# getTwips --
#
# This procedure converts a distance in inches or centimeters into
# twips (1/1440 of an inch).
#
# Arguments:
# arg -			A number followed by "i" or "c"

proc getTwips {arg} {
    if {[scan $arg "%f%s" distance units] != 2} {
	puts stderr "bad distance \"$arg\""
	return 0
    }
    if {[string length $units] > 1} {
	puts stderr "additional characters after unit \"$arg\""
	set units [string index $units 0]
    }
    switch -- $units {
	c	{
	    set distance [expr {$distance * 567}]
	}
	i	{
	    set distance [expr {$distance * 1440}]
	}
	default {
	    puts stderr "bad units in distance \"$arg\""
	    return 0
	}
    }
    return $distance
}

# incrNestingLevel --
#
# This procedure does the work of the .RS macro, which increments
# the number of state(indent)ations that affect things like .PP.
#
# Arguments:
# None.

proc incrNestingLevel {} {
    global state

    incr state(nestingLevel)
    set oldp $state(paragraph)
    set state(paragraph) 0
    newPara 0i
    set state(paragraph) $oldp
}

# decrNestingLevel --
#
# This procedure does the work of the .RE macro, which decrements
# the number of indentations that affect things like .PP.
#
# Arguments:
# None.

proc decrNestingLevel {} {
    global state

    if {$state(nestingLevel) == 0} {
	puts stderr "Nesting level decremented below 0"
    } else {
	incr state(nestingLevel) -1
    }
}
