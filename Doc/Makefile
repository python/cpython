# Makefile for Python documentation.
# Actually, it is easier to just do:
#	latex tut
#	latex tut
#	latex lib
#	latex lib
#	latex ref
#	latex ref
# (Doing everything twice so the table of contents comes out right!)

LATEX=		latex
DVIPS=		dvips
TEXPREVIEW=	xdvi

PRINT=		lpr

ALL=		tut.ps lib.ps ref.ps

all:		$(ALL)

tut:		tut.dvi
		$(TEXPREVIEW) tut

tut.dvi tut.ps:	tut.toc tut.tex myformat.sty

lib:		lib.dvi
		$(TEXPREVIEW) lib

lib.dvi lib.ps:	lib.toc lib.tex lib1.tex lib2.tex lib3.tex myformat.sty

ref:		ref.dvi
		$(TEXPREVIEW) ref

ref.dvi ref.ps:	ref.toc ref.tex myformat.sty

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
