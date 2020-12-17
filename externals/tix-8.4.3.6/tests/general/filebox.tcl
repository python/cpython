# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: filebox.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# filebox.tcl --
#
#	Tests the File selection box and dialog widget.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Testing the (Ex)FileSelectBox and (Ex)FileSelectDialog widgets."
}

proc FdTest_GetFile {args} {
    global fdTest_selected

    set fdTest_selected [tixEvent value]
}

proc Test {} {
    global fdTest_fullPath

    if [tixStrEq [tix platform] "unix"] {
	set fdTest_fullPath /etc/passwd
    } else {
	set fdTest_fullPath C:\\Windows\\System.ini
    }

    Test_FileSelectBox
    Test_FileSelectDialog

    Test_ExFileSelectBox
    Test_ExFileSelectDialog
}

proc Test_FileSelectBox {} {
    global fdTest_selected fdTest_fullPath

    TestBlock filebox-1.1 {FileSelectBox} {
	set w [tixFileSelectBox .f -command FdTest_GetFile]
	pack $w -expand yes -fill both
	update

	InvokeComboBoxByKey [$w subwidget selection] "$fdTest_fullPath"
	Assert {[tixStrEq $fdTest_selected "$fdTest_fullPath"]}
    }
    catch {
	destroy $w
    }
}

proc Test_FileSelectDialog {} {
    global fdTest_selected fdTest_fullPath

    TestBlock filebox-2.1 {FileSelectDialog} {
	set w [tixFileSelectDialog .f -command FdTest_GetFile]
	$w popup
	update

	InvokeComboBoxByKey [$w subwidget fsbox subwidget selection] \
	    "$fdTest_fullPath"
	Assert {[tixStrEq $fdTest_selected "$fdTest_fullPath"]}
    }
    catch {
	destroy $w
    }
}

proc Test_ExFileSelectBox {} {
    global fdTest_selected fdTest_fullPath

    TestBlock filebox-3.1 {ExFileSelectBox} {
	set w [tixExFileSelectBox .f -command FdTest_GetFile]
	pack $w -expand yes -fill both
	update

	$w subwidget file config -selection "$fdTest_fullPath" \
	    -value "$fdTest_fullPath"
	Assert {[tixStrEq $fdTest_selected "$fdTest_fullPath"]}
    }

    TestBlock filebox-3.2 {Keyboard input in ExFileSelectBox entry subwidget} {
	set dirCbx  [$w subwidget dir]
	set fileCbx [$w subwidget file]
	set okBtn   [$w subwidget ok]

	foreach file {Foo bar "Foo Bar"} {
	    set fdTest_selected ""

	    InvokeComboBoxByKey $fileCbx $file
	    set fullPath [tixFSJoin [$dirCbx cget -value] $file]
	    update

	    Assert {[tixStrEq "$fdTest_selected" "$fullPath"]}
	}
    }

    TestBlock filebox-3.3 {Keyboard and then press OK} {
	foreach file {bar "Foo Bar"} {
	    set fdTest_selected ""

	    SetComboBoxByKey $fileCbx $file
	    Click $okBtn
	    set fullPath [tixFSJoin [$dirCbx cget -value] $file]
	    update

	    Assert {[tixStrEq "$fdTest_selected" "$fullPath"]}
	}
    }

    catch {
	destroy $w
    }
}

proc Test_ExFileSelectDialog {} {
    global fdTest_selected fdTest_fullPath

    TestBlock filebox-4.1 {ExFileSelectDialog} {
	set w [tixExFileSelectDialog .f -command FdTest_GetFile]
	$w popup
	update

	InvokeComboBoxByKey [$w subwidget fsbox subwidget file] \
	    $fdTest_fullPath
	Assert {[tixStrEq $fdTest_selected "$fdTest_fullPath"]}
    }

    catch {
	destroy $w
    }
}
