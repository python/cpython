# Simple makefile to control SGML generation for the entire document tree.
# This should be used from the top-level directory (Doc/), not the directory
# that actually contains this file:
#
#  $ pwd
#  .../Doc
#  $ make -f tools/sgmlconv/Makefile

TOPDIR=.
TOOLSDIR=$(TOPDIR)/tools

SGMLRULES=../$(TOOLSDIR)/sgmlconv/make.rules
SUBDIRS=api ext lib mac ref tut

all:	sgml

.PHONY: sgml xml

sgml:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; \
	     $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR) sgml) \
	done

xml:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; \
	     $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR) xml) \
	done
