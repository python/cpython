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
.PHONY: $(SUBDIRS)

sgml:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; \
	     $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR) sgml) || exit $$?; \
	done

xml:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; \
	     $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR) xml) || exit $$?; \
	done

tarball:  sgml
	tar cf - tools/sgmlconv */*.sgml | gzip -9 >sgml-1.5.2b1.tgz

api:
	cd api; $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR)

ext:
	cd ext; $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR)

lib:
	cd lib; $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR)

mac:
	cd mac; $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR)

ref:
	cd ref; $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR)

tut:
	cd tut; $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR)

clean:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; \
	     $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR) clean) \
	done

clobber:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; \
	     $(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR) clobber) \
	done
