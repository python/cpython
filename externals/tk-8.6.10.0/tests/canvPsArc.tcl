# This file creates a screen to exercise Postscript generation
# for bitmaps in canvases.  It is part of the Tk visual test suite,
# which is invoked via the "visual" script.

catch {destroy .t}
toplevel .t
wm title .t "Postscript Tests for Canvases"
wm iconname .t "Postscript"
wm geom .t +0+0
wm minsize .t 1 1

set c .t.c

message .t.m -text {This screen exercises the Postscript-generation abilities of Tk canvas widgets for arcs.  Click on "Print" to print the canvas to your default printer.  You can click on items in the canvas to delete them.} -width 6i
pack .t.m -side top -fill both

frame .t.bot
pack .t.bot -side bottom -fill both
button .t.bot.quit -text Quit -command {destroy .t}
button .t.bot.print -text Print -command "lpr $c"
pack .t.bot.print .t.bot.quit -side left -pady 1m -expand 1

canvas $c -width 6i -height 6i -bd 2 -relief sunken
pack $c -expand yes -fill both -padx 2m -pady 2m

$c create arc .5i .5i 2i 2i -style pieslice -start 20 -extent 90 \
    -fill black -outline {}
$c create arc 2.5i 0 4.5i 1i -style pieslice -start -45 -extent -135 \
    -fill {} -outline black -outlinestipple gray50 -width 3m
$c create arc 5.0i .5i 6.5i 2i -style pieslice -start 45 -extent 315 \
    -fill black -stipple gray25 -outline black -width 1m

$c create arc -.5i 2.5i 2.0i 3.5i -style chord -start 90 -extent 270 \
    -fill black -outline {}
$c create arc 2.5i 2i 4i 6i -style chord -start 20 -extent 140 \
    -fill black -stipple gray50 -outline black -width 2m
$c create arc 4i 2.5i 8i 4.5i -style chord -start 60 -extent 60 \
    -fill {} -outline black

$c create arc .5i 4.5i 2i 6i -style arc -start 135 -extent 315 -width 3m \
    -outline black -outlinestipple gray25
$c create arc 3.5i 4.5i 5.5i 5.5i -style arc -start 45 -extent -90 -width 1m \
    -outline black
