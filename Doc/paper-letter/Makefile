# you could set this to a4
PAPER=letter

# Where are the various programs?
DVIPS=		dvips -N0 -t $(PAPER)
PYTHON=		python

TOOLSDIR=	../tools

MKHOWTO=	$(TOOLSDIR)/mkhowto
MKDVI=		$(MKHOWTO) --dvi
MKPDF=		$(MKHOWTO) --pdf


# what's what
MANDVIFILES=	api.dvi ext.dvi lib.dvi mac.dvi ref.dvi tut.dvi
HOWTODVIFILES=	doc.dvi inst.dvi dist.dvi

MANPDFFILES=	api.pdf ext.pdf lib.pdf mac.pdf ref.pdf tut.pdf
HOWTOPDFFILES=	doc.pdf inst.pdf dist.pdf

MANPSFILES=	api.ps ext.ps lib.ps mac.ps ref.ps tut.ps
HOWTOPSFILES=	doc.ps inst.ps dist.ps

DVIFILES=	$(MANDVIFILES) $(HOWTODVIFILES)
PDFFILES=	$(MANPDFFILES) $(HOWTOPDFFILES)
PSFILES=	$(MANPSFILES) $(HOWTOPSFILES)


include ../Makefile.deps


SCRIPTS:	  ../tools/indfix.py

$(DVIFILES):	  $(SCRIPTS) $(MKHOWTO)
$(PDFFILES):	  $(SCRIPTS) $(MKHOWTO)


# Rules to build PostScript and PDF formats
.SUFFIXES: .dvi .ps

.dvi.ps:
	$(DVIPS) -o $@ $<


all:	ps
dvi:	$(DVIFILES)
pdf:	$(PDFFILES)
ps:	$(PSFILES)


README:	$(PSFILES) $(TOOLSDIR)/getpagecounts
	$(TOOLSDIR)/getpagecounts >$@


# Python/C API Reference Manual
api.dvi: api.tex $(APIFILES)
	$(MKDVI) api.tex

api.pdf: api.tex $(APIFILES)
	$(MKPDF) api.tex

api.tex: ../api/api.tex ../api/refcounts.dat ../tools/anno-api.py
	$(PYTHON) ../tools/anno-api.py -o $@ ../api/api.tex

# Distributing Python Modules
dist.dvi: $(DISTFILES)
	$(MKDVI) ../dist/dist.tex

dist.pdf: $(DISTFILES)
	$(MKPDF) ../dist/dist.tex

# Documenting Python
doc.dvi: $(DOCFILES)
	$(MKHOWTO) --dvi ../doc/doc.tex

doc.pdf: $(DOCFILES)
	$(MKHOWTO) --pdf ../doc/doc.tex

# Extending and Embedding the Python Interpreter
ext.dvi: $(EXTFILES)
	$(MKDVI) ../ext/ext.tex

ext.pdf: $(EXTFILES)
	$(MKPDF) ../ext/ext.tex

# Installing Python Modules
inst.dvi: $(INSTFILES)
	$(MKHOWTO) --dvi ../inst/inst.tex

inst.pdf: $(INSTFILES)
	$(MKHOWTO) --pdf ../inst/inst.tex

# Python Library Reference
lib.dvi: $(LIBFILES)
	$(MKDVI) ../lib/lib.tex

lib.pdf: $(LIBFILES)
	$(MKPDF) ../lib/lib.tex

# Macintosh Library Modules
mac.dvi: $(MACFILES)
	$(MKHOWTO) --dvi ../mac/mac.tex

mac.pdf: $(MACFILES)
	$(MKHOWTO) --pdf ../mac/mac.tex

# Python Reference Manual
ref.dvi: $(REFFILES)
	$(MKDVI) ../ref/ref.tex

ref.pdf: $(REFFILES)
	$(MKPDF) ../ref/ref.tex

# Python Tutorial
tut.dvi: $(TUTFILES)
	$(MKDVI) ../tut/tut.tex

tut.pdf: $(TUTFILES)
	$(MKPDF) ../tut/tut.tex


clean:
	rm -f *~ *.aux *.idx *.ilg *.ind *.log *.toc *.bkm *.syn *.pla api.tex

clobber: clean
	rm -f $(DVIFILES) $(PSFILES) $(PDFFILES)

realclean: clobber
distclean: clobber
