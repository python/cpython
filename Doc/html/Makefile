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
MKAUX=	   PAPER=$(PAPER) TEXINPUTS=$(TEXINPUTS) $(TOOLSDIR)/mkdvi.sh --aux
MKHTML=	   PAPER=$(PAPER) $(TOOLSDIR)/mkhtml.sh
KPSEWHICH= TEXINPUTS=$(TEXINPUTS) kpsewhich tex

BUILDINDEX=$(TOOLSDIR)/buildindex.py

# make it clear to l2h, since our support only generates HTML 4.0
L2HARGS=   -html_version 4.0

PYTHONDOCS='<hr>Send comments to <a href="mailto:python-docs@python.org">python-docs@python.org</a>.'
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


all:	$(INDEXFILES) modindex.html

.PHONY: api ext lib mac ref tut

api: api/api.html
doc: doc/doc.html
ext: ext/ext.html
lib: lib/lib.html
mac: mac/mac.html
ref: ref/ref.html
tut: tut/tut.html

$(INDEXFILES): $(COMMONPERL) $(TOPDIR)/html/about.dat

modindex.html: lib/lib.html mac/mac.html $(TOOLSDIR)/mkmodindex
	$(TOOLSDIR)/mkmodindex --columns 4 --output modindex.html \
		--address $(PYTHONDOCS) \
		lib/modindex.html mac/modindex.html

api/api.html:  $(PAPERDIR)/api.aux $(BUILDINDEX)
	$(MKHTML) api $(L2HARGS)

doc/doc.html:  $(DOCFILES) $(BUILDINDEX) $(TOPDIR)/perl/ltxmarkup.perl
	$(TOOLSDIR)/mkhowto --address $(PYTHONDOCS) --html \
		$(TOPDIR)/doc/doc.tex

ext/ext.html:  $(PAPERDIR)/ext.aux
	$(MKHTML) ext $(L2HARGS)

lib/lib.html:  $(PAPERDIR)/lib.aux $(BUILDINDEX)
	$(TOOLSDIR)/fix_libaux.sed <`$(KPSEWHICH) lib.aux` >lib1.aux
	mv lib1.aux `$(KPSEWHICH) lib.aux`
	$(MKHTML) lib $(L2HARGS)

mac/mac.html:  $(MACFILES) $(BUILDINDEX)
	$(TOOLSDIR)/mkhowto --address $(PYTHONDOCS) --html \
		$(TOPDIR)/mac/mac.tex

ref/ref.html:  $(PAPERDIR)/ref.aux $(BUILDINDEX)
	$(MKHTML) ref $(L2HARGS)

tut/tut.html:  $(PAPERDIR)/tut.aux
	$(MKHTML) tut $(L2HARGS)


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

clean:
	rm -rf @webchecker.pickle

clobber: clean
	rm -rf api/ doc/ ext/ lib/ mac/ ref/ tut/
