# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: FileEnt.tcl,v 1.7 2004/03/28 02:44:57 hobbs Exp $
#
# FileEnt.tcl --
#
# 	TixFileEntry Widget: an entry box for entering filenames.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixFileEntry {
    -classname TixFileEntry
    -superclass tixLabelWidget
    -method {
	invoke filedialog update
    }
    -flag {
	-activatecmd -command -dialogtype -disablecallback -disabledforeground
	-filebitmap -selectmode -state -validatecmd -value -variable
    }
    -forcecall {
	-variable
    }
    -static {
	-filebitmap
    }
    -configspec {
	{-activatecmd activateCmd ActivateCmd ""}
	{-command command Command ""}
	{-dialogtype dialogType DialogType ""}
	{-disablecallback disableCallback DisableCallback 0 tixVerifyBoolean}
	{-disabledforeground disabledForeground DisabledForeground #303030}
	{-filebitmap fileBitmap FileBitmap ""}
	{-selectmode selectMode SelectMode normal}
	{-state state State normal}
	{-validatecmd validateCmd ValidateCmd ""}
	{-value value Value ""}
	{-variable variable Variable ""}
    }
    -default {
	{*frame.borderWidth		2}
	{*frame.relief			sunken}
	{*Button.highlightThickness	0}
	{*Entry.highlightThickness	0}
	{*Entry.borderWidth 		0}
    }
}

proc tixFileEntry:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec
    set data(varInited)	  0

    if {$data(-filebitmap) eq ""} {
	set data(-filebitmap) [tix getbitmap openfile]
    }
}

proc tixFileEntry:ConstructFramedWidget {w frame} {
    upvar #0 $w data

    tixChainMethod $w ConstructFramedWidget $frame

    set data(w:entry)  [entry  $frame.entry]
    set data(w:button) [button $frame.button -bitmap $data(-filebitmap) \
			    -takefocus 0]
    set data(entryfg) [$data(w:entry) cget -fg]

    pack $data(w:button) -side right -fill both
    pack $data(w:entry)  -side left  -expand yes -fill both
}

proc tixFileEntry:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    $data(w:button) config -command [list tixFileEntry:OpenFile $w]
    tixSetMegaWidget $data(w:entry) $w

    # If user press <return>, verify the value and call the -command
    #
    bind $data(w:entry) <Return> [list tixFileEntry:invoke $w]
    bind $data(w:entry) <KeyPress> {
	if {[set [tixGetMegaWidget %W](-selectmode)] eq "immediate"} {
	    tixFileEntry:invoke [tixGetMegaWidget %W]
	}
    }
    bind $data(w:entry) <FocusOut>  {
        if {"%d" eq "NotifyNonlinear" || "%d" eq "NotifyNonlinearVirtual"} {
	    tixFileEntry:invoke [tixGetMegaWidget %W]
        }
    }
    bind $w <FocusIn> [list focus $data(w:entry)]
}

#----------------------------------------------------------------------
#                           CONFIG OPTIONS
#----------------------------------------------------------------------
proc tixFileEntry:config-state {w value} {
    upvar #0 $w data

    if {$value eq "normal"} {
	$data(w:button) config -state $value
	$data(w:entry)  config -state $value -fg $data(entryfg)
	catch {$data(w:label)  config -fg $data(entryfg)}
    } else {
	$data(w:button) config -state $value
	$data(w:entry)  config -state $value -fg $data(-disabledforeground)
	catch {$data(w:label)  config -fg $data(-disabledforeground)}
    }

    return ""
}

proc tixFileEntry:config-value {w value} {
    tixFileEntry:SetValue $w $value
}

proc tixFileEntry:config-variable {w arg} {
    upvar #0 $w data

    if {[tixVariable:ConfigVariable $w $arg]} {
       # The value of data(-value) is changed if tixVariable:ConfigVariable 
       # returns true
       tixFileEntry:SetValue $w $data(-value)
    }
    catch {
	unset data(varInited)
    }
    set data(-variable) $arg
}

