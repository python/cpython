# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SHList2.tcl,v 1.4 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates how to use multiple columns and multiple styles
# in the tixHList widget
#
# In a tixHList widget, you can have one ore more columns. 
#

proc RunSample {w} {

    # We create the frame and the ScrolledHList widget
    # at the top of the dialog box
    #
    frame $w.top -relief raised -bd 1

    # Put a simple hierachy into the HList (two levels). Use colors and
    # separator widgets (frames) to make the list look fancy
    #
    tixScrolledHList $w.top.a -options {
	hlist.columns 3
	hlist.header  true
    }
    pack $w.top.a -expand yes -fill both -padx 10 -pady 10 -side left
 
    set hlist [$w.top.a subwidget hlist]

    # Create the title for the HList widget
    #	>> Notice that we have set the hlist.header subwidget option to true
    #      so that the header is displayed
    #

    # First some styles for the headers
    set style(header) [tixDisplayStyle text  -refwindow $hlist \
	-fg black -anchor c \
        -padx 8 -pady 2\
	-font [tix option get bold_font ]]

    $hlist header create 0 -itemtype text -text Name \
	-style $style(header)
    $hlist header create 1 -itemtype text -text Position \
	-style $style(header)

    # Notice that we use 3 columns in the hlist widget. This way when the user
    # expands the windows wide, the right side of the header doesn't look
    # chopped off. The following line ensures that the 3 column header is
    # not shown unless the hlist window is wider than its contents.
    #
    $hlist column width 2 0

    # This is our little relational database
    #
    set boss {doe "John Doe"	Director}

    set managers {
	{jeff  "Jeff Waxman"	Manager}
	{john  "John Lee"	Manager}
	{peter "Peter Kenson"	Manager}
    }

    set employees {
	{alex	john 	"Alex Kellman"		Clerk}
	{alan	john 	"Alan Adams"		Clerk}
	{andy	peter 	"Andreas Crawford"	Salesman}
	{doug	jeff  	"Douglas Bloom"		Clerk}
	{jon	peter	"Jon Baraki"		Salesman}
	{chris	jeff	"Chris Geoffrey"	Clerk}
	{chuck	jeff	"Chuck McLean"		Cleaner}
    }

    set style(mgr_name)  [tixDisplayStyle text  -refwindow $hlist \
	-font [tix option get bold_font ]]
    set style(mgr_posn)  [tixDisplayStyle text  -refwindow $hlist \
	    -padx 8]

    set style(empl_name) [tixDisplayStyle text  -refwindow $hlist \
	-font [tix option get bold_font ]]
    set style(empl_posn) [tixDisplayStyle text  -refwindow $hlist \
	    -padx 8 ]

    # Let configure the appearance of the HList subwidget 
    #
    $hlist config -separator "." -width 25 -drawbranch 0 -indent 10
    $hlist column width 0 -char 20

    # Create the boss
    #
    $hlist add . -itemtype text -text [lindex $boss 1] \
	-style $style(mgr_name)
    $hlist item create . 1 -itemtype text -text [lindex $boss 2] \
	-style $style(mgr_posn)

    # Create the managers
    #
    set index 0
    foreach line $managers {
	set row [$hlist add .[lindex $line 0] -itemtype text \
	    -text [lindex $line 1] -style $style(mgr_name)]
	$hlist item create $row 1 -itemtype text -text [lindex $line 2] \
	    -style $style(mgr_posn)
	incr index
    }

    foreach line $employees {
	# "." is the separator character we chose above
	#
	set entrypath .[lindex $line 1].[lindex $line 0]
	#              ^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^
	#	       parent entryPath / child's name

	set row [$hlist add $entrypath -text [lindex $line 2] \
	    -style $style(empl_name)]
	$hlist item create $row 1 -itemtype text -text [lindex $line 3] \
	    -style $style(empl_posn)

	# [Hint] Make sure the .[lindex $line 1].[lindex $line 0] you choose
	#	 are unique names. If you cannot be sure of this (because of
	#	 the structure of your database, e.g.) you can use the
	#	 "addchild" widget command instead:
	#
	#  $hlist addchild [lindex $line 1] -text [lindex $line 2]
	#                  ^^^^^^^^^^^^^^^^
	#                  parent entryPath 

    }

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
    bind .demo <Destroy> exit
}

