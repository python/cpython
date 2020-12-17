# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#
#	$Id: WmDefault.tcl,v 1.7 2008/03/17 22:47:00 hobbs Exp $
#
#	Description: Package for making Tk apps use the CDE/KDE/Gnome/Windows scheme
#	Prefix: wm_default::
#	Url: http://tix.sourceforge.net/Tixapps/
#
# Usage:
#	It should be sufficient at the beginning of a wish app to simply:
#
#	    package require wm_default
#	    wm_default::setup
#	    wm_default::addoptions
#
# 	wm_default::setup takes an optional argument - the scheme if already
#	known, one of: windows gnome kde1 kde2 cde kde
# 	wm_default::addoptions takes optional arguments - pairs of variables
#	and values to override the kde settings. e.g. 
#		wm_default::addoptions -background blue
#
# Description:
#       package for making Tk apps look nice under CDE or KDE or Windows
#
#	The stuff below attempts to use the options database and the
#	various files under ~/.dt, $DTHOME, and /usr/dt to figure out
#	the user's current font and color set.  It then uses tk's
#	palette routines to set sensible defaults, and then override
#	some of the options to try to make them look like CDE.
#
#	There really *must* be an easier way to get text background
#	colors and the radiobutton highlight colors out of the
#	options database or winfo atom...  Unfortunately, I can't
#	figure out how...

# This package is based on the cde package by D. J. Hagberg, Jr.:
# dhagberg@glatmos.com

########################################################################
#
#
 # Copyright 1998 D. J. Hagberg, Jr. and Global Atmospherics, Inc.
 #
 # Permission to use, copy, modify, and distribute this software and its
 # documentation for any purpose and without fee is hereby granted, provided
 # that the above copyright notice appear in all copies.
 # D. J. Hagberg, Jr. and Global Atmospherics, Inc. make no representations
 # about the suitability of this software for any It is provided "as is"
 # without express or implied warranty.  By use of this software the user
 # agrees to  indemnify and hold harmless D. J. Hagberg, Jr. and Global
 # Atmospherics, Inc. from any claims or liability for loss arising out
 # of such use.
########################################################################

package require Tk

proc tixDetermineWM {} {
    # Returns one of cde kde1 kde2 gnome windows or ""
    global tcl_platform env

    set type ""
    if {$tcl_platform(platform) eq "windows"} {
	set type windows
    } else {
	# The most definitive way is to check the X atoms
	# I'm not sure if we can determine this using regular Tk wm calls
	# I don't want to intern these symbols if they're not there.
	if {![catch {exec xlsatoms} xatoms]} {
	    if {[string match *GNOME_SESSION_CORBA_COOKIE* $xatoms]} {
		set type gnome
	    } elseif {[string match *KDEChangeStyle* $xatoms]} {
		set type kde1
	    } elseif {[string match *KDE_DESKTOP_WINDOW* $xatoms]} {
		set type kde2
	    }
	}
	if {$type != ""} {
	    # drop through
	} elseif {[info exists env(KDEDIR)] && [file isdir $env(KDEDIR)]} {
	    # one or two?
	    set type kde2
	} elseif {[info exists env(DTHOME)] && [file isdir $env(DTHOME)]} {
	    set type cde
	} else {
	    # Maybe look for other Unix window managers?
	    # if {[file isfile $env(HOME)/.fwm2rc]} {}
	    # if {[file isfile $env(HOME)/.fwmrc]} {}
	    # if {[file isfile $env(HOME)/.twmrc]} {}
	    # But twm and fwm don't color applications; mwm maybe?
	    # Hope someone comes up with the code for openlook :-)
	    return ""
	}
    }
    return $type
}

namespace eval ::wm_default {
    global tcl_platform env

    set _usetix [llength [info commands "tix"]]

    variable wm ""

    variable _frame_widgets {*Frame *Toplevel}
    #what about tixGrid?
    if {$_usetix} {
	lappend _frame_widgets *TixLabelFrame *TixButtonBox *TixCObjView \
		*TixListNoteBook *TixPanedWindow *TixStdButtonBox \
		*TixExFileSelectBox
    }
    variable _menu_font_widgets {*Menu *Menubutton}
    if {$_usetix} {lappend _menu_font_widgets *TixMenu}
    variable _button_font_widgets {*Button}

