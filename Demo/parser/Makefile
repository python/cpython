parser.dvi:  parser.tex ../../Doc/libparser.tex
	TEXINPUTS=../../Doc:: $(LATEX) parser

#  Use a new name for this; the included file uses 'clean' already....
clean-parser:
	rm -f *.log *.aux *.dvi *.pyc *.ps

dist:
	(cd ../..; \
	 tar cf - `cat Demo/parser/FILES` | gzip >parsermodule-1.4.tar.gz)

include ../../Doc/Makefile
