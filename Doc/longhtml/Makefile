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
MKAUX=	   PAPER=$(PAPER) TEXINPUTS=$(TEXINPUTS) $(TOOLSDIR)/mkdvi.sh --aux
MKHTML=	   PAPER=$(PAPER) $(TOOLSDIR)/mkhtml.sh --numeric
KPSEWHICH= TEXINPUTS=$(TEXINPUTS) kpsewhich tex

BUILDINDEX=$(TOOLSDIR)/buildindex.py

# make it clear to l2h, since our support only generates HTML 4.0
L2HARGS=   -html_version 4.0

PYTHONDOCS='<hr>Send comments on this document to <a href="mailto:python-docs@python.org">python-docs@python.org</a>.'
HTMLBASE=  file:`pwd`

INDEXFILES=api/api.html \
	doc/doc.html \
	ext/ext.html \
	lib/lib.html \
	mac/mac.html \
	ref/ref.html \
	tut/tut.html

COMMONPERL= $(TOPDIR)/perl/manual.perl \
	$(TOPDIR)/perl/python.perl \
	$(TOPDIR)/perl/l2hinit.perl


all:	$(INDEXFILES) icons

.PHONY: api ext lib mac ref tut

api: api/api.html
doc: doc/doc.html
ext: ext/ext.html
lib: lib/lib.html
mac: mac/mac.html
ref: ref/ref.html
tut: tut/tut.html

$(INDEXFILES): $(COMMONPERL) \
	       $(TOPDIR)/html/about.dat

icons:	../html/icons
	if [ ! -d icons ] ; then mkdir icons; fi
	cp ../html/icons/*.* icons/

api/api.html:  $(PAPERDIR)/api.aux $(BUILDINDEX) $(TOOLSDIR)/mkhtml.sh
	$(MKHTML) api $(L2HARGS) -split 3

doc/doc.html:  $(DOCFILES) $(BUILDINDEX) $(TOPDIR)/perl/ltxmarkup.perl
	$(TOOLSDIR)/mkhowto --numeric --split=3 --address $(PYTHONDOCS) \
		--html $(TOPDIR)/doc/doc.tex

ext/ext.html:  $(PAPERDIR)/ext.aux $(TOOLSDIR)/mkhtml.sh
	$(MKHTML) ext $(L2HARGS) -split 3

lib/lib.html:  $(PAPERDIR)/lib.aux $(BUILDINDEX) $(TOOLSDIR)/mkhtml.sh
	$(TOOLSDIR)/fix_libaux.sed <`$(KPSEWHICH) lib.aux` >lib1.aux
	mv lib1.aux `$(KPSEWHICH) lib.aux`
	$(MKHTML) lib $(L2HARGS) -split 3

mac/mac.html:  $(MACFILES) $(BUILDINDEX)
	$(TOOLSDIR)/mkhowto --numeric --split=3 --address $(PYTHONDOCS) \
		--html $(TOPDIR)/mac/mac.tex

ref/ref.html:  $(PAPERDIR)/ref.aux $(BUILDINDEX) $(TOOLSDIR)/mkhtml.sh
	$(MKHTML) ref $(L2HARGS) -split 3

tut/tut.html:  $(PAPERDIR)/tut.aux $(TOOLSDIR)/mkhtml.sh
	$(MKHTML) tut $(L2HARGS) -split 3


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


clean:
	:

clobber:
	rm -rf api/ doc/ ext/ lib/ mac/ ref/ tut/
