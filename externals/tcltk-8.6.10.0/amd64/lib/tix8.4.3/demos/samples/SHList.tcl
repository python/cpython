# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: SHList.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixScrolledHList widget.
#

proc RunSample {w} {

    # We create the frame and the ScrolledHList widget
    # at the top of the dialog box
    #
    frame $w.top -relief raised -bd 1

    # Put a simple hierachy into the HList (two levels). Use colors and
    # separator widgets (frames) to make the list look fancy
    #
    tixScrolledHList $w.top.a
    pack $w.top.a -expand yes -fill both -padx 10 -pady 10 -side left


    # This is our little relational database
    #
    set bosses {
	{jeff  "Jeff Waxman"}
	{john  "John Lee"}
	{peter "Peter Kenson"}
    }

    set employees {
	{alex	john 	"Alex Kellman"}
	{alan	john 	"Alan Adams"}
	{andy	peter 	"Andreas Crawford"}
	{doug	jeff  	"Douglas Bloom"}
	{jon	peter	"Jon Baraki"}
	{chris	jeff	"Chris Geoffrey"}
	{chuck	jeff	"Chuck McLean"}
    }

    set hlist [$w.top.a subwidget hlist]

    # Let configure the appearance of the HList subwidget 
    #
    $hlist config -separator "." -width 25 -drawbranch 0 -indent 10

    set index 0
    foreach line $bosses {
	if {$index != 0} {
	    frame $hlist.sep$index -bd 2 -height 2 -width 150 -relief sunken \
		-bg [$hlist cget -bg]

	    $hlist addchild {} -itemtype window \
		-window $hlist.sep$index -state disabled
	}
	$hlist add [lindex $line 0] -itemtype text \
	    -text [lindex $line 1]
	incr index
    }

    foreach line $employees {
	# "." is the separator character we chose above
	#
	set entrypath [lindex $line 1].[lindex $line 0]
	#             ^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^
	#	      parent entryPath / child's name

	$hlist add $entrypath -text [lindex $line 2]

	# [Hint] Make sure the [lindex $line 1].[lindex $line 0] you choose
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

