##
## Utility functions for Man->HTML converter. Note that these
## functions are specifically intended to work with the format as used
## by Tcl and Tk; they do not cope with arbitrary nroff markup.
##
## Copyright (c) 1995-1997 Roger E. Critchlow Jr
## Copyright (c) 2004-2011 Donal K. Fellows

set ::manual(report-level) 1

proc manerror {msg} {
    global manual
    set name {}
    set subj {}
    set procname [lindex [info level -1] 0]
    if {[info exists manual(name)]} {
	set name $manual(name)
    }
    if {[info exists manual(section)] && [string length $manual(section)]} {
	puts stderr "$name: $manual(section): $procname: $msg"
    } else {
	puts stderr "$name: $procname: $msg"
    }
}

proc manreport {level msg} {
    global manual
    if {$level < $manual(report-level)} {
	uplevel 1 [list manerror $msg]
    }
}

proc fatal {msg} {
    global manual
    uplevel 1 [list manerror $msg]
    exit 1
}

##
## templating
##
proc indexfile {} {
    if {[info exists ::TARGET] && $::TARGET eq "devsite"} {
	return "index.tml"
    } else {
	return "contents.htm"
    }
}

proc copyright {copyright {level {}}} {
    # We don't actually generate a separate copyright page anymore
    #set page "${level}copyright.htm"
    #return "<A HREF=\"$page\">Copyright</A> &#169; [htmlize-text [lrange $copyright 2 end]]"
    # obfuscate any email addresses that may appear in name
    set who [string map {@ (at)} [lrange $copyright 2 end]]
    return "Copyright &copy; [htmlize-text $who]"
}

proc copyout {copyrights {level {}}} {
    set count 0
    set out "<div class=\"copy\">"
    foreach c $copyrights {
	if {$count > 0} {
	    append out <BR>
	}
	append out "[copyright $c $level]\n"
	incr count
    }
    append out "</div>"
    return $out
}

proc CSS {{level ""}} {
    return "<link rel=\"stylesheet\" href=\"${level}$::CSSFILE\" type=\"text/css\" media=\"all\">\n"
}

proc DOCTYPE {} {
    return "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">"
}

proc htmlhead {title header args} {
    set level ""
    if {[lindex $args end] eq "../[indexfile]"} {
	# XXX hack - assume same level for CSS file
	set level "../"
    }
    set out "[DOCTYPE]\n<HTML>\n<HEAD><TITLE>$title</TITLE>\n[CSS $level]</HEAD>\n"
    foreach {uptitle url} $args {
	set header "<a href=\"$url\">$uptitle</a> <small>&gt;</small> $header"
    }
    append out "<BODY><H2>$header</H2>"
    global manual
    if {[info exists manual(subheader)]} {
	set subs {}
	foreach {name subdir} $manual(subheader) {
	    if {$name eq $title} {
		lappend subs $name
	    } else {
		lappend subs "<A HREF=\"${level}$subdir/[indexfile]\">$name</A>"
	    }
	}
	append out "\n<H3>[join $subs { | }]</H3>"
    }
    return $out
}

