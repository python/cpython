# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Console.tcl,v 1.5 2008/02/27 22:17:28 hobbs Exp $
#
# Console.tcl --
#
#	This code constructs the console window for an application.
#	It can be used by non-unix systems that do not have built-in
#	support for shells.
#
#	This file was distributed as a part of Tk 4.1 by Sun
#	Microsystems, Inc. and subsequently modified by Expert
#	Interface Techonoligies and included as a part of Tix.
#
#	Some of the functions in this file have been renamed from
#	using a "tk" prefix to a "tix" prefix to avoid namespace
#	conflict with the original file.
#
# Copyright (c) 1995-1996 Sun Microsystems, Inc.
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "docs/license.tcltk" for information on usage and
# redistribution of the original file "console.tcl". These license
# terms do NOT apply to other files in the Tix distribution.
#
# See the file "license.terms" for information on usage and
# redistribution * of this file, and for a DISCLAIMER OF ALL
# WARRANTIES.

# tixConsoleInit --
# This procedure constructs and configures the console windows.
#
# Arguments:
# 	None.

foreach fun {tkTextSetCursor} {
    if {![llength [info commands $fun]]} {
	tk::unsupported::ExposePrivateCommand $fun
    }
}
unset fun

proc tixConsoleInit {} {
    global tcl_platform

    uplevel #0 set tixConsoleTextFont Courier
    uplevel #0 set tixConsoleTextSize 14

    set f [frame .f]
    set fontcb [tixComboBox $f.size -label "" -command "tixConsoleSetFont" \
	-variable tixConsoleTextFont \
	-options {
	    entry.width    15
	    listbox.height 5
	}]
    set sizecb [tixComboBox $f.font -label "" -command "tixConsoleSetFont" \
	-variable tixConsoleTextSize \
	-options {
	    entry.width    4
	    listbox.width  6
	    listbox.height 5
	}]
    pack $fontcb $sizecb -side left
    pack $f -side top -fill x -padx 2 -pady 2
    foreach font {
	"Courier New"
	"Courier"
	"Helvetica"
	"Lucida"
	"Lucida Typewriter"
	"MS LineDraw"
	"System"
	"Times Roman"
    } {
	$fontcb subwidget listbox insert end $font
    }

    for {set s 6} {$s < 25} {incr s} {
	$sizecb subwidget listbox insert end $s
    }

    bind [$fontcb subwidget entry] <Escape> "focus .console"
    bind [$sizecb subwidget entry] <Escape> "focus .console"

    text .console  -yscrollcommand ".sb set" -setgrid true \
	-highlightcolor [. cget -bg] -highlightbackground [. cget -bg]
    scrollbar .sb -command ".console yview" -highlightcolor [. cget -bg] \
	-highlightbackground [. cget -bg]
    pack .sb -side right -fill both
    pack .console -fill both -expand 1 -side left

    tixConsoleBind .console

    .console tag configure stderr -foreground red
    .console tag configure stdin -foreground blue

    focus .console
    
    wm protocol . WM_DELETE_WINDOW { wm withdraw . }
    wm title . "Console"
    flush stdout
    .console mark set output [.console index "end - 1 char"]
    tkTextSetCursor .console end
    .console mark set promptEnd insert
    .console mark gravity promptEnd left

    tixConsoleSetFont
}

proc tixConsoleSetFont {args} {
    if ![winfo exists .console] tixConsoleInit

    global tixConsoleTextFont tixConsoleTextSize

    set font  -*-$tixConsoleTextFont-medium-r-normal-*-$tixConsoleTextSize-*-*-*-*-*-*-*
    .console config -font $font
}

# tixConsoleInvoke --
# Processes the command line input.  If the command is complete it
# is evaled in the main interpreter.  Otherwise, the continuation
# prompt is added and more input may be added.
#
# Arguments:
# None.

