# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: ChkList.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This program demonstrates the use of the tixCheckList widget.
#

proc RunSample {w} {
    set top [frame $w.f -bd 1 -relief raised]
    set box [tixButtonBox $w.b -bd 1 -relief raised]

    pack $box -side bottom -fill both
    pack $top -side top -fill both -expand yes

    #------------------------------------------------------------
    # Create the 1st CheckList (Multiple Selection)
    #
    set f [frame $top.f1]
    pack $f -side left -expand yes -fill both -padx 4

    set l [label $f.l -text "Choose languages: "]
    pack $l -side top -fill x -padx 4 -pady 4

    set c1 [tixCheckList $f.c -scrollbar auto]
    pack $c1 -expand yes -fill both -padx 4 -pady 4

    set b1 [button $f.btn -text "Results >>" -command "ChkList_Result $c1"]
    pack $b1 -anchor c
    #------------------------------------------------------------
    # Create the 2nd CheckList (Single Selection, using the -radio option)
    #
    set f [frame $top.f2]
    pack $f -side left -expand yes -fill both -padx 4

    set l [label $f.l -text "Choose one language: "]
    pack $l -side top -fill x -padx 4 -pady 4

    set c2 [tixCheckList $f.c -scrollbar auto -radio true]
    pack $c2 -expand yes -fill both -padx 4 -pady 4

    # Fill up the two checklists with languages
    #
    set names(1) "Ada"
    set names(2) "BCPL"
    set names(3) "C"
    set names(4) "Dylan"
    set names(5) "Eiffel"
    set names(6) "Fortran"
    set names(7) "Incr Tcl"
    set names(8) "Python"
    set names(9) "Scheme"
    set names(0) "TCL"

    set h1 [$c1 subwidget hlist]
    set h2 [$c2 subwidget hlist]

    foreach ent {1 2 3 4 5 6 7 8 9 0} {
	$h1 add $ent -itemtype imagetext -text $names($ent)
    }

    foreach ent {1 2 3 4 5 6 7 8 9 0} {
	$h2 add $ent -itemtype imagetext -text $names($ent)
	$c2 setstatus $ent off
    }

    $c1 setstatus 1 on
    $c1 setstatus 2 on
    $c1 setstatus 3 default
    $c1 setstatus 4 off
    $c1 setstatus 5 off
    $c1 setstatus 6 on
    $c1 setstatus 7 off
    $c1 setstatus 8 on
    $c1 setstatus 9 on
    $c1 setstatus 0 default


    #------------------------------------------------------------
    # Create the 3nd CheckList (a tree). Also, we disable some
    # sub-selections if the top-level selections are not selected.
    # i.e., if the user doesn't like any functional languages,
    # make sure he doesn't select Lisp.
    #
    set f [frame $top.f3]
    pack $f -side left -expand yes -fill both -padx 4

    set l [label $f.l -text "Choose languages: "]
    pack $l -side top -fill x -padx 4 -pady 4

    set c3 [tixCheckList $f.c -scrollbar auto -options {
	hlist.indicator 1
	hlist.indent    20
    }]
    pack $c3 -expand yes -fill both -padx 4 -pady 4

    set h3 [$c3 subwidget hlist]

    $h3 add 0 -itemtype imagetext -text "Functional Languages"
    $h3 add 1 -itemtype imagetext -text "Imperative Languages"

    $h3 add 0.0 -itemtype imagetext -text Lisp
    $h3 add 0.1 -itemtype imagetext -text Scheme
    $h3 add 1.0 -itemtype imagetext -text C
    $h3 add 1.1 -itemtype imagetext -text Python

    $c3 setstatus 0   on
    $c3 setstatus 1   on
    $c3 setstatus 0.0 off
    $c3 setstatus 0.1 off
    $c3 setstatus 1.0 on
    $c3 setstatus 1.1 off

    $c3 config -browsecmd "ChkList:Monitor $c3"
    $c3 config -command   "ChkList:Monitor $c3"

    $c3 autosetmode

    global chklist tixOption
    set chklist(disabled) [tixDisplayStyle imagetext -fg $tixOption(disabled_fg) \
	-refwindow [$c3 subwidget hlist]]
    set chklist(normal)   [tixDisplayStyle imagetext -fg black \
	-refwindow [$c3 subwidget hlist]]

    # Create the buttons
    #
    $box add ok     -text Ok     -command "destroy $w" -width 6
    $box add cancel -text Cancel -command "destroy $w" -width 6
}

proc ChkList_Result {clist} {
    tixDemo:Status "Selected items: [$clist getselection on]"
    tixDemo:Status "Unselected items: [$clist getselection off]"
    tixDemo:Status "Default items: [$clist getselection default]"
}

# This function monitors if any of the two "general groups"
# (functional and imperative languages) are de-selected. If so, it
# sets all the sub-selections to non-selectable by setting their -state
# to disabled.
#
proc ChkList:Monitor {c3 ent} {
    global chklist

    set h [$c3 subwidget hlist]

    if {[$c3 getstatus 0] == "on"} {
	set state normal
    } else {
	set state disabled
    }

    $h entryconfig 0.0 -state $state -style $chklist($state)
    $h entryconfig 0.1 -state $state -style $chklist($state)

    if {[$c3 getstatus 1] == "on"} {
	set state normal
    } else {
	set state disabled
    }

    $h entryconfig 1.0 -state $state -style $chklist($state)
    $h entryconfig 1.1 -state $state -style $chklist($state)
}

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
