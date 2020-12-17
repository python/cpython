# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: DirBox.tcl,v 1.4 2004/03/28 02:44:57 hobbs Exp $
#
# DirBox.tcl --
#
#	Implements the tixDirSelectBox widget.
#
# 	   - overrides the -browsecmd and -command options of the
#	     HList subwidget
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixDirSelectBox {
    -classname TixDirSelectBox
    -superclass tixPrimitive
    -method {
    }
    -flag {
	-command -disablecallback -value
    }
    -configspec {
	{-command command Command ""}
	{-disablecallback disableCallback DisableCallback 0 tixVerifyBoolean}
	{-label label Label "Directory:"}
	{-value value Value ""}
    }
    -forcecall {
	-value -label
    }
    -default {
	{*combo*listbox.height 		5}
	{*combo.label.anchor		w}
	{*combo.labelSide		top}
	{*combo.history			true}
	{*combo.historyLimit		20}
    }
}

proc tixDirSelectBox:InitWidgetRec {w} {
    upvar #0 $w data
    tixChainMethod $w InitWidgetRec
}

proc tixDirSelectBox:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget
    set data(w:dircbx) [tixFileComboBox $w.dircbx]
    set data(w:dirlist)  [tixDirList $w.dirlist]

    pack $data(w:dircbx) -side top -fill x -padx 4 -pady 2
    pack $data(w:dirlist) -side top -fill both -expand yes -padx 4 -pady 2

    if {$data(-value) eq ""} {
	set data(-value) [pwd]
    }
}

proc tixDirSelectBox:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    $data(w:dircbx) config -command [list tixDirSelectBox:Cmd-DirCbx $w]
    $data(w:dirlist) config -command [list tixDirSelectBox:Cmd-DirList $w]\
	-browsecmd [list tixDirSelectBox:Browse-DirList $w]
}

#----------------------------------------------------------------------
# Incoming event: User
#----------------------------------------------------------------------

# User activates the FileComboBox
#
#
proc tixDirSelectBox:Cmd-DirCbx {w args} {
    upvar #0 $w data

    set fInfo [tixEvent value]
    set path [lindex $fInfo 0]

    if {![file exists $path]} {
	# 1.1 Check for validity. The pathname cannot contain invalid chars
	#
	if {![tixFSIsValid $path]} {
	    tk_messageBox -title "Invalid Directory" \
		-type ok -icon error \
		-message "\"$path\" is not a valid directory name"
	    $data(w:dircbx) config \
		-text [tixFSDisplayName [file normalize $data(-value)]] \
		-directory $data(-value)
	    return
	}

	# 1.2 Prompt for creation
	#
	set choice [tk_messageBox -title "Create Directory?" \
			-type yesno -icon question \
			-message "Directory \"$path\" does not exist.\
			\nDo you want to create it?"]
	if {$choice eq "yes"
	    && [catch {file mkdir [file dirname $path]} err]} {
	    tk_messageBox -title "Error Creating Directory" \
		-icon error -type ok \
		-message "Cannot create directory \"$path\":\n$err"
	    set choice "no"
	}
	if {$choice eq "no"} {
	    $data(w:dircbx) config \
		-text [tixFSDisplayName [file normalize $data(-value)]] \
		-directory $data(-value)
	    return
	}
	tixDirSelectBox:SetValue $w $path 1 1
    } elseif {![file isdirectory $path]} {
	# 2.1: Can't choose a non-directory file
	#
	tk_messageBox -title "Invalid Directory" \
	    -type ok -icon error \
	    -message "\"$path\" is not a directory"
	$data(w:dircbx) config \
	    -text [tixFSDisplayName [file normalize $data(-value)]] \
	    -directory $data(-value)
	return
    } else {
	# OK. It is an existing directory
	#
	tixDirSelectBox:SetValue $w $path 1 1
    }
}

# User activates the dir list
#
#
proc tixDirSelectBox:Cmd-DirList {w args} {
    upvar #0 $w data

    set dir $data(-value)
    catch {set dir [tixEvent flag V]}
    set dir [tixFSNormalize $dir]
    tixDirSelectBox:SetValue $w $dir 0 0
}

# User browses the dir list
#
#
proc tixDirSelectBox:Browse-DirList {w args} {
    upvar #0 $w data

    set dir $data(-value)
    catch {set dir [tixEvent flag V]}
    set dir [tixFSNormalize $dir]
    tixDirSelectBox:SetValue $w $dir 0 0
}

#----------------------------------------------------------------------
# Incoming event: Application
#----------------------------------------------------------------------
proc tixDirSelectBox:config-value {w value} {
    upvar #0 $w data

    set value [tixFSNormalize $value]
    tixDirSelectBox:SetValue $w $value 1 1
    return $value
}

proc tixDirSelectBox:config-label {w value} {
    upvar #0 $w data

    $data(w:dircbx) subwidget combo config -label $value
}

#----------------------------------------------------------------------
#
#			Internal functions
#
#----------------------------------------------------------------------

# Arguments:
#	callback:Bool	Should we invoke the the -command.
# 	setlist:Bool	Should we set the -value of the DirList subwidget.
#
proc tixDirSelectBox:SetValue {w dir callback setlist} {
    upvar #0 $w data

    set data(-value) $dir
    $data(w:dircbx) config -text [tixFSDisplayName $dir] -directory $dir
    if {$setlist && [file isdirectory $dir]} {
	tixSetSilent $data(w:dirlist) $dir
    }

    if {$callback} {
	if {!$data(-disablecallback) && [llength $data(-command)]} {
	    set bind(specs) {%V}
	    set bind(%V)    $data(-value)

	    tixEvalCmdBinding $w $data(-command) bind $data(-value)
	}
    }
}
