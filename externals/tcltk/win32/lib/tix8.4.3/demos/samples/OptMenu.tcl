# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: OptMenu.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixOptionMenu widget -- you can
# use it for the user to choose from a fixed set of options
#
set opt_options {text formatted post html tex rtf}

set opt_labels(text)		"Plain Text" 
set opt_labels(formatted)	"Formatted Text"
set opt_labels(post)		"PostScript"
set opt_labels(html)		"HTML"
set opt_labels(tex)		"LaTeX"
set opt_labels(rtf)		"Rich Text Format"

proc RunSample {w} {
    catch {uplevel #0 unset demo_opt_from}
    catch {uplevel #0 unset demo_opt_to  }

    # Create the tixOptionMenu's on the top of the dialog box
    #
    frame $w.top -border 1 -relief raised

    tixOptionMenu $w.top.from -label "From File Format : " \
	-variable demo_opt_from \
	-options {
	    label.width  19
	    label.anchor e
	    menubutton.width 15
	}

    tixOptionMenu $w.top.to -label "To File Format : " \
	-variable demo_opt_to \
	-options {
	    label.width  19
	    label.anchor e
	    menubutton.width 15
	}

    # Add the available options to the two OptionMenu widgets
    #
    # [Hint] You have to add the options first before you set the
    #	     global variables "demo_opt_from" and "demo_opt_to". Otherwise
    #	     the OptionMenu widget will complain about "unknown options"!
    #
    global opt_options opt_labels
    foreach opt $opt_options {
	$w.top.from add command $opt -label $opt_labels($opt)
	$w.top.to   add command $opt -label $opt_labels($opt)
    }

    uplevel #0 set demo_opt_from html
    uplevel #0 set demo_opt_to   post

    pack $w.top.from $w.top.to -side top -anchor w -pady 3 -padx 6

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "opt:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes

    # Let's set some nice bindings for keyboard accelerators
    #
    bind $w <Alt-f> "focus $w.top.from" 
    bind $w <Alt-t> "focus $w.top.to" 
    bind $w <Alt-o> "[$w.box subwidget ok] invoke; break" 
    bind $w <Alt-c> "[$w.box subwidget cancel] invoke; break" 
}

proc opt:okcmd {w} {
    global demo_opt_from demo_opt_to opt_labels

    tixDemo:Status "You wanted to convert file from $opt_labels($demo_opt_from) to $opt_labels($demo_opt_to)"

    destroy $w
}


# This "if" statement makes it possible to run this script file inside or
# outside of the main demo program "widget".
#
if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> {if {"%W" == ".demo"} exit}
}
