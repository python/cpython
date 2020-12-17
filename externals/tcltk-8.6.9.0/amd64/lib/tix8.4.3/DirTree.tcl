# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DirTree.tcl,v 1.4 2004/03/28 02:44:57 hobbs Exp $
#
# DirTree.tcl --
#
#	Implements directory tree for Unix file systems
#
#       What the indicators mean:
#
#	(+): There are some subdirectories in this directory which are not
#	     currently visible.
#	(-): This directory has some subdirectories and they are all visible
#
#      none: The dir has no subdirectori(es).
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

##
## The tixDirTree require special FS handling due to it's limited
## separator idea (instead of real tree).
##

tixWidgetClass tixDirTree {
    -classname TixDirTree
    -superclass tixVTree
    -method {
	activate chdir refresh
    }
    -flag {
	-browsecmd -command -directory -disablecallback -showhidden -value
    }
    -configspec {
	{-browsecmd browseCmd BrowseCmd ""}
	{-command command Command ""}
	{-disablecallback disableCallback DisableCallback 0 tixVerifyBoolean}
	{-showhidden showHidden ShowHidden 0 tixVerifyBoolean}
	{-value value Value ""}
    }
    -alias {
	{-directory -value}
    }
    -default {
	{.scrollbar			auto}
	{*Scrollbar.takeFocus           0}
	{*borderWidth                   1}
	{*hlist.indicator               1}
	{*hlist.background              #c3c3c3}
	{*hlist.drawBranch              1}
	{*hlist.height                  10}
	{*hlist.highlightBackground     #d9d9d9}
	{*hlist.indent                  20}
	{*hlist.itemType                imagetext}
	{*hlist.padX                    3}
	{*hlist.padY                    0}
	{*hlist.relief                  sunken}
	{*hlist.takeFocus               1}
	{*hlist.wideSelection           0}
	{*hlist.width                   20}
    }
}

proc tixDirTree:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    if {$data(-value) == ""} {
	set data(-value) [pwd]
    }

    tixDirTree:SetDir $w [file normalize $data(-value)]
}

proc tixDirTree:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
    tixDoWhenMapped $w [list tixDirTree:StartUp $w]

    $data(w:hlist) config -separator [tixFSSep] \
	-selectmode "single" -drawbranch 1

    # We must creat an extra copy of these images to avoid flashes on
    # the screen when user changes directory
    #
    set data(images) [image create compound -window $data(w:hlist)]
    $data(images) add image -image [tix getimage act_fold]
    $data(images) add image -image [tix getimage folder]
    $data(images) add image -image [tix getimage openfold]
}

proc tixDirTree:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings
}

# Add one dir into the node (parent directory), sorted alphabetically
#
proc tixDirTree:AddToList {w fsdir image} {
    upvar #0 $w data

    set dir    [tixFSInternal $fsdir]

    if {[$data(w:hlist) info exists $dir]} { return }

    set parent [file dirname $fsdir]
    if {$fsdir eq $parent} {
	# root node
	set node ""
    } else {
	# regular node
	set node [tixFSInternal $parent]
    }
    set added 0
    set text  [tixFSDisplayFileName $fsdir]
    foreach sib [$data(w:hlist) info children $node] {
	if {[string compare $dir $sib] < 0} {
	    $data(w:hlist) add $dir -before $sib -text $text -image $image
	    set added 1
	    break
	}
    }
    if {!$added} {
	$data(w:hlist) add $dir -text $text -image $image
    }

    # Check to see if we have children (%% optimize!)
    if {[llength [tixFSListDir $fsdir 1 0 0 $data(-showhidden)]]} {
	tixVTree:SetMode $w $dir open
    }
}

proc tixDirTree:LoadDir {w fsdir {mode toggle}} {
    if {![winfo exists $w]} { return }
    upvar #0 $w data

    # Add the directory and set it to the active directory
    #
    set fsdir [tixFSNormalize $fsdir]
    set dir   [tixFSInternal $fsdir]
    if {![$data(w:hlist) info exists $dir]} {
	# Add $dir and all ancestors of $dir into the HList widget
	set fspath ""
	set imgopenfold [tix getimage openfold]
	foreach part [tixFSAncestors $fsdir] {
	    set fspath [file join $fspath $part]
	    tixDirTree:AddToList $w $fspath $imgopenfold
	}
    }
    $data(w:hlist) entryconfig $dir -image [tix getimage act_fold]

    if {$mode eq "toggle"} {
	if {[llength [$data(w:hlist) info children $dir]]} {
	    set mode flatten
	} else {
	    set mode expand
	}
    }

    if {$mode eq "expand"} {
	# Add all the sub directories of fsdir into the HList widget
	tixBusy $w on $data(w:hlist)
	set imgfolder [tix getimage folder]
	foreach part [tixFSListDir $fsdir 1 0 0 $data(-showhidden)] {
	    tixDirTree:AddToList $w [file join $fsdir $part] $imgfolder
	}
	tixWidgetDoWhenIdle tixBusy $w off $data(w:hlist)
	# correct indicator to represent children status (added above)
	if {[llength [$data(w:hlist) info children $dir]]} {
	    tixVTree:SetMode $w $dir close
	} else {
	    tixVTree:SetMode $w $dir none
	}
    } else {
	$data(w:hlist) delete offsprings $dir
	tixVTree:SetMode $w $dir open
    }
}

