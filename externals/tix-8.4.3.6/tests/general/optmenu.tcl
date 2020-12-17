# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: optmenu.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc About {} {
    return "Testing Option Menu widget"
}

proc Test {} {
    tixOptionMenu .p -label "From File Format : " -command "selectproc input" \
	-disablecallback 1 \
	-options {
	    label.width 19
	    label.anchor e
	    menubutton.width 15
	}

    pack .p

    .p add command text   -label "Plain Text"
    .p add command post   -label "PostScript"      
    .p add command format -label "Formatted Text"  
    .p add command html   -label "HTML"            
    .p add separator sep
    .p add command tex    -label "LaTeX"           
    .p add command rtf    -label "Rich Text Format"

    update

    foreach ent [.p entries] {
	test {.p delete $ent}
    }

    Assert {[.p subwidget menubutton cget -text] == {}}

    test {destroy .p}

    # Testing deleting "sep" at the end
    #
    tixOptionMenu .p -label "From File Format : " -command "selectproc input" \
	-disablecallback 1 \
	-options {
	    label.width 19
	    label.anchor e
	    menubutton.width 15
	}


    pack .p

    .p add command text   -label "Plain Text"
    .p add command post   -label "PostScript"      
    .p add command format -label "Formatted Text"  
    .p add command html   -label "HTML"            
    .p add separator sep
    .p add command tex    -label "LaTeX"           
    .p add command rtf    -label "Rich Text Format"

    test {.p delete text}
    test {.p delete post}
    test {.p delete html}
    test {.p delete format}
    test {.p delete tex}
    test {.p delete rtf}
    test {.p delete sep}

    Assert {[.p subwidget menubutton cget -text] == {}}
    test {destroy .p}

    # Testing deleting "sep" as the second-last one
    #
    tixOptionMenu .p -label "From File Format : " -command "selectproc input" \
	-disablecallback 1 \
	-options {
	    label.width 19
	    label.anchor e
	    menubutton.width 15
	}


    pack .p

    .p add command text   -label "Plain Text"
    .p add command post   -label "PostScript"      
    .p add command format -label "Formatted Text"  
    .p add command html   -label "HTML"            
    .p add separator sep
    .p add command tex    -label "LaTeX"           
    .p add command rtf    -label "Rich Text Format"

    test {.p delete text}
    global .p
    Assert {[info exists .p(text,type)] == 0}
    Assert {[info exists .p(text,name)] == 0}
    Assert {[info exists .p(text,label)] == 0}
    test {.p delete post}
    test {.p delete html}
    test {.p delete format}
    test {.p delete tex}

    Assert {[.p cget -value] == "rtf"}
    test {.p delete sep}
    Assert {[.p cget -value] == "rtf"}
    test {.p delete rtf}

    Assert {[.p subwidget menubutton cget -text] == {}}

    test {destroy .p}
}
