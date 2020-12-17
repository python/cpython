# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: WmDefault.csc,v 1.2 2001/12/09 05:03:09 idiscovery Exp $
#
#

proc tixPref:SetScheme-Color:WmDefault {args} {
    global tixOption

    package require wm_default
    if {![info exists ::wm_default::wm]} {
	wm_default::setup
	wm_default::addoptions
    }

    set tixOption(bg)           $::wm_default::background
    set tixOption(fg)           $::wm_default::foreground

    # was "#808080"
    set tixOption(dark1_bg)     $::wm_default::disabledbackground

    set tixOption(inactive_bg)  $::wm_default::disabledbackground
    set tixOption(inactive_fg)  black; # unused

    # light1 was used for listbox widgets and trough colors
    set tixOption(light1_bg)    $::wm_default::scrollbars
    set tixOption(light1_fg)    white; #unused

    # text is now used for listbox widgets
    set tixOption(list_bg)   	$::wm_default::textbackground

    set tixOption(active_bg)    $::wm_default::activebackground
    set tixOption(active_fg)    $::wm_default::activeforeground

    set tixOption(disabled_fg)  $::wm_default::disabledforeground
    # new
    set tixOption(disabled_bg)  $::wm_default::disabledtextbackground

    set tixOption(textbackground)    $::wm_default::textbackground
    set tixOption(input1_fg)    $::wm_default::textforeground

    set tixOption(select_fg)    $::wm_default::selectforeground
    set tixOption(select_bg)    $::wm_default::selectbackground
	
    set tixOption(selector)	$::wm_default::selectcolor

    set pri $tixOption(prioLevel)

    # Try to give the subwidget (hlist) the highlightThickness 
    foreach pref {*TixDirTree *TixDirList *TixTree \
	    *TixScrolledListBox  \
	    *TixScrolledTList  *TixScrolledText} {
	option add $pref.highlightThickness 0 $pri
    }


    # necessary:
    option add *TixBalloon*background 			white $pri
    option add *TixBalloon*foreground 			black $pri
    option add *TixBalloon.background 			black $pri

    # necessary: but should be restricted
    # was -   option add *Label.anchor 				w $pri
    option add *TixBalloon*Label.anchor 		w $pri
    option add *TixComboBox*Label.anchor 		w $pri
    option add *TixFileEntry*Label.anchor 		w $pri
    option add *TixLabelEntry*Label.anchor 		w $pri
    option add *TixOptionMenu*Label.anchor 		w $pri

    option add *TixComboBox*background $tixOption(background) $pri
    option add *TixFileEntry*Entry.borderWidth		0 $pri
    option add *TixFileEntry.frame.background		$tixOption(textbackground) $pri

    option add *TixFileEntry*Entry.highlightBackground $::wm_default::highlightbackground $pri

    option add *TixOptionMenu*menubutton.relief raised $pri
    option add *TixOptionMenu*menubutton.borderWidth $::wm_default::borderwidth $pri
    option add *TixResizeHandle*background $tixOption(disabledbackground) $pri

    option add *handleActiveBg 		$::wm_default::selectbackground $pri


    # These may already have been covered by wm_default

    option add *TixControl*entry.insertBackground	$tixOption(textforeground) $pri

    option add *TixDirTree*hlist.activeBackground	$tixOption(light1_bg) $pri
    option add *TixDirTree*hlist.disabledBackground	$tixOption(disabled_bg) $pri
    option add *TixDirTree*f1.borderWidth		$::wm_default::borderwidth $pri
    option add *TixDirTree*f1.relief			sunken $pri

    option add *TixDirList*hlist.activeBackground	$tixOption(light1_bg) $pri
    option add *TixDirList*hlist.disabledBackground	$tixOption(disabled_bg) $pri
    option add *TixDirList*f1.borderWidth		$::wm_default::borderwidth $pri
    option add *TixDirList*f1.relief			sunken $pri

    option add *TixScrolledHList*hlist.activeBackground	$tixOption(light1_bg) $pri
    option add *TixScrolledHList*hlist.disabledBackground	$tixOption(disabled_bg) $pri
    option add *TixScrolledHList*f1.borderWidth		$::wm_default::borderwidth $pri
    option add *TixScrolledHList*f1.relief			sunken $pri

    option add *TixTree*hlist.background		$tixOption(textbackground) $pri
    option add *TixTree*hlist.activeBackground	$tixOption(light1_bg) $pri
    option add *TixTree*hlist.disabledBackground	$tixOption(disabled_bg) $pri
    option add *TixTree*f1.borderWidth		$::wm_default::borderwidth $pri
    option add *TixTree*f1.relief			sunken $pri

    option add *TixFileEntry.background 		$tixOption(background) $pri

    option add *TixHList.activeBackground		$tixOption(light1_bg) $pri
    option add *TixHList.disabledBackground		$tixOption(disabled_bg) $pri

    option add *TixLabelEntry*entry.background		$tixOption(textbackground) $pri
    option add *TixLabelEntry*entry.foreground		$tixOption(textforeground) $pri
    option add *TixLabelEntry*entry.insertBackground	$tixOption(textforeground) $pri

    option add *TixMultiView*Listbox.borderWidth		0 $pri
    option add *TixMultiView*Listbox.highlightThickness	0 $pri
    option add *TixMultiView*Scrollbar.relief		sunken $pri
    option add *TixMultiView*Scrollbar.width		15 $pri
    option add *TixMultiView*f1.borderWidth		2 $pri
    option add *TixMultiView*f1.relief			sunken $pri
    option add *TixMultiView*f1.highlightThickness		2 $pri

    option add *TixNoteBook.Background			$tixOption(background) $pri
    option add *TixNoteBook.nbframe.Background		$tixOption(background) $pri
    option add *TixNoteBook.nbframe.backPageColor		$tixOption(background) $pri
    option add *TixNoteBook.nbframe.inactiveBackground	$tixOption(disabledbackground) $pri
    option add *TixPanedWindow.handleActiveBg 		$tixOption(active_bg) $pri
#    option add *TixPanedWindow.seperatorBg    		$tixOption(disabledbackground) $pri
#    option add *TixPanedWindow.handleBg       		$tixOption(disabledbackground) $pri

    option add *TixPopupMenu*menubutton.background 	$tixOption(dark1_bg) $pri

    option add *TixScrolledTList*tlist.background		$tixOption(textbackground) $pri

    option add *TixScrolledListBox*listbox.background		$tixOption(textbackground) $pri

    option add *TixScrolledWindow.frame.background		$tixOption(list_bg) $pri

    option add *TixTree*hlist.highlightBackground		$tixOption(background) $pri
    option add *TixTree*hlist.background			$tixOption(textbackground) $pri
    option add *TixTree*hlist.borderWidth			$::wm_default::borderwidth $pri

    option add *TixComboBox*Entry.highlightBackground		$tixOption(background) $pri
    option add *TixComboBox*Entry.background			$tixOption(textbackground) $pri
    option add *TixComboBox*Entry.foreground			$tixOption(textforeground) $pri
    option add *TixComboBox*Entry.insertBackground		$tixOption(textforeground) $pri
}

