# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Grid.tcl,v 1.6 2004/03/28 02:44:57 hobbs Exp $
#
# Grid.tcl --
#
# 	This file defines the default bindings for Tix Grid widgets.
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
# Copyright (c) 2004 ActiveState
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
foreach fun {tkCancelRepeat} {
    if {![llength [info commands $fun]]} {
	tk::unsupported::ExposePrivateCommand $fun
    }
}
unset fun

proc tixGridBind {} {
    tixBind TixGrid <ButtonPress-1> {
	tixGrid:Button-1 %W %x %y
    }
    tixBind TixGrid <Shift-ButtonPress-1> {
	tixGrid:Shift-Button-1 %W %x %y
    }
    tixBind TixGrid <Control-ButtonPress-1> {
	tixGrid:Control-Button-1 %W %x %y
    }
    tixBind TixGrid <ButtonRelease-1> {
	tixGrid:ButtonRelease-1 %W %x %y
    }
    tixBind TixGrid <Double-ButtonPress-1> {
	tixGrid:Double-1 %W  %x %y
    }
    tixBind TixGrid <B1-Motion> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixGrid:B1-Motion %W %x %y
    }
    tixBind TixGrid <Control-B1-Motion> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixGrid:Control-B1-Motion %W %x %y
    }
    tixBind TixGrid <B1-Leave> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixGrid:B1-Leave %W
    }
    tixBind TixGrid <B1-Enter> {
	tixGrid:B1-Enter %W %x %y
    }
    tixBind TixGrid <Control-B1-Leave> {
	set tkPriv(x) %x 
	set tkPriv(y) %y
	set tkPriv(X) %X
	set tkPriv(Y) %Y

	tixGrid:Control-B1-Leave %W
    }
    tixBind TixGrid <Control-B1-Enter> {
	tixGrid:Control-B1-Enter %W %x %y
    }

    # Keyboard bindings
    #
    tixBind TixGrid <Up> {
	tixGrid:DirKey %W up
    }
    tixBind TixGrid <Down> {
	tixGrid:DirKey %W down
    }
    tixBind TixGrid <Left> {
	tixGrid:DirKey %W left
    }
    tixBind TixGrid <Right> {
	tixGrid:DirKey %W right
    }
    tixBind TixGrid <Prior> {
	%W yview scroll -1 pages
    }
    tixBind TixGrid <Next> {
	%W yview scroll 1 pages
    }
    tixBind TixGrid <Return> {
	tixGrid:Return %W 
    }
    tixBind TixGrid <space> {
	tixGrid:Space %W 
    }
    #
    # Don't use tixBind because %A causes Tk 8.3.2 to crash
    #
    bind TixGrid <MouseWheel> {
        %W yview scroll [expr {- (%D / 120) * 2}] units
    }
}

#----------------------------------------------------------------------
#
#
#			 Mouse bindings
#
#
#----------------------------------------------------------------------

proc tixGrid:Button-1 {w x y} {
    if {[$w cget -state] eq "disabled"} {
	return
    }
    tixGrid:SetFocus $w

    if {[tixGrid:GetState $w] == 0} {
	tixGrid:GoState 1 $w $x $y
    }
}

proc tixGrid:Shift-Button-1 {w x y} {
    if {[$w cget -state] eq "disabled"} {
	return
    }
    tixGrid:SetFocus $w
}

proc tixGrid:Control-Button-1 {w x y} {
    if {[$w cget -state] eq "disabled"} {
	return
    }
    tixGrid:SetFocus $w

    case [tixGrid:GetState $w] {
	{s0} {
	    tixGrid:GoState s1 $w $x $y
       	}
	{b0} {
	    tixGrid:GoState b1 $w $x $y
       	}
	{m0} {
	    tixGrid:GoState m1 $w $x $y
       	}
	{e0} {
	    tixGrid:GoState e10 $w $x $y
       	}
    }
}

proc tixGrid:ButtonRelease-1 {w x y} {
    case [tixGrid:GetState $w] {
	{2} {
	    tixGrid:GoState 5 $w $x $y
	}
	{4} {
	    tixGrid:GoState 3 $w $x $y
	}
    }
}

