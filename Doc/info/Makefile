# Generate the Python "info" documentation.

PAPER=letter
TOPDIR=..
TOOLSDIR=$(TOPDIR)/tools
PAPERDIR=$(TOPDIR)/paper-$(PAPER)
HTMLDIR=$(TOPDIR)/html

MKINFO=$(TOOLSDIR)/mkinfo
SCRIPTS=$(TOOLSDIR)/html2texi.pl $(TOOLSDIR)/checkargs.pm $(TOOLSDIR)/mkinfo \
	$(TOOLSDIR)/fixinfo.el

all:	python-api.info python-ext.info python-lib.info \
	python-ref.info python-tut.info


python-api.info:	$(HTMLDIR)/api/api.html $(SCRIPTS)
	$(MKINFO) $<

python-ext.info:	$(HTMLDIR)/ext/ext.html $(SCRIPTS)
	$(MKINFO) $<

python-lib.info:	$(HTMLDIR)/lib/lib.html $(SCRIPTS)
	$(MKINFO) $<

# Not built by default; the conversion doesn't really handle it well.
python-mac.info:	$(HTMLDIR)/mac/mac.html $(SCRIPTS)
	$(MKINFO) $<

python-ref.info:	$(HTMLDIR)/ref/ref.html $(SCRIPTS)
	$(MKINFO) $<

python-tut.info:	$(HTMLDIR)/tut/tut.html $(SCRIPTS)
	$(MKINFO) $<


clean:
	rm -f *.texi~ *.texi

clobber: clean
	rm -f *.texi python-*.info python-*.info-[0-9]*


# The HTML files are dependent on the .aux files, which are dependent on the
# LaTeX source documents.  This makes sure we can build info files from a
# "clean" tree:

$(HTMLDIR)/api/api.html:  $(PAPERDIR)/api.aux $(BUILDINDEX)
	(cd $(TOPDIR); $(MAKE) htmlapi)

$(HTMLDIR)/ext/ext.html:  $(PAPERDIR)/ext.aux
	(cd $(TOPDIR); $(MAKE) htmlext)

$(HTMLDIR)/lib/lib.html:  $(PAPERDIR)/lib.aux $(BUILDINDEX)
	(cd $(TOPDIR); $(MAKE) htmllib)

$(HTMLDIR)/mac/mac.html:  $(MACFILES) $(BUILDINDEX)
	(cd $(TOPDIR); $(MAKE) htmlmac)

$(HTMLDIR)/ref/ref.html:  $(PAPERDIR)/ref.aux $(BUILDINDEX)
	(cd $(TOPDIR); $(MAKE) htmlref)

$(HTMLDIR)/tut/tut.html:  $(PAPERDIR)/tut.aux
	(cd $(TOPDIR); $(MAKE) htmltut)


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