    variable _system_font_widgets {*Label \
		*Message \
                *Scale *Radiobutton *Checkbutton
    }
    if {$_usetix} {
	lappend _system_font_widgets \
	    *TixBalloon*message *TixLabelFrame*Label \
	    *TixControl*label *TixControl*Label \
	    *TixLabelEntry*label *TixNoteBook.nbframe \
	    *TixFileEntry*label *TixComboBox*label  \
	    *TixOptionMenu*menubutton  *TixBitmapButton*label  \
	    *TixMwmClient*title *TixFileSelectBox*Label
    }
   variable _text_type_widgets {*Canvas *Entry *Listbox *Text}
   if {$_usetix} {
       lappend _text_type_widgets \
	   *TixComboBox*Entry *TixControl*entry *TixScrolledHList*hlist \
	   *TixDirTree*hlist *TixDirList*hlist *TixTree*hlist \
	   *TixMultiList*Listbox *TixScrolledListBox*listbox  \
	   *TixFileEntry*Entry *TixLabelEntry*Entry \
	   *TixFileEntry*entry *TixLabelEntry*entry \
	   *TixScrolledTList*tlist  *TixScrolledText*text \
	   *TixHList *TixCheckList*hlist
       # These arent working yet
       lappend _text_type_widgets \
	       *TixDirTree*f1 *TixDirList*f1 \
	       *TixFileSelectBox*file*listbox \
	       *TixFileSelectBox*directory*listbox \
	       *TixExFileSelectBox*filelist*listbox
   }

   variable _insert_type_widgets {*Entry *Text}
   if {$_usetix} {
       lappend _insert_type_widgets \
	   *TixControl*entry *TixComboBox*Entry
    }

   variable _select_type_widgets {*Checkbutton \
	   *Radiobutton \
	   *Menu}
   variable _active_borderwidth_widgets {*Button *Radiobutton *Checkbutton}

   # Other Widgets that are given a borderwidth of $wm_default::borderwidth
   variable _nonzero_borderwidth_widgets {}

   # Widgets that are given a borderwidth of 0
   # must not be  *Entry
   variable _null_borderwidth_widgets {*Menubutton *Label}

   variable _scrollbar_widgets {}
   variable _scrollbar_widgets {*Scrollbar}
   if {$_usetix} {
       lappend  _scrollbar_widgets \
	       *TixTree*Scrollbar *TixDirTree*Scrollbar *TixDirList*Scrollbar \
	       *TixScrolledTList*Scrollbar *TixScrolledListBox*Scrollbar \
	       *TixScrolledHList*Scrollbar *TixMultiView*Scrollbar \
	       *TixScrolledText*Scrollbar *TixScrolledWindow*Scrollbar  \
	       *TixCObjView*Scrollbar
   }

   proc debug {str} {
       if {[llength [info commands "tix"]] &&
	   [string is true -strict [tix cget -debug]]} {
	   puts $str
       }
   }

    # Return the array of what we have deduced
    proc getoptions {} {
	set retval {}
	set len [string length "::wm_default::"]
        foreach variable [lsort [info vars ::wm_default::*]] {
	    if {[string match ::wm_default::_* $variable]} {continue}
	    set key [string range $variable $len end]
	    lappend retval $key [set $variable]
	}
	return $retval
    }

    # Print out the array of what we have deduced
    proc parray {} {
	set retval ""
        foreach {key val} [::wm_default::getoptions] {
	    append retval [join [list $key $val]] "\n"
	}
	debug $retval
	return $retval
    }

    # Pick a default borderwidth in pixels
    set _bd 2
    # Pick a default font size in pixels
    set _screenheight [winfo screenheight .]
    if {$_screenheight < 500} {
	# for 800x600 or smaller
	set _pixel 10
	set _point 8
	# Pick a default borderwidth which smaller
	set _bd 1
    } elseif {$_screenheight < 800} {
	# for 1024x768
	set _pixel 12
	set _point 8
    } elseif {$_screenheight < 1100} {
	# for 1200x1000
	set _pixel 12
	set _point 8
    } else {
	set _pixel 12
	set _point 10
    }

