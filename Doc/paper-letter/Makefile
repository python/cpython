# you could set this to a4
PAPER=letter

# Where are the various programs?
LATEX=		latex
PDFLATEX=	pdflatex
DVIPS=		dvips -N0 -t $(PAPER)
MAKEINDEX=	makeindex -s ../texinputs/myindex.ist
PYTHON=		python

TOOLSDIR=	../tools
TEXINPUTS=	.:../texinputs:

MKDVI=		TEXINPUTS=$(TEXINPUTS) $(TOOLSDIR)/mkdvi.sh
MKHOWTO=	$(TOOLSDIR)/mkhowto.sh --keep
MKPDF=		TEXINPUTS=$(TEXINPUTS) $(TOOLSDIR)/mkdvi.sh --pdf


# what's what
MANDVIFILES=	api.dvi ext.dvi lib.dvi ref.dvi tut.dvi
HOWTODVIFILES=	mac.dvi

MANPDFFILES=	api.pdf ext.pdf lib.pdf ref.pdf tut.pdf
HOWTOPDFFILES=	mac.pdf

MANPSFILES=	api.ps ext.ps lib.ps ref.ps tut.ps
HOWTOPSFILES=	mac.ps

DVIFILES=	$(MANDVIFILES) $(HOWTODVIFILES)
PDFFILES=	$(MANPDFFILES) $(HOWTOPDFFILES)
PSFILES=	$(MANPSFILES) $(HOWTOPSFILES)


include ../Makefile.deps

COMMONSTYLES= ../texinputs/python.sty \
	../texinputs/pypaper.sty \
	../texinputs/myindex.ist

COMMONTEX= ../texinputs/copyright.tex \
	../texinputs/boilerplate.tex

MANSTYLES= ../texinputs/fncychap.sty \
	../texinputs/manual.cls \
	$(COMMONSTYLES)

HOWTOSTYLES= ../texinputs/howto.cls \
	$(COMMONSTYLES)


$(DVIFILES):	  ../tools/fix_hack ../tools/mkdvi.sh $(COMMONTEX)
$(PDFFILES):	  ../tools/fix_hack ../tools/mkdvi.sh $(COMMONTEX)

$(MANDVIFILES):	  $(MANSTYLES)
$(MANPDFFILES):	  $(MANSTYLES)

$(HOWTODVIFILES): ../tools/mkhowto.sh $(HOWTOSTYLES)
$(HOWTOPDFFILES): ../tools/mkhowto.sh $(HOWTOSTYLES)


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


# Python Library Reference
lib.dvi: $(TOOLSDIR)/indfix.py $(LIBFILES)
	$(TOOLSDIR)/newind.py >$*.ind
	$(TOOLSDIR)/newind.py modindex >mod$*.ind
	TEXINPUTS=../lib:$(TEXINPUTS) $(LATEX) $*
	$(MAKEINDEX) mod$*.idx
	$(TOOLSDIR)/fix_hack $*.idx
	$(MAKEINDEX) $*.idx
	$(TOOLSDIR)/indfix.py $*.ind
	TEXINPUTS=../lib:$(TEXINPUTS) $(LATEX) $*

lib.pdf: $(TOOLSDIR)/indfix.py $(LIBFILES)
	$(TOOLSDIR)/newind.py >$*.ind
	$(TOOLSDIR)/newind.py modindex >mod$*.ind
	TEXINPUTS=../lib:$(TEXINPUTS) $(PDFLATEX) $*
	$(MAKEINDEX) mod$*.idx
	$(TOOLSDIR)/fix_hack $*.idx
	$(MAKEINDEX) $*.idx
	$(TOOLSDIR)/indfix.py $*.ind
	$(TOOLSDIR)/toc2bkm.py $*
	TEXINPUTS=../lib:$(TEXINPUTS) $(PDFLATEX) $*

# Python/C API Reference Manual
api.dvi: $(APIFILES)
	$(MKDVI) api

api.pdf: $(APIFILES)
	$(MKPDF) api

# Extending and Embedding the Python Interpreter
ext.dvi: $(EXTFILES)
	$(MKDVI) ext

ext.pdf: $(EXTFILES)
	$(MKPDF) ext

# Macintosh Library Modules
mac.dvi: $(MACFILES)
	$(MKHOWTO) --dvi ../$*/$*.tex

mac.pdf: $(MACFILES)
	$(MKHOWTO) --pdf ../$*/$*.tex

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
	rm -f *~ *.aux *.idx *.ilg *.ind *.log *.toc *.bkm *.syn

clobber: clean
	rm -f $(DVIFILES) $(PSFILES) $(PDFFILES)
