# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: dirbox.tcl,v 1.3 2004/03/28 02:44:57 hobbs Exp $
#
# dirbox.tcl --
#
#	Tests the DirSelectBox and DirSelectDialog widgets.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Testing the DirSelectBox and DirSelectDialog widgets."
}

# Try to configure the directory of a widget and see if it satisfy all
# the requirements:
#
#	1: Should return error for non-existant directory, preserving
#	   the old directory
#
#	2: When given a non-normalized path, it should normalize it.
#
proc TestConfigDirectory {class spec pack} {
    global errorInfo

    set w .w

    if [winfo exists $w] {
	destroy $w
    }

    TestBlock config-dir-1.1 "Simple creating of $class" {
	# Creation without the spec. The default value should be normalized
	#

	# The default value should always be an absolute path
	#
	$class .w
	set value [$w cget $spec]
	Assert {[tixFSIsNorm $value]} 0 cont
    }
    catch {
	destroy .w
    }

    TestBlock config-dir-1.2 "Creation with arbitrary (perhaps invalid) path" {
	foreach item [GetCases_FsNormDir] {
	    if [info exists errorInfo] {
		set errorInfo ""
	    }

	    set text    [lindex $item 0]
	    set want    [lindex $item 1]
	    set wanterr [lindex $item 2]

	    set err [catch {
		set w [$class .w $spec $text]
		set got [$w cget -value]
	    }]
	    Assert {$err == $wanterr}
	    if {!$err} {
		set want [tixFSDisplayName $want]
		Assert {[tixStrEq $want $got]}
	    }

	    catch {
		destroy .w
	    }
	}
    }

    catch {
	destroy .w
    }

    TestBlock config-dir-1.2 "Config with arbitrary (perhaps invalid) path" {
	set w [$class .w]

	foreach item [GetCases_FsNormDir] {
	    if [info exists errorInfo] {
		set errorInfo ""
	    }

	    set text    [lindex $item 0]
	    set want    [lindex $item 1]
	    set wanterr [lindex $item 2]

	    set err [catch {
		$w config $spec $text
		set got [$w cget -value]
	    }]
	    Assert {$err == $wanterr}

	    if $err {
		# Should hold the previous -value
		#
		set value [$w cget $spec]
		Assert {[tixFSIsNorm $value]} 0 cont
	    } else {
		set value [$w cget $spec]
		Assert {[tixFSIsNorm $value]} 0 cont

		set want [tixFSDisplayName $want]
		Assert {[tixStrEq $want $got]}
	    }

	    if $pack {
		pack $w -expand yes -fill both -padx 10 -pady 10
		update idletasks
	    }
	}
    }

    catch {
	destroy $w
    }
}

proc TestRand {max} {
    global testRandSeed

    if ![info exists testRandSeed] {
	set testRandSeed [expr [lindex [time {cd [pwd]}] 0] * 47 + 147]
    }

    set x [expr ($testRandSeed + 47) * [lindex [time {cd [pwd]}] 0]]
    set x [expr $x + 7 * $max]
    set testRandSeed [expr ($x % $max) + $max]

    return [expr $testRandSeed % $max]
}

# TestHListWildClick --
#
#	Randomly click around an hlist widget
#
# Args:
#	hlist:widget	The HList widget.
#	mode:		Either "single" or "double", indicating which type
#			of mouse click is desired.
#	cmd:		Command to call after each click.
#
proc TestHListWildClick {hlist mode cmd} {
    # The percentage chance that we sould traverse to a child node
    #
    set chance 40

    for {set x 0} {$x < 10} {incr x} {
	set node [$hlist info children ""]
	if [tixStrEq $node ""] {
	    return
	}

	while 1 {
	    set ran [TestRand 100]
	    if {$ran >= $chance} {
		break
	    }
	    set children [$hlist info children $node]
	    if [tixStrEq $children ""] {
		break
	    }
	    set node [lindex $children [expr $ran % [llength $children]]]
	}

	TestBlock wild-click-1.1 "clicking \"$node\" of HList" {
	    if {![regexp -nocase alex [$hlist info data $node]]} {
		#
		# dirty fix: "alex" may be an AFS mounted file. Reading this
		# directory may start an FTP session, which may be slow like
		# hell
		#
		ClickHListEntry $hlist $node $mode
		eval $cmd [list $node]
	    }
	}
    }
}


proc DirboxTest_Cmd {args} {
    global dirboxTest_selected

    set dirboxTest_selected [tixEvent value]
}

proc DirboxTest_Compare {isDirBox w h node} {
    global dirboxTest_selected

    set selFile [$h info data $node]

    Assert {[tixStrEq "$dirboxTest_selected" "$selFile"]}
    set dirboxTest_selected ""

    if {$isDirBox} {
	set entry [$w subwidget dircbx subwidget combo subwidget entry]
	set entText [$entry get]
	Assert {[tixStrEq "$entText" "$selFile"]}
    }
}

proc Test {} {
    global dirboxTest_selected

    #------------------------------------------------------------
    # (1) DirList
    #------------------------------------------------------------

    TestBlock dirbox-1.1 {Generic testing of tixDirList} {
	TestConfigDirectory tixDirList -value 1
    }

    TestBlock dirbox-1.2 {Wild click on the hlist subwidget} {
	set dirboxTest_selected ""
	set w [tixDirList .c -command DirboxTest_Cmd]
	set h [$w subwidget hlist]
	pack $w -expand yes -fill both
	TestHListWildClick $h double "DirboxTest_Compare 0 $w $h"
    }
    catch {
	destroy $w
    }

    #------------------------------------------------------------
    # (2) DirTree
    #------------------------------------------------------------
    
    TestBlock dirbox-2.1 {Generic testing of tixDirTree} {
#	TestConfigDirectory tixDirTree -value 1
    }

    TestBlock dirbox-2.2 {Wild click on the hlist subwidget} {
	set dirboxTest_selected ""
	set w [tixDirTree .c -command DirboxTest_Cmd]
	set h [$w subwidget hlist]
	pack $w -expand yes -fill both
#	TestHListWildClick $h double "DirboxTest_Compare 0 $w $h"
    }
    catch {
	destroy $w
    }

    #------------------------------------------------------------
    # (3) DirBox
    #------------------------------------------------------------

    TestBlock dirbox-3.1 {Generic testing of tixDirSelectBox} {
#	TestConfigDirectory tixDirSelectBox -value 1
    }

    TestBlock dirbox-3.2 {Wild click on the hlist subwidget} {
	set dirboxTest_selected ""
	set w [tixDirSelectBox .c -command DirboxTest_Cmd]
	set h [$w subwidget dirlist subwidget hlist]
	pack $w -expand yes -fill both
#	TestHListWildClick $h double "DirboxTest_Compare 0 $w $h"
    }
    catch {
	destroy $w
    }

    TestBlock dirbox-4.1 {-disablecallback option} {
	global dirbox_called
	tixDirList .c -command dirbox_callback
	pack .c
	set dirbox_called 0
	.c config -disablecallback 1
	.c config -value [pwd]
	.c config -disablecallback 0
	Assert {$dirbox_called == 0}
    }
    catch {
	destroy .c
    }
}

proc dirbox_callback {args} {
    global dirbox_called
    set dirbox_called 1
}
    
