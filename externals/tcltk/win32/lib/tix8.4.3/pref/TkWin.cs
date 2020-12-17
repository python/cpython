#
# $Id: TkWin.cs,v 1.1 2000/10/12 01:41:04 idiscovery Exp $
#
proc tixSetScheme-Color {} {
    global tixOption

    set tixOption(bg)           SystemButtonFace
    set tixOption(fg)           SystemButtonText

    set tixOption(dark1_bg)     SystemScrollbar
    set tixOption(dark1_fg)     SystemButtonText
#     set tixOption(dark2_bg)     SystemDisabledText
#     set tixOption(dark2_fg)     black
    set tixOption(inactive_bg)  SystemButtonFace
    set tixOption(inactive_fg)  SystemButtonText

    set tixOption(light1_bg)    SystemButtonFace
#     set tixOption(light1_fg)    white
#     set tixOption(light2_bg)    #fcfcfc
#     set tixOption(light2_fg)    white

    set tixOption(active_bg)    $tixOption(dark1_bg)
    set tixOption(active_fg)    $tixOption(fg)
    set tixOption(disabled_fg)  SystemDisabledText

    set tixOption(input1_bg)    SystemWindow
#     set tixOption(input2_bg)    
#     set tixOption(output1_bg)   $tixOption(dark1_bg)
#     set tixOption(output2_bg)   $tixOption(bg)

    set tixOption(select_fg)    SystemHighlightText
    set tixOption(select_bg)    SystemHighlight

    set tixOption(selector)	SystemHighlight
}

proc tixSetScheme-Mono {} {
    global tixOption

    set tixOption(bg)           SystemButtonFace
    set tixOption(fg)           SystemButtonText

    set tixOption(dark1_bg)     SystemScrollbar
    set tixOption(dark1_fg)     SystemButtonText
#     set tixOption(dark2_bg)     SystemDisabledText
#     set tixOption(dark2_fg)     black
    set tixOption(inactive_bg)  SystemButtonFace
    set tixOption(inactive_fg)  SystemButtonText

    set tixOption(light1_bg)    SystemButtonFace
#     set tixOption(light1_fg)    white
#     set tixOption(light2_bg)    #fcfcfc
#     set tixOption(light2_fg)    white

    set tixOption(active_bg)    $tixOption(dark1_bg)
    set tixOption(active_fg)    $tixOption(fg)
    set tixOption(disabled_fg)  SystemDisabledText

    set tixOption(input1_bg)    white
#     set tixOption(input2_bg)    
#     set tixOption(output1_bg)   $tixOption(dark1_bg)
#     set tixOption(output2_bg)   $tixOption(bg)

    set tixOption(select_fg)    SystemHighlightText
    set tixOption(select_bg)    SystemHighlight

    set tixOption(selector)	SystemHighlight
}