#----------------------------------------------------------------------
#                         User Commands
#----------------------------------------------------------------------
proc tixFileEntry:invoke {w} {
    upvar #0 $w data

    if {![catch {$data(w:entry) index sel.first}]} {
	# THIS ENTRY OWNS SELECTION --> TURN IT OFF
	#
	$data(w:entry) select from end
	$data(w:entry) select to   end
    }

    tixFileEntry:SetValue $w [$data(w:entry) get]
}

proc tixFileEntry:filedialog {w args} {
    upvar #0 $w data

    if {[llength $args]} {
	return [eval [tix filedialog $data(-dialogtype)] $args]
    } else {
	return [tix filedialog $data(-dialogtype)]
    }
}

proc tixFileEntry:update {w} {
    upvar #0 $w data

    if {[$data(w:entry) get] ne $data(-value)} {
	tixFileEntry:invoke $w
    }
}
#----------------------------------------------------------------------
#                       Internal Commands
#----------------------------------------------------------------------
proc tixFileEntry:OpenFile {w} {
     upvar #0 $w data

     if {$data(-activatecmd) != ""} {
	 uplevel #0 $data(-activatecmd)
     }

     switch -- $data(-dialogtype) tk_chooseDirectory {
	 set args [list -parent [winfo toplevel $w]]
	 if {[set initial $data(-value)] != ""} {
	     lappend args -initialdir $data(value)
	 }
	 set retval [eval [linsert $args 0 tk_chooseDirectory]]

	 if {$retval != ""} {tixFileEntry:SetValue $w [tixFSNative $retval]}
     } tk_getOpenFile - tk_getSaveFile {
	 set args [list -parent [winfo toplevel $w]]

	 if {[set initial [$data(w:entry) get]] != ""} {
	     switch -glob -- $initial *.py {
		 set types [list {"Python Files" {.py .pyw}} {"All Files" *}]
	     } *.txt {
		 set types [list {"Text Files" .txt} {"All Files" *}]
	     } *.tcl {
		 set types [list {"Tcl Files" .tcl} {"All Files" *}]
	     } * - default {
		 set types [list {"All Files" *}]
	     }
	     if {[file isfile $initial]} {
		 lappend args -initialdir [file dir $initial] \
			 -initialfile $initial
	     } elseif {[file isdir $initial]} {
		 lappend args -initialdir $initial
	     }
	 } else {
	     set types [list {"All Files" *}]
	 }
	 lappend args -filetypes $types

	 set retval [eval $data(-dialogtype) $args]
	 if {$retval != ""} {tixFileEntry:SetValue $w [tixFSNative $retval]}
     } default {
	 set filedlg [tix filedialog $data(-dialogtype)]

	 $filedlg config -parent [winfo toplevel $w] \
		 -command [list tixFileEntry:FileDlgCallback $w]

	 focus $data(w:entry)

	 $filedlg popup
     }
}

proc tixFileEntry:FileDlgCallback {w args} {
    set filename [tixEvent flag V]

    tixFileEntry:SetValue $w $filename
}

proc tixFileEntry:SetValue {w value} {
    upvar #0 $w data

    if {[llength $data(-validatecmd)]} {
	set value [tixEvalCmdBinding $w $data(-validatecmd) "" $value]
    }

    if {$data(-state) eq "normal"} {
	$data(w:entry) delete 0 end
	$data(w:entry) insert 0 $value
	$data(w:entry) xview end
    }

    set data(-value) $value

    tixVariable:UpdateVariable $w

    if {[llength $data(-command)] && !$data(-disablecallback)} {
	if {![info exists data(varInited)]} {
	    set bind(specs) ""
	    tixEvalCmdBinding $w $data(-command) bind $value
	}
    }
}

proc tixFileEntry:Destructor {w} {
    upvar #0 $w data

    tixUnsetMegaWidget $data(w:entry)
    tixVariable:DeleteVariable $w

    # Chain this to the superclass
    #
    tixChainMethod $w Destructor
}
