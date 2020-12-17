# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: cmderror.tcl,v 1.2 2002/11/13 21:12:17 idiscovery Exp $
#
# cmderror.tcl --
#
#	This program tests whether command handler errors are processed
#	properly by the Tix toolkit.
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc About {} {
    return "Testing command handler errors are processed properly"
}

proc Test {} {
    global cmdHandlerCalled

    if {![string compare [info command tixCmdErrorHandler] ""]} {
	if ![auto_load tixCmdErrorHandler] {
	    TestAbort "toolkit error: procedure \"tixCmdErrorHandler\" not implemented"
	}
    }
    rename tixCmdErrorHandler _default_tixCmdErrorHandler
    proc tixCmdErrorHandler {msg} {
	global cmdHandlerCalled
	set cmdHandlerCalled 1
    }

    # We cause an error to occur in the -command handler of the combobox
    # widget. Such an error shouldn't cause the operation to fail.
    # See the programmer's documentation of tixCmdErrorHandler for details.
    #
    catch {
	tixComboBox .c -command CmdNotFound
	.c invoke
	set cmdNotFailed 1
    }
    Assert {[info exists cmdNotFailed]}
    Assert {[info exists cmdHandlerCalled]}

    # Clean up
    #
    destroy .c
    rename tixCmdErrorHandler ""
    rename _default_tixCmdErrorHandler tixCmdErrorHandler
    unset cmdHandlerCalled

}
