set tix_version 8.2.0

package require Tcl 8.0

# $Id: tix-man2html.tcl,v 1.4 2001/12/09 05:06:29 idiscovery Exp $
#
# Convert Ousterhout format man pages into highly crosslinked
# hypertext.
#
# Along the way detect many unmatched font changes and other odd
# things.
#
# Note well, this program is a hack rather than a piece of software
# engineering.  In that sense it's probably a good example of things
# that a scripting language, like Tcl, can do well.  It is offered as
# an example of how someone might convert a specific set of man pages
# into hypertext, not as a general solution to the problem.  If you
# try to use this, you'll be very much on your own.
#
# Copyright (c) 1995-1997 Roger E. Critchlow Jr
#
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.
# 
# IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
# FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
# ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
# DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# 
# THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
# IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
# NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
# MODIFICATIONS.
#
# Revisions:
#  May 15, 1995 - initial release
#  May 16, 1995 - added a back to home link to toplevel table of
#	contents.
#  May 18, 1995 - broke toplevel table of contents into separate
#	pages for each section, and broke long table of contents
#	into a one page for each man page.
#  Mar 10, 1996 - updated for tcl7.5b3/tk4.1b3
#  Apr 14, 1996 - incorporated command line parsing from Tom Tromey,
#		  <tromey@creche.cygnus.com> -- thanks Tom.
#		- updated for tcl7.5/tk4.1 final release.
#		- converted to same copyright as the man pages.
#  Sep 14, 1996 - made various modifications for tcl7.6b1/tk4.2b1
#  Oct 18, 1996 - added tcl7.6/tk4.2 to the list of distributions.
#  Oct 22, 1996 - major hacking on indentation code and elsewhere.
#  Mar  4, 1997 - 
#  May 28, 1997 - added tcl8.0b1/tk8.0b1 to the list of distributions
#		- cleaned source for tclsh8.0 execution
#		- renamed output files for windoze installation
#		- added spaces to tables
#  Oct 24, 1997 - moved from 8.0b1 to 8.0 release
#  Jan 19, 2001 - added Tix support

set Version "0.20"

proc parse_command_line {} {
    global argv Version

    # These variables determine where the man pages come from and where
    # the converted pages go to.
    global tcltkdir tixdir tkdir tcldir webdir

    # Set defaults based on original code.
    set tcltkdir ../..
    set tkdir {}
    set tcldir {}
    set webdir ../man/html

    # Directory names for Tcl, Tk and Tix, in priority order.
    set tclDirList {tcl8.3.3 tcl8.3.2 tcl8.2.3 tcl8.3 tcl8.2 tcl8.1 tcl8.0 tcl}
    set tkDirList {tk8.3.3 tk8.3.2 tk8.2.3 tk8.3 tk8.2 tk8.1 tk8.0 tk}
    set tixDirList {tix-8.2.0 tix}

    # Handle arguments a la GNU:
    #   --version
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

	    default {
		puts stderr "tcltk-man-html: unrecognized option -- `$option'"
		exit 1
	    }
	}
    }

    # Find Tcl.
    foreach dir $tclDirList {
	if {[file isdirectory $tcltkdir/$dir]} then {
	    set tcldir $dir
	    break
	}
    }
    if {$tcldir == ""} then {
	puts stderr "tcltk-man-html: couldn't find Tcl below $tcltkdir"
	exit 1
    }

    # Find Tk.
    foreach dir $tkDirList {
	if {[file isdirectory $tcltkdir/$dir]} then {
	    set tkdir $dir
	    break
	}
    }
    if {$tkdir == ""} then {
	puts stderr "tcltk-man-html: couldn't find Tk below $tcltkdir"
	exit 1
    }

    # Find Tix.
    foreach dir $tixDirList {
	if {[file isdirectory $tcltkdir/$dir]} then {
	    set tixdir $dir
	    break
	}
    }
    if {$tixdir == ""} then {
	puts stderr "tcltk-man-html: couldn't find Tix below $tcltkdir"
	exit 1
    }

    # the title for the man pages overall
    global overall_title env tix_version

    if {[info exists env(WITH_TCL_TK)]} {
        set overall_title "[capitalize tix$tix_version]/[capitalize $tcldir]/[capitalize $tkdir] Reference Manual"
    } else {
        set overall_title "[capitalize tix$tix_version] Reference Manual"
    }
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
    if {[info exists manual(name)]} {
	set name $manual(name)
    }
    if {[info exists manual(section)] && [string length $manual(section)]} {
	puts stderr "$name: $manual(section):  $msg"
    } else {
	puts stderr "$name: $msg"
    }
}

proc manreport {level msg} {
    global manual
    if {$level < $manual(report-level)} {
	manerror $msg
    }
}

