# This file creates a visual test for bevels drawn around text in text
# widgets.  It is part of the Tk visual test suite, which is invoked
# via the "visual" script.

catch {destroy .t}
toplevel .t
wm title .t "Visual Tests for Borders in Text Widgets"
wm iconname .t "Text Borders"
wm geom .t +0+0

text .t.t -width 60 -height 30 -setgrid true -xscrollcommand {.t.h set} \
	-font {Courier 12} \
	-yscrollcommand {.t.v set} -wrap none -relief raised -bd 2
scrollbar .t.v -orient vertical -command ".t.t yview"
scrollbar .t.h -orient horizontal -command ".t.t xview"
button .t.quit -text Quit -command {destroy .t}
pack .t.quit -side bottom -pady 3 -ipadx 4 -ipady 2
pack .t.h -side bottom -fill x
pack .t.v -side right -fill y
pack .t.t -expand yes -fill both
wm minsize .t 1 1

if {[winfo depth .t] > 1} {
    .t.t tag configure r1 -relief raised -borderwidth 2 -background #b2dfee
    .t.t tag configure r2 -relief raised -borderwidth 2 -background #b2dfee \
	    -offset 2
    .t.t tag configure s1 -relief sunken -borderwidth 2 -background #b2dfee
} else {
    .t.t tag configure r1 -relief raised -borderwidth 2 -background white
    .t.t tag configure r2 -relief raised -borderwidth 2 -background white \
	    -offset 2
    .t.t tag configure s1 -relief sunken -borderwidth 2 -background white
}
.t.t tag configure indent1 -lmargin1 100
.t.t tag configure indent2 -lmargin1 200

.t.t insert end {This display contains a bunch of raised and sunken
regions to exercise the bevel-drawing facilities of
DisplayLineBackground.  The letters have the following
significance:

r - should appear raised
u - should appear raised and also slightly offset vertically
s - should appear sunken
S - should appear solid
n - preceding relief should extend right to end of line.
* - should appear "normal"
x - extra long lines to allow horizontal scrolling.

Try scrolling the text both vertically and horizontally to
be sure that the bevels are still drawn correctly.

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

Pass 1 (side bevels):

}
.t.t insert end ****
.t.t insert end rrrrrrr r1
.t.t insert end uuuu r2
.t.t insert end ************
.t.t insert end ssssssssssssssssss s1
.t.t insert end \n\n****************
.t.t insert end rrrrrrrrrrrrrrn\n r1

.t.t insert end "\nPass 2 (top bevels):\n\n"
.t.t insert end rrrrrrrrrrrrrr r1
.t.t insert end rrrrr {r1 dummy}
.t.t insert end rrrrrrrrrrrrrrrrrrr r1
.t.t insert end \n************
.t.t insert end rrrrrrrrrrrrrrrrr r1
.t.t insert end ***********\n
.t.t insert end rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr r1
.t.t insert end \n\n***
.t.t insert end rrrrrrrrrrrrrrrrrrr r1
.t.t insert end ***********\n*
.t.t insert end rrrrrrrrr r1
.t.t insert end ********
.t.t insert end rrrrrrrrrrrrrrrrrrrrrrrrr r1
.t.t insert end \n\n*
.t.t insert end *** dummy
.t.t insert end rrrrrrrrrrrrrrrrrrrrrrrrr r1
.t.t insert end n\nrrrrrrrrrrrrrrr {r1 indent1}
.t.t insert end \n\n***
.t.t insert end rrr r1
.t.t insert end \n
.t.t insert end rrrr {r1 indent1}

.t.t insert end \n\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n
.t.t insert end "Pass 3 (bottom bevels):\n\n"
.t.t insert end *******
.t.t insert end ********** dummy
.t.t insert end rrrrrrrrrrrrrrrr r1
.t.t insert end **********\n
.t.t insert end rrrrrrrrr r1
.t.t insert end uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu r2
.t.t insert end \n********************
.t.t insert end rrrrrrrrrrrrrrr r1
.t.t insert end ************\n\n*
.t.t insert end rrrrrrrrrrrr r1
.t.t insert end ********
.t.t insert end rrrrrrrrrrrrrrrrrrrrrrrrr r1
.t.t insert end \n*****
.t.t insert end rrrrrrrrrrrrrrrrrrrr r1
.t.t insert end **********\n\n
.t.t insert end rrrrrrrrrrrrrrr {r1 indent1}
.t.t insert end \n** dummy
.t.t insert end **
.t.t insert end rrrrrrrrrrrrrrrrrrrrn\n r1
.t.t insert end \n
.t.t insert end rrrr {r1 indent1}
.t.t insert end \n***
.t.t insert end rrr r1

.t.t insert end \n\nMiscellaneous:\n\n
.t.t insert end rrr r1
.t.t insert end *****
.t.t insert end rrr r1
foreach i {1 2 3} {
    .t.t insert end \n
    .t.t insert end ***
    .t.t insert end rrrrr r1
}
.t.t insert end \n
.t.t insert end rrr r1
.t.t insert end *****
.t.t insert end rrr r1

font configure TkFixedFont -size 20
.t.t tag configure sol100 -relief solid -borderwidth 100 \
                          -foreground red -font TkFixedFont
.t.t tag configure sol12 -relief solid -borderwidth 12 \
                          -foreground red -font TkFixedFont
.t.t tag configure big -font TkFixedFont
set ind [.t.t index end]

.t.t insert end "\n\nBorders do not leak on the neighbour chars"
.t.t insert end "\nOnly \"S\" is on dark background"
.t.t insert end {
 xxx
 x} {} S sol100 {x
 xxx}

.t.t insert end "\n\nA very thick border grows toward the inside of the tagged area only"
.t.t insert end "\nOnly \"S\" is on dark background"
.t.t insert end {
 xxxx} {} SSSSS sol100 {xxxx
 x} {} SSSSSSSSSSSSSSSSSS sol100 {x
 xxx} {} SSSSSSSSS sol100 xxxx {}

.t.t insert end "\n\nA thinner border is continuous"
.t.t insert end {
 xxxx} {} SSSSS sol12 {xxxx
 x} {} SSSSSSSSSSSSSSSSSS sol12 {x
 xxx} {} SSSSSSSSS sol12 xxxx {}

.t.t tag add big $ind end

