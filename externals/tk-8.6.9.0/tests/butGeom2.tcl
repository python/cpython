# This file creates a visual test for button layout.  It is part of
# the Tk visual test suite, which is invoked via the "visual" script.

catch {destroy .t}
toplevel .t
wm title .t "Visual Tests for Button Geometry"
wm iconname .t "Button Geometry"
wm geom .t +0+0
wm minsize .t 1 1

label .t.l -text {This screen exercises the color options for various flavors of buttons.  Select display options below, and they will be applied to the appropiate button widgets.} -wraplength 5i
pack .t.l -side top -fill both

button .t.quit -text Quit -command {destroy .t}
pack .t.quit -side bottom -pady 2m

set sepId 1
proc sep {} {
    global sepId
    frame .t.sep$sepId -height 2 -bd 1 -relief sunken
    pack .t.sep$sepId -side top -padx 2m -pady 2m -fill x
    incr sepId
}

# Create buttons that control configuration options.

frame .t.control
pack .t.control -side top -fill x -pady 3m
frame .t.control.left
frame .t.control.right
pack .t.control.left .t.control.right -side left -expand 1 -fill x
label .t.anchorLabel -text "Color:"
frame .t.control.left.f -width 6c -height 3c
pack .t.anchorLabel .t.control.left.f -in .t.control.left -side top -anchor w
foreach opt {activebackground activeforeground background disabledforeground foreground highlightbackground highlightcolor } {
    #button .t.color-$opt -text $opt -command "config -$opt \[tk_chooseColor]"
    menubutton .t.color-$opt -text $opt -menu .t.color-$opt.m -indicatoron 1 \
        -relief raised -bd 2
    menu .t.color-$opt.m -tearoff 0
    .t.color-$opt.m add command -label Red -command "config -$opt red"
    .t.color-$opt.m add command -label Green -command "config -$opt green"
    .t.color-$opt.m add command -label Blue -command "config -$opt blue"
    .t.color-$opt.m add command -label Other... \
          -command "config -$opt \[tk_chooseColor]"
    pack .t.color-$opt -in .t.control.left.f -fill x
}

set default disabled
label .t.default -text Default:
radiobutton .t.default-normal -text "Default normal" -relief flat \
	-command "config-but -default normal" -variable default \
	-value normal
radiobutton .t.default-active -text "Default active" -relief flat \
	-command "config-but -default active" -variable default \
	-value active
radiobutton .t.default-disabled -text "Default disabled" -relief flat \
	-command "config-but -default disabled" -variable default \
	-value disabled
pack .t.default .t.default-normal .t.default-active .t.default-disabled \
	-in .t.control.right -anchor w

sep
frame .t.f1
pack .t.f1 -side top -expand 1 -fill both
sep
frame .t.f2
pack .t.f2 -side top -expand 1 -fill both
sep
frame .t.f3
pack .t.f3 -side top -expand 1 -fill both
sep
frame .t.f4
pack .t.f4 -side top -expand 1 -fill both
sep

label .t.l1 -text Label -bd 2 -relief sunken
label .t.l2 -text "Explicit\nnewlines\n\nin the text" -bd 2 -relief sunken
label .t.l3 -text "This text is quite long, so it must be wrapped automatically by Tk" -wraplength 2i -bd 2 -relief sunken -underline 50
pack .t.l1 .t.l2 .t.l3 -in .t.f1 -side left -padx 5m -pady 3m \
	-expand y -fill both

button .t.b1 -text Button
button .t.b2 -text "Explicit\nnewlines\n\nin the text"
button .t.b3 -text "This text is quite long, so it must be wrapped automatically by Tk" -wraplength 2i -underline 50
pack .t.b1 .t.b2 .t.b3 -in .t.f2 -side left -padx 5m -pady 3m \
	-expand y -fill both

checkbutton .t.c1 -text Checkbutton -variable a
checkbutton .t.c2 -text "Explicit\nnewlines\n\nin the text" -variable b
checkbutton .t.c3 -text "This text is quite long, so it must be wrapped automatically by Tk" -wraplength 2i -variable c -underline 50
pack .t.c1 .t.c2 .t.c3 -in .t.f3 -side left -padx 5m -pady 3m \
	-expand y -fill both

radiobutton .t.r1 -text Radiobutton -value a
radiobutton .t.r2 -text "Explicit\nnewlines\n\nin the text" -value b
radiobutton .t.r3 -text "This text is quite long, so it must be wrapped automatically by Tk" -wraplength 2i -value c -underline 50
pack .t.r1 .t.r2 .t.r3 -in .t.f4 -side left -padx 5m -pady 3m \
	-expand y -fill both

proc config {option value} {
    foreach w {.t.l1 .t.l2 .t.l3 .t.b1 .t.b2 .t.b3 .t.c1 .t.c2 .t.c3
	    .t.r1 .t.r2 .t.r3} {
	catch {$w configure $option $value}
    }
}

proc config-but {option value} {
    foreach w {.t.b1 .t.b2 .t.b3} {
	$w configure $option $value
    }
}