proc tixDirTree:ToggleDir {w value mode} {
    upvar #0 $w data

    tixDirTree:LoadDir $w $value $mode
    tixDirTree:CallCommand $w
}

proc tixDirTree:CallCommand {w} {
    upvar #0 $w data

    if {[llength $data(-command)] && !$data(-disablecallback)} {
	set bind(specs) {%V}
	set bind(%V)    $data(-value)

	tixEvalCmdBinding $w $data(-command) bind $data(-value)
    }
}

proc tixDirTree:CallBrowseCmd {w ent} {
    upvar #0 $w data

    if {[llength $data(-browsecmd)] && !$data(-disablecallback)} {
	set bind(specs) {%V}
	set bind(%V)    $data(-value)

	tixEvalCmdBinding $w $data(-browsecmd) bind [list $data(-value)]
    }
}

proc tixDirTree:StartUp {w} {
    if {![winfo exists $w]} { return }

    upvar #0 $w data

    # make sure that all the basic volumes are listed
    set imgopenfold [tix getimage openfold]
    foreach fspath [tixFSVolumes] {
	tixDirTree:AddToList $w $fspath $imgopenfold
    }

    tixDirTree:LoadDir $w [tixFSExternal $data(i-directory)]
}

proc tixDirTree:ChangeDir {w fsdir {forced 0}} {
    upvar #0 $w data

    set dir [tixFSInternal $fsdir]
    if {!$forced && $data(i-directory) eq $dir} {
	return
    }

    if {!$forced && [$data(w:hlist) info exists $dir]} {
	# Set the old directory to "non active"
	#
	if {[$data(w:hlist) info exists $data(i-directory)]} {
	    $data(w:hlist) entryconfig $data(i-directory) \
		-image [tix getimage folder]
	}

	$data(w:hlist) entryconfig $dir -image [tix getimage act_fold]
    } else {
	if {$forced} {
	    if {[llength [$data(w:hlist) info children $dir]]} {
		set mode expand
	    } else {
		set mode flatten
	    }
	} else {
	    set mode toggle
	}
	tixDirTree:LoadDir $w $fsdir $mode
	tixDirTree:CallCommand $w
    }
    tixDirTree:SetDir $w $fsdir
}


proc tixDirTree:SetDir {w path} {
    upvar #0 $w data

    set data(i-directory) [tixFSInternal $path]
    set data(-value) [tixFSNativeNorm $path]
}

#----------------------------------------------------------------------
#
# Virtual Methods
#
#----------------------------------------------------------------------
proc tixDirTree:OpenCmd {w ent} {
    set fsdir [tixFSExternal $ent]
    tixDirTree:ToggleDir $w $fsdir expand
    tixDirTree:ChangeDir $w $fsdir
    tixDirTree:CallBrowseCmd $w $fsdir
}

proc tixDirTree:CloseCmd {w ent} {
    set fsdir [tixFSExternal $ent]
    tixDirTree:ToggleDir $w $fsdir flatten
    tixDirTree:ChangeDir $w $fsdir
    tixDirTree:CallBrowseCmd $w $fsdir
}

proc tixDirTree:Command {w B} {
    upvar #0 $w data
    upvar $B bind

    set ent [tixEvent flag V]
    tixChainMethod $w Command $B

    if {[llength $data(-command)]} {
	set fsdir [tixFSExternal $ent]
	tixEvalCmdBinding $w $data(-command) bind $fsdir
    }
}

# This is a virtual method
#
proc tixDirTree:BrowseCmd {w B} {
    upvar #0 $w data
    upvar 1 $B bind

    set ent [tixEvent flag V]
    set fsdir [tixFSExternal $ent]

    # This is a hack because %V may have been modified by callbrowsecmd
    set fsdir [file normalize $fsdir]

    tixDirTree:ChangeDir $w $fsdir
    tixDirTree:CallBrowseCmd $w $fsdir
}

#----------------------------------------------------------------------
#
# Public Methods
#
#----------------------------------------------------------------------
proc tixDirTree:chdir {w value} {
    tixDirTree:ChangeDir $w [file normalize $value]
}

proc tixDirTree:refresh {w {dir ""}} {
    upvar #0 $w data

    if {$dir eq ""} {
	set dir $data(-value)
    }
    set dir [file normalize $dir]

    tixDirTree:ChangeDir $w $dir 1

    # Delete any stale directories that no longer exist
    #
    foreach child [$data(w:hlist) info children [tixFSInternal $dir]] {
	if {![file exists [tixFSExternal $child]]} {
	    $data(w:hlist) delete entry $child
	}
    }
}

proc tixDirTree:config-directory {w value} {
    tixDirTree:ChangeDir $w [file normalize $value]
}