proc tixGrid:B1-Motion {w x y} {
    case [tixGrid:GetState $w] {
	{2 4} {
	    tixGrid:GoState 4 $w $x $y
	}
    }
}

proc tixGrid:Control-B1-Motion {w x y} {
    case [tixGrid:GetState $w] {
	{s2 s4} {
	    tixGrid:GoState s4 $w $x $y 
	}
	{b2 b4} {
	    tixGrid:GoState b4 $w $x $y 
	}
	{m2 m5} {
	    tixGrid:GoState m4 $w $x $y 
	}
    }
}

proc tixGrid:Double-1 {w x y} {
    case [tixGrid:GetState $w] {
	{s0} {
	    tixGrid:GoState s7 $w $x $y
	}
	{b0} {
	    tixGrid:GoState b7 $w $x $y
	}
    }
}

proc tixGrid:B1-Leave {w} {
    case [tixGrid:GetState $w] {
	{s2 s4} {
	    tixGrid:GoState s5 $w
	}
	{b2 b4} {
	    tixGrid:GoState b5 $w
	}
	{m2 m5} {
	    tixGrid:GoState m8 $w
	}
	{e2 e5} {
	    tixGrid:GoState e8 $w
	}
    }
}

proc tixGrid:B1-Enter {w x y} {
    case [tixGrid:GetState $w] {
	{s5 s6} {
	    tixGrid:GoState s4 $w $x $y
	}
	{b5 b6} {
	    tixGrid:GoState b4 $w $x $y
	}
	{m8 m9} {
	    tixGrid:GoState m4 $w $x $y
	}
	{e8 e9} {
	    tixGrid:GoState e4 $w $x $y
	}
    }
}

proc tixGrid:Control-B1-Leave {w} {
    case [tixGrid:GetState $w] {
	{s2 s4} {
	    tixGrid:GoState s5 $w
	}
	{b2 b4} {
	    tixGrid:GoState b5 $w
	}
	{m2 m5} {
	    tixGrid:GoState m8 $w
	}
    }
}

proc tixGrid:Control-B1-Enter {w x y} {
    case [tixGrid:GetState $w] {
	{s5 s6} {
	    tixGrid:GoState s4 $w $x $y
	}
	{b5 b6} {
	    tixGrid:GoState b4 $w $x $y
	}
	{m8 m9} {
	    tixGrid:GoState m4 $w $x $y
	}
    }
}

proc tixGrid:AutoScan {w} {
    case [tixGrid:GetState $w] {
	{s5 s6} {
	    tixGrid:GoState s6 $w
	}
	{b5 b6} {
	    tixGrid:GoState b6 $w
	}
	{m8 m9} {
	    tixGrid:GoState m9 $w
	}
	{e8 e9} {
	    tixGrid:GoState e9 $w
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
proc tixGrid:DirKey {w key} {
    if {[$w cget -state] eq "disabled"} {
	return
    }
    case [tixGrid:GetState $w] {
	{s0} {
	    tixGrid:GoState s8 $w $key
       	}
	{b0} {
	    tixGrid:GoState b8 $w $key
       	}
    }
}

proc tixGrid:Return {w} {
    if {[$w cget -state] eq "disabled"} {
	return
    }
    case [tixGrid:GetState $w] {
	{s0} {
	    tixGrid:GoState s9 $w
       	}
	{b0} {
	    tixGrid:GoState b9 $w
       	}
    }
}

proc tixGrid:Space {w} {
    if {[$w cget -state] eq "disabled"} {
	return
    }
    case [tixGrid:GetState $w] {
	{s0} {
	    tixGrid:GoState s10 $w
       	}
 	{b0} {
	    tixGrid:GoState b10 $w
       	}
   }
}

#----------------------------------------------------------------------
#
#			STATE MANIPULATION
#
#
#----------------------------------------------------------------------
proc tixGrid:GetState {w} {
    global $w:priv:state

    if {![info exists $w:priv:state]} {
	set $w:priv:state 0
    }
    return [set $w:priv:state]
}

proc tixGrid:SetState {w n} {
    global $w:priv:state

    set $w:priv:state $n
}

proc tixGrid:GoState {n w args} {

#   puts "going from [tixGrid:GetState $w] --> $n"

    tixGrid:SetState $w $n
    eval tixGrid:GoState-$n $w $args
}

#----------------------------------------------------------------------
#		   SELECTION ROUTINES
#----------------------------------------------------------------------
proc tixGrid:SelectSingle {w ent} {
    $w selection set [lindex $ent 0] [lindex $ent 1]
    tixGrid:CallBrowseCmd $w $ent
}

#----------------------------------------------------------------------
#	SINGLE SELECTION
#----------------------------------------------------------------------
proc tixGrid:GoState-0 {w} {
    set list $w:_list
    global $list

    if {[info exists $list]} {
	foreach cmd [set $list] {
	    uplevel #0 $cmd
	}
	if {[info exists $list]} {
	    unset $list
	}
    }
}

proc tixGrid:GoState-1 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	tixGrid:SetAnchor $w $ent
    }
    tixGrid:CheckEdit $w
    $w selection clear 0 0 max max

    if {[$w cget -selectmode] ne "single"} {
	tixGrid:SelectSingle $w $ent
    }
    tixGrid:GoState 2 $w
}

proc tixGrid:GoState-2 {w} {
}

proc tixGrid:GoState-3 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	tixGrid:SelectSingle $w $ent
    }
    tixGrid:GoState 0 $w
}

