# Generate the Python "info" documentation.

TOPDIR=..
TOOLSDIR=$(TOPDIR)/tools
HTMLDIR=$(TOPDIR)/html

# The emacs binary used to build the info docs. GNU Emacs 21 is required.
EMACS=emacs

MKINFO=$(TOOLSDIR)/mkinfo
SCRIPTS=$(TOOLSDIR)/checkargs.pm $(TOOLSDIR)/mkinfo $(TOOLSDIR)/py2texi.el

# set VERSION to code the VERSION number into the info file name
# allowing installation of more than one set of python info docs
# into the same directory
VERSION=

all:	check-emacs-version \
	python$(VERSION)-api.info python$(VERSION)-ext.info \
	python$(VERSION)-lib.info python$(VERSION)-ref.info \
	python$(VERSION)-tut.info python$(VERSION)-dist.info

#	python$(VERSION)-doc.info python$(VERSION)-inst.info
#	python$(VERSION)-mac.info

check-emacs-version:
	@v="`$(EMACS) --version 2>&1 | egrep '^(GNU |X)Emacs [12]*'`"; \
	if `echo "$$v" | grep '^GNU Emacs 21' >/dev/null 2>&1`; then \
	  echo "Using $(EMACS) to build the info docs"; \
	else \
	  echo "GNU Emacs 21 is required to build the info docs"; \
	  echo "Found $$v"; \
	  false; \
	fi

python$(VERSION)-api.info:	../api/api.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

python$(VERSION)-ext.info:	../ext/ext.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

python$(VERSION)-lib.info:	../lib/lib.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

# Not built by default; the conversion doesn't really handle it well.
python$(VERSION)-mac.info:	../mac/mac.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

python$(VERSION)-ref.info:	../ref/ref.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

python$(VERSION)-tut.info:	../tut/tut.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

# Not built by default; the conversion doesn't handle it at all.
python$(VERSION)-doc.info:	../doc/doc.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

python$(VERSION)-dist.info:	../dist/dist.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

# Not built by default; the conversion chokes on two @end multitable's
python$(VERSION)-inst.info:	../inst/inst.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $@

clean:
	rm -f *.texi~ *.texi

clobber: clean
	rm -f *.texi python*-*.info python*-*.info-[0-9]*
