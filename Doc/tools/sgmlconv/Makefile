# Simple makefile to control XML generation for the entire document tree.
# This should be used from the top-level directory (Doc/), not the directory
# that actually contains this file:
#
#  $ pwd
#  .../Doc
#  $ make -f tools/sgmlconv/Makefile

TOPDIR=.
TOOLSDIR=tools

SGMLRULES=../$(TOOLSDIR)/sgmlconv/make.rules
# The 'inst' directory breaks the conversion, so skip it for now.
SUBDIRS=api dist ext lib mac ref tut
SUBMAKE=$(MAKE) -f $(SGMLRULES) TOOLSDIR=../$(TOOLSDIR)

all:	xml

.PHONY: esis xml
.PHONY: $(SUBDIRS)

xml:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; $(SUBMAKE) xml) || exit $$? ; done

esis:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; $(SUBMAKE) esis) || exit $$? ; done

esis1:
	for DIR in $(SUBDIRS) ; do \
	    (cd $$DIR; $(SUBMAKE) esis1) || exit $$? ; done

tarball:  xml
	tar cf - tools/sgmlconv */*.xml | gzip -9 >xml-1.5.2b2.tgz

api:
	cd api; $(SUBMAKE)

dist:
	cd dist; $(SUBMAKE)

ext:
	cd ext; $(SUBMAKE)

inst:
	cd inst; $(SUBMAKE)

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
