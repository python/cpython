LATEX=		latex
DVIPS=		dvips
TEXPREVIEW=	texsgi

PRINT=		lpr

TBL=		tbl
TROFF=		ditroff
MS=		-ms
FUNNYTAB=	$$HOME/lib/funnytab
PREVIEW=	dpv -Pfa -f $(FUNNYTAB)
PSDIT=		psdit
REFER=		refer
PROCESS=	$(REFER) -e -n $*.ms | $(TBL) | $(TROFF) $(MS)

tut:		tut.dvi
		$(TEXPREVIEW) tut

tut.dvi:	tut.tex tut.toc

mod:		mod.dvi
		$(TEXPREVIEW) mod

mod.dvi:	mod.tex mod.toc

ALL=		tut.ps mod.ps

all:		$(ALL)

print:		$(ALL)
		$(PRINT) $(ALL)

$(ALL):		mystyle.sty

clean:
		rm -f *.dit *.ps core @* ,*

.SUFFIXES:	# Remove default suffixes

.SUFFIXES:	.ms .dit .tex .aux .toc .dvi .ps

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

.ms.dit:
		$(PROCESS) >$@

.dit.ps:
		$(PSDIT) <$*.dit >$@

.ms.ps:
		$(PROCESS) | $(PSDIT) >$@
