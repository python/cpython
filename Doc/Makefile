DESTDIR=/usr/local
LIBDESTDIR=$DESTDIR/lib
LIBDEST=$LIBDESTDIR/python
DOCDESTDIR=$LIBDEST/doc

all:	tut ref lib qua

tut:
	latex tut
	latex tut
	dvips tut >tut.ps

ref:
	touch ref.ind
	latex ref
	makeindex ref
	latex ref
	dvips ref >ref.ps

lib:
	touch lib.ind
	latex lib
	makeindex lib
	latex lib
	dvips lib >lib.ps

qua:
	latex qua
	bibtex qua
	latex qua
	latex qua
	dvips lib >lib.ps

clean:
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl
	# Sources: .tex, .bib, .sty
	# Useful results: .dvi, .ps

clobber: clean
	 rm -f *.dvi *.ps