proc tixPref:SetScheme-Mono:Gray {} {
    global tixOption

    package require wm_default
    if {![info exists ::wm_default::wm]} {
	wm_default::setup
	wm_default::addoptions
    }

    set tixOption(background)           lightgray
    set tixOption(foreground)           black

    set tixOption(dark1_bg)     gray70

    set tixOption(inactive_bg)  lightgray
    set tixOption(inactive_fg)  black

    set tixOption(light1_bg)    gray90
    set tixOption(light1_fg)    white

    set tixOption(active_bg)    gray90
    set tixOption(active_fg)    $tixOption(foreground)
    set tixOption(disabled_fg)  gray55

    set tixOption(textbackground)    $tixOption(light1_bg)

    set tixOption(select_fg)    white
    set tixOption(select_bg)    black

    set tixOption(selector)	black

    set pri $tixOption(prioLevel)

    # Override what you want with optional arguments to wm_default::adoptions

    # necessary:
    option add *TixBalloon*background 			white $pri
    option add *TixBalloon*foreground 			black $pri
    option add *TixBalloon.background 			black $pri

    # necessary: but should be restricted
    # was -   option add *Label.anchor 				w $pri
    option add *TixBalloon*Label.anchor 		w $pri
    option add *TixComboBox*Label.anchor 		w $pri
    option add *TixFileEntry*Label.anchor 		w $pri
    option add *TixLabelEntry*Label.anchor 		w $pri

#    option add *TixDirTree*hlist.highlightBackground	$tixOption(background) $pri
#    option add *TixDirTree*hlist.background		$tixOption(light1_bg) $pri
#    option add *TixDirTree*hlist.activeBackground	$tixOption(light1_bg) $pri
#    option add *TixDirTree*hlist.disabledBackground	$tixOption(disabled_bg) $pri
#    option add *TixDirTree*f1.borderWidth		$::wm_default::borderwidth $pri
    option add *TixDirTree*f1.relief			sunken $pri
#    option add *TixDirList*hlist.highlightBackground	$tixOption(background) $pri
#    option add *TixDirList*hlist.background		$tixOption(light1_bg) $pri
#    option add *TixDirList*hlist.activeBackground	$tixOption(light1_bg) $pri
#    option add *TixDirList*hlist.disabledBackground	$tixOption(disabled_bg) $pri
#    option add *TixDirList*f1.borderWidth		$::wm_default::borderwidth $pri
    option add *TixDirList*f1.relief			sunken $pri
#    option add *TixScrolledHList*hlist.highlightBackground	$tixOption(background) $pri
#    option add *TixScrolledHList*hlist.background		$tixOption(light1_bg) $pri
#    option add *TixScrolledHList*hlist.activeBackground	$tixOption(light1_bg) $pri
#    option add *TixScrolledHList*hlist.disabledBackground	$tixOption(disabled_bg) $pri
#    option add *TixScrolledHList*f1.borderWidth		$::wm_default::borderwidth $pri
    option add *TixScrolledHList*f1.relief			sunken $pri
#    option add *TixTree*hlist.highlightBackground	$tixOption(background) $pri
#    option add *TixTree*hlist.background		$tixOption(light1_bg) $pri
#    option add *TixTree*hlist.activeBackground	$tixOption(light1_bg) $pri
#    option add *TixTree*hlist.disabledBackground	$tixOption(disabled_bg) $pri
#    option add *TixTree*f1.borderWidth		$::wm_default::borderwidth $pri
    option add *TixTree*f1.relief			sunken $pri
#    option add *TixHList.background			$tixOption(light1_bg) $pri
#    option add *TixHList.activeBackground		$tixOption(light1_bg) $pri
#    option add *TixHList.disabledBackground		$tixOption(light1_bg) $pri
#    option add *TixMultiView*Listbox.borderWidth		0 $pri
#    option add *TixMultiView*Listbox.highlightThickness	0 $pri
    option add *TixMultiView*Scrollbar.relief		sunken $pri
#    option add *TixMultiView*f1.borderWidth		2 $pri
    option add *TixMultiView*f1.relief			sunken $pri
#    option add *TixMultiView*f1.highlightThickness		2 $pri
#    option add *TixMDIMenuBar*menubar.relief		raised $pri
#    option add *TixMDIMenuBar*menubar.borderWidth		2 $pri
#    option add *TixMDIMenuBar*Menubutton.padY 		2 $pri
#    option add *TixNoteBook.Background			$tixOption(background) $pri
#    option add *TixNoteBook.nbframe.Background		$tixOption(background) $pri
#    option add *TixNoteBook.nbframe.backPageColor		$tixOption(background) $pri
#    option add *TixNoteBook.nbframe.inactiveBackground	$tixOption(inactive_bg) $pri
#    option add *TixPanedWindow.handleActiveBg 		$tixOption(active_bg) $pri
#    option add *TixPanedWindow.seperatorBg    		$tixOption(disabledbackground) $pri
#    option add *TixPanedWindow.handleBg       		$tixOption(disabledbackground) $pri
#    option add *TixPopupMenu*menubutton.background 	$tixOption(dark1_bg) $pri
#    option add *TixScrolledHList*hlist.highlightBackground	$tixOption(background) $pri
#    option add *TixScrolledHList*hlist.background		$tixOption(light1_bg) $pri
#    option add *TixScrolledTList*tlist.highlightBackground	$tixOption(background) $pri
#    option add *TixScrolledTList*tlist.background		$tixOption(light1_bg) $pri
#    option add *TixScrolledListBox*listbox.highlightBackground	$tixOption(background) $pri
#    option add *TixScrolledWindow.frame.background		$tixOption(light1_bg) $pri
#    option add *TixTree*hlist.highlightBackground	$tixOption(background) $pri
#    option add *TixTree*hlist.background		$tixOption(light1_bg) $pri
#    option add *TixTree*hlist.borderWidth		$::wm_default::borderwidth $pri

    # These were missing

#    option add *TixMenu*Menu.selectColor $NIMLook(foreground) $pri

#    option add *TixMDIMenuBar*Menubutton.padY 2 $pri
#    option add *TixMDIMenuBar*menubar.borderWidth 2 $pri
#    option add *TixMDIMenuBar*menubar.relief raised $pri

#    option add *TixMultiView*Listbox.borderWidth 0 $pri
#    option add *TixMultiView*Listbox.highlightThickness 0 $pri
#    option add *TixMultiView*Scrollbar.relief sunken $pri
#    option add *TixMultiView*f1.borderWidth 2 $pri
#    option add *TixMultiView*f1.highlightThickness 2 $pri
    option add *TixMultiView*f1.relief sunken $pri

}

