all:	tut ref lib # qua

tut:
	latex tut
	latex tut

ref:
	touch ref.ind
	latex ref
	makeindex ref
	latex ref

lib:
	touch lib.ind
	latex lib
	makeindex lib
	latex lib

qua:
	latex qua
	latex qua
	bibtex qua
	latex qua

clean:
	rm -f *~ *.aux *.dvi *.idx *.ilg *.ind *.log *.ps *.toc
	# Keep: .tex, .bib, .sty!
