# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
# tixDemo --
#
# 	This is a demo program of all the available Tix widgets. If
#	have installed Tix properly, you can execute this program
#	by changing to this directory and executing
#	the following in csh
#
#		% env TIX_LIBRARY=../library tixwish tixwidgets.tcl
#
#	Or this in sh
#
#		$ TIX_LIBRARY=../library tixwish tixwidgets.tcl
#
#----------------------------------------------------------------------
#
#	This file has not been properly documented. It is NOT intended
#	to be used as an introductory demo program about Tix
#	programming. For such demos, please see the files in the
#	demos/samples directory or go to the "Samples" page in the
#	"widget demo"
#
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

package require Tix
tix initstyle

tk appname "TixDemo#[pid]"

proc tixDemo:MkMainWindow {w} {
    global demo auto_path demo_dir

    # add this directory to the auto_path
    lappend auto_path $demo_dir
    tix addbitmapdir [file join $demo_dir bitmaps]

    toplevel $w
    wm title $w "Tix Widget Demonstration"
    wm geometry $w 830x566+100+100

    set demo(balloon) [tixBalloon .demos_balloon]

    set menu   [tixDemo:MkMainMenu     $w]
    set frame2 [tixDemo:MkMainNoteBook $w]
    set frame3 [tixDemo:MkMainStatus   $w]

    $w configure -menu $menu

    pack $frame3 -side bottom -fill x
    pack $frame2 -side top    -expand yes -fill both -padx 4 -pady 4

    $demo(balloon) config -statusbar $demo(statusbar)
    set demo(notebook) $frame2
}

proc tixDemo:MkMainMenu {top} {
    global useBallons

    set m [menu $top.menu -tearoff 0]
    $m add cascade -label "File" -menu $m.file -underline 0
    $m add cascade -label "Help" -menu $m.help -underline 0

    menu $m.file -tearoff 0
    $m.file add command -label "Open ... " -command tixDemo:FileOpen \
	-underline 1 -accelerator "Ctrl+O"
    $m.file add sep
    $m.file add command -label "Exit     " -command tixDemo:Exit \
	-underline 1 -accelerator "Ctrl+X"

    menu $m.help -tearoff 0
    $m.help add checkbutton -under 0  -label "Balloon Help " \
	-variable useBallons -onvalue 1 -offvalue 0

    trace variable useBallons w tixDemo:BalloonHelp

    set useBallons 1

    return $m
}

# Create the main display area of the widget programm. This area should
# utilize the "tixNoteBook" widget once it is available. But now
# we use the cheap substitute "tixStackWindow"
#
proc tixDemo:MkMainNoteBook {top} {
    global demo
    set hasGL 0

    option add *TixNoteBook.tagPadX 6
    option add *TixNoteBook.tagPadY 4
    option add *TixNoteBook.borderWidth 2

    set w [tixNoteBook $top.f2 -ipadx 5 -ipady 5]

    $w add wel -createcmd [list tixDemo:CreatePage tixDemo:MkWelcome  $w wel] \
	-label "Welcome" -under 0
    $w add cho -createcmd [list tixDemo:CreatePage MkChoosers $w cho] \
	-label "Choosers" -under 0
    $w add scr -createcmd [list tixDemo:CreatePage MkScroll   $w scr] \
	-label "Scrolled Widgets" -under 0
# There currently is no MkManag.tcl that this expects ?!? - JH
#    $w add mgr -createcmd [list tixDemo:CreatePage MkManager  $w mgr] \
#	-label "Manager Widgets" -under 0
    $w add dir -createcmd [list tixDemo:CreatePage MkDirList  $w dir] \
	-label "Directory List" -under 0
    $w add exp -createcmd [list tixDemo:CreatePage MkSample   $w exp] \
	-label "Run Sample Programs" -under 0

    if {$hasGL} {
	$w add glw -createcmd [list MkGL $w glw] -tag "GL Widgets"
    }

    return $w
}

proc tixDemo:CreatePage {command w name} {
    tixBusy $w on
    set code [catch {$command $w $name} err]
    tixBusy $w off
    return -code $code $err
}

proc tixDemo:MkMainStatus {top} {
    global demo demo_dir

    set w [frame $top.f3 -relief raised -bd 1]
    set demo(statusbar) \
	[label $w.status -relief sunken -bd 1]

    tixForm $demo(statusbar) -padx 3 -pady 3 -left 0 -right %70
    $w.status configure -text [file native $demo_dir]
    return $w
}

proc tixDemo:Status {msg} {
    global demo

    $demo(statusbar) configure -text $msg
}


proc tixDemo:MkWelcome {nb page} {
    set w [$nb subwidget $page]

    set bar  [tixDemo:MkWelcomeBar  $w]
    set text [tixDemo:MkWelcomeText $w]

    pack $bar  -side top -fill x -padx 2 -pady 2
    pack $text -side top -fill both -expand yes
}

proc tixDemo:MkWelcomeBar {top} {
    global demo

    set w [frame $top.bar -bd 2 -relief groove]

    # Create and configure comboBox 1
    #
    tixComboBox $w.cbx1 -command [list tixDemo:MainTextFont $top] \
	-options {
	    entry.width    15
	    listbox.height 3
	}
    tixComboBox $w.cbx2 -command [list tixDemo:MainTextFont $top] \
	-options {
	    entry.width 4
	    listbox.height 3
	}
    set demo(welfont) $w.cbx1
    set demo(welsize) $w.cbx2
    
    $w.cbx1 insert end "Courier"
    $w.cbx1 insert end "Helvetica"
    $w.cbx1 insert end "Lucida"
    $w.cbx1 insert end "Times Roman"

    $w.cbx2 insert end 8
    $w.cbx2 insert end 10
    $w.cbx2 insert end 12
    $w.cbx2 insert end 14
    $w.cbx2 insert end 18

    $w.cbx1 pick 1
    $w.cbx2 pick 3

    # Pack the comboboxes together
    #
    pack $w.cbx1 $w.cbx2 -side left -padx 4 -pady 4

    $demo(balloon) bind $w.cbx1\
	-msg "Choose\na font" -statusmsg "Choose a font for this page"
    $demo(balloon) bind $w.cbx2\
	-msg "Point size" -statusmsg "Choose the font size for this page"


    tixDoWhenIdle tixDemo:MainTextFont $top
    return $w
}

proc tixDemo:MkWelcomeText {top} {
    global demo tix_version

    set w [tixScrolledWindow $top.f3 -scrollbar auto]
    set win [$w subwidget window]

    label $win.title -font [list times -18 bold] \
	-bd 0 -width 30 -anchor n\
	-text "Welcome to TIX version $tix_version"

    message $win.msg -font [list helvetica -14 bold] \
	-bd 0 -width 400 -anchor n\
	-text "\
Tix $tix_version is a library of mega-widgets based on TK. This program \
demonstrates the widgets in the Tix widget. You can choose the pages \
in this window to look at the corresponding widgets. \
To quit this program, choose the \"File | Exit\" command."


    pack $win.title -expand yes -fill both -padx 10 -pady 10
    pack $win.msg -expand yes -fill both -padx 10 -pady 10
    set demo(welmsg) $win.msg 
    return $w
}

proc tixDemo:MainTextFont {w args} {
    global demo

    if {![info exists demo(welmsg)]} {
	return
    }

    set font  [$demo(welfont) cget -value]
    set point [$demo(welsize) cget -value]

    case $font {
	"Courier" {
	    set f courier
	}
	"Helvetica" {
	    set f helvetica
	}
	"Lucida" {
	    set f lucida
	}
	default {
	    set f times
	}
    }

    set xfont [list $f -$point]
    if [catch {$demo(welmsg) config -font $xfont} err] {
	puts \a$err
    }
}

proc tixDemo:FileOpen {} {
    global demo demo_dir
    set filedlg [tix filedialog tixExFileSelectDialog]
    if {![info exists demo(filedialog)]} {
	$filedlg subwidget fsbox config -pattern *.tcl
	$filedlg subwidget fsbox config -directory [file join $demo_dir samples]
	$filedlg config -command tixDemo:FileOpen:Doit
	set demo(filedialog) $filedlg
    }
    $filedlg config -title "Open Tix Sample Programs"
    wm transient $filedlg ""
    wm deiconify $filedlg
    after idle raise $filedlg
    $filedlg popup
    tixPushGrab $filedlg
}

proc tixDemo:FileOpen:Doit {filename} {
    global demo
  
    tixPopGrab
    LoadFile $filename
    $demo(filedialog) popdown
}

#----------------------------------------------------------------------
# Balloon Help
#----------------------------------------------------------------------
proc tixDemo:BalloonHelp {args} {
    global demo useBallons

    if {$useBallons} {
	$demo(balloon) config -state "both"
    } else {
	$demo(balloon) config -state "none"
    }
}

#----------------------------------------------------------------------
# Self-testing
#
#	The following code are called by the Tix test suite. It opens
#	every page in the demo program.
#----------------------------------------------------------------------
proc tixDemo:SelfTest {} {
    global demo testConfig

    if ![info exists testConfig] {
	return
    }

    tixDemo:MkMainWindow .widget

    update
    foreach p [$demo(notebook) pages] {
	$demo(notebook) raise $p
	update
    }

    destroy .widget
}

proc tixDemo:Exit {} {
    destroy .widget
}

#----------------------------------------------------------------------
# Start!
#----------------------------------------------------------------------

if {![info exists testConfig]} {
    #
    # If the testConfig variable exists, we are driven by the regression
    # test. In that case, don't open the main window. The test program will
    # call Widget:SelfTest
    #
    set kids [winfo children .]
    wm withdraw .
    set ::demo_dir [file normalize [file dirname [info script]]]
    tixDemo:MkMainWindow .widget
    wm transient .widget ""
    if {[llength $kids] < 1} {bind .widget <Destroy> "exit"}
}
