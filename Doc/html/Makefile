# Convenience Makefile for building HTML documentation.  You probably need to
# set TEXINPUTS from the command line for this to be useful, unless you
# actually build the .dvi files in the top level directory.
#
# Note that the .dvi files must already be built and TEXINPUTS must include the
# directory where latex's working files (esp. *.aux) are kept.

PAPER=letter
TOPDIR=..
TOOLSDIR=$(TOPDIR)/tools
PAPERDIR=$(TOPDIR)/paper-$(PAPER)

TEXINPUTS=$(TOPDIR)/paper-$(PAPER):$(TOPDIR)/texinputs:

# Where are the various programs?
PYTHON=	   python
WEBCHECKER=$(PYTHON) $(TOPDIR)/../Tools/webchecker/webchecker.py
WCNEW=$(PYTHON) $(TOPDIR)/../Tools/webchecker/wcnew.py
MKAUX=	   PAPER=$(PAPER) TEXINPUTS=$(TEXINPUTS) $(TOOLSDIR)/mkdvi.sh --aux
MKHTML=	   PAPER=$(PAPER) $(TOOLSDIR)/mkhtml.sh
KPSEWHICH= TEXINPUTS=$(TEXINPUTS) kpsewhich tex

BUILDINDEX=$(TOOLSDIR)/buildindex.py

# make it clear to l2h, since our support only generates HTML 4.0
L2HARGS=   -html_version 4.0

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


all:	$(INDEXFILES) index.html modindex.html

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

# The index.html target is at the end since it screws up font-lock.

modindex.html: lib/lib.html mac/mac.html $(TOOLSDIR)/mkmodindex
	$(TOOLSDIR)/mkmodindex --columns 4 --output modindex.html \
		--address $(PYTHONDOCS) \
		lib/modindex.html mac/modindex.html

api/api.html:  $(PAPERDIR)/api.aux $(BUILDINDEX) $(TOOLSDIR)/mkhtml.sh
	$(MKHTML) api $(L2HARGS) -split 5

doc/doc.html:  $(DOCFILES) $(BUILDINDEX) $(TOPDIR)/perl/ltxmarkup.perl
	$(TOOLSDIR)/mkhowto --about stdabout.dat --address $(PYTHONDOCS) \
		--html $(TOPDIR)/doc/doc.tex

ext/ext.html:  $(PAPERDIR)/ext.aux $(TOOLSDIR)/mkhtml.sh
	$(MKHTML) ext $(L2HARGS) -split 5

lib/lib.html:  $(PAPERDIR)/lib.aux $(BUILDINDEX) $(TOOLSDIR)/mkhtml.sh
	$(TOOLSDIR)/fix_libaux.sed <`$(KPSEWHICH) lib.aux` >lib1.aux
	mv lib1.aux `$(KPSEWHICH) lib.aux`
	$(MKHTML) lib $(L2HARGS) -split 5

mac/mac.html:  $(MACFILES) $(BUILDINDEX)
	$(TOOLSDIR)/mkhowto --about stdabout.dat --address $(PYTHONDOCS) \
		--html $(TOPDIR)/mac/mac.tex

ref/ref.html:  $(PAPERDIR)/ref.aux $(BUILDINDEX) $(TOOLSDIR)/mkhtml.sh
	$(MKHTML) ref $(L2HARGS) -split 5

tut/tut.html:  $(PAPERDIR)/tut.aux $(TOOLSDIR)/mkhtml.sh
	$(MKHTML) --numeric tut $(L2HARGS) -split 3

inst/inst.html:$(INSTFILES) $(TOPDIR)/perl/distutils.perl
	$(TOOLSDIR)/mkhowto --about stdabout.dat --address $(PYTHONDOCS) \
		--html $(TOPDIR)/inst/inst.tex

dist/dist.html:$(DISTFILES) $(TOPDIR)/perl/distutils.perl
	$(TOOLSDIR)/mkhowto --about stdabout.dat --address $(PYTHONDOCS) \
		--html $(TOPDIR)/dist/dist.tex

include ../Makefile.deps

$(PAPERDIR)/api.aux: $(APIFILES)
	(cd $(PAPERDIR); $(MKAUX) api)

$(PAPERDIR)/ext.aux: $(EXTFILES)
	(cd $(PAPERDIR); $(MKAUX) ext)

$(PAPERDIR)/lib.aux: $(LIBFILES)
	(cd $(PAPERDIR); $(MKAUX) lib)

$(PAPERDIR)/ref.aux: $(REFFILES)
	(cd $(PAPERDIR); $(MKAUX) ref)

$(PAPERDIR)/tut.aux: $(TUTFILES)
	(cd $(PAPERDIR); $(MKAUX) tut)


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

wcnew: all
	$(WCNEW) $(HTMLBASE)/api/
	$(WCNEW) $(HTMLBASE)/doc/
	$(WCNEW) $(HTMLBASE)/ext/
	$(WCNEW) -m290000 $(HTMLBASE)/lib/
	$(WCNEW) $(HTMLBASE)/mac/
	$(WCNEW) $(HTMLBASE)/ref/
	$(WCNEW) $(HTMLBASE)/tut/


clean:
	rm -rf @webchecker.pickle

distclean realclean clobber: clean
	rm -rf index.html modindex.html 
	rm -rf api/ doc/ ext/ lib/ mac/ ref/ tut/ inst/ dist/


# This is really ugly, but we're not dependent on $(RELEASE), which isn't
# defined here.  It also maintains the proper dependency on boilerplate.tex.

# It's at the end of the file since it wedges font-lock in XEmacs.

BOILERPLATE=$(TOPDIR)/texinputs/boilerplate.tex
index.html: index.html.in $(BOILERPLATE)
	REL=`grep '\\release{' $(BOILERPLATE) | sed 's/.*\\release{\(.*\)}.*$$/\1/'`; \
		REL=`echo "$$REL" | sed 's/[$$]//g'`; \
		sed "s/@RELEASE@/$$REL/g" $< >TEMP
	DATE=`grep '\\date{' $(BOILERPLATE) | sed 's/.*\\date{\(.*\)}.*$$/\1/'` ; \
		if [ "$$DATE" = '\today' ] ; then DATE=`date '+%B %e, %Y'`;fi;\
		sed "s/@DATE@/$$DATE/g" TEMP >$@
	rm -f TEMP

