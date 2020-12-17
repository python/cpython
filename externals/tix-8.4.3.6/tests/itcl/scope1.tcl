# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: scope1.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
proc About {} {
    return "Testing creation of Tix widgets inside ITCL classes"
}

proc Test {} {
    class foo {
        inherit itk::Widget

        constructor {args} {
	    itk_component add lab {
		label $itk_interior.lab \
		    -textvariable [code choice($this)]
	    }

	    itk_component add le {
		tixOptionMenu $itk_interior.le \
		    -label "File format" \
		    -variable [code choice($this)] \
		    -command "$this foocmd"
	    }

	    foreach cmd {HTML PostScript ASCII} {
		$itk_component(le) add command $cmd
	    }

	    pack $itk_component(lab) $itk_component(le) \
		-anchor e \
		-padx 10 \
		-pady 10 \
		-fill x

	    eval itk_initialize $args
        }
        common choice

	method foocmd {args} {
	    puts $args
	}
        method set_format {format} {
	    set choice($this) $format
        }
    }
    usual TixOptionMenu {
    }

    foo .xy
    pack .xy
    .xy set_format ASCII
    update
    .xy component le config -value  PostScript
    update
    .xy component le config -value  HTML
}