# Leave the standard widgets alone
if {0} {
    option add *Background		$tixOption(background) $pri
    option add *background		$tixOption(background) $pri
    option add *Foreground		$tixOption(foreground) $pri
    option add *foreground		$tixOption(foreground) $pri
    option add *activeBackground	$tixOption(active_bg) $pri
    option add *activeForeground	$tixOption(active_fg) $pri
    option add *HighlightBackground	$tixOption(background) $pri

    option add *selectBackground	$tixOption(select_bg) $pri
    option add *selectForeground	$tixOption(select_fg) $pri
    option add *selectBorderWidth		0 $pri

    option add *Menu.selectColor	$tixOption(foreground) $pri
    option add *TixMenu.selectColor	$tixOption(foreground) $pri
    option add *Menubutton.padY		5 $pri

    option add *Button.borderWidth		2 $pri
    option add *Button.anchor			c $pri

    option add *Checkbutton.selectColor		$tixOption(selector) $pri
    option add *Radiobutton.selectColor		$tixOption(selector) $pri
    option add *Entry.relief			sunken $pri
    option add *Entry.highlightBackground	$tixOption(background) $pri
    option add *Entry.background		$tixOption(textbackground) $pri
    option add *Entry.foreground		$tixOption(textforeground) $pri
    option add *Entry.insertBackground		$tixOption(textforeground) $pri
    option add *Label.anchor			w $pri
    option add *Label.borderWidth		0 $pri

    option add *Listbox.background		$tixOption(textbackground) $pri
    option add *Listbox.relief			sunken $pri

    option add *Scale.foreground		$tixOption(foreground) $pri
    option add *Scale.activeForeground		$tixOption(background) $pri
    option add *Scale.background		$tixOption(background) $pri
    option add *Scale.sliderForeground		$tixOption(background) $pri
    option add *Scale.sliderBackground		$tixOption(light1_bg) $pri

    option add *Scrollbar.relief		sunken $pri
    option add *Scrollbar.borderWidth		$::wm_default::borderwidth $pri
    option add *Scrollbar.width			15 $pri

    option add *Text.background			$tixOption(textbackground) $pri
    option add *Text.relief			sunken $pri

}
