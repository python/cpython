# Makefile for Python documentation.
# Actually, it is easier to just do:
#	latex tut
#	latex tut
#	latex lib
#	latex lib
# (Doing everything twice so the table of contents comes out right!)

LATEX=		latex
DVIPS=		dvips
TEXPREVIEW=	xdvi

PRINT=		lpr

tut:		tut.dvi
		$(TEXPREVIEW) tut

tut.dvi tut.ps:	tut.toc tut.tex myformat.sty

lib:		lib.dvi
		$(TEXPREVIEW) lib

lib.dvi lib.ps:	lib.toc lib.tex lib1.tex lib2.tex lib3.tex myformat.sty

ALL=		tut.ps lib.ps

all:		$(ALL)

print:		$(ALL)
		$(PRINT) $(ALL)

clean:
		rm -f *.dvi *.aux *.toc *.log *.ps core [#@,]* *~

.SUFFIXES:	# Remove default suffixes

.SUFFIXES:	.tex .aux .toc .dvi .ps

.tex.aux:
		$(LATEX) $*

.tex.toc:
		$(LATEX) $*

.tex.dvi:
		$(LATEX) $*

.dvi.ps:
		$(DVIPS) $* >$*.ps

.tex.ps:
		$(LATEX) $*
		$(DVIPS) $* >$*.ps