proc fatal {msg} {
    global manual
    manerror $msg
    exit 1
}
##
## parsing
##
proc unquote arg {
    return [string map [list \" {}] $arg]
}

proc parse-directive {line codename restname} {
    upvar $codename code $restname rest
    return [regexp {^(\.[.a-zA-Z0-9]*) *(.*)} $line all code rest]
}

proc process-text {text} {
    global manual
    # preprocess text
    set text [string map [list \
	    {\&}	"\t" \
	    {&}		{&amp;} \
	    {\\}	{&#92;} \
	    {\e}	{&#92;} \
	    {\ }	{&nbsp;} \
	    {\|}	{&nbsp;} \
	    {\0}	{ } \
	    {\%}	{} \
	    "\\\n"	"\n" \
	    \"		{&quot;} \
	    {<}		{&lt;} \
	    {>}		{&gt;} \
	    {\(+-}	{&#177;} \
	    {\fP}	{\fR} \
	    {\.}	. \
	    ] $text]
    regsub -all {\\o'o\^'} $text {\&ocirc;} text; # o-circumflex in re_syntax.n
    regsub -all {\\-\\\|\\-} $text -- text;	# two hyphens
    regsub -all -- {\\-\\\^\\-} $text -- text;	# two hyphens
    regsub -all {\\-} $text - text;		# a hyphen
    regsub -all "\\\\\n" $text "\\&\#92;\n" text; # backslashed newline
    while {[regexp {\\} $text]} {
	# C R
	if {[regsub {^([^\\]*)\\fC([^\\]*)\\fR(.*)$} $text {\1<TT>\2</TT>\3} text]} continue
	# B R
	if {[regsub {^([^\\]*)\\fB([^\\]*)\\fR(.*)$} $text {\1<B>\2</B>\3} text]} continue
	# B I
	if {[regsub {^([^\\]*)\\fB([^\\]*)\\fI(.*)$} $text {\1<B>\2</B>\\fI\3} text]} continue
	# I R
	if {[regsub {^([^\\]*)\\fI([^\\]*)\\fR(.*)$} $text {\1<I>\2</I>\3} text]} continue
	# I B
	if {[regsub {^([^\\]*)\\fI([^\\]*)\\fB(.*)$} $text {\1<I>\2</I>\\fB\3} text]} continue
	# B B, I I, R R
	if {[regsub {^([^\\]*)\\fB([^\\]*)\\fB(.*)$} $text {\1\\fB\2\3} ntext]
	    || [regsub {^([^\\]*)\\fI([^\\]*)\\fI(.*)$} $text {\1\\fI\2\3} ntext]
	    || [regsub {^([^\\]*)\\fR([^\\]*)\\fR(.*)$} $text {\1\\fR\2\3} ntext]} {
	    manerror "process-text: impotent font change: $text"
	    set text $ntext
	    continue
	}
	# unrecognized 
	manerror "process-text: uncaught backslash: $text"
	set text [string map [list "\\" "#92;"] $text]
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
    return [expr {[string first . $line] == 0}]
}
proc split-directive {line opname restname} {
    upvar $opname op $restname rest
    set op [string range $line 0 2]
    set rest [string trim [string range $line 3 end]]
}
proc next-op-is {op restname} {
    global manual
    upvar $restname rest
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
	if {[string equal $arg $targ]} {
	    incr nback
	    incr manual(text-pointer)
	    continue
	}
	if {[regexp {^@([_a-zA-Z0-9]+)$} $arg all name]} {
	    upvar $name var
	    set var $targ
	    incr nback
	    incr manual(text-pointer)
	    continue
	}
	if {[regexp {^(\.[a-zA-Z][a-zA-Z])@([_a-zA-Z0-9]+)$} $arg all op name]\
		&& [string equal $op [lindex $targ 0]]} {
	    upvar $name var
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
    lappend manual(section-toc) "<DD><A HREF=\"$manual(name).htm#$here\" NAME=\"$there\">$text</A>"
    return "<A NAME=\"$here\">$text</A>"
}
proc option-toc {name class switch} {
    global manual
    if {[string equal $manual(section) "WIDGET-SPECIFIC OPTIONS"] ||
        [string equal $manual(section) "CONFIGURATION OPTIONS"] ||
        [regexp ITEMS $manual(section)]} {
	# link the defined option into the long table of contents
	set link [long-toc "$switch, $name, $class"]
	regsub -- "$switch, $name, $class" $link "$switch" link
	return $link
    } elseif {[string equal $manual(name):$manual(section) \
	    "options:DESCRIPTION"]} {
	# link the defined standard option to the long table of
	# contents and make a target for the standard option references
	# from other man pages.
	set first [lindex $switch 0]
	set here M$first
	set there L[incr manual(long-toc-n)]
	set manual(standard-option-$first) "<A HREF=\"$manual(name).htm#$here\">$switch, $name, $class</A>"
	lappend manual(section-toc) "<DD><A HREF=\"$manual(name).htm#$here\" NAME=\"$there\">$switch, $name, $class</A>"
	return "<A NAME=\"$here\">$switch</A>"
    } else {
	error "option-toc in $manual(name) section $manual(section)"
    }
}
proc std-option-toc {name} {
    global manual
    if {[info exists manual(standard-option-$name)]} {
	lappend manual(section-toc) <DD>$manual(standard-option-$name)
	return $manual(standard-option-$name)
    }
    set here M[incr manual(section-toc-n)]
    set there L[incr manual(long-toc-n)]
    set other M$name
    lappend manual(section-toc) "<DD><A HREF=\"options.htm#$other\">$name</A>"
    return "<A HREF=\"options.htm#$other\">$name</A>"
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
	switch -exact [llength $rest] {
	    3 {
		set switch [lindex $rest 0]
		set name [lindex $rest 1]
		set class [lindex $rest 2]
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
	if {![regexp {^(<.>)([-a-zA-Z0-9 ]+)(</.>)$} $switch all oswitch switch cswitch]} {
	    if {![regexp {^(<.>)([-a-zA-Z0-9 ]+) or ([-a-zA-Z0-9 ]+)(</.>)$} $switch all oswitch switch1 switch2 cswitch]} {
		error "not Switch: $switch"
	    } else {
		set switch "$switch1$cswitch or $oswitch$switch2"
	    }
	}
	if {![regexp {^(<.>)([a-zA-Z0-9]*)(</.>)$} $name all oname name cname]} {
	    error "not Name: $name"
	}
	if {![regexp {^(<.>)([a-zA-Z0-9]*)(</.>)$} $class all oclass class cclass]} {
	    error "not Class: $class"
	}
	man-puts "$para<DT>Command-Line Name: $oswitch[option-toc $name $class $switch]$cswitch"
	man-puts "<DT>Database Name: $oname$name$cname"
	man-puts "<DT>Database Class: $oclass$class$cclass"
	man-puts <DD>[next-text]
	set para <P>
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
    man-puts <DL><P><DD>
    while {[more-text]} {
	set line [next-text]
	if {[is-a-directive $line]} {
	    split-directive $line code rest
	    switch -exact $code {
		.RE {
		    break
		}
		.SH {
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
    if {[string equal $rest {}]} {
	# blank label, plain indent, no contents entry
	man-puts <DL><P><DD>
	while {[more-text]} {
	    set line [next-text]
	    if {[is-a-directive $line]} {
		split-directive $line code rest
		if {[string equal $code ".IP"] && [string equal $rest {}]} {
		    man-puts "<P>"
		    continue
		}
		if {[lsearch {.br .DS .RS} $code] >= 0} {
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
	if {[string compare $context ".SH"]} {
	    man-puts <P>
	}
	man-puts <DL>
	lappend manual(section-toc) <DL>
	backup-text 1
	set accept_RE 0
	while {[more-text]} {
	    set line [next-text]
	    if {[is-a-directive $line]} {
		split-directive $line code rest
		switch -exact $code {
		    .IP {
			if {$accept_RE} {
			    output-IP-list .IP $code $rest
			    continue
			}
			if {[string equal $manual(section) "ARGUMENTS"] || \
				[regexp {^\[[0-9]+\]$} $rest]} {
			    man-puts "<P><DT>$rest<DD>"
			} else {
                            set canlink 0
                            if {[regexp {<B>([A-Za-z0-9_]+)</B>} $rest \
                                     foo ref]} {
                                if {"$foo" == "$rest"} {
                                    set canlink 1
                                }
                            }
                            set outtext "<P><DT>[long-toc $rest]<DD>"
                            if {$canlink} {
                                regsub ">$rest</A>" $outtext \
                                    "></A><B>[cross-reference $ref]</B>" \
                                    outtext
                            }
                            man-puts $outtext
			}
			if {[string equal $manual(name):$manual(section) \
				"selection:DESCRIPTION"]} {
			    if {[match-text .RE @rest .RS .RS]} {
				man-puts <DT>[long-toc $rest]<DD>
			    }
			}
		    }
		    .sp -
		    .br -
		    .DS -
		    .CS {
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
                    .LP {
                        man-puts "<P>$rest"
                    }
		    .PP {
			if {[match-text @rest1 .br @rest2 .RS]} {
			    # yet another nroff kludge as above
			    man-puts "<P><DT>[long-toc $rest1]"
			    man-puts "<DT>[long-toc $rest2]<DD>"
			    incr accept_RE 1
			} elseif {[match-text @rest .RE]} {
			    # gad, this is getting ridiculous
			    if { ! $accept_RE} {
				man-puts "</DL><P>$rest<DL>"
				backup-text 1
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
			if { ! $accept_RE} {
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
	}
	man-puts <P></DL>
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
    lappend manual(section-toc) <DL><DD>$line</DL>
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

    regsub {[(][31n][)]} $ref "" ref

    if {[string match Tcl_* $ref]} {
	set lref $ref
    } elseif {[string match Tk_* $ref]} {
	set lref $ref
    } elseif {[string equal $ref "Tcl"]} {
	set lref $ref
    } elseif {[regexp -nocase tix $ref]} {
	set lref $ref
    } else {
	set lref [string tolower $ref]
    }

    ##
    ## nothing to reference
    ##
    if { ! [info exists manual(name-$lref)]} {
	foreach name {array file history info interp string trace
	after clipboard grab image option pack place selection tk tkwait update winfo wm} {
	    if {[regexp "^$name \[a-z0-9]*\$" $lref] && \
		    [string compare $manual(tail) "$name.n"] && \
                    [info exists manual(name-$name)]} {                    
		return "<A HREF=\"../$manual(name-$name).htm\">$ref</A>"
	    }
	}
	if {[lsearch {stdin stdout stderr end} $lref] >= 0} {
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
	if {[lsearch $name $manual(wing-file)/$manual(name)] >= 0} {
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
	if {$tcl_i >= 0 && "$manual(wing-file)" == {TclCmd} ||  "$manual(wing-file)" == {TclLib}} {
	    return "<A HREF=\"../$tcl_ref.htm\">$ref</A>"
	}
	if {$tk_i >= 0 && "$manual(wing-file)" == {TkCmd} || "$manual(wing-file)" == {TkLib}} {
	    return "<A HREF=\"../$tk_ref.htm\">$ref</A>"
	}
	if {"$lref" == {exit} && "$manual(tail)" == {tclsh.1} && $tcl_i >= 0} {
	    return "<A HREF=\"../$tcl_ref.htm\">$ref</A>"
	}
	puts stderr "multiple cross reference to $ref in $manual(name-$lref) from $manual(wing-file)/$manual(tail)"
	return $ref
    }
    ##
    ## exceptions, sigh, to the rule
    ##
    switch $manual(tail) {
	canvas.n {
	    if {$lref == {focus}} {
		upvar tail tail
		set clue [string first command $tail]
		if {$clue < 0 ||  $clue > 5} {
		    return $ref
		}
	    }
	    if {[lsearch {bitmap image text} $lref] >= 0} {
		return $ref
	    }
	}
	checkbutton.n -
	radiobutton.n {
	    if {[lsearch {image} $lref] >= 0} {
		return $ref
	    }
	}
	menu.n {
	    if {[lsearch {checkbutton radiobutton} $lref] >= 0} {
		return $ref
	    }
	}
	options.n {
	    if {[lsearch {bitmap image set} $lref] >= 0} {
		return $ref
	    }
	}
	regexp.n {
	    if {[lsearch {string} $lref] >= 0} {
		return $ref
	    }
	}
	source.n {
	    if {[lsearch {text} $lref] >= 0} {
		return $ref
	    }
	}
	history.n {
	    if {[lsearch {exec} $lref] >= 0} {
		return $ref
	    }
	}
	return.n {
	    if {[lsearch {error continue break} $lref] >= 0} {
		return $ref
	    }
	}
	scrollbar.n {
	    if {[lsearch {set} $lref] >= 0} {
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
    if { ! [info exists offsets]} {
	return $text
    }
    ##
    ## sort the offsets
    ##
    set offsets [lsort -integer $offsets]
    ##
    ## see which we want to use
    ##
    switch -exact $invert([lindex $offsets 0]) {
	anchor {
	    if {$offset(end-anchor) < 0} { return [reference-error {Missing end anchor} $text]; }
	    set head [string range $text 0 $offset(end-anchor)]
	    set tail [string range $text [expr $offset(end-anchor)+1] end]
	    return $head[insert-cross-references $tail]
	}
	quote {
	    if {$offset(end-quote) < 0} { return [reference-error {Missing end quote} $text]; }
	    if {"$invert([lindex $offsets 1])" == {tk}} { set offsets [lreplace $offsets 1 1]; }
	    if {"$invert([lindex $offsets 1])" == {tcl}} { set offsets [lreplace $offsets 1 1]; }
	    switch -exact $invert([lindex $offsets 1]) {
		end-quote {
		    set head [string range $text 0 [expr $offset(quote)-1]]
		    set body [string range $text [expr $offset(quote)+2] [expr $offset(end-quote)-1]]
		    set tail [string range $text [expr $offset(end-quote)+2] end]
		    return $head``[cross-reference $body]''[insert-cross-references $tail]
		}
		bold -
		anchor {
		    set head [string range $text 0 [expr $offset(end-quote)+1]]
		    set tail [string range $text [expr $offset(end-quote)+2] end]
		    return $head[insert-cross-references $tail]
		}
	    }
	    return [reference-error {Uncaught quote case} $text]
	}
	bold {
	    if {$offset(end-bold) < 0} { return $text; }
	    if {"$invert([lindex $offsets 1])" == {tk}} { set offsets [lreplace $offsets 1 1]; }
	    if {"$invert([lindex $offsets 1])" == {tcl}} { set offsets [lreplace $offsets 1 1]; }
	    switch -exact $invert([lindex $offsets 1]) {
		end-bold {
		    set head [string range $text 0 [expr $offset(bold)-1]]
		    set body [string range $text [expr $offset(bold)+3] [expr $offset(end-bold)-1]]
		    set tail [string range $text [expr $offset(end-bold)+4] end]
		    return $head<B>[cross-reference $body]</B>[insert-cross-references $tail]
		}
		anchor {
		    set head [string range $text 0 [expr $offset(end-bold)+3]]
		    set tail [string range $text [expr $offset(end-bold)+4] end]
		    return $head[insert-cross-references $tail]
		}
	    }
	    return [reference-error {Uncaught bold case} $text]
	}
	tk {
	    set head [string range $text 0 [expr $offset(tk)-1]]
	    set tail [string range $text $offset(tk) end]
	    if { ! [regexp {^(Tk_[a-zA-Z0-9_]+)(.*)$} $tail all body tail]} { return [reference-error {Tk regexp failed} $text]; }
	    return $head[cross-reference $body][insert-cross-references $tail]
	}
	tcl {
	    set head [string range $text 0 [expr $offset(tcl)-1]]
	    set tail [string range $text $offset(tcl) end]
	    if { ! [regexp {^(Tcl_[a-zA-Z0-9_]+)(.*)$} $tail all body tail]} { return [reference-error {Tcl regexp failed} $text]; }
	    return $head[cross-reference $body][insert-cross-references $tail]
	}
	Tcl1 -
	Tcl2 {
	    set off [lindex $offsets 0]
	    set head [string range $text 0 [expr $off-1]]
	    set body Tcl
	    set tail [string range $text [expr $off+3] end]
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
    switch -exact $code {
	.BS -
	.BE {
	    # man-puts <HR>
	}
	.SH {
	    # drain any open lists
	    # announce the subject
	    set manual(section) $rest
	    # start our own stack of stuff
	    set manual($manual(name)-$manual(section)) {}
	    lappend manual(has-$manual(section)) $manual(name)
	    man-puts "<H3>[long-toc $manual(section)]</H3>"
	    # some sections can simply free wheel their way through the text
	    # some sections can be processed in their own loops
	    switch -exact $manual(section) {
		NAME {
		    if {[lsearch {CrtImgType.3 CrtItemType.3 CrtPhImgFmt.3} $manual(tail)] >= 0} {
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
			if {[next-op-is .nf rest]
			 || [next-op-is .br rest]
			 || [next-op-is .fi rest]} {
			    continue
			}
			if {[next-op-is .SH rest]
		         || [next-op-is .BE rest]
			 || [next-op-is .SO rest]} {
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
			} else {
			    foreach more [split $more \n] {
				man-puts $more<BR>
				if {[lsearch {TclLib TkLib} $manual(wing-file)] < 0} {
				    lappend manual(section-toc) <DD>$more
				}
			    }
			}
		    }
		    lappend manual(section-toc) </DL>
		    return
		}
		{SEE ALSO} {
		    while {[more-text]} {
			if {[next-op-is .SH rest]} {
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
			    if { ! [regexp {^<B>.*</B>$} $cr]} {
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
			if {[next-op-is .SH rest]} {
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
		output-IP-list .SH .IP $rest
		return
	    }
	    if {[next-op-is .PP rest]} {
		return
	    }
	    return
	}
	.SO {
	    if {[match-text @stuff .SE]} {
		output-directive {.SH STANDARD OPTIONS}
		set opts {}
		foreach line [split $stuff \n] {
		    foreach option [split $line \t] {
			lappend opts $option
		    }
		}
		man-puts <DL>
		lappend manual(section-toc) <DL>
		foreach option [lsort $opts] {
		    man-puts "<DT><B>[std-option-toc $option]</B>"
		}
		man-puts </DL>
		lappend manual(section-toc) </DL>
	    } else {
		manerror "unexpected .SO format:\n[expand-next-text 2]"
	    }
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
		
	    }
	    if {[match-text @stuff .DE]} {
		man-puts <PRE>$stuff</PRE>
	    } elseif {[match-text .fi @ul1 @ul2 .nf @stuff .DE]} {
		man-puts "<PRE>[lindex $ul1 1][lindex $ul2 1]\n$stuff</PRE>"
	    } else {
		manerror "unexpected .DS format:\n[expand-next-text 2]"
	    }
	    return
	}
	.CS {
	    if {[next-op-is .ta rest]} {
		
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
	    switch -exact $manual(name):$manual(section) {
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
	.UL -
	.ne {
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
    foreach copyright [concat $l1 $l2] {
	if {[regexp {^Copyright +\(c\) +([0-9]+) +(by +)?([A-Za-z].*)$} $copyright all date by who]} {
	    lappend dates($who) $date
	    continue
	}
	if {[regexp {^Copyright +\(c\) +([0-9]+)-([0-9]+) +(by +)?([A-Za-z].*)$} $copyright all from to by who]} {
	    for {set date $from} {$date <= $to} {incr date} {
		lappend dates($who) $date
	    }
	    continue
	}
	if {[regexp {^Copyright +\(c\) +([0-9]+), *([0-9]+) +(by +)?([A-Za-z].*)$} $copyright all date1 date2 by who]} {
	    lappend dates($who) $date1 $date2
	    continue
	}
	puts "oops: $copyright"
    }
    foreach who [array names dates] {
	set list [lsort $dates($who)]
	if {[llength $list] == 1 || [lindex $list 0] == [lrange $list end end]} {
	    lappend merge "Copyright (c) [lindex $list 0] $who"
	} else {
	    lappend merge "Copyright (c) [lindex $list 0]-[lrange $list end end] $who"
	}
    }
    return [lsort $merge]
}
    
proc makedirhier {dir} {
    if { ! [file isdirectory $dir]} {
	makedirhier [file dirname $dir]
	if { ! [file isdirectory $dir]} {
	    if {[catch {exec mkdir $dir} error]} {
		error "cannot create directory $dir: $error"
	    }
	}
    }
}
    
##
## foreach of the man directories specified by args
## convert manpages into hypertext in the directory
## specified by html.
##
proc make-man-pages {html args} {
    global env manual overall_title
    makedirhier $html
    if { ! [file isdirectory $html]} {
	exec mkdir $html
    }
    set manual(short-toc-n) 1
    set manual(short-toc-fp) [open $html/contents.htm w]
    puts $manual(short-toc-fp) "<HTML><HEAD><TITLE>$overall_title</TITLE></HEAD>"
    puts $manual(short-toc-fp) {<BODY BGCOLOR="#ffffff" TEXT="#000000" LINK="#0000ff" VLINK="#800000" ALINK="#800080">
<FONT FACE="Tahoma, Arial, Helvetica">}
    puts $manual(short-toc-fp) "<HR><H3>$overall_title</H3><HR><DL>"
    set manual(merge-copyrights) {}
    foreach arg $args {
	set manual(wing-glob) [lindex $arg 0]
	set manual(wing-name) [lindex $arg 1]
	set manual(wing-file) [lindex $arg 2]
	set manual(wing-description) [lindex $arg 3]
	set manual(wing-copyrights) {}
	makedirhier $html/$manual(wing-file)
	set manual(wing-toc-fp) [open $html/$manual(wing-file)/contents.htm w]
	# whistle
	puts stderr "scanning section $manual(wing-name)"
	# put the entry for this section into the short table of contents
	puts $manual(short-toc-fp) "<DT><A HREF=\"$manual(wing-file)/contents.htm\">$manual(wing-name)</A><DD>$manual(wing-description)"
	# initialize the wing table of contents
	puts $manual(wing-toc-fp) "<HTML><HEAD><TITLE>$manual(wing-name) Manual</TITLE></HEAD>"
	puts $manual(wing-toc-fp) {<BODY BGCOLOR="#ffffff" TEXT="#000000" LINK="#0000ff" VLINK="#800000" ALINK="#800080">
<FONT FACE="Tahoma, Arial, Helvetica">}
	puts $manual(wing-toc-fp) "<HR><H3>$manual(wing-name)</H3><HR>"
	# initialize the short table of contents for this section
	set manual(wing-toc) {}
	# initialize the man directory for this section
	makedirhier $html/$manual(wing-file)
	# initialize the long table of contents for this section
	set manual(long-toc-n) 1
	# get the manual pages for this section
	set manual(pages) [lsort [glob $manual(wing-glob)]]
	if {[lsearch -glob $manual(pages) */options.n] >= 0} {
	    set n [lsearch $manual(pages) */options.n]
	    set manual(pages) "[lindex $manual(pages) $n] [lreplace $manual(pages) $n $n]"
	}
	# set manual(pages) [lrange $manual(pages) 0 5]
	foreach manual(page) $manual(pages) {
	    # whistle
	    puts stderr "scanning page $manual(page)"
	    set manual(tail) [file tail $manual(page)]
	    set manual(name) [file root $manual(tail)]
	    set manual(section) {}
	    if {[lsearch {case pack-old menubar} $manual(name)] >= 0} {
		# obsolete
		manerror "discarding $manual(name)"
		continue
	    }
	    set manual(infp) [open "$manual(page)"]
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
	    manreport 100 "$manual(name)"
	    while {[gets $manual(infp) line] >= 0} {
		manreport 100 $line
		if {[regexp {^[`'][/\\]} $line]} {
		    if {[regexp {Copyright \(c\).*$} $line copyright]} {
			lappend manual(copyrights) $copyright
		    }
		    # comment
		    continue
		}
		if {"$line" == {'}} {
		    # comment
		    continue
		}
		if {[parse-directive $line code rest]} {
		    switch -exact $code {
			.ad -
			.na -
			.so -
			.ne -
			.AS -
			.VE -
			.VS -
			. {
			    # ignore
			    continue
			}
		    }
		    if {"$manual(partial-text)" != {}} {
			lappend manual(text) [process-text $manual(partial-text)]
			set manual(partial-text) {}
		    }
		    switch -exact $code {
			.SH {
			    if {[llength $rest] == 0} {
				gets $manual(infp) rest
			    }
			    lappend manual(text) ".SH [unquote $rest]"
			}
			.TH {
			    lappend manual(text) "$code [unquote $rest]"
			}
			.HS -
			.UL -
			.ta {
			    lappend manual(text) "$code [unquote $rest]"
			}
			.BS -
			.BE -
			.br -
			.fi -
			.sp -
			.nf {
			    if {"$rest" != {}} {
				manerror "unexpected argument: $line"
			    }
			    lappend manual(text) $code
			}
			.AP {
			    lappend manual(text) [concat .IP [process-text "[lindex $rest 0] \\fB[lindex $rest 1]\\fR ([lindex $rest 2])"]]
			}
			.IP {
			    regexp {^(.*) +[0-9]+$} $rest all rest
			    lappend manual(text) ".IP [process-text [unquote [string trim $rest]]]"
			}
			.TP {
			    set next [gets $manual(infp)]
			    if {"$next" != {'}} {
				lappend manual(text) ".IP [process-text $next]"
			    }
			}
			.OP {
			    lappend manual(text) [concat .OP [process-text \
								  "\\fB[lindex $rest 0]\\fR \\fB[lindex $rest 1]\\fR \\fB[lindex $rest 2]\\fR"]]
			}
			.PP {
			    lappend manual(text) {.PP}
                        }
			.LP {
			    lappend manual(text) {.LP}
			}
			.RS {
			    incr manual(.RS)
			    lappend manual(text) $code
			}
			.RE {
			    incr manual(.RS) -1
			    lappend manual(text) $code
			}
			.SO {
			    incr manual(.SO)
			    lappend manual(text) $code
			}
			.SE {
			    incr manual(.SO) -1
			    lappend manual(text) $code
			}
			.DS {
			    incr manual(.DS)
			    lappend manual(text) $code
			}
			.DE {
			    incr manual(.DS) -1
			    lappend manual(text) $code
			}
			.CS {
			    incr manual(.CS)
			    lappend manual(text) $code
			}
			.CE {
			    incr manual(.CS) -1
			    lappend manual(text) $code
			}
			.de {
			    while {[gets $manual(infp) line] >= 0} {
				if {[regexp {^\.\.} $line]} {
				    break
				}
			    }
			}
			.. {
			    error "found .. outside of .de"
			}
			default {
			    manerror "unrecognized format directive: $line"
			}
		    }
		} else {
		    if {"$manual(partial-text)" == {}} {
			set manual(partial-text) $line
		    } else {
			append manual(partial-text) \n$line
		    }
		}
	    }
	    if {"$manual(partial-text)" != {}} {
		lappend manual(text) [process-text $manual(partial-text)]
	    }
	    close $manual(infp)
	    # fixups
	    if {$manual(.RS) != 0} {
		if {"$manual(name)" != {selection}} {
		    puts "unbalanced .RS .RE"
		}
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
	    if {[next-op-is .HS rest]} {
		set manual($manual(name)-title) "[lrange $rest 1 end] [lindex $rest 0] manual page"
		while {[more-text]} {
		    set line [next-text]
		    if {[is-a-directive $line]} {
			output-directive $line
		    } else {
			man-puts $line
		    }
		}
		man-puts <HR><PRE>
		foreach copyright $manual(copyrights) {
		    man-puts "<A HREF=\"../copyright.htm\">Copyright</A> &#169; [lrange $copyright 2 end]"
		}
		man-puts "<A HREF=\"../copyright.htm\">Copyright</A> &#169; 1995-1997 Roger E. Critchlow Jr.</PRE>"
		set manual(wing-copyrights) [merge-copyrights $manual(wing-copyrights) $manual(copyrights)]
	    } elseif {[next-op-is .TH rest]} {
		set manual($manual(name)-title) "[lrange $rest 4 end] - [lindex $rest 0] manual page"
		while {[more-text]} {
		    set line [next-text]
		    if {[is-a-directive $line]} {
			output-directive $line
		    } else {
			man-puts $line
		    }
		}
		man-puts <HR><PRE>
		foreach copyright $manual(copyrights) {
		    man-puts "<A HREF=\"../copyright.htm\">Copyright</A> &#169; [lrange $copyright 2 end]"
		}
		man-puts "<A HREF=\"../copyright.htm\">Copyright</A> &#169; 1995-1997 Roger E. Critchlow Jr.</PRE>"
		set manual(wing-copyrights) [merge-copyrights $manual(wing-copyrights) $manual(copyrights)]
	    } else {
		manerror "no .HS or .TH record found"
	    }
	    #
	    # make the long table of contents for this page
	    #
	    set manual(toc-$manual(wing-file)-$manual(name)) [concat <DL> $manual(section-toc) </DL><HR>]
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
	set perline [expr 120 / $width]
	set nrows [expr ([llength $manual(wing-toc)]+$perline)/$perline]
	set n 0
        catch {unset rows}
	foreach name [lsort $manual(wing-toc)] {
	    set tail $manual(name-$name)
	    if {[llength $tail] > 1} {
		manerror "$name is defined in more than one file: $tail"
		set tail [lindex $tail [expr [llength $tail]-1]]
	    }
	    set tail [file tail $tail]
	    append rows([expr $n%$nrows]) "<td> <a href=\"$tail.htm\">$name</a>"
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
	puts $manual(wing-toc-fp) "<HR><PRE>"
	foreach copyright $manual(wing-copyrights) {
	    puts $manual(wing-toc-fp) "<A HREF=\"../copyright.htm\">Copyright</A> &#169; [lrange $copyright 2 end]"
	}
	puts $manual(wing-toc-fp) "<A HREF=\"../copyright.htm\">Copyright</A> &#169; 1995-1997 Roger E. Critchlow Jr."
	puts $manual(wing-toc-fp) "</PRE></FONT></BODY></HTML>"
	close $manual(wing-toc-fp)
	set manual(merge-copyrights) [merge-copyrights $manual(merge-copyrights) $manual(wing-copyrights)]
    }

    ##
    ## build the keyword index.
    ##
    proc strcasecmp {a b} { return [string compare -nocase $a $b] }
    set keys [lsort -command strcasecmp [array names manual keyword-*]]
    makedirhier $html/Keywords
    catch {eval exec rm -f [glob $html/Keywords/*]}
    puts $manual(short-toc-fp) {<DT><A HREF="Keywords/contents.htm">Keywords</A><DD>The keywords from the Tcl/Tk man pages.}
    set keyfp [open $html/Keywords/contents.htm w]
    puts $keyfp "<HTML><HEAD><TITLE>Tcl/Tk Keywords</TITLE></HEAD>"
    puts $keyfp {<BODY BGCOLOR="#ffffff" TEXT="#000000" LINK="#0000ff" VLINK="#800000" ALINK="#800080">
<FONT FACE="Tahoma, Arial, Helvetica">}
    puts $keyfp "<HR><H3>Tcl/Tk Keywords</H3><HR><H2>"
    foreach a {A B C D E F G H I J K L M N O P Q R S T U V W X Y Z} {
	puts $keyfp "<A HREF=\"$a.htm\">$a</A>"
	set afp [open $html/Keywords/$a.htm w]
	puts $afp "<HTML><HEAD><TITLE>Tcl/Tk Keywords - $a</TITLE></HEAD>"
	puts $afp {<BODY BGCOLOR="#ffffff" TEXT="#000000" LINK="#0000ff" VLINK="#800000" ALINK="#800080">
	<FONT FACE="Tahoma, Arial, Helvetica">}
	puts $afp "<HR><H3>Tcl/Tk Keywords - $a</H3><HR><H2>"
	foreach b {A B C D E F G H I J K L M N O P Q R S T U V W X Y Z} {
	    puts $afp "<A HREF=\"$b.htm\">$b</A>"
	}
	puts $afp "</H2><HR><DL>"
	foreach k $keys {
	    if {[regexp -nocase -- "^keyword-$a" $k]} {
		set k [string range $k 8 end]
		puts $afp "<DT><A NAME=\"$k\">$k</A><DD>"
		set refs {}
		foreach man $manual(keyword-$k) {
		    set name [lindex $man 0]
		    set file [lindex $man 1]
		    lappend refs "<A HREF=\"../$file\">$name</A>"
		}
		puts $afp [join $refs {, }]
	    }
	}
	puts $afp "</DL><HR><PRE>"
	# insert merged copyrights
	foreach copyright $manual(merge-copyrights) {
	    puts $afp "<A HREF=\"copyright.htm\">Copyright</A> &#169; [lrange $copyright 2 end]"
	}
	puts $afp "<A HREF=\"copyright.htm\">Copyright</A> &#169; 1995-1997 Roger E. Critchlow Jr."
	puts $afp "</PRE></FONT></BODY></HTML>"
	close $afp
    }
    puts $keyfp "</H2><HR><PRE>"

    # insert merged copyrights
    foreach copyright $manual(merge-copyrights) {
	puts $keyfp "<A HREF=\"copyright.htm\">Copyright</A> &#169; [lrange $copyright 2 end]"
    }
    puts $keyfp "<A HREF=\"copyright.htm\">Copyright</A> &#169; 1995-1997 Roger E. Critchlow Jr."
    puts $keyfp </PRE></FONT></BODY></HTML>
    close $keyfp

    ##
    ## finish off short table of contents
    ##
    puts $manual(short-toc-fp) {<DT><A HREF="http://www.elf.org">Source</A><DD>More information about these man pages.}
    puts $manual(short-toc-fp) "</DL><HR><PRE>"
    # insert merged copyrights
    foreach copyright $manual(merge-copyrights) {
	puts $manual(short-toc-fp) "<A HREF=\"copyright.htm\">Copyright</A> &#169; [lrange $copyright 2 end]"
    }
    puts $manual(short-toc-fp) "<A HREF=\"copyright.htm\">Copyright</A> &#169; 1995-1997 Roger E. Critchlow Jr."
    puts $manual(short-toc-fp) "</PRE></FONT></BODY></HTML>"
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
	set manual(outfp) [open $html/$manual(wing-file)/$manual(name).htm w]
	puts $manual(outfp) "<HTML><HEAD><TITLE>$manual($manual(name)-title)</TITLE></HEAD>"
	puts $manual(outfp) {
<BODY BGCOLOR="#ffffff" TEXT="#000000" LINK="#0000ff" VLINK="#800000" ALINK="#800080">
<FONT FACE="Tahoma, Arial, Helvetica">}
	if {($ntext > 60) && ($ntoc > 32) || [lsearch {
	    Hash LinkVar SetVar TraceVar ConfigWidg CrtImgType CrtItemType
	    CrtPhImgFmt DoOneEvent GetBitmap GetColor GetCursor GetDash
	    GetJustify GetPixels GetVisual ParseArgv QueueEvent
	} $manual(tail)] >= 0} {
	    foreach item $toc {
		puts $manual(outfp) $item
	    }
	}
	foreach item $text {
	    puts $manual(outfp) [insert-cross-references $item]
	}
	puts $manual(outfp) {</FONT></BODY></HTML>}
	close $manual(outfp)
    }
    return {}
}

set usercmddesc {The interpreters which implement Tcl and Tk.}
set tclcmddesc {The commands which the <B>tclsh</B> interpreter implements.}
set tkcmddesc {The additional commands which the <B>wish</B> interpreter implements.}
set tixcmddesc {The additional commands which the <B>Tix</B> extension implements.}
set tcllibdesc {The C functions which a Tcl extended C program may use.}
set tklibdesc {The additional C functions which a Tk extended C program may use.}
		
parse_command_line

proc addfile {file} {
    global testfiles
    if {![info exists testfiles]} {
        set testfiles $file
    } else {
        append testfiles ,$file
    }
}

addfile TixIntro.n
#-addfile compound.n
#-addfile pixmap.n
#-addfile tix.n
#-addfile tixBalloon.n
#-addfile tixButtonBox.n
#-addfile tixCheckList.n
#-addfile tixComboBox.n
#-addfile tixControl.n
#-addfile tixDestroy.n
#-addfile tixDirList.n
#-addfile tixDirSelectDialog.n
#-addfile tixDirTree.n
#-addfile tixDisplayStyle.n
#-addfile tixExFileSelectBox.n
#-addfile tixExFileSelectDialog.n
#-addfile tixFileEntry.n
#-addfile tixFileSelectBox.n
#-addfile tixFileSelectDialog.n
#-addfile tixForm.n
#-addfile tixGetBoolean.n
#-addfile tixGetInt.n
#-addfile tixGrid.n
#-addfile tixHList.n
#-addfile tixInputOnly.n
#-addfile tixLabelEntry.n
#-addfile tixLabelFrame.n
#-addfile tixListNoteBook.n
#-addfile tixMeter.n
#-addfile tixMwm.n
#-addfile tixNBFrame.n
#-addfile tixNoteBook.n
#-addfile tixOptionMenu.n
#-addfile tixPanedWindow.n
#-addfile tixPopupMenu.n
#-addfile tixScrolledHList.n
#-addfile tixScrolledListBox.n
#-addfile tixScrolledText.n
#-addfile tixScrolledWindow.n
#-addfile tixSelect.n
#-addfile tixStdButtonBox.n
#-addfile tixTList.n
#-addfile tixTree.n
#-addfile tixUtils.n

if {[info exists env(TEST_ONLY)]} {
    #
    # Only test one or a few files
    #
    if {[catch {
	make-man-pages $webdir \
	    "$tcltkdir/$tkdir/doc/{button.n,image.n,options.n} {Tk Commands} TkCmd {$tkcmddesc}" \
	    "$tcltkdir/$tixdir/man/{$testfiles} {Tix Commands} TixCmd {$tixcmddesc}" \
    } error]} {
	puts $error\n$errorInfo
    }
} elseif {[info exists env(WITH_TCL_TK)]} {
    #
    # Full distribution: all Tcl, Tk and Tix man pages
    #
    if {[catch {
	make-man-pages $webdir \
	    "$tcltkdir/{$tkdir,$tcldir,$tixdir}/doc/*.1 {Tcl/Tk Applications} UserCmd {$usercmddesc}" \
	    "$tcltkdir/$tcldir/doc/*.n {Tcl Commands} TclCmd {$tclcmddesc}" \
	    "$tcltkdir/$tkdir/doc/*.n {Tk Commands} TkCmd {$tkcmddesc}" \
	    "$tcltkdir/$tcldir/doc/*.3 {Tcl Library} TclLib {$tcllibdesc}" \
	    "$tcltkdir/$tkdir/doc/*.3 {Tk Library} TkLib {$tklibdesc}" \
	 "$tcltkdir/$tixdir/man/*.n {Tix Commands} TixCmd {$tixcmddesc}"\
    } error]} {
	puts $error\n$errorInfo
    }
} else {
    #
    # Standard distribution: all Tix man pages and select Tcl/Tk pages
    #
    if {[catch {
	make-man-pages $webdir \
         "$tcltkdir/$tkdir/doc/{options.n} {Tk Commands} TkCmd {$tkcmddesc}" \
	 "$tcltkdir/$tkdir/doc/{ConfigWidg.3} {Tk Library} TkLib {$tklibdesc}"\
	 "$tcltkdir/$tixdir/man/*.n {Tix Commands} TixCmd {$tixcmddesc}"\
         "$tcltkdir/$tixdir/man/*.1 {Applications} UserCmd {$usercmddesc}" \
    } error]} {
	puts $error\n$errorInfo
    }
}
