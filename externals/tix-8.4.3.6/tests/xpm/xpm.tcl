# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: xpm.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc About {} {
    return "the pixmap image reader"
}

proc Test {} {

    set data {
/* XPM */
static char * folder_xpm[] = {
/* width height num_colors chars_per_pixel */
"16 12 4 1",
/* colors */
" 	s None	c None",
".	c black",
"X	c #f0ff80",
"+	c red",
/* pixels */
"   ....         ",
"  .XXXX.        ",
" .XXXXXX.       ",
".............   ",
".XXXXXXXXXXX.   ",
".XXXXX+XXXXX.   ",
".XXXXX+XXXXX.   ",
".XX+++++++XX.   ",
".XXXXX+XXXXX.   ",
".XXXXX+XXXXX.   ",
".XXXXXXXXXXX.   ",
".............   "};
    }

set data1 {
/* XPM */
static char * news4_xpm[] = {
/* width height ncolors chars_per_pixel */
"45 34 6 1",
/* colors */
" 	s None	c None",
".	c black",
"X	c lemon chiffon",
"o	c tan",
"O	c blue",
"+	c dark slate grey",
/* pixels */
"                                             ",
"                                             ",
"                       .                     ",
"                      .X.                    ",
"                    ..XX.                    ",
"                   .XXX.X.                   ",
"                  .XXX.XX.                   ",
"                 .XXX.XXXX.                  ",
"               ..XXX.XXX.XX.                 ",
"              .XX...XXX.o..X.                ",
"             .XX.OO.XX.oooo.X..              ",
"            .XXX..O.X.oo..oo..X..            ",
"          ..XXX.X..XX..o...oo.XXX.           ",
"         .XXXX.XXXXX.XX.oo...XXXXX.          ",
"         .XX..XXXX..XXXX.o.XXXX.XXX.         ",
"        .X.X.XXXX.XXX.XX..XXX..XXXX.         ",
"       ..X.XXXXX.XX..XXXXXXX.XXXX.XX.        ",
"       .X.X.XXX.XX.XXXX.XXX.XXXX.XXX.        ",
"        .X.X.X.XX.XXXX.XXXXXXX..XXX..        ",
"         .X.X.XX.XXX..XX.XXXX.XXX...+        ",
"        ++.X.X.XXXX.XXX.XXXX.XXX..++         ",
"       ++++.X.X.XX.XX..XXX.XXXX..++          ",
"       +++++.X.X.XXX.XXXX.XXX...++           ",
"        +++++.X.X.X.XXX..XXX..+++            ",
"         +++++.X.X.XXX.XXXX..++              ",
"          +++++.X.X.X.XXX...++               ",
"            ++++.X.X.XXX..+++                ",
"             ++++.X.X.X..++                  ",
"               +++.XX...++                   ",
"                 ++...+++                    ",
"                   ++++                      ",
"                                             ",
"                                             ",
"                                             "};
}


    # Test for create
    #
    #

    # Good pixmap
    #
    test {set pixmap1 [image create pixmap -file f-ok.xpm]}

    # With some comments 
    #
    test {set pixmap2 [image create pixmap -file f-commt.xpm]}

    # Bad color (should use "black" by default)
    #
    test {set pixmap3 [image create pixmap -file f-badcol.xpm]}

    # Shortened lines (should show garbage, shouldn't core dump)
    #
    test {set pixmap4 [image create pixmap -file f-shortln.xpm]}

    # Two chars per pixel
    #
    test {set pixmap5 [image create pixmap -file 2cpp.xpm]}

    # Bad pixel (should show garbage for undefined pixels)
    #
    test {set pixmap6 [image create pixmap -file f-badpix.xpm]}


    # Data switch
    #
    test {set pixmap7 [image create pixmap -data $data]}


    # Missing one line
    #
    test {image create pixmap -file f-missline.xpm} {File For}

    # Multi-word color names
    #
    test {set pixmap8 [image create pixmap -data $data1]}

    # Brace used as pixel value
    #
    test {set pixmap9 [image create pixmap -file brace.xpm]}

    # Many /* ... */ comments
    #
    test {set pixmap10 [image create pixmap -file brace.xpm]}

    set num 10
    for {set i 1} {$i < $num} {incr i} {
	button .b$i -image [set pixmap$i] -bg red
	pack .b$i
    }

    update

    for {set i 1} {$i < $num} {incr i} {
	destroy .b$i
	image delete [set pixmap$i]
    }

}
