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
PYTHON=		python
WEBCHECKER=	$(PYTHON) $(TOPDIR)/../Tools/webchecker/webchecker.py
MKHTML=		PAPER=$(PAPER) $(TOOLSDIR)/mkhtml.sh
KPSEWHICH=	TEXINPUTS=$(TEXINPUTS) kpsewhich tex

BUILDINDEX=	$(TOOLSDIR)/buildindex.py

INDEXFILES=api/api.html \
	ext/ext.html \
	lib/lib.html \
	mac/mac.html \
	ref/ref.html \
	tut/tut.html

COMMONPERL= $(TOPDIR)/perl/manual.perl \
	$(TOPDIR)/perl/python.perl \
	$(TOPDIR)/perl/l2hinit.perl


all:	icons $(INDEXFILES)

.PHONY: api ext lib mac ref tut

api: icons api/api.html
ext: icons ext/ext.html
lib: icons lib/lib.html
mac: icons mac/mac.html
ref: icons ref/ref.html
tut: icons tut/tut.html

$(INDEXFILES): $(COMMONPERL) $(TOPDIR)/html/about.dat

api/api.html:  $(PAPERDIR)/api.aux $(BUILDINDEX)
	$(MKHTML) api $(L2HARGS)

ext/ext.html:  $(PAPERDIR)/ext.aux
	$(MKHTML) ext $(L2HARGS)

lib/lib.html:  $(PAPERDIR)/lib.aux $(BUILDINDEX)
	$(TOOLSDIR)/fix_libaux.sed <`$(KPSEWHICH) lib.aux` >lib1.aux
	mv lib1.aux `$(KPSEWHICH) lib.aux`
	$(MKHTML) lib $(L2HARGS)

mac/mac.html:  $(MACFILES) $(BUILDINDEX)
	$(TOOLSDIR)/mkhowto.sh --address '<hr>Send comments to <a href="mailto:python-docs@python.org">python-docs@python.org</a>.' --html $(TOPDIR)/mac/mac.tex

ref/ref.html:  $(PAPERDIR)/ref.aux $(BUILDINDEX)
	$(MKHTML) ref $(L2HARGS)

tut/tut.html:  $(PAPERDIR)/tut.aux
	$(MKHTML) tut $(L2HARGS)


include ../Makefile.deps

$(PAPERDIR)/api.aux: $(APIFILES)
	(cd $(PAPERDIR); $(MAKE) PAPER=$(PAPER) api.dvi)

$(PAPERDIR)/ext.aux: $(EXTFILES)
	(cd $(PAPERDIR); $(MAKE) PAPER=$(PAPER) ext.dvi)

$(PAPERDIR)/lib.aux: $(LIBFILES)
	(cd $(PAPERDIR); $(MAKE) PAPER=$(PAPER) lib.dvi)

$(PAPERDIR)/ref.aux: $(REFFILES)
	(cd $(PAPERDIR); $(MAKE) PAPER=$(PAPER) ref.dvi)

$(PAPERDIR)/tut.aux: $(TUTFILES)
	(cd $(PAPERDIR); $(MAKE) PAPER=$(PAPER) tut.dvi)

# copy the icons/ directory over...
icons:
	mkdir icons/
	cp $(TOPDIR)/icons/*.gif icons/


webcheck:
	$(WEBCHECKER) file:`pwd`/$(HTMLDIR)/api/
	$(WEBCHECKER) file:`pwd`/$(HTMLDIR)/ext/
	$(WEBCHECKER) -m290000 file:`pwd`/$(HTMLDIR)/lib/
	$(WEBCHECKER) file:`pwd`/$(HTMLDIR)/mac/
	$(WEBCHECKER) file:`pwd`/$(HTMLDIR)/ref/
	$(WEBCHECKER) file:`pwd`/$(HTMLDIR)/tut/

clean:
	rm -rf icons/ @webchecker.pickle

clobber: clean
	rm -rf api/ ext/ lib/ mac/ ref/ tut/
