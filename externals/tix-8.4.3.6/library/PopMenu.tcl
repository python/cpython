# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: PopMenu.tcl,v 1.7 2004/03/28 02:44:57 hobbs Exp $
#
# PopMenu.tcl --
#
#	This file implements the TixPopupMenu widget
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
# inMenubutton -	
#--------------------------------------------------------------------------
#
foreach fun {tkMenuUnpost tkMbButtonUp tkMbEnter tkMbPost} {
    if {![llength [info commands $fun]]} {
	tk::unsupported::ExposePrivateCommand $fun
    }
}
unset fun

tixWidgetClass tixPopupMenu {
    -classname TixPopupMenu
    -superclass tixShell
    -method {
	bind post unbind
    }
    -flag {
	 -buttons -installcolormap -postcmd -spring -state -title
    }
    -configspec {
	{-buttons buttons Buttons {{3 {Any}}}}
	{-installcolormap installColormap InstallColormap false}
	{-postcmd postCmd PostCmd ""}
	{-spring spring Spring 1 tixVerifyBoolean}
	{-state state State normal}
	{-cursor corsor Cursor arrow}
    }
    -static {
	-buttons
    }
    -default  {
	{*Menu.tearOff			0}
    }
}

proc tixPopupMenu:InitWidgetRec {w} {
    upvar #0 $w data

    tixChainMethod $w InitWidgetRec

    set data(g:clients)   ""
}

proc tixPopupMenu:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    wm overrideredirect $w 1
    wm withdraw $w

    set data(w:menubutton) [menubutton $w.menubutton -text $data(-title) \
			    -menu $w.menubutton.menu -anchor w]
    set data(w:menu) [menu $w.menubutton.menu]

    pack $data(w:menubutton) -expand yes -fill both
}

proc tixPopupMenu:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    foreach elm $data(-buttons) {
	set btn [lindex $elm 0]
	foreach mod [lindex $elm 1] {
	    tixBind TixPopupMenu:MB:$w <$mod-ButtonPress-$btn> \
		"tixPopupMenu:Unpost $w"

	    tixBind TixPopupMenu:$w <$mod-ButtonPress-$btn> \
		"tixPopupMenu:post $w %W %x %y"
	}

	tixBind TixPopupMenu:MB:$w <ButtonRelease-$btn> \
	    "tixPopupMenu:BtnRelease $w %X %Y"

	tixBind TixPopupMenu:M:$w <Unmap> \
	    "tixPopupMenu:Unmap $w"
	tixBind TixPopupMenu:$w <ButtonRelease-$btn> \
	    "tixPopupMenu:BtnRelease $w %X %Y"

	tixAddBindTag $data(w:menubutton) TixPopupMenu:MB:$w
	tixAddBindTag $data(w:menu)       TixPopupMenu:M:$w
    }
}


#----------------------------------------------------------------------
# PrivateMethods:
#----------------------------------------------------------------------
proc tixPopupMenu:Unpost {w} {
    upvar #0 $w data

    catch {
	tkMenuUnpost ""
    }
#   tkMbButtonUp $data(w:menubutton)
}

proc tixPopupMenu:BtnRelease {w rootX rootY} {
    upvar #0 $w data

    set cW [winfo containing -displayof $w $rootX $rootY]

    if {$data(-spring)} {
	tixPopupMenu:Unpost $w
    }
}

proc tixPopupMenu:Unmap {w} {
    upvar #0 $w data
    wm withdraw $w
}

proc tixPopupMenu:Destructor {w} {
    upvar #0 $w data

    foreach client $data(g:clients) {
	if {[winfo exists $client]} {
	    tixDeleteBindTag $client TixPopupMenu:$w
	}
    }

    # delete the extra bindings
    #
    foreach tag [list TixPopupMenu:MB:$w TixPopupMenu:M:$w] {
	foreach e [bind $tag] {
	    bind $tag $e ""
	}
    }

    tixChainMethod $w Destructor
}

proc tixPopupMenu:config-title {w value} {
    upvar #0 $w data

    $data(w:menubutton) config -text $value
}

#----------------------------------------------------------------------
# PublicMethods:
#----------------------------------------------------------------------
proc tixPopupMenu:bind {w args} {
    upvar #0 $w data

    foreach client $args {
	if {[lsearch $data(g:clients) $client] == -1} {
	    lappend data(g:clients) $client
	    tixAppendBindTag $client TixPopupMenu:$w
	}
    }
}

proc tixPopupMenu:unbind {w args} {
    upvar #0 $w data

    foreach client $args {
	if {[winfo exists $client]} {
	    set index [lsearch $data(g:clients) $client]
	    if {$index != -1} {
		tixDeleteBindTag $client TixPopupMenu:$w
		set data(g:clients) [lreplace $data(g:clients) $index $index]
	    }
	}
    }
}

proc tixPopupMenu:post {w client x y} {
    upvar #0 $w data
    global tkPriv

    if {$data(-state)  == "disabled"} {
	return
    }

    set rootx [expr $x + [winfo rootx $client]]
    set rooty [expr $y + [winfo rooty $client]]

    if {$data(-postcmd) != ""} {
	set ret [tixEvalCmdBinding $w $data(-postcmd) "" $rootx $rooty]
	if {![tixGetBoolean $ret]} {
	    return
	}
    }

    if {[string is true -strict $data(-installcolormap)]} {
	wm colormapwindows . $w
    }


    set menuWidth [winfo reqwidth $data(w:menu)]
    set width     [winfo reqwidth  $w]
    set height    [winfo reqheight $w]

    if {$width < $menuWidth} {
	set width $menuWidth
    }

    set wx $rootx
    set wy $rooty

    # trick: the following lines allow the popup menu
    # acquire a stable width and height when it is finally
    # put on the visible screen. Advoid flashing
    #
    wm geometry $w +10000+10000
    wm deiconify $w
    raise $w

    update
    wm geometry $w ${width}x${height}+${wx}+${wy}
    update

    tkMbEnter $data(w:menubutton)
    tkMbPost $tkPriv(inMenubutton) $rootx $rooty
}
