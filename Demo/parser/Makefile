parser.dvi:  parser.tex ../../Doc/libparser.tex
	TEXINPUTS=../../Doc:: $(LATEX) parser

parser.ps: parser.dvi
	$(DVIPS) parser >parser.ps

parser.texi: @lib.texi
	$(EMACS) -batch -l ../../Doc/fix.el -f save-buffer -kill
	mv @lib.texi parser.texi

@lib.texi:../../Doc/libparser.tex \
	 texipre.dat ../../Doc/texipost.dat \
	 ../../Doc/partparse.py ../../Doc/fix.el
	$(PYTHON) ../../Doc/partparse.py -o @lib.texi \
		-t ../../Doc/texipost.dat \
		../../Doc/libparser.tex

parserwww: parser.texi
	if test -d html ; then true; else mkdir html ; fi
	$(PYTHON) ../../Doc/texi2html.py -d parser.texi html

#  Use a new name for this; the included file uses 'clean' already....
clean-parser:
	rm -f *.log *.aux *.dvi *.pyc *.ps

dist:
	(cd ../..; \
	 tar cf - `cat Demo/parser/FILES` | gzip >parsermodule-1.4.tar.gz)

include ../../Doc/Makefile
