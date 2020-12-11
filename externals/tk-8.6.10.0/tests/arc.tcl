# This file creates a visual test for arcs.  It is part of the Tk
# visual test suite, which is invoked via the "visual" script.

catch {destroy .t}
toplevel .t
wm title .t "Visual Tests for Canvas Arcs"
wm iconname .t "Arcs"
wm geom .t +0+0
wm minsize .t 1 1

canvas .t.c -width 650 -height 600 -relief raised
pack .t.c -expand yes -fill both
button .t.quit -text Quit -command {destroy .t}
pack .t.quit -side bottom -pady 3 -ipadx 4 -ipady 2

puts "depth is [winfo depth .t]"
if {[winfo depth .t] > 1} {
    set fill1 aquamarine3
    set fill2 aquamarine3
    set fill3 IndianRed1
    set outline2 IndianRed3
} else {
    set fill1 black
    set fill2 white
    set fill3 Black
    set outline2 white
}
set outline black

.t.c create arc 20 20 220 120 -start 30 -extent 270 -outline $fill1 -width 14 \
	-style arc
.t.c create arc 260 20 460 120 -start 30 -extent 270 -fill $fill2 -width 14 \
	-style chord -outline $outline
.t.c create arc 500 20 620 160 -start 30 -extent 270 -fill {} -width 14 \
	-style chord -outline $outline -outlinestipple gray50
.t.c create arc 20 260 140 460 -start 45 -extent 90 -fill $fill2 -width 14 \
	-style pieslice -outline $outline
.t.c create arc 180 260 300 460 -start 45 -extent 90 -fill {} -width 14 \
	-style pieslice -outline $outline
.t.c create arc 340 260 460 460 -start 30 -extent 150 -fill $fill2 -width 14 \
	-style chord -outline $outline -stipple gray50 -outlinestipple gray25
.t.c create arc 500 260 620 460 -start 30 -extent 150 -fill {} -width 14 \
	-style chord -outline $outline
.t.c create arc 20 450 140 570 -start 135 -extent 270 -fill $fill1 -width 14 \
	-style pieslice -outline {}
.t.c create arc 180 450 300 570 -start 30 -extent -90 -fill $fill1 -width 14 \
	-style pieslice -outline {}
.t.c create arc 340 450 460 570 -start 320 -extent 270 -fill $fill1 -width 14 \
	-style chord -outline {}
.t.c create arc 500 450 620 570 -start 350 -extent -110 -fill $fill1 -width 14 \
	-style chord -outline {}
.t.c addtag arc withtag all
.t.c addtag circle withtag [.t.c create oval 320 200 340 220 -fill MistyRose3]

.t.c bind arc <Any-Enter> {
    set prevFill [lindex [.t.c itemconf current -fill] 4]
    set prevOutline [lindex [.t.c itemconf current -outline] 4]
    if {($prevFill != "") || ($prevOutline == "")} {
	.t.c itemconf current -fill $fill3
    }
    if {$prevOutline != ""} {
	.t.c itemconf current -outline $outline2
    }
}
.t.c bind arc <Any-Leave> {.t.c itemconf current -fill $prevFill -outline $prevOutline}

bind .t.c <1> {markarea %x %y}
bind .t.c <B1-Motion> {strokearea %x %y}

proc markarea {x y} {
    global areaX1 areaY1
    set areaX1 $x
    set areaY1 $y
}

proc strokearea {x y} {
    global areaX1 areaY1 areaX2 areaY2
    if {($areaX1 != $x) && ($areaY1 != $y)} {
	.t.c delete area
	.t.c addtag area withtag [.t.c create rect $areaX1 $areaY1 $x $y \
		-outline black]
	set areaX2 $x
	set areaY2 $y
    }
}

bind .t.c <Control-f> {
    puts stdout "Enclosed: [.t.c find enclosed $areaX1 $areaY1 $areaX2 $areaY2]"
    puts stdout "Overlapping: [.t.c find overl $areaX1 $areaY1 $areaX2 $areaY2]"
}

bind .t.c <3> {puts stdout "%x %y"}

# The code below allows the circle to be move by shift-dragging.

bind .t.c <Shift-1> {
    set curx %x
    set cury %y
}

bind .t.c <Shift-B1-Motion> {
    .t.c move circle [expr {%x-$curx}] [expr {%y-$cury}]
    set curx %x
    set cury %y
}

# The binding below flashes the closest item to the mouse.

bind .t.c <Control-c> {
    set closest [.t.c find closest %x %y]
    set oldfill [lindex [.t.c itemconf $closest -fill] 4]
    .t.c itemconf $closest -fill IndianRed1
    after 200 [list .t.c itemconfig $closest -fill $oldfill]
}

proc c {option value} {.t.c itemconf 2 $option $value}

bind .t.c a {
    set go 1
    set i 1
    while {$go} {
	if {$i >= 50} {
	    set delta -5
	}
	if {$i <= 5} {
	    set delta 5
	}
	incr i $delta
	c -start $i
	c -extent [expr {360-2*$i}]
	after 20
	update
    }
}

bind .t.c b {set go 0}

bind .t.c <Control-x> {.t.c delete current}













