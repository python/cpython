# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: TList.tcl,v 1.6 2002/01/24 09:13:58 idiscovery Exp $
#
# TList.tcl --
#
#	This file defines the default bindings for Tix Tabular Listbox
#	widgets.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

global tkPriv
if {![llength [info globals tkPriv]]} {
    tk::unsupported::ExposePrivateVariable tkPriv
}

#--------------------------------------------------------------------------
# tkPriv elements used in this file:
#
# afterId -		Token returned by "after" for autoscanning.
# fakeRelease -		Cancel the ButtonRelease-1 after the user double click
#--------------------------------------------------------------------------
#
proc tixTListBind {} {
    tixBind TixTList <ButtonPress-1> {
	tixTList:Button-1 %W %x %y
    }
    tixBind TixTList <Shift-ButtonPress-1> {
	tixTList:Shift-Button-1 %W %x %y
    }
    tixBind TixTList <Control-ButtonPress-1> {
	tixTList:Control-Button-1 %W %x %y
    }
    tixBind TixTList <ButtonRelease-1> {
	tixTList:ButtonRelease-1 %W %x %y
    }
    tixBind TixTList <Double-ButtonPress-1> {
	tixTList:Double-1 %W  %x %y
    }
    tixBind TixTList <B1-Motion> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixTList:B1-Motion %W %x %y
    }
    tixBind TixTList <Control-B1-Motion> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixTList:Control-B1-Motion %W %x %y
    }
    tixBind TixTList <B1-Leave> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixTList:B1-Leave %W
    }
    tixBind TixTList <B1-Enter> {
	tixTList:B1-Enter %W %x %y
    }
    tixBind TixTList <Control-B1-Leave> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixTList:Control-B1-Leave %W
    }
    tixBind TixTList <Control-B1-Enter> {
	tixTList:Control-B1-Enter %W %x %y
    }

    # Keyboard bindings
    #
    tixBind TixTList <Up> {
	tixTList:DirKey %W up
    }
    tixBind TixTList <Down> {
	tixTList:DirKey %W down
    }
    tixBind TixTList <Left> {
	tixTList:DirKey %W left
    }
    tixBind TixTList <Right> {
	tixTList:DirKey %W right
    }
    tixBind TixTList <Prior> {
	%W yview scroll -1 pages
    }
    tixBind TixTList <Next> {
	%W yview scroll 1 pages
    }
    tixBind TixTList <Return> {
	tixTList:Return %W 
    }
    tixBind TixTList <space> {
	tixTList:Space %W 
    }
    #
    # Don't use tixBind because %A causes Tk 8.3.2 to crash
    #
    bind TixTList <MouseWheel> {
        if {"[%W cget -orient]" == "vertical"} {
            %W xview scroll [expr {- (%D / 120) * 4}] units
        } else {
            %W yview scroll [expr {- (%D / 120) * 2}] units
        }
    }
}

#----------------------------------------------------------------------
#
#
#			 Mouse bindings
#
#
#----------------------------------------------------------------------

proc tixTList:Button-1 {w x y} {
    if {[$w cget -state] == "disabled"} {
	return
    }
    if {[$w cget -takefocus]} {
	focus $w
    }
    case [tixTList:GetState $w] {
	{s0} {
	    tixTList:GoState s1 $w $x $y
       	}
	{b0} {
	    tixTList:GoState b1 $w $x $y
       	}
	{m0} {
	    tixTList:GoState m1 $w $x $y
       	}
	{e0} {
	    tixTList:GoState e1 $w $x $y
       	}
    }
}

proc tixTList:Shift-Button-1 {w x y} {
    if {[$w cget -state] == "disabled"} {
	return
    }
    if {[$w cget -takefocus]} {
	focus $w
    }
    case [tixTList:GetState $w] {
	{s0} {
	    tixTList:GoState s1 $w $x $y
       	}
	{b0} {
	    tixTList:GoState b1 $w $x $y
       	}
	{m0} {
	    tixTList:GoState m7 $w $x $y
       	}
	{e0} {
	    tixTList:GoState e7 $w $x $y
       	}
    }
}

