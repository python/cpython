# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: CmpImg1.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixNoteBook widget, which allows
# you to lay out your interface using a "notebook" metaphore
#

proc RunSample {w} {

    # We use these options to set the sizes of the subwidgets inside the
    # notebook, so that they are well-aligned on the screen.
    #
    set name [tixOptionName $w]
    option add *$name*TixControl*entry.width 10
    option add *$name*TixControl*label.width 18
    option add *$name*TixControl*label.anchor e
    option add *$name*TixNoteBook*tabPadX 8

    # Create the notebook widget and set its backpagecolor to gray.
    # Note that the -backpagecolor option belongs to the "nbframe"
    # subwidget.
    tixNoteBook $w.nb -ipadx 6 -ipady 6
    $w config -bg gray
    $w.nb subwidget nbframe config -backpagecolor gray -tabpady 0

    # Create the two tabs on the notebook. The -underline option
    # puts a underline on the first character of the labels of the tabs.
    # Keyboard accelerators will be defined automatically according
    # to the underlined character.	
    #
    global network_pixmap hard_disk_pixmap
    set img0 [image create pixmap -data $network_pixmap]
    set img1 [image create pixmap -data $hard_disk_pixmap]

    set hd_img [image create compound -window [$w.nb subwidget nbframe]]
    $hd_img add line
    $hd_img add text -text "Hard Disk" -underline 0
    $hd_img add space -width 7
    $hd_img add image -image $img1
 
    $w.nb add hard_disk -image $hd_img

    set net_img [image create compound -window [$w.nb subwidget nbframe]]
    $net_img add line
    $net_img add text -text "Network" -underline 0
    $net_img add space -width 7
    $net_img add image -image $img0

    $w.nb add network  -image $net_img
    # Create the first page
    #
    set f [$w.nb subwidget hard_disk]
    
    tixControl $f.a -value 12   -label "Access Time: "
    tixControl $f.w -value 400  -label "Write Throughput: "
    tixControl $f.r -value 400  -label "Read Throughput: "
    tixControl $f.c -value 1021 -label "Capacity: "
    pack $f.a $f.w $f.r $f.c  -side top -padx 20 -pady 2
    
    # Create the second page	
    #
    set f [$w.nb subwidget network]
    
    tixControl $f.a -value 12   -label "Access Time: "
    tixControl $f.w -value 400  -label "Write Throughput: "
    tixControl $f.r -value 400  -label "Read Throughput: "
    tixControl $f.c -value 1021 -label "Capacity: "
    tixControl $f.u -value 10   -label "Users: "
    
    pack $f.a $f.w $f.r $f.c $f.u -side top -padx 20 -pady 2
    pack $w.nb -expand yes -fill both -padx 5 -pady 5

}

set network_pixmap {/* XPM */
static char * netw_xpm[] = {
/* width height ncolors chars_per_pixel */
"32 32 7 1",
/* colors */
" 	s None	c None",
".	c #000000000000",
"X	c white",
"o	c #c000c000c000",
"O	c #404040",
"+	c blue",
"@	c red",
/* pixels */
"                                ",
"                 .............. ",
"                 .XXXXXXXXXXXX. ",
"                 .XooooooooooO. ",
"                 .Xo.......XoO. ",
"                 .Xo.++++o+XoO. ",
"                 .Xo.++++o+XoO. ",
"                 .Xo.++oo++XoO. ",
"                 .Xo.++++++XoO. ",
"                 .Xo.+o++++XoO. ",
"                 .Xo.++++++XoO. ",
"                 .Xo.XXXXXXXoO. ",
"                 .XooooooooooO. ",
"                 .Xo@ooo....oO. ",
" ..............  .XooooooooooO. ",
" .XXXXXXXXXXXX.  .XooooooooooO. ",
" .XooooooooooO.  .OOOOOOOOOOOO. ",
" .Xo.......XoO.  .............. ",
" .Xo.++++o+XoO.        @        ",
" .Xo.++++o+XoO.        @        ",
" .Xo.++oo++XoO.        @        ",
" .Xo.++++++XoO.        @        ",
" .Xo.+o++++XoO.        @        ",
" .Xo.++++++XoO.      .....      ",
" .Xo.XXXXXXXoO.      .XXX.      ",
" .XooooooooooO.@@@@@@.X O.      ",
" .Xo@ooo....oO.      .OOO.      ",
" .XooooooooooO.      .....      ",
" .XooooooooooO.                 ",
" .OOOOOOOOOOOO.                 ",
" ..............                 ",
"                                "};}

set hard_disk_pixmap {/* XPM */
static char * drivea_xpm[] = {
/* width height ncolors chars_per_pixel */
"32 32 5 1",
/* colors */
" 	s None	c None",
".	c #000000000000",
"X	c white",
"o	c #c000c000c000",
"O	c #800080008000",
/* pixels */
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"   ..........................   ",
"   .XXXXXXXXXXXXXXXXXXXXXXXo.   ",
"   .XooooooooooooooooooooooO.   ",
"   .Xooooooooooooooooo..oooO.   ",
"   .Xooooooooooooooooo..oooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .Xoooooooo.......oooooooO.   ",
"   .Xoo...................oO.   ",
"   .Xoooooooo.......oooooooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .oOOOOOOOOOOOOOOOOOOOOOOO.   ",
"   ..........................   ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                "};}



if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