proc tixConsoleInvoke {args} {
    if ![winfo exists .console] tixConsoleInit

    if {[.console dlineinfo insert] != {}} {
	set setend 1
    } else {
	set setend 0
    }
    set ranges [.console tag ranges input]
    set cmd ""
    if {$ranges != ""} {
	set pos 0
	while {[lindex $ranges $pos] != ""} {
	    set start [lindex $ranges $pos]
	    set end [lindex $ranges [incr pos]]
	    append cmd [.console get $start $end]
	    incr pos
	}
    }
    if {$cmd == ""} {
	tixConsolePrompt
    } elseif {[info complete $cmd]} {
	.console mark set output end
	.console tag delete input
	set err [catch {
	    set result [interp record $cmd]
	} result]

	if {$result != ""} {
	    if {$err} {
		.console insert insert "$result\n" stderr
	    } else {
		.console insert insert "$result\n"
	    }
	}
	tixConsoleHistory reset
	tixConsolePrompt
    } else {
	tixConsolePrompt partial
    }
    if {$setend} {
	.console yview -pickplace insert
    }
}

# tixConsoleHistory --
# This procedure implements command line history for the
# console.  In general is evals the history command in the
# main interpreter to obtain the history.  The global variable
# histNum is used to store the current location in the history.
#
# Arguments:
# cmd -	Which action to take: prev, next, reset.

set histNum 1
proc tixConsoleHistory {cmd} {
    if ![winfo exists .console] tixConsoleInit

    global histNum
    
    switch $cmd {
    	prev {
	    incr histNum -1
	    if {$histNum == 0} {
		set cmd {history event [expr [history nextid] -1]}
	    } else {
		set cmd "history event $histNum"
	    }
    	    if {[catch {interp eval $cmd} cmd]} {
    	    	incr histNum
    	    	return
    	    }
	    .console delete promptEnd end
    	    .console insert promptEnd $cmd {input stdin}
    	}
    	next {
	    incr histNum
	    if {$histNum == 0} {
		set cmd {history event [expr [history nextid] -1]}
	    } elseif {$histNum > 0} {
		set cmd ""
		set histNum 1
	    } else {
		set cmd "history event $histNum"
	    }
	    if {$cmd != ""} {
		catch {interp eval $cmd} cmd
	    }
	    .console delete promptEnd end
	    .console insert promptEnd $cmd {input stdin}
    	}
    	reset {
    	    set histNum 1
    	}
    }
}

# tixConsolePrompt --
# This procedure draws the prompt.  If tcl_prompt1 or tcl_prompt2
# exists in the main interpreter it will be called to generate the 
# prompt.  Otherwise, a hard coded default prompt is printed.
#
# Arguments:
# partial -	Flag to specify which prompt to print.

proc tixConsolePrompt {{partial normal}} {
    if ![winfo exists .console] tixConsoleInit

    if {$partial == "normal"} {
	set temp [.console index "end - 1 char"]
	.console mark set output end
    	if {[interp eval "info exists tcl_prompt1"]} {
    	    interp eval "eval \[set tcl_prompt1\]"
    	} else {
    	    puts -nonewline "% "
    	}
    } else {
	set temp [.console index output]
	.console mark set output end
    	if {[interp eval "info exists tcl_prompt2"]} {
    	    interp eval "eval \[set tcl_prompt2\]"
    	} else {
	    puts -nonewline "> "
    	}
    }

    flush stdout
    .console mark set output $temp
    tkTextSetCursor .console end
    .console mark set promptEnd insert
    .console mark gravity promptEnd left
}

# tixConsoleBind --
# This procedure first ensures that the default bindings for the Text
# class have been defined.  Then certain bindings are overridden for
# the class.
#
# Arguments:
# None.

