# This file creates a screen to exercise Postscript generation
# for images in canvases.  It is part of the Tk visual test suite,
# which is invoked via the "visual" script.

# Build a test image in a canvas
proc BuildTestImage {} {
    global BitmapImage PhotoImage visual level
    catch {destroy .t.f}
    frame .t.f -visual $visual -colormap new
    pack .t.f -side top -after .t.top
    bind .t.f <Enter> {wm colormapwindows .t {.t.f .t}}
    bind .t.f <Leave> {wm colormapwindows .t {.t .t.f}}
    canvas .t.f.c -width 550 -height 350 -borderwidth 2 -relief raised
    pack .t.f.c
    .t.f.c create rectangle 25 25 525 325 -fill {} -outline black
    .t.f.c create image 50 50 -anchor nw -image $BitmapImage
    .t.f.c create image 250 50 -anchor nw -image $PhotoImage
}

# Put postscript in a file
proc FilePostscript { canvas } {
    global level
    $canvas postscript -file /tmp/test.ps -colormode $level
}

# Send postscript output to printer
proc PrintPostcript { canvas } {
    global level
    $canvas postscript -file tmp.ps -colormode $level
    exec lpr tmp.ps
}

catch {destroy .t}
toplevel .t
wm title .t "Postscript Tests for Canvases: Images"
wm iconname .t "Postscript"

message .t.m -text {This screen exercises the Postscript-generation abilities of Tk canvas widgets for images.  Click the buttons below to select a Visual type for the canvas and colormode for the Postscript output.  Then click "Print" to send the results to the default printer, or "Print to file" to put the Postscript output in a file called "/tmp/test.ps".  You can also click on items in the canvas to delete them.
NOTE: Some Postscript printers may not be able to handle Postscript generated in color mode.} -width 6i
pack .t.m -side top -fill both

frame .t.top
pack .t.top -side top
frame .t.top.l -relief raised -borderwidth 2
frame .t.top.r -relief raised -borderwidth 2
pack .t.top.l .t.top.r -side left -fill both -expand 1

label .t.visuals -text "Visuals"
pack .t.visuals -in .t.top.l

set visual [lindex [winfo visualsavailable .] 0]
foreach v [winfo visualsavailable .] {
    # The hack below is necessary for some systems, which have more than one
    # visual of the same type...
    if {![winfo exists .t.$v]} {
        radiobutton .t.$v -text $v -variable visual -value $v \
		-command BuildTestImage
        pack .t.$v -in .t.top.l -anchor w
    }
}

label .t.levels -text "Color Levels"
pack .t.levels -in .t.top.r
set level monochrome
foreach l { monochrome gray color } {
    radiobutton .t.$l -text $l -variable level -value $l
    pack .t.$l -in .t.top.r -anchor w
}

set BitmapImage [image create bitmap \
	-file [file join [file dirname [info script]] face.xbm] \
	-background white -foreground black]
set PhotoImage [image create photo \
	-file [file join [file dirname [info script]] teapot.ppm]]

BuildTestImage

frame .t.bot
pack .t.bot -side top -fill x -expand 1

button .t.file -text "Print to File" -command { FilePostscript .t.f.c }
button .t.print -text "Print" -command { PrintPostscript .t.f.c }
button .t.quit -text "Quit" -command { destroy .t }
pack .t.file .t.print .t.quit -in .t.bot -side left -fill x -expand 1
