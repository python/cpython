#!/bin/sh
# The next line is executed by /bin/sh, but not tcl \
exec tclsh8.4 "$0" ${1+"$@"}

package require Tcl 8.5

# Convert Ousterhout format man pages into highly crosslinked hypertext.
#
# Along the way detect many unmatched font changes and other odd things.
#
# Note well, this program is a hack rather than a piece of software
# engineering.  In that sense it's probably a good example of things
# that a scripting language, like Tcl, can do well.  It is offered as
# an example of how someone might convert a specific set of man pages
# into hypertext, not as a general solution to the problem.  If you
# try to use this, you'll be very much on your own.
#
# Copyright (c) 1995-1997 Roger E. Critchlow Jr

set Version "0.40"

set ::CSSFILE "docs.css"

set ::logo {
    <a href="http://sourceforge.net/projects/tcl">
    <img src="http://sflogo.sourceforge.net/sflogo.php?group_id=10894&amp;type=14" 
         width="150" height="40" 
         alt="Get Tcl at SourceForge.net. Fast, secure and Free Open Source software downloads" />
    </a>
}

proc parse_command_line {} {
    global argv Version

    # These variables determine where the man pages come from and where
    # the converted pages go to.
    global tcltkdir tkdir tcldir webdir build_tcl build_tk
    global build_tdbc build_tdbcodbc build_tdbcsqlite3 build_tdbcmysql
    global build_tdbcpostgres
    global tdbcdir tdbcodbcdir tdbcsqlite3dir tdbcmysqldir tdbcpostgresdir

    # Set defaults based on original code.
    set tcltkdir ../..
    set tkdir {}
    set tcldir {}
    set tdbcdir {}
    set tdbcodbcdir {}
    set tdbcsqlite3dir {}
    set tdbcmysqldir {}
    set tdbcpostgresdir {}
    set webdir ../html
    set build_tcl 0
    set build_tk 0
    set build_tdbc 0
    set build_tdbcodbc 0
    set build_tdbcsqlite3 0
    set build_tdbcmysql 0
    set build_tdbcpostgres 0

    # Default search version is a glob pattern
    set useversion {{,[8-9].[0-9]{,[.ab][0-9]{,[0-9]}}}}

    # Handle arguments a la GNU:
    #   --version
    #   --useversion=<version>
    #   --help
    #   --srcdir=/path
    #   --htmldir=/path

    foreach option $argv {
	switch -glob -- $option {
	    --version {
		puts "tcltk-man-html $Version"
		exit 0
	    }

	    --help {
		puts "usage: tcltk-man-html \[OPTION\] ...\n"
		puts "  --help              print this help, then exit"
		puts "  --version           print version number, then exit"
		puts "  --srcdir=DIR        find tcl and tk source below DIR"
		puts "  --htmldir=DIR       put generated HTML in DIR"
		puts "  --tcl               build tcl help"
		puts "  --tk                build tk help"
		puts "  --useversion        version of tcl/tk to search for"
		exit 0
	    }

	    --srcdir=* {
		# length of "--srcdir=" is 9.
		set tcltkdir [string range $option 9 end]
	    }

	    --htmldir=* {
		# length of "--htmldir=" is 10
		set webdir [string range $option 10 end]
	    }

	    --useversion=* {
		# length of "--useversion=" is 13
		set useversion [string range $option 13 end]
	    }

	    --tcl {
		set build_tcl 1
	    }

	    --tk {
		set build_tk 1
	    }

	    --tdbc {
		set build_tdbc 1
	    }

	    --tdbcodbc {
		set build_tdbcodbc 1
	    }

	    --tdbcsqlite3 {
		set build_tdbcsqlite3 1
	    }

	    --tdbcmysql {
		set build_tdbcmysql 1
	    }

	    --tdbcpostgres {
		set build_tdbcpostgres 1
	    }

	    default {
		puts stderr "tcltk-man-html: unrecognized option -- `$option'"
		exit 1
	    }
	}
    }

    if {$build_tcl} {
	# Find Tcl.
	set tcldir [lindex [lsort [glob -nocomplain -tails -type d \
		-directory $tcltkdir tcl$useversion]] end]
	if {$tcldir eq ""} {
	    puts stderr "tcltk-man-html: couldn't find Tcl below $tcltkdir"
	    exit 1
	}
	puts "using Tcl source directory $tcldir"
    }

    if {$build_tk} {
	# Find Tk.
	set tkdir [lindex [lsort [glob -nocomplain -tails -type d \
				      -directory $tcltkdir tk$useversion]] end]
	if {$tkdir eq ""} {
	    puts stderr "tcltk-man-html: couldn't find Tk below $tcltkdir"
	    exit 1
	}
	puts "using Tk source directory $tkdir"
    }

    if {$build_tdbc} {
	# Find Tdbc.
	set tdbcdir [lindex [lsort [glob -nocomplain -tails -type d \
					-directory $tcltkdir \
					tdbc$useversion]] end]
	if {$tdbcdir eq ""} {
	    puts stderr "tcltk-man-html: couldn't find Tdbc below $tcltkdir"
	    exit 1
	}
	puts "using Tdbc source directory $tcldir"
    }

    if {$build_tdbcodbc} {
	# Find Tdbcodbc.
	set tdbcodbcdir [lindex [lsort [glob -nocomplain -tails -type d \
					-directory $tcltkdir \
					tdbcodbc$useversion]] end]
	if {$tdbcodbcdir eq ""} {
	    puts stderr "tcltk-man-html: couldn't find Tdbcodbc below $tcltkdir"
	    exit 1
	}
	puts "using Tdbcodbc source directory $tcldir"
    }

    if {$build_tdbcsqlite3} {
	# Find Tdbcsqlite3.
	set tdbcsqlite3dir [lindex [lsort [glob -nocomplain -tails -type d \
					-directory $tcltkdir \
					tdbcsqlite3$useversion]] end]
	if {$tdbcsqlite3dir eq ""} {
	    puts stderr "tcltk-man-html: couldn't find Tdbcsqlite3 below $tcltkdir"
	    exit 1
	}
	puts "using Tdbcsqlite3 source directory $tcldir"
    }

    if {$build_tdbcmysql} {
	# Find Tdbcmysql.
	set tdbcmysqldir [lindex [lsort [glob -nocomplain -tails -type d \
					-directory $tcltkdir \
					tdbcmysql$useversion]] end]
	if {$tdbcmysqldir eq ""} {
	    puts stderr "tcltk-man-html: couldn't find Tdbcmysql below $tcltkdir"
	    exit 1
	}
	puts "using Tdbcmysql source directory $tcldir"
    }


    if {$build_tdbcpostgres} {
	# Find Tdbcpostgres.
	set tdbcpostgresdir [lindex [lsort [glob -nocomplain -tails -type d \
						-directory $tcltkdir \
						tdbcpostgres$useversion]] end]
	if {$tdbcpostgresdir eq ""} {
	    puts stderr "tcltk-man-html: couldn't find Tdbcpostgres below \
                         $tcltkdir"
	    exit 1
	}
	puts "using Tdbcpostgres source directory $tcldir"
    }

    # the title for the man pages overall
    global overall_title
    set overall_title ""
    if {$build_tcl} {
	append overall_title "[capitalize $tcldir]"
    }
    if {$build_tcl && $build_tk} {
	append overall_title "/"
    }
    if {$build_tk} {
	append overall_title "[capitalize $tkdir]"
    }
    if {!$build_tcl && !$build_tk &&
	($build_tdbc || $build_tdbcodbc || $build_tdbcsqlite3
	 || $build_tdbcmysql || $build_tdbcpostgres)} {
	append overall_title "[capitalize $tdbcdir]"
    }
    append overall_title " Documentation"
}