proc tixConsoleBind {win} {
    if ![winfo exists .console] tixConsoleInit

    bindtags $win "$win Text . all"

    # Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
    # Otherwise, if a widget binding for one of these is defined, the
    # <KeyPress> class binding will also fire and insert the character,
    # which is wrong.  Ditto for <Escape>.

    bind $win <Alt-KeyPress> {# nothing }
    bind $win <Meta-KeyPress> {# nothing}
    bind $win <Control-KeyPress> {# nothing}
    bind $win <Escape> {# nothing}
    bind $win <KP_Enter> {# nothing}

    bind $win <Tab> {
	tixConsoleInsert %W \t
	focus %W
	break
    }
    bind $win <Return> {
	%W mark set insert {end - 1c}
	tixConsoleInsert %W "\n"
	tixConsoleInvoke
	break
    }
    bind $win <Delete> {
	if {[%W tag nextrange sel 1.0 end] != ""} {
	    %W tag remove sel sel.first promptEnd
	} else {
	    if {[%W compare insert < promptEnd]} {
		break
	    }
	}
    }
    bind $win <BackSpace> {
	if {[%W tag nextrange sel 1.0 end] != ""} {
	    %W tag remove sel sel.first promptEnd
	} else {
	    if {[%W compare insert <= promptEnd]} {
		break
	    }
	}
    }
    foreach left {Control-a Home} {
	bind $win <$left> {
	    if {[%W compare insert < promptEnd]} {
		tkTextSetCursor %W {insert linestart}
	    } else {
		tkTextSetCursor %W promptEnd
            }
	    break
	}
    }
    foreach right {Control-e End} {
	bind $win <$right> {
	    tkTextSetCursor %W {insert lineend}
	    break
	}
    }
    bind $win <Control-d> {
	if {[%W compare insert < promptEnd]} {
	    break
	}
    }
    bind $win <Control-k> {
	if {[%W compare insert < promptEnd]} {
	    %W mark set insert promptEnd
	}
    }
    bind $win <Control-t> {
	if {[%W compare insert < promptEnd]} {
	    break
	}
    }
    bind $win <Meta-d> {
	if {[%W compare insert < promptEnd]} {
	    break
	}
    }
    bind $win <Meta-BackSpace> {
	if {[%W compare insert <= promptEnd]} {
	    break
	}
    }
    bind $win <Control-h> {
	if {[%W compare insert <= promptEnd]} {
	    break
	}
    }
    foreach prev {Control-p Up} {
	bind $win <$prev> {
	    tixConsoleHistory prev
	    break
	}
    }
    foreach prev {Control-n Down} {
	bind $win <$prev> {
	    tixConsoleHistory next
	    break
	}
    }
    bind $win <Control-v> {
	if {[%W compare insert > promptEnd]} {
	    catch {
		%W insert insert [selection get -displayof %W] {input stdin}
		%W see insert
	    }
	}
	break
    }
    bind $win <Insert> {
	catch {tixConsoleInsert %W [selection get -displayof %W]}
	break
    }
    bind $win <KeyPress> {
	tixConsoleInsert %W %A
	break
    }
    foreach left {Control-b Left} {
	bind $win <$left> {
	    if {[%W compare insert == promptEnd]} {
		break
	    }
	    tkTextSetCursor %W insert-1c
	    break
	}
    }
    foreach right {Control-f Right} {
	bind $win <$right> {
	    tkTextSetCursor %W insert+1c
	    break
	}
    }
    bind $win <Control-Up> {
	%W yview scroll -1 unit
	break;
    }
    bind $win <Control-Down> {
	%W yview scroll 1 unit
	break;
    }
    bind $win <Prior> {
	%W yview scroll -1 pages
    }
    bind $win <Next> {
	%W yview scroll  1 pages
    }
    bind $win <F9> {
	eval destroy [winfo child .]
	source $tix_library/Console.tcl
    }
    foreach copy {F16 Meta-w Control-i} {
	bind $win <$copy> {
	    if {[selection own -displayof %W] == "%W"} {
		clipboard clear -displayof %W
		catch {
		    clipboard append -displayof %W [selection get -displayof %W]
		}
	    }
	    break
	}
    }
    foreach paste {F18 Control-y} {
	bind $win <$paste> {
	    catch {
	        set clip [selection get -displayof %W -selection CLIPBOARD]
		set list [split $clip \n\r]
		tixConsoleInsert %W [lindex $list 0]
		foreach x [lrange $list 1 end] {
		    %W mark set insert {end - 1c}
		    tixConsoleInsert %W "\n"
		    tixConsoleInvoke
		    tixConsoleInsert %W $x
		}
	    }
	    break
	}
    }
}

# tixConsoleInsert --
# Insert a string into a text at the point of the insertion cursor.
# If there is a selection in the text, and it covers the point of the
# insertion cursor, then delete the selection before inserting.  Insertion
# is restricted to the prompt area.
#
# Arguments:
# w -		The text window in which to insert the string
# s -		The string to insert (usually just a single character)

proc tixConsoleInsert {w s} {
    if ![winfo exists .console] tixConsoleInit

    if {[.console dlineinfo insert] != {}} {
	set setend 1
    } else {
	set setend 0
    }
    if {$s == ""} {
	return
    }
    catch {
	if {[$w compare sel.first <= insert]
		&& [$w compare sel.last >= insert]} {
	    $w tag remove sel sel.first promptEnd
	    $w delete sel.first sel.last
	}
    }
    if {[$w compare insert < promptEnd]} {
	$w mark set insert end	
    }
    $w insert insert $s {input stdin}
    if $setend {
	.console see insert
    }
}



# tixConsoleOutput --
#
# This routine is called directly by ConsolePutsCmd to cause a string
# to be displayed in the console.
#
# Arguments:
# dest -	The output tag to be used: either "stderr" or "stdout".
# string -	The string to be displayed.

proc tixConsoleOutput {dest string} {
    if ![winfo exists .console] tixConsoleInit

    if {[.console dlineinfo insert] != {}} {
	set setend 1
    } else {
	set setend 0
    }
    .console insert output $string $dest
    if $setend {
	.console see insert
    }
}

# tixConsoleExit --
#
# This routine is called by ConsoleEventProc when the main window of
# the application is destroyed.
#
# Arguments:
# None.

proc tixConsoleExit {} {
    if ![winfo exists .console] tixConsoleInit

    exit
}

# Configure the default Tk console
proc tixConsoleEvalAppend {inter} {
    global tixOption
    # A slave like the console interp has no global variables set!
    
    if {!$inter} {
	console hide

	# Change the menubar to Close the console instead of exiting
	# Your code must provide a way for the user to do a "console show"
	console eval {
	    if {[winfo exists .menubar.file]} {
		.menubar.file entryconfigure "Hide Console" \
			-underline 0 \
			-label Close \
			-command [list wm withdraw .]
		.menubar.file entryconfigure Exit -state disabled
	    }
	}
    }

    console eval ".option configure -font \{$tixOption(fixed_font)\}"

    console eval {
	if {[winfo exists .menubar.edit]} {
	    .menubar.edit add sep
	    .menubar.edit add command \
		    -accelerator 'Ctrl+l' \
		    -underline 0 \
		    -label Clear \
		    -command [list .console delete 1.0 end]
	    bind .console <Control-Key-l> [list .console delete 1.0 end]
	}
	if {![winfo exists .menubar.font]} {
	    set m .menubar.font
	    menu $m -tearoff 0
	    .menubar add cascade -menu .menubar.font \
		    -underline 0 -label Options

	    global _TixConsole
	    set font [font actual [.console cget -font]]
	    set pos [lsearch $font -family]
	    set _TixConsole(font) [lindex $font [incr pos]]
	    set pos [lsearch $font -size]
	    set _TixConsole(size) [lindex $font [incr pos]]
	    set pos [lsearch $font -weight]
	    set _TixConsole(weight) [lindex $font [incr pos]]

	    set allowed {System Fixedsys Terminal {MS Serif} 
	    {MS Sans Serif} Courier {Lucida Console} Tahoma 
	    Arial {Courier New} {Times New Roman} 
	    {Arial Black} Verdana  Garamond  {Arial Narrow}}
	    .menubar.font add cascade -label Font -menu $m.font
	    menu $m.font -tearoff 0
	    foreach font [lsort [font families]] {
		if {[lsearch $allowed $font] < 0} {continue}
		$m.font add radiobutton -label $font \
			-variable _TixConsole(font) \
			-value $font \
			-command \
			".console configure -font \"\{$font\} \$_TixConsole(size) \$_TixConsole(weight)\""
	    }

	    .menubar.font add cascade -label Size -menu $m.size
	    menu $m.size -tearoff 0
	    foreach size {8 9 10 12 14 16 18} {
		$m.size add radiobutton -label $size \
			-variable _TixConsole(size) \
			-value $size \
			-command \
			".console configure -font \"\{\$_TixConsole(font)\} $size \$_TixConsole(weight)\""
	    }

	    .menubar.font add cascade -label Weight -menu $m.weight
	    menu $m.weight -tearoff 0
	    foreach weight {normal bold} {
		$m.weight add radiobutton -label [string totit $weight] \
			-variable _TixConsole(weight) \
			-value $weight \
			-command \
			".console configure -font \"\{\$_TixConsole(font)\} \$_TixConsole(size) $weight\""
	    }

	}
    }
}