    # setup defaults depending on the OS and Window Manager
    # Really should do another version for mono
    if {$tcl_platform(platform) eq "windows"} {

	if {$tcl_platform(osVersion) < 5} {
	    set _prop_default "MS Sans Serif"
	} else {
	    set _prop_default "Tahoma"
	}

	# make sure this font is installed 
	set _allowed [string tolow [font families]]
	foreach font [list $_prop_default "MS Sans Serif" Tahoma Arial System] {
	    if {[lsearch -exact $_allowed [string tolow $font]] > -1} {
		set _prop_default $font
		break
	    }
	}

	set _fixed_default {Courier New}
	# make sure this font is installed 
	foreach font [list $_fixed_default Courier System] {
	    if {[lsearch -exact $_allowed [string tolow $font]] > -1} {
		set _fixed_default $font
		break
	    }
	}

	# Windows colors:
	#    "3dDarkShadow",		COLOR_3DDKSHADOW,
	#    "3dLight",			COLOR_3DLIGHT,
	#    "ActiveBorder",		COLOR_ACTIVEBORDER,
	#    "ActiveCaption",		COLOR_ACTIVECAPTION,
	#    "AppWorkspace",		COLOR_APPWORKSPACE,
	#    "Background",		COLOR_BACKGROUND,
	#    "ButtonFace",		COLOR_BTNFACE,
	#    "ButtonHighlight",		COLOR_BTNHIGHLIGHT,
	#    "ButtonShadow",		COLOR_BTNSHADOW,
	#    "ButtonText",		COLOR_BTNTEXT,
	#    "CaptionText",		COLOR_CAPTIONTEXT,
	#    "DisabledText",		COLOR_GRAYTEXT,
	#    "GrayText",		COLOR_GRAYTEXT,
	#    "Highlight",		COLOR_HIGHLIGHT,
	#    "HighlightText",		COLOR_HIGHLIGHTTEXT,
	#    "InactiveBorder",		COLOR_INACTIVEBORDER,
	#    "InactiveCaption",		COLOR_INACTIVECAPTION,
	#    "InactiveCaptionText",	COLOR_INACTIVECAPTIONTEXT,
	#    "InfoBackground",		COLOR_INFOBK,
	#    "InfoText",			COLOR_INFOTEXT,
	#    "Menu",			COLOR_MENU,
	#    "MenuText",			COLOR_MENUTEXT,
	#    "Scrollbar",		COLOR_SCROLLBAR,
	#    "Window",			COLOR_WINDOW,
	#    "WindowFrame",		COLOR_WINDOWFRAME,
	#    "WindowText",		COLOR_WINDOWTEXT,

	variable \
		background       	"SystemButtonFace" \
		foreground       	"SystemButtonText" \
		disabledforeground      "SystemDisabledText" \
		disabledbackground      "SystemButtonShadow" \
		textfamily	     	$_prop_default \
		systemfamily       	$_prop_default \
		menufamily 	      	$_prop_default \
		fixedfamily       	$_fixed_default \
		fontsize	      	$_point \
		textbackground   	"SystemWindow" \
		textforeground   	"SystemWindowText" \
		disabledtextbackground  "SystemDisabledText" \
		selectbackground 	"SystemHighlight" \
		selectforeground 	"SystemHighlightText" \
		selectcolor      	"SystemWindow" \
		highlightcolor  	"SystemWindowFrame" \
		highlightbackground 	"SystemButtonFace" \
		scrollbars		"SystemScrollbar" \
		borderwidth      	$_bd \
		menubackground		"SystemMenu" \
		menuforeground		"SystemMenuText"

	variable highlightthickness         	1

	# Windows does not have an activebackground, but Tk does
	variable activebackground $background
	variable activeforeground $foreground
    } else {
	# intended for Unix

	# Tk uses the following defaults:

	#define NORMAL_BG	"#d9d9d9"
	#define ACTIVE_BG	"#ececec"
	#define SELECT_BG	"#c3c3c3"
	#define TROUGH		"#c3c3c3"
	#define INDICATOR	"#b03060"
	#define DISABLED	"#a3a3a3"

	# We know . exists and it has a background
	# This should be "#d9d9d9" under default Tk
	set _bg [. cget -background]

	set _prop_default helvetica
	# make sure this font is installed 
	set _allowed [string tolow [font families]]
	foreach font [list $_prop_default times fixed] {
	    if {[lsearch -exact $_allowed [string tolow $font]] > -1} {
		set _prop_default $font
		break
	    }
	}
	set _fixed_default courier
	# make sure this font is installed 
	foreach font [list $_fixed_default fixed] {
	    if {[lsearch -exact $_allowed [string tolow $font]] > -1} {
		set _fixed_default $font
		break
	    }
	}

	variable \
		background       	$_bg \
		foreground       	Black \
		disabledforeground      #808080 \
		disabledbackground      #a3a3a3 \
		textfamily	 	$_prop_default \
		systemfamily       	$_prop_default \
		menufamily       	$_prop_default \
		fixedfamily       	$_fixed_default \
		fontsize		$_pixel \
		textbackground   	white \
		textforeground   	Black \
		disabledtextbackground  $_bg \
		selectbackground 	#000080 \
		selectforeground 	white \
		selectcolor      	yellow \
		highlightcolor  	Black \
		highlightbackground     $_bg \
		scrollbars		"#c3c3c3" \
		borderwidth      	$_bd \
		menubackground       	$_bg \
		menuforeground       	Black

	variable highlightthickness         	1

	# Windows does not have an activebackground, but Tk does
	variable activebackground "#ececec"
	variable activeforeground $foreground
   }

