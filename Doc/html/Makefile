# Convenience Makefile for building HTML documentation.  You probably
# need to set TEXINPUTS from the command line for this to be useful,
# unless you actually build the .dvi files in the top level directory.

PAPER=letter
TOPDIR=..
TOOLSDIR=$(TOPDIR)/tools
PAPERDIR=$(TOPDIR)/paper-$(PAPER)

TEXINPUTS=$(TOPDIR)/paper-$(PAPER):$(TOPDIR)/texinputs:

# Where are the various programs?
PYTHON=	   python
WEBCHECKER=$(PYTHON) $(TOPDIR)/../Tools/webchecker/webchecker.py
MKHOWTO=   $(TOOLSDIR)/mkhowto --about $(TOPDIR)/html/stdabout.dat \
		--address $(PYTHONDOCS) --up-link ../index.html \
		--up-title "Python Documentation Index"
MKHTML=	   $(MKHOWTO) --html

BUILDINDEX=$(TOOLSDIR)/buildindex.py

PYTHONDOCS='<hr>See <i><a href="about.html">About this document...</a></i> for information on suggesting changes.'
HTMLBASE=  file:`pwd`

INDEXFILES=api/api.html \
	doc/doc.html \
	ext/ext.html \
	lib/lib.html \
	mac/mac.html \
	ref/ref.html \
	tut/tut.html \
	inst/inst.html \
	dist/dist.html

COMMONPERL= $(TOPDIR)/perl/manual.perl \
	$(TOPDIR)/perl/python.perl \
	$(TOPDIR)/perl/l2hinit.perl


all:	$(INDEXFILES) index.html modindex.html acks.html

.PHONY: api ext lib mac ref tut inst dist

api: api/api.html
doc: doc/doc.html
ext: ext/ext.html
lib: lib/lib.html
mac: mac/mac.html
ref: ref/ref.html
tut: tut/tut.html
inst: inst/inst.html
dist: dist/dist.html

$(INDEXFILES): $(COMMONPERL) \
	       $(TOPDIR)/html/about.dat \
	       $(TOPDIR)/tools/node2label.pl

include ../Makefile.deps

# The index.html target is at the end since it screws up font-lock.

acks.html: ../ACKS $(TOOLSDIR)/support.py $(TOOLSDIR)/mkackshtml
	$(TOOLSDIR)/mkackshtml --address $(PYTHONDOCS) --output acks.html \
		<../ACKS

modindex.html: $(TOOLSDIR)/support.py $(TOOLSDIR)/mkmodindex
modindex.html: lib/lib.html mac/mac.html
	$(TOOLSDIR)/mkmodindex --columns 4 --output modindex.html \
		--address $(PYTHONDOCS) \
		lib/modindex.html mac/modindex.html

api/api.html:  $(APIFILES) $(BUILDINDEX)
	$(MKHTML) $(TOPDIR)/api/api.tex

doc/doc.html:  $(DOCFILES) $(BUILDINDEX)
	$(MKHTML) $(TOPDIR)/doc/doc.tex

ext/ext.html:  $(EXTFILES)
	echo $(EXTFILES)
	$(MKHTML) $(TOPDIR)/ext/ext.tex

lib/lib.html:  $(LIBFILES) $(BUILDINDEX)
	$(MKHTML) $(TOPDIR)/lib/lib.tex

mac/mac.html:  $(MACFILES) $(BUILDINDEX)
	$(MKHTML) $(TOPDIR)/mac/mac.tex

ref/ref.html:  $(REFFILES) $(BUILDINDEX)
	$(MKHTML) $(TOPDIR)/ref/ref.tex

tut/tut.html:  $(TUTFILES) $(TOOLSDIR)/mkhowto
	$(MKHTML) --numeric --split 3 $(TOPDIR)/tut/tut.tex

inst/inst.html: $(INSTFILES) $(TOPDIR)/perl/distutils.perl
	$(MKHTML) $(TOPDIR)/inst/inst.tex

dist/dist.html:$(DISTFILES) $(TOPDIR)/perl/distutils.perl
	$(MKHTML) $(TOPDIR)/dist/dist.tex

webcheck: all
	$(WEBCHECKER) $(HTMLBASE)/api/
	$(WEBCHECKER) $(HTMLBASE)/doc/
	$(WEBCHECKER) $(HTMLBASE)/ext/
	$(WEBCHECKER) -m290000 $(HTMLBASE)/lib/
	$(WEBCHECKER) $(HTMLBASE)/mac/
	$(WEBCHECKER) $(HTMLBASE)/ref/
	$(WEBCHECKER) $(HTMLBASE)/tut/
	$(WEBCHECKER) $(HTMLBASE)/dist/
	$(WEBCHECKER) $(HTMLBASE)/inst/

clean:
	rm -rf @webchecker.pickle

distclean realclean clobber: clean
	rm -rf index.html modindex.html acks.html
	rm -rf api/ doc/ ext/ lib/ mac/ ref/ tut/ inst/ dist/


# This is really ugly, but we're not dependent on $(RELEASE), which isn't
# defined here.  It also maintains the proper dependency on boilerplate.tex.

# It's at the end of the file since it wedges font-lock in XEmacs.

BOILERPLATE=$(TOPDIR)/texinputs/boilerplate.tex
index.html: $(TOPDIR)/html/index.html.in $(BOILERPLATE)
	REL=`grep '\\release{' $(BOILERPLATE) | sed 's/.*\\release{\(.*\)}.*$$/\1/'`; \
		REL=`echo "$$REL" | sed 's/[$$]//g'`; \
		sed "s/@RELEASE@/$$REL/g" $< >TEMP
	DATE=`grep '\\date{' $(BOILERPLATE) | sed 's/.*\\date{\(.*\)}.*$$/\1/'` ; \
		if [ "$$DATE" = '\today' ] ; then DATE=`date '+%B %e, %Y'`;fi;\
		sed "s/@DATE@/$$DATE/g" TEMP >$@
	rm -f TEMP
