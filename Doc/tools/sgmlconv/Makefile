# Simple makefile to control SGML generation for the entire document tree.
# This should be used from the top-level directory (Doc/), not the directory
# that actually contains this file:
#
#  $ pwd
#  .../Doc
#  $ make -f tools/sgmlconv/Makefile

TOPDIR=.
TOOLSDIR=tools

SGMLRULES=../$(TOOLSDIR)/sgmlconv/make.rules
SUBDIRS=api ext lib mac ref tut
SUBMAKE=$(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR)

all:	xml

.PHONY: sgml xml
.PHONY: $(SUBDIRS)

sgml:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; $(SUBMAKE) sgml) || exit $$? ; done

xml:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; $(SUBMAKE) xml) || exit $$? ; done

esis:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; $(SUBMAKE) esis) || exit $$? ; done

tarball:  sgml
	tar cf - sgml tools/sgmlconv */*.sgml | gzip -9 >sgml-1.5.2b2.tgz

api:
	cd api; $(SUBMAKE)

ext:
	cd ext; $(SUBMAKE)

lib:
	cd lib; $(SUBMAKE)

mac:
	cd mac; $(SUBMAKE)

ref:
	cd ref; $(SUBMAKE)

tut:
	cd tut; $(SUBMAKE)

clean:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; $(SUBMAKE) clean) ; done

clobber:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; $(SUBMAKE) clobber) ; done