##
## parsing
##
proc unquote arg {
    return [string map [list \" {}] $arg]
}

proc parse-directive {line codename restname} {
    upvar 1 $codename code $restname rest
    return [regexp {^(\.[.a-zA-Z0-9]*) *(.*)} $line all code rest]
}

proc htmlize-text {text {charmap {}}} {
    # contains some extras for use in nroff->html processing
    # build on the list passed in, if any
    lappend charmap \
	"&ndash;" "&ndash;" \
	{&}	{&amp;} \
	{\\}	"&#92;" \
	{\e}	"&#92;" \
	{\ }	{&nbsp;} \
	{\|}	{&nbsp;} \
	{\0}	{ } \
	\"	{&quot;} \
	{<}	{&lt;} \
	{>}	{&gt;} \
	\u201c "&#8220;" \
	\u201d "&#8221;"

    return [string map $charmap $text]
}

proc process-text {text} {
    global manual
    # preprocess text; note that this is an incomplete map, and will probably
    # need to have things added to it as the manuals expand to use them.
    set charmap [list \
	    {\&}	"\t" \
	    {\%}	{} \
	    "\\\n"	"\n" \
	    {\(+-}	"&#177;" \
	    {\(co}	"&copy;" \
	    {\(em}	"&#8212;" \
	    {\(en}	"&#8211;" \
	    {\(fm}	"&#8242;" \
	    {\(mu}	"&#215;" \
	    {\(mi}	"&#8722;" \
	    {\(->}	"<font size=\"+1\">&#8594;</font>" \
	    {\fP}	{\fR} \
	    {\.}	. \
	    {\(bu}	"&#8226;" \
	    {\*(qo}	"&ocirc;" \
	    ]
    lappend charmap {\-\|\-} --        ; # two hyphens
    lappend charmap {\-} -             ; # a hyphen

    set text [htmlize-text $text $charmap]
    # General quoted entity
    regsub -all {\\N'(\d+)'} $text "\\&#\\1;" text
    while {[string first "\\" $text] >= 0} {
	# C R
	if {[regsub {^([^\\]*)\\fC([^\\]*)\\fR(.*)$} $text \
		{\1<TT>\2</TT>\3} text]} continue
	# B R
	if {[regsub {^([^\\]*)\\fB([^\\]*)\\fR(.*)$} $text \
		{\1<B>\2</B>\3} text]} continue
	# B I
	if {[regsub {^([^\\]*)\\fB([^\\]*)\\fI(.*)$} $text \
		{\1<B>\2</B>\\fI\3} text]} continue
	# I R
	if {[regsub {^([^\\]*)\\fI([^\\]*)\\fR(.*)$} $text \
		{\1<I>\2</I>\3} text]} continue
	# I B
	if {[regsub {^([^\\]*)\\fI([^\\]*)\\fB(.*)$} $text \
		{\1<I>\2</I>\\fB\3} text]} continue
	# B B, I I, R R
	if {
	    [regsub {^([^\\]*)\\fB([^\\]*)\\fB(.*)$} $text \
		{\1\\fB\2\3} ntext]
	    || [regsub {^([^\\]*)\\fI([^\\]*)\\fI(.*)$} $text \
		    {\1\\fI\2\3} ntext]
	    || [regsub {^([^\\]*)\\fR([^\\]*)\\fR(.*)$} $text \
		    {\1\\fR\2\3} ntext]
	} {
	    manerror "impotent font change: $text"
	    set text $ntext
	    continue
	}
	# unrecognized
	manerror "uncaught backslash: $text"
	set text [string map [list "\\" "&#92;"] $text]
    }
    return $text
}

##
## pass 2 text input and matching
##
proc open-text {} {
    global manual
    set manual(text-length) [llength $manual(text)]
    set manual(text-pointer) 0
}

proc more-text {} {
    global manual
    return [expr {$manual(text-pointer) < $manual(text-length)}]
}

proc next-text {} {
    global manual
    if {[more-text]} {
	set text [lindex $manual(text) $manual(text-pointer)]
	incr manual(text-pointer)
	return $text
    }
    manerror "read past end of text"
    error "fatal"
}

proc is-a-directive {line} {
    return [string match .* $line]
}

proc split-directive {line opname restname} {
    upvar 1 $opname op $restname rest
    set op [string range $line 0 2]
    set rest [string trim [string range $line 3 end]]
}

proc next-op-is {op restname} {
    global manual
    upvar 1 $restname rest
    if {[more-text]} {
	set text [lindex $manual(text) $manual(text-pointer)]
	if {[string equal -length 3 $text $op]} {
	    set rest [string range $text 4 end]
	    incr manual(text-pointer)
	    return 1
	}
    }
    return 0
}

proc backup-text {n} {
    global manual
    if {$manual(text-pointer)-$n >= 0} {
	incr manual(text-pointer) -$n
    }
}

proc match-text args {
    global manual
    set nargs [llength $args]
    if {$manual(text-pointer) + $nargs > $manual(text-length)} {
	return 0
    }
    set nback 0
    foreach arg $args {
	if {![more-text]} {
	    backup-text $nback
	    return 0
	}
	set arg [string trim $arg]
	set targ [string trim [lindex $manual(text) $manual(text-pointer)]]
	if {$arg eq $targ} {
	    incr nback
	    incr manual(text-pointer)
	    continue
	}
	if {[regexp {^@(\w+)$} $arg all name]} {
	    upvar 1 $name var
	    set var $targ
	    incr nback
	    incr manual(text-pointer)
	    continue
	}
	if {[regexp -nocase {^(\.[A-Z][A-Z])@(\w+)$} $arg all op name]\
		&& [string equal $op [lindex $targ 0]]} {
	    upvar 1 $name var
	    set var [lrange $targ 1 end]
	    incr nback
	    incr manual(text-pointer)
	    continue
	}
	backup-text $nback
	return 0
    }
    return 1
}

proc expand-next-text {n} {
    global manual
    return [join [lrange $manual(text) $manual(text-pointer) \
	    [expr {$manual(text-pointer)+$n-1}]] \n\n]
}

##
## pass 2 output
##
proc man-puts {text} {
    global manual
    lappend manual(output-$manual(wing-file)-$manual(name)) $text
}

##
## build hypertext links to tables of contents
##
proc long-toc {text} {
    global manual
    set here M[incr manual(section-toc-n)]
    set manual($manual(name)-id-$text) $here
    set there L[incr manual(long-toc-n)]
    lappend manual(section-toc) \
	    "<DD><A HREF=\"$manual(name).htm#$here\" NAME=\"$there\">$text</A>"
    return "<A NAME=\"$here\">$text</A>"
}

proc option-toc {name class switch} {
    global manual
    # Special case handling, oh we hate it but must do it
    if {[string match "*OPTIONS" $manual(section)]} {
	if {$manual(name) ne "ttk_widget" && ($manual(name) ne "ttk_entry" ||
		![string match validate* $name])} {
	    # link the defined option into the long table of contents
	    set link [long-toc "$switch, $name, $class"]
	    regsub -- "$switch, $name, $class" $link "$switch" link
	    return $link
	}
    } elseif {"$manual(name):$manual(section)" ne "options:DESCRIPTION"} {
	error "option-toc in $manual(name) section $manual(section)"
    }

    # link the defined standard option to the long table of contents and make
    # a target for the standard option references from other man pages.

    set first [lindex $switch 0]
    set here M$first
    set there L[incr manual(long-toc-n)]
    set manual(standard-option-$manual(name)-$first) \
	"<A HREF=\"$manual(name).htm#$here\">$switch, $name, $class</A>"
    lappend manual(section-toc) \
	"<DD><A HREF=\"$manual(name).htm#$here\" NAME=\"$there\">$switch, $name, $class</A>"
    return "<A NAME=\"$here\">$switch</A>"
}

proc std-option-toc {name page} {
    global manual
    if {[info exists manual(standard-option-$page-$name)]} {
	lappend manual(section-toc) <DD>$manual(standard-option-$page-$name)
	return $manual(standard-option-$page-$name)
    }
    manerror "missing reference to \"$name\" in $page.n"
    set here M[incr manual(section-toc-n)]
    set there L[incr manual(long-toc-n)]
    set other M$name
    lappend manual(section-toc) "<DD><A HREF=\"$page.htm#$other\">$name</A>"
    return "<A HREF=\"$page.htm#$other\">$name</A>"
}

##
## process the widget option section
## in widget and options man pages
##
proc output-widget-options {rest} {
    global manual
    man-puts <DL>
    lappend manual(section-toc) <DL>
    backup-text 1
    set para {}
    while {[next-op-is .OP rest]} {
	switch -exact -- [llength $rest] {
	    3 {
		lassign $rest switch name class
	    }
	    5 {
		set switch [lrange $rest 0 2]
		set name [lindex $rest 3]
		set class [lindex $rest 4]
	    }
	    default {
		fatal "bad .OP $rest"
	    }
	}
	if {![regexp {^(<.>)([-\w ]+)(</.>)$} $switch \
		all oswitch switch cswitch]} {
	    if {![regexp {^(<.>)([-\w ]+) or ([-\w ]+)(</.>)$} $switch \
		    all oswitch switch1 switch2 cswitch]} {
		error "not Switch: $switch"
	    }
	    set switch "$switch1$cswitch or $oswitch$switch2"
	}
	if {![regexp {^(<.>)([\w]*)(</.>)$} $name all oname name cname]} {
	    error "not Name: $name"
	}
	if {![regexp {^(<.>)([\w]*)(</.>)$} $class all oclass class cclass]} {
	    error "not Class: $class"
	}
	man-puts "$para<DT>Command-Line Name: $oswitch[option-toc $name $class $switch]$cswitch"
	man-puts "<DT>Database Name: $oname$name$cname"
	man-puts "<DT>Database Class: $oclass$class$cclass"
	man-puts <DD>[next-text]
	set para <P>

	if {[next-op-is .RS rest]} {
	    while {[more-text]} {
		set line [next-text]
		if {[is-a-directive $line]} {
		    split-directive $line code rest
		    switch -exact -- $code {
			.RE {
			    break
			}
			.SH - .SS {
			    manerror "unbalanced .RS at section end"
			    backup-text 1
			    break
			}
			default {
			    output-directive $line
			}
		    }
		} else {
		    man-puts $line
		}
	    }
	}
    }
    man-puts </DL>
    lappend manual(section-toc) </DL>
}

##
## process .RS lists
##
proc output-RS-list {} {
    global manual
    if {[next-op-is .IP rest]} {
	output-IP-list .RS .IP $rest
	if {[match-text .RE .sp .RS @rest .IP @rest2]} {
	    man-puts <P>$rest
	    output-IP-list .RS .IP $rest2
	}
	if {[match-text .RE .sp .RS @rest .RE]} {
	    man-puts <P>$rest
	    return
	}
	if {[next-op-is .RE rest]} {
	    return
	}
    }
    man-puts <DL><DD>
    while {[more-text]} {
	set line [next-text]
	if {[is-a-directive $line]} {
	    split-directive $line code rest
	    switch -exact -- $code {
		.RE {
		    break
		}
		.SH - .SS {
		    manerror "unbalanced .RS at section end"
		    backup-text 1
		    break
		}
		default {
		    output-directive $line
		}
	    }
	} else {
	    man-puts $line
	}
    }
    man-puts </DL>
}

##
## process .IP lists which may be plain indents,
## numeric lists, or definition lists
##
proc output-IP-list {context code rest} {
    global manual
    if {![string length $rest]} {
	# blank label, plain indent, no contents entry
	man-puts <DL><DD>
	while {[more-text]} {
	    set line [next-text]
	    if {[is-a-directive $line]} {
		split-directive $line code rest
		if {$code eq ".IP" && $rest eq {}} {
		    man-puts "<P>"
		    continue
		}
		if {$code in {.br .DS .RS}} {
		    output-directive $line
		} else {
		    backup-text 1
		    break
		}
	    } else {
		man-puts $line
	    }
	}
	man-puts </DL>
    } else {
	# labelled list, make contents
	if {$context ne ".SH" && $context ne ".SS"} {
	    man-puts <P>
	}
	set dl "<DL class=\"[string tolower $manual(section)]\">"
	set enddl "</DL>"
	if {$code eq ".IP"} {
	    if {[regexp {^\[[\da-f]+\]|\(?[\da-f]+\)$} $rest]} {
		set dl "<OL class=\"[string tolower $manual(section)]\">"
		set enddl "</OL>"
	    } elseif {"&#8226;" eq $rest} {
		set dl "<UL class=\"[string tolower $manual(section)]\">"
		set enddl "</UL>"
	    }
	}
	man-puts $dl
	lappend manual(section-toc) $dl
	backup-text 1
	set accept_RE 0
	set para {}
	while {[more-text]} {
	    set line [next-text]
	    if {[is-a-directive $line]} {
		split-directive $line code rest
		switch -exact -- $code {
		    .IP {
			if {$accept_RE} {
			    output-IP-list .IP $code $rest
			    continue
			}
			if {$manual(section) eq "ARGUMENTS"} {
			    man-puts "$para<DT>$rest<DD>"
			} elseif {[regexp {^\[([\da-f]+)\]$} $rest -> value]} {
			    man-puts "$para<LI value=\"$value\">"
			} elseif {[regexp {^\(?([\da-f]+)\)$} $rest -> value]} {
			    man-puts "$para<LI value=\"$value\">"
			} elseif {"&#8226;" eq $rest} {
			    man-puts "$para<LI>"
			} else {
			    man-puts "$para<DT>[long-toc $rest]<DD>"
			}
		    }
		    .sp - .br - .DS - .CS {
			output-directive $line
		    }
		    .RS {
			if {[match-text .RS]} {
			    output-directive $line
			    incr accept_RE 1
			} elseif {[match-text .CS]} {
			    output-directive .CS
			    incr accept_RE 1
			} elseif {[match-text .PP]} {
			    output-directive .PP
			    incr accept_RE 1
			} elseif {[match-text .DS]} {
			    output-directive .DS
			    incr accept_RE 1
			} else {
			    output-directive $line
			}
		    }
		    .PP {
			if {[match-text @rest1 .br @rest2 .RS]} {
			    # yet another nroff kludge as above
			    man-puts "$para<DT>[long-toc $rest1]"
			    man-puts "<DT>[long-toc $rest2]<DD>"
			    incr accept_RE 1
			} elseif {[match-text @rest .RE]} {
			    # gad, this is getting ridiculous
			    if {!$accept_RE} {
				man-puts "$enddl<P>$rest$dl"
				backup-text 1
				set para {}
				break
			    }
			    man-puts "<P>$rest"
			    incr accept_RE -1
			} elseif {$accept_RE} {
			    output-directive $line
			} else {
			    backup-text 1
			    break
			}
		    }
		    .RE {
			if {!$accept_RE} {
			    backup-text 1
			    break
			}
			incr accept_RE -1
		    }
		    default {
			backup-text 1
			break
		    }
		}
	    } else {
		man-puts $line
	    }
	    set para <P>
	}
	man-puts "$para$enddl"
	lappend manual(section-toc) $enddl
	if {$accept_RE} {
	    manerror "missing .RE in output-IP-list"
	}
    }
}

##
## handle the NAME section lines
## there's only one line in the NAME section,
## consisting of a comma separated list of names,
## followed by a hyphen and a short description.
##
proc output-name {line} {
    global manual
    # split name line into pieces
    regexp {^([^-]+) - (.*)$} [regsub -all {[ \n\r\t]+} $line " "] -> head tail
    # output line to manual page untouched
    man-puts "$head &mdash; $tail"
    # output line to long table of contents
    lappend manual(section-toc) "<DL><DD>$head &mdash; $tail</DD></DL>"
    # separate out the names for future reference
    foreach name [split $head ,] {
	set name [string trim $name]
	if {[llength $name] > 1} {
	    manerror "name has a space: {$name}\nfrom: $line"
	}
	lappend manual(wing-toc) $name
	lappend manual(name-$name) $manual(wing-file)/$manual(name)
    }
    set manual(tooltip-$manual(wing-file)/$manual(name).htm) $line
}

##
## build a cross-reference link if appropriate
##
proc cross-reference {ref} {
    global manual remap_link_target
    global ensemble_commands exclude_refs_map exclude_when_followed_by_map
    set manname $manual(name)
    set mantail $manual(tail)
    if {[string match "Tcl_*" $ref] || [string match "Tk_*" $ref] || [string match "Ttk_*" $ref] || [string match "Itcl_*" $ref] || [string match "Tdbc_*" $ref]} {
	regexp {^\w+} $ref lref
	##
	## apply a link remapping if available
	##
	if {[info exists remap_link_target($lref)]} {
	    set lref $remap_link_target($lref)
	}
    } elseif {$ref eq "Tcl"} {
	set lref $ref
    } elseif {
	[regexp {^[A-Z0-9 ?!]+$} $ref]
	&& [info exists manual($manname-id-$ref)]
    } {
	return "<A HREF=\"#$manual($manname-id-$ref)\">$ref</A>"
    } else {
	set lref [string tolower $ref]
	##
	## apply a link remapping if available
	##
	if {[info exists remap_link_target($lref)]} {
	    set lref $remap_link_target($lref)
	}
    }
    ##
    ## nothing to reference
    ##
    if {![info exists manual(name-$lref)]} {
	foreach name $ensemble_commands {
	    if {
		[regexp "^$name \[a-z0-9]*\$" $lref] &&
		[info exists manual(name-$name)] &&
		$mantail ne "$name.n" &&
		(![info exists exclude_refs_map($mantail)] ||
		$manual(name-$name) ni $exclude_refs_map($mantail))
	    } {
		return "<A HREF=\"../$manual(name-$name).htm\">$ref</A>"
	    }
	}
	if {$lref in {end}} {
	    # no good place to send this tcl token?
	}
	return $ref
    }
    set manref $manual(name-$lref)
    ##
    ## would be a self reference
    ##
    foreach name $manref {
	if {"$manual(wing-file)/$manname" in $name} {
	    return $ref
	}
    }
    ##
    ## multiple choices for reference
    ##
    if {[llength $manref] > 1} {
	set tcl_i [lsearch -glob $manref *TclCmd*]
	if {$tcl_i >= 0 && $manual(wing-file) eq "TclCmd"
		|| $manual(wing-file) eq "TclLib"} {
	    set tcl_ref [lindex $manref $tcl_i]
	    return "<A HREF=\"../$tcl_ref.htm\">$ref</A>"
	}
	set tk_i [lsearch -glob $manref *TkCmd*]
	if {$tk_i >= 0 && $manual(wing-file) eq "TkCmd"
		|| $manual(wing-file) eq "TkLib"} {
	    set tk_ref [lindex $manref $tk_i]
	    return "<A HREF=\"../$tk_ref.htm\">$ref</A>"
	}
	if {$lref eq "exit" && $mantail eq "tclsh.1" && $tcl_i >= 0} {
	    set tcl_ref [lindex $manref $tcl_i]
	    return "<A HREF=\"../$tcl_ref.htm\">$ref</A>"
	}
	puts stderr "multiple cross reference to $ref in $manref from $manual(wing-file)/$mantail"
	return $ref
    }
    ##
    ## exceptions, sigh, to the rule
    ##
    if {[info exists exclude_when_followed_by_map($mantail)]} {
	upvar 1 text tail
	set following_word [lindex [regexp -inline {\S+} $tail] 0]
	foreach {this that} $exclude_when_followed_by_map($mantail) {
	    # only a ref if $this is not followed by $that
	    if {$lref eq $this && [string match $that* $following_word]} {
		return $ref
	    }
	}
    }
    if {
	[info exists exclude_refs_map($mantail)]
	&& $lref in $exclude_refs_map($mantail)
    } {
	return $ref
    }
    ##
    ## return the cross reference
    ##
    return "<A HREF=\"../$manref.htm\">$ref</A>"
}

##
## reference generation errors
##
proc reference-error {msg text} {
    global manual
    puts stderr "$manual(tail): $msg: {$text}"
    return $text
}

##
## insert as many cross references into this text string as are appropriate
##
proc insert-cross-references {text} {
    global manual
    set result ""

    while 1 {
	##
	## we identify cross references by:
	##     ``quotation''
	##    <B>emboldening</B>
	##    Tcl_ prefix
	##    Tk_ prefix
	##	  [a-zA-Z0-9]+ manual entry
	## and we avoid messing with already anchored text
	##
	##
	## find where each item lives - EXPENSIVE - and accumulate a list
	##
	unset -nocomplain offsets
	foreach {name pattern} {
	    anchor     {<A }	end-anchor {</A>}
	    quote      {``}	end-quote  {''}
	    bold       {<B>}	end-bold   {</B>}
	    c.tcl      {Tcl_}
	    c.tk       {Tk_}
	    c.ttk      {Ttk_}
	    c.tdbc     {Tdbc_}
	    c.itcl     {Itcl_}
	    Tcl1       {Tcl manual entry}
	    Tcl2       {Tcl overview manual entry}
	    url	       {http://}
	} {
	    set o [string first $pattern $text]
	    if {[set offset($name) $o] >= 0} {
		set invert($o) $name
		lappend offsets $o
	    }
	}
	##
	## if nothing, then we're done.
	##
	if {![info exists offsets]} {
	    return [append result $text]
	}
	##
	## sort the offsets
	##
	set offsets [lsort -integer $offsets]
	##
	## see which we want to use
	##
	switch -exact -- $invert([lindex $offsets 0]) {
	    anchor {
		if {$offset(end-anchor) < 0} {
		    return [reference-error {Missing end anchor} $text]
		}
		append result [string range $text 0 $offset(end-anchor)]
		set text [string range $text[set text ""] \
			      [expr {$offset(end-anchor)+1}] end]
		continue
	    }
	    quote {
		if {$offset(end-quote) < 0} {
		    return [reference-error "Missing end quote" $text]
		}
		if {$invert([lindex $offsets 1]) in {tcl tk ttk}} {
		    set offsets [lreplace $offsets 1 1]
		}
		switch -exact -- $invert([lindex $offsets 1]) {
		    end-quote {
			append result [string range $text 0 [expr {$offset(quote)-1}]]
			set body [string range $text [expr {$offset(quote)+2}] \
				      [expr {$offset(end-quote)-1}]]
			set text [string range $text[set text ""] \
				      [expr {$offset(end-quote)+2}] end]
			append result `` [cross-reference $body] ''
			continue
		    }
		    bold - anchor {
			append result [string range $text \
				      0 [expr {$offset(end-quote)+1}]]
			set text [string range $text[set text ""] \
				      [expr {$offset(end-quote)+2}] end]
			continue
		    }
		}
		return [reference-error "Uncaught quote case" $text]
	    }
	    bold {
		if {$offset(end-bold) < 0} {
		    return [append result $text]
		}
		if {[string match "c.*" $invert([lindex $offsets 1])]} {
		    set offsets [lreplace $offsets 1 1]
		}
		switch -exact -- $invert([lindex $offsets 1]) {
		    url - end-bold {
			append result \
			    [string range $text 0 [expr {$offset(bold)-1}]]
			set body [string range $text [expr {$offset(bold)+3}] \
				      [expr {$offset(end-bold)-1}]]
			set text [string range $text[set text ""] \
				      [expr {$offset(end-bold)+4}] end]
			regsub {http://[\w/.]+} $body {<A HREF="&">&</A>} body
			append result <B> [cross-reference $body] </B>
			continue
		    }
		    anchor {
			append result \
			    [string range $text 0 [expr {$offset(end-bold)+3}]]
			set text [string range $text[set text ""] \
				      [expr {$offset(end-bold)+4}] end]
			continue
		    }
		    default {
			return [reference-error "Uncaught bold case" $text]
		    }
		}
	    }
	    c.tk - c.ttk - c.tcl - c.tdbc - c.itcl {
		append result [string range $text 0 \
				   [expr {[lindex $offsets 0]-1}]]
		regexp -indices -start [lindex $offsets 0] {\w+} $text range
		set body [string range $text {*}$range]
		set text [string range $text[set text ""] \
			      [expr {[lindex $range 1]+1}] end]
		append result [cross-reference $body]
		continue
	    }
	    Tcl1 - Tcl2 {
		set off [lindex $offsets 0]
		append result [string range $text 0 [expr {$off-1}]]
		set text [string range $text[set text ""] [expr {$off+3}] end]
		append result [cross-reference Tcl]
		continue
	    }
	    url {
		set off [lindex $offsets 0]
		append result [string range $text 0 [expr {$off-1}]]
		regexp -indices -start $off {http://[\w/.]+} $text range
		set url [string range $text {*}$range]
		append result "<A HREF=\"[string trimright $url .]\">$url</A>"
		set text [string range $text[set text ""] \
			      [expr {[lindex $range 1]+1}] end]
		continue
	    }
	    end-anchor - end-bold - end-quote {
		return [reference-error "Out of place $invert([lindex $offsets 0])" $text]
	    }
	}
    }
}

##
## process formatting directives
##
proc output-directive {line} {
    global manual
    # process format directive
    split-directive $line code rest
    switch -exact -- $code {
	.BS - .BE {
	    # man-puts <HR>
	}
	.SH - .SS {
	    # drain any open lists
	    # announce the subject
	    set manual(section) $rest
	    # start our own stack of stuff
	    set manual($manual(name)-$manual(section)) {}
	    lappend manual(has-$manual(section)) $manual(name)
	    if {$code ne ".SS"} {
		man-puts "<H3>[long-toc $manual(section)]</H3>"
	    } else {
		man-puts "<H4>[long-toc $manual(section)]</H4>"
	    }
	    # some sections can simply free wheel their way through the text
	    # some sections can be processed in their own loops
	    switch -exact -- [string index $code end]:$manual(section) {
		H:NAME {
		    set names {}
		    while {1} {
			set line [next-text]
			if {[is-a-directive $line]} {
			    backup-text 1
			    if {[llength $names]} {
				output-name [join $names { }]
			    }
			    return
			}
			lappend names [string trim $line]
		    }
		}
		H:SYNOPSIS {
		    lappend manual(section-toc) <DL>
		    while {1} {
			if {
			    [next-op-is .nf rest]
			    || [next-op-is .br rest]
			    || [next-op-is .fi rest]
			} {
			    continue
			}
			if {
			    [next-op-is .SH rest]
			    || [next-op-is .SS rest]
			    || [next-op-is .BE rest]
			    || [next-op-is .SO rest]
			} {
			    backup-text 1
			    break
			}
			if {[next-op-is .sp rest]} {
			    #man-puts <P>
			    continue
			}
			set more [next-text]
			if {[is-a-directive $more]} {
			    manerror "in SYNOPSIS found $more"
			    backup-text 1
			    break
			}
			foreach more [split $more \n] {
			    regexp {^(\s*)(.*)} $more -> spaces more
			    set spaces [string map {" " "&nbsp;"} $spaces]
			    if {[string length $spaces]} {
				set spaces <TT>$spaces</TT>
			    }
			    man-puts $spaces$more<BR>
			    if {$manual(wing-file) in {TclLib TkLib}} {
				lappend manual(section-toc) <DD>$more
			    }
			}
		    }
		    lappend manual(section-toc) </DL>
		    return
		}
		{H:SEE ALSO} {
		    while {[more-text]} {
			if {[next-op-is .SH rest] || [next-op-is .SS rest]} {
			    backup-text 1
			    return
			}
			set more [next-text]
			if {[is-a-directive $more]} {
			    manerror "$more"
			    backup-text 1
			    return
			}
			set nmore {}
			foreach cr [split $more ,] {
			    set cr [string trim $cr]
			    if {![regexp {^<B>.*</B>$} $cr]} {
				set cr <B>$cr</B>
			    }
			    if {[regexp {^<B>(.*)\([13n]\)</B>$} $cr all name]} {
				set cr <B>$name</B>
			    }
			    lappend nmore $cr
			}
			man-puts [join $nmore {, }]
		    }
		    return
		}
		H:KEYWORDS {
		    while {[more-text]} {
			if {[next-op-is .SH rest] || [next-op-is .SS rest]} {
			    backup-text 1
			    return
			}
			set more [next-text]
			if {[is-a-directive $more]} {
			    manerror "$more"
			    backup-text 1
			    return
			}
			set keys {}
			foreach key [split $more ,] {
			    set key [string trim $key]
			    lappend manual(keyword-$key) [list $manual(name) \
				    $manual(wing-file)/$manual(name).htm]
			    set initial [string toupper [string index $key 0]]
			    lappend keys "<A href=\"../Keywords/$initial.htm\#$key\">$key</A>"
			}
			man-puts [join $keys {, }]
		    }
		    return
		}
	    }
	    if {[next-op-is .IP rest]} {
		output-IP-list $code .IP $rest
		return
	    }
	    if {[next-op-is .PP rest]} {
		return
	    }
	    return
	}
	.SO {
	    # When there's a sequence of multiple .SO chunks, process into one
	    set optslist {}
	    while 1 {
		if {[match-text @stuff .SE]} {
		    foreach opt [split $stuff \n\t] {
			lappend optslist [list $opt $rest]
		    }
		} else {
		    manerror "unexpected .SO format:\n[expand-next-text 2]"
		}
		if {![next-op-is .SO rest]} {
		    break
		}
	    }
	    output-directive {.SH STANDARD OPTIONS}
	    man-puts <DL>
	    lappend manual(section-toc) <DL>
	    foreach optionpair [lsort -dictionary -index 0 $optslist] {
		lassign $optionpair option targetPage
		man-puts "<DT><B>[std-option-toc $option $targetPage]</B>"
	    }
	    man-puts </DL>
	    lappend manual(section-toc) </DL>
	}
	.OP {
	    output-widget-options $rest
	    return
	}
	.IP {
	    output-IP-list .IP .IP $rest
	    return
	}
	.PP - .sp {
	    man-puts <P>
	}
	.RS {
	    output-RS-list
	    return
	}
	.br {
	    man-puts <BR>
	    return
	}
	.DS {
	    if {[next-op-is .ta rest]} {
		# skip the leading .ta directive if it is there
	    }
	    if {[match-text @stuff .DE]} {
		set td "<td><p class=\"tablecell\">"
		set bodyText [string map [list \n <tr>$td \t $td] \n$stuff]
		man-puts "<dl><dd><table border=\"0\">$bodyText</table></dl>"
		#man-puts <PRE>$stuff</PRE>
	    } elseif {[match-text .fi @ul1 @ul2 .nf @stuff .DE]} {
		man-puts "<PRE>[lindex $ul1 1][lindex $ul2 1]\n$stuff</PRE>"
	    } else {
		manerror "unexpected .DS format:\n[expand-next-text 2]"
	    }
	    return
	}
	.CS {
	    if {[next-op-is .ta rest]} {
		# ???
	    }
	    if {[match-text @stuff .CE]} {
		man-puts <PRE>$stuff</PRE>
	    } else {
		manerror "unexpected .CS format:\n[expand-next-text 2]"
	    }
	    return
	}
	.nf {
	    if {[match-text @more .fi]} {
		foreach more [split $more \n] {
		    man-puts $more<BR>
		}
	    } elseif {[match-text .RS @more .RE .fi]} {
		man-puts <DL><DD>
		foreach more [split $more \n] {
		    man-puts $more<BR>
		}
		man-puts </DL>
	    } elseif {[match-text .RS @more .RS @more2 .RE .RE .fi]} {
		man-puts <DL><DD>
		foreach more [split $more \n] {
		    man-puts $more<BR>
		}
		man-puts <DL><DD>
		foreach more2 [split $more2 \n] {
		    man-puts $more2<BR>
		}
		man-puts </DL></DL>
	    } elseif {[match-text .RS @more .RS @more2 .RE @more3 .RE .fi]} {
		man-puts <DL><DD>
		foreach more [split $more \n] {
		    man-puts $more<BR>
		}
		man-puts <DL><DD>
		foreach more2 [split $more2 \n] {
		    man-puts $more2<BR>
		}
		man-puts </DL><DD>
		foreach more3 [split $more3 \n] {
		    man-puts $more3<BR>
		}
		man-puts </DL>
	    } elseif {[match-text .sp .RS @more .RS @more2 .sp .RE .RE .fi]} {
		man-puts <P><DL><DD>
		foreach more [split $more \n] {
		    man-puts $more<BR>
		}
		man-puts <DL><DD>
		foreach more2 [split $more2 \n] {
		    man-puts $more2<BR>
		}
		man-puts </DL></DL><P>
	    } elseif {[match-text .RS .sp @more .sp .RE .fi]} {
		man-puts <P><DL><DD>
		foreach more [split $more \n] {
		    man-puts $more<BR>
		}
		man-puts </DL><P>
	    } else {
		manerror "ignoring $line"
	    }
	}
	.RE - .DE - .CE {
	    manerror "unexpected $code"
	    return
	}
	.ta - .fi - .na - .ad - .UL - .ie - .el - .ne {
	    manerror "ignoring $line"
	}
	default {
	    manerror "unrecognized format directive: $line"
	}
    }
}

##
## merge copyright listings
##
proc merge-copyrights {l1 l2} {
    set merge {}
    set re1 {^Copyright +(?:\(c\)|\\\(co|&copy;) +(\w.*?)(?:all rights reserved)?(?:\. )*$}
    set re2 {^(\d+) +(?:by +)?(\w.*)$}         ;# date who
    set re3 {^(\d+)-(\d+) +(?:by +)?(\w.*)$}   ;# from to who
    set re4 {^(\d+), *(\d+) +(?:by +)?(\w.*)$} ;# date1 date2 who
    foreach copyright [concat $l1 $l2] {
	if {[regexp -nocase -- $re1 $copyright -> info]} {
	    set info [string trimright $info ". "] ; # remove extra period
	    if {[regexp -- $re2 $info -> date who]} {
		lappend dates($who) $date
		continue
	    } elseif {[regexp -- $re3 $info -> from to who]} {
		for {set date $from} {$date <= $to} {incr date} {
		    lappend dates($who) $date
		}
		continue
	    } elseif {[regexp -- $re3 $info -> date1 date2 who]} {
		lappend dates($who) $date1 $date2
		continue
	    }
	}
	puts "oops: $copyright"
    }
    foreach who [array names dates] {
	set list [lsort -dictionary $dates($who)]
	if {[llength $list] == 1 || [lindex $list 0] eq [lrange $list end end]} {
	    lappend merge "Copyright &copy; [lindex $list 0] $who"
	} else {
	    lappend merge "Copyright &copy; [lindex $list 0]-[lrange $list end end] $who"
	}
    }
    return [lsort -dictionary $merge]
}

##
## foreach of the man pages in the section specified by
## sectionDescriptor, convert manpages into hypertext in
## the directory specified by outputDir.
##
proc make-manpage-section {outputDir sectionDescriptor} {
    global manual overall_title tcltkdesc verbose
    global excluded_pages forced_index_pages process_first_patterns

    set LQ \u201c
    set RQ \u201d

    lassign $sectionDescriptor \
	manual(wing-glob) \
	manual(wing-name) \
	manual(wing-file) \
	manual(wing-description)
    set manual(wing-copyrights) {}
    makedirhier $outputDir/$manual(wing-file)
    set manual(wing-toc-fp) [open $outputDir/$manual(wing-file)/[indexfile] w]
    # whistle
    puts stderr "scanning section $manual(wing-name)"
    # put the entry for this section into the short table of contents
    if {[regexp {^(.+), version (.+)$} $manual(wing-name) -> name version]} {
	puts $manual(short-toc-fp) "<DT><A HREF=\"$manual(wing-file)/[indexfile]\" TITLE=\"version $version\">$name</A></DT><DD>$manual(wing-description)</DD>"
    } else {
	puts $manual(short-toc-fp) "<DT><A HREF=\"$manual(wing-file)/[indexfile]\">$manual(wing-name)</A></DT><DD>$manual(wing-description)</DD>"
    }
    # initialize the wing table of contents
    puts $manual(wing-toc-fp) [htmlhead $manual(wing-name) \
	    $manual(wing-name) $overall_title "../[indexfile]"]
    # initialize the short table of contents for this section
    set manual(wing-toc) {}
    # initialize the man directory for this section
    makedirhier $outputDir/$manual(wing-file)
    # initialize the long table of contents for this section
    set manual(long-toc-n) 1
    # get the manual pages for this section
    set manual(pages) [lsort -dictionary [glob -nocomplain $manual(wing-glob)]]
    # Some pages have to go first so that their links override others
    foreach pat $process_first_patterns {
	set n [lsearch -glob $manual(pages) $pat]
	if {$n >= 0} {
	    set f [lindex $manual(pages) $n]
	    puts stderr "shuffling [file tail $f] to front of processing queue"
	    set manual(pages) \
		[linsert [lreplace $manual(pages) $n $n] 0 $f]
	}
    }
    # set manual(pages) [lrange $manual(pages) 0 5]
    foreach manual_page $manual(pages) {
	set manual(page) [file normalize $manual_page]
	# whistle
	if {$verbose} {
	    puts stderr "scanning page $manual(page)"
	} else {
	    puts -nonewline stderr .
	}
	set manual(tail) [file tail $manual(page)]
	set manual(name) [file root $manual(tail)]
	set manual(section) {}
	if {$manual(name) in $excluded_pages} {
	    # obsolete
	    if {!$verbose} {
		puts stderr ""
	    }
	    manerror "discarding $manual(name)"
	    continue
	}
	set manual(infp) [open $manual(page)]
	set manual(text) {}
	set manual(partial-text) {}
	foreach p {.RS .DS .CS .SO} {
	    set manual($p) 0
	}
	set manual(stack) {}
	set manual(section) {}
	set manual(section-toc) {}
	set manual(section-toc-n) 1
	set manual(copyrights) {}
	lappend manual(all-pages) $manual(wing-file)/$manual(tail)
	lappend manual(all-page-domains) $manual(wing-name)
	manreport 100 $manual(name)
	while {[gets $manual(infp) line] >= 0} {
	    manreport 100 $line
	    if {[regexp {^[`'][/\\]} $line]} {
		if {[regexp {Copyright (?:\(c\)|\\\(co).*$} $line copyright]} {
		    lappend manual(copyrights) $copyright
		}
		# comment
		continue
	    }
	    if {"$line" eq {'}} {
		# comment
		continue
	    }
	    if {![parse-directive $line code rest]} {
		addbuffer $line
		continue
	    }
	    switch -exact -- $code {
		.if - .nr - .ti - .in - .ie - .el -
		.ad - .na - .so - .ne - .AS - .HS - .VE - .VS - . {
		    # ignore
		    continue
		}
	    }
	    switch -exact -- $code {
		.SH - .SS {
		    flushbuffer
		    if {[llength $rest] == 0} {
			gets $manual(infp) rest
		    }
		    lappend manual(text) "$code [unquote $rest]"
		}
		.TH {
		    flushbuffer
		    lappend manual(text) "$code [unquote $rest]"
		}
		.QW {
		    lassign [regexp -all -inline {\"(?:[^""]+)\"|\S+} $rest] \
			inQuote afterwards
		    addbuffer $LQ [unquote $inQuote] $RQ [unquote $afterwards]
		}
		.PQ {
		    lassign [regexp -all -inline {\"(?:[^""]+)\"|\S+} $rest] \
			inQuote punctuation afterwards
		    addbuffer ( $LQ [unquote $inQuote] $RQ \
			    [unquote $punctuation] ) [unquote $afterwards]
		}
		.QR {
		    lassign [regexp -all -inline {\"(?:[^""]+)\"|\S+} $rest] \
			rangeFrom rangeTo afterwards
		    addbuffer $LQ [unquote $rangeFrom] "&ndash;" \
			    [unquote $rangeTo] $RQ [unquote $afterwards]
		}
		.MT {
		    addbuffer $LQ$RQ
		}
		.HS - .UL - .ta {
		    flushbuffer
		    lappend manual(text) "$code [unquote $rest]"
		}
		.BS - .BE - .br - .fi - .sp - .nf {
		    flushbuffer
		    if {$rest ne ""} {
			if {!$verbose} {
			    puts stderr ""
			}
			manerror "unexpected argument: $line"
		    }
		    lappend manual(text) $code
		}
		.AP {
		    flushbuffer
		    lappend manual(text) [concat .IP [process-text \
			    "[lindex $rest 0] \\fB[lindex $rest 1]\\fR ([lindex $rest 2])"]]
		}
		.IP {
		    flushbuffer
		    regexp {^(.*) +\d+$} $rest all rest
		    lappend manual(text) ".IP [process-text \
			    [unquote [string trim $rest]]]"
		}
		.TP {
		    flushbuffer
		    while {[is-a-directive [set next [gets $manual(infp)]]]} {
			if {!$verbose} {
			    puts stderr ""
			}
			manerror "ignoring $next after .TP"
		    }
		    if {"$next" ne {'}} {
			lappend manual(text) ".IP [process-text $next]"
		    }
		}
		.OP {
		    flushbuffer
		    lassign $rest cmdName dbName dbClass
		    lappend manual(text) [concat .OP [process-text \
			    "\\fB$cmdName\\fR \\fB$dbName\\fR \\fB$dbClass\\fR"]]
		}
		.PP - .LP {
		    flushbuffer
		    lappend manual(text) {.PP}
		}
		.RS {
		    flushbuffer
		    incr manual(.RS)
		    lappend manual(text) $code
		}
		.RE {
		    flushbuffer
		    incr manual(.RS) -1
		    lappend manual(text) $code
		}
		.SO {
		    flushbuffer
		    incr manual(.SO)
		    if {[llength $rest] == 0} {
			lappend manual(text) "$code options"
		    } else {
			lappend manual(text) "$code [unquote $rest]"
		    }
		}
		.SE {
		    flushbuffer
		    incr manual(.SO) -1
		    lappend manual(text) $code
		}
		.DS {
		    flushbuffer
		    incr manual(.DS)
		    lappend manual(text) $code
		}
		.DE {
		    flushbuffer
		    incr manual(.DS) -1
		    lappend manual(text) $code
		}
		.CS {
		    flushbuffer
		    incr manual(.CS)
		    lappend manual(text) $code
		}
		.CE {
		    flushbuffer
		    incr manual(.CS) -1
		    lappend manual(text) $code
		}
		.de {
		    while {[gets $manual(infp) line] >= 0} {
			if {[string match "..*" $line]} {
			    break
			}
		    }
		}
		.. {
		    if {!$verbose} {
			puts stderr ""
		    }
		    error "found .. outside of .de"
		}
		default {
		    if {!$verbose} {
			puts stderr ""
		    }
		    flushbuffer
		    manerror "unrecognized format directive: $line"
		}
	    }
	}
	flushbuffer
	close $manual(infp)
	# fixups
	if {$manual(.RS) != 0} {
	    if {!$verbose} {
		puts stderr ""
	    }
	    puts "unbalanced .RS .RE"
	}
	if {$manual(.DS) != 0} {
	    if {!$verbose} {
		puts stderr ""
	    }
	    puts "unbalanced .DS .DE"
	}
	if {$manual(.CS) != 0} {
	    if {!$verbose} {
		puts stderr ""
	    }
	    puts "unbalanced .CS .CE"
	}
	if {$manual(.SO) != 0} {
	    if {!$verbose} {
		puts stderr ""
	    }
	    puts "unbalanced .SO .SE"
	}
	# output conversion
	open-text
	set haserror 0
	if {[next-op-is .HS rest]} {
	    set manual($manual(wing-file)-$manual(name)-title) \
		"[join [lrange $rest 1 end] { }] [lindex $rest 0] manual page"
	} elseif {[next-op-is .TH rest]} {
	    set manual($manual(wing-file)-$manual(name)-title) \
		"[lindex $rest 0] manual page - [join [lrange $rest 4 end] { }]"
	} else {
	    set haserror 1
	    if {!$verbose} {
		puts stderr ""
	    }
	    manerror "no .HS or .TH record found"
	}
	if {!$haserror} {
	    while {[more-text]} {
		set line [next-text]
		if {[is-a-directive $line]} {
		    output-directive $line
		} else {
		    man-puts $line
		}
	    }
	    man-puts [copyout $manual(copyrights) "../"]
	    set manual(wing-copyrights) [merge-copyrights \
		    $manual(wing-copyrights) $manual(copyrights)]
	}
	#
	# make the long table of contents for this page
	#
	set manual(toc-$manual(wing-file)-$manual(name)) \
	    [concat <DL> $manual(section-toc) </DL>]
    }
    if {!$verbose} {
	puts stderr ""
    }

    #
    # make the wing table of contents for the section
    #
    set width 0
    foreach name $manual(wing-toc) {
	if {[string length $name] > $width} {
	    set width [string length $name]
	}
    }
    set perline [expr {118 / $width}]
    set nrows [expr {([llength $manual(wing-toc)]+$perline)/$perline}]
    set n 0
    catch {unset rows}
    foreach name [lsort -dictionary $manual(wing-toc)] {
	set tail $manual(name-$name)
	if {[llength $tail] > 1} {
	    manerror "$name is defined in more than one file: $tail"
	    set tail [lindex $tail [expr {[llength $tail]-1}]]
	}
	set tail [file tail $tail]
	if {[info exists manual(tooltip-$manual(wing-file)/$tail.htm)]} {
	    set tooltip $manual(tooltip-$manual(wing-file)/$tail.htm)
	    set tooltip [string map {[ {\[} ] {\]} $ {\$} \\ \\\\} $tooltip]
	    regsub {^[^-]+-\s*(.)} $tooltip {[string totitle \1]} tooltip
	    append rows([expr {$n%$nrows}]) \
		"<td> <a href=\"$tail.htm\" title=\"[subst $tooltip]\">$name</a> </td>"
	} else {
	    append rows([expr {$n%$nrows}]) \
		"<td> <a href=\"$tail.htm\">$name</a> </td>"
	}
	incr n
    }
    puts $manual(wing-toc-fp) <table>
    foreach row [lsort -integer [array names rows]] {
	puts $manual(wing-toc-fp) <tr>$rows($row)</tr>
    }
    puts $manual(wing-toc-fp) </table>

    #
    # insert wing copyrights
    #
    puts $manual(wing-toc-fp) [copyout $manual(wing-copyrights) "../"]
    puts $manual(wing-toc-fp) "</BODY></HTML>"
    close $manual(wing-toc-fp)
    set manual(merge-copyrights) \
	[merge-copyrights $manual(merge-copyrights) $manual(wing-copyrights)]
}

proc makedirhier {dir} {
    try {
	if {![file isdirectory $dir]} {
	    file mkdir $dir
	}
    } on error msg {
	return -code error "cannot create directory $dir: $msg"
    }
}

proc addbuffer {args} {
    global manual
    if {$manual(partial-text) ne ""} {
	append manual(partial-text) \n
    }
    append manual(partial-text) [join $args ""]
}
proc flushbuffer {} {
    global manual
    if {$manual(partial-text) ne ""} {
	lappend manual(text) [process-text $manual(partial-text)]
	set manual(partial-text) ""
    }
}

return
