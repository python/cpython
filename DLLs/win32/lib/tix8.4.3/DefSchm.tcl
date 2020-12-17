# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DefSchm.tcl,v 1.2 2001/12/09 05:04:02 idiscovery Exp $
#
# DefSchm.tcl --
#
#	Implements the default color and font schemes for Tix.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc tixSetDefaultFontset {} {
    global tixOption tcl_platform

    switch -- $tcl_platform(platform) "windows" {
	# This should be Tahoma for Win2000/XP
	set font "MS Sans Serif"
	set fixedfont "Courier New"
	set size 8
     } unix {
	set font "helvetica"
	set fixedfont "courier"
	set size -12
    }

    set tixOption(font)         [list $font $size]
    set tixOption(bold_font)    [list $font $size bold]
    set tixOption(menu_font)    [list $font $size]
    set tixOption(italic_font)  [list $font $size bold italic]
    set tixOption(fixed_font)   [list $fixedfont $size]
    set tixOption(border1)      1
}

proc tixSetDefaultScheme-Color {} {
    global tixOption

    set tixOption(bg)           #d9d9d9
    set tixOption(fg)           black

    set tixOption(dark1_bg)     #c3c3c3
    set tixOption(dark1_fg)     black
    set tixOption(dark2_bg)     #a3a3a3
    set tixOption(dark2_fg)     black
    set tixOption(inactive_bg)  #a3a3a3
    set tixOption(inactive_fg)  black

    set tixOption(light1_bg)    #ececec
    set tixOption(light1_fg)    white
    set tixOption(light2_bg)    #fcfcfc
    set tixOption(light2_fg)    white

    set tixOption(active_bg)    $tixOption(dark1_bg)
    set tixOption(active_fg)    $tixOption(fg)
    set tixOption(disabled_fg)  gray55

    set tixOption(input1_bg)    #d9d9d9
    set tixOption(input2_bg)    #d9d9d9
    set tixOption(output1_bg)   $tixOption(dark1_bg)
    set tixOption(output2_bg)   $tixOption(bg)

    set tixOption(select_fg)    black
    set tixOption(select_bg)    #c3c3c3

    set tixOption(selector)	#b03060
}

proc tixSetDefaultScheme-Mono {} {

    global tixOption

    set tixOption(bg)           lightgray
    set tixOption(fg)           black

    set tixOption(dark1_bg)     gray70
    set tixOption(dark1_fg)     black
    set tixOption(dark2_bg)     gray60
    set tixOption(dark2_fg)     white
    set tixOption(inactive_bg)  lightgray
    set tixOption(inactive_fg)  black

    set tixOption(light1_bg)    gray90
    set tixOption(light1_fg)    white
    set tixOption(light2_bg)    gray95
    set tixOption(light2_fg)    white

    set tixOption(active_bg)    gray90
    set tixOption(active_fg)    $tixOption(fg)
    set tixOption(disabled_fg)  gray55

    set tixOption(input1_bg)    $tixOption(light1_bg)
    set tixOption(input2_bg)    $tixOption(light1_bg)
    set tixOption(output1_bg)   $tixOption(light1_bg)
    set tixOption(output2_bg)   $tixOption(light1_bg)

    set tixOption(select_fg)    white
    set tixOption(select_bg)    black

    set tixOption(selector)	black
}