   # priority should be userDefault?
   if {$_usetix} {
       variable priority [tix cget -schemepriority]
   } else {
       variable priority 75
   }

   # variables that will be derived during addoptions - set to null for now
   variable system_font {}
   variable menu_font  {}
   variable fixed_font {}
   variable text_font  {}

   # Different desktops have different visible regions
   # This is not working properly yet.
   variable geometry 0+0+[winfo screenwidth .]+$_screenheight

   # Different desktops have different focusmodels: clicktofocus or
   # followsmouse This is not working properly yet
   variable focusmodel clicktofocus

   # Some desktops have standardized link colors
   # This is not working properly yet
   variable linkcolor "#0000ff" vlinkcolor "#800000" alinkcolor "#800080"

   proc default {args} {
	# Override the defaults with any optional arguments
	foreach {var val} $args {
	    set $var $val
	}
    }

    proc setup {{type ""}} {
	# type is one of the recognized window managers
	# one of: cde kde win
	global tcl_platform env

	if {$type == ""} {
	    if {[set type [::tixDetermineWM]] == ""} {
		set ::wm_default::wm ""
		# Generic unix
		return
	    }
	}
	setup-$type
	set ::wm_default::wm $type
	# todo - make the menubutton enter and leave events more compatible
    }

    proc setup-windows {args} {
	# Already done by Tk above.
	# Should find out the useable window region
    }

    proc setup-gnome {args} {
	# GNOME is still barely supported because of the difficulty
	# of finding and parsing sawfish definition files.
	# First you have to find what window manager, then what theme,
	# then find the files, then parse them according to each Wm's syntax.
	global env

	set bg $wm_default::background
	set fg $wm_default::foreground
	set txtFont $wm_default::textfamily
	set btnFont $wm_default::systemfamily

	debug "Setting up Gnome environment"

	set file ~/.gnome/theme-switcher-capplet
	if {![file exists $file] || \
		[catch {open $file} fd] || $fd == ""} {
	    debug "Skipping $file"
	} else {
	    debug "Reading $file"
	    set contents [read $fd]
	    catch {close $fd}

	    if {![regexp -- "use_theme_font=true" $contents]} {
		# not activated
	    } elseif {[regexp -- "\nfont=(\[-a-zA-Z0-9,\]+)" $contents \
			   foo font]} {
		set_unix_font $font
	    }
	}

    }

    proc set_unix_font {font} {

	set list [split $font "-"]
	set font [lindex $list 2]
	set font [string tolow $font]
	if {$font != "" && [lsearch -exact [font families] $font] > -1} {
	    set wm_default::textfamily $font
	    debug "Setting textfamily to $font"
	    set wm_default::systemfamily $font
	    set wm_default::menufamily $font
	} else {
	    debug "Unable to set font: '$list'"
	}

	if {[set size [lindex $list 7]] != "" && \
		[string is integer -strict $size]} {
	    debug "Setting fontsize to '$size'"
	    set wm_default::fontsize $size
	} elseif {[set size [lindex $list 8]] != "" && \
		[string is integer -strict $size]} {
	    if {$size > 100} {set size [expr {$size / 10}]}
	    debug "Setting fontsize to '$size'"
	    set wm_default::fontsize $size
	} else {
	    debug "Unable to set fontsize: '$list'"
	}
    }

    # Common to KDE1 and KDE2
    proc setup-kde {args} {
	global env

	set file ~/.kderc
	if {![file exists $file] || [catch {open $file} fd] || $fd == ""} {
	    debug "Skipping $file"
	} else {
	    debug "Reading $file"
	    set contents [read $fd]
	    catch {close $fd}

	    if {[regexp -- "\nfixedfamily=(\[-a-zA-Z0-9, \]+)" $contents \
		    foo font]} {
		set list [split $font ","]
		set font [lindex $list 0]
		set wm_default::fixedfamily $font
		debug "Setting fixedfamily to $font"
	    }
	    if {[regexp -- "\nfont=(\[-a-zA-Z0-9, \]+)" $contents \
		    foo font]} {
		set list [split $font ","]
		set font [lindex $list 0]
		set wm_default::textfamily $font
		debug "Setting textfamily to $font"
		set wm_default::systemfamily $font
		set wm_default::menufamily $font
	    }
	}

    }

    proc setup-kde1 {args} {
	# Shortcut for the moment
	return [eval setup-kde $args]
    }

    proc set-kde2-color {str contents var} {
	if {[regexp -- "\n${str}=(\[0-9,\]+)" $contents -> color]} {
	    set color [eval [list format "#%02x%02x%02x"] [split $color ","]]
	    set ::wm_default::$var $color
	    debug "setting $var to $color"
	}
    }

