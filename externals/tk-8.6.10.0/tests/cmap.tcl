# This file creates a visual test for colormaps and the WM_COLORMAP_WINDOWS
# property.  It is part of the Tk visual test suite, which is invoked
# via the "visual" script.

catch {destroy .t}
toplevel .t -colormap new
wm title .t "Visual Test for Colormaps"
wm iconname .t "Colormaps"
wm geom .t +0+0

# The following procedure creates a whole bunch of frames within a
# window, in order to eat up all the colors in a colormap.

proc colors {w redInc greenInc blueInc} {
    set red 0
    set green 0
    set blue 0
    for {set y 0} {$y < 8} {incr y} {
	for {set x 0} {$x < 8} {incr x} {
	    frame $w.f$x,$y -width 40 -height 40 -bd 2 -relief raised \
		    -bg [format #%02x%02x%02x $red $green $blue]
	    place $w.f$x,$y -x [expr {40*$x}] -y [expr {40*$y}]
	    incr red $redInc
	    incr green $greenInc
	    incr blue $blueInc
	}
    }
}

message .t.m -width 6i -text {This window displays two nested frames, each with a whole bunch of subwindows that eat up a lot of colors.  The toplevel window has its own colormap, which is inherited by the outer frame.  The inner frame has its own colormap.  As you move the mouse around, the colors in the frames should change back and forth.}
pack .t.m -side top -fill x

button .t.quit -text Quit -command {destroy .t}
pack .t.quit -side bottom -pady 3 -ipadx 4 -ipady 2

frame .t.f -width 700 -height 450 -relief raised -bd 2
pack .t.f -side top -padx 1c -pady 1c
colors .t.f 4 0 0
frame .t.f.f -width 350 -height 350 -colormap new -bd 2 -relief raised
place .t.f.f -relx 1.0 -rely 0 -anchor ne
colors .t.f.f 0 4 0
bind .t.f.f <Enter> {wm colormapwindows .t {.t.f.f .t}}
bind .t.f.f <Leave> {wm colormapwindows .t {.t .t.f.f}}

catch {destroy .t2}
toplevel .t2
wm title .t2 "Visual Test for Colormaps"
wm iconname .t2 "Colormaps"
wm geom .t2 +0-0

message .t2.m -width 6i -text {This window just eats up most of the colors in the default colormap.}
pack .t2.m -side top -fill x

button .t2.quit -text Quit -command {destroy .t2}
pack .t2.quit -side bottom -pady 3 -ipadx 4 -ipady 2

frame .t2.f -height 320 -width 320
pack .t2.f -side bottom
colors .t2.f 0 0 4