proc tixTList:Control-Button-1 {w x y} {
    if {[$w cget -state] == "disabled"} {
	return
    }
    if {[$w cget -takefocus]} {
	focus $w
    }
    case [tixTList:GetState $w] {
	{s0} {
	    tixTList:GoState s1 $w $x $y
       	}
	{b0} {
	    tixTList:GoState b1 $w $x $y
       	}
	{m0} {
	    tixTList:GoState m1 $w $x $y
       	}
	{e0} {
	    tixTList:GoState e10 $w $x $y
       	}
    }
}

proc tixTList:ButtonRelease-1 {w x y} {
    case [tixTList:GetState $w] {
	{s2 s4 s5 s6} {
	    tixTList:GoState s3 $w
	}
	{b2 b4 b5 b6} {
	    tixTList:GoState b3 $w
	}
	{m2} {
	    tixTList:GoState m3 $w
	}
	{m5} {
	    tixTList:GoState m6 $w $x $y
	}
	{m9} {
	    tixTList:GoState m0 $w
	}
	{e2} {
	    tixTList:GoState e3 $w
	}
	{e5} {
	    tixTList:GoState e6 $w $x $y
	}
	{e9} {
	    tixTList:GoState e0 $w
	}
    }
}

proc tixTList:B1-Motion {w x y} {
    case [tixTList:GetState $w] {
	{s2 s4} {
	    tixTList:GoState s4 $w $x $y 
	}
	{b2 b4} {
	    tixTList:GoState b4 $w $x $y 
	}
	{m2 m5} {
	    tixTList:GoState m4 $w $x $y 
	}
	{e2 e5} {
	    tixTList:GoState e4 $w $x $y 
	}
    }
}

proc tixTList:Control-B1-Motion {w x y} {
    case [tixTList:GetState $w] {
	{s2 s4} {
	    tixTList:GoState s4 $w $x $y 
	}
	{b2 b4} {
	    tixTList:GoState b4 $w $x $y 
	}
	{m2 m5} {
	    tixTList:GoState m4 $w $x $y 
	}
    }
}

proc tixTList:Double-1 {w x y} {
    case [tixTList:GetState $w] {
	{s0} {
	    tixTList:GoState s7 $w $x $y
	}
	{b0} {
	    tixTList:GoState b7 $w $x $y
	}
    }
}

proc tixTList:B1-Leave {w} {
    case [tixTList:GetState $w] {
	{s2 s4} {
	    tixTList:GoState s5 $w
	}
	{b2 b4} {
	    tixTList:GoState b5 $w
	}
	{m2 m5} {
	    tixTList:GoState m8 $w
	}
	{e2 e5} {
	    tixTList:GoState e8 $w
	}
    }
}

proc tixTList:B1-Enter {w x y} {
    case [tixTList:GetState $w] {
	{s5 s6} {
	    tixTList:GoState s4 $w $x $y
	}
	{b5 b6} {
	    tixTList:GoState b4 $w $x $y
	}
	{m8 m9} {
	    tixTList:GoState m4 $w $x $y
	}
	{e8 e9} {
	    tixTList:GoState e4 $w $x $y
	}
    }
}

proc tixTList:Control-B1-Leave {w} {
    case [tixTList:GetState $w] {
	{s2 s4} {
	    tixTList:GoState s5 $w
	}
	{b2 b4} {
	    tixTList:GoState b5 $w
	}
	{m2 m5} {
	    tixTList:GoState m8 $w
	}
    }
}

proc tixTList:Control-B1-Enter {w x y} {
    case [tixTList:GetState $w] {
	{s5 s6} {
	    tixTList:GoState s4 $w $x $y
	}
	{b5 b6} {
	    tixTList:GoState b4 $w $x $y
	}
	{m8 m9} {
	    tixTList:GoState m4 $w $x $y
	}
    }
}

