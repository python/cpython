#!/usr/bin/env tclsh

if {[catch {package require Tcl 8.6-} msg]} {
    puts stderr "ERROR: $msg"
    puts stderr "If running this script from 'make html', set the\
	NATIVE_TCLSH environment\nvariable to point to an installed\
	tclsh8.6 (or the equivalent tclsh86.exe\non Windows)."
    exit 1
}

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
# Copyright (c) 2004-2010 Donal K. Fellows

set ::Version "50/8.6"
set ::CSSFILE "docs.css"

##
## Source the utility functions that provide most of the
## implementation of the transformation from nroff to html.
##
source [file join [file dirname [info script]] tcltk-man2html-utils.tcl]

proc parse_command_line {} {
    global argv Version

    # These variables determine where the man pages come from and where
    # the converted pages go to.
    global tcltkdir tkdir tcldir webdir build_tcl build_tk verbose

    # Set defaults based on original code.
    set tcltkdir ../..
    set tkdir {}
    set tcldir {}
    set webdir ../html
    set build_tcl 0
    set build_tk 0
    set verbose 0
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
		puts "  --verbose           whether to print longer messages"
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

	    --verbose=* {
		set verbose [string range $option \
				 [string length --verbose=] end]
	    }
	    default {
		puts stderr "tcltk-man-html: unrecognized option -- `$option'"
		exit 1
	    }
	}
    }

    if {!$build_tcl && !$build_tk} {
	set build_tcl 1;
	set build_tk 1
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

    puts "verbose messages are [expr {$verbose ? {on} : {off}}]"

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
    append overall_title " Documentation"
}

proc capitalize {string} {
    return [string toupper $string 0]
}

##
## Returns the style sheet.
##
proc css-style args {
    upvar 1 style style
    set body [uplevel 1 [list subst [lindex $args end]]]
    set tokens [join [lrange $args 0 end-1] ", "]
    append style $tokens " \{" $body "\}\n"
}
proc css-stylesheet {} {
    set hBd "1px dotted #11577b"

    css-style body div p th td li dd ul ol dl dt blockquote {
	font-family: Verdana, sans-serif;
    }
    css-style pre code {
	font-family: 'Courier New', Courier, monospace;
    }
    css-style pre {
	background-color:  #f6fcec;
	border-top:        1px solid #6A6A6A;
	border-bottom:     1px solid #6A6A6A;
	padding:           1em;
	overflow:          auto;
    }
    css-style body {
	background-color:  #FFFFFF;
	font-size:         12px;
	line-height:       1.25;
	letter-spacing:    .2px;
	padding-left:      .5em;
    }
    css-style h1 h2 h3 h4 {
	font-family:       Georgia, serif;
	padding-left:      1em;
	margin-top:        1em;
    }
    css-style h1 {
	font-size:         18px;
	color:             #11577b;
	border-bottom:     $hBd;
	margin-top:        0px;
    }
    css-style h2 {
	font-size:         14px;
	color:             #11577b;
	background-color:  #c5dce8;
	padding-left:      1em;
	border:            1px solid #6A6A6A;
    }
    css-style h3 h4 {
	color:             #1674A4;
	background-color:  #e8f2f6;
	border-bottom:     $hBd;
	border-top:        $hBd;
    }
    css-style h3 {
	font-size: 12px;
    }
    css-style h4 {
	font-size: 11px;
    }
    css-style ".keylist dt" ".arguments dt" {
	width: 20em;
	float: left;
	padding: 2px;
	border-top: 1px solid #999;
    }
    css-style ".keylist dt" { font-weight: bold; }
    css-style ".keylist dd" ".arguments dd" {
	margin-left: 20em;
	padding: 2px;
	border-top: 1px solid #999;
    }
    css-style .copy {
	background-color:  #f6fcfc;
	white-space:       pre;
	font-size:         80%;
	border-top:        1px solid #6A6A6A;
	margin-top:        2em;
    }
    css-style .tablecell {
	font-size:	   12px;
	padding-left:	   .5em;
	padding-right:	   .5em;
    }
}

