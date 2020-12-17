# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: MkScroll.tcl,v 1.3 2001/12/09 05:34:59 idiscovery Exp $
#
# MkScroll.tcl --
#
#	This file implements the "Scrolled Widgets" page in the widget demo
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

proc MkScroll {nb page} {
    set w [$nb subwidget $page]

    set name [tixOptionName $w]
    option add *$name*TixLabelFrame*label.padX 4

    tixLabelFrame $w.sls -label "tixScrolledListBox"
    tixLabelFrame $w.swn -label "tixScrolledWindow"
    tixLabelFrame $w.stx -label "tixScrolledText"
    MkSList   [$w.sls subwidget frame]

    MkSText   [$w.stx subwidget frame]
    MkSWindow [$w.swn subwidget frame]
 
    tixForm $w.sls -top 0 -left 0 -right %33 -bottom -1
    tixForm $w.swn -top 0 -left $w.sls -right %66 -bottom -1
    tixForm $w.stx -top 0 -left $w.swn -right -1 -bottom -1
}

#----------------------------------------------------------------------
# ScrolledListBox
#----------------------------------------------------------------------
proc MkSList {w} {
    frame $w.top -width 300 -height 330
    frame $w.bot

    message $w.top.msg \
	-relief flat -width 200 -anchor n\
	-text {This TixScrolledListBox is configured so that it uses\
scrollbars only when it is necessary. Use the handles to\
resize the listbox and watch the scrollbars automatically\
appear and disappear.}

    set list [tixScrolledListBox $w.top.list -scrollbar auto]
    place $list -x 50 -y 150 -width 120 -height 80
    $list subwidget listbox insert end Alabama
    $list subwidget listbox insert end California
    $list subwidget listbox insert end Montana
    $list subwidget listbox insert end "New Jersy"
    $list subwidget listbox insert end "New York"
    $list subwidget listbox insert end Pennsylvania
    $list subwidget listbox insert end Washington

    set rh [tixResizeHandle $w.top.r -relief raised \
	    -handlesize 8 -gridded true -minwidth 50 -minheight 30]

    button $w.bot.btn -text Reset -command "SList:Reset $rh $list"
    pack propagate $w.top 0
    pack $w.top.msg -fill x
    pack $w.bot.btn -anchor c 
    pack $w.top -expand yes -fill both
    pack $w.bot -fill both

    bind $list <Map> "tixDoWhenIdle $rh attachwidget $list"
}

proc SList:Reset {rh list} {
    place $list -x 50 -y 150 -width 120 -height 80
    update
    $rh attachwidget $list
}

#----------------------------------------------------------------------
# ScrolledWindow
#----------------------------------------------------------------------
proc MkSWindow {w} {
    global demo_dir
    frame $w.top -width 330 -height 330
    frame $w.bot

    message $w.top.msg \
	-relief flat -width 200 -anchor n\
	-text {The TixScrolledWindow widget allows you\
to scroll any kind of TK widget. It is more versatile\
than a scrolled canvas widget}

    set win [tixScrolledWindow $w.top.win -scrollbar auto]
    set f [$win subwidget window]
    set image [image create photo -file $demo_dir/bitmaps/tix.gif]

    label $f.b1 -image $image

    pack $f.b1 -expand yes -fill both

    place $win -x 30 -y 150 -width 190 -height 120
    set rh [tixResizeHandle $w.top.r -relief raised \
	    -handlesize 8 -gridded true -minwidth 50 -minheight 30]

    button $w.bot.btn -text Reset -command "SWindow:Reset $rh $win"
    pack propagate $w.top 0
    pack $w.top.msg -fill x
    pack $w.bot.btn -anchor c 
    pack $w.top -expand yes -fill both
    pack $w.bot -fill both

    bind $win <Map> "tixDoWhenIdle $rh attachwidget $win"
}

proc SWindow:Reset {rh win} {
    place $win -x 30 -y 150 -width 190 -height 120
    update
    $rh attachwidget $win
}

#----------------------------------------------------------------------
# ScrolledText
#----------------------------------------------------------------------
proc MkSText {w} {
    frame $w.top -width 330 -height 330
    frame $w.bot

    message $w.top.msg \
	-relief flat -width 200 -anchor n\
	-text {The TixScrolledWindow widget allows you\
to scroll any kind of TK widget. It is more versatile\
than a scrolled canvas widget}

    set win [tixScrolledText $w.top.win -scrollbar both]
    $win subwidget text config -wrap none

    place $win -x 30 -y 150 -width 190 -height 100
    set rh [tixResizeHandle $w.top.r -relief raised \
	    -handlesize 8 -gridded true -minwidth 50 -minheight 30]

    button $w.bot.btn -text Reset -command "SText:Reset $rh $win"
    pack propagate $w.top 0
    pack $w.top.msg -fill x
    pack $w.bot.btn -anchor c 
    pack $w.top -expand yes -fill both
    pack $w.bot -fill both

    bind $win <Map> "tixDoWhenIdle $rh attachwidget $win"
}

proc SText:Reset {rh win} {
    place $win -x 30 -y 150 -width 190 -height 100
    update
    $rh attachwidget $win
}