    proc setup-kde2 {args} {
	global env

	set bg $wm_default::background
	set fg $wm_default::foreground
	set txtFont $wm_default::textfamily
	set btnFont $wm_default::systemfamily

	debug "Setting up KDE environment"

	# Look for system the user settings
	set dirs ~/.kde
	if {[info exists env(KDEDIR)] && [file isdir $env(KDEDIR)]} {
	    lappend dirs $env(KDEDIR)
	}
	# read them sequentially and overwrite the previous values
	foreach dir $dirs {
	    set file $dir/share/config/kdeglobals
	    if {![file exists $file] || [catch {open $file} fd] || $fd == ""} {
		debug "Skipping $file"
	    } else {
		debug "Reading $file"
		set contents [read $fd]
		catch {close $fd}
		set-kde2-color background $contents background
		set-kde2-color foreground $contents foreground
		set-kde2-color selectBackground $contents selectbackground
		set-kde2-color selectForeground $contents selectforeground
		set-kde2-color windowBackground $contents textbackground
		set-kde2-color visitedLinkColor $contents vlinkcolor
		set-kde2-color linkColor $contents linkcolor
		if {[regexp -- "\nactiveFont=(\[-a-zA-Z0-9, \]+)" $contents \
			 foo font]} {
		    set list [split $font ","]
		    set font [lindex $list 0]
		    set wm_default::textfamily $font
		    set size [lindex $list 1]
		    if {[string is integer -strict $size]} {
			set wm_default::fontsize $size
		    }
		    debug "Setting textfamily to $font"
		    set wm_default::systemfamily $font
		    set wm_default::menufamily $font
		}
	    }

	    # should pick up visitedLinkColor

	    set file $dir/share/config/kwmrc
	    if {![file exists $file] || [catch {open $file} fd] || $fd == ""} {
		debug "Skipping $file"
	    } else {
		debug "Reading $file"
		set contents [read $fd]
		catch {close $fd}
		if {[regexp -- "\nDesktop1Region=(\[0-9+\]+)" $contents \
			foo region]} {
		    set wm_default::geometry $region
		    debug "Setting geometry to $region"
		}

		if {[regexp -- "\nFocusPolicy=ClickToFocus" $contents \
			foo region]} {
		    set wm_default::focusmodel clicktofocus
		    debug "Setting focusmodel to clicktofocus"
		} else {
		    # followsmouse
		}
	    }
	}

	return [eval setup-kde $args]
    }

    proc setup-cde {args} {
	namespace import wm_default::*

	set bg $wm_default::background
	set fg $wm_default::foreground
	set txtFont $wm_default::textfamily
	set sysFont $wm_default::systemfamily

	debug "Setting up CDE environment"

        # if any of these options are missing, we must not be under CDE
	set txtFont [option get . textfamilyList textfamilyList]
	set sysFont [option get . systemfamilyList systemfamilyList]
        if {$txtFont ne "" && $sysFont ne ""} {
	    set txtFont [lindex [split $txtFont :] 0]
	    set sysFont [lindex [split $sysFont :] 0]
	    if {$txtFont ne ""} {set textfamily $txtFont}
	    if {$sysFont ne ""} {set systemfamily $sysFont}
	    #
	    # If we can find the user's dt.resources file, we can find out the
	    # palette and text background/foreground colors
	    #
	    set txtBg $bg
	    set txtFg $fg
	    set selFg  $selectforeground
	    set selBg  $selectbackground
	    set selCol $selectcolor
	    set fh ""
	    set palf ""
	    set cur_rsrc ~/.dt/sessions/current/dt.resources
	    set hom_rsrc ~/.dt/sessions/home/dt.resources
	    if {[file readable $cur_rsrc] && [file readable $hom_rsrc]} {
		if {[file mtime $cur_rsrc] > [file mtime $hom_rsrc]} {
		    if {[catch {open $cur_rsrc r} fh]} {set fh ""}
		} else {
		    if {[catch {open $hom_rsrc r} fh]} {set fh ""}
		}
	    } elseif {[file readable $cur_rsrc]} {
		if {[catch {open $cur_rsrc r} fh]} {set fh ""}
	    } elseif {[file readable $hom_rsrc]} {
		if {[catch {open $hom_rsrc r} fh]} {set fh ""}
	    }
	    if {[string length $fh] > 0} {
		while {[gets $fh ln] != -1} {
		    regexp -- "^\\*0\\*ColorPalette:\[ \t]*(.*)\$" $ln nil palf
		    regexp -- "^Window.Color.Background:\[ \t]*(.*)\$" $ln nil txtBg
		    regexp -- "^Window.Color.Foreground:\[ \t]*(.*)\$" $ln nil txtFg
		}
		catch {close $fh}
		if {$txtBg ne $bg} {
		    set selBg $txtFg
		    set selFg $txtBg
		}
	    }
	    #
	    # If the *0*ColorPalette setting was found above, try to find the
	    # indicated file in ~/.dt, $DTHOME, or /usr/dt.  The 3rd line in the
	    # file will be the radiobutton/checkbutton selectColor.
	    #
	    if {[string length $palf]} {
		set dtdir /usr/dt
		if [info exists env(DTHOME)] {
		    set dtdir $env(DTHOME)
		}
		if {[file readable ~/.dt/palettes/$palf]} {
		    set palf ~/.dt/palettes/$palf
		} elseif {[file readable $dtdir/palettes/$palf]} {
		    set palf $dtdir/palettes/$palf
		} else {
		    set palf ""
		}
		if {[string length $palf]} {
		    if {![catch {open $palf r} fh]} {
			# selectColor will be the 3rd line in the file --
			set ln ""; catch {gets $fh; gets $fh; gets $fh ln}
			set ln [string trim $ln]
			if {[string length $ln]} {set selCol $ln}
			close $fh
		    }
		}
	    }
	    set wm_default::background $bg
	    set wm_default::foreground $fg
	    set wm_default::textfamily $txtFont
	    set wm_default::systemfamily $sysFont
	    set wm_default::menufamily $sysFont
	    set wm_default::textbackground $txtBg
	    set wm_default::textforeground $txtFg
	    set wm_default::selectbackground $selBg
	    set wm_default::selectforeground $selFg
	    set wm_default::selectcolor $selCol
	}
    }

