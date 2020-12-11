# GNUmakefile --
#
#	This Makefile builds the HTML version
#	of the Tix man pages. You normally do not need to use this
#	Makefile, as all the HTML man pages are already included in
#	the Tix source distribution.
#
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# $Id: Makefile,v 1.2 2001/12/09 03:25:15 idiscovery Exp $

TCLSH		= tclsh
MAN2HTML	= ../tools/tix-man2html.tcl
UPDATEIDX	= ../tools/man2html-fixindex.tcl
NROFF_SRCS	= \
	TixIntro.n compound.n pixmap.n tix.n tixBalloon.n tixButtonBox.n \
	tixCheckList.n tixComboBox.n tixControl.n tixDestroy.n tixDirList.n \
	tixDirSelectDialog.n tixDirTree.n tixDisplayStyle.n \
	tixExFileSelectBox.n tixExFileSelectDialog.n tixFileEntry.n \
	tixFileSelectBox.n tixFileSelectDialog.n tixForm.n tixGetBoolean.n \
	tixGetInt.n tixGrid.n tixHList.n tixInputOnly.n \
	tixLabelEntry.n tixLabelFrame.n tixListNoteBook.n \
	tixMeter.n tixMwm.n tixNBFrame.n tixNoteBook.n tixOptionMenu.n \
	tixPanedWindow.n tixPopupMenu.n tixScrolledHList.n tixScrolledListBox.n \
	tixScrolledText.n tixScrolledWindow.n tixSelect.n tixStdButtonBox.n \
	tixTList.n tixTree.n tixUtils.n tixwish.1
    
all: html/contents.htm

html/contents.htm: ${NROFF_SRCS} ${MAN2HTML} ${UPDATEIDX} GNUmakefile
	${TCLSH} ${MAN2HTML} --htmldir=html --srcdir=../..
	${TCLSH} ${UPDATEIDX} html

distclean:
	-cd html; rm -rf *.htm *~ Keywords UserCmd
	-cd html; rm -rf TclCmd TclLib
	-cd html; rm -rf TkCmd TkLib
	-cd html; rm -rf TixCmd TixLib

test:
	env TEST_ONLY=true $(MAKE) all

full:
	env WITH_TCL_TK=true $(MAKE) all
