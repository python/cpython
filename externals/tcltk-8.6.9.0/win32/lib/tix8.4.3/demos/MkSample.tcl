# -*-mode: tcl; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: MkSample.tcl,v 1.3 2001/12/09 05:34:59 idiscovery Exp $
#
# MkSample.tcl --
#
#	This file implements the "Sample" page in the widget demo
#
#	This file has not been properly documented. It is NOT intended
#	to be used as an introductory demo program about Tix
#	programming. For such demos, please see the files in the
#	demos/samples directory or go to the "Samples" page in the
#	"widget demo"
#
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
#
#

set tix_demo_running 1
set samples_dir [file join $demo_dir samples]
set sample_filename {}

uplevel #0 source [list [file join $samples_dir AllSampl.tcl]]


proc MkSample {nb page} {
    global tixOption

    #----------------------------------------------------------------------
    set w [$nb subwidget $page]

    set pane [tixPanedWindow $w.pane -orient horizontal]
    pack $pane -expand yes -fill both

    set f1 [$pane add 1 -expand 1]
    set f2 [$pane add 2 -expand 3]
    $f1 config -relief flat
    $f2 config -relief flat

    # Left pane: the Tree:
    #
    set lab [label $f1.lab  -text "Select a sample program:" -anchor w]
    set tree [tixTree $f1.slb \
	-options {
	    hlist.selectMode single
	    hlist.width  40
	}]
    $tree config \
	-command   "Sample:Action $w $tree run" \
	-browsecmd "Sample:Action $w $tree browse"

    pack $lab -side top -fill x -padx 5 -pady 5
    pack $tree -side top -fill both -expand yes -padx 5

    # Right pane: the Text
    #
    set labe [tixLabelEntry $f2.lab -label "Source:" -options {
	label.anchor w
    }]

    $labe subwidget entry config -state disabled

    set stext [tixScrolledText $f2.stext]
    set f3 [frame $f2.f3]

    set run  [button $f3.run  -text "Run ..."  \
	-command "Sample:Action $w $tree run"]
    set view [button $f3.view -text "View Source in Browser ..." \
	-command "Sample:Action $w $tree view"]

    pack $run $view -side left -fill y -pady 2

    pack $labe -side top -fill x -padx 7 -pady 2
    pack $f3 -side bottom -fill x -padx 7
    pack $stext -side top -fill both -expand yes -padx 7

    #
    # Set up the text subwidget

    set text [$stext subwidget text]
    bind $text <Up>    "%W yview scroll -1 unit"
    bind $text <Down>  "%W yview scroll 1 unit"
    bind $text <Left>  "%W xview scroll -1 unit"
    bind $text <Right> "%W xview scroll 1 unit"
    bind $text <Tab>   {focus [tk_focusNext %W]; break}

    bindtags $text "$text Text [winfo toplevel $text] all"

    $text config -bg [$tree subwidget hlist cget -bg] \
	-state disabled -font $tixOption(fixed_font) -wrap none

    $run  config -state disabled
    $view config -state disabled

    global demo
    set demo(w:run)  $run
    set demo(w:view) $view
    set demo(w:tree) $tree
    set demo(w:lab1) $labe
    set demo(w:stext) $stext

    set hlist [$tree subwidget hlist]
    $hlist config -separator "." -width 30 -drawbranch 0 \
	-wideselect false

    set style [tixDisplayStyle imagetext -refwindow $hlist \
	-fg $tixOption(fg) -padx 4]

    set file   [tix getimage textfile]
    set folder [tix getimage openfold]

    ForAllSamples root "" \
	[list AddSampleToHList $tree $hlist $style $file $folder]
}

# AddSampleToHList --
#
#	A callback from ForAllSamples. Add all the possible sample files
#	into the Tree widget.
#
proc AddSampleToHList {tree hlist style file folder token type text dest} {
    case $type {
	d {
	    return [$hlist addchild $token -itemtype imagetext -style $style \
		-image $folder -text $text]
	}
	done {
	    if {![tixStrEq $token ""]} {
		$tree setmode $token close
		$tree close $token
	    }
	}
	f {
	    return [$hlist addchild $token -itemtype imagetext \
		-image $file -text $text -data [list $text $dest]]
	}
    }
}

proc Sample:Action {w slb action args} {
    global samples demo_dir demo samples_dir

    set hlist [$slb subwidget hlist]
    set ent [$hlist info anchor]

    if {$ent == ""} {
	$demo(w:run)  config -state disabled
	$demo(w:view) config -state disabled
	return
    }
    if {[$hlist info data $ent] == {}} {
	# This is just a comment
	$demo(w:run)  config -state disabled
	$demo(w:view) config -state disabled
	return
    } else {
	$demo(w:run)  config -state normal
	$demo(w:view) config -state normal
    }

    set theSample [$hlist info data $ent]
    set title [lindex $theSample 0]
    set prog  [lindex $theSample 1]

    case $action {
	"run" {
	    RunProg $title $prog
	}
	"view" {
	    LoadFile [file join $samples_dir $prog]
	}
	"browse" {
	    # Bring up a short description of the sample program
	    # in the scrolled text about

	    set text [$demo(w:stext) subwidget text]
	    uplevel #0 set sample_filename [list [file join $samples_dir $prog]]
	    tixWidgetDoWhenIdle ReadFileWhenIdle $text

	    $demo(w:lab1) subwidget entry config -state normal
	    $demo(w:lab1) subwidget entry delete 0 end
	    $demo(w:lab1) subwidget entry insert end [file join $samples_dir $prog]
	    $demo(w:lab1) subwidget entry xview end
	    $demo(w:lab1) subwidget entry config -state disabled
	}
    }
}

proc RunProg {title prog} {
    global samples demo_dir demo samples_dir

    set w .[lindex [split $prog .] 0]
    set w [string tolower $w]

    if [winfo exists $w] {
	wm deiconify $w
	raise $w
	return
    }

    uplevel #0 source [list [file join $samples_dir $prog]]

    toplevel $w 
    wm title $w $title
    RunSample $w
}


proc LoadFile {filename} {
    global tixOption

    set tmp $filename
    regsub -all . $filename _ tmp
    set w [string tolower .$tmp]

    if [winfo exists $w] {
	wm deiconify $w
	raise $w
	return
    }

    toplevel $w 
    wm title $w "Source View: $filename"

    button $w.b -text Close -command "destroy $w"
    set t [tixScrolledText $w.text]
    tixForm $w.b    -left 0 -bottom -0 -padx 4 -pady 4
    tixForm $w.text -left 0 -right -0 -top 0 -bottom $w.b

    $t subwidget text config -highlightcolor [$t cget -bg] -bd 2 \
	-bg [$t cget -bg] -font $tixOption(fixed_font) 
    if {$filename == {}} {
	return
    }

    set text [$w.text subwidget text]
    $text config -wrap none

    ReadFile $text $filename
}

proc ReadFileWhenIdle {text} {
    global sample_filename

    if ![file isdir $sample_filename] {
	ReadFile $text $sample_filename
    }
}

proc ReadFile {text filename} {
    set oldState [$text cget -state]
    $text config -state normal
    $text delete 0.0 end

	set fd [open $filename {RDONLY}]
	$text delete 1.0 end
    
	while {![eof $fd]} {
	    $text insert end [gets $fd]\n
	}
	close $fd

    $text see 1.0
    $text config -state $oldState
}