    proc derivefonts {} {
	global tcl_platform env

	# variables that will be derived
	variable system_font 
	variable menu_font 
	variable fixed_font 
	variable text_font 

        #
        # Set default fonts 
        #

	global tcl_platform env
	switch -exact -- $tcl_platform(platform) windows {
	    set system_font [list $::wm_default::systemfamily $::wm_default::fontsize]
	    set menu_font [list $::wm_default::menufamily $::wm_default::fontsize]
	    set text_font [list $::wm_default::textfamily $::wm_default::fontsize]
	    set fixed_font [list $::wm_default::fixedfamily $::wm_default::fontsize]
	} default {
	    set system_font [list $::wm_default::systemfamily -$::wm_default::fontsize]
	    if {[set type $::wm_default::wm] == ""} {
		# Generic unix
		# some Unix Wms seem to make Menu fonts bold - ugly IMHO
		set menu_font [list $::wm_default::menufamily -$::wm_default::fontsize bold]
	    } else {
		# gnome kde1 kde2 cde kde don't
		set menu_font [list $::wm_default::menufamily -$::wm_default::fontsize]
	    }
	    set text_font [list $::wm_default::textfamily -$::wm_default::fontsize]
	    set fixed_font [list $::wm_default::fixedfamily -$::wm_default::fontsize]
	}
    }

