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
	./fix_hack ref.idx
	makeindex ref
	latex ref
	dvips ref >ref.ps

lib:
	touch lib.ind
	latex lib
	./fix_hack lib.idx
	makeindex lib
	latex lib
	dvips lib >lib.ps

qua:
	latex qua
	bibtex qua
	latex qua
	latex qua
	dvips lib >lib.ps

libinfo:
	@echo This may take a while...
	python -c 'import partparse; partparse.main()' lib[1-5].tex
	sh fix.sh

clean:
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl *.pyc
	# Sources: .tex, .bib, .sty
	# Useful results: .dvi, .ps

clobber: clean
	 rm -f *.dvi *.ps *.info *.info-[0-9]*