proc tixTList:AutoScan {w} {
    case [tixTList:GetState $w] {
	{s5 s6} {
	    tixTList:GoState s6 $w
	}
	{b5 b6} {
	    tixTList:GoState b6 $w
	}
	{m8 m9} {
	    tixTList:GoState m9 $w
	}
	{e8 e9} {
	    tixTList:GoState e9 $w
	}
    }
}

#----------------------------------------------------------------------
#
#
#			 Key bindings
#
#
#----------------------------------------------------------------------
proc tixTList:DirKey {w key} {
    if {[$w cget -state] == "disabled"} {
	return
    }
    case [tixTList:GetState $w] {
	{s0} {
	    tixTList:GoState s8 $w $key
       	}
	{b0} {
	    tixTList:GoState b8 $w $key
       	}
    }
}

proc tixTList:Return {w} {
    if {[$w cget -state] == "disabled"} {
	return
    }
    case [tixTList:GetState $w] {
	{s0} {
	    tixTList:GoState s9 $w
       	}
	{b0} {
	    tixTList:GoState b9 $w
       	}
    }
}

proc tixTList:Space {w} {
    if {[$w cget -state] == "disabled"} {
	return
    }
    case [tixTList:GetState $w] {
	{s0} {
	    tixTList:GoState s10 $w
       	}
 	{b0} {
	    tixTList:GoState b10 $w
       	}
   }
}

#----------------------------------------------------------------------
#
#			STATE MANIPULATION
#
#
#----------------------------------------------------------------------
proc tixTList:GetState {w} {
    global $w:priv:state

    if {[info exists $w:priv:state]} {
        #
        # If the app has changed the selectmode, reset the state to the
        # original state.
        #
        set type [string index [$w cget -selectmode] 0]
        if {"[string index [set $w:priv:state] 0]" != "$type"} {
            unset $w:priv:state
        }
    }

    if {![info exists $w:priv:state]} {
	case [$w cget -selectmode] {
	    single {
		set $w:priv:state s0
	    }
	    browse {
		set $w:priv:state b0
	    }
	    multiple {
		set $w:priv:state m0
	    }
	    extended {
		set $w:priv:state e0
	    }
	    default {
		set $w:priv:state unknown
	    }
	}
    }
    return [set $w:priv:state]
}

proc tixTList:SetState {w n} {
    global $w:priv:state

    set $w:priv:state $n
}

proc tixTList:GoState {n w args} {

#   puts "going from [tixTList:GetState $w] --> $n"

    tixTList:SetState $w $n
    eval tixTList:GoState-$n $w $args
}

#----------------------------------------------------------------------
#			States
#----------------------------------------------------------------------

#----------------------------------------------------------------------
#	SINGLE SELECTION
#----------------------------------------------------------------------
proc tixTList:GoState-s0 {w} {
}

proc tixTList:GoState-s1 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
        $w see $ent
    }
    tixTList:GoState s2 $w
}

proc tixTList:GoState-s2 {w} {
}

proc tixTList:GoState-s3 {w} {
    set ent [$w info anchor]
    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState s0 $w
}

proc tixTList:GoState-s4 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
        $w see $ent
    }
}

proc tixTList:GoState-s5 {w} {
    tixTList:StartScan $w
}

proc tixTList:GoState-s6 {w} {
    global tkPriv

    tixTList:DoScan $w
}

proc tixTList:GoState-s7 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixTList:CallCommand $w $ent
    }
    tixTList:GoState s0 $w
}

proc tixTList:GoState-s8 {w key} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
    } else {
	set anchor [$w info $key $anchor]
    }

    $w anchor set $anchor
    $w see $anchor
    tixTList:GoState s0 $w
}

