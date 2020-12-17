#
# $Id: Bisque.cs,v 1.1.1.1 2000/05/17 11:08:47 idiscovery Exp $
#
proc tixSetScheme-Color {} {
    global tixOption

    set tixOption(bg)           bisque1
    set tixOption(fg)           black

    set tixOption(dark1_bg)     bisque2
    set tixOption(dark1_fg)     black
    set tixOption(dark2_bg)     bisque3
    set tixOption(dark2_fg)     black
    set tixOption(inactive_bg)  bisque3
    set tixOption(inactive_fg)  black

    set tixOption(light1_bg)    bisque1
    set tixOption(light1_fg)    white
    set tixOption(light2_bg)    bisque1
    set tixOption(light2_fg)    white

    set tixOption(active_bg)    $tixOption(dark1_bg)
    set tixOption(active_fg)    $tixOption(fg)
    set tixOption(disabled_fg)  gray55

    set tixOption(input1_bg)    bisque2
    set tixOption(input2_bg)    bisque2
    set tixOption(output1_bg)   $tixOption(dark1_bg)
    set tixOption(output2_bg)   $tixOption(bg)

    set tixOption(select_fg)    black
    set tixOption(select_bg)    bisque2

    set tixOption(selector)	#b03060
}