proc tixGrid:GoState-5 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	tixGrid:SelectSingle $w $ent
	tixGrid:SetEdit $w $ent
    }
    tixGrid:GoState 0 $w
}


proc tixGrid:GoState-4 {w x y} {
    set ent [$w nearest $x $y]

    case [$w cget -selectmode] {
	single {
	    tixGrid:SetAnchor $w $ent
	}
	browse {
	    tixGrid:SetAnchor $w $ent
	    $w selection clear 0 0 max max
	    tixGrid:SelectSingle $w $ent
	}
	{multiple extended} {
	    set anchor [$w anchor get]
	    $w selection adjust [lindex $anchor 0] [lindex $anchor 1] \
		[lindex $ent 0] [lindex $ent 1]
	}
    }
}

proc tixGrid:GoState-s5 {w} {
    tixGrid:StartScan $w
}

proc tixGrid:GoState-s6 {w} {
    global tkPriv

    tixGrid:DoScan $w
}

proc tixGrid:GoState-s7 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixGrid:CallCommand $w $ent
    }
    tixGrid:GoState s0 $w
}

proc tixGrid:GoState-s8 {w key} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
    } else {
	set anchor [$w info $key $anchor]
    }

    $w anchor set $anchor
    $w see $anchor
    tixGrid:GoState s0 $w
}

proc tixGrid:GoState-s9 {w} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
	$w anchor set $anchor
	$w see $anchor
    }

    if {[$w info anchor] != ""} {
	# ! may not have any elements
	#
	tixGrid:CallCommand $w [$w info anchor]
	$w selection clear 
	$w selection set $anchor
    }

    tixGrid:GoState s0 $w
}

proc tixGrid:GoState-s10 {w} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
	$w anchor set $anchor
	$w see $anchor
    }

    if {[$w info anchor] != ""} {
	# ! may not have any elements
	#
	tixGrid:CallBrowseCmd $w [$w info anchor]
	$w selection clear 
	$w selection set $anchor
    }

    tixGrid:GoState s0 $w
}

#----------------------------------------------------------------------
#	BROWSE SELECTION
#----------------------------------------------------------------------
proc tixGrid:GoState-b0 {w} {
}

proc tixGrid:GoState-b1 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
	$w selection clear
	$w selection set $ent
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState b2 $w
}

proc tixGrid:GoState-b2 {w} {
}

proc tixGrid:GoState-b3 {w} {
    set ent [$w info anchor]
    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState b0 $w
}

proc tixGrid:GoState-b4 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
	$w selection clear
	$w selection set $ent
	tixGrid:CallBrowseCmd $w $ent
    }
}

