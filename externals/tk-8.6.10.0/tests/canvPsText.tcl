# This file creates a screen to exercise Postscript generation
# for text in canvases.  It is part of the Tk visual test suite,
# which is invoked via the "visual" script.

catch {destroy .t}
toplevel .t
wm title .t "Postscript Tests for Canvases"
wm iconname .t "Postscript"
wm geom .t +0+0
wm minsize .t 1 1

set c .t.c

message .t.m -text {This screen exercises the Postscript-generation abilities of Tk canvas widgets for text.  Click on "Print" to print the canvas to your default printer.  The "Stipple" button can be used to turn stippling on and off for the text, but beware:  many Postscript printers cannot handle stippled text.  You can click on items in the canvas to delete them.} -width 6i
pack .t.m -side top -fill both

set stipple {}
checkbutton .t.stipple -text Stippling -variable stipple -onvalue gray50 \
	-offvalue {} -command "setStipple $c" -relief flat
pack .t.stipple -side top -pady 2m -expand 1 -anchor w

frame .t.bot
pack .t.bot -side bottom -fill both
button .t.bot.quit -text Quit -command {destroy .t}
button .t.bot.print -text Print -command "lpr $c"
pack .t.bot.print .t.bot.quit -side left -pady 1m -expand 1

canvas $c -width 6i -height 7i -bd 2 -relief sunken
pack $c -expand yes -fill both -padx 2m -pady 2m

$c create rect 2.95i 0.45i 3.05i 0.55i -fill {} -outline black
$c create text 3.0i 0.5i -text "Center Courier Oblique 24" \
	-anchor center -tags text -font {Courier 24 italic} -stipple $stipple
$c create rect 2.95i 0.95i 3.05i 1.05i -fill {} -outline black
$c create text 3.0i 1.0i -text "Northwest Helvetica 24" \
	-anchor nw -tags text -font {Helvetica 24} -stipple $stipple
$c create rect 2.95i 1.45i 3.05i 1.55i -fill {} -outline black
$c create text 3.0i 1.5i -text "North Helvetica Oblique 12 " \
	-anchor n -tags text -font {Helvetica 12 italic} -stipple $stipple
$c create rect 2.95i 1.95i 3.05i 2.05i -fill {} -outline blue
$c create text 3.0i 2.0i -text "Northeast Helvetica Bold 24" \
	-anchor ne -tags text -font {Helvetica 24 bold} -stipple $stipple
$c create rect 2.95i 2.45i 3.05i 2.55i -fill {} -outline black
$c create text 3.0i 2.5i -text "East Helvetica Bold Oblique 18" \
	-anchor e -tags text -font {Helvetica 18 {bold italic}} -stipple $stipple
$c create rect 2.95i 2.95i 3.05i 3.05i -fill {} -outline black
$c create text 3.0i 3.0i -text "Southeast Times 10" \
	-anchor se -tags text -font {Times 10} -stipple $stipple
$c create rect 2.95i 3.45i 3.05i 3.55i -fill {} -outline black
$c create text 3.0i 3.5i -text "South Times Italic 24" \
	-anchor s -tags text -font {Times 24 italic} -stipple $stipple
$c create rect 2.95i 3.95i 3.05i 4.05i -fill {} -outline black
$c create text 3.0i 4.0i -text "Southwest Times Bold 18" \
	-anchor sw -tags text -font {Times 18 bold} -stipple $stipple
$c create rect 2.95i 4.45i 3.05i 4.55i -fill {} -outline black
$c create text 3.0i 4.5i -text "West Times Bold Italic 24"\
	-anchor w -tags text -font {Times 24 {bold italic}} -stipple $stipple

$c create rect 0.95i 5.20i 1.05i 5.30i -fill {} -outline black
$c create text 1.0i 5.25i -width 1.9i -anchor c -justify left -tags text \
	-font {Times 18 bold} -stipple $stipple \
	-text "This is a sample text item to see how left justification works"
$c create rect 2.95i 5.20i 3.05i 5.30i -fill {} -outline black
$c create text 3.0i 5.25i -width 1.8i -anchor c -justify center -tags text \
	-font {Times 18 bold} -stipple $stipple \
	-text "This is a sample text item to see how center justification works"
$c create rect 4.95i 5.20i 5.05i 5.30i -fill {} -outline black
$c create text 5.0i 5.25i -width 1.8i -anchor c -justify right -tags text \
	-font {Times 18 bold} -stipple $stipple \
	-text "This is a sample text item to see how right justification works"

$c create text 3.0i 6.0i -width 5.0i -anchor n -justify right -tags text \
	-text "This text is\nright justified\nwith a line length equal to\n\
	the size of the enclosing rectangle.\nMake sure it prints right\
	justified as well."
$c create rect 0.5i 6.0i 5.5i 6.9i -fill {} -outline black

proc setStipple c {
    global stipple
    $c itemconfigure text -stipple $stipple
}













