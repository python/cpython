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

ext:
	touch ext.ind
	latex ext
	./fix_hack ext.idx
	makeindex ext
	latex ext
	dvips ext >ext.ps

qua:
	latex qua
	bibtex qua
	latex qua
	latex qua
	dvips qua >qua.ps

lib.texi: lib1.tex lib2.tex lib3.tex lib4.tex lib5.tex texipre.dat texipost.dat
	python partparse.py -o @lib.texi lib[1-5].tex
	mv @lib.texi lib.texi

.PRECIOUS:	lib.texi

python-lib.info: lib.texi
	emacs -batch -l fix.el -f save-buffer -kill
	makeinfo +footnote-style end +fill-column 72 +paragraph-indent 0 \
	         lib.texi

lib.info: python-lib.info

# This target is very local to CWI...
libwww: lib.texi
	texi2html -d lib.texi /usr/local/ftp.cwi.nl/pub/www/texinfo/python

clean:
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl *.pyc
	# Sources: .tex, .bib, .sty
	# Useful results: .dvi, .ps, .texi, .info

clobber: clean
	 rm -f *.dvi *.ps *.texi *.info *.info-[0-9]*
