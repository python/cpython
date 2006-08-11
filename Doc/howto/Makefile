# Makefile for the HOWTO directory
# LaTeX HOWTOs can be turned into HTML, PDF, PS, DVI or plain text output.
# reST HOWTOs can only be turned into HTML.

# Variables to change

# Paper size for non-HTML formats (letter or a4)
PAPER=letter

# Arguments to rst2html.py, and location of the script
RSTARGS = --input-encoding=utf-8
RST2HTML = rst2html.py

# List of HOWTOs that aren't to be processed.  This should contain the
# base name of the HOWTO without any extension (e.g. 'advocacy',
# 'unicode').
REMOVE_HOWTOS =

MKHOWTO=../tools/mkhowto
WEBDIR=.
PAPERDIR=../paper-$(PAPER)
HTMLDIR=../html

# Determine list of files to be built
TEX_SOURCES = $(wildcard *.tex)
RST_SOURCES = $(wildcard *.rst)
TEX_NAMES = $(filter-out $(REMOVE_HOWTOS),$(patsubst %.tex,%,$(TEX_SOURCES)))

PAPER_PATHS=$(addprefix $(PAPERDIR)/,$(TEX_NAMES))
DVI  =$(addsuffix .dvi,$(PAPER_PATHS))
PDF  =$(addsuffix .pdf,$(PAPER_PATHS))
PS   =$(addsuffix .ps,$(PAPER_PATHS))

ALL_HOWTO_NAMES = $(TEX_NAMES) $(patsubst %.rst,%,$(RST_SOURCES))
HOWTO_NAMES = $(filter-out $(REMOVE_HOWTOS),$(ALL_HOWTO_NAMES))
HTML = $(addprefix $(HTMLDIR)/,$(HOWTO_NAMES))

# Rules for building various formats

# reST to HTML
$(HTMLDIR)/%: %.rst
	if [ ! -d $@ ] ; then mkdir $@ ; fi
	$(RST2HTML) $(RSTARGS) $< >$@/index.html

# LaTeX to various output formats
$(PAPERDIR)/%.dvi : %.tex
	$(MKHOWTO) --dvi $<
	mv $*.dvi $@

$(PAPERDIR)/%.pdf : %.tex
	$(MKHOWTO) --pdf $<
	mv $*.pdf $@

$(PAPERDIR)/%.ps : %.tex
	$(MKHOWTO) --ps $<
	mv $*.ps $@

$(HTMLDIR)/% : %.tex
	$(MKHOWTO) --html --iconserver="." --dir $@ $<

# Rule that isn't actually used -- we no longer support the 'txt' target.
$(PAPERDIR)/%.txt : %.tex
	$(MKHOWTO) --text $<
	mv $@ txt

default:
	@echo "'all'    -- build all files"
	@echo "'dvi', 'pdf', 'ps', 'html' -- build one format"

all: dvi pdf ps html

.PHONY : dvi pdf ps html
dvi:  $(DVI)
pdf:  $(PDF)
ps:   $(PS)
html: $(HTML)

clean:
	rm -f *~ *.log *.ind *.l2h *.aux *.toc *.how *.bkm
	rm -f *.dvi *.pdf *.ps

clobber:
	rm -rf $(HTML)
	rm -rf $(DVI) $(PDF) $(PS)