proc tixGrid:GoState-b5 {w} {
    tixGrid:StartScan $w
}

proc tixGrid:GoState-b6 {w} {
    global tkPriv

    tixGrid:DoScan $w
}

proc tixGrid:GoState-b7 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixGrid:CallCommand $w $ent
    }
    tixGrid:GoState b0 $w
}

proc tixGrid:GoState-b8 {w key} {
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

    tixGrid:CallBrowseCmd $w $anchor
    tixGrid:GoState b0 $w
}

proc tixGrid:GoState-b9 {w} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
	$w anchor set $anchor
	$w see $anchor
    }

    if {[$w info anchor] != ""} {
	# ! may not have any elements
	#
	tixGrid:CallCommand $w [$w info anchor]
	$w selection clear 
	$w selection set $anchor
    }

    tixGrid:GoState b0 $w
}

proc tixGrid:GoState-b10 {w} {
    set anchor [$w info anchor]

    if {$anchor == ""} {
	set anchor 0
	$w anchor set $anchor
	$w see $anchor
    }

    if {[$w info anchor] != ""} {
	# ! may not have any elements
	#
	tixGrid:CallBrowseCmd $w [$w info anchor]
	$w selection clear 
	$w selection set $anchor
    }

    tixGrid:GoState b0 $w
}

#----------------------------------------------------------------------
#	MULTIPLE SELECTION
#----------------------------------------------------------------------
proc tixGrid:GoState-m0 {w} {
}

proc tixGrid:GoState-m1 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
	$w selection clear
	$w selection set $ent
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState m2 $w
}

proc tixGrid:GoState-m2 {w} {
}

proc tixGrid:GoState-m3 {w} {
    set ent [$w info anchor]
    if {$ent != ""} {
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState m0 $w
}

proc tixGrid:GoState-m4 {w x y} {
    set from [$w info anchor]
    set to   [$w nearest $x $y]
    if {$to != ""} {
	$w selection clear
	$w selection set $from $to
	tixGrid:CallBrowseCmd $w $to
    }
    tixGrid:GoState m5 $w
}

proc tixGrid:GoState-m5 {w} {
}

proc tixGrid:GoState-m6 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState m0 $w  
}

proc tixGrid:GoState-m7 {w x y} {
    set from [$w info anchor]
    set to   [$w nearest $x $y]
    if {$from == ""} {
	set from $to
	$w anchor set $from
    }
    if {$to != ""} {
	$w selection clear
	$w selection set $from $to
	tixGrid:CallBrowseCmd $w $to
    }
    tixGrid:GoState m5 $w
}


proc tixGrid:GoState-m8 {w} {
    tixGrid:StartScan $w
}

proc tixGrid:GoState-m9 {w} {
    tixGrid:DoScan $w
}

proc tixGrid:GoState-xm7 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixGrid:CallCommand $w $ent
    }
    tixGrid:GoState m0 $w
}

#----------------------------------------------------------------------
#	EXTENDED SELECTION
#----------------------------------------------------------------------
proc tixGrid:GoState-e0 {w} {
}

proc tixGrid:GoState-e1 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	$w anchor set $ent
	$w selection clear
	$w selection set $ent
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState e2 $w
}

proc tixGrid:GoState-e2 {w} {
}

proc tixGrid:GoState-e3 {w} {
    set ent [$w info anchor]
    if {$ent != ""} {
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState e0 $w
}

proc tixGrid:GoState-e4 {w x y} {
    set from [$w info anchor]
    set to   [$w nearest $x $y]
    if {$to != ""} {
	$w selection clear
	$w selection set $from $to
	tixGrid:CallBrowseCmd $w $to
    }
    tixGrid:GoState e5 $w
}

proc tixGrid:GoState-e5 {w} {
}

proc tixGrid:GoState-e6 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState e0 $w  
}

proc tixGrid:GoState-e7 {w x y} {
    set from [$w info anchor]
    set to   [$w nearest $x $y]
    if {$from == ""} {
	set from $to
	$w anchor set $from
    }
    if {$to != ""} {
	$w selection clear
	$w selection set $from $to
	tixGrid:CallBrowseCmd $w $to
    }
    tixGrid:GoState e5 $w
}


