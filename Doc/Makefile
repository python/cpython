# Makefile for Python documentation.

LATEX=		latex
DVIPS=		dvips
TEXPREVIEW=	xdvi

PRINT=		lpr

tut:		tut.dvi
		$(TEXPREVIEW) tut

tut.dvi tut.ps:	tut.toc tut.tex myformat.sty

mod:		mod.dvi
		$(TEXPREVIEW) mod

mod.dvi mod.ps:	mod.toc mod.tex mod1.tex mod2.tex mod3.tex myformat.sty

ALL=		tut.ps mod.ps

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
