#
# $Id: TkWin.fs,v 1.2 2001/12/09 05:03:09 idiscovery Exp $
#

proc tixSetFontset {} {

    global tixOption

    set tixOption(font)         "windows-message"
    set tixOption(bold_font)    "windows-status"
    set tixOption(menu_font)    "windows-menu"
    set tixOption(italic_font)  "windows-message"
    set tixOption(fixed_font)   "systemfixed"
    set tixOption(border1)      1
}

