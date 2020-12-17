# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DynTree.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates how to use the TixTree widget to display
# dynamic hierachical data (the files in the Unix file system)
#

proc RunSample {w} {

    # We create the frame and the ScrolledHList widget
    # at the top of the dialog box
    #
    frame $w.top -relief raised -bd 1

    # Create a TixTree widget to display the hypothetical DOS disk drive
    # 
    #
    tixTree $w.top.a  -options {
	hlist.separator "/"
	hlist.width 35
	hlist.height 25
    }

    pack $w.top.a -expand yes -fill both -padx 10 -pady 10 -side left
 
    set tree $w.top.a 
    set hlist [$tree subwidget hlist]

    $tree config -opencmd "DynTree:OpenDir $tree"

    # Add the root directory the TixTree widget
    DynTree:AddDir $tree /

    # The / directory is added in the "open" mode. The user can open it
    # and then browse its subdirectories ...
    

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

proc DynTree:AddDir {tree dir} {
    set hlist [$tree subwidget hlist]

    if {$dir == "/"} {
	set text /
    } else {
	set text [file tail $dir]
    }

    $hlist add $dir -itemtype imagetext \
	-text $text -image [tix getimage folder]

    catch {
	# We need a catch here because the directory may not be readable by us
	#
	$tree setmode $dir none
	if {[glob -nocomplain $dir/*] != {}} {
	    $tree setmode $dir open
	}
    }
}


# This command is called whenever the user presses the (+) indicator or
# double clicks on a directory whose mode is "open". It loads the files
# inside that directory into the Tree widget.
#
# Note we didn't specify the -closecmd option for the Tree widget, so it
# performs the default action when the user presses the (-) indicator or
# double clicks on a directory whose mode is "close": hide all of its child
# entries
#
proc DynTree:OpenDir {tree dir} {
    set PWD [pwd]
    set hlist [$tree subwidget hlist]

    if {[$hlist info children $dir] != {}} {
	# We have already loaded this directory. Let's just
	# show all the child entries
	#
	# Note: since we load the directory only once, it will not be
	#       refreshed if the you add or remove files from this
	#	directory.
	#
	foreach kid [$hlist info children $dir] {
	    $hlist show entry $kid
	}
	return
    }

    if [catch {cd $dir}] {
	# We can't read that directory, better not do anything
	cd $PWD
	return
    }

    set files [lsort [glob -nocomplain *]]
    foreach f $files {
	if [file isdirectory $f] {
	    if {$dir == "/"} {
		set subdir /$f
	    } else {
		set subdir $dir/$f
	    }
	    DynTree:AddDir $tree $subdir
	} else {
	    if {$dir == "/"} {
		set file /$f
	    } else {
		set file $dir/$f
	    }

	    $hlist add $file -itemtype imagetext \
		-text $f -image [tix getimage file]
	}
    }

    cd $PWD
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