proc capitalize {string} {
    return [string toupper $string 0]
}

##
##
##
set manual(report-level) 1

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
    set out "<div class=\"copy\">"
    foreach c $copyrights {
	append out "[copyright $c $level]\n"
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
proc gencss {} {
    set hBd "1px dotted #11577b"
    return "
body, div, p, th, td, li, dd, ul, ol, dl, dt, blockquote {
    font-family: Verdana, sans-serif;
}

pre, code { font-family: 'Courier New', Courier, monospace; }

pre {
    background-color:  #f6fcec;
    border-top:        1px solid #6A6A6A;
    border-bottom:     1px solid #6A6A6A;
    padding:           1em;
    overflow:          auto;
}

body {
    background-color:  #FFFFFF;
    font-size:         12px;
    line-height:       1.25;
    letter-spacing:    .2px;
    padding-left:      .5em;
}

h1, h2, h3, h4 {
    font-family:       Georgia, serif;
    padding-left:      1em;
    margin-top:        1em;
}

h1 {
    font-size:         18px;
    color:             #11577b;
    border-bottom:     $hBd;
    margin-top:        0px;
}

h2 {
    font-size:         14px;
    color:             #11577b;
    background-color:  #c5dce8;
    padding-left:      1em;
    border:            1px solid #6A6A6A;
}

h3, h4 {
    color:             #1674A4;
    background-color:  #e8f2f6;
    border-bottom:     $hBd;
    border-top:        $hBd;
}

h3 { font-size: 12px; }
h4 { font-size: 11px; }

.keylist dt, .arguments dt {
  width: 20em;
  float: left;
  padding: 2px;
  border-top: 1px solid #999;
}

.keylist dt { font-weight: bold; }

.keylist dd, .arguments dd {
  margin-left: 20em;
  padding: 2px;
  border-top: 1px solid #999;
}

.copy {
    background-color:  #f6fcfc;
    white-space:       pre;
    font-size:         80%;
    border-top:        1px solid #6A6A6A;
    margin-top:        2em;
}
"
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
	    {\(fm}	"&#8242;" \
	    {\(mu}	"&#215;" \
	    {\(mi}	"&#8722;" \
	    {\(->}	"<font size=\"+1\">&#8594;</font>" \
	    {\fP}	{\fR} \
	    {\.}	. \
	    {\(bu}	"&#8226;" \
	    ]
    lappend charmap {\o'o^'} {&ocirc;} ; # o-circumflex in re_syntax.n
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
	} then {
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
    set there L[incr manual(long-toc-n)]
    lappend manual(section-toc) \
	    "<DD><A HREF=\"$manual(name).htm#$here\" NAME=\"$there\">$text</A>"
    return "<A NAME=\"$here\">$text</A>"
}
proc option-toc {name class switch} {
    global manual
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
			if {$manual(section) eq "ARGUMENTS" || \
				[regexp {^\[\d+\]$} $rest]} {
			    man-puts "$para<DT>$rest<DD>"
			} elseif {"&#8226;" eq $rest} {
			    man-puts "$para<DT><DD>$rest&nbsp;"
			} else {
			    man-puts "$para<DT>[long-toc $rest]<DD>"
			}
			if {"$manual(name):$manual(section)" eq \
				"selection:DESCRIPTION"} {
			    if {[match-text .RE @rest .RS .RS]} {
				man-puts <DT>[long-toc $rest]<DD>
			    }
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
				man-puts "</DL><P>$rest<DL>"
				backup-text 1
				set para {}
				break
			    } else {
				man-puts "<P>$rest"
				incr accept_RE -1
			    }
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
	man-puts "$para</DL>"
	lappend manual(section-toc) </DL>
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
    regexp {^([^-]+) - (.*)$} $line all head tail
    # output line to manual page untouched
    man-puts $line
    # output line to long table of contents
    lappend manual(section-toc) <DL><DD>$line</DD></DL>
    # separate out the names for future reference
    foreach name [split $head ,] {
	set name [string trim $name]
	if {[llength $name] > 1} {
	    manerror "name has a space: {$name}\nfrom: $line"
	}
	lappend manual(wing-toc) $name
	lappend manual(name-$name) $manual(wing-file)/$manual(name)
    }
}
##
## build a cross-reference link if appropriate
##
proc cross-reference {ref} {
    global manual
    if {[string match "Tcl_*" $ref]} {
	set lref $ref
    } elseif {[string match "Tk_*" $ref]} {
	set lref $ref
    } elseif {$ref eq "Tcl"} {
	set lref $ref
    } else {
	set lref [string tolower $ref]
    }
    ##
    ## nothing to reference
    ##
    if {![info exists manual(name-$lref)]} {
	foreach name {
	    array file history info interp string trace after clipboard grab
	    image option pack place selection tk tkwait update winfo wm
	} {
	    if {[regexp "^$name \[a-z0-9]*\$" $lref] && \
		    [info exists manual(name-$name)] && \
		    $manual(tail) ne "$name.n"} {
		return "<A HREF=\"../$manual(name-$name).htm\">$ref</A>"
	    }
	}
	if {$lref in {stdin stdout stderr end}} {
	    # no good place to send these
	    # tcl tokens?
	    # also end
	}
	return $ref
    }
    ##
    ## would be a self reference
    ##
    foreach name $manual(name-$lref) {
	if {"$manual(wing-file)/$manual(name)" in $name} {
	    return $ref
	}
    }
    ##
    ## multiple choices for reference
    ##
    if {[llength $manual(name-$lref)] > 1} {
	set tcl_i [lsearch -glob $manual(name-$lref) *TclCmd*]
	set tcl_ref [lindex $manual(name-$lref) $tcl_i]
	set tk_i [lsearch -glob $manual(name-$lref) *TkCmd*]
	set tk_ref [lindex $manual(name-$lref) $tk_i]
	if {$tcl_i >= 0 && $manual(wing-file) eq "TclCmd"
		|| $manual(wing-file) eq "TclLib"} {
	    return "<A HREF=\"../$tcl_ref.htm\">$ref</A>"
	}
	if {$tk_i >= 0 && $manual(wing-file) eq "TkCmd"
		|| $manual(wing-file) eq "TkLib"} {
	    return "<A HREF=\"../$tk_ref.htm\">$ref</A>"
	}
	if {$lref eq "exit" && $manual(tail) eq "tclsh.1" && $tcl_i >= 0} {
	    return "<A HREF=\"../$tcl_ref.htm\">$ref</A>"
	}
	puts stderr "multiple cross reference to $ref in $manual(name-$lref) from $manual(wing-file)/$manual(tail)"
	return $ref
    }
    ##
    ## exceptions, sigh, to the rule
    ##
    switch -exact -- $manual(tail) {
	canvas.n {
	    if {$lref eq "focus"} {
		upvar 1 tail tail
		set clue [string first command $tail]
		if {$clue < 0 ||  $clue > 5} {
		    return $ref
		}
	    }
	    if {$lref in {bitmap image text}} {
		return $ref
	    }
	}
	checkbutton.n - radiobutton.n {
	    if {$lref in {image}} {
		return $ref
	    }
	}
	menu.n {
	    if {$lref in {checkbutton radiobutton}} {
		return $ref
	    }
	}
	options.n {
	    if {$lref in {bitmap image set}} {
		return $ref
	    }
	}
	regexp.n {
	    if {$lref in {string}} {
		return $ref
	    }
	}
	source.n {
	    if {$lref in {text}} {
		return $ref
	    }
	}
	history.n {
	    if {$lref in {exec}} {
		return $ref
	    }
	}
	return.n {
	    if {$lref in {error continue break}} {
		return $ref
	    }
	}
	scrollbar.n {
	    if {$lref in {set}} {
		return $ref
	    }
	}
    }
    ##
    ## return the cross reference
    ##
    return "<A HREF=\"../$manual(name-$lref).htm\">$ref</A>"
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
    ## find where each item lives
    ##
    array set offset [list \
	    anchor [string first {<A } $text] \
	    end-anchor [string first {</A>} $text] \
	    quote [string first {``} $text] \
	    end-quote [string first {''} $text] \
	    bold [string first {<B>} $text] \
	    end-bold [string first {</B>} $text] \
	    tcl [string first {Tcl_} $text] \
	    tk [string first {Tk_} $text] \
	    Tcl1 [string first {Tcl manual entry} $text] \
	    Tcl2 [string first {Tcl overview manual entry} $text] \
	    ]
    ##
    ## accumulate a list
    ##
    foreach name [array names offset] {
	if {$offset($name) >= 0} {
	    set invert($offset($name)) $name
	    lappend offsets $offset($name)
	}
    }
    ##
    ## if nothing, then we're done.
    ##
    if {![info exists offsets]} {
	return $text
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
	    set head [string range $text 0 $offset(end-anchor)]
	    set tail [string range $text [expr {$offset(end-anchor)+1}] end]
	    return $head[insert-cross-references $tail]
	}
	quote {
	    if {$offset(end-quote) < 0} {
		return [reference-error "Missing end quote" $text]
	    }
	    if {$invert([lindex $offsets 1]) eq "tk"} {
		set offsets [lreplace $offsets 1 1]
	    }
	    if {$invert([lindex $offsets 1]) eq "tcl"} {
		set offsets [lreplace $offsets 1 1]
	    }
	    switch -exact -- $invert([lindex $offsets 1]) {
		end-quote {
		    set head [string range $text 0 [expr {$offset(quote)-1}]]
		    set body [string range $text [expr {$offset(quote)+2}] \
			    [expr {$offset(end-quote)-1}]]
		    set tail [string range $text \
			    [expr {$offset(end-quote)+2}] end]
		    return "$head``[cross-reference $body]''[insert-cross-references $tail]"
		}
		bold -
		anchor {
		    set head [string range $text \
			    0 [expr {$offset(end-quote)+1}]]
		    set tail [string range $text \
			    [expr {$offset(end-quote)+2}] end]
		    return "$head[insert-cross-references $tail]"
		}
	    }
	    return [reference-error "Uncaught quote case" $text]
	}
	bold {
	    if {$offset(end-bold) < 0} {
		return $text
	    }
	    if {$invert([lindex $offsets 1]) eq "tk"} {
		set offsets [lreplace $offsets 1 1]
	    }
	    if {$invert([lindex $offsets 1]) eq "tcl"} {
		set offsets [lreplace $offsets 1 1]
	    }
	    switch -exact -- $invert([lindex $offsets 1]) {
		end-bold {
		    set head [string range $text 0 [expr {$offset(bold)-1}]]
		    set body [string range $text [expr {$offset(bold)+3}] \
			    [expr {$offset(end-bold)-1}]]
		    set tail [string range $text \
			    [expr {$offset(end-bold)+4}] end]
		    return "$head<B>[cross-reference $body]</B>[insert-cross-references $tail]"
		}
		anchor {
		    set head [string range $text \
			    0 [expr {$offset(end-bold)+3}]]
		    set tail [string range $text \
			    [expr {$offset(end-bold)+4}] end]
		    return "$head[insert-cross-references $tail]"
		}
	    }
	    return [reference-error "Uncaught bold case" $text]
	}
	tk {
	    set head [string range $text 0 [expr {$offset(tk)-1}]]
	    set tail [string range $text $offset(tk) end]
	    if {![regexp {^(Tk_\w+)(.*)$} $tail all body tail]} {
		return [reference-error "Tk regexp failed" $text]
	    }
	    return $head[cross-reference $body][insert-cross-references $tail]
	}
	tcl {
	    set head [string range $text 0 [expr {$offset(tcl)-1}]]
	    set tail [string range $text $offset(tcl) end]
	    if {![regexp {^(Tcl_\w+)(.*)$} $tail all body tail]} {
		return [reference-error {Tcl regexp failed} $text]
	    }
	    return $head[cross-reference $body][insert-cross-references $tail]
	}
	Tcl1 -
	Tcl2 {
	    set off [lindex $offsets 0]
	    set head [string range $text 0 [expr {$off-1}]]
	    set body Tcl
	    set tail [string range $text [expr {$off+3}] end]
	    return $head[cross-reference $body][insert-cross-references $tail]
	}
	end-anchor -
	end-bold -
	end-quote {
	    return [reference-error "Out of place $invert([lindex $offsets 0])" $text]
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
	    switch -exact -- $manual(section) {
		NAME {
		    if {$manual(tail) in {CrtImgType.3 CrtItemType.3 CrtPhImgFmt.3}} {
			# these manual pages have two NAME sections
			if {[info exists manual($manual(tail)-NAME)]} {
			    return
			}
			set manual($manual(tail)-NAME) 1
		    }
		    set names {}
		    while {1} {
			set line [next-text]
			if {[is-a-directive $line]} {
			    backup-text 1
			    output-name [join $names { }]
			    return
			} else {
			    lappend names [string trim $line]
			}
		    }
		}
		SYNOPSIS {
		    lappend manual(section-toc) <DL>
		    while {1} {
			if {
			    [next-op-is .nf rest]
			    || [next-op-is .br rest]
			    || [next-op-is .fi rest]
			} then {
			    continue
			}
			if {
			    [next-op-is .SH rest]
			    || [next-op-is .SS rest]
			    || [next-op-is .BE rest]
			    || [next-op-is .SO rest]
			} then {
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
			    man-puts $more<BR>
			    if {$manual(wing-file) in {TclLib TkLib}} {
				lappend manual(section-toc) <DD>$more
			    }
			}
		    }
		    lappend manual(section-toc) </DL>
		    return
		}
		{SEE ALSO} {
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
		KEYWORDS {
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
			    lappend manual(keyword-$key) [list $manual(name) $manual(wing-file)/$manual(name).htm]
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
	.PP {
	    man-puts <P>
	}
	.RS {
	    output-RS-list
	    return
	}
	.RE {
	    manerror "unexpected .RE"
	    return
	}
	.br {
	    man-puts <BR>
	    return
	}
	.DE {
	    manerror "unexpected .DE"
	    return
	}
	.DS {
	    if {[next-op-is .ta rest]} {
		# skip the leading .ta directive if it is there
	    }
	    if {[match-text @stuff .DE]} {
		set td "<td><p style=\"font-size:12px;padding-left:.5em;padding-right:.5em;\">"
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
	.CE {
	    manerror "unexpected .CE"
	    return
	}
	.sp {
	    man-puts <P>
	}
	.ta {
	    # these are tab stop settings for short tables
	    switch -exact -- $manual(name):$manual(section) {
		{bind:MODIFIERS} -
		{bind:EVENT TYPES} -
		{bind:BINDING SCRIPTS AND SUBSTITUTIONS} -
		{expr:OPERANDS} -
		{expr:MATH FUNCTIONS} -
		{history:DESCRIPTION} -
		{history:HISTORY REVISION} -
		{switch:DESCRIPTION} -
		{upvar:DESCRIPTION} {
		    return;			# fix.me
		}
		default {
		    manerror "ignoring $line"
		}
	    }
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
	.fi {
	    manerror "ignoring $line"
	}
	.na -
	.ad -
	.nr -
	.if -
	.UL -
	.ne {
	    manerror "ignoring $line"
	}
	.\\\" {
	    manerror "ignoring comment $line"
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

proc makedirhier {dir} {
    if {![file isdirectory $dir] && \
	    [catch {file mkdir $dir} error]} {
	return -code error "cannot create directory $dir: $error"
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

##
## foreach of the man directories specified by args
## convert manpages into hypertext in the directory
## specified by html.
##
proc make-man-pages {html args} {
    global manual overall_title tcltkdesc
    makedirhier $html
    set cssfd [open $html/$::CSSFILE w]
    puts $cssfd [gencss]
    close $cssfd
    set manual(short-toc-n) 1
    set manual(short-toc-fp) [open $html/[indexfile] w]
    puts $manual(short-toc-fp) [htmlhead $overall_title $overall_title]
    puts $manual(short-toc-fp) "<DL class=\"keylist\">"
    set manual(merge-copyrights) {}
    foreach arg $args {
	# preprocess to set up subheader for the rest of the files
	if {![llength $arg]} {
	    continue
	}
	set name [lindex $arg 1]
	set file [lindex $arg 2]
	lappend manual(subheader) $name $file
    }
    foreach arg $args {
	if {![llength $arg]} {
	    continue
	}
	set manual(wing-glob) [lindex $arg 0]
	set manual(wing-name) [lindex $arg 1]
	set manual(wing-file) [lindex $arg 2]
	set manual(wing-description) [lindex $arg 3]
	set manual(wing-copyrights) {}
	makedirhier $html/$manual(wing-file)
	set manual(wing-toc-fp) [open $html/$manual(wing-file)/[indexfile] w]
	# whistle
	puts stderr "scanning section $manual(wing-name)"
	# put the entry for this section into the short table of contents
	puts $manual(short-toc-fp) "<DT><A HREF=\"$manual(wing-file)/[indexfile]\">$manual(wing-name)</A></DT><DD>$manual(wing-description)</DD>"
	# initialize the wing table of contents
	puts $manual(wing-toc-fp) [htmlhead $manual(wing-name) \
		$manual(wing-name) $overall_title "../[indexfile]"]
	# initialize the short table of contents for this section
	set manual(wing-toc) {}
	# initialize the man directory for this section
	makedirhier $html/$manual(wing-file)
	# initialize the long table of contents for this section
	set manual(long-toc-n) 1
	# get the manual pages for this section
	set manual(pages) [lsort -dictionary [glob $manual(wing-glob)]]
	set n [lsearch -glob $manual(pages) */ttk_widget.n]
	if {$n >= 0} {
	    set manual(pages) "[lindex $manual(pages) $n] [lreplace $manual(pages) $n $n]"
	}
	set n [lsearch -glob $manual(pages) */options.n]
	if {$n >= 0} {
	    set manual(pages) "[lindex $manual(pages) $n] [lreplace $manual(pages) $n $n]"
	}
	# set manual(pages) [lrange $manual(pages) 0 5]
	set LQ \u201c
	set RQ \u201d
	foreach manual_page $manual(pages) {
	    set manual(page) $manual_page
	    # whistle
	    puts stderr "scanning page $manual(page)"
	    set manual(tail) [file tail $manual(page)]
	    set manual(name) [file root $manual(tail)]
	    set manual(section) {}
	    if {$manual(name) in {case pack-old menubar}} {
		# obsolete
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
	    manreport 100 $manual(name)
	    set ignored 0
	    while {[gets $manual(infp) line] >= 0} {
		manreport 100 $line
		if {"$line" eq "'\\\" IGNORE"} {
		    set ignored 1
		    continue
		}
		if {"$line" eq "'\\\" END IGNORE"} {
		    set ignored 0
		    continue
		}
		if {$ignored} {
		    continue
		}
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
		    .ad - .na - .so - .ne - .AS - .VE - .VS - . {
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
			set rest [regexp -all -inline {\"(?:[^""]+)\"|\S+} $rest]
			addbuffer $LQ [unquote [lindex $rest 0]] $RQ \
			    [unquote [lindex $rest 1]]
		    }
		    .PQ {
			set rest [regexp -all -inline {\"(?:[^""]+)\"|\S+} $rest]
			addbuffer ( $LQ [unquote [lindex $rest 0]] $RQ \
			    [unquote [lindex $rest 1]] ) \
			    [unquote [lindex $rest 2]]
		    }
		    .QR {
			set rest [regexp -all -inline {\"(?:[^""]+)\"|\S+} $rest]
			addbuffer $LQ [unquote [lindex $rest 0]] - \
			    [unquote [lindex $rest 1]] $RQ \
			    [unquote [lindex $rest 2]]
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
			if {"$rest" ne {}} {
			    manerror "unexpected argument: $line"
			}
			lappend manual(text) $code
		    }
		    .AP {
			flushbuffer
			lappend manual(text) [concat .IP [process-text "[lindex $rest 0] \\fB[lindex $rest 1]\\fR ([lindex $rest 2])"]]
		    }
		    .IP {
			flushbuffer
			regexp {^(.*) +\d+$} $rest all rest
			lappend manual(text) ".IP [process-text [unquote [string trim $rest]]]"
		    }
		    .TP {
			flushbuffer
			while {[is-a-directive [set next [gets $manual(infp)]]]} {
			    manerror "ignoring $next after .TP"
			}
			if {"$next" ne {'}} {
			    lappend manual(text) ".IP [process-text $next]"
			}
		    }
		    .OP {
			flushbuffer
			lappend manual(text) [concat .OP [process-text \
				"\\fB[lindex $rest 0]\\fR \\fB[lindex $rest 1]\\fR \\fB[lindex $rest 2]\\fR"]]
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
			error "found .. outside of .de"
		    }
		    default {
			flushbuffer
			manerror "unrecognized format directive: $line"
		    }
		}
	    }
	    flushbuffer
	    close $manual(infp)
	    # fixups
	    if {$manual(.RS) != 0} {
		puts "unbalanced .RS .RE"
	    }
	    if {$manual(.DS) != 0} {
		puts "unbalanced .DS .DE"
	    }
	    if {$manual(.CS) != 0} {
		puts "unbalanced .CS .CE"
	    }
	    if {$manual(.SO) != 0} {
		puts "unbalanced .SO .SE"
	    }
	    # output conversion
	    open-text
	    set haserror 0
	    if {[next-op-is .HS rest]} {
		set manual($manual(name)-title) \
			"[lrange $rest 1 end] [lindex $rest 0] manual page"
	    } elseif {[next-op-is .TH rest]} {
		set manual($manual(name)-title) "[lindex $rest 0] manual page - [lrange $rest 4 end]"
	    } else {
		set haserror 1
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
		set manual(wing-copyrights) [merge-copyrights $manual(wing-copyrights) $manual(copyrights)]
	    }
	    #
	    # make the long table of contents for this page
	    #
	    set manual(toc-$manual(wing-file)-$manual(name)) [concat <DL> $manual(section-toc) </DL>]
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
	set perline [expr {120 / $width}]
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
	    append rows([expr {$n%$nrows}]) \
		    "<td> <a href=\"$tail.htm\">$name</a>"
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
	puts $manual(wing-toc-fp) $::logo
	puts $manual(wing-toc-fp) "</BODY></HTML>"
	close $manual(wing-toc-fp)
	set manual(merge-copyrights) [merge-copyrights $manual(merge-copyrights) $manual(wing-copyrights)]
    }

    ##
    ## build the keyword index.
    ##
    file delete -force -- $html/Keywords
    makedirhier $html/Keywords
    set keyfp [open $html/Keywords/[indexfile] w]
    puts $keyfp [htmlhead "$tcltkdesc Keywords" "$tcltkdesc Keywords" \
		     $overall_title "../[indexfile]"]
    set letters {A B C D E F G H I J K L M N O P Q R S T U V W X Y Z}
    # Create header first
    set keyheader {}
    foreach a $letters {
	set keys [array names manual "keyword-\[[string totitle $a$a]\]*"]
	if {[llength $keys]} {
	    lappend keyheader "<A HREF=\"$a.htm\">$a</A>"
	} else {
	    # No keywords for this letter
	    lappend keyheader $a
	}
    }
    set keyheader "<H3>[join $keyheader " |\n"]</H3>"
    puts $keyfp $keyheader
    foreach a $letters {
	set keys [array names manual "keyword-\[[string totitle $a$a]\]*"]
	if {![llength $keys]} {
	    continue
	}
	# Per-keyword page
	set afp [open $html/Keywords/$a.htm w]
	puts $afp [htmlhead "$tcltkdesc Keywords - $a" \
		       "$tcltkdesc Keywords - $a" \
		       $overall_title "../[indexfile]"]
	puts $afp $keyheader
	puts $afp "<DL class=\"keylist\">"
	foreach k [lsort -dictionary $keys] {
	    set k [string range $k 8 end]
	    puts $afp "<DT><A NAME=\"$k\">$k</A></DT>"
	    puts $afp "<DD>"
	    set refs {}
	    foreach man $manual(keyword-$k) {
		set name [lindex $man 0]
		set file [lindex $man 1]
		lappend refs "<A HREF=\"../$file\">$name</A>"
	    }
	    puts $afp "[join $refs {, }]</DD>"
	}
	puts $afp "</DL>"
	# insert merged copyrights
	puts $afp [copyout $manual(merge-copyrights)]
	puts $afp $::logo
	puts $afp "</BODY></HTML>"
	close $afp
    }
    # insert merged copyrights
    puts $keyfp [copyout $manual(merge-copyrights)]
    puts $keyfp $::logo
    puts $keyfp "</BODY></HTML>"
    close $keyfp

    ##
    ## finish off short table of contents
    ##
    puts $manual(short-toc-fp) "<DT><A HREF=\"Keywords/[indexfile]\">Keywords</A><DD>The keywords from the $tcltkdesc man pages."
    puts $manual(short-toc-fp) "</DL>"
    # insert merged copyrights
    puts $manual(short-toc-fp) [copyout $manual(merge-copyrights)]
    puts $manual(short-toc-fp) $::logo
    puts $manual(short-toc-fp) "</BODY></HTML>"
    close $manual(short-toc-fp)

    ##
    ## output man pages
    ##
    unset manual(section)
    foreach path $manual(all-pages) {
	set manual(wing-file) [file dirname $path]
	set manual(tail) [file tail $path]
	set manual(name) [file root $manual(tail)]
	set text $manual(output-$manual(wing-file)-$manual(name))
	set ntext 0
	foreach item $text {
	    incr ntext [llength [split $item \n]]
	    incr ntext
	}
	set toc $manual(toc-$manual(wing-file)-$manual(name))
	set ntoc 0
	foreach item $toc {
	    incr ntoc [llength [split $item \n]]
	    incr ntoc
	}
	puts stderr "rescanning page $manual(name) $ntoc/$ntext"
	set outfd [open $html/$manual(wing-file)/$manual(name).htm w]
	puts $outfd [htmlhead "$manual($manual(name)-title)" \
		$manual(name) $manual(wing-file) "[indexfile]" \
		$overall_title "../[indexfile]"]
	if {
	    (($ntext > 60) && ($ntoc > 32)) || $manual(tail) in {
		Hash LinkVar SetVar TraceVar ConfigWidg CrtImgType CrtItemType
		CrtPhImgFmt DoOneEvent GetBitmap GetColor GetCursor GetDash
		GetJustify GetPixels GetVisual ParseArgv QueueEvent
	    }
	} then {
	    foreach item $toc {
		puts $outfd $item
	    }
	}
	foreach item $text {
	    puts $outfd [insert-cross-references $item]
	}
	puts $outfd $::logo
	puts $outfd "</BODY></HTML>"
	close $outfd
    }
    return {}
}

parse_command_line

set tcltkdesc ""; set cmdesc ""; set appdir ""
if {$build_tcl} {
    append tcltkdesc "Tcl"
    append cmdesc "Tcl"
    append appdir "$tcldir"
}
if {$build_tcl && $build_tk} {
    append tcltkdesc "/"
    append cmdesc " and "
    append appdir ","
}
if {$build_tk} {
    append tcltkdesc "Tk"
    append cmdesc "Tk"
    append appdir "$tkdir"
}
if {!$build_tcl && !$build_tk} {
    set cmdesc "Tcl DataBase Connectivity"
    set tcltkdesc "TDBC"
    set sep {}
} else {
    set sep ,
}
if {$build_tdbc} {
    append appdir $sep $tdbcdir
    set sep ,
}
if {$build_tdbcodbc} {
    append appdir $sep $tdbcodbcdir
    set sep ,
}
if {$build_tdbcsqlite3} {
    append appdir $sep $tdbcsqlite3dir
    set sep ,
}
if {$build_tdbcmysql} {
    append appdir $sep $tdbcmysqldir
    set sep ,
}
if {$build_tdbcpostgres} {
    append appdir $sep $tdbcpostgresdir
    set sep ,
}

set usercmddesc "The interpreters which implement $cmdesc."
set tclcmddesc {The commands which the <B>tclsh</B> interpreter implements.}
set tkcmddesc {The additional commands which the <B>wish</B> interpreter implements.}
set tcllibdesc {The C functions which a Tcl extended C program may use.}
set tklibdesc {The additional C functions which a Tk extended C program may use.}
set tdbcdesc {The commands that are implemented by Tcl DataBase Connectivity (TDBC)}
set tdbclibdesc {The C functions that are implemented by Tcl DataBase Connectivity (TDBC)}
set tdbcodbcdesc {The ODBC driver for Tcl DataBase Connectivity (TDBC)}
set tdbcsqlite3desc {The Sqlite3 driver for Tcl DataBase Connectivity (TDBC)}
set tdbcmysqldesc {The MySQL driver for Tcl DataBase Connectivity (TDBC)}
set tdbcpostgresdesc {The Postgres driver for Tcl DataBase Connectivity (TDBC)}

    
if {[catch {
    make-man-pages $webdir \
	[expr {($build_tcl || $build_tk) ? "$tcltkdir/{$appdir}/doc/*.1 \"$tcltkdesc Applications\" UserCmd {$usercmddesc}" : ""}] \
	[expr {$build_tcl ? "$tcltkdir/$tcldir/doc/*.n {Tcl Commands} TclCmd {$tclcmddesc}" : ""}] \
	[expr {$build_tk ? "$tcltkdir/$tkdir/doc/*.n {Tk Commands} TkCmd {$tkcmddesc}" : ""}] \
	[expr {$build_tcl ? "$tcltkdir/$tcldir/doc/*.3 {Tcl Library} TclLib {$tcllibdesc}" : ""}] \
	[expr {$build_tk ? "$tcltkdir/$tkdir/doc/*.3 {Tk Library} TkLib {$tklibdesc}" : ""}] \
	[expr {$build_tdbc ? "$tcltkdir/$tdbcdir/doc/*.n {Tcl Database Connectivity} TDBC {$tdbcdesc}" : ""}] \
	[expr {$build_tdbcodbc ? "$tcltkdir/$tdbcodbcdir/doc/*.n {TDBC-ODBC Bridge} Tdbcodbc {$tdbcodbcdesc}" : ""}] \
	[expr {$build_tdbcsqlite3 ? "$tcltkdir/$tdbcsqlite3dir/doc/*.n {TDBC driver for Sqlite3} Tdbcsqlite3 {$tdbcsqlite3desc}" : ""}] \
	[expr {$build_tdbcmysql ? "$tcltkdir/$tdbcmysqldir/doc/*.n {TDBC driver for MySQL} Tdbcmysql {$tdbcmysqldesc}" : ""}] \
	[expr {$build_tdbcpostgres ? "$tcltkdir/$tdbcpostgresdir/doc/*.n {TDBC driver for Postgres} Tdbcpostgres {$tdbcpostgresdesc}" : ""}] \
	[expr {$build_tdbc ? "$tcltkdir/$tdbcdir/doc/*.3 {Tcl Database Connectivity C API} TdbcLib {$tdbclibdesc}" : ""}] \
} error]} {
    puts $error\n$errorInfo
}

# Local Variables:
# mode: tcl
# End: