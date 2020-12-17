# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: LabWidg.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# LabWidg.tcl --
#
#	TixLabelWidget: Virtual base class. Do not instantiate
#
# 	This widget class is the base class for all widgets that has a
# 	label. Most Tix compound widgets will have a label so that
# 	the app programmer doesn't need to add labels themselvel.
#
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

# LabelSide : top, left, right, bottom, none, acrosstop
#
# public widgets:
#	frame, label
#

tixWidgetClass tixLabelWidget {
    -superclass tixPrimitive
    -classname  TixLabelWidget
    -flag {
	-label -labelside -padx -pady
    }
    -static {-labelside}
    -configspec {
	{-label label Label ""}
	{-labelside labelSide LabelSide left}
	{-padx padX Pad 0}
	{-pady padY Pad 0}
    }
}

proc tixLabelWidget:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    if {$data(-labelside) != "acrosstop"} {
	set data(w:frame) [frame $w.frame]
    } else {
	set data(pw:border) [frame $w.border]
	set data(pw:pad)    [frame $w.border.pad]
	set data(w:frame)   [frame $w.border.frame]
    }

    if {$data(-labelside) != "none"} {
	set data(w:label) [label $w.label -text $data(-label)]
    }
    tixLabelWidget:Pack $w

    tixCallMethod $w ConstructFramedWidget $data(w:frame)
}

proc tixLabelWidget:ConstructFramedWidget {w frame} {
    # Do nothing
}

proc tixLabelWidget:Pack {w} {
    upvar #0 $w data

    if {[catch {tixLabelWidget:Pack-$data(-labelside) $w}]} {
	error "unknown -labelside option \"$data(-labelside)\""
    }
}

proc tixLabelWidget:Pack-acrosstop {w} {
    upvar #0 $w data

    set labHalfHeight [expr [winfo reqheight $data(w:label)] / 2]
    set padHeight [expr $labHalfHeight - [$data(pw:border) cget -bd]]
    if {$padHeight < 0} {
	set padHeight 0
    }

    tixForm $data(w:label) -top 0 -left 4\
	-padx [expr $data(-padx) +4] -pady $data(-pady)
    tixForm $data(pw:border) -top $labHalfHeight -bottom -1 \
	-left 0 -right -1 -padx $data(-padx) -pady $data(-pady)
    tixForm $data(pw:pad) -left 0 -right -1 \
	-top 0 -bottom $padHeight
    tixForm $data(w:frame) -top $data(pw:pad) -bottom -1 \
	-left 0 -right -1
}

proc tixLabelWidget:Pack-top {w} {
    upvar #0 $w data

    pack $data(w:label) -side top \
	-padx $data(-padx) -pady $data(-pady) \
	-fill x
    pack $data(w:frame) -side top \
	-padx $data(-padx) -pady $data(-pady) \
	-expand yes -fill both
}

proc tixLabelWidget:Pack-bottom {w} {
    upvar #0 $w data

    pack $data(w:label) -side bottom \
	-padx $data(-padx) -pady $data(-pady) \
	-fill x
    pack $data(w:frame) -side bottom \
	-padx $data(-padx) -pady $data(-pady) \
	-expand yes -fill both
}

proc tixLabelWidget:Pack-left {w} {
    upvar #0 $w data

    pack $data(w:label) -side left \
	-padx $data(-padx) -pady $data(-pady) \
	-fill y
    pack $data(w:frame) -side left \
	-padx $data(-padx) -pady $data(-pady) \
	-expand yes -fill both
}

proc tixLabelWidget:Pack-right {w} {
    upvar #0 $w data

    pack $data(w:label) -side right \
	-padx $data(-padx) -pady $data(-pady) \
	-fill x
    pack $data(w:frame) -side right \
	-padx $data(-padx) -pady $data(-pady) \
	-expand yes -fill both
}

proc tixLabelWidget:Pack-none {w} {
    upvar #0 $w data

    pack $data(w:frame)\
	-padx $data(-padx) -pady $data(-pady) \
	-expand yes -fill both
}

#----------------------------------------------------------------------
#                           CONFIG OPTIONS
#----------------------------------------------------------------------
proc tixLabelWidget:config-label {w value} {
    upvar #0 $w data

    $data(w:label) config -text $value

    if {$data(-labelside) == "acrosstop"} {
	tixLabelWidget:Pack-acrosstop $w
    }
}
