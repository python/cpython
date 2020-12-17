# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SListBox.tcl,v 1.4 2008/02/27 22:17:27 hobbs Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixScrolledListBox widget.
#

proc RunSample {w} {

    # We create the frame and the two ScrolledListBox widgets
    # at the top of the dialog box
    #
    frame $w.top -relief raised -bd 1

    # The first ScrolledListBox widget always shows both scrollbars
    #
    tixScrolledListBox $w.top.a -scrollbar both
    pack $w.top.a -expand yes -fill both -padx 10 -pady 10 -side left

    # The second  ScrolledListBox widget shows the scrollbars only when
    # needed.
    #
    # [Hint] See how you can use the -options switch to set the options
    #	     for the subwidgets
    #
    tixScrolledListBox $w.top.b -scrollbar auto -options {
	listbox.font 8x13
    }
    pack $w.top.b -expand yes -fill both -padx 10 -pady 10 -side left

    # Put the elements inside the two listboxes: notice that you need
    # to insert inside the "listbox" subwidget of the ScrolledListBox.
    # $w.top.a itself does NOT have an "insert" command.
    #
    $w.top.a subwidget listbox insert 0 \
	Alabama Alaska Arizona Arkansas California \
	Colorado Connecticut Delaware Florida Georgia Hawaii Idaho Illinois \
	Indiana Iowa Kansas Kentucky Louisiana Maine Maryland \
	Massachusetts Michigan Minnesota Mississippi Missouri \
	Montana Nebraska Nevada "New Hampshire" "New Jersey" "New Mexico" \
	"New York" "North Carolina" "North Dakota" \
	Ohio Oklahoma Oregon Pennsylvania "Rhode Island" \
	"South Carolina" "South Dakota" \
	Tennessee Texas Utah Vermont Virginia Washington \
	"West Virginia" Wisconsin Wyoming

    raise [$w.top.a subwidget listbox ]
    $w.top.b subwidget listbox insert 0 \
	Alabama Alaska Arizona Arkansas California \
	Colorado Connecticut Delaware Florida Georgia Hawaii Idaho Illinois \
	Indiana Iowa Kansas Kentucky

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "destroy $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}


# This "if" statement makes it possible to run this script file inside or
# outside of the main demo program "widget".
#
if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}

