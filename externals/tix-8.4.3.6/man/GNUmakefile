# GNUmakefile --
#
#	This GNUmakefile requires GNU make. It builds the HTML version
#	of the Tix man pages. You normally do not need to use this
#	GNUmakefile, as all the HTML man pages are already included in
#	the Tix source distribution.
#
#	You need to use this file when you are create a Tix source
#	distribution from the Tix CVS tree. See
#	../docs/MakeSrcDist.txt for more information.
#
# Copyright (c) 2000-2001 Tix Project Group.
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# $Id: GNUmakefile,v 1.1 2001/01/22 08:01:22 ioilam Exp $

TCLSH		= tclsh
MAN2HTML	= ../tools/tix-man2html.tcl
UPDATEIDX	= ../tools/man2html-fixindex.tcl
NROFF_SRCS	= $(shell echo *.n *.1)

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