proc tixGrid:GoState-e8 {w} {
    tixGrid:StartScan $w
}

proc tixGrid:GoState-e9 {w} {
    tixGrid:DoScan $w
}

proc tixGrid:GoState-e10 {w x y} {
    set ent [$w nearest $x $y]
    if {$ent != ""} {
	if {[$w info anchor] == ""} {
	    $w anchor set $ent
	}
	if {[$w selection includes $ent]} {
	    $w selection clear $ent
	} else {
	    $w selection set $ent
	}
	tixGrid:CallBrowseCmd $w $ent
    }
    tixGrid:GoState e2 $w
}

proc tixGrid:GoState-xm7 {w x y} {
    set ent [$w nearest $x $y]

    if {$ent != ""} {
	$w selection clear
	$w selection set $ent
	tixGrid:CallCommand $w $ent
    }
    tixGrid:GoState e0 $w
}

#----------------------------------------------------------------------
#	HODGE PODGE
#----------------------------------------------------------------------

proc tixGrid:GoState-12 {w x y} {
    tkCancelRepeat
    tixGrid:GoState 5 $w $x $y 
}

proc tixGrid:GoState-13 {w ent oldEnt} {
    global tkPriv
    set tkPriv(tix,indicator) $ent
    set tkPriv(tix,oldEnt)    $oldEnt
    tixGrid:IndicatorCmd $w <Arm> $ent
}

proc tixGrid:GoState-14 {w x y} {
    global tkPriv

    if {[tixGrid:InsideArmedIndicator $w $x $y]} {
	$w anchor set $tkPriv(tix,indicator)
	$w select clear
	$w select set $tkPriv(tix,indicator)
	tixGrid:IndicatorCmd $w <Activate> $tkPriv(tix,indicator)
    } else {
	tixGrid:IndicatorCmd $w <Disarm>   $tkPriv(tix,indicator)
    }

    unset tkPriv(tix,indicator)
    tixGrid:GoState 0 $w
}

proc tixGrid:GoState-16 {w ent} {
    if {$ent == ""} {
	return
    }
    if {[$w cget -selectmode] ne "single"} {
	tixGrid:Select $w $ent
	tixGrid:Browse $w $ent
    }
}

proc tixGrid:GoState-18 {w} {
    global tkPriv
    tkCancelRepeat
    tixGrid:GoState 6 $w $tkPriv(x) $tkPriv(y) 
}

proc tixGrid:GoState-20 {w x y} {
    global tkPriv

    if {![tixGrid:InsideArmedIndicator $w $x $y]} {
	tixGrid:GoState 21 $w $x $y
    } else {
	tixGrid:IndicatorCmd $w <Arm> $tkPriv(tix,indicator)
    }
}

proc tixGrid:GoState-21 {w x y} {
    global tkPriv

    if {[tixGrid:InsideArmedIndicator $w $x $y]} {
	tixGrid:GoState 20 $w $x $y
    } else {
	tixGrid:IndicatorCmd $w <Disarm> $tkPriv(tix,indicator)
    }
}

proc tixGrid:GoState-22 {w} {
    global tkPriv

    if {$tkPriv(tix,oldEnt) != ""} {
	$w anchor set $tkPriv(tix,oldEnt)
    } else {
	$w anchor clear
    }
    tixGrid:GoState 0 $w
}


#----------------------------------------------------------------------
#			callback actions
#----------------------------------------------------------------------
proc tixGrid:SetAnchor {w ent} {
    if {$ent ne ""} {
	$w anchor set [lindex $ent 0] [lindex $ent 1]
#	$w see $ent
    }
}

proc tixGrid:Select {w ent} {
    $w selection clear
    $w select set $ent
}

proc tixGrid:StartScan {w} {
    global tkPriv
    set tkPriv(afterId) [after 50 tixGrid:AutoScan $w]
}

proc tixGrid:DoScan {w} {
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
	set tkPriv(afterId) [after 50 tixGrid:AutoScan $w]
    }
}