##
## foreach of the man directories specified by args
## convert manpages into hypertext in the directory
## specified by html.
##
proc make-man-pages {html args} {
    global manual overall_title tcltkdesc verbose
    global excluded_pages forced_index_pages process_first_patterns

    makedirhier $html
    set cssfd [open $html/$::CSSFILE w]
    puts $cssfd [css-stylesheet]
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
	lassign $arg -> name file
	if {[regexp {(.*)(?: Package)? Commands(?:, version .*)?} $name -> pkg]} {
	    set name "$pkg Commands"
	} elseif {[regexp {(.*)(?: Package)? C API(?:, version .*)?} $name -> pkg]} {
	    set name "$pkg C API"
	}
	lappend manual(subheader) $name $file
    }

    ##
    ## parse the manpages in a section of the docs (split by
    ## package) and construct formatted manpages
    ##
    foreach arg $args {
	if {[llength $arg]} {
	    make-manpage-section $html $arg
	}
    }

    ##
    ## build the keyword index.
    ##
    if {!$verbose} {
	puts stderr "Assembling index"
    }
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
    set keyheader <H3>[join $keyheader " |\n"]</H3>
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
		if {[info exists manual(tooltip-$file)]} {
		    set tooltip $manual(tooltip-$file)
		    if {[string match {*[<>""]*} $tooltip]} {
			manerror "bad tooltip for $file: \"$tooltip\""
		    }
		    lappend refs "<A HREF=\"../$file\" TITLE=\"$tooltip\">$name</A>"
		} else {
		    lappend refs "<A HREF=\"../$file\">$name</A>"
		}
	    }
	    puts $afp "[join $refs {, }]</DD>"
	}
	puts $afp "</DL>"
	# insert merged copyrights
	puts $afp [copyout $manual(merge-copyrights)]
	puts $afp "</BODY></HTML>"
	close $afp
    }
    # insert merged copyrights
    puts $keyfp [copyout $manual(merge-copyrights)]
    puts $keyfp "</BODY></HTML>"
    close $keyfp

    ##
    ## finish off short table of contents
    ##
    puts $manual(short-toc-fp) "<DT><A HREF=\"Keywords/[indexfile]\">Keywords</A><DD>The keywords from the $tcltkdesc man pages."
    puts $manual(short-toc-fp) "</DL>"
    # insert merged copyrights
    puts $manual(short-toc-fp) [copyout $manual(merge-copyrights)]
    puts $manual(short-toc-fp) "</BODY></HTML>"
    close $manual(short-toc-fp)

    ##
    ## output man pages
    ##
    unset manual(section)
    if {!$verbose} {
	puts stderr "Rescanning [llength $manual(all-pages)] pages to build cross links and write out"
    }
    foreach path $manual(all-pages) wing_name $manual(all-page-domains) {
	set manual(wing-file) [file dirname $path]
	set manual(tail) [file tail $path]
	set manual(name) [file root $manual(tail)]
	try {
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
	    if {$verbose} {
		puts stderr "rescanning page $manual(name) $ntoc/$ntext"
	    } else {
		puts -nonewline stderr .
	    }
	    set outfd [open $html/$manual(wing-file)/$manual(name).htm w]
	    puts $outfd [htmlhead "$manual($manual(wing-file)-$manual(name)-title)" \
		    $manual(name) $wing_name "[indexfile]" \
		    $overall_title "../[indexfile]"]
	    if {($ntext > 60) && ($ntoc > 32)} {
		foreach item $toc {
		    puts $outfd $item
		}
	    } elseif {$manual(name) in $forced_index_pages} {
		if {!$verbose} {puts stderr ""}
		manerror "forcing index generation"
		foreach item $toc {
		    puts $outfd $item
		}
	    }
	    foreach item $text {
		puts $outfd [insert-cross-references $item]
	    }
	    puts $outfd "</BODY></HTML>"
	} on error msg {
	    if {$verbose} {
		puts stderr $msg
	    } else {
		puts stderr "\nError when processing $manual(name): $msg"
	    }
	} finally {
	    catch {close $outfd}
	}
    }
    if {!$verbose} {
	puts stderr "\nDone"
    }
    return {}
}

##
## Helper for assembling the descriptions of base packages (i.e., Tcl and Tk).
##
proc plus-base {var root glob name dir desc} {
    global tcltkdir
    if {$var} {
	if {[file exists $tcltkdir/$root/README]} {
	    set f [open $tcltkdir/$root/README]
	    set d [read $f]
	    close $f
	    if {[regexp {This is the \w+ (\S+) source distribution} $d -> version]} {
	       append name ", version $version"
	    }
	}
	set glob $root/$glob
	return [list $tcltkdir/$glob $name $dir $desc]
    }
}

