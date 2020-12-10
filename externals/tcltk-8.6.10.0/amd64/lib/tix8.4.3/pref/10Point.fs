#
# $Id: 10Point.fs,v 1.2 2002/01/24 09:17:02 idiscovery Exp $
#

proc tixSetFontset {} {
    global tixOption tcl_platform

    switch -- $tcl_platform(platform) "windows" {
	# This should be Tahoma for Win2000/XP
	set font "MS Sans Sherif"
	set fixedfont "Courier New"
    } unix {
	set font "helvetica"
	set fixedfont "courier"
    }

    set tixOption(font)         [list $font -10]
    set tixOption(bold_font)    [list $font -10 bold]
    set tixOption(menu_font)    [list $font -10]
    set tixOption(italic_font)  [list $font -10 bold italic]
    set tixOption(fixed_font)   [list $fixedfont -10]
    set tixOption(border1)      1
}
