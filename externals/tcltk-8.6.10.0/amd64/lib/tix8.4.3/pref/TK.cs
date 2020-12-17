#
# $Id: TK.cs,v 1.1.1.1 2000/05/17 11:08:47 idiscovery Exp $
#
proc tixSetScheme-Color {} {
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
