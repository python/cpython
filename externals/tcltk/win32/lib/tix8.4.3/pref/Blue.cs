#
# $Id: Blue.cs,v 1.1.1.1 2000/05/17 11:08:47 idiscovery Exp $
#
proc tixSetScheme-Color {} {
    global tixOption

    set tixOption(bg)           #9090f0
    set tixOption(fg)           black

    set tixOption(dark1_bg)     #8080d0
    set tixOption(dark1_fg)     black
    set tixOption(dark2_bg)     #7070c0
    set tixOption(dark2_fg)     black
    set tixOption(inactive_bg)  #8080da
    set tixOption(inactive_fg)  black

    set tixOption(light1_bg)    #a8a8ff
    set tixOption(light1_fg)    black
    set tixOption(light2_bg)    #c0c0ff
    set tixOption(light2_fg)    black

    set tixOption(active_bg)    $tixOption(dark1_bg)
    set tixOption(active_fg)    $tixOption(fg)
    set tixOption(disabled_fg)  gray25

    set tixOption(input1_bg)    $tixOption(light1_bg)
    set tixOption(input2_bg)    $tixOption(bg)
    set tixOption(output1_bg)   $tixOption(light1_bg)
    set tixOption(output2_bg)   $tixOption(bg)

    set tixOption(select_fg)    white
    set tixOption(select_bg)    black

    set tixOption(selector)	yellow
}
