# Generate the Python "info" documentation.

TOPDIR=..
TOOLSDIR=$(TOPDIR)/tools
HTMLDIR=$(TOPDIR)/html

MKINFO=$(TOOLSDIR)/mkinfo
SCRIPTS=$(TOOLSDIR)/html2texi.pl $(TOOLSDIR)/checkargs.pm $(TOOLSDIR)/mkinfo \
	$(TOOLSDIR)/fixinfo.el

all:	python-api.info python-ext.info python-lib.info \
	python-ref.info python-tut.info \
	python-dist.info python-inst.info


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

python-dist.info:	$(HTMLDIR)/dist/dist.html $(SCRIPTS)
	$(MKINFO) $<

python-inst.info:	$(HTMLDIR)/inst/inst.html $(SCRIPTS)
	$(MKINFO) $<

clean:
	rm -f *.texi~ *.texi

clobber: clean
	rm -f *.texi python-*.info python-*.info-[0-9]*


# This makes sure we can build info files from a "clean" tree,
# in case we haven't already built the HTML:

$(HTMLDIR)/api/api.html:
	(cd $(HTMLDIR); $(MAKE) api)

$(HTMLDIR)/ext/ext.html:
	(cd $(HTMLDIR); $(MAKE) ext)

$(HTMLDIR)/lib/lib.html:
	(cd $(HTMLDIR); $(MAKE) lib)

$(HTMLDIR)/mac/mac.html:
	(cd $(HTMLDIR); $(MAKE) mac)

$(HTMLDIR)/ref/ref.html:
	(cd $(HTMLDIR); $(MAKE) ref)

$(HTMLDIR)/tut/tut.html:
	(cd $(HTMLDIR); $(MAKE) tut)

$(HTMLDIR)/dist/dist.html:
	(cd $(HTMLDIR); $(MAKE) dist)

$(HTMLDIR)/inst/inst.html:
	(cd $(HTMLDIR); $(MAKE) inst)
