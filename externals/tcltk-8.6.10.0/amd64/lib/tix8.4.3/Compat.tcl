# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: Compat.tcl,v 1.3 2004/03/28 02:44:57 hobbs Exp $
#
# Compat.tcl --
#
# 	This file wraps around many incompatibilities from Tix 3.6
#	to Tix 4.0.
#
#	(1) "box" to "Box" changes
#	(2) "DlgBtns" to "ButtonBox" changes
#
# Copyright (c) 1993-1999 Ioi Kim Lam.
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

foreach {old new} {
    tixDlgBtns tixButtonBox
    tixStdDlgBtns tixStdButtonBox
    tixCombobox tixComboBox
    tixFileSelectbox tixFileSelectBox
    tixScrolledListbox tixScrolledListBox
} {
    interp alias {} $old {} $new
}

proc tixInit {args} {
    eval tix config $args
    puts stderr "tixInit no longer needed for this version of Tix"
}