proc tixGrid:CallBrowseCmd {w ent} {
    return

    set browsecmd [$w cget -browsecmd]
    if {$browsecmd != ""} {
	set bind(specs) {%V}
	set bind(%V)    $ent

	tixEvalCmdBinding $w $browsecmd bind $ent
    }
}

proc tixGrid:CallCommand {w ent} {
    set command [$w cget -command]
    if {$command != ""} {
	set bind(specs) {%V}
	set bind(%V)    $ent

	tixEvalCmdBinding $w $command bind $ent
    }
}

# tixGrid:EditCell --
#
#	This command is called when "$w edit set $x $y" is called. It causes
#	an SetEdit call when the grid's state is 0.
#
proc tixGrid:EditCell {w x y} {
    set list $w:_list
    global $list

    if {[tixGrid:GetState $w] == 0} {
	tixGrid:SetEdit $w [list $x $y]
    } else {
	lappend $list [list tixGrid:SetEdit $w [list $x $y]]
    }
}

# tixGrid:EditApply --
#
#	This command is called when "$w edit apply $x $y" is called. It causes
#	an CheckEdit call when the grid's state is 0.
#
proc tixGrid:EditApply {w} {
    set list $w:_list
    global $list

    if {[tixGrid:GetState $w] == 0} {
	tixGrid:CheckEdit $w
    } else {
	lappend $list [list tixGrid:CheckEdit $w]
    }
}

# tixGrid:CheckEdit --
#
#	This procedure is called when the user sets the focus on a cell.
#	If another cell is being edited, apply the changes of that cell.
#
proc tixGrid:CheckEdit {w} {
    set edit $w.tixpriv__edit
    if {[winfo exists $edit]} {
	#
	# If it -command is not empty, it is being used for another cell.
	# Invoke it so that the other cell can be updated.
	#
	if {[$edit cget -command] ne ""} {
	    $edit invoke
	}
    }
}

# tixGrid:SetEdit --
#
#	Puts a floatentry on top of an editable entry.
#
proc tixGrid:SetEdit {w ent} {
    set edit $w.tixpriv__edit
    tixGrid:CheckEdit $w

    set editnotifycmd [$w cget -editnotifycmd]
    if {$editnotifycmd eq ""} {
	return
    }
    set px [lindex $ent 0]
    set py [lindex $ent 1]

    if {![uplevel #0 $editnotifycmd $px $py]} {
	return
    }
    if {[$w info exists $px $py]} {
	if [catch {
	    set oldValue [$w entrycget $px $py -text]
	}] {
	    # The entry doesn't support -text option. Can't edit it.
	    #
	    # If the application wants to force editing of an entry, it could
	    # delete or replace the entry in the editnotifyCmd procedure.
	    #
	    return
	}
    } else {
	set oldValue ""
    }

    set bbox [$w info bbox [lindex $ent 0] [lindex $ent 1]]
    set x [lindex $bbox 0]
    set y [lindex $bbox 1]
    set W [lindex $bbox 2]
    set H [lindex $bbox 3]

    if {![winfo exists $edit]} {
	tixFloatEntry $edit
    }

    $edit config -command "tixGrid:DoneEdit $w $ent"
    $edit post $x $y $W $H

    $edit config -value $oldValue
}

proc tixGrid:DoneEdit {w x y args} {
    set edit $w.tixpriv__edit
    $edit config -command ""
    $edit unpost

    set value [tixEvent value]
    if {[$w info exists $x $y]} {
	if [catch {
	    $w entryconfig $x $y -text $value
	}] {
	    return
	}
    } elseif {$value ne ""} {
	if {[catch {
	    # This needs to be catch'ed because the default itemtype may
	    # not support the -text option
	    #
	    $w set $x $y -text $value
	}]} {
	    return
	}
    } else {
	return
    }

    set editDoneCmd [$w cget -editdonecmd]
    if {$editDoneCmd ne ""} {
	uplevel #0 $editDoneCmd $x $y
    }
}

proc tixGrid:SetFocus {w} {
    if {[$w cget -takefocus] && ![string match $w.* [focus -displayof $w]]} {
	focus $w
    }
}
