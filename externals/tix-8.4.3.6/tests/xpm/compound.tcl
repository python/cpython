# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: compound.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc About {} {
    return "the compound image type"
}

proc Test {} {
    set num 3
    # Test for create
    #
    #
    test {image create compound -foo} {missing}
    test {image create compound -window} {missing}
    test {image create compound -window foo} {path name}
    test {set image1 [image create compound -window .b]} {path name}

    for {set i 0} {$i < $num} {incr i} {
        button .b$i
        pack .b$i
    }

    # (0) Empty image
    #
    test {set image0 [image create compound -window .b0]}
    
    # (1) Simple image
    #
    test {set image1 [image create compound -window .b1]}
    
    $image1 add line
    $image1 add text -text Hello

    # (2) Two lines
    #
    test {set image2 [image create compound -window .b2]}
    
    $image2 add line
    $image2 add text -text "Line One"
    $image2 add line
    $image2 add text -text "Line Two"


    # Display them
    #
    for {set i 0} {$i < $num} {incr i} {
        .b$i config -image [set image$i]
    }
}
