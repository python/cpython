# you could set this to a4
PAPER=letter

# Where are the various programs?
LATEX=		latex
PDFLATEX=	pdflatex
DVIPS=		dvips -N0 -t $(PAPER)
MAKEINDEX=	makeindex -s ../texinputs/python.ist
PYTHON=		python

TOOLSDIR=	../tools
TEXINPUTS=	.:../texinputs:

MKDVI=		TEXINPUTS=$(TEXINPUTS) $(TOOLSDIR)/mkdvi.sh
# The --keep option is needed to keep the intermediate files for generating
# the HTML global module index.
MKHOWTO=	$(TOOLSDIR)/mkhowto --keep
MKPDF=		TEXINPUTS=$(TEXINPUTS) $(TOOLSDIR)/mkdvi.sh --pdf


# what's what
MANDVIFILES=	api.dvi ext.dvi lib.dvi ref.dvi tut.dvi
HOWTODVIFILES=	doc.dvi mac.dvi

MANPDFFILES=	api.pdf ext.pdf lib.pdf ref.pdf tut.pdf
HOWTOPDFFILES=	doc.pdf mac.pdf

MANPSFILES=	api.ps ext.ps lib.ps ref.ps tut.ps
HOWTOPSFILES=	doc.ps mac.ps

DVIFILES=	$(MANDVIFILES) $(HOWTODVIFILES)
PDFFILES=	$(MANPDFFILES) $(HOWTOPDFFILES)
PSFILES=	$(MANPSFILES) $(HOWTOPSFILES)


include ../Makefile.deps

COMMONSTYLES= ../texinputs/python.sty \
	../texinputs/pypaper.sty \
	../texinputs/python.ist

COMMONTEX= ../texinputs/copyright.tex \
	../texinputs/boilerplate.tex

MANSTYLES= ../texinputs/fncychap.sty \
	../texinputs/manual.cls \
	$(COMMONSTYLES)

HOWTOSTYLES= ../texinputs/howto.cls \
	$(COMMONSTYLES)


SCRIPTS:	  ../tools/fix_hack ../tools/mkdvi.sh ../tools/indfix.py

$(DVIFILES):	  $(SCRIPTS) $(COMMONTEX)
$(PDFFILES):	  $(SCRIPTS) $(COMMONTEX)

$(MANDVIFILES):	  $(MANSTYLES)
$(MANPDFFILES):	  $(MANSTYLES)

$(HOWTODVIFILES): ../tools/mkhowto $(HOWTOSTYLES)
$(HOWTOPDFFILES): ../tools/mkhowto $(HOWTOSTYLES)


# Rules to build PostScript and PDF formats
.SUFFIXES: .dvi .ps .pdf

.dvi.ps:
	$(DVIPS) -o $@ $<


all:	ps
dvi:	$(DVIFILES)
pdf:	$(PDFFILES)
ps:	$(PSFILES)


README:	$(PSFILES)
	$(TOOLSDIR)/getpagecounts >$@


# Python/C API Reference Manual
api.dvi: $(APIFILES)
	$(MKDVI) api

api.pdf: $(APIFILES)
	$(MKPDF) api

# Documenting Python
doc.dvi: $(DOCFILES) ../texinputs/ltxmarkup.sty
	$(MKHOWTO) --dvi ../doc/doc.tex

doc.pdf: $(DOCCFILES) ../texinputs/ltxmarkup.sty
	$(MKHOWTO) --pdf ../doc/doc.tex

# Extending and Embedding the Python Interpreter
ext.dvi: $(EXTFILES)
	$(MKDVI) ext

ext.pdf: $(EXTFILES)
	$(MKPDF) ext

# Python Library Reference
lib.dvi: $(LIBFILES)
	$(MKDVI) lib

lib.pdf: $(LIBFILES)
	$(MKPDF) lib

# Macintosh Library Modules
mac.dvi: $(MACFILES)
	$(MKHOWTO) --dvi ../mac/mac.tex

mac.pdf: $(MACFILES)
	$(MKHOWTO) --pdf ../mac/mac.tex

# Python Reference Manual
ref.dvi: $(REFFILES)
	$(MKDVI) ref

ref.pdf: $(REFFILES)
	$(MKPDF) ref

# Python Tutorial
tut.dvi: $(TUTFILES)
	$(MKDVI) tut

tut.pdf: $(TUTFILES)
	$(MKPDF) tut


clean:
	rm -f *~ *.aux *.idx *.ilg *.ind *.log *.toc *.bkm *.syn *.pla

clobber: clean
	rm -f $(DVIFILES) $(PSFILES) $(PDFFILES)

realclean: clobber
distclean: clobber
