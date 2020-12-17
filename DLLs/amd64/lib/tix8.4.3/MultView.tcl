# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: MultView.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# MultView.tcl --
#
#	Implements the multi-view widget
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#


tixWidgetClass tixMultiView {
    -classname TixMultiView
    -superclass tixPrimitive
    -method {
	add
    }
    -flag {
	-browsecmd -command -view
    }
    -forcecall {
	-view
    }
    -configspec {
	{-browsecmd browseCmd BrowseCmd ""}
	{-command command Command ""}
	{-view view View icon tixMultiView:VerifyView}
    }
    -alias {
    }

    -default {
    }
}

proc tixMultiView:InitWidgetRec {w} {
    upvar #0 $w data
    global env

    tixChainMethod $w InitWidgetRec
}

#----------------------------------------------------------------------
#		Construct widget
#----------------------------------------------------------------------
proc tixMultiView:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    set data(w:stlist) [tixScrolledTList $w.stlist]
    set data(w:sgrid)  [tixScrolledGrid $w.sgrid]
    set data(w:icon)   [tixIconView  $w.icon]

    set data(w:tlist) [$data(w:stlist) subwidget tlist]
    set data(w:grid)  [$data(w:sgrid) subwidget grid]

    $data(w:grid) config -formatcmd [list tixMultiView:GridFormat $w] \
	-leftmargin 0 -topmargin 1
}

proc tixMultiView:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings
}

proc tixMultiView:GetWid {w which} {
    upvar #0 $w data

    case $which {
	list {
	    return $data(w:stlist)
	}
	icon {
	    return $data(w:icon)
	}
	detail {
	    return $data(w:sgrid)
	}
    }
}
#----------------------------------------------------------------------
# Configuration
#----------------------------------------------------------------------
proc tixMultiView:config-view {w value} {
    upvar #0 $w data

    if {$data(-view) != ""} {
	pack forget [tixMultiView:GetWid $w $data(-view)]
    }

    pack [tixMultiView:GetWid $w $value] -expand yes -fill both
}
#----------------------------------------------------------------------
# Private methods
#----------------------------------------------------------------------
proc tixMultiView:GridFormat {w area x1 y1 x2 y2} {
    upvar #0 $w data

    case $area {
	main {
	}
	{x-margin y-margin s-margin} {
	    # cborder specifies consecutive 3d borders
	    #
	    $data(w:grid) format cborder $x1 $y1 $x2 $y2 \
		-fill 1 -relief raised -bd 2 -bg gray60 \
		-selectbackground gray80
	}
    }

}

#----------------------------------------------------------------------
# Public methods
#----------------------------------------------------------------------

# Return value is the index of "$name" in the grid subwidget
#
#
proc tixMultiView:add {w name args} {
    upvar #0 $w data

    set validOptions {-image -text}

    set opt(-image)  ""
    set opt(-text)   ""

    tixHandleOptions -nounknown opt $validOptions $args

    $data(w:icon) add $name $opt(-image) $opt(-text)
    $data(w:tlist) insert end -itemtype imagetext \
	-image $opt(-image) -text $opt(-text)
    $data(w:grid) set 0 end -itemtype imagetext \
	-image $opt(-image) -text $opt(-text)

    return max
}

#----------------------------------------------------------------------
# checker
#----------------------------------------------------------------------
proc tixMultiView:VerifyView {value} {
    case $value {
	{icon list detail} {
	    return $value
	}
    }
    error "bad view \"$value\", must be detail, icon or list"
}