proc tixTList:GoState-s9 {w} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
	$w anchor set $anchor
	$w see $anchor
    }

    if {[$w info anchor] != ""} {
	# ! may not have any elements
	#
	tixTList:CallCommand $w [$w info anchor]
	$w selection clear 
	$w selection set $anchor
    }

    tixTList:GoState s0 $w
}

proc tixTList:GoState-s10 {w} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
	$w anchor set $anchor
	$w see $anchor
    }

    if {[$w info anchor] != ""} {
	# ! may not have any elements
	#
	tixTList:CallBrowseCmd $w [$w info anchor]
	$w selection clear 
	$w selection set $anchor
    }

    tixTList:GoState s0 $w
}

#----------------------------------------------------------------------
#	BROWSE SELECTION
#----------------------------------------------------------------------
proc tixTList:GoState-b0 {w} {
}

proc tixTList:GoState-b1 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
        $w see $ent
	$w selection clear
	$w selection set $ent
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState b2 $w
}

proc tixTList:GoState-b2 {w} {
}

proc tixTList:GoState-b3 {w} {
    set ent [$w info anchor]
    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState b0 $w
}

proc tixTList:GoState-b4 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
        $w see $ent
	$w selection clear
	$w selection set $ent
	tixTList:CallBrowseCmd $w $ent
    }
}

proc tixTList:GoState-b5 {w} {
    tixTList:StartScan $w
}

proc tixTList:GoState-b6 {w} {
    global tkPriv

    tixTList:DoScan $w
}

proc tixTList:GoState-b7 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixTList:CallCommand $w $ent
    }
    tixTList:GoState b0 $w
}

proc tixTList:GoState-b8 {w key} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
    } else {
	set anchor [$w info $key $anchor]
    }

    $w anchor set $anchor
    $w selection clear
    $w selection set $anchor
    $w see $anchor

    tixTList:CallBrowseCmd $w $anchor
    tixTList:GoState b0 $w
}

proc tixTList:GoState-b9 {w} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
	$w anchor set $anchor
	$w see $anchor
    }

    if {[$w info anchor] != ""} {
	# ! may not have any elements
	#
	tixTList:CallCommand $w [$w info anchor]
	$w selection clear 
	$w selection set $anchor
    }

    tixTList:GoState b0 $w
}

proc tixTList:GoState-b10 {w} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
	$w anchor set $anchor
	$w see $anchor
    }

    if {[$w info anchor] != ""} {
	# ! may not have any elements
	#
	tixTList:CallBrowseCmd $w [$w info anchor]
	$w selection clear 
	$w selection set $anchor
    }

    tixTList:GoState b0 $w
}

#----------------------------------------------------------------------
#	MULTIPLE SELECTION
#----------------------------------------------------------------------
proc tixTList:GoState-m0 {w} {
}

proc tixTList:GoState-m1 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
        $w see $ent
	$w selection clear
	$w selection set $ent
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState m2 $w
}

proc tixTList:GoState-m2 {w} {
}

proc tixTList:GoState-m3 {w} {
    set ent [$w info anchor]
    if {$ent != ""} {
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState m0 $w
}

proc tixTList:GoState-m4 {w x y} {
    set from [$w info anchor]
    set to   [$w nearest $x $y]
    if {$to != ""} {
	$w selection clear
	$w selection set $from $to
	tixTList:CallBrowseCmd $w $to
    }
    tixTList:GoState m5 $w
}

proc tixTList:GoState-m5 {w} {
}

proc tixTList:GoState-m6 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState m0 $w  
}

proc tixTList:GoState-m7 {w x y} {
    set from [$w info anchor]
    set to   [$w nearest $x $y]
    if {$from == ""} {
	set from $to
	$w anchor set $from
        $w see $from
    }
    if {$to != ""} {
	$w selection clear
	$w selection set $from $to
	tixTList:CallBrowseCmd $w $to
    }
    tixTList:GoState m5 $w
}


proc tixTList:GoState-m8 {w} {
    tixTList:StartScan $w
}