##
## Helper for assembling the descriptions of contributed packages.
##
proc plus-pkgs {type args} {
    global build_tcl tcltkdir tcldir
    if {$type ni {n 3}} {
	error "unknown type \"$type\": must be 3 or n"
    }
    if {!$build_tcl} return
    set result {}
    set pkgsdir $tcltkdir/$tcldir/pkgs
    foreach {dir name version} $args {
	set globpat $pkgsdir/$dir/doc/*.$type
	if {![llength [glob -type f -nocomplain $globpat]]} {
	    # Fallback for manpages generated using doctools
	    set globpat $pkgsdir/$dir/doc/man/*.$type
	    if {![llength [glob -type f -nocomplain $globpat]]} {
		continue
	    }
	}
	set dir [string trimright $dir "0123456789-."]
	switch $type {
	    n {
		set title "$name Package Commands"
		if {$version ne ""} {
		    append title ", version $version"
		}
		set dir [string totitle $dir]Cmd
		set desc \
		    "The additional commands provided by the $name package."
	    }
	    3 {
		set title "$name Package C API"
		if {$version ne ""} {
		    append title ", version $version"
		}
		set dir [string totitle $dir]Lib
		set desc \
		    "The additional C functions provided by the $name package."
	    }
	}
	lappend result [list $globpat $title $dir $desc]
    }
    return $result
}

##
## Set up some special cases. It would be nice if we didn't have them,
## but we do...
##
set excluded_pages {case menubar pack-old}
set forced_index_pages {GetDash}
set process_first_patterns {*/ttk_widget.n */options.n}
set ensemble_commands {
    after array binary chan clock dde dict encoding file history info interp
    memory namespace package registry self string trace update zlib
    clipboard console font grab grid image option pack place selection tk
    tkwait ttk::style winfo wm itcl::delete itcl::find itcl::is
}
array set remap_link_target {
    stdin  Tcl_GetStdChannel
    stdout Tcl_GetStdChannel
    stderr Tcl_GetStdChannel
    style  ttk::style
    {style map} ttk::style
    {tk busy}   busy
    library     auto_execok
    safe-tcl    safe
    tclvars     env
    tcl_break   catch
    tcl_continue catch
    tcl_error   catch
    tcl_ok      catch
    tcl_return  catch
    int()       mathfunc
    wide()      mathfunc
    packagens   pkg::create
    pkgMkIndex  pkg_mkIndex
    pkg_mkIndex pkg_mkIndex
    Tcl_Obj     Tcl_NewObj
    Tcl_ObjType Tcl_RegisterObjType
    Tcl_OpenFileChannelProc Tcl_FSOpenFileChannel
    errorinfo 	env
    errorcode 	env
    tcl_pkgpath env
    Tcl_Command Tcl_CreateObjCommand
    Tcl_CmdProc Tcl_CreateObjCommand
    Tcl_CmdDeleteProc Tcl_CreateObjCommand
    Tcl_ObjCmdProc Tcl_CreateObjCommand
    Tcl_Channel Tcl_OpenFileChannel
    Tcl_WideInt Tcl_NewIntObj
    Tcl_ChannelType Tcl_CreateChannel
    Tcl_DString Tcl_DStringInit
    Tcl_Namespace Tcl_AppendExportList
    Tcl_Object  Tcl_NewObjectInstance
    Tcl_Class   Tcl_GetObjectAsClass
    Tcl_Event   Tcl_QueueEvent
    Tcl_Time	Tcl_GetTime
    Tcl_ThreadId Tcl_CreateThread
    Tk_Window	Tk_WindowId
    Tk_3DBorder Tk_Get3DBorder
    Tk_Anchor	Tk_GetAnchor
    Tk_Cursor	Tk_GetCursor
    Tk_Dash	Tk_GetDash
    Tk_Font	Tk_GetFont
    Tk_Image	Tk_GetImage
    Tk_ImageMaster Tk_GetImage
    Tk_ItemType Tk_CreateItemType
    Tk_Justify	Tk_GetJustify
    Ttk_Theme	Ttk_GetTheme
}
array set exclude_refs_map {
    bind.n		{button destroy option}
    clock.n		{next}
    history.n		{exec}
    next.n		{unknown}
    zlib.n		{binary close filename text}
    canvas.n		{bitmap text}
    console.n		{eval}
    checkbutton.n	{image}
    clipboard.n		{string}
    entry.n		{string}
    event.n		{return}
    font.n		{menu}
    getOpenFile.n	{file open text}
    grab.n		{global}
    interp.n		{time}
    menu.n		{checkbutton radiobutton}
    messageBox.n	{error info}
    options.n		{bitmap image set}
    radiobutton.n	{image}
    safe.n		{join split}
    scale.n		{label variable}
    scrollbar.n		{set}
    selection.n		{string}
    tcltest.n		{error}
    tkvars.n		{tk}
    tkwait.n		{variable}
    tm.n		{exec}
    ttk_checkbutton.n	{variable}
    ttk_combobox.n	{selection}
    ttk_entry.n		{focus variable}
    ttk_intro.n		{focus text}
    ttk_label.n		{font text}
    ttk_labelframe.n	{text}
    ttk_menubutton.n	{flush}
    ttk_notebook.n	{image text}
    ttk_progressbar.n	{variable}
    ttk_radiobutton.n	{variable}
    ttk_scale.n		{variable}
    ttk_scrollbar.n	{set}
    ttk_spinbox.n	{format}
    ttk_treeview.n	{text open}
    ttk_widget.n	{image text variable}
    TclZlib.3		{binary flush filename text}
}
array set exclude_when_followed_by_map {
    canvas.n {
	bind widget
	focus widget
	image are
	lower widget
	raise widget
    }
    selection.n {
	clipboard selection
	clipboard ;
    }
    ttk_image.n {
	image imageSpec
    }
    fontchooser.n {
	tk fontchooser
    }
}

try {
    # Parse what the user told us to do
    parse_command_line

    # Some strings depend on what options are specified
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

    apply {{} {
    global packageBuildList tcltkdir tcldir build_tcl

    # When building docs for Tcl, try to build docs for bundled packages too
    set packageBuildList {}
    if {$build_tcl} {
	set pkgsDir [file join $tcltkdir $tcldir pkgs]
	set subdirs [glob -nocomplain -types d -tails -directory $pkgsDir *]

	foreach dir [lsort $subdirs] {
	    # Parse the subdir name into (name, version) as fallback...
	    set description [split $dir -]
	    if {2 != [llength $description]} {
		regexp {([^0-9]*)(.*)} $dir -> n v
		set description [list $n $v]
	    }

	    # ... but try to extract (name, version) from subdir contents
	    try {
		try {
		    set f [open [file join $pkgsDir $dir configure.in]]
		} trap {POSIX ENOENT} {} {
		    set f [open [file join $pkgsDir $dir configure.ac]]
		}
		foreach line [split [read $f] \n] {
		    if {2 == [scan $line \
			    { AC_INIT ( [%[^]]] , [%[^]]] ) } n v]} {
			set description [list $n $v]
			break
		    }
		}
	    } finally {
		catch {close $f; unset f}
	    }

	    if {[file exists [file join $pkgsDir $dir configure]]} {
		# Looks like a package, record our best extraction attempt
		lappend packageBuildList $dir {*}$description
	    }
	}
    }

    # Get the list of packages to try, and what their human-readable names
    # are. Note that the package directory list should be version-less.
    try {
	set packageDirNameMap {}
	if {$build_tcl} {
	    set f [open $tcltkdir/$tcldir/pkgs/package.list.txt]
	    try {
		foreach line [split [read $f] \n] {
		    if {[string trim $line] eq ""} continue
		    if {[string match #* $line]} continue
		    lassign $line dir name
		    lappend packageDirNameMap $dir $name
		}
	    } finally {
		close $f
	    }
	}
    } trap {POSIX ENOENT} {} {
	set packageDirNameMap {
	    itcl {[incr Tcl]}
	    tdbc {TDBC}
	    thread Thread
	}
    }

    # Convert to human readable names, if applicable
    for {set idx 0} {$idx < [llength $packageBuildList]} {incr idx 3} {
	lassign [lrange $packageBuildList $idx $idx+2] d n v
	if {[dict exists $packageDirNameMap $n]} {
	    lset packageBuildList $idx+1 [dict get $packageDirNameMap $n]
	}
    }
    }}

    #
    # Invoke the scraper/converter engine.
    #
    make-man-pages $webdir \
	[list $tcltkdir/{$appdir}/doc/*.1 "$tcltkdesc Applications" UserCmd \
	     "The interpreters which implement $cmdesc."] \
	[plus-base $build_tcl $tcldir doc/*.n {Tcl Commands} TclCmd \
	     "The commands which the <B>tclsh</B> interpreter implements."] \
	[plus-base $build_tk $tkdir doc/*.n {Tk Commands} TkCmd \
	     "The additional commands which the <B>wish</B> interpreter implements."] \
	{*}[plus-pkgs n {*}$packageBuildList] \
	[plus-base $build_tcl $tcldir doc/*.3 {Tcl C API} TclLib \
	     "The C functions which a Tcl extended C program may use."] \
	[plus-base $build_tk $tkdir doc/*.3 {Tk C API} TkLib \
	     "The additional C functions which a Tk extended C program may use."] \
	{*}[plus-pkgs 3 {*}$packageBuildList]
} on error {msg opts} {
    # On failure make sure we show what went wrong. We're not supposed
    # to get here though; it represents a bug in the script.
    puts $msg\n[dict get $opts -errorinfo]
    exit 1
}

# Local-Variables:
# mode: tcl
# End:
