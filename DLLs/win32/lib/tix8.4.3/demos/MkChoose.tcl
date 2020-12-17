# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: MkChoose.tcl,v 1.4 2004/03/28 02:44:56 hobbs Exp $
#
# MkChoose.tcl --
#
#	This file implements the "Choosers" page in the widget demo
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

proc MkChoosers {nb page} {
    set w [$nb subwidget $page]

    set name [tixOptionName $w]
    option add *$name*TixLabelFrame*label.padX 4

    tixLabelFrame $w.til -label "Chooser Widgets"
    tixLabelFrame $w.cbx -label "tixComboBox"
    tixLabelFrame $w.ctl -label "tixControl"
    tixLabelFrame $w.sel -label "tixSelect"
    tixLabelFrame $w.opt -label "tixOptionMenu"
    tixLabelFrame $w.fil -label "tixFileEntry"
    tixLabelFrame $w.fbx -label "tixFileSelectBox"
    tixLabelFrame $w.tbr -label "Tool Bar"

    MkTitle   [$w.til subwidget frame]
    MkCombo   [$w.cbx subwidget frame]
    MkControl [$w.ctl subwidget frame]
    MkSelect  [$w.sel subwidget frame]
    MkOptMenu [$w.opt subwidget frame]
    MkFileBox [$w.fbx subwidget frame]
    MkFileEnt [$w.fil subwidget frame]
    MkToolBar [$w.tbr subwidget frame]
    
    #
    # First column: comBox and selector
    tixForm $w.cbx -top 0 -left 0 -right %33
    tixForm $w.sel -left 0 -right &$w.cbx -top $w.cbx
    tixForm $w.opt -left 0 -right &$w.cbx -top $w.sel -bottom -1

    #
    # Second column: title .. etc
    tixForm $w.til -left $w.cbx -right %66 -top 0
    tixForm $w.ctl -left $w.cbx -right &$w.til -top $w.til
    tixForm $w.fil -left $w.cbx -right &$w.til -top $w.ctl
    tixForm $w.tbr -left $w.cbx -right &$w.til -top $w.fil -bottom -1

    #
    # Third column: file selection
    tixForm $w.fbx -left %66  -right -1 -top 0
}

#----------------------------------------------------------------------
# 	ComboBox
#----------------------------------------------------------------------
proc MkCombo {w} {
    set name [tixOptionName $w]
    option add *$name*TixComboBox*label.width 10
    option add *$name*TixComboBox*label.anchor e
    option add *$name*TixComboBox*entry.width 14
    
    tixComboBox $w.static   -label "Static" \
	-editable false 
    tixComboBox $w.editable -label "Editable" \
	-editable true
    tixComboBox $w.history  -label "History" \
	-editable true -history true -anchor e
 
    $w.static insert end January
    $w.static insert end February
    $w.static insert end March
    $w.static insert end April
    $w.static insert end May
    $w.static insert end June
    $w.static insert end July
    $w.static insert end August
    $w.static insert end September
    $w.static insert end October
    $w.static insert end November
    $w.static insert end December

    $w.editable insert end "America"
    $w.editable insert end "Britain"
    $w.editable insert end "China"
    $w.editable insert end "Denmark"
    $w.editable insert end "Egypt"

    $w.history insert end "/usr/bin/mail"
    $w.history insert end "/etc/profile"
    $w.history insert end "/home/d/doe/Mail/letter"

    pack $w.static $w.editable $w.history -side top -padx 5 -pady 3
}

#----------------------------------------------------------------------
# 			The Control widgets
#----------------------------------------------------------------------
set states {Alabama "New York" Pennsylvania Washington}

proc stCmd {w by value} {
    global states

    set index [lsearch $states $value]
    set len   [llength $states]
    set index [expr {$index + $by}]

    if {$index < 0} {
	set index [expr {$len -1}]
    }
    if {$index >= $len} {
	set index 0
    }

    return [lindex $states $index]
}

proc stValidate {w value} {
    global states

    if {[lsearch $states $value] == -1} {
	return [lindex $states 0]
    } else {
	return $value
    }
}

proc MkControl {w} {
    set name [tixOptionName $w]
    option add *$name*TixControl*label.width 10
    option add *$name*TixControl*label.anchor e
    option add *$name*TixControl*entry.width 13


    tixControl $w.simple -label Numbers

    tixControl $w.spintext -label States \
	-incrcmd [list stCmd $w.spintext 1] \
	-decrcmd [list stCmd $w.spintext -1] \
	-validatecmd [list stValidate .d] \
	-value "Alabama"

    pack $w.simple $w.spintext -side top -padx 5 -pady 3
}

#----------------------------------------------------------------------
# 			The Select Widgets
#----------------------------------------------------------------------
proc MkSelect {w} {
    set name [tixOptionName $w]
    option add *$name*TixSelect*label.anchor c
    option add *$name*TixSelect*orientation vertical
    option add *$name*TixSelect*labelSide top

    tixSelect $w.sel1 -label "Mere Mortals" -allowzero true -radio true
    tixSelect $w.sel2 -label "Geeks" -allowzero true -radio false

    $w.sel1 add eat   -text Eat
    $w.sel1 add work  -text Work
    $w.sel1 add play  -text Play
    $w.sel1 add party -text Party
    $w.sel1 add sleep -text Sleep

    $w.sel2 add eat   -text Eat
    $w.sel2 add prog1 -text Program
    $w.sel2 add prog2 -text Program
    $w.sel2 add prog3 -text Program
    $w.sel2 add sleep -text Sleep

    pack $w.sel1 $w.sel2 -side left -padx 5 -pady 3 -fill x
}
#----------------------------------------------------------------------
# 			The OptMenu Widget
#----------------------------------------------------------------------
proc MkOptMenu {w} {
    set name [tixOptionName $w]

    option add *$name*TixOptionMenu*label.anchor e

    tixOptionMenu $w.menu -label "File Format : " \
	-options {
	    menubutton.width 15
	}

    $w.menu add command text   -label "Plain Text"      
    $w.menu add command post   -label "PostScript"      
    $w.menu add command format -label "Formatted Text"  
    $w.menu add command html   -label "HTML"            
    $w.menu add separator sep
    $w.menu add command tex    -label "LaTeX"           
    $w.menu add command rtf    -label "Rich Text Format"

    pack $w.menu -padx 5 -pady 3 -fill x
}

#----------------------------------------------------------------------
# 	FileEntry
#----------------------------------------------------------------------
proc MkFileEnt {w} {
    set name [tixOptionName $w]

    message $w.msg \
	-relief flat -width 240 -anchor n\
	-text {Press the "open file" icon button and a\
TixFileSelectDialog will popup.}

    tixFileEntry $w.ent -label "Select a file : "

    pack $w.msg -side top -expand yes -fill both -padx 3 -pady 3
    pack $w.ent -side top -fill x -padx 3 -pady 3
}

proc MkFileBox {w} {
    set name [tixOptionName $w]

    message $w.msg \
	-relief flat -width 240 -anchor n\
	-text {The TixFileSelectBox is Motif-style file selection\
box with various enhancements. For example, you can adjust the\
size of the two listboxes and your past selections are recorded.}

    tixFileSelectBox $w.box

    pack $w.msg -side top -expand yes -fill both -padx 3 -pady 3
    pack $w.box -side top -fill x -padx 3 -pady 3
}

#----------------------------------------------------------------------
# 	Tool Bar
#----------------------------------------------------------------------
proc MkToolBar {w} {
    set name [tixOptionName $w]

    option add $name*TixSelect*frame.borderWidth 1
    message $w.msg -relief flat -width 240 -anchor n\
	-text {The Select widget is also good for arranging buttons\
		   in a tool bar.}

    frame $w.bar -bd 2 -relief raised
    tixSelect $w.font -allowzero true  -radio false -label {}
    tixSelect $w.para -allowzero false -radio true -label {}

    $w.font add bold      -bitmap [tix getbitmap bold]
    $w.font add italic    -bitmap [tix getbitmap italic]
    $w.font add underline -bitmap [tix getbitmap underlin]
    $w.font add capital   -bitmap [tix getbitmap capital]

    $w.para add left    -bitmap [tix getbitmap leftj]
    $w.para add right   -bitmap [tix getbitmap rightj]
    $w.para add center  -bitmap [tix getbitmap centerj]
    $w.para add justify -bitmap [tix getbitmap justify]

    pack $w.msg -side top -expand yes -fill both -padx 3 -pady 3
    pack $w.bar -side top -fill x -padx 3 -pady 3
    pack $w.para $w.font -in $w.bar -side left -padx 4 -pady 3
}
#----------------------------------------------------------------------
# 	Title
#----------------------------------------------------------------------
proc MkTitle {w} {
    set name [tixOptionName $w]

    option add $name*TixSelect*frame.borderWidth 1
    message $w.msg \
	-relief flat -width 240 -anchor n\
	-text {There are many types of "choose" widgets that allow\
		   the user to input different type of information.}

    pack $w.msg -side top -expand yes -fill both -padx 3 -pady 3
}
