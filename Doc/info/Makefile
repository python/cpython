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
	api dist ext mac ref tut whatsnew \
	lib
#	doc inst

api:	python$(VERSION)-api.info
dist:	python$(VERSION)-dist.info
doc:	python$(VERSION)-doc.info
ext:	python$(VERSION)-ext.info
inst:	python$(VERSION)-inst.info
lib:	python$(VERSION)-lib.info
mac:	python$(VERSION)-mac.info
ref:	python$(VERSION)-ref.info
tut:	python$(VERSION)-tut.info
whatsnew:	$(WHATSNEW)
$(WHATSNEW):	python$(VERSION)-$(WHATSNEW).info

check-emacs-version:
	@v="`$(EMACS) --version 2>&1 | egrep '^(GNU |X)Emacs [12]*'`"; \
	if `echo "$$v" | grep '^GNU Emacs 2[12]' >/dev/null 2>&1`; then \
	  echo "Using $(EMACS) to build the info docs"; \
	else \
	  echo "GNU Emacs 21 or 22 is required to build the info docs"; \
	  echo "Found $$v"; \
	  false; \
	fi

python$(VERSION)-api.info:	../api/api.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

python$(VERSION)-ext.info:	../ext/ext.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

python$(VERSION)-lib.info:	../lib/lib.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

python$(VERSION)-mac.info:	../mac/mac.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

python$(VERSION)-ref.info:	../ref/ref.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

python$(VERSION)-tut.info:	../tut/tut.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

# Not built by default; the conversion doesn't handle \p and \op
python$(VERSION)-doc.info:	../doc/doc.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

python$(VERSION)-dist.info:	../dist/dist.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

# Not built by default; the conversion chokes on \installscheme
python$(VERSION)-inst.info:	../inst/inst.tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

# "whatsnew20" doesn't currently work
python$(VERSION)-$(WHATSNEW).info:  ../whatsnew/$(WHATSNEW).tex $(SCRIPTS)
	EMACS=$(EMACS) $(MKINFO) $< $*.texi $@

clean:
	rm -f *.texi~ *.texi

clobber: clean
	rm -f *.texi python*-*.info python*-*.info-[0-9]*