    proc addoptions {args} {
	global tcl_platform env tix_version

	# variables that will be derived
	variable system_font 
	variable menu_font 
	variable fixed_font 
	variable text_font 

	if {[info commands "tix"] != ""} {set _usetix 1} {set _usetix 0}

	# Override what you have found with any optional arguments
	foreach {var val} $args {
	    set var [string trimleft $var "-"]
	    set ::wm_default::$var $val
	}

	set pri $::wm_default::priority
	# If you are running under Tix, set the colorscheme now
	# The options below will then override the Tix settings
	if {$_usetix} {
	    # Tix's focus model is very non-standard
	    bind TixComboBox <FocusIn> ""
	    bind TixComboBox <FocusOut> ""

	    if {$tix_version < "8.2"} {
		# works??
		option add *TixNoteBook.nbframe.inactiveBackground \
			$::wm_default::disabledbackground $pri
	    } else {
		# works??
		option add *TixNoteBook.nbframe.inactiveBackground \
			$::wm_default::background $pri
	    }

	    # works??
	    option add *TixPanedWindow.seperatorBg \
		    $::wm_default::disabledbackground $pri
	    option add *TixPanedWindow.handleBg      \
		    $::wm_default::disabledbackground $pri

	    # works??
	    option add *TixPanedWindow.separatorActiveBg \
		    $::wm_default::activebackground $pri
	    option add *TixPanedWindow.handleActiveBg      \
		    $::wm_default::activebackground $pri
	    option add *TixPanedWindow.Background      \
		    $::wm_default::disabledbackground $pri

	    # works??
	    option add *TixResizeHandle*background \
		    $::wm_default::disabledbackground $pri

	}
        foreach pref $wm_default::_frame_widgets {
            option add $pref.background $::wm_default::background $pri
        }
	option add *Background $::wm_default::background $pri

	derivefonts

	# Set the global defaults to the system font
        foreach pref [list *Font *font] {
            option add $pref $system_font $pri
        }

	# Set the "system" type defaults to the system font
        foreach pref $wm_default::_menu_font_widgets {
            option add $pref.font $menu_font $pri
        }
        foreach pref $wm_default::_button_font_widgets {
            option add $pref.font $system_font $pri
            option add $pref.disabledForeground $::wm_default::disabledforeground $pri
            option add $pref.background $::wm_default::background $pri
            option add $pref.foreground $::wm_default::foreground $pri
	    option add $pref.highlightBackground $::wm_default::highlightbackground $pri
        }
        foreach pref $wm_default::_system_font_widgets {
            option add $pref.font $system_font $pri
            option add $pref.background $::wm_default::background $pri
            option add $pref.foreground $::wm_default::foreground $pri
	    option add $pref.highlightBackground $::wm_default::highlightbackground $pri
        }

        foreach pref $wm_default::_text_type_widgets {
            option add $pref.font $text_font $pri
            option add $pref.relief sunken $pri
            option add $pref.borderWidth $::wm_default::borderwidth $pri

            option add $pref.background $::wm_default::textbackground $pri
            option add $pref.foreground $::wm_default::textforeground $pri
            option add $pref.selectBackground $::wm_default::selectbackground $pri
            option add $pref.selectForeground $::wm_default::selectforeground $pri
            option add $pref.highlightThickness $::wm_default::highlightthickness $pri
            option add $pref.disabledBackground $::wm_default::disabledtextbackground $pri
        }
        foreach pref $wm_default::_insert_type_widgets {
            option add $pref.relief sunken $pri
            option add $pref.insertBackground $::wm_default::textforeground $pri
            option add $pref.highlightThickness $::wm_default::highlightthickness $pri
        }
        #
        # Set the Selector color for radiobuttons, checkbuttons, et. al
        #
        foreach pref $wm_default::_select_type_widgets {
            option add $pref.selectColor $::wm_default::selectcolor $pri
            option add $pref.background $::wm_default::background $pri
            option add $pref.foreground $::wm_default::foreground $pri

	    option add $pref.activeBackground $::wm_default::activebackground $pri
            option add $pref.activeForeground $::wm_default::activeforeground $pri
        }

	# Set the "active" defaults - this could be controversial
        foreach pref $wm_default::_menu_font_widgets {
	    option add $pref.activeBackground $::wm_default::activebackground $pri
            option add $pref.activeForeground $::wm_default::activeforeground $pri
            option add $pref.background $::wm_default::background $pri
            option add $pref.foreground $::wm_default::foreground $pri
            option add $pref.disabledForeground $::wm_default::disabledforeground $pri
            option add $pref.highlightThickness $::wm_default::highlightthickness $pri
        }

	switch -exact -- $tcl_platform(platform) windows {
	    # Make sure this is set to foreground - check marks on menus
	    set _menu_select_color $::wm_default::foreground
	} default {
	    # On unix there are recessed check boxes not check marks
	    set _menu_select_color $::wm_default::selectcolor
	}
	option add *Menu.selectColor $_menu_select_color $pri
	if {$_usetix} {
	    option add *TixMenu.selectColor $_menu_select_color $pri
            option add *TixBalloon*message.background $::wm_default::textbackground $pri
	}

	# Windows does not have an activebackground, but Tk does
        foreach pref $wm_default::_button_font_widgets {
            option add $pref.activeBackground $::wm_default::activebackground $pri
            option add $pref.activeForeground $::wm_default::activeforeground $pri
        }

        #
        # Set the default *button borderwidth
        #
	# option add *.borderWidth $::wm_default::borderwidth $pri

        foreach pref $wm_default::_active_borderwidth_widgets {
            option add $pref.activeBorderWidth $::wm_default::borderwidth $pri
            option add $pref.borderWidth $::wm_default::borderwidth $pri
        }
        foreach pref $wm_default::_nonzero_borderwidth_widgets {
            option add $pref.borderWidth $::wm_default::borderwidth $pri
        }
        foreach pref $wm_default::_null_borderwidth_widgets {
            option add $pref.borderWidth 0 $pri
        }

	if {$_usetix} {
	    if {$tix_version < "8.2"} {
		option add *TixNoteBook.nbframe.inactiveBackground \
			$::wm_default::disabledbackground $pri
		option add *TixNoteBook*nbframe.inactiveBackground \
			$::wm_default::disabledbackground $pri
	    } else {
		option add *TixNoteBook.nbframe.inactiveBackground \
			$::wm_default::background $pri
		option add *TixNoteBook*nbframe.inactiveBackground \
			$::wm_default::background $pri
	    }
	}

        foreach pref $wm_default::_scrollbar_widgets {
            option add $pref.background $::wm_default::background $pri
	    option add $pref.foreground $::wm_default::foreground $pri
	    # Tix 8.1.1 had these wrong
	    option add $pref.troughColor $::wm_default::scrollbars $pri
	    option add $pref.borderWidth $::wm_default::borderwidth $pri
	}
	option add *Scale.borderWidth $::wm_default::borderwidth $pri
	option add *Scale.troughColor $::wm_default::scrollbars $pri

	option add *highlightColor $::wm_default::highlightcolor $pri
	option add *highlightBackground $::wm_default::highlightbackground $pri
	option add *HighlightBackground $::wm_default::highlightbackground $pri

	# not _system_font_widgets
	set _focus_widgets [concat \
		$::wm_default::_frame_widgets \
		$::wm_default::_menu_font_widgets \
		$::wm_default::_text_type_widgets \
		$::wm_default::_insert_type_widgets \
		$::wm_default::_select_type_widgets \
		$::wm_default::_active_borderwidth_widgets \
		$::wm_default::_nonzero_borderwidth_widgets \
		$::wm_default::_null_borderwidth_widgets ]

	if {$_usetix} {
	    set _tix_hl_widgets [list *TixBitmapButton*label \
		    *TixComboBox*Entry \
		    *TixControl*entry \
		    *TixDirList*hlist \
		    *TixDirTree*hlist \
		    *TixFileEntry*Entry \
		    *TixFileEntry*entry \
		    *TixMultiList*Listbox \
		    *TixNoteBook.nbframe \
		    *TixOptionMenu*menubutton \
		    *TixScrolledHList*hlist \
		    *TixScrolledListBox*listbox\
		    *TixScrolledTList*tlist \
		    *TixTree*hlist]
	    eval lappend _focus_widgets $_tix_hl_widgets
	}

	foreach pref [lsort -uniq $_focus_widgets] {
	    option add $pref.highlightBackground $::wm_default::highlightbackground $pri
	}

	# Now for some things to make it look more like the WM

	# todo - look for and call
	if {$::wm_default::focusmodel eq "followsmouse"} {
	    tk_focusFollowsMouse
	}

	if {$_usetix} {
	    tixSetDefaultOptions

	    if {[lsearch -exact {windows kde1 kde2} $::wm_default::wm] > -1} {
		# Fix the way Button boxes are packed
		if {![llength [info procs ::tixButtonBox:add]]} {
		    uplevel #0 auto_load tixButtonBox
		}
		proc ::tixButtonBox:add {w name args} {
		    upvar #0 $w data
		    eval [linsert $args 0 button $w.$name]
		    if {![info exists data(-padx)]} {set data(-padx) 5}
		    if {![info exists data(-pady)]} {set data(-pady) 10}
		    if {$data(-orientation) eq "horizontal"} {
			# Push the Buttons  to the right
			if {[info commands $w.pad] == ""} {
			    label $w.pad
			    pack $w.pad -side left -expand yes -fill both
			}
			pack $w.$name -side left \
				-expand no \
				-padx $data(-padx) -pady $data(-pady)
		    } else {
			pack $w.$name -side top \
				-expand no -fill x \
				-padx $data(-padx) -pady $data(-pady)
		    }
		    lappend data(g:buttons) $name
		    set data(w:$name) $w.$name
		    return $w.$name
		}
		option add *TixButtonBox.relief flat $::wm_default::priority
		option add *TixButtonBox.borderwidth 2 $::wm_default::priority
	    }
	}
	return [getoptions]
    }

    namespace export setup addoptions getoptions parray
}

package provide wm_default 1.0


proc tixSetDefaultOptions {} {
    # Returns one of cde kde1 kde2 gnome windows or ""
    global tcl_platform env tixOption

    # There is no overlap between the wm_default variable names and
    # the old style tixOption names. So we can add the wm_default variable
    # names to the array tixOption, and allow people to upgrade in time.

    foreach variable {
	wm
	linkcolor
	vlinkcolor
	alinkcolor
	background
	foreground
	disabledforeground
	disabledbackground
	textfamily
	systemfamily
	menufamily
	fixedfamily
	fontsize
	textbackground
	textforeground
	disabledtextbackground
	selectbackground
	selectforeground
	selectcolor
	highlightcolor
	highlightbackground
	scrollbars
	borderwidth
	priority
	menubackground
	menuforeground
	activebackground
	activeforeground
	system_font
	menu_font
	fixed_font
	text_font
    } {
	set tixOption($variable) [set ::wm_default::$variable]
    }

}
