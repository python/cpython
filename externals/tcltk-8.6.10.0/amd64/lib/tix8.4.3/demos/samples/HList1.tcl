# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: HList1.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixHList widget -- you can
# use to display data in a tree structure. For example, your family tree
#
#
proc RunSample {w} {

    # Create the tixHList and the tixLabelEntry widgets on the on the top
    # of the dialog box
    #
    # [Hint] We create the tixHList and and the scrollbar by ourself,
    #	     but it is more convenient to use the tixScrolledHlist widget
    #	     which does all the chores for us.
    #
    # [Hint] Use of the -browsecmd and -command options:
    #	     We want to set the tixLabelEntry accordingly whenever the user
    #	     single-clicks on an entry in the HList box. Also, when the user
    #	     double-clicks, we want to print out the selection and close
    #	     the dialog box
    #
    frame $w.top -border 1 -relief raised

    tixHList $w.top.h -yscrollcommand "$w.top.s set" -separator / \
	-browsecmd "hlist1:browse $w.top.h" \
	-command "hlist1:activate $w.top.h"\
	-wideselection false \
	-indent 15
    scrollbar $w.top.s -command "$w.top.h yview" -takefocus 0

    # Some icons for our list entries
    #
    global folder1 folder2
    set img1 [image create bitmap -data $folder1]
    set img2 [image create bitmap -data $folder2]

    # Put our directories into the HList entry
    #
    set h $w.top.h
    set dirs {
	/
	/lib
	/pkg
	/usr
	/usr/lib
	/usr/local
	/usr/local/lib
	/pkg/lib
    }
    foreach d $dirs {
	$h add $d -itemtype imagetext -text $d -image $img2 -data $d

	# We only want the user to select the directories that
	# ends by "lib"
	if {![string match "*lib" $d]} {
	    $h entryconfig $d -state disabled -image $img1
	}
    }
    
    # We use a LabelEntry to hold the installation directory. The user
    # can choose from the DirList widget, or he can type in the directory 
    # manually
    #
    tixLabelEntry $w.top.e -label "Installation Directory:" -labelside top \
	-options {
	    entry.width 25
	    entry.textVariable demo_hlist_dir
	    label.anchor w
	}
    bind [$w.top.e subwidget entry] <Return> "hlist:okcmd $w"

    # Set the default value
    #
    uplevel #0 set demo_hlist_dir /usr/local/lib
    $h anchor set /usr/local/lib
    $h select set /usr/local/lib

    pack $w.top.h -side left -expand yes -fill both -padx 2 -pady 2
    pack $w.top.s -side left -fill y -pady 2
    pack $w.top.e -side left -expand yes -fill x -anchor s -padx 4 -pady 2

    # Use a ButtonBox to hold the buttons.
    #
    tixButtonBox $w.box -orientation horizontal
    $w.box add ok     -text Ok     -underline 0 -command "hlist:okcmd $w" \
	-width 6
    $w.box add cancel -text Cancel -underline 0 -command "destroy $w" \
	-width 6

    pack $w.box -side bottom -fill x
    pack $w.top -side top -fill both -expand yes
}

# In an actual program, you may want to tell the user how much space he has
# left in this directory
#
#
proc hlist1:browse {w dir} {
    global demo_hlist_dir

    set demo_hlist_dir [$w entrycget $dir -data]
}

# In an actual program, you will install your favorit application
# in the selected directory
#
proc hlist1:activate {w dir} {
    global demo_hlist_dir

    set demo_hlist_dir [$w entrycget $dir -data]
    tixDemo:Status "You have selected the directory $demo_hlist_dir"

    destroy [winfo toplevel $w]
}

proc hlist:okcmd {w} {
    global demo_hlist_dir

    tixDemo:Status "You have selected the directory $demo_hlist_dir"

    destroy $w
}

set folder1 {
#define foo_width 16
#define foo_height 12
static unsigned char foo_bits[] = {
   0x00, 0x00, 0x00, 0x3e, 0xfe, 0x41, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
   0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00};}

set folder2 {
#define foo_width 16
#define foo_height 12
static unsigned char foo_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfe, 0x7f, 0x02, 0x40, 0x02, 0x44, 0xf2, 0x4f,
   0xf2, 0x5f, 0xf2, 0x4f, 0x02, 0x44, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00};
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

