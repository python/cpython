# This file creates a screen to exercise Postscript generation
# for some of the graphical objects in canvases.  It is part of the Tk
# visual test suite, which is invoked via the "visual" script.

catch {destroy .t}
toplevel .t
wm title .t "Postscript Tests for Canvases"
wm iconname .t "Postscript"
wm geom .t +0+0
wm minsize .t 1 1

set c .t.mid.c

message .t.m -text {This screen exercises the Postscript-generation abilities of Tk canvas widgets.  Select what you want to display with the buttons below, then click on "Print" to print it to your default printer.  You can click on items in the canvas to delete them.} -width 4i
pack .t.m -side top -fill both

frame .t.top
pack .t.top -side top -fill both
set what rect
radiobutton .t.top.rect -text Rectangles -variable what -value rect \
	-command "mkObjs $c" -relief flat
radiobutton .t.top.oval -text Ovals -variable what -value oval \
	-command "mkObjs $c" -relief flat
radiobutton .t.top.poly -text Polygons -variable what -value poly \
	-command "mkObjs $c" -relief flat
radiobutton .t.top.line -text Lines -variable what -value line \
	-command "mkObjs $c" -relief flat
pack .t.top.rect .t.top.oval .t.top.poly .t.top.line \
	-side left -pady 2m -ipadx 2m -ipady 1m -expand 1

frame .t.bot
pack .t.bot -side bottom -fill both
button .t.bot.quit -text Quit -command {destroy .t}
button .t.bot.print -text Print -command "lpr $c"
pack .t.bot.print .t.bot.quit -side left -pady 1m -expand 1

frame .t.mid -relief sunken -bd 2
pack .t.mid -side top -expand yes -fill both -padx 2m -pady 2m
canvas $c -width 400 -height 350 -bd 0 -relief sunken
pack $c -expand yes -fill both -padx 1 -pady 1

proc mkObjs c {
    global what
    $c delete all
    if {$what == "rect"} {
	$c create rect 0 0 400 350 -outline black
	$c create rect 2 2 100 50 -fill black -stipple gray25
	$c create rect -20 180 80 320 -fill black -stipple gray50 -width .5c
	$c create rect 200 -20 240 20 -fill black
	$c create rect 380 200 420 240 -fill black
	$c create rect 200 330 240 370 -fill black
    }

    if {$what == "oval"} {
	$c create oval 50 10 150 80 -fill black -stipple gray25 -outline {}
	$c create oval 100 100 200 150 -outline {} -fill black -stipple gray50
	$c create oval 250 100 400 300 -width .5c
    }

    if {$what == "poly"} {
	$c create poly 100 200 200 50 300 200 -smooth yes -stipple gray25 \
		-outline black -width 4
	$c create poly 100 300 100 250 350 250 350 300 350 300 100 300 100 300 \
		-fill red -smooth yes
	$c create poly 20 10 40 10 40 60 80 60 80 25 30 25 30 \
		35 50 35 50 45 20 45
	$c create poly 300 20 300 120 380 80 320 100 -fill blue -outline black
	$c create poly 20 200 100 220 90 100 40 250 \
		-fill {} -outline brown -width 3
    }

    if {$what == "line"} {
	$c create line 20 20 120 20 -arrow both -width 5
	$c create line 20 80 150 80 20 200 150 200 -smooth yes
	$c create line 150 20 150 150 250 150 -width .5c -smooth yes \
		-arrow both -arrowshape {.75c 1.0c .5c} -stipple gray25
	$c create line 50 340 100 250 150 340 -join round -cap round -width 10
	$c create line 200 340 250 250 300 340 -join bevel -cap project \
		-width 10
	$c create line 300 20 380 20 300 150 380 150 -join miter -cap butt \
		-width 10 -stipple gray25
    }
}

mkObjs $c













