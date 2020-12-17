# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: ComboBox.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixComboBox widget, which is close
# to the MS Window Combo Box control.
#
proc RunSample {w} {

    # Create the comboboxes on the top of the dialog box
    #
    frame $w.top -border 1 -relief raised

    # $w.top.a is a drop-down combo box. It is not editable -- who wants
    # to invent new months?
    #
    # [Hint] The -options switch sets the options of the subwidgets.
    # [Hint] We set the label.width subwidget option of both comboboxes to 
    #        be 10 so that their labels appear to be aligned.
    #
    tixComboBox $w.top.a -label "Month: " -dropdown true \
	-command cbx:select_month -editable false -variable demo_month \
	-options {
	    listbox.height 6
	    label.width 10
	    label.anchor e
	}


    # $w.top.b is a non-drop-down combo box. It is not editable: we provide
    # four choices for the user, but he can enter an alternative year if he
    # wants to.
    #
    # [Hint] Use the padY and anchor options of the label subwidget to
    #	     aligh the label with the entry subwidget.
    # [Hint] Notice that you should use padY (the NAME of the option) and not
    #        pady (the SWITCH of the option).
    #
    tixComboBox $w.top.b -label "Year: " -dropdown false \
	-command cbx:select_year -editable true -variable demo_year \
	-options {
	    listbox.height 4
	    label.padY 5
	    label.width 10
	    label.anchor ne
	}

    pack $w.top.a -side top -anchor w
    pack $w.top.b -side top -anchor w

    # Insert the choices into the combo boxes
    #
    $w.top.a insert end January
    $w.top.a insert end February
    $w.top.a insert end March
    $w.top.a insert end April
    $w.top.a insert end May
    $w.top.a insert end June
    $w.top.a insert end July
    $w.top.a insert end August
    $w.top.a insert end September
    $w.top.a insert end October
    $w.top.a insert end November
    $w.top.a insert end December

    $w.top.b insert end 1992
    $w.top.b insert end 1993
    $w.top.b insert end 1994
    $w.top.b insert end 1995

    # Use "tixSetSilent" to set the values of the combo box if you
    # don't want your -command procedures (cbx:select_month and 
    # cbx:select_year) to be called.
    #
    tixSetSilent $w.top.a January
    tixSetSilent $w.top.b 1995

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "cbx:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

proc cbx:select_year {args} {
    tixDemo:Status "you have selected \"$args\""
}

proc cbx:select_month {s} {
    tixDemo:Status "you have selected \"$s\""
}

proc cbx:okcmd {w} {
    global demo_month demo_year

    tixDemo:Status "The month selected is $demo_month of $demo_year"
    destroy $w
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
