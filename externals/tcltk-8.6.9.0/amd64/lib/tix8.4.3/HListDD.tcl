# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: HListDD.tcl,v 1.3 2001/12/09 05:04:02 idiscovery Exp $
#
# HListDD.tcl --
#
#	!!! PRE-ALPHA CODE, NOT USED, DON'T USE !!!
#
#	This file implements drag+drop for HList.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#
# events
#
#

proc tixHListSingle:DragTimer {w ent} {
    case [tixHListSingle:GetState $w] {
	{1} {
	    # fire up
	}
    }
}





#----------------------------------------------------------------------
#
#		    Drag + Drop Bindings
#
#----------------------------------------------------------------------

	     #----------------------------------------#
	     #	          Sending Actions	      #
	     #----------------------------------------#

#----------------------------------------------------------------------
#  tixHListSingle:Send:WaitDrag --
#
#	Sender wait for dragging action
#----------------------------------------------------------------------
proc tixHListSingle:Send:WaitDrag {w x y} {
    global tixPriv

    set ent [tixHListSingle:GetNearest $w $y]
    if {$ent != ""} {
	$w anchor set $ent
	$w select clear
	$w select set $ent
 
	set tixPriv(dd,$w:moved) 0
	set tixPriv(dd,$w:entry) $ent

#	set browsecmd [$w cget -browsecmd]
#	if {$browsecmd != "" && $ent != ""} {
#	    eval $browsecmd $ent
#	}
    }
}

proc tixHListSingle:Send:StartDrag {w x y} {
    global tixPriv
    set dd [tixGetDragDropContext $w]

    if {![info exists tixPriv(dd,$w:entry)]} {
	return
    }
    if {$tixPriv(dd,$w:entry) == ""} {
	return
    }

    if {$tixPriv(dd,$w:moved) == 0} {
	$w dragsite set $tixPriv(dd,$w:entry)
	set tixPriv(dd,$w:moved) 1
	$dd config -source $w -command [list tixHListSingle:Send:Cmd $w]
	$dd startdrag $X $Y
    } else {
	$dd drag $X $Y
    }
}

proc tixHListSingle:Send:DoneDrag {w x y} {
    global tixPriv
    global moved

    if {![info exists tixPriv(dd,$w:entry)]} {
	return
    }
    if {$tixPriv(dd,$w:entry) == ""} {
	return
    }

    if {$tixPriv(dd,$w:moved) == 1} {
	set dd [tixGetDragDropContext $w]
	$dd drop $X $Y
    }
    $w dragsite clear
    catch {unset tixPriv(dd,$w:moved)}
    catch {unset tixPriv(dd,$w:entry)}
}

proc tixHListSingle:Send:Cmd {w option args} {
    set dragCmd [$w cget -dragcmd]
    if {$dragCmd != ""} {
	return [eval $dragCmd $option $args]
    }

    # Perform the default action
    #
    case "$option" {
	who {
	    return $w
	}
	types {
	    return {data text}
	}
	get {
	    global tixPriv
	    if {[lindex $args 0] == "text"} {
		if {$tixPriv(dd,$w:entry) != ""} {
		    return [$w entrycget $tixPriv(dd,$w:entry) -text]
		}
	    }
	    if {[lindex $args 0] == "data"} {
		if {$tixPriv(dd,$w:entry) != ""} {
		    return [$w entrycget $tixPriv(dd,$w:entry) -data]
		}
	    }
	}
    }
}

	     #----------------------------------------#
	     #	          Receiving Actions	      #
	     #----------------------------------------#
proc tixHListSingle:Rec:DragOver {w sender x y} {
    if {[$w cget -selectmode] != "dragdrop"} {
	return
    }

    set ent [tixHListSingle:GetNearest $w $y]
    if {$ent != ""} {
	$w dropsite set $ent
    } else {
	$w dropsite clear
    }
}

proc tixHListSingle:Rec:DragIn {w sender x y} {
    if {[$w cget -selectmode] != "dragdrop"} {
	return
    }
    set ent [tixHListSingle:GetNearest $w $y]
    if {$ent != ""} {
	$w dropsite set $ent
    } else {
	$w dropsite clear
    }
}

proc tixHListSingle:Rec:DragOut {w sender x y} {
    if {[$w cget -selectmode] != "dragdrop"} {
	return
    }
    $w dropsite clear
}

proc tixHListSingle:Rec:Drop {w sender x y} {
    if {[$w cget -selectmode] != "dragdrop"} {
	return
    }
    $w dropsite clear

    set ent [tixHListSingle:GetNearest $w $y]
    if {$ent != ""} {
	$w anchor set $ent
	$w select clear
	$w select set $ent
    }
 
    set dropCmd [$w cget -dropcmd]
    if {$dropCmd != ""} {
	eval $dropCmd $sender $x $y
	return
    }

#    set browsecmd [$w cget -browsecmd]
#    if {$browsecmd != "" && $ent != ""} {
#	eval $browsecmd [list $ent]
#    }
}

tixDropBind TixHListSingle <In>   "tixHListSingle:Rec:DragIn %W %S %x %y"
tixDropBind TixHListSingle <Over> "tixHListSingle:Rec:DragOver %W %S %x %y"
tixDropBind TixHListSingle <Out>  "tixHListSingle:Rec:DragOut %W %S %x %y"
tixDropBind TixHListSingle <Drop> "tixHListSingle:Rec:Drop %W %S %x %y"