proc tixTList:GoState-m9 {w} {
    tixTList:DoScan $w
}

proc tixTList:GoState-xm7 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixTList:CallCommand $w $ent
    }
    tixTList:GoState m0 $w
}

#----------------------------------------------------------------------
#	EXTENDED SELECTION
#----------------------------------------------------------------------
proc tixTList:GoState-e0 {w} {
}

proc tixTList:GoState-e1 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
        $w see $ent
	$w selection clear
	$w selection set $ent
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState e2 $w
}

proc tixTList:GoState-e2 {w} {
}

proc tixTList:GoState-e3 {w} {
    set ent [$w info anchor]
    if {$ent != ""} {
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState e0 $w
}

proc tixTList:GoState-e4 {w x y} {
    set from [$w info anchor]
    set to   [$w nearest $x $y]
    if {$to != ""} {
	$w selection clear
	$w selection set $from $to
	tixTList:CallBrowseCmd $w $to
    }
    tixTList:GoState e5 $w
}

proc tixTList:GoState-e5 {w} {
}

proc tixTList:GoState-e6 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState e0 $w  
}

proc tixTList:GoState-e7 {w x y} {
    set from [$w info anchor]
    set to   [$w nearest $x $y]
    if {$from == ""} {
	set from $to
	$w anchor set $from
        $w see $from
    }
    if {$to != ""} {
	$w selection clear
	$w selection set $from $to
	tixTList:CallBrowseCmd $w $to
    }
    tixTList:GoState e5 $w
}


proc tixTList:GoState-e8 {w} {
    tixTList:StartScan $w
}

proc tixTList:GoState-e9 {w} {
    tixTList:DoScan $w
}

proc tixTList:GoState-e10 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	if {[$w info anchor] == ""} {
	    $w anchor set $ent
	    $w see $ent
	}
	if {[$w selection includes $ent]} {
	    $w selection clear $ent
	} else {
	    $w selection set $ent
	}
	tixTList:CallBrowseCmd $w $ent
    }
    tixTList:GoState e2 $w
}

proc tixTList:GoState-xm7 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixTList:CallCommand $w $ent
    }
    tixTList:GoState e0 $w
}

#----------------------------------------------------------------------
#			callback actions
#----------------------------------------------------------------------
proc tixTList:SetAnchor {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != "" && [$w entrycget $ent -state] != "disabled"} {
	$w anchor set $ent
	$w see $ent
	return $ent
    }

    return ""
}

proc tixTList:Select {w ent} {
    $w selection clear
    $w select set $ent
}

proc tixTList:StartScan {w} {
    global tkPriv
    set tkPriv(afterId) [after 50 tixTList:AutoScan $w]
}

proc tixTList:DoScan {w} {
    global tkPriv
    set x $tkPriv(x)
    set y $tkPriv(y)
    set X $tkPriv(X)
    set Y $tkPriv(Y)

    set out 0
    if {$y >= [winfo height $w]} {
	$w yview scroll 1 units
	set out 1
    }
    if {$y < 0} {
	$w yview scroll -1 units
	set out 1
    }
    if {$x >= [winfo width $w]} {
	$w xview scroll 2 units
	set out 1
    } 
    if {$x < 0} {
	$w xview scroll -2 units
	set out 1
    }

    if {$out} {
	set tkPriv(afterId) [after 50 tixTList:AutoScan $w]
    }
}

proc tixTList:CallBrowseCmd {w ent} {
    set browsecmd [$w cget -browsecmd]
    if {$browsecmd != ""} {
	set bind(specs) {%V}
	set bind(%V)    $ent

	tixEvalCmdBinding $w $browsecmd bind $ent
    }
}

proc tixTList:CallCommand {w ent} {
    set command [$w cget -command]
    if {$command != ""} {
	set bind(specs) {%V}
	set bind(%V)    $ent

	tixEvalCmdBinding $w $command bind $ent
    }
}
