# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: ArrowBtn.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates how to write a new Tix widget class.
#

# ArrowBtn.tcl --
#
#	Arrow Button: a sample Tix widget.
#
set arrow(n) [image create bitmap -data {
    #define up_width 15
    #define up_height 15
    static unsigned char up_bits[] = {
	0x80, 0x00, 0xc0, 0x01, 0xe0, 0x03, 0xf0, 0x07, 0xf8, 0x0f, 0xfc, 0x1f,
	0xfe, 0x3f, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
	0xc0, 0x01, 0xc0, 0x01, 0x00, 0x00};
}]
set arrow(w) [image create bitmap -data {
    #define left_width 15
    #define left_height 15
    static unsigned char left_bits[] = {
	0x00, 0x00, 0x40, 0x00, 0x60, 0x00, 0x70, 0x00, 0x78, 0x00, 0x7c, 0x00,
	0xfe, 0x3f, 0xff, 0x3f, 0xfe, 0x3f, 0x7c, 0x00, 0x78, 0x00, 0x70, 0x00,
	0x60, 0x00, 0x40, 0x00, 0x00, 0x00};
}]
set arrow(s) [image create bitmap -data {
    #define down_width 15
    #define down_height 15
    static unsigned char down_bits[] = {
	0x00, 0x00, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
	0xc0, 0x01, 0xc0, 0x01, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x07,
	0xe0, 0x03, 0xc0, 0x01, 0x80, 0x00};
}]
set arrow(e) [image create bitmap -data {
    #define right_width 15
    #define right_height 15
    static unsigned char right_bits[] = {
	0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x07, 0x00, 0x0f, 0x00, 0x1f,
	0xfe, 0x3f, 0xfe, 0x7f, 0xfe, 0x3f, 0x00, 0x1f, 0x00, 0x0f, 0x00, 0x07,
	0x00, 0x03, 0x00, 0x01, 0x00, 0x00};
}]

tixWidgetClass tixArrowButton {
    -classname  TixArrowButton
    -superclass tixPrimitive
    -method {
        flash invoke invert
    }
    -flag {
        -direction -state
    }
    -configspec {
        {-direction direction Direction e tixArrowButton:CheckDirection}
        {-state state State normal}
    }
    -alias {
        {-dir -direction}
    }
}

proc tixArrowButton:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec
    set data(count) 0
}

proc tixArrowButton:ConstructWidget {w} {
    upvar #0 $w data
    global arrow

    tixChainMethod $w ConstructWidget

    set data(w:button) [button $w.button -image $arrow($data(-direction))]
    pack $data(w:button) -expand yes -fill both
}

proc tixArrowButton:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    bind $data(w:button) <1> "tixArrowButton:IncrCount $w"
}

proc tixArrowButton:IncrCount {w} {
    upvar #0 $w data

    incr data(count)
}

proc tixArrowButton:CheckDirection {dir} {
    if {[lsearch {n w s e} $dir] != -1} {
        return $dir
    } else {
        error "wrong direction value \"$dir\""
    }
}

proc tixArrowButton:flash {w} {
    upvar #0 $w data

    $data(w:button) flash
}

proc tixArrowButton:invoke {w} {
    upvar #0 $w data

    $data(w:button) invoke
}

proc tixArrowButton:invert {w} {
    upvar #0 $w data

    set curDirection $data(-direction)
    case $curDirection {
        n {
            set newDirection s
        }
        s {
            set newDirection n
        }
        e {
            set newDirection w
        }
        w {
            set newDirection e
        }
    }
    $w config -direction $newDirection
}

proc tixArrowButton:config-direction {w value} {
    upvar #0 $w data
    global arrow

    $data(w:button) configure -image $arrow($value)
}

proc tixArrowButton:config-state {w value} {
    upvar #0 $w data
    global arrow

    $data(w:button) configure -state $value
}

#----------------------------------------------------------------------
#
# Instantiate several widgets of the tixArrowButton class
#
#----------------------------------------------------------------------

proc RunSample {w} {
    set top [frame $w.top -border 1 -relief raised]
    tixArrowButton $top.a -dir w
    tixArrowButton $top.b -dir e

    pack $top.a $top.b -side left -expand yes -fill both -padx 50 -pady 10

    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

# This "if" statement makes it possible to run this script file inside or
# outside of the main demo program "widget".
#
if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> "exit"
}
